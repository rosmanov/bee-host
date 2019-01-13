#!/bin/sh

dir=$(cd "$(dirname "$0")" && pwd)

infile="$1"
host_path="$2"

if perldoc -l JSON::XS >/dev/null || perldoc -l JSON >/dev/null ; then
  perl $dir/patch-manifest.pl "$@"
  exit
fi

