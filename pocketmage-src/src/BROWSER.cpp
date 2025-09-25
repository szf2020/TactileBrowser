#include <pocketmage.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>
#include <lexbor/dom/interfaces/document.h>
#include <lexbor/dom/interfaces/element.h>

static constexpr const char* TAG = "BROWSER";

#define MAX_TABS 5
#define MAX_URL_LENGTH 512

// Global variables
Tab tabs[MAX_TABS];          // define storage for extern
int tab_count = 0;
int active_tab = 0;
String current_url = "";
String current_line = "";
volatile bool doFull = false;

BROWSERState CurrentBROWSERState = BROWSER_VIEW;

// Frame sources for display
FixedArenaSource<512, 16384> browserLines;
FixedArenaSource<256, 4096> tabLines;
FixedArenaSource<128, 2048> urlLines;

// Frames
Frame browserScreen(FRAME_LEFT, FRAME_RIGHT, FRAME_TOP, FRAME_BOTTOM, &browserLines, false, true);
Frame tabScreen(FRAME_LEFT, FRAME_RIGHT, FRAME_TOP, FRAME_BOTTOM + 150, &tabLines, false, true);
Frame urlScreen(FRAME_LEFT, FRAME_RIGHT, FRAME_TOP + 150, FRAME_BOTTOM, &urlLines, false, false);

Frame *CurrentFrameState = &browserScreen;
std::vector<Frame*> frames = {};

#pragma region helpers
// CURL write callback
static size_t http_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    MemoryBuffer *mem = (MemoryBuffer *)userp;
    
    char *ptr = (char*)realloc(mem->data, mem->size + real_size + 1);
    if (!ptr) {
        Serial.println("Memory reallocation failed");
        return 0;
    }
    
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->data[mem->size] = 0;
    return real_size;
}

// Download HTML content
char* download_html(const String& url) {
    CURL *curl = curl_easy_init();
    MemoryBuffer chunk = {0};
    
    if (!curl) {
        Serial.println("curl_easy_init failed");
        return nullptr;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PocketMageBrowser/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        Serial.println("curl_easy_perform failed: " + String(curl_easy_strerror(res)));
        if (chunk.data) free(chunk.data);
        chunk.data = nullptr;
    }
    
    curl_easy_cleanup(curl);
    return chunk.data;
}

// Extract title from HTML document
String extract_title(lxb_html_document_t *document) {
    if (!document) return "Untitled";

    lxb_dom_element_t *root = lxb_dom_document_element(lxb_dom_interface_document(document));
    if (!root) return "Untitled";

    lxb_dom_collection_t *collection = lxb_dom_collection_make(lxb_dom_interface_document(document), 16);
    if (!collection) return "Untitled";

    lxb_status_t status = lxb_dom_elements_by_tag_name(root, collection, 
                                                       (const lxb_char_t *)"title", 5);
    
    if (status != LXB_STATUS_OK || lxb_dom_collection_length(collection) == 0) {
        lxb_dom_collection_destroy(collection, true);
        return "Untitled";
    }

    lxb_dom_element_t *title_element = lxb_dom_collection_element(collection, 0);
    size_t text_len = 0;
    lxb_char_t *text = lxb_dom_node_text_content(lxb_dom_interface_node(title_element), &text_len);
    
    String result = "Untitled";
    if (text && text_len > 0) {
        result = String((const char*)text).substring(0, min((int)text_len, 50));
    }
    
    if (text) lxb_dom_document_destroy_text(lxb_dom_interface_document(document), text);
    lxb_dom_collection_destroy(collection, true);
    
    return result;
}

// Extract text content from HTML
void extract_content(lxb_html_document_t *document, String &content) {
    content = "";
    if (!document) return;

    lxb_dom_element_t *root = lxb_dom_document_element(lxb_dom_interface_document(document));
    if (!root) return;

    lxb_dom_collection_t *body_collection = lxb_dom_collection_make(lxb_dom_interface_document(document), 4);
    if (!body_collection) return;

    if (lxb_dom_elements_by_tag_name(root, body_collection, (const lxb_char_t *)"body", 4) != LXB_STATUS_OK ||
        lxb_dom_collection_length(body_collection) == 0) {
        lxb_dom_collection_destroy(body_collection, true);
        return;
    }

    lxb_dom_element_t *body = lxb_dom_collection_element(body_collection, 0);
    if (!body) {
        lxb_dom_collection_destroy(body_collection, true);
        return;
    }

    lxb_dom_node_t *node = lxb_dom_interface_node(body)->first_child;
    int line_count = 0;

    while (node && line_count < 100) { // Limit content extraction
        if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            const lxb_tag_id_t tag_id = lxb_dom_element_tag_id((lxb_dom_element_t *)node);
            
            if (tag_id == LXB_TAG_P || tag_id == LXB_TAG_H1 || tag_id == LXB_TAG_H2 || 
                tag_id == LXB_TAG_H3 || tag_id == LXB_TAG_A || tag_id == LXB_TAG_DIV) {
                
                size_t text_len = 0;
                lxb_char_t *text = lxb_dom_node_text_content(node, &text_len);
                
                if (text && text_len > 0) {
                    String line = String((const char*)text).substring(0, min((int)text_len, 200));
                    line.trim();
                    if (line.length() > 0) {
                        content += line + "\n";
                        line_count++;
                    }
                    lxb_dom_document_destroy_text(lxb_dom_interface_document(document), text);
                }
            }
        }
        node = node->next;
    }
    
    lxb_dom_collection_destroy(body_collection, true);
}

