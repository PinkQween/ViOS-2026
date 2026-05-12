#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
EDK2_REPO="$SCRIPT_DIR/edk2-repo"

export WORKSPACE="$SCRIPT_DIR"
export PACKAGES_PATH="$SCRIPT_DIR:$EDK2_REPO"
export EDK_TOOLS_PATH="$EDK2_REPO/BaseTools"

. "$EDK2_REPO/edksetup.sh"

build -p ViOSPkg/ViOSPkg.dsc -a X64 -b RELEASE -t GCC "$@"
