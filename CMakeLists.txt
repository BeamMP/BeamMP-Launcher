cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/curl)

project(BeamMP-Launcher)

set(CMAKE_CXX_STANDARD 14)

add_executable(BeamMP-Launcher main.cpp proxy.cpp Security.cpp http.cpp Discord.cpp UpdateCheck.cpp CoreNetwork.cpp)

target_link_libraries(BeamMP-Launcher discord-rpc libcurl_a)