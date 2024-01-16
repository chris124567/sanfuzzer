#include "mutate.h"

#include "rng.h"

void RandomByteMutator::mutate(Rng &rng, std::vector<std::shared_ptr<Mutator>> &mutators, std::vector<uint8_t> &data) {
    data[rng.Intn(data.size())] = rng.Byte();
}

void BitFlipMutator::mutate(Rng &rng, std::vector<std::shared_ptr<Mutator>> &mutators, std::vector<uint8_t> &data) {
    const uint64_t n_bits = rng.Intn(max_bits_);
    const uint64_t start_bit = rng.Intn(data.size() * 8);
    for (uint64_t i = 0; i < n_bits; i++) {
        const uint64_t current_bit = start_bit + i;
        const uint64_t byte_index = (current_bit) / 8;
        if (byte_index >= data.size()) {
            return;
        }
        data[byte_index] = data[byte_index] ^ (1 << (current_bit % 8));
    }
}

void HavocMutator::mutate(Rng &rng, std::vector<std::shared_ptr<Mutator>> &mutators, std::vector<uint8_t> &data) {
    const uint64_t limit = rng.Intn(max_operations_);
    for (uint64_t i = 0; i < limit; i++) {
        mutators[rng.Intn(mutators.size())]->mutate(rng, mutators, data);
    }
}
