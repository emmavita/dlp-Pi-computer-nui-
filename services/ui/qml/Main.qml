import QtQuick
import QtQuick.Window

Window {
    id: root
    visible: true
    visibility: Window.FullScreen
    color: "#0b0f14"
    title: "NUI"

    // Authoritative pointer from the engine (normalized 0..1 -> pixels).
    readonly property real px: bus.pointerX * width
    readonly property real py: bus.pointerY * height

    // Minimal page stack: 0 = home, 1 = keyboard.
    property int page: 0

    // Generic recursive hit test: activates the top-most item under (sx,sy)
    // that exposes an activate() function. Handles arbitrary nesting, so the
    // same code drives home tiles and keyboard keys.
    function hitAt(node, sx, sy) {
        if (!node)
            return false;
        var kids = node.children;
        if (kids) {
            for (var i = kids.length - 1; i >= 0; --i)
                if (hitAt(kids[i], sx, sy))
                    return true;
        }
        if (node.activate !== undefined && node.width > 0 && node.height > 0) {
            var p = node.mapFromItem(root, sx, sy);
            if (p.x >= 0 && p.y >= 0 && p.x <= node.width && p.y <= node.height) {
                node.activate();
                return true;
            }
        }
        return false;
    }

    function hitTest(sx, sy) {
        var pageRoot = (root.page === 0) ? homeLoader.item : kbLoader.item;
        hitAt(pageRoot, sx, sy);
    }

    // Gesture ids mirror enum nui_gesture_type in proto/nui_events.h.
    function handleGesture(type, nx, ny, p0, p1) {
        var sx = nx * width, sy = ny * height;
        switch (type) {
        case 1:  // TAP
        case 10: // DWELL_SELECT
            hitTest(sx, sy);
            break;
        case 8:  // HOME
        case 9:  // BACK
            root.page = 0;
            break;
        default:
            break;
        }
    }

    Connections {
        target: bus
        function onGesture(type, x, y, param0, param1) {
            root.handleGesture(type, x, y, param0, param1);
        }
    }

    Loader {
        id: homeLoader
        anchors.fill: parent
        active: root.page === 0
        sourceComponent: homePage
    }
    Loader {
        id: kbLoader
        anchors.fill: parent
        active: root.page === 1
        sourceComponent: kbPage
    }

    // ---- Home page ----
    Component {
        id: homePage
        Item {
            Column {
                anchors.centerIn: parent
                spacing: 24
                Text {
                    text: "Accueil"
                    color: "#7fb8cc"
                    font.pixelSize: 28
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Grid {
                    columns: 3
                    spacing: 24
                    Repeater {
                        model: ["Photos", "Musique", "Réglages", "Clavier", "Fichiers", "Aide"]
                        delegate: Tile {
                            label: modelData
                            onActivated: {
                                if (modelData === "Clavier")
                                    root.page = 1;
                            }
                        }
                    }
                }
            }
        }
    }

    // ---- Keyboard page ----
    Component {
        id: kbPage
        Item {
            ProjectedKeyboard {
                anchors.fill: parent
                onKeyPressed: function(k) { typed.text += k; }
                onClosed: root.page = 0
            }
            Text {
                id: typed
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: 40
                color: "#dfeaf2"
                font.pixelSize: 32
                text: ""
            }
        }
    }

    // ---- Global feedback overlay (always on top) ----
    DwellRing {
        centerX: root.px
        centerY: root.py
    }
    Reticle {
        x: root.px - width / 2
        y: root.py - height / 2
        confidence: bus.pointerConfidence
    }

    // Connection status hint (visual only; no audio channel on this hardware).
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        width: 12; height: 12; radius: 6
        color: bus.connected ? "#3ad07a" : "#d0553a"
    }
}
