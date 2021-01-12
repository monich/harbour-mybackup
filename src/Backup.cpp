/*
 * Copyright (C) 2020-2021 Jolla Ltd.
 * Copyright (C) 2020-2021 Slava Monich <slava@monich.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *   3. Neither the names of the copyright holders nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * any official policies, either expressed or implied.
 */

// In many cases gio is more convenient/functional than Qt :/
// It must be included before Qt headers to avoid collisions.
#include <gio/gio.h>

#include "Backup.h"
#include "BackupList.h"
#include "BackupUtil.h"
#include "ConfigClient.h"

#include "HarbourJson.h"
#include "HarbourDebug.h"

#include <MGConfItem>

#include <QDir>
#include <QVariantMap>
#include <QVariantList>

#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

const char Backup::ACTION_IMPORT[] = "import";
const char Backup::ACTION_RESTORE[] = "restore";
const char Backup::ACTION_EXPORT[] = "export";

namespace {
    const QString DOT(".");
    const QString DOTDOT("..");
    const QString SLASH("/");
    const QString USERS_DIR("users");
    const QString FILES_DIR("files");

    //
    // config.json contains two lists:
    //
    // 1. "keys" - array of "name" (starting but not ending with slash) and
    //    "value" pairs.
    // 2. "groups" - each entry has "name" (configuration group name starting
    //    and ending with slash) and optionally "keys" (see above, only names
    //    are not starting with slash this time) and another "groups" list,
    //    again containing "name" (this time ending but not starting with slash)
    //    entries, "keys", "groups" and so on, recursively.
    //
    // When DConf entries are being restored, the first kind of entries
    // replaces (or creates) a single key. The second variant completely
    // replaces the entire tree of DConf groups, deleting all the existing
    // entries, only leaving keys and subgroups found in the backup.
    //
    const QString CONFIG_STORE("config.json");
    const QString CONFIG_GROUPS("groups");
    const QString CONFIG_KEYS("keys");
    const QString CONFIG_NAME("name");
    const QString CONFIG_VALUE("value");
}

// ==========================================================================
// Backup::Private
// ==========================================================================

class Backup::Private {
public:
    static QDir backupFilesDir(const QString aBackupRoot);
    static QString backupUserRoot(const QString aBackupRoot);
    static QString backupConfigStore(const QString aBackupRoot);
    static void copyFile(const char* aDestFile, const char* aSrcFile);
    static void copyDir(const char* aDestDir, const char* aSrcDir);
    static void copyFiles(QDir destDir, QDir srcDir, const QString aEntry);
    static void copyFiles(QDir destDir, QDir srcDir, const QStringList aList);
};

QString Backup::Private::backupUserRoot(const QString aBackupRoot)
{
    return aBackupRoot + QDir::separator() +
        USERS_DIR + QDir::separator() +
        QString::number(geteuid()) + QDir::separator();

}

QDir Backup::Private::backupFilesDir(const QString aBackupRoot)
{
    return QDir(backupUserRoot(aBackupRoot) + FILES_DIR);
}

QString Backup::Private::backupConfigStore(const QString aBackupRoot)
{
    return backupUserRoot(aBackupRoot) + CONFIG_STORE;
}

void Backup::Private::copyFile(const char* aDestFile, const char* aSrcFile)
{
    GError* error = NULL;
    GFile* src = g_file_new_for_path(aSrcFile);
    GFile* dest = g_file_new_for_path(aDestFile);
    GFileCopyFlags flags = (GFileCopyFlags)(G_FILE_COPY_OVERWRITE |
        G_FILE_COPY_ALL_METADATA);
    char* srcPath = g_file_get_path(src);
    char* destPath = g_file_get_path(dest);

    // First try to create a hard link because it's so much faster
    if (link(srcPath, destPath) == 0) {
        HDEBUG(srcPath << "->" << destPath);
    } else if (g_file_copy(src, dest, flags, NULL, NULL, NULL, &error)) {
        HDEBUG(srcPath << "=>" << destPath);
    } else {
        if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND)) {
            // Caller checks if the source exists but doesn't necessarily
            // check if the destination directory exists. Try to create
            // the destination directory and retry the attempt.
            GFile* srcDir = g_file_get_parent(src);
            char* srcDirPath = g_file_get_path(srcDir);
            struct stat st;
            if (!stat(srcDirPath, &st) && S_ISDIR(st.st_mode)) {
                GFile* destDir = g_file_get_parent(dest);
                char* destDirPath = g_file_get_path(destDir);
                const mode_t mode = st.st_mode & ~S_IFMT;
                if (!g_mkdir_with_parents(destDirPath, mode)) {
                    // Try to copy ownership and mode
                    if (chown(destDirPath, st.st_uid, st.st_gid)) {
                        HWARN("Failed to chown" << destDirPath << ":" <<
                            strerror(errno));
                    }
                    if (chmod(destDirPath, mode)) {
                        HWARN("Failed to chmod" << destDirPath << ":" <<
                            strerror(errno));
                    }
                    g_error_free(error);
                    error = NULL;
                    // And make another attempt to copy the file
                    HDEBUG("Created" << destDirPath);
                    if (g_file_copy(src, dest, flags, NULL, NULL, NULL, &error)) {
                        HDEBUG(srcPath << "=>" << destPath);
                    }
                } else {
                    HWARN("Failed to create directory" << destDirPath <<
                        ":" << strerror(errno));
                }
                g_free(destDirPath);
                g_object_unref(destDir);
            }
            g_free(srcDirPath);
            g_object_unref(srcDir);
        }
        if (error) {
            HWARN(error->message);
            g_error_free(error);
        }
    }

    g_free(srcPath);
    g_free(destPath);
    g_object_unref(src);
    g_object_unref(dest);
}