// Add line to browser display
void browserAppend(const String& s) {
    browserLines.pushLine(s.c_str(), (uint16_t)s.length(), LF_NONE);
}

// Add line to tab display
void tabAppend(const String& s) {
    tabLines.pushLine(s.c_str(), (uint16_t)s.length(), LF_NONE);
}

// Add line to URL input display
void urlAppend(const String& s) {
    urlLines.pushLine(s.c_str(), (uint16_t)s.length(), LF_NONE);
}

// Clear browser content
void browserClear() { 
    browserLines.clear();
}

void tabClear() { 
    tabLines.clear();
}

void urlClear() { 
    urlLines.clear();
}

// Load URL into specified tab
bool load_url(const String& url, int tab_index) {
    if (url.length() == 0 || tab_index >= MAX_TABS) return false;
    
    // Validate URL format
    if (!url.startsWith("http://") && !url.startsWith("https://")) {
        browserAppend("Error: Invalid URL format. Use http:// or https://");
        return false;
    }

    OLED().oledWord("Loading...");
    
    // Download HTML
    char *html = download_html(url);
    if (!html) {
        browserAppend("Error: Failed to load page. Check connection.");
        OLED().oledWord("Load failed");
        return false;
    }

    // Parse HTML
    lxb_html_document_t *document = lxb_html_document_create();
    if (!document) {
        free(html);
        browserAppend("Error: Failed to create HTML parser");
        return false;
    }

    if (lxb_html_document_parse(document, (const lxb_char_t *)html, strlen(html)) != LXB_STATUS_OK) {
        browserAppend("Error: Failed to parse HTML content");
        lxb_html_document_destroy(document);
        free(html);
        return false;
    }

    // Update tab info
    tabs[tab_index].url = url;
    tabs[tab_index].title = extract_title(document);
    tabs[tab_index].content = "";
    extract_content(document, tabs[tab_index].content);
    tabs[tab_index].loaded = true;

    // Cleanup
    lxb_html_document_destroy(document);
    free(html);
    
    OLED().oledWord("Loaded!");
    return true;
}

// Update browser display with current tab content
void update_browser_display() {
    browserClear();
    
    if (tabs[active_tab].loaded) {
        browserAppend("Title: " + tabs[active_tab].title);
        browserAppend("URL: " + tabs[active_tab].url);
        browserAppend("---");
        
        // Split content into lines and add to display
        String content = tabs[active_tab].content;
        int start = 0;
        while (start < content.length()) {
            int end = content.indexOf('\n', start);
            if (end == -1) end = content.length();
            
            String line = content.substring(start, end);
            if (line.length() > 0) {
                browserAppend(line);
            }
            start = end + 1;
        }
    } else {
        browserAppend("No content loaded");
        browserAppend("Enter URL to browse");
    }
}

// Update tab display
void update_tab_display() {
    tabClear();
    
    for (int i = 0; i < tab_count; i++) {
        String prefix = (i == active_tab) ? "> " : "  ";
        String title = tabs[i].loaded ? tabs[i].title : "Empty Tab";
        if (title.length() > 20) title = title.substring(0, 20) + "...";
        tabAppend(prefix + "Tab " + String(i+1) + ": " + title);
    }
    
    if (tab_count < MAX_TABS) {
        tabAppend("  [+] New Tab");
    }
}

// Update URL input display
void update_url_display() {
    urlClear();
    urlAppend("Enter URL:");
    urlAppend("> " + current_line);
}
#pragma endregion

#pragma region browser eink functions
// Close browser and return to home
void closeBrowser() {
    frames.clear();
    frames.push_back(&browserScreen);
    CurrentBROWSERState = BROWSER_VIEW;
    
    EINK().getDisplay().setFullWindow();
    EINK().getDisplay().fillScreen(GxEPD_WHITE);
    u8g2.clearBuffer();
    
    current_line = "";
    newState = true;
    CurrentKBState = NORMAL;
    disableTimeout = false;
    
    rebootToPocketMage();
}

