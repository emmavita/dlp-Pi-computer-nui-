// qa_malformed.cpp — security/robustness test: inject malformed frames on the
// perception→engine UDS link, then a valid TAP sequence. Verifies (via the
// orchestrator) that the engine drops bad frames and stays up (fail-open on the
// trusted local bus), thanks to SOCK_SEQPACKET message boundaries.
//
// Uses a RAW AF_UNIX SOCK_SEQPACKET socket (not nui_protocol) so it can craft
// deliberately invalid headers that the normal codec would never produce.
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <ctime>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "nui_events.h"
#include "qa_hands.hpp"

static uint64_t mono_ns() {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return uint64_t(ts.tv_sec) * 1000000000ull + uint64_t(ts.tv_nsec);
}
static std::string env_or(const char* k, const char* d) {
    const char* v = std::getenv(k); return v ? std::string(v) : std::string(d);
}
static void nap(int ms) { struct timespec t{ms / 1000, (ms % 1000) * 1000000L}; nanosleep(&t, nullptr); }

static int connect_raw(const std::string& path) {
    int fd = ::socket(AF_UNIX, SOCK_SEQPACKET, 0); // blocking
    if (fd < 0) return -1;
    sockaddr_un a; std::memset(&a, 0, sizeof(a)); a.sun_family = AF_UNIX;
    std::strncpy(a.sun_path, path.c_str(), sizeof(a.sun_path) - 1);
    if (::connect(fd, reinterpret_cast<sockaddr*>(&a), sizeof(a)) < 0) { ::close(fd); return -1; }
    return fd;
}

// Send one datagram with fully controllable (possibly invalid) header fields.
static void send_frame(int fd, uint32_t magic, uint16_t version, uint16_t type,
                       uint32_t payload_len_field, const void* payload,
                       size_t actual_payload_bytes, uint64_t seq) {
    unsigned char buf[512];
    nui_header_t h;
    h.magic = magic; h.proto_version = version; h.msg_type = type;
    h.payload_len = payload_len_field; h.timestamp_ns = mono_ns(); h.sequence = seq;
    std::memcpy(buf, &h, sizeof(h));
    if (payload && actual_payload_bytes) std::memcpy(buf + sizeof(h), payload, actual_payload_bytes);
    ssize_t n = ::send(fd, buf, sizeof(h) + actual_payload_bytes, MSG_NOSIGNAL);
    (void)n;
}

int main() {
    const std::string perc = env_or("NUI_PERCEPTION_SOCK", "/tmp/nui_perception.sock");
    int fd = -1;
    for (int i = 0; i < 100 && fd < 0; ++i) { fd = connect_raw(perc); if (fd < 0) nap(20); }
    if (fd < 0) { std::fprintf(stderr, "qa_malformed: cannot connect to %s\n", perc.c_str()); return 1; }
    std::fprintf(stderr, "qa_malformed: connected\n");

    nui_hand_state_t hand = qa::hand_pointer(false, 0.5f, 0.4f);
    uint64_t seq = 0;

    // --- 5 error-producing malformed frames ---
    // 1) bad magic
    send_frame(fd, 0xDEADBEEFu, NUI_PROTO_VERSION, NUI_MSG_HAND_STATE,
               sizeof(hand), &hand, sizeof(hand), seq++);
    // 2) bad protocol version
    send_frame(fd, NUI_PROTO_MAGIC, 99, NUI_MSG_HAND_STATE,
               sizeof(hand), &hand, sizeof(hand), seq++);
    // 3) oversized payload_len field (exceeds kMaxPayload bound)
    send_frame(fd, NUI_PROTO_MAGIC, NUI_PROTO_VERSION, NUI_MSG_HAND_STATE,
               99999u, &hand, sizeof(hand), seq++);
    // 4) length mismatch: header claims full payload, only 10 bytes sent
    send_frame(fd, NUI_PROTO_MAGIC, NUI_PROTO_VERSION, NUI_MSG_HAND_STATE,
               sizeof(hand), &hand, 10, seq++);
    // 5) truncated: fewer bytes than the header itself
    {
        unsigned char tiny[10]; std::memset(tiny, 0xAB, sizeof(tiny));
        ::send(fd, tiny, sizeof(tiny), MSG_NOSIGNAL);
    }

    // --- 1 valid-but-unknown-type frame (must be safely ignored, not an error) ---
    send_frame(fd, NUI_PROTO_MAGIC, NUI_PROTO_VERSION, /*type=*/999u, 0, nullptr, 0, seq++);

    nap(60);

    // --- valid TAP sequence: proves the engine still works after the barrage ---
    auto send_hand = [&](const nui_hand_state_t& h) {
        send_frame(fd, NUI_PROTO_MAGIC, NUI_PROTO_VERSION, NUI_MSG_HAND_STATE,
                   sizeof(h), &h, sizeof(h), seq++);
    };
    nui_hand_state_t absent = qa::hand_absent();
    nui_hand_state_t released = qa::hand_pointer(false, 0.5f, 0.4f);
    nui_hand_state_t pinched = qa::hand_pointer(true, 0.5f, 0.4f);
    send_hand(absent);   nap(40);
    send_hand(released); nap(40);
    send_hand(pinched);  nap(40);
    send_hand(released); nap(40);
    send_hand(absent);   nap(40);

    std::fprintf(stderr, "qa_malformed: done (5 malformed + 1 unknown-type + valid TAP)\n");
    ::close(fd);
    return 0;
}
