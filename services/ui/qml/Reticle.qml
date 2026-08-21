import QtQuick

// Pointer reticle. Confidence (0..1) modulates opacity so the user sees when
// tracking degrades — the only feedback channel available (no audio/haptics).
Item {
    id: r
    property real confidence: 1.0
    width: 44
    height: 44
    z: 1000

    Rectangle {
        anchors.centerIn: parent
        width: 18; height: 18; radius: 9
        color: "transparent"
        border.width: 2
        border.color: Qt.rgba(0.31, 0.90, 1.0, Math.max(0.3, r.confidence))
    }
    Rectangle {
        anchors.centerIn: parent
        width: 4; height: 4; radius: 2
        color: Qt.rgba(0.31, 0.90, 1.0, Math.max(0.3, r.confidence))
    }
}
