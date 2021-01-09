import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.mybackup 1.0

Dialog {
    id: thisDialog

    readonly property int type: BackupListModel.App
    readonly property var selection: applicationModel.desktopFiles(listView.model.selectedRows)

    canAccept: selection.length

    DialogHeader {
        id: header

        spacing: 0
    }

    SilicaListView {
        id: listView

        anchors {
            top: header.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        clip: true
        flickDeceleration: maximumFlickVelocity
        model: HarbourSelectionListModel{
            sourceModel: ApplicationModel {
                id: applicationModel

                knownApps: BackupListModel.apps
            }
        }
        delegate: ListItem {
            id: listDelegate

            readonly property bool isSelected: model.selected
            highlighted: down || isSelected
            contentHeight: Theme.itemSizeMedium
            enabled: !model.knownApp

            BackupItem {
                anchors.fill: parent
                highlighted: listDelegate.highlighted
                enabled: listDelegate.enabled
                type: thisDialog.type
                name: model.name
                appIcon: model.appIcon
                background: false
            }

            onClicked: model.selected = !model.selected
        }

        ViewPlaceholder {
            enabled: applicationModel.ready && listView.count === 0
            //: Placeholder text
            //% "None of the installed applications are integrated with My Backup"
            text: qsTrId("mybackup-add_app-placeholder_text")
            //: Placeholder hint
            //% "Try installing Counter, Foil Notes, Foil Pics or Foil Auth application from either Jolla Store or OpenRepos.net"
            hintText: qsTrId("mybackup-add_app-placeholder_hint")
        }

        VerticalScrollDecorator { }
    }
}
