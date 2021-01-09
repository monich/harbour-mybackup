import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.mybackup 1.0

Item {
    id: thisItem

    width: parent ? parent.width : Screen.width

    property string path
    property alias configClient: groupModel.configClient
    readonly property var selection: currentDir.selected ? [ path ] :
        groupModel.entries(listView.model.selectedRows)

    signal changeDir(var name)

    function clearSelection() {
        currentDir.selected = false
        listView.model.clearSelection()
    }

    Column {
        width:parent.width

        IconTextListItem {
            id: currentDir

            property bool selected
            highlighted: down || selected
            icon {
                source: Qt.resolvedUrl("images/icon-config-group-open.svg")
                highlightColor: Theme.secondaryHighlightColor
            }
            text: (path.length > 1) ? path.substr(0, path.length - 1) : path
            font.bold: true
            onClicked: selected = !selected
            onSelectedChanged: {
                if (selected) {
                    listView.model.clearSelection()
                }
            }
        }

        Separator {
            id: separator

            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            color: Theme.highlightColor
            horizontalAlignment: Qt.AlignLeft
        }

        SilicaListView {
            id: listView

            width: parent.width
            height: thisItem.height - y
            flickDeceleration: maximumFlickVelocity
            clip: true
            model: HarbourSelectionListModel{
                sourceModel: ConfigGroupModel {
                    id: groupModel

                    dir: path
                }
            }
            delegate: IconTextListItem {
                readonly property bool isGroup: model.type === ConfigGroupModel.Group
                readonly property bool isSelected: model.selected
                highlighted: down || isSelected
                text: model.name
                icon {
                    source: Qt.resolvedUrl("images/" + (isGroup ? "icon-config-group.svg" : "icon-config-entry.svg"))
                    highlightColor: Theme.secondaryHighlightColor
                }
                onClicked: {
                    if (isGroup) {
                        thisItem.changeDir(model.name)
                    } else {
                        model.selected = !model.selected
                    }
                }
                onIsSelectedChanged: {
                    if (isSelected) {
                        currentDir.selected = false
                    }
                }
            }

            VerticalScrollDecorator { }
        }
    }
}
