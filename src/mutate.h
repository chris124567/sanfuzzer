#ifndef MUTATE_H
#define MUTATE_H

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <vector>

#include "rng.h"

static const constexpr uint64_t kDefaultBitFlipMaxBits = 4;
static const constexpr uint64_t kDefaultHavocMaxOperations = 16;

class Mutator {
   public:
    virtual ~Mutator(){};
    virtual void mutate(Rng &rng, std::vector<std::shared_ptr<Mutator>> &mutators, std::vector<uint8_t> &data) = 0;
};

// RandomByteMutator randomly sets a byte in the input to random value.
class RandomByteMutator : public Mutator {
   public:
    void mutate(Rng &rng, std::vector<std::shared_ptr<Mutator>> &mutators, std::vector<uint8_t> &data);
};

// BitFlipMutator randomly flips up to max_bits_ sequential bits in the input.
class BitFlipMutator : public Mutator {
   public:
    BitFlipMutator() : max_bits_(kDefaultBitFlipMaxBits){};
    BitFlipMutator(uint64_t max_bits) : max_bits_(max_bits){};
    void mutate(Rng &rng, std::vector<std::shared_ptr<Mutator>> &mutators, std::vector<uint8_t> &data);

   private:
    uint64_t max_bits_ = 0;
};

// HavocMutator randomly applies up to max_operations_ mutators to the input.
class HavocMutator : public Mutator {
   public:
    HavocMutator() : max_operations_(kDefaultHavocMaxOperations){};
    HavocMutator(uint64_t max_operations) : max_operations_(max_operations){};
    void mutate(Rng &rng, std::vector<std::shared_ptr<Mutator>> &mutators, std::vector<uint8_t> &data);

   private:
    uint64_t max_operations_ = 0;
};

#endif