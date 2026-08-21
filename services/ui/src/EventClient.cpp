// EventClient.cpp — implementation.
#include "EventClient.hpp"

#include <QSocketNotifier>
#include <QTimer>
#include <cstring>
#include "nui_events.h"

EventClient::EventClient(QString path, QObject* parent)
    : QObject(parent), path_(std::move(path)) {
    reconnect_ = new QTimer(this);
    reconnect_->setInterval(500);
    connect(reconnect_, &QTimer::timeout, this, &EventClient::tryConnect);
    tryConnect();
}

EventClient::~EventClient() = default;

void EventClient::teardown() {
    notifier_.reset();
    conn_ = nui::UdsConn(); // closes the fd
    emit connectedChanged();
    reconnect_->start();
}

void EventClient::tryConnect() {
    if (notifier_) return;
    nui::UdsConn c = nui::UdsConn::connect(path_.toStdString());
    if (!c.valid()) { reconnect_->start(); return; }
    conn_ = std::move(c);
    notifier_ = std::make_unique<QSocketNotifier>(conn_.fd(), QSocketNotifier::Read, this);
    connect(notifier_.get(), &QSocketNotifier::activated, this, &EventClient::onReadable);
    reconnect_->stop();
    emit connectedChanged();
}

void EventClient::onReadable() {
    nui_header_t hdr;
    unsigned char payload[nui::kMaxPayload];
    for (;;) {
        nui::RecvResult r = conn_.recv_msg_ex(hdr, payload);
        if (r == nui::RecvResult::Ok) {
            if (hdr.msg_type == NUI_MSG_POINTER_EVENT &&
                hdr.payload_len == sizeof(nui_pointer_event_t)) {
                nui_pointer_event_t p;
                std::memcpy(&p, payload, sizeof(p));
                px_ = p.ui_x; py_ = p.ui_y; pconf_ = p.confidence;
                emit pointerChanged();
            } else if (hdr.msg_type == NUI_MSG_GESTURE_EVENT &&
                       hdr.payload_len == sizeof(nui_gesture_event_t)) {
                nui_gesture_event_t g;
                std::memcpy(&g, payload, sizeof(g));
                emit gesture(g.type, g.x, g.y, g.param0, g.param1);
            }
            continue;
        }
        if (r == nui::RecvResult::WouldBlock) break;
        // Closed or Error -> reconnect.
        teardown();
        break;
    }
}
