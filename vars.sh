#!/bin/bash -
# List of variables shared between install.sh and uninstall.sh
#
# Copyright © 2019 Ruslan Osmanov <rrosmanov@gmail.com>

set -e -u

# Current directory
readonly dir=$(cd "$(dirname "$0")" && pwd)
# Cache file for uninstall.sh
readonly vars_cache_file="$dir/vars.cache.sh"

# Saves variables into $vars_cache_file
save_vars_cache() {
  set > "$vars_cache_file" && \
    echo "Saved cache into $vars_cache_file"
}

# Restores variables from $vars_cache_file
restore_vars_cache() {
  # Do nothing, if cache file is not readable
  [ -r "$vars_cache_file" ] || return
  # Prevent exit on errors attempting to assign read-only variables
  set +o errexit
  source "$vars_cache_file" 2>/dev/null && \
    echo "Restored cache from $vars_cache_file"
  # Restore errexit option
  set -o errexit
}

# Set target manifest directory paths for all browsers
kernel=$(uname -s)
case "$kernel" in
  Darwin)
    if [ $EUID == 0 ]; then
      # If superuser
      chrome_target_manifest_dir='/Library/Google/Chrome/NativeMessagingHosts'
      firefox_target_manifest_dir='/Library/Application Support/Mozilla/NativeMessagingHosts'
    else
      # If normal user
      chrome_target_manifest_dir="$HOME/Library/Application Support/Google/Chrome/NativeMessagingHosts"
      firefox_target_manifest_dir="$HOME/Library/Application Support/Mozilla/NativeMessagingHosts"
    fi
    ;;
  *)
    if [ $EUID == 0 ]; then
      # If superuser
      chrome_target_manifest_dir='/etc/opt/chrome/native-messaging-hosts'
      firefox_target_manifest_dir='/usr/lib/mozilla/native-messaging-hosts'
    else
      # If normal user
      chrome_target_manifest_dir="$HOME/.config/google-chrome/NativeMessagingHosts"
      firefox_target_manifest_dir="$HOME/.mozilla/native-messaging-hosts"
    fi
    ;;
esac

# Set target_dir default value
if [ $EUID == 0 ]; then
  # Superuser. Pick a globally accessible path
  : ${target_dir:=/opt/osmanov/WebExtensions}
else
  # Normal user. Local installation defaults to the current project directory
  : ${target_dir:="$dir"}
fi

# Target host application filename
target_file=beectl
# Host application name which is specified in the manifest file
host_name=com.ruslan_osmanov.bee
# Source manifest filenames
chrome_manifest_file="chrome-${host_name}.json"
firefox_manifest_file="firefox-${host_name}.json"
# Target manifest filename
target_manifest_file="${host_name}.json"
