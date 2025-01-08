# BeamMP-Launcher

The launcher is the way we communitcate to outside the game, it does a few automated actions such as but not limited to: downloading the mod, launching the game, and create a connection to a server.

**To clone this repository**: `git clone --recurse-submodules https://github.com/BeamMP/BeamMP-Launcher.git`

## How to build for Windows

Make sure you have the necessary development tools installed:

[vcpkg](https://vcpkg.io/en/)

### Release

In the root directory of the project,
1. `cmake -DCMAKE_BUILD_TYPE=Release . -B bin -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static`
2. `cmake --build bin --parallel --config Release`

Remember to change `C:/vcpkg` to wherever you have vcpkg installed.

### Debug

In the root directory of the project,
1. `cmake . -B bin -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static`
2. `cmake --build bin --parallel`

Remember to change `C:/vcpkg` to wherever you have vcpkg installed.

## How to build for Linux

Make sure you have `vcpkg` installed, as well as basic development tools, often found in packages, for example:

- Debian: `sudo apt install build-essential`
- Fedora: `sudo dnf groupinstall "Development Tools"`
- Arch: `sudo pacman -S base-devel`
- openSUSE: `zypper in -t pattern devel-basis`

### Release

In the root directory of the project,
1. `cmake -DCMAKE_BUILD_TYPE=Release . -B bin -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-linux`
2. `cmake --build bin --parallel --config Release`

### Debug

In the root directory of the project,
1. `cmake . -B bin -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-linux`
2. `cmake --build bin --parallel`

## Running out of RAM while building

Should you run out of RAM while building, you can ommit the `--parallel` instruction, it will then use less RAM due to building only on one CPU thread.

You can also specify a number of threads to use, for example `--parallel 4` will use four CPU threads, but due to the small project size, you may be faster just omitting `--parallel` instead of trying to find the highest possible multithread number


## License

BeamMP Launcher, a launcher for the BeamMP mod for BeamNG.drive
Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
