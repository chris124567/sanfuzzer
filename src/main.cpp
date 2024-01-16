#include <openssl/sha.h>
#include <sanitizer/coverage_interface.h>
#include <xxhash.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "mutate.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

static const std::string kCorpus = "./corpus";

struct Counter {
    char *start;
    long size;
};

// Not thread-safe
class Fuzzer {
   public:
    Fuzzer() {
        state_ = XXH64_createState();
        if (state_ == nullptr) {
            throw std::runtime_error("failed to create hash state");
        }
    }

    ~Fuzzer() {
        XXH64_freeState(state_);
    }

    void InitCounter(Counter c) {
        // Ignore repeated calls
        if (counters_.size() > 0 && counters_.back().start == c.start) {
            return;
        } else if (c.size < 0) {
            return;
        }
        counters_.push_back(c);
    }

    void ClearCounters() {
        for (const auto &counter : counters_) {
            // std::cout << "Clear" << std::endl;
            memset(counter.start, '\0', counter.size);
        }
    }

    uint64_t HashCounters() {
        static const XXH64_hash_t kHashSeed = 0;
        if (XXH64_reset(state_, kHashSeed) == XXH_ERROR) {
            throw std::runtime_error("failed to reset hash state");
        }
        for (const auto &counter : counters_) {
            // std::cout << "Updating hash state" << std::endl;
            if (XXH64_update(state_, counter.start, counter.size) == XXH_ERROR) {
                throw std::runtime_error("failed to update hash state");
            }
        }
        return XXH64_digest(state_);
    }

   private:
    std::vector<Counter> counters_;
    XXH64_state_t *state_ = nullptr;
};

size_t counter_index = 0;
Counter counters[256] = {0};

extern "C" void __sanitizer_cov_8bit_counters_init(char *start, char *end) {
    // [start,end) is the array of 8-bit counters created for the current DSO.
    // Capture this array in order to read/modify the counters.
    printf("__sanitizer_cov_8bit_counters_init: %lu\n", end - start);
    counters[counter_index++] = {.start = start, .size = end - start};
}

std::string bin2hex(const std::array<uint8_t, SHA256_DIGEST_LENGTH> &hash) {
    std::string out;
    out.resize(hash.size() * 2);
    for (size_t i = 0; i < hash.size(); i++) {
        out[i * 2] = "0123456789ABCDEF"[hash[i] >> 4];
        out[i * 2 + 1] = "0123456789ABCDEF"[hash[i] & 0x0F];
    }
    return out;
}

int main(void) {
    std::vector<std::vector<uint8_t>> corpus;
    for (const auto &entry : std::filesystem::directory_iterator(kCorpus)) {
        if (!entry.is_directory()) {
            // slow but doesn't really matter as corpus typically isn't very large
            FILE *fp = fopen(entry.path().string().c_str(), "r");
            if (fp == nullptr) {
                throw std::runtime_error("failed to open file");
            }

            int c{};
            std::vector<uint8_t> v;
            while ((c = fgetc(fp)) != EOF) {
                v.emplace_back(static_cast<uint8_t>(c));
            }
            fclose(fp);

            corpus.emplace_back(v);
        }
    }

    Fuzzer fuzzer;
    for (size_t i = 0; i < counter_index; i++) {
        fuzzer.InitCounter(counters[i]);
    }

    std::unordered_set<uint64_t> coverage_seen;
    for (const auto input : corpus) {
        if (LLVMFuzzerTestOneInput(input.data(), input.size()) == -1) {
            continue;
        }

        const auto coverage = fuzzer.HashCounters();
        fuzzer.ClearCounters();
        if (coverage_seen.count(coverage) == 1) {
            continue;
        }

        coverage_seen.insert(coverage);
    }
    std::cout << "added input files, "
              << "corpus: " << corpus.size() << ", cov: " << coverage_seen.size() << std::endl;

    Rng rng;
    std::vector<uint8_t> input;
    std::array<uint8_t, SHA256_DIGEST_LENGTH> hash;
    std::vector<std::shared_ptr<Mutator>> mutators = {
        std::make_shared<RandomByteMutator>(RandomByteMutator()),
        std::make_shared<BitFlipMutator>(BitFlipMutator()),
    };

    const auto begin = std::chrono::system_clock::now();
    auto last = begin;
    for (size_t i = 0; i < 1000000000; i++) {
        if (i % 100000 == 0) {
            last = std::chrono::system_clock::now();
            const std::chrono::duration<double, std::milli> duration = (last - begin);
            std::cout << duration.count() << " [" << i << "]: "
                      << "corpus: " << corpus.size() << ", cov: " << coverage_seen.size() << ", fcps: " << (i / (duration.count() / 1000)) << std::endl;
        }

        input.clear();
        const auto &reference_input = corpus[rng.Intn(corpus.size())];
        for (const uint8_t b : reference_input) {
            input.emplace_back(b);
        }

        mutators[rng.Intn(mutators.size())]->mutate(rng, mutators, input);

        if (LLVMFuzzerTestOneInput(input.data(), input.size()) == -1) {
            fuzzer.ClearCounters();
            continue;
        }

        const auto coverage = fuzzer.HashCounters();
        fuzzer.ClearCounters();
        if (coverage_seen.count(coverage) == 1) {
            continue;
        }

        std::cout << "New coverage found: "
                  << "(" << std::string(reinterpret_cast<char *>(input.data()), input.size()) << ")" << std::endl;
        coverage_seen.insert(coverage);
        corpus.push_back(input);

        {
            SHA256(input.data(), input.size(), hash.data());

            const auto output_path = kCorpus + "/" + bin2hex(hash);
            FILE *fp = fopen(output_path.c_str(), "w+");
            if (fp == nullptr) {
                throw std::runtime_error("failed to create open new corpus file");
            }
            fwrite(input.data(), sizeof(uint8_t), input.size(), fp);
            fclose(fp);
        }
    }

    return EXIT_SUCCESS;
}
