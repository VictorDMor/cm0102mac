#!/bin/bash
# Double-click to build the game from this working copy and install it: the same as
# `tools/dev.sh build cm-native-prototype` followed by `tools/dev.sh install`.
# Saves and game data are preserved; the installer refuses while the game is running.
set -u
project_dir="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)" || exit 1
cd "$project_dir" || exit 1
printf 'reCM 01-02: build and install\n\n'
if [[ ! -e .local/current/build ]]; then
    printf 'No build directory (.local/current/build). Set the project up first (README.md).\n'
    result=1
else
    printf 'Version: %s\n\n' "$(git log -1 --format='%h %s (%cd)' --date=format:'%Y-%m-%d %H:%M' 2>/dev/null)"
    # Waits for its turn if another build holds the build lock.
    printf 'Building (a few minutes when much has changed)...\n'
    if tools/dev.sh build cm-native-prototype 2>&1 | tail -n 3; [[ ${PIPESTATUS[0]} -ne 0 ]]; then
        printf '\nThe build failed; nothing was installed.\n'
        result=1
    else
        printf '\n'
        tools/dev.sh install
        result=$?
    fi
fi
if [[ -t 0 ]]; then
    printf '\nPress Return to close this window. '
    read -r answer
fi
exit "$result"
