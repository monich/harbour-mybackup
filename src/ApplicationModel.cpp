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

#include "ApplicationModel.h"

#include "BackupUtil.h"

#include "HarbourTask.h"
#include "HarbourDebug.h"

#include <QThreadPool>
#include <QImage>
#include <QDir>

#include <glib.h>

#define MODEL_ROLES_(first,role,last) \
    first(Name,name) \
    role(DesktopFile,desktopFile) \
    role(AppIcon,appIcon) \
    last(KnownApp,knownApp)

#define MODEL_ROLES(role) \
    MODEL_ROLES_(role,role,role)

// ==========================================================================
// ApplicationModel::ModelData
// ==========================================================================

class ApplicationModel::ModelData {
public:
    class RefreshTask;

    enum Role {
#define FIRST(X,x) FirstRole = Qt::UserRole, X##Role = FirstRole,
#define ROLE(X,x) X##Role,
#define LAST(X,x) X##Role, LastRole = X##Role
        MODEL_ROLES_(FIRST,ROLE,LAST)
#undef FIRST
#undef ROLE
#undef LAST
    };

    typedef QList<ModelData*> List;

    ModelData(QString aDesktopFile, const AppInfo* aAppInfo);

    QVariant get(Role aRole) const;

public:
    const QString iDesktopFile;
    const QString iAppName;
    const QString iAppIcon;
    bool iKnownApp;
};

ApplicationModel::ModelData::ModelData(QString aDesktopFile, const AppInfo* aAppInfo) :
    iDesktopFile(aDesktopFile), iAppName(aAppInfo->iAppName),
    iAppIcon(aAppInfo->iconUrl()),
    iKnownApp(false)
{
}

QVariant ApplicationModel::ModelData::get(Role aRole) const
{
    switch (aRole) {
    case DesktopFileRole:
        return iDesktopFile;
    case NameRole:
        return iAppName;
    case AppIconRole:
        return iAppIcon;
    case KnownAppRole:
        return iKnownApp;
    }
    return QVariant();
}

// ==========================================================================
// ApplicationModel::ModelData::RefreshTask
// ==========================================================================

static const char DESKTOP_GROUP[] = G_KEY_FILE_DESKTOP_GROUP;
static const char BACKUP_GROUP[] = "X-HarbourBackup";
static const char BACKUP_KEY_PATH_LIST[] = "BackupPathList";
static const char BACKUP_KEY_CONFIG_LIST[] = "BackupConfigList";
static const char CONFIG_LIST_SEPARATORS[] = ";:,";

class ApplicationModel::ModelData::RefreshTask : public HarbourTask {
    Q_OBJECT
public:
    RefreshTask(QThreadPool* aPool);
    ~RefreshTask();

    void performTask() Q_DECL_OVERRIDE;
public:
    List iData;
};

ApplicationModel::ModelData::RefreshTask::RefreshTask(QThreadPool* aPool) :
    HarbourTask(aPool)
{
}

ApplicationModel::ModelData::RefreshTask::~RefreshTask()
{
    qDeleteAll(iData);
}

void ApplicationModel::ModelData::RefreshTask::performTask()
{
    const QStringList filter("*.desktop");
    QDir appsDir("/usr/share/applications");
    QFileInfoList desktopFiles(appsDir.entryInfoList(filter, QDir::Files));

    const int n = desktopFiles.count();
    for (int i = 0; i < n; i++) {
        const QFileInfo& info = desktopFiles.at(i);
        const QString desktopFile(info.absoluteFilePath());
        AppInfo* appInfo = parseDesktopFile(desktopFile);
        if (appInfo) {
            iData.append(new ModelData(desktopFile, appInfo));
            delete appInfo;
        }
    }
}

// ==========================================================================
// ApplicationModel::Private
// ==========================================================================

class ApplicationModel::Private: public QObject {
    Q_OBJECT

public:
    Private(ApplicationModel* aParent);
    ~Private();

