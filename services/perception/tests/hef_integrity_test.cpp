// hef_integrity_test.cpp — verify SHA-256 against the FIPS 180-4 "abc" vector
// and an empty-string vector. Exits non-zero on mismatch.
#include <cstdio>
#include <cstring>
#include "perception/hef_integrity.hpp"

int main() {
    // FIPS 180-4 examples.
    const char* expect_abc =
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    const char* expect_empty =
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

    {
        nui::Sha256 h;
        h.update("abc", 3);
        std::string d = h.hex_final();
        if (d != expect_abc) {
            std::fprintf(stderr, "FAIL abc: got %s\n", d.c_str());
            return 1;
        }
    }
    {
        nui::Sha256 h;
        std::string d = h.hex_final();
        if (d != expect_empty) {
            std::fprintf(stderr, "FAIL empty: got %s\n", d.c_str());
            return 1;
        }
    }
    // Longer input (two-block) sanity: 448-bit message from FIPS.
    {
        nui::Sha256 h;
        const char* msg = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
        h.update(msg, std::strlen(msg));
        std::string d = h.hex_final();
        const char* expect =
            "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1";
        if (d != expect) {
            std::fprintf(stderr, "FAIL multiblock: got %s\n", d.c_str());
            return 1;
        }
    }
    std::fprintf(stderr, "OK: sha256 vectors match\n");
    return 0;
}
