#!/bin/bash -

project_dir="$(dirname "$0")"
"$project_dir"/build.sh "$project_dir"/CMake/Toolchain-Windows-amd64.cmake "$@"
