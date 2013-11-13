#!/bin/bash
# Usage: add.sh
git add Makefile Makedeps notes.txt *.sh \
  data/shaders/*.glsl data/scripts/*.lua data/scripts/*/*.lua \
  source/* source/*/* data/world/* data/tiles/*
