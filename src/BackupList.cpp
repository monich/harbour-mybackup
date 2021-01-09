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
#include "BackupDefs.h"

#include "HarbourJson.h"
#include "HarbourDebug.h"

#include <QDir>
#include <QList>
#include <QDateTime>
#include <QStandardPaths>

#define DEFAULT_CONFIG_FILE "backup.json"

// ==========================================================================
// BackupList::Private
// ==========================================================================

class BackupList::Private {
public:
    typedef const QStringList (Item::*PathListGetter)() const;

    ~Private();

    static const QString HOME_PREFIX;
    static const QString DOTSLASH;
    static const QString KEY_ITEMS;
    static const QString KEY_LAST_BACKUP;
    static const QString KEY_LAST_RESTORE;

public:
    void load(const QString& aFile);
    bool save(const QString& aFile);
    void append(const QList<Item*>& aList);
    void swap(Private* aPrivate);
    QStringList pathList(const QString& aExtra, PathListGetter aMethod) const;
    static QDateTime getDateTimeValue(const QVariantMap& aMap, const QString& aKey);
    static void setDateTimeValue(QVariantMap* aMap, const QString& aKey,
        const QDateTime& aDateTime);

public:
    QList<Item*> iList;
    QDateTime iLastBackup;
    QDateTime iLastRestore;
};

const QString BackupList::Private::HOME_PREFIX("~/");
const QString BackupList::Private::DOTSLASH("./");
const QString BackupList::Private::KEY_ITEMS("items");
const QString BackupList::Private::KEY_LAST_BACKUP("lastBackup");
const QString BackupList::Private::KEY_LAST_RESTORE("lastRestore");

BackupList::Private::~Private()
{
    qDeleteAll(iList);
}

void BackupList::Private::load(const QString& aFile)
{
    qDeleteAll(iList);
    iList.clear();
    iLastBackup = QDateTime();
    QVariantMap data;
    if (HarbourJson::load(aFile, data)) {
        QVariantList items = data.value(KEY_ITEMS).toList();
        const int n = items.count();
        for (int i = 0; i < n; i++) {
            Item* item = Item::create(items.at(i).toMap());
            if (item) {
                iList.append(item);
            }
        }
        iLastBackup = getDateTimeValue(data, KEY_LAST_BACKUP);
        iLastRestore = getDateTimeValue(data, KEY_LAST_RESTORE);
    }
}

bool BackupList::Private::save(const QString& aFile)
{
    QVariantList items;
    const int n = iList.count();
    for (int i = 0; i < n; i++) {
        items.append(iList.at(i)->toVariantMap());
    }
    QVariantMap data;
    data.insert(KEY_ITEMS, items);
    setDateTimeValue(&data, KEY_LAST_BACKUP, iLastBackup);
    setDateTimeValue(&data, KEY_LAST_RESTORE, iLastRestore);
    return HarbourJson::save(aFile, data);
}

QDateTime BackupList::Private::getDateTimeValue(const QVariantMap& aMap,
    const QString& aKey)
{
    const QString str = aMap.value(aKey).toString();
    return str.isEmpty() ? QDateTime() : QDateTime::fromString(str, Qt::ISODate);
}

void BackupList::Private::setDateTimeValue(QVariantMap* aMap, const QString& aKey,
    const QDateTime& aDateTime)
{
    if (aDateTime.isValid()) {
        aMap->insert(aKey, aDateTime.toUTC().toString(Qt::ISODate));
    }
}

void BackupList::Private::append(const QList<Item*>& aList)
{
    const int n = aList.count();
    for (int i = 0; i < n; i++) {
        iList.append(aList.at(i)->clone());
    }
}

void BackupList::Private::swap(Private* aPrivate)
{
    if (aPrivate != this) {
        const QList<Item*> tmpList(iList);
        const QDateTime tmpBackup(iLastBackup);
        const QDateTime tmpRestore(iLastRestore);

        iList = aPrivate->iList;
        iLastBackup = aPrivate->iLastBackup;
        iLastRestore = aPrivate->iLastRestore;

        aPrivate->iList = tmpList;
        aPrivate->iLastBackup = tmpBackup;
        aPrivate->iLastRestore = tmpRestore;
    }
}

QStringList BackupList::Private::pathList(const QString& aExtra,
    PathListGetter aMethod) const
{
    QStringList list;
    const int n = iList.count();
    for (int i = 0; i < n; i++) {
        const QStringList entries((iList.at(i)->*aMethod)());
        const int m = entries.count();
        for (int j = 0; j < m; j++) {
            const QString entry(entries.at(j));
            if (!entry.isEmpty()) {
                list.append(entry);
            }
        }
    }
    if (!aExtra.isEmpty()) {
        list.append(aExtra);
    }
    list.sort();
    for (int k = 0; k + 1 < list.count(); k++) {
        const QString path(list.at(k));
        if (path.endsWith('/')) {
            // Remove redundant paths
            while (k + 1 < list.count() && list.at(k + 1).startsWith(path)) {
                HDEBUG("Dropping" << qPrintable(list.at(k + 1)));
                list.removeAt(k + 1);
            }
        }
    }
    return list;
}

