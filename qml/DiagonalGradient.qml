import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: thisItem

    property color color: Theme.rgba(Theme.highlightBackgroundColor, 0.2)
    clip: true

    Rectangle {
        width: 2 * height
        height: thisItem.height * 1.412136
        gradient: Gradient {
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 1.0; color: thisItem.color }
        }
        transform: [
            Rotation {
                angle: 45
                origin {
                    x: 0
                    y: 0
                }
            },
            Translate {
                y:  - thisItem.height
            }
        ]
    }
}
