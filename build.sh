#!/bin/sh
# Accepts optional path to toolchain (./CMake/Toolchain-*.cmake) as $1.
# If $1 is specified, rebuilds the target pointed by the toolchain. Otherwise,
# rebuilds targets specified by all ./CMake/Toolchain-*.cmake files.
#
# Usage
# -----
#
# mkdir -p build && cd build
#
# Run all toolchains using build type 'Release':
# ../build.sh all -b Release
#
# Run all toolchains using build type 'Debug':
# pass 'VERBOSE=1' to "make" command:
# ../build.sh all -b Debug VERBOSE=1
#
# run windows toolchain using build type 'Release':
# ../build.sh ../CMake/Toolchain-Windows-i686.cmake -b Release
#
# Run all toolchains using build type 'Debug' (default):
# ../build.sh

# Resolve absolute path to the script
script="$0"
while [ -h "$script" ]; do
    ls_output=$(ls -ld "$script")
    link=$(expr "$ls_output" : '.*-> \(.*\)$')
    case "$link" in
        /*) script="$link" ;;
        *) script="$(dirname "$script")/$link" ;;
    esac
done
project_dir="$(cd "$(dirname "$script")" && pwd)"

. "${project_dir}/helpers.sh"

toolchain="$1"
shift

[ -z "$toolchain" ] && toolchain="all"

# Default: use 'build' directory only if we're in the project root.
build_dir=''
cwd="$(pwd)"
# If run from project root, default to 'build'.
printf 'Checking if cwd (%s) = project_dir (%s)\n' "$cwd" "$project_dir"
if [ "$cwd" = "$project_dir" ]; then
    printf 'Using default build directory: %s\n' "$project_dir/build"
    build_dir='build'
fi

case "$toolchain" in
  all)
      for t in "$project_dir/CMake/Toolchain-"*.cmake ; do
          case "$(basename "$t")" in
              *macos*)
                  if [ "$(get_os)" != "$OS_MACOS" ]; then
                      echo >&2 "Skipping $t on non-macOS host"
                      continue
                  fi
                  ;;
          esac
          echo Processing "$t"
          sh "$script" "$t" "$@"
      done
      ;;
  *)
    build_type='Debug'
    package_type='binary'

    while getopts ':b:p:d:' opt
    do
      case "$opt" in
        b) build_type="$OPTARG" ;;
        p) package_type="$OPTARG" ;;
        d) build_dir="$OPTARG" ;;
        \?) echo >&2 "Unknown option: '$OPTARG'" ;;
        :) echo >&2 "Option -$OPTARG requires an argument." ;;
      esac
    done
    # Shift the last option
    shift $(( OPTIND - 1 ))

    echo "Using toolchain $toolchain in '$build_type' mode"
    case "$toolchain" in
        *Windows*)
            : ${CPACK_GENERATOR:='NSIS'}
            ;;
        *Linux*)
            : ${CPACK_GENERATOR='RPM;DEB;TGZ;ZIP'}
            : ${CPACK_SOURCE_GENERATOR='RPM;TGZ;ZIP'}
            ;;
        *Darwin*|*macos*)
            : ${CPACK_GENERATOR="productbuild"}
            ;;
        *)
            echo >&2 "⚠️ Unknown toolchain: $toolchain"
            CPACK_GENERATOR='TGZ'  # fallback
            ;;
    esac

    if [ -n "$build_dir" ]; then
        printf 'Using build directory: %s\n' "$build_dir"
        mkdir -p "$build_dir" || exit 10
        cd "$build_dir" || exit 10
    else
        build_dir="$(pwd)"
        printf 'Using current directory as build directory: %s\n' "$build_dir"
    fi

    rm -rf CMakeCache.txt CMakeFiles _CPack_Packages external
    cmake --no-warn-unused-cli \
        -DCMAKE_TOOLCHAIN_FILE="$toolchain" \
        -DCMAKE_BUILD_TYPE="$build_type" \
        -DCPACK_RPM_PACKAGE_SOURCES=${CPACK_RPM_PACKAGE_SOURCES:=OFF} \
        -DCPACK_GENERATOR="$CPACK_GENERATOR"  \
        -DCPACK_SOURCE_GENERATOR="$CPACK_SOURCE_GENERATOR"  \
        -DCPACK_PACKAGING_INSTALL_PREFIX="${CPACK_PACKAGING_INSTALL_PREFIX:=/}" \
        -DCPACK_INSTALLED_DIRECTORIES="${CPACK_INSTALLED_DIRECTORIES:=''}" \
        "$project_dir"

    # Create symlink to compile_commands.json
    src_compile_commands="$(pwd)/compile_commands.json"
    dst_compile_commands="$project_dir/compile_commands.json"

    if [ -f "$src_compile_commands" ]; then
        current_link=$(readlink "$dst_compile_commands" 2>/dev/null)
        if [ "$current_link" != "$src_compile_commands" ]; then
            echo "Linking compile_commands.json to project root"
            ln -sf "$src_compile_commands" "$dst_compile_commands"
        fi
    else
        echo "⚠️  $src_compile_commands not found. Skipping symlink."
    fi

    make "$@"

    printf 'Building %s package\n' "$package_type"
    case "$package_type" in
        binary) make package ;;
        source) make package_source ;;
        *) printf >&2 'Unknown package type %s\n' "$package_type" ;;
    esac
    ;;
esac

# vim: ft=bash
