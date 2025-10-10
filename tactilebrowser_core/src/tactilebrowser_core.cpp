#include "tactilebrowser_core.h"
#include <stdlib.h>
#include <string.h>

// Global state
static RenderInterface* global_renderer = NULL;
static RenderResult (*global_html_downloader)(const char* url, MemoryBuffer* buffer) = NULL;
static Renderer global_renderer_struct = {0};

// Initialize the core library
bool tactilebrowser_core_init(void) {
    if (!html_parser_init()) return false;
    if (!dom_renderer_init()) return false;
    return true;
}

// Cleanup the core library
void tactilebrowser_core_cleanup(void) {
    dom_renderer_cleanup();
    html_parser_cleanup();
}

// Set platform-specific HTML downloader
void tactilebrowser_set_html_downloader(RenderResult (*downloader)(const char* url, MemoryBuffer* buffer)) {
    global_html_downloader = downloader;
    html_parser.download_html = downloader;
}

// Set platform-specific renderer
void tactilebrowser_set_renderer(RenderInterface* renderer) {
    global_renderer = renderer;
    global_renderer_struct.interface = renderer;
}

// Render URL to container
RenderResult tactilebrowser_render_url(const char* url, void* container, int max_width, int max_height) {
    if (!url || !container || !global_renderer) return RENDER_ERROR_UNKNOWN;

    RenderContext context = {
        .renderer = &global_renderer_struct,
        .root_container = container,
        .current_y = 0,
        .max_width = max_width,
        .max_height = max_height
    };

    return render_html_to_container(url, &context);
}

// Utility functions
void memory_buffer_init(MemoryBuffer* buffer) {
    if (buffer) {
        buffer->data = NULL;
        buffer->size = 0;
    }
}

void memory_buffer_free(MemoryBuffer* buffer) {
    if (buffer && buffer->data) {
        free(buffer->data);
        buffer->data = NULL;
        buffer->size = 0;
    }
}

char* safe_strdup(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char* result = (char*)malloc(len);
    if (result) memcpy(result, str, len);
    return result;
}

char* safe_strndup(const char* str, size_t n) {
    if (!str) return NULL;
    size_t len = strlen(str);
    if (n < len) len = n;
    char* result = (char*)malloc(len + 1);
    if (result) {
        memcpy(result, str, len);
        result[len] = '\0';
    }
    return result;
}