// ==========================================================================
// BackupList
// ==========================================================================

BackupList::BackupList(const QString aFile) :
    iPrivate(new Private)
{
    load(aFile);
}

BackupList::BackupList() :
    iPrivate(new Private)
{
}

BackupList::~BackupList()
{
    delete iPrivate;
}

QString BackupList::configDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
        QStringLiteral("/" APP_NAME);
}

QString BackupList::defaultConfigFile()
{
    return configDir() + QStringLiteral("/" DEFAULT_CONFIG_FILE);
}

void BackupList::load(const QString aFile)
{
    iPrivate->load(aFile);
}

bool BackupList::save(const QString aFile) const
{
    return iPrivate->save(aFile);
}

int BackupList::count() const
{
    return iPrivate->iList.count();
}

void BackupList::appendItem(BackupList::Item* aItem)
{
    // Takes ownership
    iPrivate->iList.append(aItem);
}

const BackupList::Item* BackupList::itemAt(int aPos) const
{
    if (aPos >= 0 && aPos < iPrivate->iList.count()) {
        return iPrivate->iList.at(aPos);
    } else {
        return Q_NULLPTR;
    }
}

const BackupList::Item* BackupList::moveItem(int aFrom, int aTo)
{
    const int n = iPrivate->iList.count();
    if (aFrom >= 0 && aFrom < n && aTo >= 0 && aTo < n) {
        const BackupList::Item* item = iPrivate->iList.at(aFrom);
        iPrivate->iList.move(aFrom, aTo);
        return item;
    }
    return Q_NULLPTR;
}

void BackupList::removeItemAt(int aPos)
{
    if (aPos >= 0 && aPos < iPrivate->iList.count()) {
        delete iPrivate->iList.takeAt(aPos);
    }
}

QDateTime BackupList::lastBackup() const
{
    return iPrivate->iLastBackup;
}

QDateTime BackupList::lastRestore() const
{
    return iPrivate->iLastRestore;
}

void BackupList::updateLastBackup()
{
    iPrivate->iLastBackup = QDateTime::currentDateTime();
}

void BackupList::updateLastRestore()
{
    iPrivate->iLastRestore = QDateTime::currentDateTime();
}

uint BackupList::diff(const BackupList& aList) const
{
    uint diff = DiffNone;
    if (&aList != this) {
        // Compare items
        const int n = iPrivate->iList.count();
        if (aList.iPrivate->iList.count() != n) {
            diff |= DiffItems;
        } else {
            for (int i = 0; i < n; i++) {
                const Item* item1 = iPrivate->iList.at(i);
                const Item* item2 = aList.iPrivate->iList.at(i);
                if (!item1->equals(item2)) {
                    diff |= DiffItems;
                    break;
                }
            }
        }
        // Compare backup and restore dates
        if (iPrivate->iLastBackup != aList.iPrivate->iLastBackup) {
            diff |= DiffLastBackup;
        }
        if (iPrivate->iLastRestore != aList.iPrivate->iLastRestore) {
            diff |= DiffLastRestore;
        }
    }
    return diff;
}

void BackupList::copyFrom(const BackupList& aList)
{
    if (this != &aList) {
        qDeleteAll(iPrivate->iList);
        iPrivate->iList.clear();
        iPrivate->append(aList.iPrivate->iList);
    }
}

void BackupList::swap(BackupList& aList)
{
    iPrivate->swap(aList.iPrivate);
}

QStringList BackupList::backupFileList(const QString aExtraPath) const
{
    QStringList list(iPrivate->pathList(aExtraPath, &Item::pathList));
    // Only leave names relative to the home directory
    for (int i = list.count() - 1; i >= 0; i--) {
        const QString entry = list.at(i);
        const QString rel(BackupUtil::relativeToHome(entry));
        if (rel.isEmpty()) {
            HDEBUG("Dropping" << qPrintable(entry));
            list.removeAt(i);
        } else if (rel != entry) {
            HDEBUG(qPrintable(entry) << "=>"  << qPrintable(rel));
            list.replace(i, rel);
        }
    }
    // Re-sort the list and remove the dups
    if (list.count() > 1) {
        list.sort();
        QString last(list.last());
        for (int i = list.count() - 2; i >= 0; i--) {
            const QString entry = list.at(i);
            if (entry == last) {
                HDEBUG("Dropping duplicate" << qPrintable(entry));
                list.removeAt(i);
            } else {
                last = entry;
            }
        }
    }
    HDEBUG("Path list" << list);
    return list;
}

QStringList BackupList::backupConfigList(const QString aExtraEntry) const
{
    QStringList list(iPrivate->pathList(aExtraEntry, &Item::configList));
    for (int i = list.count() - 1; i >= 0; i--) {
        const QString entry = list.at(i);
        if (!entry.startsWith('/')) {
            HDEBUG("Dropping" << qPrintable(entry));
            list.removeAt(i);
        }
    }
    HDEBUG("Config list" << list);
    return list;
}
