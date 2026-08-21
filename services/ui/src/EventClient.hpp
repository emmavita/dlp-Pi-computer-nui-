// EventClient.hpp — bridges the NUI UDS event bus into the Qt event loop and
// exposes pointer state + gesture signals to QML.
//
// The engine exposes its ui link as an AF_UNIX SOCK_SEQPACKET socket (Phase 6).
// QLocalSocket does not support SEQPACKET, so we integrate the raw fd (from
// nui::UdsConn) via QSocketNotifier and decode with the existing nui_protocol.
#ifndef NUI_UI_EVENT_CLIENT_HPP
#define NUI_UI_EVENT_CLIENT_HPP

#include <QObject>
#include <QString>
#include <memory>
#include "nui_protocol/uds.hpp"

class QSocketNotifier;
class QTimer;

class EventClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(qreal pointerX READ pointerX NOTIFY pointerChanged)
    Q_PROPERTY(qreal pointerY READ pointerY NOTIFY pointerChanged)
    Q_PROPERTY(qreal pointerConfidence READ pointerConfidence NOTIFY pointerChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

public:
    explicit EventClient(QString path, QObject* parent = nullptr);
    ~EventClient() override;

    qreal pointerX() const { return px_; }
    qreal pointerY() const { return py_; }
    qreal pointerConfidence() const { return pconf_; }
    bool  connected() const { return static_cast<bool>(notifier_); }

signals:
    void pointerChanged();
    void connectedChanged();
    // type mirrors enum nui_gesture_type in proto/nui_events.h.
    void gesture(int type, qreal x, qreal y, qreal param0, qreal param1);

private slots:
    void onReadable();
    void tryConnect();

private:
    void teardown();

    QString                          path_;
    nui::UdsConn                     conn_;
    std::unique_ptr<QSocketNotifier> notifier_;
    QTimer*                          reconnect_ = nullptr;
    qreal px_ = 0.5, py_ = 0.5, pconf_ = 0.0;
};

#endif // NUI_UI_EVENT_CLIENT_HPP
