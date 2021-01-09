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

#include "DirectoryContentsModel.h"

#include "HarbourTask.h"
#include "HarbourDebug.h"

#include <QWeakPointer>
#include <QSharedPointer>
#include <QFileInfo>
#include <QFileInfoList>
#include <QThreadPool>
#include <QList>
#include <QDir>

#define MODEL_ROLES_(first,role,last) \
    first(Type,type) \
    last(Name,name)

#define MODEL_ROLES(role) \
    MODEL_ROLES_(role,role,role)

// ==========================================================================
// DirectoryContentsModel::ModelData
// ==========================================================================

class DirectoryContentsModel::ModelData {
public:
    class RefreshTask;
    typedef QList<ModelData*> List;

    enum Role {
#define FIRST(X,x) FirstRole = Qt::UserRole, X##Role = FirstRole,
#define ROLE(X,x) X##Role,
#define LAST(X,x) X##Role, LastRole = X##Role
        MODEL_ROLES_(FIRST,ROLE,LAST)
#undef FIRST
#undef ROLE
#undef LAST
    };

    ModelData(const QFileInfo& aFileInfo);

    QVariant get(Role aRole) const;

public:
    QFileInfo iFileInfo;
};

DirectoryContentsModel::ModelData::ModelData(const QFileInfo& aFileInfo) :
    iFileInfo(aFileInfo)
{
}

QVariant DirectoryContentsModel::ModelData::get(Role aRole) const
{
    switch (aRole) {
    case TypeRole:
        return (int)iFileInfo.isDir() ? Directory : File;
    case NameRole:
        return iFileInfo.fileName();
    default:
        return QVariant();
    }
}

// ==========================================================================
// DirectoryContentsModel::ModelData::RefreshTask
// ==========================================================================

class DirectoryContentsModel::ModelData::RefreshTask : public HarbourTask {
    Q_OBJECT
public:
    RefreshTask(QThreadPool* aPool, QString aPath);

    void performTask() Q_DECL_OVERRIDE;

public:
    QDir iDir;
    QFileInfoList iList;
};

DirectoryContentsModel::ModelData::RefreshTask::RefreshTask(QThreadPool* aPool,
    QString aPath) :
    HarbourTask(aPool),
    iDir(aPath)
{
}

void DirectoryContentsModel::ModelData::RefreshTask::performTask()
{
    iList = iDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs |
        QDir::Files | QDir::Hidden | QDir::NoSymLinks | QDir::Readable,
        QDir::DirsFirst | QDir::Name);
    HDEBUG(qPrintable(iDir.path()) << iList.count() << "entries");
}

// ==========================================================================
// DirectoryContentsModel::Private
// ==========================================================================

class DirectoryContentsModel::Private: public QObject {
    Q_OBJECT
public:
    Private(DirectoryContentsModel* aParent);
    ~Private();

    static QSharedPointer<QThreadPool> sharedThreadPool();
    DirectoryContentsModel* parentModel() const;
    ModelData* dataAt(int aIndex) const;
    int rowCount() const;
    bool setPath(QString aPath);
    void refresh();

public Q_SLOTS:
    void onRefreshTaskDone();

public:
    QString iPath;
    QSharedPointer<QThreadPool> iThreadPool;
    ModelData::RefreshTask* iRefreshTask;
    ModelData::List iData;
};

DirectoryContentsModel::Private::Private(DirectoryContentsModel* aParent) :
    QObject(aParent),
    iThreadPool(sharedThreadPool()),
    iRefreshTask(Q_NULLPTR)
{
}

DirectoryContentsModel::Private::~Private()
{
    if (iRefreshTask) iRefreshTask->release();
    qDeleteAll(iData);
}

QSharedPointer<QThreadPool> DirectoryContentsModel::Private::sharedThreadPool()
{
    static QWeakPointer<QThreadPool> gSharedPool;
    QSharedPointer<QThreadPool> pool(gSharedPool);
    if (pool.isNull()) {
        pool = QSharedPointer<QThreadPool>::create();
        gSharedPool = pool;
    }
    return pool;
}

