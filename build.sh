#!/bin/sh

# About
# =====
#   Simple script building all in case CMake didn't work for you.
#

echo Building main...
g++ -o run src/main.cpp src/lib.cpp -I include -std=c++20 -fcoroutines

echo Building tests...
g++ -o run_test_async tests/test_async.cpp src/lib.cpp -I include -std=c++20 -fcoroutines
g++ -o run_test_lib tests/test_lib.cpp src/lib.cpp -I include -std=c++20 -fcoroutines

echo Done.
