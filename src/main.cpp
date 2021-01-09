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

#include "Backup.h"
#include "BackupApp.h"

#include <glib.h>
#include <stdio.h>

#define RET_OK (0)
#define RET_CMDLINE (1)
#define RET_ERR (2)

//
// The same executable both runs the app and does the backup/restore.
// We determine the context based on the command line arguments. That's
// done in order to be able to make backup unit rpm a minimal noarch
// containing just text files and no binaries.
//
__attribute__((visibility("default"))) int main(int argc, char *argv[])
{
    int ret = RET_CMDLINE;
    char* action = NULL;
    char* dir = NULL;
    char* home = NULL;

    // Using glib to parse command line arguments (if any)
    GOptionContext* options = g_option_context_new(NULL);
    const GOptionEntry entries[] = {
        { "action", 0, 0, G_OPTION_ARG_STRING, &action,
          "Action to perform (import|export)", "ACTION" },
        { "home-dir", 0, 0, G_OPTION_ARG_FILENAME, &home,
          "Home directory", "DIR" },
        { "dir", 0, 0, G_OPTION_ARG_FILENAME, &dir,
          "Backup directory", "DIR" },
        { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
    };

    g_option_context_add_main_entries(options, entries, NULL);
    g_option_context_set_ignore_unknown_options(options, TRUE);

    // Don't let glib to damage the original args
    int tmp_argc = argc;
    char** tmp_argv = (char**) g_memdup(argv, sizeof(argv[0]) * argc);
    g_option_context_parse(options, &tmp_argc, &tmp_argv, NULL);
    g_free(tmp_argv);

    // Are we started by backup or running the app?
    if (action && dir && home) {
        // Looks like backup/restore action
        const Backup::Action backupAction =
            (!g_strcmp0(action, Backup::ACTION_IMPORT) ||
             !g_strcmp0(action, Backup::ACTION_RESTORE))?
                Backup::ImportAction :
            !g_strcmp0(action, Backup::ACTION_EXPORT) ?
                Backup::ExportAction :
                Backup::NoAction;

        if (backupAction != Backup::NoAction) {
            ret = Backup::run(backupAction, home, dir);
        } else {
            char* help = g_option_context_get_help(options, TRUE, NULL);

            printf("%s", help);
            g_free(help);
        }
    } else {
        // Run the app
        ret = BackupApp::main(argc, argv);
    }

    g_free(dir);
    g_free(home);
    g_free(action);
    return ret;
}
