cmake_minimum_required(VERSION 3.8)

project("BouncingBalls")

set(CMAKE_WIN32_EXECUTABLE "True")
set(CMAKE_CXX_FLAGS
        "/Wall /std:c++20 /DUNICODE")

set(SOURCE_FILES
        src/WinMain.h src/WinMain.cpp)

add_executable(BouncingBalls ${SOURCE_FILES})
