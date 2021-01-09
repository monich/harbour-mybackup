import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.mybackup 1.0

import "harbour"

Page {
    id: thisPage

    // Very subtle background image
    HarbourHighlightIcon {
        readonly property int screenWidth: isLandscape ? Screen.height : Screen.width
        readonly property int screenHeight: isLandscape ? Screen.width : Screen.height
        x: Math.round((screenWidth - width)/2)
        y: Math.round((screenHeight - height + Theme.itemSizeSmall)/2) // Slightly below the center
        width: Math.min(Screen.width, Screen.height) - 2 * Theme.itemSizeSmall
        height: width
        sourceSize.width: width
        highlightColor: Theme.primaryColor
        source: "images/app-icon.svg"
        opacity: 0.02
    }

    Loader {
        active: !BackupListModel.configured
        anchors.fill: parent
        sourceComponent: Component {
            BackupConfigureView {
            }
        }
    }

    Loader {
        active: BackupListModel.configured
        anchors.fill: parent
        sourceComponent: Component {
            BackupListView {
                page: thisPage
            }
        }
    }

}
