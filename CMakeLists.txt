cmake_minimum_required(VERSION 3.10)
project(Launcher)

if (WIN32)
    message(STATUS "MSVC -> forcing use of statically-linked runtime.")
    STRING(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
    STRING(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
endif(WIN32)

add_subdirectory("evpp")

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

file(GLOB source_files "src/*.cpp" "src/*/*.cpp" "src/*/*.hpp" "include/*.h"  "include/*/*.h" "include/*/*/*.h")

add_executable(${PROJECT_NAME} ${source_files})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "BeamMP-Launcher")

target_include_directories(${PROJECT_NAME} PUBLIC include evpp)

add_definitions("-DEVPP_HTTP_CLIENT_SUPPORTS_SSL")

if (WIN32)
    find_package(glog CONFIG REQUIRED)
    find_package(Libevent CONFIG REQUIRED)
    find_package(ZLIB REQUIRED)
    find_package(OpenSSL REQUIRED)
    #-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static
    set(VcpkgRoot ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET})
    include_directories(${VcpkgRoot}/include)
    link_directories(${VcpkgRoot}/lib)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${VcpkgRoot}/lib/discord-rpc.lib evpp_static
            ${VcpkgRoot}/lib/event.lib libevent::core libevent::extra glog::glog ZLIB::ZLIB
            OpenSSL::SSL OpenSSL::Crypto libevent::openssl ws2_32)
else(WIN32) #MINGW
    add_definitions("-D_WIN32_WINNT=0x0600")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s -static-libgcc -static-libstdc++ --static")
    target_link_libraries(${PROJECT_NAME} discord-rpc evpp_static glog event pthread ssl crypto event_openssl ws2_32 ssp dbghelp iphlpapi z)
endif(WIN32)