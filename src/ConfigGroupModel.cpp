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

#include "ConfigGroupModel.h"
#include "ConfigClient.h"

#include "HarbourTask.h"
#include "HarbourDebug.h"

#include <QWeakPointer>
#include <QSharedPointer>
#include <QThreadPool>
#include <QList>

#define MODEL_ROLES_(first,role,last) \
    first(Type,type) \
    last(Name,name)

#define MODEL_ROLES(role) \
    MODEL_ROLES_(role,role,role)

class ConfigGroupModel::Private: public QObject {
    Q_OBJECT
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

    Private(ConfigGroupModel* aParent);
    ~Private();

    static QSharedPointer<QThreadPool> sharedThreadPool();
    ConfigGroupModel* parentModel() const;
    bool setDir(QString aDir);
    void considerRefresh();
    void refresh();

public Q_SLOTS:
    void onRefreshTaskDone();

public:
    QString iDir;
    QStringList iContents;
    QSharedPointer<QThreadPool> iThreadPool;
    ConfigClient iClient;
    RefreshTask* iRefreshTask;
};

// ==========================================================================
// ConfigGroupModel::Private::RefreshTask
// ==========================================================================

class ConfigGroupModel::Private::RefreshTask : public HarbourTask {
    Q_OBJECT
public:
    RefreshTask(QThreadPool* aPool, ConfigClient aClient, QString aDir);

    void performTask() Q_DECL_OVERRIDE;
    static bool lessThan(const QString& aStr1, const QString& aStr2);

public:
    ConfigClient iClient;
    QString iDir;
    QStringList iList;
};

ConfigGroupModel::Private::RefreshTask::RefreshTask(QThreadPool* aPool,
    ConfigClient aClient, QString aDir) :
    HarbourTask(aPool),
    iClient(aClient),
    iDir(aDir)
{
}

void ConfigGroupModel::Private::RefreshTask::performTask()
{
    iList = iClient.list(iDir);
    qSort(iList.begin(), iList.end(), lessThan);
    HDEBUG(qPrintable(iDir) << iList.count() << "entries");
}

bool ConfigGroupModel::Private::RefreshTask::lessThan(const QString& aStr1, const QString& aStr2)
{
    const bool endsWithSlash1 = aStr1.endsWith(QChar('/'));
    const bool endsWithSlash2 = aStr2.endsWith(QChar('/'));
    if (endsWithSlash1 && !endsWithSlash2) {
        return true;
    } else if (!endsWithSlash1 && endsWithSlash2) {
        return false;
    } else {
        return aStr1 < aStr2;
    }
}

// ==========================================================================
// ConfigGroupModel::Private
// ==========================================================================

ConfigGroupModel::Private::Private(ConfigGroupModel* aParent) :
    QObject(aParent),
    iThreadPool(sharedThreadPool()),
    iRefreshTask(Q_NULLPTR)
{
}

ConfigGroupModel::Private::~Private()
{
    if (iRefreshTask) iRefreshTask->release();
}

QSharedPointer<QThreadPool> ConfigGroupModel::Private::sharedThreadPool()
{
    static QWeakPointer<QThreadPool> gSharedPool;
    QSharedPointer<QThreadPool> pool(gSharedPool);
    if (pool.isNull()) {
        pool = QSharedPointer<QThreadPool>::create();
        gSharedPool = pool;
    }
    return pool;
}

inline ConfigGroupModel* ConfigGroupModel::Private::parentModel() const
{
    return qobject_cast<ConfigGroupModel*>(parent());
}

bool ConfigGroupModel::Private::setDir(QString aDir)
{
    if (iDir != aDir) {
        HDEBUG(aDir);
        iDir = aDir;
        considerRefresh();
        return true;
    }
    return false;
}

void ConfigGroupModel::Private::considerRefresh()
{
    if (iClient.isValid() && !iDir.isEmpty()) {
        refresh();
    }
}

void ConfigGroupModel::Private::refresh()
{
    bool wasRefreshing;
    if (iRefreshTask) {
        iRefreshTask->release();
        wasRefreshing = true;
    } else {
        wasRefreshing = false;
    }
    iRefreshTask = new RefreshTask(iThreadPool.data(), iClient, iDir);
    iRefreshTask->submit(this, SLOT(onRefreshTaskDone()));
    if (!wasRefreshing) {
        Q_EMIT parentModel()->readyChanged();
    }
}

void ConfigGroupModel::Private::onRefreshTaskDone()
{
    HDEBUG("Refreshed" << qPrintable(iDir));
    if (sender() == iRefreshTask) {
        const QStringList list(iRefreshTask->iList);
        iRefreshTask->release();
        iRefreshTask = NULL;

        // Update the model
        ConfigGroupModel* model = parentModel();
        model->beginResetModel();
        iContents = list;
        model->endResetModel();
        Q_EMIT model->readyChanged();
    }
}

// ==========================================================================
// ConfigGroupModel
// ==========================================================================

#define SUPER QAbstractListModel

ConfigGroupModel::ConfigGroupModel(QObject* aParent) :
    SUPER(aParent),
    iPrivate(new Private(this))
{
}

ConfigGroupModel::~ConfigGroupModel()
{
    delete iPrivate;
}

bool ConfigGroupModel::ready() const
{
    return !iPrivate->iRefreshTask;
}

QString ConfigGroupModel::dir() const
{
    return iPrivate->iDir;
}

void ConfigGroupModel::setDir(QString aDir)
{
    if (iPrivate->setDir(aDir)) {
        Q_EMIT dirChanged();
    }
}

QVariant ConfigGroupModel::configClient() const
{
    return QVariant::fromValue(iPrivate->iClient);
}

void ConfigGroupModel::setConfigClient(QVariant aClient)
{
    ConfigClient client(aClient.value<ConfigClient>());
    if (iPrivate->iClient != client) {
        iPrivate->iClient = client;
        iPrivate->considerRefresh();
        Q_EMIT configClientChanged();
    }
}

QStringList ConfigGroupModel::entries(QList<int> aRows) const
{
    QStringList out;
    const int nrows = aRows.count();
    const int count = iPrivate->iContents.count();
    const QString dir(iPrivate->iDir);
    for (int i = 0; i < nrows; i++) {
        const int row = aRows.at(i);
        if (row >= 0 && row < count) {
            out.append(dir + iPrivate->iContents.at(row));
        }
    }
    return out;
}

QHash<int,QByteArray> ConfigGroupModel::roleNames() const
{
    QHash<int,QByteArray> roles;
#define ROLE(X,x) roles.insert(Private::X##Role, #x);
MODEL_ROLES(ROLE)
#undef ROLE
    return roles;
}

int ConfigGroupModel::rowCount(const QModelIndex& aParent) const
{
    return iPrivate->iContents.count();
}

QVariant ConfigGroupModel::data(const QModelIndex& aIndex, int aRole) const
{
    const int row = aIndex.row();
    if (row >= 0 && row < iPrivate->iContents.count()) {
        QString entry = iPrivate->iContents.at(row);
        switch ((Private::Role)aRole) {
        case Private::NameRole:
            return entry.endsWith('/') ? entry.left(entry.length()-1) : entry;
        case Private::TypeRole:
            return entry.endsWith('/') ? Group : Entry;
        }
    }
    return QVariant();
}

#include "ConfigGroupModel.moc"
