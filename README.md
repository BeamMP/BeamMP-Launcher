# BeamMP-Launcher

The launcher is the way we communitcate to outside the game, it does a few automated actions such as but not limited to: downloading the mod, launching the game, and create a connection to a server.

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

Copyright (c) 2019-present Anonymous275.
BeamMP Launcher code is not in the public domain and is not free software.
One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries,
the only permission that has been granted is to use the software in its compiled form as distributed from the BeamMP.com website.
Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
