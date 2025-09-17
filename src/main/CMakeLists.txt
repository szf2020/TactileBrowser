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

# Create undefined_check directory
add_custom_target(
    create_undefined_check_dir
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/undefined_check
)

# Check for undefined symbols
add_custom_command(
    TARGET ${COMPONENT_LIB}
    POST_BUILD
    DEPENDS create_undefined_check_dir
    COMMAND ${CMAKE_OBJDUMP} -t $<TARGET_FILE:${COMPONENT_LIB}> | grep "UND" > ${CMAKE_BINARY_DIR}/undefined_check/undefined_symbols.txt || true
    COMMAND ${CMAKE_COMMAND} -E echo "Checking for unresolved symbols..."
    COMMAND ${CMAKE_COMMAND} -E file SIZE ${CMAKE_BINARY_DIR}/undefined_check/undefined_symbols.txt UNDEFINED_SYMBOLS_SIZE
    COMMAND ${CMAKE_COMMAND} -E if "${UNDEFINED_SYMBOLS_SIZE}" GREATER 0
        ${CMAKE_COMMAND} -E echo "❌ Unresolved symbols detected:"
        ${CMAKE_COMMAND} -E cat ${CMAKE_BINARY_DIR}/undefined_check/undefined_symbols.txt
        ${CMAKE_COMMAND} -E false
    COMMAND ${CMAKE_COMMAND} -E else
        ${CMAKE_COMMAND} -E echo "No unresolved symbols found."
    COMMAND ${CMAKE_COMMAND} -E endif
    COMMENT "Checking for undefined symbols in ${COMPONENT_LIB}"
)
