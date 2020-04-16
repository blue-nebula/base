#! /bin/bash

set -e
set -x

BUILD_DIR=$(mktemp -d redeclipse-legacy-build-XXXXXX)

cleanup () {
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
}

trap cleanup EXIT

# store repo root as variable
REPO_ROOT=$(realpath $(dirname $(dirname $0)))
OLD_CWD=$(realpath .)

pushd "$BUILD_DIR"

cmake "$REPO_ROOT"

make -j$(nprocs)
