#!/bin/bash
# Usage: todo.sh
grep --color -n "T[O]D[O]" notes.txt *.sh data/shaders/* data/scripts/* data/scripts/*/* source/*.* source/proto/*.proto
