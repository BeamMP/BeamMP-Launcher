cmake_minimum_required(VERSION 3.10)
project(Launcher)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

file(GLOB source_files "src/*.cpp" "src/*/*.cpp" "src/*/*.hpp" "include/*.h"  "include/*/*.h" "include/*/*/*.h")
add_executable(${PROJECT_NAME} ${source_files})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "BeamMP-Launcher")
find_package(ZLIB REQUIRED)
find_package(CURL CONFIG REQUIRED)

# This might cause issues with old windows headers, but it's worth the trouble to keep the code
#   completely cross platform. For fixes to common issues arising from /permissive- visit:
#   https://docs.microsoft.com/en-us/cpp/build/reference/permissive-standards-conformance

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /permissive-")
message(STATUS "MSVC -> forcing use of statically-linked runtime.")
STRING(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
STRING(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
#-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static



target_link_libraries(${PROJECT_NAME} PRIVATE rstrtmgr discord-rpc CURL::libcurl ZLIB::ZLIB)
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)