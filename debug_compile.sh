#!/bin/bash
# Usage: debug_compile.sh
cd "$(dirname "$0")"
./compile.sh -c "g++-4.8 -Og -std=c++11 -Wall -Wextra -pedantic -ggdb -DLUA_DEBUG -DGL_DEBUG"
