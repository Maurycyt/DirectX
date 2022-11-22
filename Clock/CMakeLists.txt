cmake_minimum_required(VERSION 3.8)

set(PROJECT_NAME "Clock")

project(${PROJECT_NAME})

set(CMAKE_WIN32_EXECUTABLE "True")
set(CMAKE_CXX_FLAGS
        "/Wall /std:c++20 /DUNICODE")

set(SOURCE_FILES
        src/WinMain.h src/WinMain.cpp src/DirectX2DHelper.cpp src/DirectX2DHelper.h)

find_library(DIRECT2D d2d1)
message(${DIRECT2D})

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} ${DIRECT2D})
