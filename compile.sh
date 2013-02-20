if [ ! -d bin ]
then
  mkdir bin
fi
for cpp in source/*.cpp; do
  echo "Compiling $cpp..."
  g++-4.7 -c -O3 -std=c++0x -I./boost_1_53_0/install/include -o bin/$(basename $cpp).o $cpp
done
echo "Linking..."
g++-4.7 -O3 -L./boost_1_53_0/install/lib -o bin/yugen bin/*.o -lboost_filesystem -lboost_system
