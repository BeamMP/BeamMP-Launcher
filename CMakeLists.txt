cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
project(BeamMP-Launcher)
set(CMAKE_CXX_STANDARD 17)

file(GLOB source_files
        "src/*.cpp" "src/include/*.h"
        "src/include/*.hpp" "src/curl/*.h"
        "src/Network 2.0/*.cpp" "src/Network 2.0/*.h"
        "src/Memory/*.hpp" "src/Memory/*.cpp")

add_executable(BeamMP-Launcher ${source_files})
target_link_libraries(BeamMP-Launcher ws2_32 urlmon discord-rpc libcurl_a zlibstatic)