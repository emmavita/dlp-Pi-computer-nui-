// uds.hpp — AF_UNIX SOCK_SEQPACKET transport for the NUI event bus.
//
// SEQPACKET preserves message boundaries (one send == one recv), is reliable
// and ordered, so no manual framing is needed beyond the fixed header used for
// validation and metadata. Backpressure policy:
//   - continuous streams (HAND_STATE, POINTER_EVENT): non-blocking, drop-on-full
//     (latest-wins; a stale pose is useless).
//   - discrete events (GESTURE_EVENT): reliable delivery, never dropped.
#ifndef NUI_PROTOCOL_UDS_HPP
#define NUI_PROTOCOL_UDS_HPP

#include <cstdint>
#include <string>
#include "nui_protocol/codec.hpp"

namespace nui {

enum class SendResult { Ok, Dropped, Error };
enum class RecvResult { Ok, WouldBlock, Closed, Error };

// One connected SEQPACKET socket. Move-only (owns an fd).
class UdsConn {
public:
    UdsConn() = default;
    explicit UdsConn(int fd) : fd_(fd) {}
    ~UdsConn();
    UdsConn(const UdsConn&) = delete;
    UdsConn& operator=(const UdsConn&) = delete;
    UdsConn(UdsConn&& o) noexcept : fd_(o.fd_) { o.fd_ = -1; }
    UdsConn& operator=(UdsConn&& o) noexcept;

    // Connect to a listening server at `path`.
    static UdsConn connect(const std::string& path);

    bool valid() const { return fd_ >= 0; }
    int  fd() const { return fd_; }

    // Send one message. `reliable=false` drops the message if the socket buffer
    // is full (EAGAIN) instead of blocking.
    SendResult send_msg(std::uint16_t type, const void* payload,
                        std::uint32_t payload_len, std::uint64_t ts_ns,
                        std::uint64_t seq, bool reliable);

    // Receive one message. On success fills `hdr` and copies the payload into
    // `out` (must hold at least kMaxPayload bytes); returns payload length via
    // `hdr.payload_len`. Returns false on peer close or error.
    bool recv_msg(nui_header_t& hdr, void* out);

    // Same, but distinguishes would-block (non-blocking socket, no data) from a
    // closed peer, so event-loop clients can reconnect on close without busy-looping.
    RecvResult recv_msg_ex(nui_header_t& hdr, void* out);

private:
    int fd_ = -1;
};

// Listening SEQPACKET server bound to a filesystem path.
class UdsListener {
public:
    explicit UdsListener(const std::string& path); // binds + listens; throws std::runtime_error on failure
    ~UdsListener();
    UdsListener(const UdsListener&) = delete;
    UdsListener& operator=(const UdsListener&) = delete;

    // Block until a client connects; returns the connected socket.
    UdsConn accept_one();

    int fd() const { return fd_; }

private:
    int         fd_ = -1;
    std::string path_;
};

} // namespace nui

#endif // NUI_PROTOCOL_UDS_HPP
