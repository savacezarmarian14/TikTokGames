#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
BUILD_TYPE="Debug"
RUN_AFTER_BUILD=0
CLEAN_FIRST=0
RUN_TOWER_COUNT=""
SOUNDS_SRC_DIR="assets/sounds"
SOUNDS_BUILD_DIR="$BUILD_DIR/assets/sounds"
GIFT_ICONS_SRC_DIR="assets/gift_icons"
GIFT_ICONS_BUILD_DIR="$BUILD_DIR/assets/gift_icons"

NPROC="$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"

usage() {
  cat <<EOF
Usage: $0 [--release] [--clean] [--run N]

Options:
  --release       Build the Release configuration.
  --clean         Remove the existing build tree before configuring.
  --run N         Execute the built binary with N towers after build.
  --help, -h      Show this help message.
EOF
}

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
        usage
        exit 1
      fi

      RUN_TOWER_COUNT="$1"
      if ! [[ "$RUN_TOWER_COUNT" =~ ^[0-9]+$ ]] || [[ "$RUN_TOWER_COUNT" -lt 2 ]] || [[ "$RUN_TOWER_COUNT" -gt 4 ]]; then
        echo "N must be an integer between 2 and 4"
        usage
        exit 1
      fi
      shift
      ;;
    --clean)
      CLEAN_FIRST=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1"
      usage
      exit 1
      ;;
  esac
done

if [ "$CLEAN_FIRST" -eq 1 ]; then
  rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"

cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR" --parallel "$NPROC"

if [ ! -d "$SOUNDS_SRC_DIR" ]; then
  echo "Missing audio assets directory: $SOUNDS_SRC_DIR"
  exit 1
fi

mkdir -p "$SOUNDS_BUILD_DIR"
cmake -E copy_directory "$SOUNDS_SRC_DIR" "$SOUNDS_BUILD_DIR"

if [ ! -d "$GIFT_ICONS_SRC_DIR" ]; then
  echo "Missing gift icon assets directory: $GIFT_ICONS_SRC_DIR"
  exit 1
fi

mkdir -p "$GIFT_ICONS_BUILD_DIR"
cmake -E copy_directory "$GIFT_ICONS_SRC_DIR" "$GIFT_ICONS_BUILD_DIR"

if [ "$RUN_AFTER_BUILD" -eq 1 ]; then
  (
    cd "$BUILD_DIR"
    TIKTOK_WAR_AUDIO="${TIKTOK_WAR_AUDIO:-1}" ./tiktok_war "$RUN_TOWER_COUNT"
  )
fi
