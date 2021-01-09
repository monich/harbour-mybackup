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

#include "BackupList.h"
#include "BackupUtil.h"
#include "ApplicationModel.h"

#include "HarbourDebug.h"

#include <QStandardPaths>

// ==========================================================================
// BackupList::Item::Private
// ==========================================================================

class BackupList::Item::Private {
public:
    class App;
    class Path;
    class Config;

    static const QString KEY_TYPE;
    static const QString KEY_PATH;

    Private(const QString aType, QString aPath) : iType(aType), iPath(aPath) { }

    const QString name() const;

public:
    const QString iType;
    const QString iPath;
};


const QString BackupList::Item::Private::KEY_TYPE("type");
const QString BackupList::Item::Private::KEY_PATH("path");

const QString BackupList::Item::Private::name() const
{
    QString str(iPath);
    while (str.endsWith(QChar('/'))) {
        str = str.left(str.length() - 1);
    }
    return str;
}

// ==========================================================================
// BackupList::Item::Private::App
// ==========================================================================

class BackupList::Item::Private::App : public BackupList::Item {
public:
    static const QString TYPE;

    App(QString aDesktopFile, const ApplicationModel::AppInfo* aAppInfo);
    App(const App* aApp);

    static Item* fromDesktopFile(QString aDesktopFile);
    Item* clone() const Q_DECL_OVERRIDE;
    Type type() const Q_DECL_OVERRIDE;
    QVariant get(Role aRole) const Q_DECL_OVERRIDE;
    bool equals(const Item* aItem) const Q_DECL_OVERRIDE;
    const QStringList pathList() const Q_DECL_OVERRIDE;
    const QStringList configList() const Q_DECL_OVERRIDE;

public:
    const QString iAppName;
    const QString iAppIcon;
    const QStringList iBackupPathList;
    const QStringList iBackupConfigList;
};

const QString BackupList::Item::Private::App::TYPE("app");

BackupList::Item* BackupList::Item::Private::App::fromDesktopFile(QString aDesktopFile)
{
    QScopedPointer<ApplicationModel::AppInfo> appInfo(ApplicationModel::parseDesktopFile(aDesktopFile));
    return appInfo.isNull() ? Q_NULLPTR : new App(aDesktopFile, appInfo.data());
}

BackupList::Item::Private::App::App(QString aDesktopFile, const ApplicationModel::AppInfo* aAppInfo) :
    Item(TYPE, aDesktopFile),
    iAppName(aAppInfo->iAppName),
    iAppIcon(aAppInfo->iconUrl()),
    iBackupPathList(aAppInfo->iBackupPathList),
    iBackupConfigList(aAppInfo->iBackupConfigList)
{
}

BackupList::Item::Private::App::App(const App* aApp) :
    Item(TYPE, aApp->path()),
    iAppName(aApp->iAppName),
    iAppIcon(aApp->iAppIcon),
    iBackupPathList(aApp->iBackupPathList),
    iBackupConfigList(aApp->iBackupConfigList)
{
}

BackupList::Item* BackupList::Item::Private::App::clone() const
{
    return new App(this);
}

BackupList::Item::Type BackupList::Item::Private::App::type() const
{
    return BackupList::Item::App;
}

const QStringList BackupList::Item::Private::App::pathList() const
{
    return iBackupPathList;
}

const QStringList BackupList::Item::Private::App::configList() const
{
    return iBackupConfigList;
}

bool BackupList::Item::Private::App::equals(const Item* aItem) const
{
    if (Item::equals(aItem)) {
        App* other = (App*)aItem;
        return other == this || (
            other->iAppName == iAppName &&
            other->iAppIcon == iAppIcon &&
            other->iBackupPathList == iBackupPathList &&
            other->iBackupConfigList == iBackupConfigList);
    }
    return false;
}

QVariant BackupList::Item::Private::App::get(Role aRole) const
{
    switch (aRole) {
    case NameRole:
        return iAppName;
    case AppIconRole:
        return iAppIcon;
    case AppPathListRole:
        return BackupUtil::beautifyPathList(iBackupPathList);
    case AppConfigListRole:
        return iBackupConfigList;
    default:
        return Item::get(aRole);
    }
}

// ==========================================================================
// BackupList::Item::Private::Path
// ==========================================================================

class BackupList::Item::Private::Path : public BackupList::Item {
public:
    static const QString TYPE;

    Path(QString aPath) : Item(TYPE, aPath) { }
    Path(const Path* aPath) : Item(TYPE, aPath->path()) { }

