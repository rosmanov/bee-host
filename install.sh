#!/bin/bash -
# Installs native messaging host application for Browser's External Editor extension
#
# Arguments:
# $1: Optional target directory.
#
# Copyright © 2019,2020 Ruslan Osmanov <rrosmanov@gmail.com>

set -e -u

# Prints a fatal error
err()
{
  echo >&2 "Error: $@"
  exit 1
}

# Get the current directory
dir=$(cd "$(dirname "$0")" && pwd)

[ $# -ne 0 ] && target_dir="$1"

source "$dir/vars.sh"
: ${target_dir:="$dir"}
save_vars_cache

source_host_file='beectl'

# Work from the current directory
cd "$dir"

# Copy app to the target path
target_path="$target_dir/$target_file"
install -D -m 0755 "$dir/$source_host_file" "$target_path" && \
  printf "Installed host application into '%s'\n" "$target_path"

# Copy manifests into browser-specific directories
json_patch="{\"path\":\"$target_path\"}"
tmp_manifest_file='tmp-manifest.json'

target_manifest_path="$chrome_target_manifest_dir/$target_manifest_file"
./json-patch "$dir/$chrome_manifest_file" "$json_patch" > "$tmp_manifest_file" && \
install -D -m 0644 "$tmp_manifest_file" "$target_manifest_path" && \
  printf "Installed Chrome manifest into '%s'\n" "$target_manifest_path"

target_manifest_path="$firefox_target_manifest_dir/$target_manifest_file"
./json-patch "$dir/$firefox_manifest_file" "$json_patch" > "$tmp_manifest_file" && \
install -D -m 0644 "$tmp_manifest_file" "$target_manifest_path" && \
  printf "Installed Firefox manifest into '%s'\n" "$target_manifest_path"
