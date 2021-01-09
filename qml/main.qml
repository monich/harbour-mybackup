import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.mybackup 1.0

ApplicationWindow {
    id: window

    allowedOrientations: Orientation.All
    initialPage: Component { MainPage { allowedOrientations: window.allowedOrientations } }
    cover: Component {
        CoverPage {
            onNewItem: {
                while (pageStack.pop(null, PageStackAction.Immediate));
                pageStack.push("BackupAddPage.qml", {
                    allowedOrientations: allowedOrientations,
                    parentPage: pageStack.currentPage
                }, PageStackAction.Immediate).add.connect(function(type,selection) {
                    BackupListModel.add(type, selection)
                })
                activate()
            }
        }
    }
}
