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

#ifndef BACKUP_LIST_H
#define BACKUP_LIST_H

#include <QVariant>

#define BACKUP_LIST_ITEM_ROLES_(first,role,last) \
    first(Type,type) \
    role(Name,name) \
    role(Path,path) \
    role(AppIcon,appIcon) \
    role(AppPathList,appPathList) \
    last(AppConfigList,appConfigList)

#define BACKUP_LIST_ITEM_ROLES(role) \
    BACKUP_LIST_ITEM_ROLES_(role,role,role)

class BackupList {
    Q_DISABLE_COPY(BackupList)
public:
    enum Diff {
        DiffNone = 0x00,
        DiffItems = 0x01,
        DiffLastBackup = 0x02,
        DiffLastRestore = 0x04
    };

    class Item {
    public:
        enum Type {
            App,
            Path,
            Config
        };

        enum Role {
    #define BACKUP_LIST_ITEM_FIRST(X,x) FirstRole = Qt::UserRole, X##Role = FirstRole,
    #define BACKUP_LIST_ITEM_ROLE(X,x) X##Role,
    #define BACKUP_LIST_ITEM_LAST(X,x) X##Role, LastRole = X##Role
            BACKUP_LIST_ITEM_ROLES_(BACKUP_LIST_ITEM_FIRST,\
            BACKUP_LIST_ITEM_ROLE,\
            BACKUP_LIST_ITEM_LAST)
    #undef BACKUP_LIST_ITEM_FIRST
    #undef BACKUP_LIST_ITEM_ROLE
    #undef BACKUP_LIST_ITEM_LAST
        };

        Item(const QString aType, const QString aPath);
        virtual ~Item();
        virtual Item* clone() const = 0;
        virtual Type type() const = 0;
        virtual const QStringList pathList() const = 0;
        virtual const QStringList configList() const = 0;
        virtual bool equals(const Item* aItem) const;
        virtual QVariant get(Role aRole) const;
        const QString path() const;
        QVariantMap toVariantMap() const;

        static Item* create(QVariantMap aData);
        static Item* create(Type aType, QString aPath);

    private:
        class Private;
        Private* iPrivate;
    };

    BackupList(const QString aFile);
    BackupList();
    ~BackupList();

    static QString configDir();
    static QString defaultConfigFile();

    void load(const QString aFile);
    bool save(const QString aFile) const;

    int count() const;
    void appendItem(Item* aItem); // Takes ownership
    const Item* itemAt(int aPos) const;
    const Item* moveItem(int aFrom, int aTo);
    void removeItemAt(int aPos);

    QDateTime lastBackup() const;
    QDateTime lastRestore() const;
    void updateLastBackup();
    void updateLastRestore();

    uint diff(const BackupList& aList) const;
    void copyFrom(const BackupList& aList);
    void swap(BackupList& aList);

    QStringList backupFileList(const QString aExtraPath) const;
    QStringList backupConfigList(const QString aExtraEntry) const;

private:
    class Private;
    Private* iPrivate;
};

#endif // BACKUP_LIST_H
