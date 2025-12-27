#! /bin/bash

set -euxo pipefail

if [ "$ARCH" == "" ]; then
    ARCH=$(uname -m)
    echo "Warning: \$ARCH unset, guessing as $ARCH"
fi

# use RAM disk if possible
if [ "$CI" == "" ] && [ -d /dev/shm ]; then
    TEMP_BASE=/dev/shm
else
    TEMP_BASE=/tmp
fi

# save one processor core if possible for other stuff on dev machines
if [ "$CI" == "" ]; then
    NPROC=$(nproc --ignore=1)
else
    NPROC=$(nproc)
fi

BUILD_DIR=$(mktemp -d -p "$TEMP_BASE" blue-nebula-build-XXXXXX)

cleanup () {
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
}

trap cleanup EXIT

# store repo root as variable
OLD_CWD="$(readlink -f .)"
REPO_ROOT="$(readlink -f "$(dirname "${BASH_SOURCE[0]}")")/../.."

pushd "$BUILD_DIR"

mkdir build
cd build

cmake "$REPO_ROOT" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr

make -j"$NPROC"

make install DESTDIR=AppDir &>install.log

wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-"$ARCH".AppImage

chmod +x linuxdeploy*.AppImage

git_version="$(cd "$REPO_ROOT" && git describe --tags)"

# configure AppImageUpdate
export UPD_INFO="gh-releases-zsync|blue-nebula|release|continuous|Blue_Nebula-*$ARCH.AppImage.zsync"
export VERSION="$git_version"

./linuxdeploy-"$ARCH".AppImage --appdir AppDir --output appimage

mv Blue_Nebula*.AppImage* "$OLD_CWD"