    static QString iconUrl(QString aIconName);
    ModelData* dataAt(int aIndex) const;
    ApplicationModel* parentModel() const;
    int rowCount() const;

public Q_SLOTS:
    void onRefreshTaskDone();

public:
    QThreadPool* iThreadPool;
    ModelData::RefreshTask* iRefreshTask;
    ModelData::List iData;
    QStringList iKnownApps;
};

ApplicationModel::Private::Private(ApplicationModel* aParent) :
    QObject(aParent),
    iThreadPool(new QThreadPool(this)),
    iRefreshTask(new ModelData::RefreshTask(iThreadPool))
{
    iRefreshTask->submit(this, SLOT(onRefreshTaskDone()));
}

ApplicationModel::Private::~Private()
{
    if (iRefreshTask) iRefreshTask->release();
    iThreadPool->waitForDone();
    delete iThreadPool;
    qDeleteAll(iData);
}

inline ApplicationModel* ApplicationModel::Private::parentModel() const
{
    return qobject_cast<ApplicationModel*>(parent());
}

inline int ApplicationModel::Private::rowCount() const
{
    return iData.count();
}

ApplicationModel::ModelData* ApplicationModel::Private::dataAt(int aIndex) const
{
    if (aIndex >= 0 && aIndex < iData.count()) {
        return iData.at(aIndex);
    } else {
        return NULL;
    }
}

void ApplicationModel::Private::onRefreshTaskDone()
{
    HDEBUG("Refresh task done");
    if (sender() == iRefreshTask) {
        const ModelData::List data(iRefreshTask->iData);
        iRefreshTask->iData.clear();
        iRefreshTask->release();
        iRefreshTask = NULL;

        // Update the model
        ApplicationModel* model = parentModel();
        if (!data.isEmpty()) {
            model->beginResetModel();
            iData = data;
            const int n = data.count();
            for (int i = 0; i < n; i++) {
                ModelData* app = iData.at(i);
                app->iKnownApp = iKnownApps.contains(app->iDesktopFile);
            }
            model->endResetModel();
        }
        Q_EMIT model->readyChanged();
    }
}

QString ApplicationModel::Private::iconUrl(QString aIconName)
{
    QDir hicolor("/usr/share/icons/hicolor");
    QFileInfoList subdirs(hicolor.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot));
    const int n = subdirs.count();
    QImage bestImage;
    QString bestPath;
    for (int i = 0; i < n; i++) {
        QImage image;
        const QFileInfo& dir = subdirs.at(i);
        const QString path(dir.absoluteFilePath() + "/apps/" + aIconName + ".png");
        if (image.load(path) && !image.isNull()) {
            if (bestImage.isNull() || image.width() > bestImage.width()) {
                HDEBUG(image);
                bestImage = image;
                bestPath = path;
            }
        }
    }
    if (bestImage.isNull()) {
        // Assume a system-provided icon
        bestPath = QString::fromLatin1("image://theme/") + aIconName;
        HDEBUG("Assuming" << qPrintable(bestPath));
    } else {
        bestPath = QString::fromLatin1("file://") + bestPath;
    }
    return bestPath;
}

// ==========================================================================
// ApplicationModel::AppInfo
// ==========================================================================

QString ApplicationModel::AppInfo::iconUrl() const
{
    return Private::iconUrl(iAppIcon);
}

// ==========================================================================
// ApplicationModel
// ==========================================================================

#define SUPER QAbstractListModel

ApplicationModel::ApplicationModel(QObject* aParent) :
    SUPER(aParent),
    iPrivate(new Private(this))
{
}

ApplicationModel::~ApplicationModel()
{
    delete iPrivate;
}

bool ApplicationModel::ready() const
{
    return !iPrivate->iRefreshTask;
}

QStringList ApplicationModel::knownApps() const
{
    return iPrivate->iKnownApps;
}

