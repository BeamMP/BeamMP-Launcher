#!/bin/bash

set -ex

cmake . -B bin $1 -DCMAKE_BUILD_TYPE=Release -DBeamMP-Launcher_ENABLE_LTO=ON -DVCPKG_TARGET_TRIPLET=x64-windows-static
