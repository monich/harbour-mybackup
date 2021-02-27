NAME = mybackup

CONFIG += openrepos

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
    icons/*.svg

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
    src = $${_PRO_FILE_PWD_}/$${icon_dir}/harbour-$${NAME}.png
    dest = /usr/share/icons/hicolor/$${s}x$${s}/apps
    $${icon_target}.path = $${dest}
    $${icon_target}.commands = $(INSTALL_FILE) \"$${src}\" $(INSTALL_ROOT)$${dest}/$${TARGET}.png
    INSTALLS += $${icon_target}
}

# Desktop file

openrepos {
    desktop.files = $${OUT_PWD}/$${TARGET}.desktop
    desktop.extra = sed s/harbour/openrepos/g $${_PRO_FILE_PWD_}/harbour-$${NAME}.desktop > $${desktop.files}
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
    export(OTHER_FILES)

    in = $${_PRO_FILE_PWD_}/$$rel
    out = $${OUT_PWD}/translations/$${PREFIX}-$$1

    s = $$replace(1,-,_)
    lupdate_target = lupdate_$$s
    qm_target = qm_$$s

    $${lupdate_target}.commands = lupdate -noobsolete -locations none $${TRANSLATION_SOURCES} -ts \"$${in}.ts\" && \
        mkdir -p \"$${OUT_PWD}/translations\" &&  [ \"$${in}.ts\" != \"$${out}.ts\" ] && \
        cp -af \"$${in}.ts\" \"$${out}.ts\" || :

    $${qm_target}.path = $$TRANSLATIONS_PATH
    $${qm_target}.depends = $${lupdate_target}
    $${qm_target}.commands = lrelease -idbased \"$${out}.ts\" && \
        $(INSTALL_FILE) \"$${out}.qm\" $(INSTALL_ROOT)$${TRANSLATIONS_PATH}/

    QMAKE_EXTRA_TARGETS += $${lupdate_target} $${qm_target}
    INSTALLS += $${qm_target}

    export($${lupdate_target}.commands)
    export($${qm_target}.path)
    export($${qm_target}.depends)
    export($${qm_target}.commands)
    export(QMAKE_EXTRA_TARGETS)
    export(INSTALLS)
}

LANGUAGES = pl ru sv zh_CN

addTrFile($${NAME})
for(l, LANGUAGES) {
    addTrFile($${NAME}-$$l)
}

