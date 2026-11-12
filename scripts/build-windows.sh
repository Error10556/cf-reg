#!/usr/bin/env bash
# SPDX-License-Identifier: 0BSD
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
work="$root/build-win"
mkdir -p "$work"
cmake -S "$root" -B "$work/native" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$root/cmake/mingw-w64.cmake" \
    -DCMAKE_BUILD_TYPE=Release -DCFREG_BUILD_GUI=ON -DBUILD_TESTING=ON
cmake --build "$work/native" --parallel "${JOBS:-$(nproc)}"
python3 "$root/scripts/package-windows.py"
