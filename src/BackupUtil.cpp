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

#include "BackupUtil.h"

#include "HarbourDebug.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

// ==========================================================================
// BackupUtil::Private
// ==========================================================================

class BackupUtil::Private {
public:
    static const QString TILDA;
    static const QString HOME_PREFIX;
    static const QString DOT_SLASH;
    static const QString homePath();
};

const QString BackupUtil::Private::TILDA("~");
const QString BackupUtil::Private::HOME_PREFIX("~/");
const QString BackupUtil::Private::DOT_SLASH("./");

const QString BackupUtil::Private::homePath()
{
    QString home(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    while (home.endsWith(QChar('/'))) {
        home = home.left(home.length() - 1);
    }
    HDEBUG("HOME =" << qPrintable(home));
    return home;
}

// ==========================================================================
// BackupUtil
// ==========================================================================

const QString BackupUtil::HOME(BackupUtil::Private::homePath());

QString BackupUtil::stripLeadingSeparators(QString aPath)
{
    static const QChar sep(QDir::separator());
    while (aPath.startsWith(sep)) {
        aPath = aPath.right(aPath.length() - 1);
    }
    return aPath;
}

QString BackupUtil::shortenHomePath(QString aPath)
{
    if (aPath.startsWith(HOME)) {
        const int pathLen = aPath.length();
        const int homeLen = HOME.length();
        if (pathLen == homeLen) {
            return Private::TILDA;
        } else if (aPath.at(homeLen) == '/') {
            return Private::HOME_PREFIX +
                stripLeadingSeparators(aPath.right(pathLen - homeLen));
        }
    }
    // Otherwise leave it as is
    return aPath;
}

QString BackupUtil::relativeToHome(QString aPath)
{
    const int pathLen = aPath.length();
    if (aPath.startsWith(HOME)) {
        const int homeLen = HOME.length();
        if (pathLen == homeLen) {
            // Exactly home path => ./
            return Private::DOT_SLASH;
        } else if (aPath.at(homeLen) == QDir::separator()) {
            // Convert something like /home/user//////foo => foo
            // And something like /home/user////// => ./
            const QString tail(stripLeadingSeparators
                (aPath.right(pathLen - homeLen - 1)));
            return tail.isEmpty() ? Private::DOT_SLASH : tail;
        }
    } else if (aPath.startsWith(Private::HOME_PREFIX)) {
        // Interpret ~/ as a reference to home directory
        // Convert something like ~//////foo => foo
        // and something like ~////// => ./
        const QString tail(stripLeadingSeparators
            (aPath.right(pathLen - Private::HOME_PREFIX.length())));
        return tail.isEmpty() ? Private::DOT_SLASH : tail;
    } else if (aPath.startsWith(Private::TILDA)) {
        // Convert ~ => ./
        return Private::DOT_SLASH;
    } else if (!aPath.startsWith(QDir::separator())) {
        // Looks like it's already relative to home
        return aPath;
    }
    // Return empty string for absolute paths
    return QString();
}

bool BackupUtil::isRelativeToHome(const QString aPath)
{
    if (aPath.startsWith(HOME)) {
        const int homeLen = HOME.length();
        return aPath.length() == homeLen ||
            aPath.at(homeLen) == QDir::separator();
    } else {
        return !aPath.startsWith(QDir::separator());
    }
}

QString BackupUtil::beautifyPath(QString aPath)
{
    const int pathLen = aPath.length();
    if (aPath.startsWith(HOME)) {
        const int homeLen = HOME.length();
        if (pathLen == homeLen) {
            // Exactly home path => ~
            return Private::TILDA;
        } else if (aPath.at(homeLen) == '/') {
            // Convert something like /home/user//////foo => ~/foo
            // And something like /home/user////// => ~
            const QString tail(stripLeadingSeparators
                (aPath.right(pathLen - homeLen - 1)));
            return tail.isEmpty() ? Private::TILDA :
                Private::HOME_PREFIX + tail;
        } else {
            // Weird absolute path
            return aPath;
        }
    } else if (aPath.startsWith(QDir::separator()) ||
        aPath.startsWith(Private::TILDA)) {
        // It's either already beautiful or irreparably ugly
        return aPath;
    } else if (aPath.startsWith(Private::DOT_SLASH)) {
        // Replace .///// with ~/
        return Private::HOME_PREFIX + stripLeadingSeparators
            (aPath.right(pathLen - Private::DOT_SLASH.length()));
    } else {
        return Private::HOME_PREFIX + aPath;
    }
}

QStringList BackupUtil::beautifyPathList(QStringList aPathList)
{
    const int n = aPathList.length();
    for (int i = 0; i < n; i++) {
        QString rawPath(aPathList.at(i));
        const QString beautitulPath(beautifyPath(rawPath));
        if (beautitulPath != rawPath) {
            aPathList.replace(i, beautitulPath);
        }
    }
    return aPathList;
}
