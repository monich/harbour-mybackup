import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.mybackup 1.0

import "harbour"

Page {
    id: thisPage

    property int type
    property string name
    property alias title: header.title
    property alias path: pathLabel.text
    property url iconSource
    readonly property int imageSize: Theme.iconSizeLarge
    property var pathList
    property var configList

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height + Theme.paddingLarge

        Column {
            id: content

            width: parent.width

            PageHeader {
                id: header
                description: (type === BackupListModel.App) ? name : ""
            }

            Row {
                x: Theme.horizontalPageMargin
                spacing: Theme.paddingLarge

                Item {
                    width: imageSize
                    height: imageSize

                    Loader {
                        visible: active
                        active: type === BackupListModel.App
                        anchors.fill: parent
                        sourceComponent: Component {
                            Image {
                                sourceSize: Qt.size(imageSize, imageSize)
                                smooth: true
                                source: iconSource
                                fillMode: Image.PreserveAspectFit
                                visible: status === Image.Ready
                            }
                        }
                    }

                    Loader {
                        visible: active
                        active: type !== BackupListModel.App
                        anchors.fill: parent
                        sourceComponent: Component {
                            HarbourHighlightIcon {
                                sourceSize: Qt.size(imageSize, imageSize)
                                smooth: true
                                source: iconSource
                                fillMode: Image.PreserveAspectFit
                                visible: status === Image.Ready
                            }
                        }
                    }
                }

                Label {
                    id: pathLabel

                    width: thisPage.width - 2 * parent.x - x
                    height: Math.max(imageSize, implicitHeight)
                    font.pixelSize: Theme.fontSizeSmall
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    color: Theme.secondaryColor
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    textFormat: Text.PlainText
                    text: description
                }
            }

            BackupItemInfoSection {
                //: Backup item info section
                //% "Files"
                title: qsTrId("mybackup-item_info-section-files")
                iconSource: "images/icon-folder.svg"
                model: pathList
            }

            BackupItemInfoSection {
                //: Backup item info section
                //% "Configuration"
                title: qsTrId("mybackup-item_info-section-config")
                iconSource: "images/icon-config.svg"
                model: configList
            }
        }

        VerticalScrollDecorator { }
    }
}