void ApplicationModel::setKnownApps(QStringList aApps)
{
    if (iPrivate->iKnownApps != aApps) {
        iPrivate->iKnownApps = aApps;
        Q_EMIT knownAppsChanged();
    }
}

QStringList ApplicationModel::desktopFiles(QList<int> aRows) const
{
    QStringList out;
    const int nrows = aRows.count();
    for (int i = 0; i < nrows; i++) {
        ModelData* data = iPrivate->dataAt(aRows.at(i));
        if (data) {
            out.append(data->iDesktopFile);
        }
    }
    return out;
}

QHash<int,QByteArray> ApplicationModel::roleNames() const
{
    QHash<int,QByteArray> roles;
#define ROLE(X,x) roles.insert(ModelData::X##Role, #x);
MODEL_ROLES(ROLE)
#undef ROLE
    return roles;
}

int ApplicationModel::rowCount(const QModelIndex& aParent) const
{
    return iPrivate->rowCount();
}

QVariant ApplicationModel::data(const QModelIndex& aIndex, int aRole) const
{
    ModelData* data = iPrivate->dataAt(aIndex.row());
    return data ? data->get((ModelData::Role)aRole) : QVariant();
}

ApplicationModel::AppInfo* ApplicationModel::parseDesktopFile(QFileInfo aDesktopFile)
{
    AppInfo* appInfo = Q_NULLPTR;
    const QString path(aDesktopFile.absoluteFilePath());
    const QByteArray path8bit(path.toLocal8Bit());
    GKeyFile* keyfile = g_key_file_new();
    if (g_key_file_load_from_file(keyfile, path8bit, G_KEY_FILE_NONE, NULL) &&
        g_key_file_has_group(keyfile, BACKUP_GROUP)) {
        appInfo = new AppInfo;
        HDEBUG("Checking" << path8bit.constData());

        // App name
        char* name = g_key_file_get_locale_string(keyfile, DESKTOP_GROUP,
            G_KEY_FILE_DESKTOP_KEY_NAME, NULL, NULL);
        if (name) {
            appInfo->iAppName = QString::fromUtf8(name);
            g_free(name);
        }
        if (appInfo->iAppName.isEmpty()) {
            appInfo->iAppName = aDesktopFile.baseName();
        }

        // App icon
        char* icon = g_key_file_get_locale_string(keyfile, DESKTOP_GROUP,
            G_KEY_FILE_DESKTOP_KEY_ICON, NULL, NULL);
        if (icon) {
            appInfo->iAppIcon = QLatin1String(icon);
            g_free(icon);
        }

        // Backup paths
        char* sval = g_key_file_get_string(keyfile, BACKUP_GROUP,
            BACKUP_KEY_PATH_LIST, NULL);
        if (sval) {
            char** list = g_strsplit_set(sval, CONFIG_LIST_SEPARATORS, -1);
            char** ptr = list;

            while (*ptr) {
                // Only leave paths relative to home. We don't (yet) backup
                // absolute paths as those are typically readonly for the
                // backup process and therefore can't be restored.
                const QString relPath(BackupUtil::relativeToHome
                    (QString::fromLocal8Bit(*ptr++)));
                if (!relPath.isEmpty()) {
                    appInfo->iBackupPathList.append(relPath);
                }
            }
            g_strfreev(list);
            g_free(sval);
        }

        // Backup dconf entries
        sval = g_key_file_get_string(keyfile, BACKUP_GROUP,
            BACKUP_KEY_CONFIG_LIST, NULL);
        if (sval) {
            char** list = g_strsplit_set(sval, CONFIG_LIST_SEPARATORS, -1);
            char** ptr = list;

            while (*ptr) {
                appInfo->iBackupConfigList.append(QString::fromLocal8Bit(*ptr++));
            }
            g_strfreev(list);
            g_free(sval);
        }
    }
    g_key_file_free(keyfile);
    return appInfo;
}

#include "ApplicationModel.moc"
