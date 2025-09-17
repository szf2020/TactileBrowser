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
add_compile_options(-Wno-include-next)

# Check for undefined symbols (simplified approach)
add_custom_command(
    TARGET ${COMPONENT_LIB}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/undefined_check
    COMMAND bash -c "${CMAKE_OBJDUMP} -t $<TARGET_FILE:${COMPONENT_LIB}> | grep 'UND' > ${CMAKE_BINARY_DIR}/undefined_check/undefined_symbols.txt || echo 'No undefined symbols'"
    COMMAND bash -c "if [ -s ${CMAKE_BINARY_DIR}/undefined_check/undefined_symbols.txt ]; then echo '❌ Unresolved symbols detected:'; cat ${CMAKE_BINARY_DIR}/undefined_check/undefined_symbols.txt; exit 1; else echo '✅ No unresolved symbols found.'; fi"
    COMMENT "Checking for undefined symbols in ${COMPONENT_LIB}"
)
