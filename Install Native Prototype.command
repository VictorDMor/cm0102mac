#!/bin/bash
# One-time installation of the supplied game files and existing native build.
set -u
project_dir="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)" || exit 1
printf 'CM 01-02 — Native ARM64 prototype installation\n\n'
printf 'Open CM-01-02.cdr before the first installation. Later runs use the installed files.\n'
printf 'This is a playable development build; some services remain unfinished.\n\n'
python3 "$project_dir/tools/install_native.py"
result=$?
if [[ -t 0 ]]; then
    printf '\nPress Return to close this installer. '
    read -r answer
fi
exit "$result"