void Backup::Private::copyDir(const char* aDestDir, const char* aSrcDir)
{
    // Make sure that the source is a directory
    struct stat st;
    if (!stat(aSrcDir, &st) && S_ISDIR(st.st_mode)) {
        // Create the destination directory if necessary
        bool destDirExists = g_file_test(aDestDir, G_FILE_TEST_IS_DIR);
        if (!destDirExists) {
            const mode_t mode = st.st_mode & ~S_IFMT;
            remove(aDestDir); // In case if there's file with the same name
            if (g_mkdir_with_parents(aDestDir, mode) == 0) {
                // Try to copy ownership and mode
                if (chown(aDestDir, st.st_uid, st.st_gid)) {
                    HWARN("Failed to chown" << aDestDir << ":" <<
                        strerror(errno));
                }
                if (chmod(aDestDir, mode)) {
                    HWARN("Failed to chmod" << aDestDir << ":" <<
                        strerror(errno));
                }
                destDirExists = true;
                HDEBUG("Created" << aDestDir);
            } else {
                HWARN("Failed to create directory" << aDestDir << ":" <<
                    strerror(errno));
            }
        }
        if (destDirExists) {
            // Copy the contents
            GDir* dir = g_dir_open(aSrcDir, 0, NULL);
            if (dir) {
                const char* name;
                while ((name = g_dir_read_name(dir)) != NULL) {
                    char* src = g_build_filename(aSrcDir, name, NULL);
                    if (!stat(src, &st)) {
                        char* dest = g_build_filename(aDestDir, name, NULL);
                        // Be slightly paranoid :)
                        if (strcmp(src, dest)) {
                            if (S_ISREG(st.st_mode)) {
                                copyFile(dest, src);
                            } else {
                                copyDir(dest, src);
                            }
                        }
                        g_free(dest);
                    }
                    g_free(src);
                }
                g_dir_close(dir);
            }
        }
    } else {
        HWARN("Skipping" << aSrcDir);
    }
}

void Backup::Private::copyFiles(QDir aDestDir, QDir aSrcDir,
    const QString aEntry)
{
    // BackupList::backupFileList makes sure that paths are relative
    // to the home directory. Caller makes sure that the destination
    // directory exists.
    bool copyTree = false;
    QString entry(aEntry);
    if (entry.endsWith(QDir::separator())) {
        copyTree = true;
        do { entry = entry.left(entry.length() - 1); }
        while (entry.endsWith(QDir::separator()));
    }
    QFileInfo srcInfo(aSrcDir, entry);
    const QByteArray srcPath(srcInfo.absoluteFilePath().toLocal8Bit());
    if (srcInfo.exists()) {
        QFileInfo destInfo(aDestDir, entry);
        const QByteArray destPath(destInfo.absoluteFilePath().toLocal8Bit());
        if (copyTree) {
            if (srcInfo.isDir()) {
                // Copy directory tree
                copyDir(destPath.constData(), srcPath.constData());
            } else {
                HWARN(srcPath.constData() << "is not a directory");
            }
        } else {
            if (srcInfo.isFile()) {
                copyFile(destPath.constData(), srcPath.constData());
            } else {
                HWARN(srcPath.constData() << "is not a file");
            }
        }
    } else {
        // This is not an error, just skip it non-existing ones
        HDEBUG(srcPath.constData() << "doesn't exist");
    }
}

void Backup::Private::copyFiles(QDir aDestDir, QDir aSrcDir, const QStringList aList)
{
    const int n = aList.count();
    for (int i = 0; i < n; i++) {
        copyFiles(aDestDir, aSrcDir, aList.at(i));
    }
}

// ==========================================================================
// Backup::Import
// ==========================================================================

