import QtQuick

// Dwell progress ring. COSMETIC affordance only: it fills while the pointer is
// stationary to signal an imminent dwell-select. The AUTHORITATIVE selection is
// the engine's DWELL_SELECT gesture; this ring does not itself trigger actions.
// The fill duration here is a UI hint, independent from the engine threshold.
Item {
    id: d
    property real centerX: 0
    property real centerY: 0
    property real moveTol: 12       // px, cosmetic stationarity tolerance
    property real fillSeconds: 0.8  // cosmetic fill time
    property real progress: 0
    property real lastX: 0
    property real lastY: 0
    z: 999
    visible: progress > 0.02

    Canvas {
        id: cv
        width: 60; height: 60
        x: d.centerX - width / 2
        y: d.centerY - height / 2
        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            ctx.beginPath();
            ctx.arc(width / 2, height / 2, 24,
                    -Math.PI / 2, -Math.PI / 2 + d.progress * 2 * Math.PI);
            ctx.lineWidth = 4;
            ctx.strokeStyle = "rgba(80,230,255,0.9)";
            ctx.stroke();
        }
    }
    onProgressChanged: cv.requestPaint()

    Timer {
        interval: 33
        running: true
        repeat: true
        onTriggered: {
            var dx = d.centerX - d.lastX;
            var dy = d.centerY - d.lastY;
            if (Math.sqrt(dx * dx + dy * dy) > d.moveTol) {
                d.lastX = d.centerX;
                d.lastY = d.centerY;
                d.progress = 0;
            } else {
                d.progress = Math.min(1.0, d.progress + (0.033 / d.fillSeconds));
            }
        }
    }
}
