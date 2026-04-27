#!/usr/bin/env bash
set -e

BUILD_DIR="build"
BUILD_TYPE="Debug"

build() {
    cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    cmake --build "$BUILD_DIR"
}

clean() {
    rm -rf "$BUILD_DIR"
}

run() {
    build
    "$BUILD_DIR/TikTokArena"
}

debug() {
    cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug
    cmake --build "$BUILD_DIR"
    gdb "$BUILD_DIR/TikTokArena"
}

case "$1" in
    build|"") build ;;
    clean) clean ;;
    run) run ;;
    debug) debug ;;
    rebuild) clean; build ;;
    *) echo "Usage: $0 {build|clean|run|debug|rebuild}"; exit 1 ;;
esac
