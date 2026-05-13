#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
EDK2_REPO="$SCRIPT_DIR/edk2-repo"

export WORKSPACE="$SCRIPT_DIR"
export PACKAGES_PATH="$WORKSPACE:$WORKSPACE/edk2-repo:$WORKSPACE/../../../"
export EDK_TOOLS_PATH="$EDK2_REPO/BaseTools"
export CONF_PATH="$SCRIPT_DIR/Conf"
export PYTHON_COMMAND=python3

. "$EDK2_REPO/edksetup.sh"

build -p ViOSPkg/ViOSPkg.dsc -a X64 -b RELEASE -t GCC "$@"
