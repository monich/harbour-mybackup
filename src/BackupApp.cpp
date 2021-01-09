/*
 * Copyright (C) 2020-2021 Jolla Ltd.
 * Copyright (C) 2020-2021 Slava Monich <slava@monich.com>
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

#include "BackupApp.h"
#include "BackupUtil.h"
#include "BackupListModel.h"
#include "ApplicationModel.h"
#include "ConfigGroupModel.h"
#include "ConfigPathModel.h"
#include "DirectoryContentsModel.h"
#include "DirectoryPathModel.h"

#include "HarbourOrganizeListModel.h"
#include "HarbourSelectionListModel.h"
#include "HarbourSystemInfo.h"
#include "HarbourDebug.h"

#include <sailfishapp.h>

#include <QGuiApplication>
#include <QQuickView>
#include <QLocale>
#include <QTranslator>
#include <QStandardPaths>

#define APP_NAME "harbour-mybackup"
#define APP_QML_IMPORT  "harbour.mybackup"
#define APP_QML_IMPORT_V1 1
#define APP_QML_IMPORT_V2 0

#define REGISTER_SINGLETON(class,uri,v1,v2) \
    qmlRegisterSingletonType<class>(uri, v1, v2, #class, class::createSingleton)
#define REGISTER_TYPE(class,uri,v1,v2) \
    qmlRegisterType<class>(uri, v1, v2, #class)

static void register_types(const char* uri, int v1, int v2)
{
    REGISTER_SINGLETON(HarbourSystemInfo, uri, v1, v2);
    REGISTER_SINGLETON(BackupListModel, uri, v1, v2);
    REGISTER_TYPE(HarbourOrganizeListModel, uri, v1, v2);
    REGISTER_TYPE(HarbourSelectionListModel, uri, v1, v2);
    REGISTER_TYPE(ApplicationModel, uri, v1, v2);
    REGISTER_TYPE(ConfigGroupModel, uri, v1, v2);
    REGISTER_TYPE(ConfigPathModel, uri, v1, v2);
    REGISTER_TYPE(DirectoryContentsModel, uri, v1, v2);
    REGISTER_TYPE(DirectoryPathModel, uri, v1, v2);
}

int BackupApp::main(int argc, char *argv[])
{
    QGuiApplication* app = SailfishApp::application(argc, argv);

    app->setApplicationName(APP_NAME);
    register_types(APP_QML_IMPORT, APP_QML_IMPORT_V1, APP_QML_IMPORT_V2);

    // Load translations
    QLocale locale;
    QTranslator* tr = new QTranslator(app);
    const QString transDir = SailfishApp::pathTo("translations").toLocalFile();
    const QString transFile(APP_NAME);
    if (tr->load(locale, transFile, "-", transDir) ||
        tr->load(transFile, transDir)) {
        app->installTranslator(tr);
    } else {
        HDEBUG("Failed to load translator for" << locale);
        delete tr;
    }

    // Create the view
    QQuickView* view = SailfishApp::createView();
    QQmlContext* context = view->rootContext();

    // Context properties
    context->setContextProperty("Home", BackupUtil::HOME);

    // Initialize the view and show it
    //: Application title
    //% "My Backup"
    view->setTitle(qtTrId("mybackup-app_name"));
    view->setSource(SailfishApp::pathTo("qml/main.qml"));
    view->showFullScreen();

    int ret = app->exec();

    delete view;
    delete app;
    return ret;
}