// Draw browser frame
void drawBrowser() {
    EINK().getDisplay().setFullWindow();
    EINK().setTXTFont(EINK().getCurrentFont());
    
    EINK().getDisplay().firstPage();
    do {
        // Status bar
        switch (CurrentBROWSERState) {
            case BROWSER_VIEW:
                EINK().drawStatusBar("TAB->tabs | FN->URL | ESC->home");
                break;
            case BROWSER_URL_INPUT:
                EINK().drawStatusBar("Enter URL | CR->load | ESC->cancel");
                break;
            case BROWSER_TAB_SELECT:
                EINK().drawStatusBar("Select tab | CR->switch | ESC->back");
                break;
            case BROWSER_HELP:
                EINK().drawStatusBar("Browser Help | ESC->back");
                break;
        }
        
        // Title
        EINK().getDisplay().setCursor(20, 20);
        EINK().getDisplay().print("PocketMage Browser");
        
        // Tab indicator
        EINK().getDisplay().setCursor(200, 20);
        EINK().getDisplay().print("Tab " + String(active_tab + 1) + "/" + String(tab_count));
        
    } while (EINK().getDisplay().nextPage());
}
#pragma endregion

#pragma region input handling
// Handle enter (CR) input
void browserCRInput() {
    current_line.trim();
    
    if (current_line.length() == 0) return;
    
    switch (CurrentBROWSERState) {
        case BROWSER_URL_INPUT:
            // Load URL in current tab
            if (load_url(current_line, active_tab)) {
                update_browser_display();
                CurrentBROWSERState = BROWSER_VIEW;
                CurrentFrameState = &browserScreen;
                newLineAdded = true;
            }
            break;
            
        case BROWSER_TAB_SELECT:
            // Switch to selected tab or create new
            int tab_num = current_line.toInt();
            if (tab_num > 0 && tab_num <= tab_count) {
                active_tab = tab_num - 1;
                update_browser_display();
                CurrentBROWSERState = BROWSER_VIEW;
                CurrentFrameState = &browserScreen;
                newLineAdded = true;
            } else if (tab_num == tab_count + 1 && tab_count < MAX_TABS) {
                // Create new tab
                tabs[tab_count].url = "";
                tabs[tab_count].title = "New Tab";
                tabs[tab_count].content = "";
                tabs[tab_count].loaded = false;
                tab_count++;
                active_tab = tab_count - 1;
                update_browser_display();
                CurrentBROWSERState = BROWSER_VIEW;
                CurrentFrameState = &browserScreen;
                newLineAdded = true;
            }
            break;
    }
    
    current_line = "";
}
#pragma endregion

#pragma region main functions
// Browser initialization
void BROWSER_INIT() {
    Serial.println("Initializing BROWSER!");
    
    CurrentKBState = NORMAL;
    CurrentBROWSERState = BROWSER_VIEW;
    CurrentFrameState = &browserScreen;
    
    frames.clear();
    frames.push_back(&browserScreen);
    
    // Initialize tabs
    for (int i = 0; i < MAX_TABS; i++) {
        tabs[i].url = "";
        tabs[i].title = "Empty Tab";
        tabs[i].content = "";
        tabs[i].loaded = false;
    }
    
    tab_count = 1;
    active_tab = 0;
    current_line = "";
    
    dynamicScroll = 0;
    prev_dynamicScroll = -1;
    lastTouch = -1;
    newState = true;
    doFull = true;
    disableTimeout = true;
    
    EINK().setTXTFont(&FreeMonoBold9pt7b);
    
    // Initialize CURL
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    update_browser_display();
}

