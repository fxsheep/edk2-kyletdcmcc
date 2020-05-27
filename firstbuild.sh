#!/bin/bash
set -e
mkdir workspace
. build_common.sh
make -C ../edk2/BaseTools
