if [ ! -d bin ]
then
  mkdir bin
fi
for cpp in source/*.cpp; do
  echo "Compiling $cpp..."
  g++ -c -O3 -std=c++0x -o bin/$(basename $cpp).o $cpp
done
echo "Linking..."
g++ -O3 -o bin/yugen bin/*.o
