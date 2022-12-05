cmake_minimum_required(VERSION 3.8)

set(PROJECT_NAME "AsteroiDoom")

project(${PROJECT_NAME})

set(CMAKE_WIN32_EXECUTABLE "True")
set(CMAKE_CXX_FLAGS
        "/Wall /std:c++20 /DUNICODE /TP /Zc:__cplusplus /EHs")

set(SOURCE_FILES
        src/WinMain.cpp
        src/DirectX2DUtils.cpp
        src/GameController.cpp
        src/collidable/base/CollidableObject.cpp
        src/collidable/specific/Spaceship.cpp
        src/collidable/specific/Asteroid.cpp
        src/collidable/specific/Projectile.cpp
        src/collidable/Arena.cpp
        src/collidable/base/DamagableObject.cpp
        src/collidable/base/DamagingObject.cpp
        src/BitmapUtils.cpp)

find_library(DIRECT2D d2d1)
message(${DIRECT2D})

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} ${DIRECT2D})
