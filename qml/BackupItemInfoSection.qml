import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Column {
    id: section

    property alias title: sectionHeader.text
    property alias iconSource: icon.source
    property alias model: repeater.model

    width: parent.width
    visible: repeater.count > 0

    SectionHeader { id: sectionHeader }

    Row {
        x: Theme.horizontalPageMargin
        spacing: Theme.paddingLarge

        HarbourHighlightIcon {
            id: icon

            highlightColor: Theme.secondaryColor
            sourceSize: Qt.size(imageSize, imageSize)
            smooth: true
            fillMode: Image.PreserveAspectFit
            visible: status === Image.Ready
        }

        Column {
            id: listColumn

            width: section.width - 2 * parent.x - x

            Repeater {
                id: repeater

                delegate: Column {
                    width: parent.width

                    Rectangle {
                        color: (index % 2) ? Theme.rgba(label.color, 0.05) : "transparent"
                        x: -Theme.paddingMedium
                        width: parent.width - x
                        height: label.implicitHeight + Theme.paddingMedium

                        Label {
                            id: label

                            x: Theme.paddingMedium
                            y: Theme.paddingMedium/2
                            width: parent.width - x
                            font.pixelSize: Theme.fontSizeSmall
                            horizontalAlignment: Text.AlignLeft
                            color: Theme.secondaryColor
                            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                            textFormat: Text.PlainText
                            text: modelData
                        }
                    }
                }
            }
        }
    }
}
