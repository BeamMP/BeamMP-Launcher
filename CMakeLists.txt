cmake_minimum_required(VERSION 3.0)
project(BeamMP-Launcher)

if (WIN32)
    message(STATUS "MSVC -> forcing use of statically-linked runtime.")
    STRING(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
    STRING(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
    STRING(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    STRING(REPLACE "/MDd" "/MTd" CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
    find_package(ZLIB REQUIRED)
    find_package(OpenSSL REQUIRED)
    #-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static
    set(VcpkgRoot ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET})
    include_directories(${VcpkgRoot}/include)
    link_directories(${VcpkgRoot}/lib)
endif(WIN32)

set(wxUSE_STL ON)
set(wxBUILD_SHARED OFF)
set(wxBUILD_MSVC_MULTIPROC ON)
set(wxBUILD_USE_STATIC_RUNTIME ON)
add_subdirectory(include/wxWidgets)
add_subdirectory(include/rapidjson)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

add_executable(${PROJECT_NAME}
        src/main.cpp include/easylogging++.h
        src/Launcher.cpp include/Launcher.h
        src/Logger.cpp include/Logger.h
        src/gui/Gui.cpp include/Json.h
        src/gui/gifs.cpp src/gui/gifs.h
        )


if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${VcpkgRoot}/lib/discord-rpc.lib
            ZLIB::ZLIB OpenSSL::SSL OpenSSL::Crypto ws2_32 wx::net wx::core wx::base)
elseif (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s")
    target_link_libraries(${PROJECT_NAME} wx::net wx::core wx::base)
else(WIN32) #MINGW
    add_definitions("-D_WIN32_WINNT=0x0600")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -s --static")
    target_link_libraries(${PROJECT_NAME} discord-rpc ssl crypto ws2_32 ssp crypt32 z)
endif(WIN32)
target_include_directories(${PROJECT_NAME} PRIVATE "include")