#include <unistd.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

__attribute__((noinline)) extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 11) {
        std::cout << "BAILING OUT" << std::endl;
        return -1;
    }

    if (data[0] == 'a') {
        if (data[1] == 'b') {
            if (data[2] == 'c') {
                if (data[3] == 'd') {
                    if (data[4] == 'e') {
                        if (data[5] == 'f') {
                            if (data[6] == 'g') {
                                if (data[7] == 'h') {
                                    if ((data[8] & 0b10101010) == data[9] && data[9] == data[10]) {
                                        std::cout << "CRASH!" << std::endl;
                                        std::cout << "INPUT: " << std::string(reinterpret_cast<const char *>(data), size) << std::endl;

                                        // OOB read
                                        uint8_t a = data[99999999];
                                        std::cout << a << std::endl;
                                        // If ASAN isn't enabled crash anyways
                                        abort();
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
