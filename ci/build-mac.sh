#! /bin/bash

set -e
set -x

BUILD_DIR=$(mktemp -d blue-nebula-build-XXXXXX)

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

# can be overwritten by the user
BUILD_TYPE="${BUILD_TYPE:-Debug}"
cmake "$REPO_ROOT" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

make -j$(nproc)
