// uds.cpp — implementation of the SEQPACKET NUI event bus transport.
#include "nui_protocol/uds.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>

namespace nui {

namespace {
void set_nonblocking(int fd) {
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags >= 0) ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Copy a path into sockaddr_un, guarding against overflow.
bool fill_addr(sockaddr_un& addr, const std::string& path) {
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (path.size() + 1 > sizeof(addr.sun_path)) return false;
    std::memcpy(addr.sun_path, path.c_str(), path.size() + 1);
    return true;
}
} // namespace

UdsConn::~UdsConn() {
    if (fd_ >= 0) ::close(fd_);
}

UdsConn& UdsConn::operator=(UdsConn&& o) noexcept {
    if (this != &o) {
        if (fd_ >= 0) ::close(fd_);
        fd_ = o.fd_;
        o.fd_ = -1;
    }
    return *this;
}

UdsConn UdsConn::connect(const std::string& path) {
    int fd = ::socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (fd < 0) return UdsConn();
    sockaddr_un addr;
    if (!fill_addr(addr, path)) { ::close(fd); return UdsConn(); }
    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd);
        return UdsConn();
    }
    set_nonblocking(fd);
    return UdsConn(fd);
}

SendResult UdsConn::send_msg(std::uint16_t type, const void* payload,
                             std::uint32_t payload_len, std::uint64_t ts_ns,
                             std::uint64_t seq, bool reliable) {
    if (fd_ < 0) return SendResult::Error;
    if (payload_len > kMaxPayload) return SendResult::Error;

    unsigned char buf[kMaxMessage];
    nui_header_t hdr;
    const std::size_t total = make_header(hdr, type, payload_len, ts_ns, seq);
    std::memcpy(buf, &hdr, sizeof(hdr));
    if (payload_len && payload) std::memcpy(buf + sizeof(hdr), payload, payload_len);

    for (;;) {
        ssize_t n = ::send(fd_, buf, total, MSG_NOSIGNAL);
        if (n == static_cast<ssize_t>(total)) return SendResult::Ok;
        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (!reliable) return SendResult::Dropped; // latest-wins policy
                // reliable: wait until writable, then retry
                fd_set wf; FD_ZERO(&wf); FD_SET(fd_, &wf);
                if (::select(fd_ + 1, nullptr, &wf, nullptr, nullptr) <= 0)
                    return SendResult::Error;
                continue;
            }
            return SendResult::Error;
        }
        // Partial send cannot happen on SEQPACKET for a single datagram.
        return SendResult::Error;
    }
}

bool UdsConn::recv_msg(nui_header_t& hdr, void* out) {
    return recv_msg_ex(hdr, out) == RecvResult::Ok;
}

RecvResult UdsConn::recv_msg_ex(nui_header_t& hdr, void* out) {
    if (fd_ < 0) return RecvResult::Error;
    unsigned char buf[kMaxMessage];
    for (;;) {
        ssize_t n = ::recv(fd_, buf, sizeof(buf), 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return RecvResult::WouldBlock;
            return RecvResult::Error;
        }
        if (n == 0) return RecvResult::Closed; // peer closed
        if (static_cast<std::size_t>(n) < sizeof(nui_header_t)) return RecvResult::Error;
        std::memcpy(&hdr, buf, sizeof(hdr));
        if (!valid_header(hdr)) return RecvResult::Error;
        if (sizeof(hdr) + hdr.payload_len != static_cast<std::size_t>(n)) return RecvResult::Error;
        if (hdr.payload_len && out)
            std::memcpy(out, buf + sizeof(hdr), hdr.payload_len);
        return RecvResult::Ok;
    }
}

UdsListener::UdsListener(const std::string& path) : path_(path) {
    fd_ = ::socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (fd_ < 0) throw std::runtime_error("UdsListener: socket() failed");
    ::unlink(path_.c_str()); // remove stale socket
    sockaddr_un addr;
    if (!fill_addr(addr, path_)) {
        ::close(fd_); fd_ = -1;
        throw std::runtime_error("UdsListener: path too long");
    }
    if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd_); fd_ = -1;
        throw std::runtime_error("UdsListener: bind() failed");
    }
    if (::listen(fd_, 1) < 0) {
        ::close(fd_); fd_ = -1;
        throw std::runtime_error("UdsListener: listen() failed");
    }
}

UdsListener::~UdsListener() {
    if (fd_ >= 0) ::close(fd_);
    if (!path_.empty()) ::unlink(path_.c_str());
}

UdsConn UdsListener::accept_one() {
    for (;;) {
        int cfd = ::accept(fd_, nullptr, nullptr);
        if (cfd < 0) {
            if (errno == EINTR) continue;
            return UdsConn();
        }
        set_nonblocking(cfd);
        return UdsConn(cfd);
    }
}

} // namespace nui
