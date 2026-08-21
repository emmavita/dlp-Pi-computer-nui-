import QtQuick

// On-surface keyboard: the text-input method for this NUI (no physical keyboard,
// no microphone on the target hardware). Keys are Tiles; Main's recursive hit
// test activates the key under the pointer on TAP/DWELL.
Item {
    id: kb
    signal keyPressed(string k)
    signal closed()

    property var rows: ["AZERTYUIOP", "QSDFGHJKLM", "WXCVBN"]

    Column {
        anchors.centerIn: parent
        spacing: 12

        Repeater {
            model: kb.rows
            delegate: Row {
                property string rowStr: modelData
                spacing: 12
                Repeater {
                    model: rowStr.length
                    delegate: Tile {
                        label: rowStr.charAt(index)
                        width: 96
                        height: 96
                        onActivated: kb.keyPressed(label)
                    }
                }
            }
        }

        Row {
            spacing: 12
            anchors.horizontalCenter: parent.horizontalCenter
            Tile {
                label: "Espace"
                width: 300
                height: 84
                onActivated: kb.keyPressed(" ")
            }
            Tile {
                label: "Fermer"
                width: 180
                height: 84
                onActivated: kb.closed()
            }
        }
    }
}
