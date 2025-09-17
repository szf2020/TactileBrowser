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

# Portable post-build check for unresolved symbols
add_custom_command(
    TARGET ${COMPONENT_LIB}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E sh -c "${CMAKE_OBJDUMP} -T $<TARGET_FILE:${COMPONENT_LIB}> | grep 'UND' > ${CMAKE_BINARY_DIR}/undefined_symbols.txt || true"
    COMMAND ${CMAKE_COMMAND} -E sh -c "if [ -s ${CMAKE_BINARY_DIR}/undefined_symbols.txt ]; then echo '❌ Unresolved symbols detected:'; cat ${CMAKE_BINARY_DIR}/undefined_symbols.txt; exit 1; fi"
    COMMENT "Checking for unresolved symbols in component..."
)
