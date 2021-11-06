cmake_minimum_required(VERSION 3.19)
project(doom++)

set(CMAKE_CXX_STANDARD 20)

find_package(fmt REQUIRED)
find_package(SDL2 2.0.1 REQUIRED)

include(cmake/CompilerWarnings.cmake)

add_executable(${PROJECT_NAME} main.cpp
        core/game_data.cpp
        game/arena.cpp
        game/arena.hpp
        game/demo_screen.cpp
        game/demo_screen.hpp
        game/mobj.hpp
        game/system.cpp
        grfx/icon.cpp
        grfx/sdl_system.cpp
        grfx/system.cpp
        menu/draw_text.cpp
        menu/system.cpp
        rndr/bsp_renderer.cpp
        rndr/column.cpp
        rndr/system.cpp
        rndr/trigonometry.cpp
        rndr/visplane.cpp
        rndr/visplane.hpp
        core/real.hpp
        game/level.cpp)

set_project_warnings(${PROJECT_NAME})
target_compile_definitions(${PROJECT_NAME} PRIVATE
        ROOT_DIR="${CMAKE_CURRENT_LIST_DIR}"
        PACKAGE_NAME="${PROJECT_NAME}"
        )

target_compile_options(${PROJECT_NAME} PRIVATE -fconcepts-diagnostics-depth=10 -fsanitize=address -fno-omit-frame-pointer)
target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
target_include_directories(${PROJECT_NAME} PRIVATE ./)
target_link_libraries(${PROJECT_NAME} fmt::fmt SDL2::SDL2)