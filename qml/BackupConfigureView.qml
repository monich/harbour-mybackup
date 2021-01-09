import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Item {
    anchors.fill: parent

    HarbourHighlightIcon {
        y: Math.round((button.y - height)/2)
        anchors.horizontalCenter: parent.horizontalCenter
        sourceSize: Qt.size(Theme.itemSizeHuge, Theme.itemSizeHuge)
        source: "images/app-icon.svg"
        fillMode: Image.PreserveAspectFit
        asynchronous: true
    }

    Button {
        id: button

        anchors.centerIn: parent
        //: Button label
        //% "Install backup unit"
        text: qsTrId("mybackup-configure-button")
    }
}
