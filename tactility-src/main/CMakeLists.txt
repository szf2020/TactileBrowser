# Gather all source files
file(GLOB_RECURSE SOURCE_FILES Source/*.cpp *.cpp)

# Register component
idf_component_register(
    SRCS ${SOURCE_FILES}
    INCLUDE_DIRS "Source" "../../tactilebrowser_core/include"
    REQUIRES TactilitySDK esp_http_client newlib
)

# Force C standard
set_target_properties(${COMPONENT_LIB} PROPERTIES C_STANDARD 99)

# Add core library
add_subdirectory(../../tactilebrowser_core ${CMAKE_BINARY_DIR}/tactilebrowser_core)

# Link all required libraries with --whole-archive
target_link_libraries(${COMPONENT_LIB} PRIVATE
    -Wl,--whole-archive lexbor_static tactilebrowser_core esp_http_client newlib TactilityC lvgl -Wl,--no-whole-archive
)

# Force inclusion of required symbols for ELF loader
target_link_options(${COMPONENT_LIB} PRIVATE
    -u memcmp
    -u memcpy
    -u lv_textarea_class
    -static
)

# Suppress warnings from #include_next in Lexbor
add_compile_options(-Wno-include-next)
