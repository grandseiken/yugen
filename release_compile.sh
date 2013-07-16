#!/bin/bash
# Usage: release_compile.sh
cd "$(dirname "$0")"
./compile.sh -c "g++-4.8 -O3 -std=c++11"
