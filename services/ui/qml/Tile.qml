import QtQuick

// Activatable target. activate() is called by Main's hit test on TAP/DWELL.
Rectangle {
    id: tile
    property string label: ""
    signal activated()

    width: 220
    height: 140
    radius: 16
    color: "#16202b"
    border.color: "#22303f"
    border.width: 1

    Text {
        anchors.centerIn: parent
        text: tile.label
        color: "#dfeaf2"
        font.pixelSize: 24
    }

    function activate() {
        flash.restart();
        tile.activated();
    }

    SequentialAnimation {
        id: flash
        ColorAnimation { target: tile; property: "color"; to: "#2b6cff"; duration: 90 }
        ColorAnimation { target: tile; property: "color"; to: "#16202b"; duration: 260 }
    }
}
