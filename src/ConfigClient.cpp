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

#include <client/dconf-client.h>

#include "ConfigClient.h"

ConfigClient::ConfigClient(struct _DConfClient* aDConf) :
    iClient(aDConf ?  (DConfClient*) g_object_ref(aDConf) : Q_NULLPTR)
{
}

ConfigClient::ConfigClient(const ConfigClient& aClient) :
    iClient(aClient.iClient ? (DConfClient*) g_object_ref(aClient.iClient) : Q_NULLPTR)
{
}

ConfigClient::ConfigClient() :
    iClient(Q_NULLPTR)
{
}

ConfigClient::~ConfigClient()
{
    if (iClient) {
        g_object_unref(iClient);
    }
}

ConfigClient& ConfigClient::operator = (const ConfigClient& aClient)
{
    if (iClient != aClient.iClient) {
        if (iClient) {
            g_object_unref(iClient);
        }
        iClient = aClient.iClient;
        if (iClient) {
            g_object_ref(iClient);
        }
    }
    return *this;
}

ConfigClient ConfigClient::create()
{
    DConfClient* client = dconf_client_new();
    ConfigClient ref(client);
    g_object_unref(client);
    return ref;
}

QStringList ConfigClient::list(QString aDir) const
{
    QStringList out;
    if (iClient) {
        const QByteArray dir(aDir.toLocal8Bit());
        gint n = 0;
        gchar** entries = dconf_client_list(iClient, dir.constData(), &n);
        if (entries) {
            for (char** ptr = entries; *ptr; ptr++) {
                out.append(QString(QLatin1String(*ptr)));
            }
            g_strfreev(entries);
        }
    }
    return out;
}

void ConfigClient::sync()
{
    if (iClient) {
        dconf_client_sync(iClient);
    }
}
