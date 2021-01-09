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

#ifndef BACKUP_LIST_MODEL_H
#define BACKUP_LIST_MODEL_H

#include <QDateTime>
#include <QStringList>
#include <QAbstractListModel>

class QQmlEngine;
class QJSEngine;

class BackupListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool configured READ configured NOTIFY configuredChanged)
    Q_PROPERTY(QDateTime lastBackup READ lastBackup NOTIFY lastBackupChanged)
    Q_PROPERTY(QDateTime lastRestore READ lastRestore NOTIFY lastRestoreChanged)
    Q_PROPERTY(QStringList apps READ apps NOTIFY appsChanged)
    Q_DISABLE_COPY(BackupListModel)

    class Private;
    class ModelData;

public:
    enum Type {
        App,
        Path,
        Config
    };
    Q_ENUMS(Type)

    BackupListModel(QObject* aParent = Q_NULLPTR);
    ~BackupListModel();

    bool configured() const;
    QDateTime lastBackup() const;
    QDateTime lastRestore() const;
    QStringList apps() const;

    Q_INVOKABLE void add(Type aType, QStringList aItems);
    Q_INVOKABLE void removeAt(int aRow);

    // QAbstractItemModel
    QHash<int,QByteArray> roleNames() const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex& aParent) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex& aIndex, int aRole) const Q_DECL_OVERRIDE;
    bool moveRows(const QModelIndex &aSrcParent, int aSrcRow, int aCount,
        const QModelIndex &aDestParent, int aDestRow) Q_DECL_OVERRIDE;

    // Callback for qmlRegisterSingletonType<BackupListModel>
    static QObject* createSingleton(QQmlEngine* aEngine, QJSEngine* aScript);

Q_SIGNALS:
    void configuredChanged();
    void lastBackupChanged();
    void lastRestoreChanged();
    void appsChanged();

private:
    Private* iPrivate;
};

#endif // BACKUP_LIST_MODEL_H
