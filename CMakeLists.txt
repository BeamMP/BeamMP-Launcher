cmake_minimum_required(VERSION 3.15)

include_directories(${PROJECT_SOURCE_DIR}/include)

project(BeamNG-MP-Launcher)

set(CMAKE_CXX_STANDARD 14)

add_executable(BeamNG-MP-Launcher main.cpp proxy.cpp Security.cpp http.cpp Discord.cpp)

target_link_libraries(BeamNG-MP-Launcher discord-rpc)