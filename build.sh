#!/bin/sh
set -eux

# only compile target with -O2 but fuzzer with Ofast
CFLAGS="-std=c++17 -g -ggdb -O2 -march=native"
COVFLAGS="-fsanitize-coverage=inline-8bit-counters -fsanitize-coverage-allowlist=allowlist.txt -fsanitize=address"
LDFLAGS="-flto -lxxhash -lssl -lcrypto"

clang-format -i -style="{BasedOnStyle: Google, IndentWidth: 4, ColumnLimit: 0}" src/* target/*
clang++ -c src/*.cpp $CFLAGS -Ofast
clang++ -c target/*.cpp $CFLAGS $COVFLAGS
clang++ -o main *.o $CFLAGS $COVFLAGS $LDFLAGS
