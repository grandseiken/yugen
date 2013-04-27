#!/bin/bash
# Usage: compile.sh [-c compiler] [target1, [target2 ...]]
cd "$(dirname "$0")"
# Make output directory.
if [ ! -d bin ]
then
  mkdir bin
fi

# Defaults.
gcc="g++-4.7 -O3"
targets[0]="yugen"
targets[1]="yedit"
targets_size=2

cflag="-Werror -Wall -Wextra -pedantic -std=c++0x -I./depend/boost_1_53_0/include -I./depend/sfml_2_0/include -I./depend/lua_5_2_2/include -I./depend/protobuf_2_5_0/include"
lflag="-Werror -Wall -Wextra -pedantic -L./depend/boost_1_53_0/lib -L./depend/sfml_2_0/lib -L./depend/lua_5_2_2/lib -L./depend/protobuf_2_5_0/lib"
libs="-Wl,-Bstatic -lboost_filesystem -lboost_system -lsfml-graphics-s -lsfml-window-s -lsfml-system-s -llua -lprotobuf -Wl,-Bdynamic -lGLEW -lGL -lX11 -lXrandr -ljpeg"

# Grab compiler options.
first=0
if [ $# -gt 0 ] && [ $1 == "-c" ]; then
  gcc=$2
  first=2
fi

# Save compiler options so we can recompile if they change.
if [ ! -f bin/.cmd ] || [ "`cat bin/.cmd`" != "$gcc" ]; then
  rm -rf bin
  mkdir bin
fi
echo "$gcc" > bin/.cmd

# Figure out compile targets.
compile_targets_size=$targets_size
if [ $# -gt $first ]; then
  compile_targets_size=$#
  i=$first
  while [ $i -lt $compile_targets_size ]; do
    let j=$i+1
    compile_targets[$i]=${!j}
    let i=$i+1
  done 
else
  compile_targets_size=$targets_size
  i=0
  while [ $i -lt $targets_size ]; do
    compile_targets[$i]=${targets[$i]}
    let i=$i+1
  done
fi

# Recompile proto files if necessary.
error=0
change=0
old=$(cat bin/.proto.md5 2> /dev/null)
new=$(md5sum source/proto/* 2> /dev/null)
if [ -f bin/.proto.md5 ] && [ "$old" == "$new" ]; then
  echo "Up to date: source/proto"
else
  echo "Compiling source/proto..."
  depend/protobuf_2_5_0/bin/protoc -I=source/proto --cpp_out=source/proto source/proto/*.proto
  success=$?
  for pb in source/proto/*.pb.cc; do
    echo "Compiling $pb..."
    base=$(basename $pb)
    $gcc -c $cflag -o bin/$base.o $pb
    let success=$success && $?
  done
  if [ $success -eq 0 ]; then
    md5sum source/proto/* > bin/.proto.md5 2> /dev/null
  else
    if [ -f bin/.proto.md5 ]; then
      rm bin/.proto.md5
    fi
    error=1
  fi
  change=1
fi

# Remove checksums for files which are no longer there.
for o in bin/*.cpp.o; do
  base=$(basename `echo $o | cut -d . -f 1,2`)
  if [ ! -f source/$base ] && [ -f $o ]; then
    rm $o
    rm bin/$base.md5
  fi
done
# Recompile source files if necessary.
for cpp in source/*.cpp; do
  base=$(basename $cpp)
  include_graph[0]=$base
  include_count=1
  modified=1
  while [ $modified -eq 1 ]; do
    modified=0
    list=""
    i=0
    while [ $i -lt $include_count ]; do
      list="source/${include_graph[$i]} $list"
      let i=$i+1
    done
    vals=$(grep -h '^#include \+\"' $list 2> /dev/null)
    vals=$(echo $vals | sed 's/#include \+"\([^"]\+\)"/\1/g' 2> /dev/null)
    for include in $vals; do
      seen=0
      i=0
      while [ $i -lt $include_count ]; do
        if [[ "$include" == "${include_graph[$i]}" ]]; then
          seen=1
        fi
        let i=$i+1
      done
      if [ $seen -eq 0 ]; then
        modified=1
        include_graph[$include_count]=$include
        let include_count=$include_count+1
      fi
    done
  done
  list=""
  i=0
  while [ $i -lt $include_count ]; do
    list="source/${include_graph[$i]} $list"
    let i=$i+1
  done
  old=$(cat bin/$base.md5 2> /dev/null)
  new=$(md5sum $list 2> /dev/null)
  if [ -f bin/$base.md5 ] && [ "$old" == "$new" ]; then
    echo "Up to date: $cpp"
  else
    echo "Compiling $cpp..."
    $gcc -c $cflag -o bin/$base.o $cpp
    if [ $? -eq 0 ]; then
      md5sum $list > bin/$base.md5 2> /dev/null
    else
      if [ -f bin/$base.md5 ]; then
        rm bin/$base.md5
      fi
      error=1
    fi
    change=1
  fi
done

if [ ! $error -eq 0 ]; then
  exit 1
fi
i=0
# Link binaries.
while [ $i -lt $compile_targets_size ]; do
  target=${compile_targets[$i]}
  objects=""
  for o in bin/*.o; do
    base=$(basename `echo $o | cut -d . -f 1,2`)
    j=0
    excise=0
    while [ $j -lt $targets_size ]; do
      if [[ $base == ${targets[$j]}.cpp ]] && [[ $base != $target.cpp ]]; then
        excise=1
      fi
      let j=$j+1
    done
    if [ $excise -ne 1 ]; then
      objects="$o $objects"
    fi
  done
  if [ $change -eq 1 ] || [ ! -f bin/$target.md5 ]; then
    echo "Linking $target..."
    $gcc $lflag -o bin/$target $objects $libs
    if [ $? -eq 0 ]; then
      md5sum bin/$target > bin/$target.md5
    elif [ -f bin/$target.md5 ]; then
      rm bin/$target.md5
      exit 1
    fi
  else
    echo "Up to date: $target"
  fi
  let i=$i+1
done
