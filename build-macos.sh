#!/bin/bash -

project_dir="$(dirname "$0")"
"$project_dir"/build.sh "$project_dir"/CMake/Toolchain-macos-arm64.cmake "$@"
"$project_dir"/build.sh "$project_dir"/CMake/Toolchain-macos-x86_64.cmake "$@"
