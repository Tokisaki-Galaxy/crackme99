#pragma once
#include <string_view>
#include <string>
#include <cstdint>
#include <vector>

// 混沌引擎：必须保证 Keygen 和 CrackMe 完全一致
class ChaosEngine {
    uint64_t state;
public:
    ChaosEngine(std::string_view seed_str) {
        state = 0xCBF29CE484222325;
        for (char c : seed_str) {
            state ^= (uint8_t)c;
            state *= 0x100000001B3;
        }
    }

    uint8_t next_byte() {
        uint64_t x = state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        state = x;
        return static_cast<uint8_t>(state & 0xFF);
    }
};