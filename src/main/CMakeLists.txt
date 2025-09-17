file(GLOB_RECURSE SOURCE_FILES Source/*.c)

idf_component_register(
    SRCS ${SOURCE_FILES}
    INCLUDE_DIRS "Source" "lexbor/source"
    REQUIRES TactilitySDK esp_http_client
)

set_target_properties(${COMPONENT_LIB} PROPERTIES C_STANDARD 99)

# Add Lexbor
add_subdirectory(lexbor)
target_link_libraries(${COMPONENT_LIB} PRIVATE lexbor_static)

# Suppress #include_next warnings
add_compile_options(-Wno-include-next-warning)

add_custom_command(
    TARGET ${COMPONENT_LIB}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/undefined_check
    COMMAND /bin/sh -c " \
        ${CMAKE_OBJDUMP} -T $<TARGET_FILE:${COMPONENT_LIB}> | grep 'UND' > ${CMAKE_BINARY_DIR}/undefined_check/undefined_symbols.txt || :; \
        if [ -s ${CMAKE_BINARY_DIR}/undefined_check/undefined_symbols.txt ]; then \
            echo '❌ Unresolved symbols detected:'; \
            cat ${CMAKE_BINARY_DIR}/undefined_check/undefined_symbols.txt; \
            exit 1; \
        fi \
    "
)

