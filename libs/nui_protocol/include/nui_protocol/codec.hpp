// codec.hpp — header framing helpers for the fixed-layout NUI protocol.
#ifndef NUI_PROTOCOL_CODEC_HPP
#define NUI_PROTOCOL_CODEC_HPP

#include <cstdint>
#include <cstring>
#include "nui_events.h"

namespace nui {

// Largest payload determines the receive buffer size.
constexpr std::size_t kMaxPayload = sizeof(nui_hand_state_t);
constexpr std::size_t kMaxMessage = sizeof(nui_header_t) + kMaxPayload;

// Fill a header in place. Returns total message size (header + payload).
inline std::size_t make_header(nui_header_t& h, std::uint16_t type,
                               std::uint32_t payload_len,
                               std::uint64_t ts_ns, std::uint64_t seq) {
    h.magic         = NUI_PROTO_MAGIC;
    h.proto_version = static_cast<std::uint16_t>(NUI_PROTO_VERSION);
    h.msg_type      = type;
    h.payload_len   = payload_len;
    h.timestamp_ns  = ts_ns;
    h.sequence      = seq;
    return sizeof(nui_header_t) + payload_len;
}

// Validate a received header against magic/version and a max payload bound.
inline bool valid_header(const nui_header_t& h) {
    return h.magic == NUI_PROTO_MAGIC
        && h.proto_version == static_cast<std::uint16_t>(NUI_PROTO_VERSION)
        && h.payload_len <= kMaxPayload;
}

} // namespace nui

#endif // NUI_PROTOCOL_CODEC_HPP
