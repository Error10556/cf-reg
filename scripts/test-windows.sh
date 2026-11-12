#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
work="$root/build-win"
runtime="$work/native-dist"
export WINEPREFIX="$work/wine-test"
export WINEDEBUG=${WINEDEBUG:--all,err+all}
export WINEPATH="Z:${runtime//\//\\}"
if (( $# == 0 )); then set -- xlmodel gui; fi
for test in "$@"; do
    echo "Running Windows $test test under Wine"
    timeout 120s wine "$work/native/${test}_test.exe"
    echo "$test passed"
done