class Backup::Import {
public:
    static void restoreSubGroups(const QString aPrefix, const QVariantList aSubGroups);
    static void restoreSubKeys(const QString aPrefix, const QVariantList aSubKeys);
    static void restoreGroups(const QVariantList aGroups);
    static void restoreKey(const QString aKey, const QVariant aValue);
    static void restoreKeys(const QVariantList aKeys);
    static void restoreConfig(const QString aBackupRoot, const QStringList aConfigList);
    static void restore(const QString aHome, const QString aBackupRoot,
        const QStringList aFileList, const QStringList aConfigList);
};

void Backup::Import::restoreSubGroups(const QString aPrefix,
    const QVariantList aSubGroups)
{
    const int n = aSubGroups.count();
    for (int i = 0; i < n; i++) {
        const QVariantMap entry(aSubGroups.at(i).toMap());
        const QString subgroup(entry.value(CONFIG_NAME).toString());
        if (!subgroup.startsWith('/') && subgroup.endsWith('/')) {
            const QString group(aPrefix + subgroup);
            restoreSubGroups(group, entry.value(CONFIG_GROUPS).toList());
            restoreSubKeys(group, entry.value(CONFIG_KEYS).toList());
        } else {
            HWARN("Ignoring configuration subgroup" << subgroup);
        }
    }
}

void Backup::Import::restoreKey(const QString aKey, const QVariant aValue)
{
    if (aValue.isValid()) {
        HDEBUG(aKey << "=" << aValue);
        MGConfItem item(aKey);
        item.set(aValue);
        item.sync();
    }
}

void Backup::Import::restoreSubKeys(const QString aPrefix,
    const QVariantList aSubKeys)
{
    const int n = aSubKeys.count();
    for (int i = 0; i < n; i++) {
        const QVariantMap entry(aSubKeys.at(i).toMap());
        const QString subkey(entry.value(CONFIG_NAME).toString());
        if (!subkey.startsWith('/') && !subkey.endsWith('/')) {
            restoreKey(aPrefix + subkey, entry.value(CONFIG_VALUE));
        } else {
            HWARN("Ignoring configuration subkey" << subkey);
        }
    }
}

void Backup::Import::restoreGroups(const QVariantList aGroups)
{
    const int n = aGroups.count();
    if (n > 0) {
        ConfigClient dconf(ConfigClient::create());
        for (int i = 0; i < n; i++) {
            const QVariantMap entry(aGroups.at(i).toMap());
            const QString group(entry.value(CONFIG_NAME).toString());
            if (group.startsWith('/') && group.endsWith('/')) {
                MGConfItem item(group);
                item.set(QVariant()); // Clear the group
                item.sync();
                restoreSubGroups(group, entry.value(CONFIG_GROUPS).toList());
                restoreSubKeys(group, entry.value(CONFIG_KEYS).toList());
            } else {
                HWARN("Ignoring configuration group" << group);
            }
        }
        dconf.sync();
    }
}

void Backup::Import::restoreKeys(const QVariantList aKeys)
{
    const int n = aKeys.count();
    if (n > 0) {
        ConfigClient dconf(ConfigClient::create());
        for (int i = 0; i < n; i++) {
            const QVariantMap entry(aKeys.at(i).toMap());
            const QString key(entry.value(CONFIG_NAME).toString());
            if (key.startsWith('/') && !key.endsWith('/')) {
                restoreKey(key, entry.value(CONFIG_VALUE));
            } else {
                HWARN("Ignoring configuration key" << key);
            }
        }
        dconf.sync();
    }
}

void Backup::Import::restoreConfig(const QString aBackupRoot,
    const QStringList aList)
{
    QVariantMap data;
    if (HarbourJson::load(Private::backupConfigStore(aBackupRoot), data)) {
        restoreGroups(data.value(CONFIG_GROUPS).toList());
        restoreKeys(data.value(CONFIG_KEYS).toList());
    }
}

void Backup::Import::restore(const QString aHome, const QString aBackupRoot,
    const QStringList aFileList, const QStringList aConfigList)
{
    HDEBUG("Restoring files" << aBackupRoot << "=>" << aHome);
    Private::copyFiles(QDir(aHome), Private::backupFilesDir(aBackupRoot), aFileList);
    restoreConfig(aBackupRoot, aConfigList);
}

// ==========================================================================
// Backup::Export
// ==========================================================================

class Backup::Export {
public:
    static void storeKey(QVariantList* aList, const QString aGroup, const QString aName);
    static QVariantMap groupEntry(const QString aName, const QVariantList aGroups, const QVariantList aKeys);
    static QVariantMap storeGroup(ConfigClient aClient, const QString aParent, const QString aGroup);
    static QVariantMap storeConfig(const QStringList aConfigList);
    static void backupConfig(const QString aBackupRoot, const QStringList aConfigList);
    static void backup(const QString aHome, const QString aBackupRoot,
        const QStringList aFileList, const QStringList aConfigList);
};

