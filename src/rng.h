#ifndef RNG_H
#define RNG_H

#include <cstdint>
#include <cstdlib>

class Rng {
   public:
    // Rng initializes the random number generator with the default seed.
    Rng();

    // Rng initializes the random number generator with a specific seed.
    Rng(uint64_t seed);

    // Rand generates a random uint64_t.
    uint64_t Rand();

    // Intn generates a random number in [0, limit).
    uint64_t Intn(uint64_t limit);

    // Byte returns a random byte expressed as a uint8_t.
    uint8_t Byte();

   private:
    uint64_t x_ = 0;
};

#endif