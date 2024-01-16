# sanfuzzer

Extremely simple in process coverage guided fuzzer using [SanitizerCoverage's](https://clang.llvm.org/docs/SanitizerCoverage.html) 8-bit counters option.

It requires linking against an object file containing a function definition of the form `extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)` (the same definition libfuzzer uses).  See target/target.cpp and build.sh for an example.

It contains three mutators:

- `RandomByteMutator` which assigns a random byte index in the input to a random value from 0 to 255
- `BitFlipMutator` which flips up to 4 sequential bits (configurable)
- `HavocMutator` which combines operations using all other mutators up to 16 times (configurable)

[xxhash64](https://github.com/Cyan4973/xxHash) is used to figure out if coverage from the current run was unique.  I selected it based on it being fast and having a near 0 collision rate.  libssl is used to calculate SHA256 of input files identified as adding new coverage to use as a filename for them in the corpus directory.

## Issues

- Doesn't handle crashes on its own
- Missing useful mutation strategies like known integers, splicing, etc

## Example

target/target.cpp contains an example of a program where coverage guided fuzzing is necessary to find the crash.

It contains logic like this:
```
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
```

The odds of us randomly generating an input that satisfies all of those constraints and actually crashes are very low (1/256^11).  So if we have coverage information and can build off inputs that we know add coverage we can reduce the problem to something closer to the order of 256\*11.  Not exactly, because there are multiple mutators.  Or, say our corpus is {'a...', 'ab...' 'abc....'}, randomly selecting from it will get us 'a...' and 'ab...' most of the time rather than 'abc...' which in the case of this program would be the input we need to mutate to make progress coverage-wise.  But adding coverage guidance still makes finding the crash go from impossible to possible in less than a second.

Just run:

```
$ ./run.sh
...
+ ./main
__sanitizer_cov_8bit_counters_init: 19
added input files, corpus: 1, cov: 1
0.000168 [0]: corpus: 1, cov: 1, fcps: 0
New coverage found: (2422222222222222222222222222)
New coverage found: (a222222222222222222222222222)
New coverage found: (ab22222222222222222222222222)
33.3049 [100000]: corpus: 4, cov: 4, fcps: 3.00256e+06
New coverage found: (abc2222222222222222222222222)
66.2534 [200000]: corpus: 5, cov: 5, fcps: 3.01871e+06
102.223 [300000]: corpus: 5, cov: 5, fcps: 2.93475e+06
New coverage found: (abcd222222222222222222222222)
137.03 [400000]: corpus: 6, cov: 6, fcps: 2.91907e+06
New coverage found: (abcde22222222222222222222222)
167.283 [500000]: corpus: 7, cov: 7, fcps: 2.98895e+06
New coverage found: (abcdef2222222222222222222222)
198.583 [600000]: corpus: 8, cov: 8, fcps: 3.02141e+06
231.908 [700000]: corpus: 8, cov: 8, fcps: 3.01844e+06
New coverage found: (abcdefg222222222222222222222)
265.596 [800000]: corpus: 9, cov: 9, fcps: 3.01209e+06
New coverage found: (abcdefgh22222222222222222222)
New coverage found: (abcdefgh2"222222222222222222)
CRASH!
INPUT: abcdefgh2""22222222222222222
AddressSanitizer:DEADLYSIGNAL
=================================================================
==12259==ERROR: AddressSanitizer: SEGV on unknown address 0x603005f5e31f (pc 0x55bfe8d04c14 bp 0x7ffe197788f0 sp 0x7ffe19778880 T0)
==12259==The signal is caused by a READ memory access.
    #0 0x55bfe8d04c14 in LLVMFuzzerTestOneInput /home/christopher/prog/cpp/sanfuzzer/target/target.cpp:27:53
    #1 0x55bfe8d028dd in main /home/christopher/prog/cpp/sanfuzzer/src/main.cpp:168:13
    #2 0x7f216ee23b4b in __libc_start_call_main csu/../sysdeps/nptl/libc_start_call_main.h:58:16
    #3 0x7f216ee23c04 in __libc_start_main@GLIBC_2.2.5 csu/../csu/libc-start.c:360:3
    #4 0x55bfe8c09620 in _start /builddir/glibc-2.38/csu/../sysdeps/x86_64/start.S:115

AddressSanitizer can not provide additional info.
SUMMARY: AddressSanitizer: SEGV /home/christopher/prog/cpp/sanfuzzer/target/target.cpp:27:53 in LLVMFuzzerTestOneInput
==12259==ABORTING
```
