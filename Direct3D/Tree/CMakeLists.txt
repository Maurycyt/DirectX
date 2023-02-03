cmake_minimum_required(VERSION 3.8)

set(PROJECT_NAME "Tree")

project(${PROJECT_NAME})

set(CMAKE_WIN32_EXECUTABLE "True")
set(CMAKE_CXX_FLAGS
        "/Wall /std:c++20 /DUNICODE /TP /Zc:__cplusplus /EHs")

set(SOURCE_FILES
        src/WinMain.cpp src/DirectX3DHelper.cpp)

find_library(DIRECT2D d2d1)
find_library(DIRECT3D d3d12)
find_library(DXGI dxgi)
message(${DIRECT2D})
message(${DIRECT3D})
message(${DXGI})

find_program (FXC fxc.exe)
message(${FXC})

add_custom_target(
        HLSL_Shaders ALL
        COMMAND ${FXC} /T vs_5_1 /Vn vs_main /Fh vertex_shader.h VertexShader.hlsl
        COMMAND ${FXC} /T ps_5_1 /Vn ps_main /Fh pixel_shader.h PixelShader.hlsl
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
        VERBATIM
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

add_dependencies(${PROJECT_NAME} HLSL_Shaders)

target_link_libraries(${PROJECT_NAME} ${DIRECT2D} ${DIRECT3D} ${DXGI})
