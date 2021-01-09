import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.mybackup 1.0

Dialog {
    id: thisDialog

    canAccept: selection.length

    readonly property int type: BackupListModel.Path
    readonly property var selection: pathView.currentItem.selection

    DialogHeader {
        id: header

        spacing: 0
    }

    SilicaListView {
        id: pathView

        anchors {
            top: header.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        clip: true
        interactive: !scrollAnimation.running
        orientation: ListView.Horizontal
        snapMode: ListView.SnapOneItem
        highlightRangeMode: ListView.StrictlyEnforceRange
        flickDeceleration: maximumFlickVelocity
        quickScrollEnabled: false
        model: DirectoryPathModel { id: pathModel }
        delegate: DirectoryList {
            readonly property bool isCurrentItem: ListView.isCurrentItem
            width: pathView.width
            height: pathView.height
            directoryName: "~" + ((relativePath !== "") ? ("/" + relativePath) : "")
            path: fullPath
            onChangeDir: {
                pathModel.setDepth(index + 1)
                pathModel.enter(name)
                pathView.scrollToEnd()
            }
            onIsCurrentItemChanged: {
                if (!isCurrentItem) {
                    clearSelection()
                }
            }
        }

        function scrollToEnd() {
            scrollAnimation.from = contentX
            scrollAnimation.to = originX + (count - 1) * width
            scrollAnimation.start()
        }

        NumberAnimation {
            id: scrollAnimation

            target: pathView
            property: "contentX"
            duration: 350
            easing.type: Easing.InOutQuad
            alwaysRunToEnd: true
        }
    }
}
