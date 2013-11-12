#!/bin/bash
# Usage: wc.sh
echo Scripts and docs:
cat Makefile notes.txt *.sh | wc
echo GLSL:
cat data/shaders/*.glsl | wc
echo Lua:
cat data/scripts/*.lua data/scripts/*/*.lua | wc
echo C++:
cat source/*.* source/*/* | wc
echo Total:
cat Makefile notes.txt *.sh data/shaders/*.glsl \
  data/scripts/*.lua data/scripts/*/*.lua source/*.* source/*/* | wc
