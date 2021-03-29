cmake_minimum_required(VERSION 3.10)
project(Launcher)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

file(GLOB source_files "src/*.cpp" "src/*/*.cpp" "src/*/*.hpp" "include/*.h"  "include/*/*.h" "include/*/*/*.h")
add_executable(${PROJECT_NAME} ${source_files})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "BeamMP-Launcher")

if (WIN32)
    find_package(ZLIB REQUIRED)
    find_package(CURL REQUIRED)
    message(STATUS "MSVC -> forcing use of statically-linked runtime.")
    STRING(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
    STRING(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    #-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static
    set(VcpkgRoot ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET})
    include_directories(${VcpkgRoot}/include)
    link_directories(${VcpkgRoot}/lib)
    target_link_libraries(${PROJECT_NAME} PRIVATE discord-rpc CURL::libcurl ZLIB::ZLIB)
else(WIN32) #MINGW
    add_definitions("-D_WIN32_WINNT=0x0600")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s -static-libgcc -static-libstdc++ --static")
    target_link_libraries(${PROJECT_NAME} discord-rpc.a ssp z ws2_32)
endif(WIN32)

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)