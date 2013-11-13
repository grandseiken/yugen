#!/bin/sh
# Usage:
# ./deps.sh depfile buildfile outdir file
DEPFILE=$1
BUILDFILE=$2
FILE=$3

# Get include dependencies from a single file.
get_deps() {
  DIR=$(dirname $1)
  # Extract the filenames in lines that look like:
  #   #include "foo.h"
  LIST=$( \
      grep -ho '^#include \+\"[^"]\+\"' $1 | \
      sed "s/#include \+\"\([^\"]\+\)\"/\1/g")

  DEPS=""
  for DEP in $LIST; do
    # Prefix directory and collapse pathname.
    DEP="$DIR/$DEP"
    DEP=$(echo $DEP | \
        sed ":repeat; s/[^/]\+\/\(\.\.\/\)*\.\.\//\1/; t repeat")
    DEPS="$DEPS $DEP"
  done

  echo $DEPS
}

# Get dependencies from a set of files that are not contained in a list of
# known dependencies.
get_new_deps() {
  # Find dependencies in first list.
  DEPS=""
  for FILE in $1; do
    T=$(get_deps $FILE)
    DEPS="$DEPS $T"
  done
 
  # Deduplicate and filter these new dependencies by second list.
  NEW_DEPS=""
  ANY=1
  for DEP in $DEPS; do
    echo "$2 $NEW_DEPS" | fgrep $DEP > /dev/null
    if [ $? -ne 0 ]; then
      NEW_DEPS="$NEW_DEPS $DEP"
      ANY=0
    fi
  done

  echo $NEW_DEPS
  return $ANY
}

# Extract deps until we have them all.
DEPS=""
NEW_DEPS="$FILE"
ANY=0
while [ $ANY -eq 0 ]; do
  DEPS="$DEPS $NEW_DEPS"
  NEW_DEPS=$(get_new_deps "$NEW_DEPS" "$DEPS")
  ANY=$?
done

# Write dependency file. The .BUILD target depends on the corresponding file and
# its include dependencies. We use double-colon rules to suppress target
# override warnings.
echo "$BUILDFILE::$DEPS\n\t@touch $BUILDFILE" > $DEPFILE
