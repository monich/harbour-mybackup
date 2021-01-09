import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.mybackup 1.0

SilicaListView {
    id: backupItemList

    property Item dragItem
    property Page page

    readonly property real dragThresholdX: Theme.itemSizeSmall/2
    readonly property real dragThresholdY: Theme.itemSizeSmall/5
    readonly property real noDragThresholdY: Theme.itemSizeMedium
    readonly property real maxContentY: Math.max(originY + contentHeight - height, originY)

    anchors.fill: parent

    model: HarbourOrganizeListModel {
        id: listModel

        sourceModel: BackupListModel
    }

    header: PageHeader {
        id: header

        //: Main page title
        //% "Backup items"
        title: qsTrId("mybackup-main_page-title")

        BackupListSeparator {
            anchors.bottom: parent.bottom
        }
    }

    delegate: ListItem {
        id: listDelegate

        readonly property int modelIndex: index
        readonly property bool dragging: dragItem === listDelegate

        contentHeight: Theme.itemSizeMedium

        menu: Component {
            id: contextMenu
            ContextMenu {
                MenuItem {
                    //: Context menu item
                    //% "Remove"
                    text: qsTrId("mybackup-menu-remove")
                    onClicked: remove()
                }
            }
        }

        ListView.onRemove: animateRemoval(listDelegate)

        function remove() {
            //: Remorse item text
            //% "Removing"
            remorseAction(qsTrId("mybackup-remorse-removing"), function() {
                BackupListModel.removeAt(modelIndex)
            })
        }

        Item {
            id: draggableItem

            width: listDelegate.width
            height: listDelegate.contentHeight
            scale: listDelegate.dragging ? 1.05 : 1

            BackupItem {
                id: backupItem

                anchors.fill: parent
                highlighted: listDelegate.highlighted
                type: model.type
                path: model.path
                name: model.name
                appIcon: model.appIcon ? model.appIcon : ""
                color: listDelegate.dragging ? Theme.rgba(Theme.highlightBackgroundColor, 0.2) : "transparent"
                Behavior on color { ColorAnimation { duration: 150 } }
            }

            Connections {
                target: listDelegate.dragging ? backupItemList : null
                onContentYChanged: draggableItem.handleScrolling()
            }

            onYChanged: handleScrolling()

            function handleScrolling() {
                if (listDelegate.dragging) {
                    var i = Math.floor((y + backupItemList.contentY + height/2)/height)
                    if (i >= 0 && i !== modelIndex) {
                        // Swap the items
                        backupItemList.model.dragPos = i
                    }
                    var bottom = y + height * 3 / 2
                    if (bottom > backupItemList.height) {
                        // Scroll the list down
                        scrollAnimation.stop()
                        scrollAnimation.from = backupItemList.contentY
                        scrollAnimation.to = Math.min(backupItemList.maxContentY, backupItemList.contentY + bottom + height - backupItemList.height)
                        if (scrollAnimation.to > scrollAnimation.from) {
                            scrollAnimation.start()
                        }
                    } else {
                        var top = y - height / 2
                        if (top < 0) {
                            // Scroll the list up
                            scrollAnimation.stop()
                            scrollAnimation.from = backupItemList.contentY
                            scrollAnimation.to = Math.max(backupItemList.originY, backupItemList.contentY + top - height)
                            if (scrollAnimation.to < scrollAnimation.from) {
                                scrollAnimation.start()
                            }
                        }
                    }
                }
            }

            states: [
                State {
                    name: "dragging"
                    when: listDelegate.dragging
                    ParentChange {
                        target: draggableItem
                        parent: backupItemList
                        y: draggableItem.mapToItem(backupItemList, 0, 0).y
                    }
                },
                State {
                    name: "normal"
                    when: !listDelegate.dragging
                    ParentChange {
                        target: draggableItem
                        parent: listDelegate.contentItem
                        y: 0
                    }
                }
            ]

            Behavior on scale {
                NumberAnimation {
                    duration: 150
                    easing.type: Easing.InOutQuad
                }
            }
        }

        drag.target: dragging ? draggableItem : null
        drag.axis: Drag.YAxis

        property real pressX
        property real pressY
        property bool detectingDrag

        onDraggingChanged: {
            if (dragging) {
                detectingDrag = false
            }
        }

        onPressed: {
            cancelDrag()
            dragTimer.restart()
            pressX = mouseX
            pressY = mouseY
            detectingDrag = true
        }

        onClicked: {
            detectingDrag = false
            cancelDrag()
            pageStack.push("BackupItemInfoPage.qml", {
                allowedOrientations: page.allowedOrientations,
                iconSource: backupItem.iconSource,
                title: backupItem.description,
                name: model.name,
                path: model.path,
                type: model.type,
                pathList: model.appPathList,
                configList: model.appConfigList
            })
        }
        onPressAndHold: {
            detectingDrag = false
            cancelDrag()
        }
        onReleased: stopDrag()
        onCanceled: stopDrag()
        onMouseXChanged: {
            if (detectingDrag && pressed && !menuOpen && !dragTimer.running && Math.abs(mouseX - pressX) > backupItemList.dragThresholdX) {
                if (Math.abs(mouseY - pressY) < backupItemList.noDragThresholdY) {
                    startDrag()
                } else {
                    detectingDrag = false
                }
            }
        }
        onMouseYChanged: {
            if (detectingDrag && pressed && !menuOpen && !dragTimer.running) {
                var deltaY = Math.abs(mouseY - pressY)
                if (deltaY > backupItemList.dragThresholdY) {
                    if (deltaY < backupItemList.noDragThresholdY) {
                        startDrag()
                    } else {
                        detectingDrag = false
                    }
                }
            }
        }

        function stopDrag() {
            detectingDrag = false
            if (dragItem === listDelegate) {
                dragItem = null
                listModel.dragIndex = -1
                dragTimer.stop()
            }
        }

        function startDrag() {
            detectingDrag = false
            dragItem = listDelegate
            listModel.dragIndex = modelIndex
        }
    }

    footer: BackgroundItem {
        id: footerItem

        BackupListSeparator {
            visible: backupItemList.count > 0
        }

        Row {
            id: footerItemContent

            readonly property color contentColor: footerItem.highlighted ? Theme.highlightColor : Theme.primaryColor

            x: Theme.horizontalPageMargin
            height: parent.height
            spacing: Theme.horizontalPageMargin + Theme.paddingLarge

            Item {
                // Aligh with BackupItem content
                width: Theme.iconSizeMedium
                height: parent.height

                Image {
                    anchors.centerIn: parent
                    source: "image://theme/icon-m-add?" + footerItemContent.contentColor
                }
            }

            Label {
                readonly property int maxWidth: backupItemList.width - x - 2 * footerItemContent.x

                //: Pressable footer label
                //% "Add new item"
                text: qsTrId("mybackup-main_page-add_item")
                width: Math.min(implicitWidth, maxWidth)
                anchors.verticalCenter: parent.verticalCenter
                truncationMode: TruncationMode.Fade
                color: footerItemContent.contentColor
            }
        }

        onClicked: {
            pageStack.push("BackupAddPage.qml", {
                allowedOrientations: page.allowedOrientations,
                parentPage: page
            }).add.connect(function(type,selection) {
                BackupListModel.add(type, selection)
            })
        }
    }

    moveDisplaced: Transition {
        NumberAnimation {
            properties: "x,y"
            duration: 50
            easing.type: Easing.InOutQuad
        }
    }

    NumberAnimation {
        id: scrollAnimation

        target: backupItemList
        properties: "contentY"
        duration: 75
        easing.type: Easing.InQuad
    }

    Timer {
        id: dragTimer
        interval: 150
    }

    function cancelDrag() {
        dragItem = null
        listModel.dragIndex = -1
        dragTimer.stop()
    }

    VerticalScrollDecorator { }
}