inline DirectoryContentsModel* DirectoryContentsModel::Private::parentModel() const
{
    return qobject_cast<DirectoryContentsModel*>(parent());
}

inline int DirectoryContentsModel::Private::rowCount() const
{
    return iData.count();
}

DirectoryContentsModel::ModelData* DirectoryContentsModel::Private::dataAt(int aIndex) const
{
    if (aIndex >= 0 && aIndex < iData.count()) {
        return iData.at(aIndex);
    } else {
        return NULL;
    }
}

bool DirectoryContentsModel::Private::setPath(QString aPath)
{
    if (iPath != aPath) {
        HDEBUG(aPath);
        iPath = aPath;
        refresh();
        return true;
    }
    return false;
}

void DirectoryContentsModel::Private::refresh()
{
    bool wasRefreshing;
    if (iRefreshTask) {
        iRefreshTask->release();
        wasRefreshing = true;
    } else {
        wasRefreshing = false;
    }
    iRefreshTask = new ModelData::RefreshTask(iThreadPool.data(), iPath);
    iRefreshTask->submit(this, SLOT(onRefreshTaskDone()));
    if (!wasRefreshing) {
        Q_EMIT parentModel()->readyChanged();
    }
}

void DirectoryContentsModel::Private::onRefreshTaskDone()
{
    HDEBUG("Refreshed" << qPrintable(iPath));
    if (sender() == iRefreshTask) {
        const QFileInfoList list(iRefreshTask->iList);
        iRefreshTask->release();
        iRefreshTask = NULL;

        // Update the model
        const int n = list.count();
        DirectoryContentsModel* model = parentModel();
        model->beginResetModel();
        qDeleteAll(iData);
        iData.clear();
        for (int i = 0; i < n; i++) {
            iData.append(new ModelData(list.at(i)));
        }
        model->endResetModel();
        Q_EMIT model->readyChanged();
    }
}

// ==========================================================================
// DirectoryContentsModel
// ==========================================================================

#define SUPER QAbstractListModel

DirectoryContentsModel::DirectoryContentsModel(QObject* aParent) :
    SUPER(aParent),
    iPrivate(new Private(this))
{
}

DirectoryContentsModel::~DirectoryContentsModel()
{
    delete iPrivate;
}

bool DirectoryContentsModel::ready() const
{
    return !iPrivate->iRefreshTask;
}

QString DirectoryContentsModel::path() const
{
    return iPrivate->iPath;
}

void DirectoryContentsModel::setPath(QString aPath)
{
    if (iPrivate->setPath(aPath)) {
        Q_EMIT pathChanged();
    }
}

QStringList DirectoryContentsModel::entries(QList<int> aRows) const
{
    QStringList out;
    const int nrows = aRows.count();
    if (nrows > 0) {
        const QChar sep('/');
        const QString prefix(iPrivate->iPath.endsWith(sep) ? iPrivate->iPath :
            (iPrivate->iPath + sep));
        for (int i = 0; i < nrows; i++) {
            ModelData* data = iPrivate->dataAt(aRows.at(i));
            if (data) {
                const QString name(data->iFileInfo.fileName());
                out.append(data->iFileInfo.isDir() ? (prefix + name + sep) :
                    (prefix + name));
            }
        }
    }
    return out;
}

QHash<int,QByteArray> DirectoryContentsModel::roleNames() const
{
    QHash<int,QByteArray> roles;
#define ROLE(X,x) roles.insert(ModelData::X##Role, #x);
MODEL_ROLES(ROLE)
#undef ROLE
    return roles;
}

int DirectoryContentsModel::rowCount(const QModelIndex& aParent) const
{
    return iPrivate->rowCount();
}

QVariant DirectoryContentsModel::data(const QModelIndex& aIndex, int aRole) const
{
    ModelData* data = iPrivate->dataAt(aIndex.row());
    return data ? data->get((ModelData::Role)aRole) : QVariant();
}

#include "DirectoryContentsModel.moc"
