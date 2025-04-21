#!/bin/bash -
#
# Uninstall native messaging host
#
# Copyright © 2014-2025 Ruslan Osmanov <608192+rosmanov@users.noreply.github.com>

set -e -u

dir=$(cd "$(dirname "$0")" && pwd)

# Set common configuration variables
source "$dir/vars.sh"

# Restore configuration varibales saved in insallation phase
restore_vars_cache

# Remove installed files
readonly files=( \
  "$target_dir/$target_file" \
  "$chrome_target_manifest_dir/$target_manifest_file" \
  "$chromium_target_manifest_dir/$target_manifest_file" \
  "$firefox_target_manifest_dir/$target_manifest_file" \
  )
printf 'Removing %s\n' "${files[*]}"
rm -f "${files[@]}"
