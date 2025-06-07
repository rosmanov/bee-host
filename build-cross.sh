#!/bin/sh -
# Builds cross-platform artifacts for BeeCTL
#
# Usage:
# ./build-cross.sh [-b build_type] [-h]
set -e

project_dir="$(cd "$(dirname "$0")" && pwd)"
cd "$project_dir"

. helpers.sh

build_type="Release"
artifacts_dir="${project_dir}/artifacts"

while getopts "b:h" opt; do
  case $opt in
    b) build_type="$OPTARG" ;;
    h)
        echo "Usage: $0 [-b build_type] [-h]"
        exit
        ;;
  esac
done

info 'Building Linux and Windows binaries'
DOCKER_BUILDKIT=1 docker build \
    --platform=linux/amd64 \
    --build-arg BUILD_TYPE="${build_type}" \
    -t beectl-build-artifacts .
docker create --name beectl-extract-artifacts beectl-build-artifacts
mkdir -p "$artifacts_dir"
docker cp beectl-extract-artifacts:/artifacts/. "$artifacts_dir"
docker rm beectl-extract-artifacts

case "$(get_os)" in
    "$OS_MACOS")
        info 'Building macOS'
        build_dir="${project_dir}/build/macos"
        mkdir -p "$build_dir"
        ./build-macos.sh -b "$build_type" -d "$build_dir" || {
            die 'Failed to build macOS artifacts.'
        }
        cp "${build_dir}"/*.pkg "${artifacts_dir}/"
        info 'Building macOS complete.'
        info "macOS artifacts are available in: ${artifacts_dir}"
        ;;
esac

info 'Building cross-platform artifacts complete.'
info "Artifacts are available in: ${artifacts_dir}"

# vim: ft=bash
