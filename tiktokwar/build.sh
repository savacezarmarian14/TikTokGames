#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
BUILD_TYPE="Debug"
RUN_AFTER_BUILD=0
CLEAN_FIRST=0
RUN_TOWER_COUNT=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --release)
      BUILD_TYPE="Release"
      shift
      ;;
    --run)
      RUN_AFTER_BUILD=1
      shift

      if [[ $# -eq 0 ]]; then
        echo "Missing value for --run"
        echo "Usage: ./build.sh [--release] [--clean] [--run N]"
        exit 1
      fi

      RUN_TOWER_COUNT="$1"
      if ! [[ "$RUN_TOWER_COUNT" =~ ^[0-9]+$ ]] || [[ "$RUN_TOWER_COUNT" -lt 3 ]]; then
        echo "N must be an integer >= 3"
        echo "Usage: ./build.sh [--release] [--clean] [--run N]"
        exit 1
      fi
      shift
      ;;
    --clean)
      CLEAN_FIRST=1
      shift
      ;;
    *)
      echo "Unknown option: $1"
      echo "Usage: ./build.sh [--release] [--clean] [--run N]"
      exit 1
      ;;
  esac
done

if [ "$CLEAN_FIRST" -eq 1 ]; then
  rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ..
cmake --build . -j"$(nproc)"

if [ "$RUN_AFTER_BUILD" -eq 1 ]; then
  ./tiktok_war "$RUN_TOWER_COUNT"
fi