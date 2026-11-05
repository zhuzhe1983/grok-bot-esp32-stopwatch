#!/usr/bin/env bash
set -euo pipefail
task_root="$(cd "$(dirname "$0")/.." && pwd -P)"
cmake -S "$task_root/tests" -B "$task_root/build-native" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$task_root/build-native" --parallel
ctest --test-dir "$task_root/build-native" --output-on-failure
