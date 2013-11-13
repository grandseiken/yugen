#!/bin/bash
# Usage: todo.sh
grep --color -n "T[O]D[O]" \
  Makefile Makedeps notes.txt *.sh \
  data/shaders/*.glsl data/scripts/*.lua data/scripts/*/*.lua \
  source/* source/*/*