// Keyboard handler
void processKB_BROWSER() {
    if (OLEDPowerSave) {
        u8g2.setPowerSave(0);
        OLEDPowerSave = false;
    }
    
    disableTimeout = false;
    int currentMillis = millis();
    
    if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
        char inchar = KB().updateKeypress();
        
        EINK().setTXTFont(EINK().getCurrentFont());
        
        // Handle scroll input
        updateScrollFromTouch_Frame();
        
        // Handle keyboard input
        if (inchar == 0) {
            // No input
        }
        else if (inchar == 8) { // Backspace
            if (current_line.length() > 0) {
                current_line.remove(current_line.length() - 1);
            }
        }
        else if (inchar == 13) { // Enter
            browserCRInput();
        }
        else if (inchar == 20) { // ESC/Clear
            switch (CurrentBROWSERState) {
                case BROWSER_VIEW:
                    closeBrowser();
                    break;
                case BROWSER_URL_INPUT:
                case BROWSER_TAB_SELECT:
                    CurrentBROWSERState = BROWSER_VIEW;
                    CurrentFrameState = &browserScreen;
                    current_line = "";
                    newLineAdded = true;
                    break;
            }
        }
        else if (inchar == 9) { // Tab - switch to tab selection
            if (CurrentBROWSERState == BROWSER_VIEW) {
                CurrentBROWSERState = BROWSER_TAB_SELECT;
                CurrentFrameState = &tabScreen;
                update_tab_display();
                current_line = "";
                newLineAdded = true;
            }
        }
        else if (inchar == 18) { // FN - URL input
            if (CurrentBROWSERState == BROWSER_VIEW) {
                CurrentBROWSERState = BROWSER_URL_INPUT;
                CurrentFrameState = &urlScreen;
                current_line = tabs[active_tab].url;
                update_url_display();
                newLineAdded = true;
            } else {
                if (CurrentKBState == FUNC) CurrentKBState = NORMAL;
                else CurrentKBState = FUNC;
            }
        }
        else if (inchar == 17) { // Shift
            if (CurrentKBState == SHIFT) CurrentKBState = NORMAL;
            else CurrentKBState = SHIFT;
        }
        else if (inchar == 19 || inchar == 12) { // Left arrow - scroll up
            if (dynamicScroll < CurrentFrameState->source->size() - 10) {
                dynamicScroll++;
                newLineAdded = true;
            }
        }
        else if (inchar == 21 || inchar == 6) { // Right arrow - scroll down
            if (dynamicScroll > 0) {
                dynamicScroll--;
                newLineAdded = true;
            }
        }
        else {
            // Add character to current line
            if (CurrentBROWSERState == BROWSER_URL_INPUT || 
                CurrentBROWSERState == BROWSER_TAB_SELECT) {
                current_line += inchar;
                if (CurrentBROWSERState == BROWSER_URL_INPUT) {
                    update_url_display();
                }
            }
            
            if (CurrentKBState == SHIFT) CurrentKBState = NORMAL;
        }
        
        // Update OLED display
        currentMillis = millis();
        if (currentMillis - OLEDFPSMillis >= 16) {
            OLEDFPSMillis = currentMillis;
            
            if (lastTouch == -1) {
                switch (CurrentBROWSERState) {
                    case BROWSER_VIEW:
                        OLED().oledLine("Browser: Tab " + String(active_tab + 1));
                        break;
                    case BROWSER_URL_INPUT:
                        OLED().oledLine("URL: " + current_line);
                        break;
                    case BROWSER_TAB_SELECT:
                        OLED().oledLine("Tab: " + current_line);
                        break;
                }
                
                if (prev_dynamicScroll != dynamicScroll) {
                    prev_dynamicScroll = dynamicScroll;
                }
                updateScroll(CurrentFrameState, prev_dynamicScroll, dynamicScroll);
            } else {
                oledScrollFrame();
            }
        }
        
        KBBounceMillis = currentMillis;
    }
}

// E-ink handler
void einkHandler_BROWSER() {
    if (newLineAdded || newState) {
        if (newState && doFull) {
            EINK().setFastFullRefresh(false);
            drawBrowser();
            einkFramesDynamic(frames, false);
            doFull = false;
        } else if (newLineAdded && !newState) {
            refresh_count++;
            if (refresh_count > REFRESH_MAX_BROWSER) {
                drawBrowser();
                EINK().setFastFullRefresh(false);
                einkFramesDynamic(frames, true);
                refresh_count = 0;
            } else {
                einkFramesDynamic(frames, false);
            }
            EINK().setFastFullRefresh(true);
        } else if (newState && !newLineAdded) {
            drawBrowser();
        }
        
        newState = false;
        newLineAdded = false;
    }
}
#pragma endregion

// Main entry points
void processKB() {
    processKB_BROWSER();
}

void applicationEinkHandler() {
    einkHandler_BROWSER();
}

// Setup function
void setup() {
    PocketMage_INIT();
    BROWSER_INIT();
}

// Main loop
void loop() {
    // Check battery
    pocketmage::power::updateBattState();
    
    // Run keyboard loop
    processKB();
    
    // Yield to watchdog
    vTaskDelay(50 / portTICK_PERIOD_MS);
    yield();
}

// E-ink handler task (migrated from einkFunc.cpp)
void einkHandler(void* parameter) {
    vTaskDelay(pdMS_TO_TICKS(250));
    for (;;) {
        applicationEinkHandler();
        vTaskDelay(pdMS_TO_TICKS(50));
        yield();
    }
}
