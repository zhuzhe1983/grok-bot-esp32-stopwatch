#!/usr/bin/env bash
set -euo pipefail
task_root="$(cd "$(dirname "$0")/.." && pwd -P)"
: "${IDF_PATH:?Source ESP-IDF 5.5.1 export.sh first}"
idf.py -C "$task_root" -B "$task_root/build" build
python3 "$task_root/scripts/package.py"
