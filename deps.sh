# Usage:
# ./deps.sh depfile buildfile outdir file
DEPFILE=$1
BUILDFILE=$2
OUTDIR=$3
FILE=$4
DIR=$(dirname $FILE)

# Extract the filenames in lines that look like:
#   #include "foo.h"
LIST=$( \
    grep -ho '^#include \+\"[^"]\+\"' $FILE | \
    sed "s/#include \+\"\([^\"]\+\)\"/\1/g")

DEPS=""
for DEP in $LIST; do
  # Prefix directory and collapse pathname. We don't need to support e.g
  # ../../foo.h since we only have one level of source directory nesting right
  # now.
  DEP=$(echo ./$DIR/$DEP | sed "s/[^/.]\+\/..\///g" | \
      sed "s/\/source\//\/$OUTDIR\//g")
  DEPS="$DEPS $DEP.BUILD"
done

# Write dependency file.
rm -f $DEPFILE
# Explicitly mark generated protobuf files as intermediate so that they will be
# deleted afterwards.
echo $FILE | grep '\.pb\.[^.]*$' > /dev/null
if [ $? -eq 0 ]; then
  echo ".INTERMEDIATE: $FILE\n" >> $DEPFILE
fi
# The .BUILD target depends on the corresponding file, and the .BUILD targets
# of the include dependencies of that file. We use double-colon rules to
# suppress target override warnings.
echo ".INTERMEDIATE: $BUILDFILE" >> $DEPFILE
echo "$BUILDFILE:: $FILE $DEPS\n\t@touch $BUILDFILE\n" >> $DEPFILE
