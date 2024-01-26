cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

if (WIN32)
    set(VCPKG_TARGET_TRIPLET x64-windows-static)
endif()

include(cmake/Vcpkg.cmake) # needs to happen before project()

project(
    "BeamMP-Launcher" # replace this
    VERSION 2.0.100
    LANGUAGES CXX
)

message(STATUS "MSVC -> forcing use of statically-linked runtime.")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(cmake/StandardSettings.cmake)
include(cmake/StaticAnalyzers.cmake)
include(cmake/Git.cmake)

add_subdirectory(deps/BeamMP-Protocol)

# below are options which should be changed

### SETTINGS ###

# add all headers (.h, .hpp) to this
set(PRJ_HEADERS 
    src/Http.h
    src/Launcher.h
    src/Platform.h
    src/Identity.h
    src/Version.h
    src/Hashing.h
    src/Compression.h
    src/Config.h
    src/ServerNetwork.h
)
# add all source files (.cpp) to this, except the one with main()
set(PRJ_SOURCES
    src/Launcher.cpp
    src/Launcher.cpp
    src/PlatformWindows.cpp
    src/PlatformLinux.cpp
    src/Identity.cpp
    src/Http.cpp
    src/Version.cpp
    src/Hashing.cpp
    src/Config.cpp
    src/ServerNetwork.cpp
)
# set the source file containing main()
set(PRJ_MAIN src/main.cpp)
# set the source file containing the test's main
set(PRJ_TEST_MAIN tests/test_main.cpp)
# set include paths not part of libraries
set(PRJ_INCLUDE_DIRS src)
# set compile features (e.g. standard version)
set(PRJ_COMPILE_FEATURES cxx_std_20)
# set #defines (test enable/disable not included here)
set(PRJ_DEFINITIONS CPPHTTPLIB_OPENSSL_SUPPORT)
# add all libraries used by the project (WARNING: also set them in vcpkg.json!)
set(PRJ_LIBRARIES 
    fmt::fmt
    doctest::doctest
    Threads::Threads
    spdlog::spdlog
    httplib::httplib
    Boost::system
    Boost::iostreams
    Boost::thread
    Boost::filesystem
    cryptopp::cryptopp
    ZLIB::ZLIB
    OpenSSL::SSL
    OpenSSL::Crypto
    protocol
    zstd::libzstd_static
)

# add dependency find_package calls and similar here
find_package(fmt CONFIG REQUIRED)
find_package(doctest CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system iostreams thread filesystem)
find_package(cryptopp CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(zstd CONFIG REQUIRED)

# to enable multithreading and the Threads::Threads dependency
include(FindThreads)

### END SETTINGS ###

# DONT change anything beyond this point unless you've read the cmake bible and 
# swore on it not to bonk up the ci/cd pipelines with your changes.

####################


# enables compile_commands.json for clang-related tools (such as the clang LS)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# build debug builds by default (if not specified otherwise)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

if(UNIX)
    # this will allow to use same _DEBUG macro available in both Linux as well as Windows - MSCV environment. Easy to put Debug specific code.
    add_compile_options("$<$<CONFIG:DEBUG>:-D_DEBUG>")
endif(UNIX)

include(cmake/CompilerWarnings.cmake)

# set MT library for msvc - this is required (says documentation)
# linux/mac/etc should simply ignore this by default.

set(PRJ_DEFINITIONS ${PRJ_DEFINITIONS}
    PRJ_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    PRJ_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    PRJ_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    PRJ_GIT_HASH="${PRJ_GIT_HASH}"
)

add_executable(${PROJECT_NAME} ${PRJ_HEADERS} ${PRJ_SOURCES} ${PRJ_MAIN})
target_link_libraries(${PROJECT_NAME} ${PRJ_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${PRJ_INCLUDE_DIRS})
target_compile_features(${PROJECT_NAME} PRIVATE ${PRJ_COMPILE_FEATURES})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${PRJ_DEFINITIONS} ${PRJ_WARNINGS}
    DOCTEST_CONFIG_DISABLE # disables all test code in the final executable
)
if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DPLATFORM_WINDOWS=1)
else ()
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DPLATFORM_LINUX=1)
endif ()

# setup all warnings (from cmake/CompilerWarnings.cmake)
set_project_warnings(${PROJECT_NAME})

if(${PROJECT_NAME}_ENABLE_UNIT_TESTING)
    message(STATUS "Unit tests are enabled and will be built as '${PROJECT_NAME}-tests'")
    add_executable(${PROJECT_NAME}-tests ${PRJ_HEADERS} ${PRJ_SOURCES} ${PRJ_TEST_MAIN})
    target_link_libraries(${PROJECT_NAME}-tests ${PRJ_LIBRARIES})
    target_include_directories(${PROJECT_NAME}-tests PRIVATE ${PRJ_INCLUDE_DIRS})
    target_compile_features(${PROJECT_NAME}-tests PRIVATE ${PRJ_COMPILE_FEATURES})
    target_compile_definitions(${PROJECT_NAME}-tests PRIVATE ${PRJ_DEFINITIONS} ${PRJ_WARNINGS})
    set_project_warnings(${PROJECT_NAME}-tests)
    if (WIN32)
        target_compile_definitions(${PROJECT_NAME}-tests PRIVATE -DPLATFORM_WINDOWS=1)
    else ()
        target_compile_definitions(${PROJECT_NAME}-tests PRIVATE -DPLATFORM_LINUX=1)
    endif ()
endif()


