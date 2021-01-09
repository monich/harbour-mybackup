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

#include "DirectoryPathModel.h"

#include "HarbourDebug.h"

#include <QStandardPaths>
#include <QDir>

#define MODEL_ROLES_(first,role,last) \
    first(Name,name) \
    role(RelativePath,relativePath) \
    last(FullPath,fullPath)

#define MODEL_ROLES(role) \
    MODEL_ROLES_(role,role,role)

// ==========================================================================
// DirectoryPathModel::Private
// ==========================================================================

class DirectoryPathModel::Private {
public:
    enum Role {
#define FIRST(X,x) FirstRole = Qt::UserRole, X##Role = FirstRole,
#define ROLE(X,x) X##Role,
#define LAST(X,x) X##Role, LastRole = X##Role
        MODEL_ROLES_(FIRST,ROLE,LAST)
#undef FIRST
#undef ROLE
#undef LAST
    };

    Private();

    int rowCount() const;
    QString fullPath(int aRow) const;
    QString relativePath(int aRow) const;
    QVariant data(int aRow, Role aRole) const;

public:
    QString iRootPath;
    QVector<QString> iSubDirs;
};

DirectoryPathModel::Private::Private() :
    iRootPath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation))
{
}

inline int DirectoryPathModel::Private::rowCount() const
{
    return iSubDirs.count() + 1;
}

QString DirectoryPathModel::Private::relativePath(int aRow) const
{
    QString path;
    const int n = qMin(iSubDirs.count(), aRow);
    if (n > 0) {
        path += iSubDirs.at(0);
        const QChar sep(QDir::separator());
        for (int i = 1; i < n; i++) {
            path += sep;
            path += iSubDirs.at(i);
        }
    }
    return path;
}

QString DirectoryPathModel::Private::fullPath(int aRow) const
{
    const QString rel(relativePath(aRow));
    const QChar sep(QDir::separator());
    if (iRootPath.endsWith(sep)) {
        return rel.isEmpty() ? iRootPath : (iRootPath + rel + sep);
    } else {
        return rel.isEmpty() ? (iRootPath + sep) : (iRootPath + sep + rel + sep);
    }
}

QVariant DirectoryPathModel::Private::data(int aRow, Role aRole) const
{
    if (aRow <= iSubDirs.count()) {
        switch (aRole) {
        case NameRole: return (aRow > 0) ? iSubDirs.at(aRow - 1) : QString();
        case RelativePathRole: return relativePath(aRow);
        case FullPathRole: return fullPath(aRow);
        }
    }
    return QVariant();
}

// ==========================================================================
// DirectoryPathModel
// ==========================================================================

#define SUPER QAbstractListModel

DirectoryPathModel::DirectoryPathModel(QObject* aParent) :
    SUPER(aParent),
    iPrivate(new Private)
{
}

DirectoryPathModel::~DirectoryPathModel()
{
    delete iPrivate;
}

QString DirectoryPathModel::rootPath() const
{
    return iPrivate->iRootPath;
}

void DirectoryPathModel::setRootPath(QString aPath)
{
    if (iPrivate->iRootPath != aPath) {
        iPrivate->iRootPath = aPath;
        Q_EMIT rootPathChanged();
    }
}

void DirectoryPathModel::setDepth(int aDepth)
{
    const int n = iPrivate->rowCount();
    if (aDepth > 0 && aDepth < n) {
        HDEBUG(aDepth);
        beginRemoveRows(QModelIndex(), aDepth, n - 1);
        iPrivate->iSubDirs.resize(aDepth - 1);
        endRemoveRows();
    }
}

void DirectoryPathModel::enter(QString aSubDir)
{
    if (!aSubDir.isEmpty()) {
        HDEBUG(aSubDir);
        const int n = iPrivate->rowCount();
        beginInsertRows(QModelIndex(), n, n);
        iPrivate->iSubDirs.append(aSubDir);
        endInsertRows();
    }
}

QHash<int,QByteArray> DirectoryPathModel::roleNames() const
{
    QHash<int,QByteArray> roles;
#define ROLE(X,x) roles.insert(Private::X##Role, #x);
MODEL_ROLES(ROLE)
#undef ROLE
    return roles;
}

int DirectoryPathModel::rowCount(const QModelIndex& aParent) const
{
    return iPrivate->rowCount();
}

QVariant DirectoryPathModel::data(const QModelIndex& aIndex, int aRole) const
{
    return iPrivate->data(aIndex.row(), (Private::Role)aRole);
}
