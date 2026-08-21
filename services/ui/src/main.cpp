// main.cpp — nui-ui: fullscreen Qt Quick application (Wayland client under
// labwc on the target). Receives pointer/gesture events from the engine over
// the NUI UDS bus and renders visual feedback + a gesture-driven shell.
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>
#include "EventClient.hpp"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    const QString sock = qEnvironmentVariable("NUI_UI_SOCK", "/tmp/nui_ui.sock");
    EventClient bus(sock);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("bus", &bus);
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
