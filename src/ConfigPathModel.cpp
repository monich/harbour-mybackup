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

#include "ConfigPathModel.h"
#include "ConfigClient.h"

#include "HarbourDebug.h"

// ==========================================================================
// ConfigPathModel::Private
// ==========================================================================

class ConfigPathModel::Private {
public:
    enum Role {
        PathRole = Qt::UserRole
    };

    static const QString SEPARATOR;

    Private();

    int rowCount() const;
    QString path(int aRow) const;
    QVariant get(int aRow, Role aRole) const;

public:
    ConfigClient iClient;
    QVector<QString> iSubDirs;
};

const QString ConfigPathModel::Private::SEPARATOR("/");

ConfigPathModel::Private::Private() :
    iClient(ConfigClient::create())
{
}

inline int ConfigPathModel::Private::rowCount() const
{
    return iSubDirs.count() + 1;
}

QString ConfigPathModel::Private::path(int aRow) const
{
    QString path(SEPARATOR);
    const int n = qMin(iSubDirs.count(), aRow);
    for (int i = 0; i < n; i++) {
        path += iSubDirs.at(i);
        path += SEPARATOR;
    }
    return path;
}

QVariant ConfigPathModel::Private::get(int aRow, Role aRole) const
{
    if (aRow <= iSubDirs.count()) {
        switch (aRole) {
        case PathRole: return path(aRow);
        }
    }
    return QVariant();
}

// ==========================================================================
// ConfigPathModel
// ==========================================================================

#define SUPER QAbstractListModel

ConfigPathModel::ConfigPathModel(QObject* aParent) :
    SUPER(aParent),
    iPrivate(new Private)
{
}

ConfigPathModel::~ConfigPathModel()
{
    delete iPrivate;
}

QVariant ConfigPathModel::configClient() const
{
    return QVariant::fromValue(iPrivate->iClient);
}

void ConfigPathModel::setDepth(int aDepth)
{
    const int n = iPrivate->rowCount();
    if (aDepth > 0 && aDepth < n) {
        HDEBUG(aDepth);
        beginRemoveRows(QModelIndex(), aDepth, n - 1);
        iPrivate->iSubDirs.resize(aDepth - 1);
        endRemoveRows();
    }
}

void ConfigPathModel::enter(QString aSubDir)
{
    if (!aSubDir.isEmpty()) {
        HDEBUG(aSubDir);
        const int n = iPrivate->rowCount();
        beginInsertRows(QModelIndex(), n, n);
        iPrivate->iSubDirs.append(aSubDir);
        endInsertRows();
    }
}

QHash<int,QByteArray> ConfigPathModel::roleNames() const
{
    QHash<int,QByteArray> roles;
    roles.insert(Private::PathRole, "path");
    return roles;
}

int ConfigPathModel::rowCount(const QModelIndex& aParent) const
{
    return iPrivate->rowCount();
}

QVariant ConfigPathModel::data(const QModelIndex& aIndex, int aRole) const
{
    return iPrivate->get(aIndex.row(), (Private::Role)aRole);
}
