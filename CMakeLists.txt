cmake_minimum_required(VERSION 3.23)
project(P3TestFolder)

set(CMAKE_CXX_STANDARD 14)

include_directories(libWad)

add_executable(P3TestFolder
        libWad/Wad.cpp
        libWad/Wad.h
        libWad/wad_dump.cpp
        libtest.cpp)

find_package(GTest REQUIRED)
target_link_libraries(P3TestFolder GTest::GTest GTest::Main)