OS_MACOS=macos
OS_LINUX=linux

# Exits with an error code.
# $1 Optional error message
die() {
    [ $# -gt 0 ] && printf >&2 '(!!) %s\n' "$1"
    exit 2
}
info() {
    printf >&2 '(i) %s\n' "$1"
}

command_exists() {
    command -v "$1" >/dev/null
}

get_os() {
    : ${OSTYPE:=$(uname -s)}
    case "$OSTYPE" in
        [Ll]inux*)
            printf '%s\n' "$OS_LINUX"
            ;;
        [Dd]arwin*)
            printf '%s\n' "$OS_MACOS"
            ;;
        *)
            die "Unsupported OS: $OSTYPE."
            ;;
    esac
}

get_project_dir() {
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

    printf '%s\n' "$(cd "$(dirname "$script")" && pwd)"
}

# vim: ft=bash
