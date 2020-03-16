cmake_minimum_required(VERSION 3.15)

project(BeamNG-MP-Launcher)

set(CMAKE_CXX_STANDARD 14)

add_executable(BeamNG-MP-Launcher main.cpp enet.h proxy.cpp Security.cpp)