QVariantMap Backup::Export::groupEntry(const QString aName,
    const QVariantList aGroups, const QVariantList aKeys)
{
    QVariantMap group;
    if (!aGroups.isEmpty()) {
        group.insert(CONFIG_GROUPS, aGroups);
    }
    if (!aKeys.isEmpty()) {
        group.insert(CONFIG_KEYS, aKeys);
    }
    if (!group.isEmpty() && !aName.isEmpty()) {
        group.insert(CONFIG_NAME, aName);
    }
    return group;
}

void Backup::Export::storeKey(QVariantList* aList,
    const QString aGroup, const QString aName)
{
    const QString path(aGroup + aName);
    MGConfItem item(path);
    const QVariant value(item.value());
    if (value.isValid()) {
        QVariantMap key;
        key.insert(CONFIG_NAME, aName);
        key.insert(CONFIG_VALUE, value);
        aList->append(key);
        HDEBUG(path << "=" << value);
    } else {
        HDEBUG(path << "doesn't exist");
    }
}

QVariantMap Backup::Export::storeGroup(ConfigClient aClient,
    const QString aParent, const QString aGroup)
{
    QVariantList subgroups, subkeys;
    const QString path(aParent + aGroup);
    const QStringList entries(aClient.list(path));
    const int n = entries.count();
    for (int i = 0; i < n; i++) {
        const QString name(entries.at(i));
        if (name.endsWith('/')) {
            HDEBUG("Group" << (path + name));
            const QVariantMap subgroup(storeGroup(aClient, path, name));
            if (!subgroup.isEmpty()) {
                subgroups.append(subgroup);
            }
        } else {
            storeKey(&subkeys, path, name);
        }
    }
    return groupEntry(aGroup, subgroups, subkeys);
}

QVariantMap Backup::Export::storeConfig(const QStringList aConfigList)
{
    QVariantList groups, keys;
    ConfigClient dconf(ConfigClient::create());
    const int n = aConfigList.count();
    for (int i = 0; i < n; i++) {
        const QString name(aConfigList.at(i));
        if (name.startsWith('/')) {
            if (name.endsWith('/')) {
                HDEBUG("Group" << name);
                const QVariantMap group(storeGroup(dconf, QString(), name));
                if (!group.isEmpty()) {
                    groups.append(group);
                }
            } else {
                storeKey(&keys, QString(), name);
            }
        } else {
            HWARN("Ignoring configuration entry" << name);
        }
    }
    return groupEntry(QString(), groups, keys);
}

void Backup::Export::backupConfig(const QString aBackupRoot,
    const QStringList aConfigList)
{
    const QString file(Private::backupConfigStore(aBackupRoot));
    const QVariantMap config(storeConfig(aConfigList));
    HDEBUG("Writing" << qPrintable(file));
    HarbourJson::save(file, config);
}

void Backup::Export::backup(const QString aHome, const QString aBackupRoot,
    const QStringList aFileList, const QStringList aConfigList)
{
    HDEBUG("Backing up files" << aHome << "=>" << aBackupRoot);
    Private::copyFiles(Private::backupFilesDir(aBackupRoot), QDir(aHome), aFileList);
    backupConfig(aBackupRoot, aConfigList);
}

// ==========================================================================
// Backup
// ==========================================================================

int Backup::run(Action aAction, const char* aHome, const char* aBackupRoot)
{
    const QString configDir(BackupList::configDir() + QDir::separator());
    const QString configFile(BackupList::defaultConfigFile());
    const QString configDirRel(BackupUtil::relativeToHome(configDir));
    const QString configFileRel(BackupUtil::relativeToHome(configFile));
    BackupList backup;
    switch (aAction) {
    case ImportAction:
        // Load backup configuration from the backup
        backup.load(QFileInfo(Private::backupFilesDir(aBackupRoot),
            configFileRel).absoluteFilePath());
        Import::restore(QString::fromLocal8Bit(aHome),
            QString::fromLocal8Bit(aBackupRoot),
            backup.backupFileList(configDirRel),
            backup.backupConfigList(QString()));
        backup.updateLastRestore();
        backup.save(configFile);
        break;
    case ExportAction:
        backup.load(configFile);
        backup.updateLastBackup();
        backup.save(configFile);
        Export::backup(QString::fromLocal8Bit(aHome),
            QString::fromLocal8Bit(aBackupRoot),
            backup.backupFileList(configDirRel),
            backup.backupConfigList(QString()));
        break;
    case NoAction:
        break;
    }
    return 0;
}
