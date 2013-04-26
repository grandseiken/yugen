#!/bin/bash
# Usage: compile_debug.sh
cd "$(dirname "$0")"
./compile.sh -c "g++-4.7 -ggdb"
