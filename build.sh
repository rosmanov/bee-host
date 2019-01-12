#!/bin/bash -
# Accepts optional path to toolchain (./CMake/Toolchain-*.cmake) as $1.
# If $1 is specified, rebuilds the target pointed by the toolchain. Otherwise,
# rebuilds targets specified by all ./CMake/Toolchain-*.cmake files.
#
# Usage
# -----
#
# Run all toolchains using build type 'Release':
# ./build.sh all -b Release
#
# Run all toolchains using build type 'Debug':
# pass 'VERBOSE=1' to "make" command:
# ./build.sh all -b Debug VERBOSE=1
#
# Run Windows toolchain using build type 'Release':
# ./build.sh ./CMake/Toolchain-Windows-i686.cmake -b Release
#
# Run all toolchains using build type 'Debug' (default):
# ./build.sh

cd "$(dirname "$0")"

toolchain="$1"
shift

: ${toolchain:=all}

case "$toolchain" in
  all)
    for t in ./CMake/Toolchain-*.cmake ; do
      echo Processing "$t"
      "$0" "$t" "$@"
    done
    ;;
  *)
    : ${build_type:=Debug}

    while getopts ':b:' f
    do
      case $f in
        b)
          build_type="$OPTARG"
          ;;
        \?)
          echo >&2 "Unknown option: '$OPTARG'"
          ;;
        :)
          echo >&2 "Option -$OPTARG requires an argument."
          ;;
      esac
    done
    # Shift the last option
    shift $(( OPTIND - 1 ))

    echo "Using toolchain $toolchain in '$build_type' mode"
    rm -rf CMakeCache.txt CMakeFiles
    cmake -DCMAKE_TOOLCHAIN_FILE="$toolchain" -DCMAKE_BUILD_TYPE="$build_type" .
    make VERBOSE=1 "$@"
esac
