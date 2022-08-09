# BeamMP-Launcher

The launcher is the way we communicate to servers, it does a few automated actions such as but not limited to:
downloading the mod, launching the game, and establishing server connections.


# Dependencies

it is recommended to use vcpkg and link it to clion or your cmake project via the cmake options using `-DCMAKE_TOOLCHAIN_FILE=PATH_VCPKG/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static`
notice how we specify the target being windows-static you need to install the `static` versions of these dependencies.

- [zlib](https://github.com/madler/zlib) for compression (might be soon removed)
- [discord-rpc](https://github.com/discord/discord-rpc) for discord rich presence 
- [nlohmann-json](https://github.com/nlohmann/json) for reading and parsing JSON 
- [openssl](https://github.com/openssl/openssl) for secure networking 
- [minhook](https://github.com/TsudaKageyu/minhook) for function hooking

# Copyright

Copyright (c) 2019-present Anonymous275 (@Anonymous-275), 
BeamMP-Launcher code is not in the public domain and is not free software. 
One must be granted explicit permission by the copyright holder(s) in order to modify or distribute any part of the source or binaries. 
Special permission to modify the source-code is implicitly granted only for the purpose of upstreaming those changes directly to github.com/BeamMP/BeamMP-Launcher via a GitHub pull-request.
Commercial usage is prohibited, unless explicit permission has been granted prior to usage.
