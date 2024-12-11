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

Should you run out of RAM while building, you can ommit the `--parallel` intruction, it will then use less RAM due to building only on one CPU thread.

You can also specify a number of threads to use, for example `--parallel 4` will use four CPU threads, but due to the small project size, you may be faster just omitting `--parallel` instead of trying to find the highest possible multithread number
