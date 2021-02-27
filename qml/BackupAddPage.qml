import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Page {
    id: thisPage

    property Page parentPage

    signal add(var type, var selection)

    SilicaListView {
        anchors.fill: parent

        header: PageHeader {
            id: header

            //: Add backup item page title
            //% "Add backup item"
            title: qsTrId("mybackup-add_page-title")
        }

        model: ListModel {
            ListElement {
                //: Add backup item page label (backup item type)
                //% "Application"
                title: qsTrId("mybackup-add_page-item_app")
                icon: "icon-app.svg"
                pageSource: "BackupAddAppDialog.qml"
            }
            ListElement {
                //: Add backup item page label (backup item type)
                //% "File or directory"
                title: qsTrId("mybackup-add_page-item_path")
                icon: "icon-folder.svg"
                pageSource: "BackupAddPathDialog.qml"
            }
            ListElement {
                //: Add backup item page label (backup item type)
                //% "Configuration"
                title: qsTrId("mybackup-add_page-item_config")
                icon: "icon-config.svg"
                pageSource: "BackupAddConfigDialog.qml"
            }
        }

        delegate: IconTextListItem {
            iconSize: height
            text: model.title
            icon.source: Qt.resolvedUrl("images/" + model.icon)
            onClicked: {
                var dialog = pageStack.push(model.pageSource, {
                    allowedOrientations: thisPage.allowedOrientations,
                    acceptDestinationAction: PageStackAction.Pop,
                    acceptDestination: parentPage
                })
                dialog.accepted.connect(function() {
                    var n = dialog.selection.length
                    for (var i = 0; i < n; i++) {
                        thisPage.add(dialog.type, dialog.selection[i])
                    }
                })
            }
        }

        VerticalScrollDecorator { }
    }
}
