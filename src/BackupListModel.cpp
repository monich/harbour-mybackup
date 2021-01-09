/*
 * Copyright (C) 2020 Jolla Ltd.
 * Copyright (C) 2020 Slava Monich <slava@monich.com>
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

#include "BackupListModel.h"
#include "BackupList.h"

#include "HarbourTask.h"
#include "HarbourDebug.h"

#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>

Q_STATIC_ASSERT((int)BackupListModel::App == (int)BackupList::Item::App);
Q_STATIC_ASSERT((int)BackupListModel::Path == (int)BackupList::Item::Path);
Q_STATIC_ASSERT((int)BackupListModel::Config == (int)BackupList::Item::Config);

// ==========================================================================
// BackupListModel::Private
// ==========================================================================

class BackupListModel::Private : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(Private)

public:
    Private(BackupListModel* aParent);
    ~Private();

    BackupListModel* parentModel();
    void saveModel();
    bool updateApps();

private:
    void writeState();

private Q_SLOTS:
    void flushChanges();
    void onSaveTimerExpired();
    void onConfigFileChanged(QString aPath);
    void onConfigDirectoryChanged(QString aPath);

public:
    bool iConfigured;
    const QString iConfigFile;
    QStringList iApps;
    BackupList iList;
    QTimer* iSaveTimer;
    QTimer* iHoldoffTimer;
    QFileSystemWatcher* iConfigFileWatcher;
    int iIgnoreConfigFileChange;
};

BackupListModel::Private::Private(BackupListModel* aParent) :
    QObject(aParent),
    iConfigured(true),
    iConfigFile(BackupList::defaultConfigFile()),
    iSaveTimer(new QTimer(this)),
    iHoldoffTimer(new QTimer(this)),
    iConfigFileWatcher(new QFileSystemWatcher(this)),
    iIgnoreConfigFileChange(0)
{
    // Current state is saved at least every 10 seconds
    iSaveTimer->setInterval(10000);
    iSaveTimer->setSingleShot(true);
    connect(iSaveTimer, SIGNAL(timeout()), SLOT(onSaveTimerExpired()));
    // And not more often than every second
    iHoldoffTimer->setInterval(1000);
    iHoldoffTimer->setSingleShot(true);
    connect(iHoldoffTimer, SIGNAL(timeout()), SLOT(flushChanges()));
    // Load the state
    HDEBUG("Loading" << qPrintable(iConfigFile));
    iList.load(iConfigFile);
    updateApps();

    // Watch the config file changes
    QFileInfo configFile(iConfigFile);
    QDir configFileDir(configFile.dir());
    configFileDir.mkpath(QStringLiteral("."));
    connect(iConfigFileWatcher, SIGNAL(fileChanged(QString)),
        SLOT(onConfigFileChanged(QString)));
    connect(iConfigFileWatcher, SIGNAL(directoryChanged(QString)),
        SLOT(onConfigDirectoryChanged(QString)));
    iConfigFileWatcher->addPath(configFileDir.absolutePath());
    if (!iConfigFileWatcher->files().contains(iConfigFile)) {
        HDEBUG("Watching" << qPrintable(iConfigFile));
        iConfigFileWatcher->addPath(iConfigFile);
    }
}

BackupListModel::Private::~Private()
{
    flushChanges();
}

inline BackupListModel* BackupListModel::Private::parentModel()
{
    return qobject_cast<BackupListModel*>(parent());
}

void BackupListModel::Private::onConfigDirectoryChanged(QString aPath)
{
    HDEBUG(qPrintable(aPath));
    if (QFile::exists(iConfigFile)) {
        if (!iConfigFileWatcher->files().contains(iConfigFile)) {
            HDEBUG("Watching" << qPrintable(iConfigFile));
            iConfigFileWatcher->addPath(iConfigFile);
        }
    }
}

void BackupListModel::Private::onConfigFileChanged(QString aPath)
{
    if (iIgnoreConfigFileChange) {
        iIgnoreConfigFileChange--;
        HDEBUG("Ignoring" << qPrintable(aPath));
    } else {
        HDEBUG(qPrintable(aPath));
        BackupList list(iConfigFile);
        const uint diff = iList.diff(list);
        if (diff) {
            BackupListModel* model = parentModel();
            if (diff & BackupList::DiffItems) {
                HDEBUG("Configuration has changed");
                model->beginResetModel();
                iList.swap(list);
                if (updateApps()) {
                    Q_EMIT model->appsChanged();
                }
                model->endResetModel();
            } else {
                // Still swap but without resetting the list view
                iList.swap(list);
            }
            if (diff & BackupList::DiffLastBackup) {
                HDEBUG("Last backup date changed to" <<
                    qPrintable(iList.lastBackup().toLocalTime().toString()));
                Q_EMIT model->lastBackupChanged();
            }
            if (diff & BackupList::DiffLastRestore) {
                HDEBUG("Last restore date changed to" <<
                    qPrintable(iList.lastRestore().toLocalTime().toString()));
                Q_EMIT model->lastRestoreChanged();
            }
        }
    }
}

bool BackupListModel::Private::updateApps()
{
    QStringList apps;
    const int n = iList.count();
    for (int i = 0; i < n; i++) {
        const BackupList::Item* item = iList.itemAt(i);
        if (item->type() == BackupList::Item::App) {
            apps.append(item->path());
        }
    }
    apps.sort();
    if (iApps != apps) {
        iApps = apps;
        HDEBUG(apps);
        return true;
    } else {
        return false;
    }
}

void BackupListModel::Private::saveModel()
{
    if (!iHoldoffTimer->isActive()) {
        // Idle state, write the file right away
        iSaveTimer->stop();
        writeState();
    } else {
        // Make sure it eventually gets saved even if changes will
        // keep happening in quick succession
        if (!iSaveTimer->isActive()) {
            iSaveTimer->start();
        }
    }
    // Restart hold off timer
    iHoldoffTimer->start();
}

void BackupListModel::Private::flushChanges()
{
    if (iSaveTimer->isActive()) {
        iSaveTimer->stop();
        writeState();
    }
}

void BackupListModel::Private::onSaveTimerExpired()
{
    iHoldoffTimer->start();
    writeState();
}

void BackupListModel::Private::writeState()
{
    HDEBUG("Writing" << qPrintable(iConfigFile));
    iIgnoreConfigFileChange++;
    iList.save(iConfigFile);
}

// ==========================================================================
// BackupListModel
// ==========================================================================

#define SUPER QAbstractListModel

BackupListModel::BackupListModel(QObject* aParent) :
    SUPER(aParent),
    iPrivate(new Private(this))
{
}

BackupListModel::~BackupListModel()
{
    delete iPrivate;
}

// Callback for qmlRegisterSingletonType<BackupListModel>
QObject* BackupListModel::createSingleton(QQmlEngine*, QJSEngine*)
{
    return new BackupListModel();
}

bool BackupListModel::configured() const
{
    return iPrivate->iConfigured;
}

QDateTime BackupListModel::lastBackup() const
{
    return iPrivate->iList.lastBackup();
}

QDateTime BackupListModel::lastRestore() const
{
    return iPrivate->iList.lastRestore();
}

QStringList BackupListModel::apps() const
{
    return iPrivate->iApps;
}

QHash<int,QByteArray> BackupListModel::roleNames() const
{
    QHash<int,QByteArray> roles;
#define ROLE(X,x) roles.insert(BackupList::Item::X##Role, #x);
BACKUP_LIST_ITEM_ROLES(ROLE)
#undef ROLE
    return roles;
}

int BackupListModel::rowCount(const QModelIndex& aParent) const
{
    return iPrivate->iList.count();
}

QVariant BackupListModel::data(const QModelIndex& aIndex, int aRole) const
{
    const BackupList::Item* item = iPrivate->iList.itemAt(aIndex.row());
    return item ? item->get((BackupList::Item::Role)aRole) : QVariant();
}

bool BackupListModel::moveRows(const QModelIndex &aSrcParent, int aSrcRow,
    int aCount, const QModelIndex &aDestParent, int aDestRow)
{
    const int size = iPrivate->iList.count();
    if (aSrcParent == aDestParent &&
        aSrcRow != aDestRow &&
        aSrcRow >= 0 && aSrcRow < size &&
        aDestRow >= 0 && aDestRow < size) {
        HDEBUG(aSrcRow << "->" << aDestRow);
        beginMoveRows(aSrcParent, aSrcRow, aSrcRow, aDestParent,
           (aDestRow < aSrcRow) ? aDestRow : (aDestRow + 1));
        iPrivate->iList.moveItem(aSrcRow, aDestRow);
        iPrivate->saveModel();
        endMoveRows();
        return true;
    } else {
        return false;
    }
}

void BackupListModel::add(Type aType, QStringList aPaths)
{
    HDEBUG(aType << aPaths);
    const int n = aPaths.count();
    if (n > 0) {
        const BackupList::Item::Type type = (BackupList::Item::Type)aType;
        for (int i = 0; i < n; i++) {
            BackupList::Item* item = BackupList::Item::create(type, aPaths.at(i));
            if (item) {
                // Could be optimized to add all new items at once
                // but it's not worth the trouble because they are
                // usually being added one by one.
                const int k = iPrivate->iList.count();
                beginInsertRows(QModelIndex(), k, k);
                iPrivate->iList.appendItem(item);
                iPrivate->saveModel();
                if (iPrivate->updateApps()) {
                    Q_EMIT appsChanged();
                }
                endInsertRows();
            }
        }
    }
}

void BackupListModel::removeAt(int aRow)
{
    HDEBUG(aRow);
    if (aRow >= 0 && aRow < iPrivate->iList.count()) {
        beginRemoveRows(QModelIndex(), aRow, aRow);
        iPrivate->iList.removeItemAt(aRow);
        iPrivate->saveModel();
        if (iPrivate->updateApps()) {
            Q_EMIT appsChanged();
        }
        endRemoveRows();
    }
}

#include "BackupListModel.moc"
