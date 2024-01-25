#!/bin/bash

set -ex

apt-get update -y

apt-get install -y curl zip unzip tar cmake make git g++ ninja-build