    static QString homePath();

    Item* clone() const Q_DECL_OVERRIDE;
    Type type() const Q_DECL_OVERRIDE;
    QVariant get(Role aRole) const Q_DECL_OVERRIDE;
    const QStringList pathList() const Q_DECL_OVERRIDE;
    const QStringList configList() const Q_DECL_OVERRIDE;
};

const QString BackupList::Item::Private::Path::TYPE("path");

BackupList::Item* BackupList::Item::Private::Path::clone() const
{
    return new Path(this);
}

BackupList::Item::Type BackupList::Item::Private::Path::type() const
{
    return BackupList::Item::Path;
}

const QStringList BackupList::Item::Private::Path::pathList() const
{
    return QStringList(iPrivate->iPath);
}

const QStringList BackupList::Item::Private::Path::configList() const
{
    return QStringList();
}

QVariant BackupList::Item::Private::Path::get(Role aRole) const
{
    return (aRole == NameRole) ?
        BackupUtil::beautifyPath(iPrivate->name()) :
        Item::get(aRole);
}

// ==========================================================================
// BackupList::Item::Private::Config
// ==========================================================================

class BackupList::Item::Private::Config : public BackupList::Item {
public:
    static const QString TYPE;

    Config(QString aPath) : Item(TYPE, aPath) { }
    Config(const Config* aConfig) : Item(TYPE, aConfig->path()) { }

    Item* clone() const Q_DECL_OVERRIDE;
    Type type() const Q_DECL_OVERRIDE;
    QVariant get(Role aRole) const Q_DECL_OVERRIDE;
    const QStringList pathList() const Q_DECL_OVERRIDE;
    const QStringList configList() const Q_DECL_OVERRIDE;
};

const QString BackupList::Item::Private::Config::TYPE("config");

BackupList::Item* BackupList::Item::Private::Config::clone() const
{
    return new Config(this);
}

BackupList::Item::Type BackupList::Item::Private::Config::type() const
{
    return BackupList::Item::Config;
}

const QStringList BackupList::Item::Private::Config::pathList() const
{
    return QStringList();
}

const QStringList BackupList::Item::Private::Config::configList() const
{
    return QStringList(iPrivate->iPath);
}

QVariant BackupList::Item::Private::Config::get(Role aRole) const
{
    if (aRole == NameRole) {
        const QString str(iPrivate->name());
        return str.isEmpty() ? QString(QChar('/')) : str;
    }
    return Item::get(aRole);
}

// ==========================================================================
// BackupList::Item
// ==========================================================================

BackupList::Item::Item(const QString aType, const QString aPath) :
    iPrivate(new Private(aType, aPath))
{
}

BackupList::Item::~Item()
{
    delete iPrivate;
}

bool BackupList::Item::equals(const Item* aItem) const
{
    return aItem && (aItem == this || (aItem->type() == type() &&
        iPrivate->iPath == aItem->iPrivate->iPath));
}

QVariant BackupList::Item::get(Role aRole) const
{
    switch (aRole) {
    case TypeRole:
        return QVariant::fromValue((int)type());
    case NameRole:
        return iPrivate->name();
    case PathRole:
        return iPrivate->iPath;
    default:
        return QVariant();
    }
}

const QString BackupList::Item::path() const
{
    return iPrivate->iPath;
}

QVariantMap BackupList::Item::toVariantMap() const
{
    QVariantMap map;
    map.insert(Private::KEY_PATH, iPrivate->iPath);
    map.insert(Private::KEY_TYPE, iPrivate->iType);
    return map;
}

BackupList::Item* BackupList::Item::create(QVariantMap aData)
{
    const QString path(aData.value(Private::KEY_PATH).toString());
    if (!path.isEmpty()) {
        const QString type(aData.value(Private::KEY_TYPE).toString());
        if (type == Private::App::TYPE) {
            return Private::App::fromDesktopFile(path);
        } else if (type == Private::Path::TYPE) {
            return new Private::Path(path);
        } else if (type == Private::Config::TYPE) {
            return new Private::Config(path);
        }
    }
    return Q_NULLPTR;
}

BackupList::Item* BackupList::Item::create(Type aType, QString aPath)
{
    if (!aPath.isEmpty()) {
        switch (aType) {
        case App: return Private::App::fromDesktopFile(aPath);
        case Path: return new Private::Path(aPath);
        case Config: return new Private::Config(aPath);
        }
    }
    return Q_NULLPTR;
}
