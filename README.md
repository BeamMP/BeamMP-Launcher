# BeamMP-Launcher

The launcher is the way we communitcate to outside the game, it does a few automated actions such as but not limited to: downloading the mod, launching the game, and create a connection to a server.

**To clone this repository**: `git clone --recurse-submodules https://github.com/BeamMP/BeamMP-Launcher.git`

## How to build - Release

In the root directory of the project,
1. `cmake -DCMAKE_BUILD_TYPE=Release . -B bin -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static`
2. `cmake --build bin --parallel --config Release`

Remember to change `C:/vcpkg` to wherever you have vcpkg installed.

## How to build - Debug

In the root directory of the project,
1. `cmake . -B bin -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static`
2. `cmake --build bin --parallel`

Remember to change `C:/vcpkg` to wherever you have vcpkg installed.
