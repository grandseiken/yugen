FILES=`find ./source | egrep ".cpp$|.c$|.proto$"`
for x in $FILES; do
  XMOD=`echo $x | sed "s/\.proto$/.pb.cc/g"`
  OUT=`echo $XMOD | sed "s/\/source\//\/$1\//g"`
  OUT=$OUT.deps
  DIR=`dirname $x`
  DEPEND=`grep -h '^\(#include\|import\) \+\"' $x | \
    sed "s/\(#include\|import\) \+\"\([^\"]\+\)\"/\2/g"`
  LIST=""
  for y in $DEPEND; do
    y=`echo $y | sed "s/\.proto;/.pb.h/g"`
    D=`echo $DIR/$y | sed "s/[^/]\+\/..\///g"`
    LIST="$LIST $D"
  done
  echo $XMOD: $LIST > $OUT
done
