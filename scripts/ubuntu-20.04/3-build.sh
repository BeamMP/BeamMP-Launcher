#!/bin/bash

set -ex

cmake --build bin --parallel -t BeamMP-Launcher

objcopy --only-keep-debug bin/BeamMP-Launcher bin/BeamMP-Launcher.debug
objcopy --add-gnu-debuglink bin/BeamMP-Launcher bin/BeamMP-Launcher.debug

strip -s bin/BeamMP-Launcher
