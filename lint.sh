#!/bin/bash
# Usage: lint.sh
cd "$(dirname "$0")"

function lint
{
  egrep -n "$1" $file | sed -e "s/^\([0-9]\+\):/$2:\t/g"
}

function lint_highlight
{
  sed -e "s/ *\(\/\|*\)\*\*\+\/\?//g" $file | egrep -n "$1" | sed -e "s/^\([0-9]\+\):/$2:\t/g" | egrep -B 9999 -A 9999 --color "$1"
}

function lint_common
{
  lint '^.{81}.*$' 'Line \1 too long'
  lint_highlight '\( | \)|\[ | \]|\{ |[^ ] \}|[^ ]  +[^ ]' 'Line \1 whitespace'
  lint_highlight ' +$' 'Line \1 trailing whitespace'
  lint_highlight $'\t' 'Line \1 tab'
  lint_highlight ',[^ "].|;[^ }"].|:[^ :A-Za-z_*~"].' 'Line \1 separator format'
  lint_highlight '(\]|\)|\])[^, ;)\]}]' 'Line \1 bracket format'
}

function lint_cstyle
{
  lint_highlight '[^ ] +static' 'Line \1 bad order'
  lint_highlight '\} *else' 'Line \1 else line'
}

# TODO: lint things on adjacent lines (for indent checking).
for file in source/*.* source/proto/*.proto; do
  echo Linting: $file
  lint_common
  lint_cstyle
  lint_highlight '[^A-Za-z_0-9](delete|int|short|long)[^A-Za-z_0-9]' 'Line \1 not allowed'
done

for file in data/shaders/*; do
  echo Linting: $file
  lint_common
  lint_cstyle
done

for file in data/scripts/*; do
  echo Linting: $file
  lint_common
done
