#!/bin/bash
if [ ! -d bin ]
then
  mkdir bin
fi
change=0
error=0
for cpp in source/*.cpp; do
  base=$(basename $cpp)
  if [ -f bin/$base.md5 ] && [ "`cat bin/$base.md5`" == "`md5sum $cpp`" ]; then
    echo "Up to date: $cpp"
  else
    echo "Compiling $cpp..."
    g++-4.7 -c -O3 -Wall -std=c++0x -I./boost_1_53_0/install/include -o bin/$base.o $cpp
    if [$? -eq 0]; then
      md5sum $cpp > bin/$base.md5
    else
      rm bin/$base.md5
      error=1
    fi
    change=1
  fi
done
if [ $error -eq 0 ]; then
  if [ $change -eq 1 ]; then
    echo "Linking..."
    g++-4.7 -O3 -Wall -L./boost_1_53_0/install/lib -o bin/yugen bin/*.o -static -lboost_filesystem -lboost_system
  else
    echo "Up to date: yugen"
  fi
fi
