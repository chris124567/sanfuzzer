#include "rng.h"

#include <cstdint>
#include <iostream>

uint64_t Rng::Rand() {
    uint64_t current = x_;
    current ^= current << 13;
    current ^= current >> 17;
    current ^= current << 5;
    return x_ = current;
}

Rng::Rng() {
    x_ = 1706980716;
}

Rng::Rng(uint64_t seed) {
    x_ = seed;
}

uint64_t Rng::Intn(uint64_t limit) {
    if (limit == 0) {
        throw std::runtime_error("invalid limit");
    }
    return Rand() % limit;
}

uint8_t Rng::Byte() {
    // get first byte
    return Rand() & ((1 << 8) - 1);
}
