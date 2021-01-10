NAME = mybackup

#CONFIG += openrepos

openrepos {
    PREFIX = openrepos
    DEFINES += OPENREPOS
} else {
    PREFIX = harbour
}

TARGET = $${PREFIX}-$${NAME}
CONFIG += sailfishapp link_pkgconfig
PKGCONFIG += sailfishapp mlite5 glib-2.0 gio-2.0 gobject-2.0 dconf
QT += qml quick
LIBS += -ldl

QMAKE_CXXFLAGS += -Wno-unused-parameter -Wno-psabi
QMAKE_CFLAGS += -Wno-unused-parameter

TARGET_DATA_DIR = /usr/share/$${TARGET}
TRANSLATIONS_PATH = $${TARGET_DATA_DIR}/translations

CONFIG(debug, debug|release) {
    DEFINES += DEBUG HARBOUR_DEBUG
}

# Directories

HARBOUR_LIB_DIR = $${_PRO_FILE_PWD_}/harbour-lib

OTHER_FILES += \
    LICENSE \
    README.md \
    rpm/*.spec \
    *.json \
    *.desktop \
    js/*.js \
    qml/*.qml \
    qml/images/*.svg \
    icons/*.svg \
    translations/*.ts

# App

HEADERS += \
    src/ApplicationModel.h \
    src/Backup.h \
    src/BackupApp.h \
    src/BackupDefs.h \
    src/BackupList.h \
    src/BackupListModel.h \
    src/BackupUtil.h \
    src/ConfigClient.h \
    src/ConfigGroupModel.h \
    src/ConfigPathModel.h \
    src/DirectoryContentsModel.h \
    src/DirectoryPathModel.h

SOURCES += \
    src/ApplicationModel.cpp \
    src/Backup.cpp \
    src/BackupApp.cpp \
    src/BackupList.cpp \
    src/BackupListItem.cpp \
    src/BackupListModel.cpp \
    src/BackupUtil.cpp \
    src/ConfigClient.cpp \
    src/ConfigGroupModel.cpp \
    src/ConfigPathModel.cpp \
    src/DirectoryContentsModel.cpp \
    src/DirectoryPathModel.cpp \
    src/main.cpp

app_js.files = js/*.js
app_js.path = /usr/share/$${TARGET}/js/
INSTALLS += app_js

# harbour-lib

HARBOUR_LIB_INCLUDE = $${HARBOUR_LIB_DIR}/include
HARBOUR_LIB_SRC = $${HARBOUR_LIB_DIR}/src
HARBOUR_LIB_QML = $${HARBOUR_LIB_DIR}/qml

INCLUDEPATH += \
    $${HARBOUR_LIB_INCLUDE}

HEADERS += \
    $${HARBOUR_LIB_INCLUDE}/HarbourDebug.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourJson.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourOrganizeListModel.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourSelectionListModel.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourSystemInfo.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourTask.h

SOURCES += \
    $${HARBOUR_LIB_SRC}/HarbourJson.cpp \
    $${HARBOUR_LIB_SRC}/HarbourOrganizeListModel.cpp \
    $${HARBOUR_LIB_SRC}/HarbourSelectionListModel.cpp \
    $${HARBOUR_LIB_SRC}/HarbourSystemInfo.cpp \
    $${HARBOUR_LIB_SRC}/HarbourTask.cpp

HARBOUR_QML_COMPONENTS = \
    $${HARBOUR_LIB_QML}/HarbourGrayscaleEffect.qml \
    $${HARBOUR_LIB_QML}/HarbourHighlightIcon.qml

OTHER_FILES += $${HARBOUR_QML_COMPONENTS}

qml_components.files = $${HARBOUR_QML_COMPONENTS}
qml_components.path = $${TARGET_DATA_DIR}/qml/harbour
INSTALLS += qml_components

# Icons

ICON_SIZES = 86 108 128 256
for(s, ICON_SIZES) {
    icon_target = icon_$${s}
    icon_dir = icons/$${s}x$${s}
    $${icon_target}.files = $${icon_dir}/$${TARGET}.png
    $${icon_target}.path = /usr/share/icons/hicolor/$${s}x$${s}/apps
    openrepos {
        $${icon_target}.extra = cp $${icon_dir}/harbour-$${NAME}.png $$eval($${icon_target}.files)
        $${icon_target}.CONFIG += no_check_exist
    }
    INSTALLS += $${icon_target}
}

# Desktop file

openrepos {
    desktop.extra = sed s/harbour/openrepos/g harbour-$${NAME}.desktop > $${TARGET}.desktop
    desktop.CONFIG += no_check_exist
}

# Backup unit

openrepos {
    backup_unit.files = openrepos-$${NAME}.json
    backup_unit.path = /usr/share/jolla-vault/units
    INSTALLS += backup_unit
}

# Translations

TRANSLATION_SOURCES = \
  $${_PRO_FILE_PWD_}/qml \
  $${_PRO_FILE_PWD_}/src

defineTest(addTrFile) {
    rel = translations/harbour-$${1}
    OTHER_FILES += $${rel}.ts

    in = $${_PRO_FILE_PWD_}/$$rel
    out = $${OUT_PWD}/translations/$${PREFIX}-$$1

    s = $$replace(1,-,_)
    lupdate_target = lupdate_$$s
    lrelease_target = lrelease_$$s

    $${lupdate_target}.commands = lupdate -noobsolete -locations none $${TRANSLATION_SOURCES} -ts \"$${in}.ts\" && \
        mkdir -p \"$${OUT_PWD}/translations\" &&  [ \"$${in}.ts\" != \"$${out}.ts\" ] && \
        cp -af \"$${in}.ts\" \"$${out}.ts\" || :

    $${lrelease_target}.target = $${out}.qm
    $${lrelease_target}.depends = $${lupdate_target}
    $${lrelease_target}.commands = lrelease -idbased \"$${out}.ts\"

    QMAKE_EXTRA_TARGETS += $${lrelease_target} $${lupdate_target}
    PRE_TARGETDEPS += $${out}.qm
    qm.files += $${out}.qm

    export($${lupdate_target}.commands)
    export($${lrelease_target}.target)
    export($${lrelease_target}.depends)
    export($${lrelease_target}.commands)
    export(QMAKE_EXTRA_TARGETS)
    export(PRE_TARGETDEPS)
    export(qm.files)
}

LANGUAGES = pl ru sv

addTrFile($${NAME})
for(l, LANGUAGES) {
    addTrFile($${NAME}-$$l)
}

qm.path = $$TRANSLATIONS_PATH
qm.CONFIG += no_check_exist
INSTALLS += qm
