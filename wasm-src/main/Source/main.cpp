#include <emscripten.h>
#include <emscripten/html5.h>
#include <lvgl.h>
#include <tactilebrowser_core.h>

// Global LVGL context
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
static Renderer renderer_wrapper;
static char current_url[512] = "https://example.com";
static lv_obj_t* content_area = nullptr;

// Platform-specific HTML downloader (browser fetch API via Emscripten)
extern "C" {
EMSCRIPTEN_KEEPALIVE
RenderResult download_html_callback(const char* url, MemoryBuffer* buffer) {
    // For WASM, use emscripten_fetch API (async) or provide a JS binding here.
    // This is a stub -- see glue.js for actual implementation.
    // Populate buffer->data and buffer->size via JS interop.
    return RENDER_ERROR_NETWORK;
}
}

// LVGL Renderer implementation (stub, you need to implement using raw lvgl API for WASM)
static bool lvgl_init(Renderer* renderer) { return true; }
static void lvgl_cleanup(Renderer* renderer) {}
static void* lvgl_create_label(Renderer* renderer, const char* text, int x, int y) {
    lv_obj_t* label = lv_label_create(content_area);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_width(label, SCREEN_WIDTH);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    return label;
}
static void* lvgl_create_button(Renderer* renderer, const char* text, int x, int y) {
    lv_obj_t* btn = lv_btn_create(content_area);
    lv_obj_set_pos(btn, x, y);
    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    return btn;
}
static void* lvgl_create_container(Renderer* renderer, int x, int y, int width, int height) {
    lv_obj_t* container = lv_obj_create(content_area);
    lv_obj_set_pos(container, x, y);
    lv_obj_set_size(container, width, height);
    return container;
}
static void lvgl_set_text_color(Renderer* renderer, void* widget, uint32_t color) {}
static void lvgl_set_bg_color(Renderer* renderer, void* widget, uint32_t color) {}
static void lvgl_set_text_align(Renderer* renderer, void* widget, int align) {}
static void lvgl_clear_container(Renderer* renderer, void* container) {
    lv_obj_clean((lv_obj_t*)container);
}
static int lvgl_get_height(Renderer* renderer, void* widget) {
    return lv_obj_get_height((lv_obj_t*)widget);
}

// Expose load_url to JS
extern "C" {
EMSCRIPTEN_KEEPALIVE
void load_url(const char* url) {
    strncpy(current_url, url, sizeof(current_url) - 1);
    lv_obj_clean(content_area);
    RenderResult result = tactilebrowser_render_url(url, content_area, SCREEN_WIDTH, SCREEN_HEIGHT - 50);
    if (result != RENDER_SUCCESS) {
        // Set status via JS
        EM_ASM({
            window.TactileBrowserWasm.setStatus('Failed to load', '#FF6B6B');
        });
    } else {
        EM_ASM({
            window.TactileBrowserWasm.setStatus('Loaded', '#FFD93D');
        });
    }
}

// Export helper so JS can start the initial navigation after module init.
EMSCRIPTEN_KEEPALIVE
void start_app() {
    // Don't call load_url from C++ startup: let JS trigger it when everything (DOM + glue) is ready.
    // This function is present so JS can call it explicitly if needed.
    EM_ASM({
        window.TactileBrowserWasm.setStatus('Start requested from JS', '#999');
    });
}
}

int main() {
    lv_init();
    // Initialize renderer
    static RenderInterface renderer_iface = {
        lvgl_init, lvgl_cleanup, lvgl_create_label, lvgl_create_button,
        lvgl_create_container, lvgl_set_text_color, lvgl_set_bg_color,
        lvgl_set_text_align, lvgl_clear_container, lvgl_get_height, nullptr
    };
    renderer_wrapper.interface = &renderer_iface;
    renderer_wrapper.platform_data = nullptr;

    tactilebrowser_set_renderer(&renderer_iface);
    tactilebrowser_set_html_downloader(download_html_callback);

    // Set up UI
    content_area = lv_obj_create(lv_screen_active());
    lv_obj_set_size(content_area, SCREEN_WIDTH, SCREEN_HEIGHT - 50);

    // don't automatically call load_url here on startup.
    // load_url(current_url);

    // Main loop
    emscripten_set_main_loop([](){ lv_timer_handler(); }, 0, 1);
    return 0;
}