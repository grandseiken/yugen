#!/bin/bash
# Usage: lint.sh
cd "$(dirname "$0")"

function lint {
  egrep -n "$1" $file | sed -e "s/^\([0-9]\+\):/$2:\t/g"
}

function lint_highlight {
  egrep -n "$1" $file | sed -e "s/^\([0-9]\+\):/$2:\t/g" | egrep --color "$1"
}

for file in source/*; do
  echo Linting: $file
  lint '^.{81}.*$' 'Line \1 too long'
  lint_highlight '\( | \)|\[ | \]|\{ |[^ ] \}' 'Line \1 whitespace'
  lint_highlight ' +$' 'Line \1 trailing whitespace'
done
