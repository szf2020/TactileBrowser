cmake_minimum_required(VERSION 3.20)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

if (DEFINED ENV{TACTILITY_SDK_PATH})
    set(TACTILITY_SDK_PATH $ENV{TACTILITY_SDK_PATH})
else()
    set(TACTILITY_SDK_PATH "../../release/TactilitySDK")
    message(WARNING "⚠️ TACTILITY_SDK_PATH environment variable is not set, defaulting to ${TACTILITY_SDK_PATH}")
endif()

include("${TACTILITY_SDK_PATH}/TactilitySDK.cmake")
set(EXTRA_COMPONENT_DIRS ${TACTILITY_SDK_PATH})

project(TactileBrowser)
tactility_project(TactileBrowser)

# Post-build check for unresolved symbols
add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME}.elf
    POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -T $<TARGET_FILE:${CMAKE_PROJECT_NAME}.elf> | grep "UND" && 
            (echo "❌ Error: Unresolved symbols detected!" && false) || true
    COMMENT "Checking for unresolved symbols..."
)
