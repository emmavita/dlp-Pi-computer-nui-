// hef_integrity.hpp — verify a .hef model file against an expected SHA-256.
// Self-contained SHA-256 (FIPS 180-4), no external crypto dependency, so the
// perception service can validate model integrity before loading (security,
// Phase 4/6) without pulling in a TLS stack.
#ifndef NUI_PERCEPTION_HEF_INTEGRITY_HPP
#define NUI_PERCEPTION_HEF_INTEGRITY_HPP

#include <array>
#include <cstdint>
#include <string>

namespace nui {

// SHA-256 streaming hasher.
class Sha256 {
public:
    Sha256() { reset(); }
    void reset();
    void update(const void* data, std::size_t len);
    // Returns the 32-byte digest as a lowercase hex string (64 chars).
    std::string hex_final();

private:
    void transform(const uint8_t block[64]);
    std::array<uint32_t, 8> h_{};
    uint8_t  buf_[64]{};
    std::size_t buf_len_ = 0;
    uint64_t total_bits_ = 0;
};

// Hash a whole file. Returns empty string if the file cannot be read.
std::string sha256_file(const std::string& path);

// Convenience: true iff sha256_file(path) equals expected_hex (case-insensitive).
bool verify_hef(const std::string& path, const std::string& expected_hex);

} // namespace nui

#endif // NUI_PERCEPTION_HEF_INTEGRITY_HPP
