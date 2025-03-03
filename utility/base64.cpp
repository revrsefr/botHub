#include "base64.h"
#include <vector>
#include <string>

static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string base64_encode(const std::string& input) {
    std::string output;
    int val = 0;
    int bits = -6;
    const int mask = 0x3F;

    for (unsigned char c : input) {
        val = (val << 8) + c;
        bits += 8;

        while (bits >= 0) {
            output.push_back(base64_chars[(val >> bits) & mask]);
            bits -= 6;
        }
    }

    if (bits > -6) {
        output.push_back(base64_chars[((val << 8) >> (bits + 8)) & mask]);
    }

    while (output.size() % 4) {
        output.push_back('=');
    }

    return output;
}
