#!/bin/bash
cd "$(dirname "$0")"
if [ ! -d bin ]
then
  mkdir bin
fi
cflag="-O3 -Wall -std=c++0x -I./depend/boost_1_53_0/include -I./depend/sfml_2_0/include"
lflag="-O3 -Wall -L./depend/boost_1_53_0/lib -L./depend/sfml_2_0/lib"
libs="-Wl,-Bstatic -lboost_filesystem -lboost_system -lsfml-graphics-s -lsfml-window-s -lsfml-system-s -Wl,-Bdynamic -lGLEW -lGL -lX11 -lXrandr -ljpeg"

change=0
error=0
for cpp in source/*.cpp; do
  base=$(basename $cpp)
  if [ -f bin/$base.md5 ] && [ "`cat bin/$base.md5`" == "`md5sum $cpp`" ]; then
    echo "Up to date: $cpp"
  else
    echo "Compiling $cpp..."
    g++-4.7 -c $cflag -o bin/$base.o $cpp
    if [ $? -eq 0 ]; then
      md5sum $cpp > bin/$base.md5
    else
      if [ -f bin/$base.md5 ]; then
        rm bin/$base.md5
      fi
      error=1
    fi
    change=1
  fi
done
if [ $error -eq 0 ]; then
  if [ $change -eq 1 ] || [ ! -f bin/yugen.md5 ]; then
    echo "Linking..."
    g++-4.7 $lflag -o bin/yugen bin/*.o $libs
    if [ $? -eq 0 ]; then
      md5sum bin/yugen > bin/yugen.md5
    elif [ -f bin/yugen.md5 ]; then
      rm bin/yugen.md5
    fi
  else
    echo "Up to date: yugen"
  fi
fi
