import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.mybackup 1.0

import "harbour"

CoverBackground {
    id: cover

    readonly property date lastBackup: BackupListModel.lastBackup
    readonly property date lastRestore: BackupListModel.lastRestore
    readonly property bool haveLastBackup: !isNaN(lastBackup)
    readonly property bool haveLastRestore: !isNaN(lastRestore)

    signal newItem()

    function dateString(date) {
        return date.toLocaleDateString(Qt.locale(), "dd.MM.yyyy")
    }

    function timeString(time) {
        return time.toLocaleTimeString(Qt.locale(), "hh:mm")
    }

    function dateTimeString(dateTime,sep) {
        return dateString(dateTime) + sep + timeString(dateTime)
    }

    Item {
        x: Theme.paddingLarge
        y: Theme.paddingLarge
        width: parent.width - x*2
        height: parent.height - y - Theme.itemSizeSmall/cover.scale

        HarbourHighlightIcon {
            anchors {
                horizontalCenter: parent.horizontalCenter
                bottom: parent.bottom
                bottomMargin: Theme.paddingLarge
            }
            highlightColor: Theme.secondaryHighlightColor
            sourceSize.width: Math.round(cover.width * 88 / 48)
            source: "images/app-icon.svg"
            asynchronous: true
            opacity: 0.05
        }

        Label {
            x: Theme.paddingLarge
            width: parent.width - x*2
            anchors.centerIn: parent
            visible: !BackupListModel.configured
            horizontalAlignment: Text.AlignHCenter
            color: Theme.secondaryColor
            minimumPixelSize: Theme.fontSizeMedium
            wrapMode: Text.Wrap
            fontSizeMode: Text.Fit
            //: Cover page label
            //% "Not configured"
            text: qsTrId("mybackup-cover-not_configured")
        }

        Column {
            width: parent.width
            anchors.verticalCenter: parent.verticalCenter
            visible: BackupListModel.configured
            spacing: Theme.paddingLarge

            Column {
                visible: haveLastBackup
                width: parent.width

                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeLarge
                    minimumPixelSize: Theme.fontSizeMedium
                    wrapMode: Text.Wrap
                    fontSizeMode: Text.Fit
                    //: Cover page label
                    //% "Last backup"
                    text: qsTrId("mybackup-cover-last_backup")
                }

                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    minimumPixelSize: Theme.fontSizeExtraSmall
                    wrapMode: Text.Wrap
                    fontSizeMode: Text.Fit
                    text: cover.dateTimeString(lastBackup, "\n")
                }
            }

            Column {
                visible: haveLastRestore
                width: parent.width

                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeLarge
                    minimumPixelSize: Theme.fontSizeMedium
                    wrapMode: Text.Wrap
                    fontSizeMode: Text.Fit
                    //: Cover page label
                    //% "Last restore"
                    text: qsTrId("mybackup-cover-last_restore")
                }

                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    minimumPixelSize: Theme.fontSizeExtraSmall
                    wrapMode: Text.Wrap
                    fontSizeMode: Text.Fit
                    text: cover.dateTimeString(lastRestore, "\n")
                }
            }

            Label {
                width: parent.width
                visible: !haveLastBackup && !haveLastRestore
                horizontalAlignment: Text.AlignHCenter
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeLarge
                minimumPixelSize: Theme.fontSizeMedium
                wrapMode: Text.Wrap
                fontSizeMode: Text.Fit
                //: Cover page label
                //% "Nothing has been backed up or restored yet"
                text: qsTrId("mybackup-cover-nothing_backed_up")
            }
        }
    }

    CoverActionList {
        enabled: BackupListModel.configured
        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: cover.newItem()
        }
    }
}
