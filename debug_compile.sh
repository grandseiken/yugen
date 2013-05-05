#!/bin/bash
# Usage: debug_compile.sh
cd "$(dirname "$0")"
./compile.sh -c "g++-4.8 -std=c++11 -Werror -Wall -Wextra -pedantic -ggdb"
