import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.mybackup 1.0

import "harbour"

Rectangle {
    id: thisItem

    property bool highlighted
    property int type
    property string name
    property string path
    property string appIcon
    property alias separator: separator.visible
    property alias background: background.visible

    readonly property var iconSource: type === BackupListModel.App ? appIcon :
        iconImage.item ? iconImage.item.source : ""
    readonly property int imageSize: Theme.iconSizeMedium
    readonly property bool isHomePath: path === Home + "/"
    readonly property bool pathEndsWithSlash: {
        var pos = path.lastIndexOf("/")
        return (pos >= 0 && pos === path.length - 1)
    }
    readonly property string description: {
        switch (type) {
        case BackupListModel.App:
            //: Backup item description
            //% "Application"
            return qsTrId("mybackup-entry_description-application")
        case BackupListModel.Path:
            return pathEndsWithSlash ? (
                //: Backup item description
                //% "Home directory"
                isHomePath ? qsTrId("mybackup-entry_description-home") :
                //: Backup item description
                //% "Directory"
                qsTrId("mybackup-entry_description-directory")) :
                //: Backup item description
                //% "File"
                qsTrId("mybackup-entry_description-file")
        case BackupListModel.Config:
            return pathEndsWithSlash ?
                //: Backup item description
                //% "Configuration group"
                qsTrId("mybackup-entry_description-configuration_group") :
                //: Backup item description
                //% "Configuration value"
                qsTrId("mybackup-entry_description-configuration_value")
        default:
            return ""
        }
    }

    color: "transparent"

    Row {
        id: background

        height: thisItem.height
        readonly property real iconAreaWidth: 2 * Theme.horizontalPageMargin + imageSize

        DiagonalGradient {
            width: background.iconAreaWidth
            height: parent.height
        }
        DiagonalGradient {
            width: thisItem.width - background.iconAreaWidth
            height: parent.height
        }
    }

    Row {
        x: Theme.horizontalPageMargin
        width: parent.width - x
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.horizontalPageMargin + Theme.paddingLarge
        opacity: thisItem.enabled ? 1 : 0.3

        Item {
            id: imageItem

            width: imageSize
            height: imageSize
            anchors.verticalCenter: parent.verticalCenter

            Loader {
                id: appImage

                visible: active
                active: appIcon.length > 0
                anchors.fill: parent
                sourceComponent: Component {
                    Image {
                        id: appIconImage

                        sourceSize: Qt.size(imageSize, imageSize)
                        smooth: true
                        source: appIcon
                        fillMode: Image.PreserveAspectFit
                        visible: status === Image.Ready
                        layer.enabled: !thisItem.enabled
                        layer.effect: HarbourGrayscaleEffect {
                            source: appIconImage
                        }
                    }
                }
            }

            Loader {
                id: iconImage

                visible: active
                active: appIcon.length === 0
                anchors.fill: parent
                sourceComponent: Component {
                    HarbourHighlightIcon {
                        sourceSize: Qt.size(imageSize, imageSize)
                        smooth: true
                        source: {
                            switch (type) {
                            case BackupListModel.App:
                                return Qt.resolvedUrl("images/icon-app.svg")
                            case BackupListModel.Path:
                                return Qt.resolvedUrl("images/" + (pathEndsWithSlash ? "icon-folder.svg" : "icon-file.svg"))
                            case BackupListModel.Config:
                                return Qt.resolvedUrl("images/" + (pathEndsWithSlash ? "icon-config-group.svg" : "icon-config-entry.svg"))
                            default:
                                return ""
                            }
                        }
                        fillMode: Image.PreserveAspectFit
                        visible: status === Image.Ready
                    }
                }
            }
        }

        Column {
            width: parent.width - x
            anchors.verticalCenter: parent.verticalCenter

            Label {
                id: nameLabel

                width: parent.width
                truncationMode: TruncationMode.Fade
                font {
                    pixelSize: Theme.fontSizeSmall
                    bold: true
                }
                color: (thisItem.enabled && highlighted) ? Theme.secondaryHighlightColor : Theme.secondaryColor
                textFormat: Text.PlainText
                text: name
            }

            Label {
                width: parent.width
                font.pixelSize: Theme.fontSizeExtraSmall
                truncationMode: TruncationMode.Fade
                horizontalAlignment: Text.AlignLeft
                color: (thisItem.enabled && highlighted) ? Theme.secondaryHighlightColor : Theme.secondaryColor
                textFormat: Text.PlainText
                text: description
            }
        }
    }

    Separator {
        id: separator

        visible: false
        width: parent.width
        anchors.bottom: parent.bottom
        color: Theme.highlightColor
        horizontalAlignment: Qt.AlignHCenter
    }
}
