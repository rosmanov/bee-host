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
        echo "Options:"
        echo "  -b build_type  Specify the build type: Debug or Release (default: ${build_type})"
        echo "  -h             Show this help message"
        exit
        ;;
  esac
done

if [ -d "$artifacts_dir" ]; then
    if ! read_yes "Artifacts directory already exists. Do you want to remove it? [Y/n] " 'Y'; then
        die "Artifacts directory already exists: ${artifacts_dir}"
    fi
    info "Removing existing artifacts directory: ${artifacts_dir}"
    rm -rf "$artifacts_dir"
fi

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

        macos_artifacts_dir="${artifacts_dir}/macos"
        mkdir -p "$macos_artifacts_dir"
        cp "${build_dir}"/*.pkg "${macos_artifacts_dir}/"
        info 'Building macOS complete.'
        info "macOS artifacts are available in: ${macos_artifacts_dir}"
        ;;
esac

info 'Building cross-platform artifacts complete.'
info "Artifacts are available in: ${artifacts_dir}"

# vim: ft=bash
