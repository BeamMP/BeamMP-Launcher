#!/bin/bash

set -ex

cmake --build bin --parallel -t BeamMP-Launcher --config Release
