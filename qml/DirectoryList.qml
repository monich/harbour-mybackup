import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.mybackup 1.0

Item {
    id: thisItem

    width: parent ? parent.width : Screen.width

    property alias path: contentsModel.path
    property alias directoryName: currentDir.text
    readonly property var selection: currentDir.selected ? [ path ] :
        contentsModel.entries(listView.model.selectedRows)

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
                source: Qt.resolvedUrl("images/icon-folder-open.svg")
                highlightColor: Theme.secondaryHighlightColor
            }
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
                sourceModel: DirectoryContentsModel {
                    id: contentsModel
                }
            }
            delegate: IconTextListItem {
                readonly property bool isDirectory: model.type === DirectoryContentsModel.Directory
                readonly property bool isSelected: model.selected
                highlighted: down || isSelected
                text: model.name
                icon {
                    source: Qt.resolvedUrl("images/" + (isDirectory ? "icon-folder.svg" : "icon-file.svg"))
                    highlightColor: Theme.secondaryHighlightColor
                }
                onClicked: {
                    if (isDirectory) {
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
