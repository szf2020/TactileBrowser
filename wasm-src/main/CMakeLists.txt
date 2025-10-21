cmake_minimum_required(VERSION 3.15)
project(TactileBrowserWasm C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")

# Emscripten-specific settings
set(CMAKE_EXECUTABLE_SUFFIX ".html")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sUSE_SDL_TTF=2 -sALLOW_MEMORY_GROWTH=1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sUSE_SDL_TTF=2 -sALLOW_MEMORY_GROWTH=1")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 -sASYNCIFY -sEXPORT_ES6=1 -sMODULARIZE=1")

# LVGL, tactilebrowser_core includes
include_directories(
    ${CMAKE_SOURCE_DIR}/../../tactilebrowser_core/include
    ${CMAKE_SOURCE_DIR}/../../tactilebrowser_core/lvgl
    ${CMAKE_SOURCE_DIR}/../../tactilebrowser_core/lvgl/src
)

add_executable(TactileBrowserWasm Sounce/main.cpp)

# Link core library, lvgl, and other needed libs
target_link_libraries(TactileBrowserWasm
    tactilebrowser_core
    lvgl
    m
)

# Emscripten flags for HTML5 main loop and fetch API
set_target_properties(TactileBrowserWasm PROPERTIES LINK_FLAGS "-sUSE_SDL=2 -sASYNCIFY -sALLOW_MEMORY_GROWTH=1 -sEXPORT_ES6=1 -sMODULARIZE=1")
