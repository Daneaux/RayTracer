cmake_minimum_required(VERSION 3.20)
project(RT2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMMON_SOURCES
    src/main.cpp
    src/Application.cpp
    src/Window.cpp
    src/DXDevice.cpp
    src/SwapChainTarget.cpp
    src/Camera.cpp
    src/InputManager.cpp
    src/Scene.cpp
    src/math/MathTypes.cpp
    src/renderer/FullscreenQuad.cpp
    src/renderer/GPURenderer.cpp
    src/renderer/SoftwareRenderer.cpp
    src/renderer/RenderFactory.cpp
)


add_executable(${PROJECT_NAME} WIN32
    ${COMMON_SOURCES}
    ${RENDERER_SOURCES}
)


target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d11
    dxgi
    d3dcompiler
    dxguid
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
    COMMENT "Copying shader files to output directory"
)
