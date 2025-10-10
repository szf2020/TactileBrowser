#include "dom_renderer.h"
#include "tactilebrowser_core.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Default renderer (placeholder)
static RenderInterface* default_renderer = NULL;

// DOM renderer interface implementation
static RenderResult dom_renderer_render_document(lxb_html_document_t* document, RenderContext* context) {
    if (!document || !context || !context->renderer) return RENDER_ERROR_UNKNOWN;

    lxb_dom_element_t* body = html_parser.find_body_element(document);
    if (!body) return RENDER_ERROR_PARSE;

    // Clear container
    if (context->renderer->interface->clear_container) {
        context->renderer->interface->clear_container(context->renderer, context->root_container);
    }

    // Reset Y position
    context->current_y = 10;

    // Render body content
    dom_renderer.render_node(lxb_dom_interface_node(body), context);

    return RENDER_SUCCESS;
}

static void dom_renderer_render_node(lxb_dom_node_t* node, RenderContext* context) {
    if (!node || !context) return;

    lxb_dom_node_type_t node_type = node->type;

    if (node_type == LXB_DOM_NODE_TYPE_TEXT) {
        size_t len;
        const char* txt = html_parser.get_element_text((lxb_dom_element_t*)node, &len);

        if (txt && len > 0) {
            // Trim whitespace
            const char* start = txt;
            const char* end = txt + len - 1;

            while (start <= end && isspace((unsigned char)*start)) start++;
            while (end >= start && isspace((unsigned char)*end)) end--;

            if (start <= end) {
                size_t trimmed_len = end - start + 1;
                char* text = safe_strndup(start, trimmed_len);
                if (text) {
                    void* label = dom_renderer.create_element_widget(ELEMENT_PARAGRAPH, context, text);
                    if (label && context->renderer->interface->get_height) {
                        context->current_y += context->renderer->interface->get_height(context->renderer, label) + 5;
                    }
                    free(text);
                }
            }
        }
    }
    else if (node_type == LXB_DOM_NODE_TYPE_ELEMENT) {
        lxb_dom_element_t* el = (lxb_dom_element_t*)node;

        size_t tag_len;
        const char* tag_name = html_parser.get_element_tag(el, &tag_len);
        ElementType elem_type = get_element_type(tag_name, tag_len);

        // Get text content
        size_t text_len;
        const char* text_content = html_parser.get_element_text(el, &text_len);

        // Create widget
        void* widget = dom_renderer.create_element_widget(elem_type, context, text_content ? (const char*)text_content : "");

        if (widget) {
            // Apply inline styles
            size_t style_len;
            const char* style_attr = html_parser.get_element_attr(el, "style", &style_len);

            if (style_attr && style_len > 0) {
                char* style = safe_strndup(style_attr, style_len);
                if (style) {
                    dom_renderer.apply_styles(widget, context, style);
                    free(style);
                }
            }

            // Handle special elements
            if (elem_type == ELEMENT_BUTTON && text_content) {
                // Button text is already set in create_element_widget
            }

            // Update Y position
            if (context->renderer->interface->get_height) {
                context->current_y += context->renderer->interface->get_height(context->renderer, widget) + 5;
            }

            // Render children
            lxb_dom_node_t* child = html_parser.get_first_child(node);
            while (child) {
                dom_renderer.render_node(child, context);
                child = html_parser.get_next_sibling(child);
            }
        }
    }
}

static void* dom_renderer_create_element_widget(ElementType type, RenderContext* context, const char* text) {
    if (!context || !context->renderer) return NULL;

    void* widget = NULL;

    switch (type) {
        case ELEMENT_HEADING1:
        case ELEMENT_HEADING2:
        case ELEMENT_HEADING3:
        case ELEMENT_PARAGRAPH:
        case ELEMENT_LINK:
        case ELEMENT_SPAN:
            if (context->renderer->interface->create_label) {
                widget = context->renderer->interface->create_label(context->renderer, text, 0, context->current_y);
                // Apply heading colors
                if (type == ELEMENT_HEADING1 || type == ELEMENT_HEADING2 || type == ELEMENT_HEADING3) {
                    if (context->renderer->interface->set_text_color) {
                        context->renderer->interface->set_text_color(context->renderer, widget, 0x000080);
                    }
                } else if (type == ELEMENT_LINK) {
                    if (context->renderer->interface->set_text_color) {
                        context->renderer->interface->set_text_color(context->renderer, widget, 0x0000EE);
                    }
                }
            }
            break;

        case ELEMENT_BUTTON:
            if (context->renderer->interface->create_button) {
                widget = context->renderer->interface->create_button(context->renderer, text, 0, context->current_y);
            }
            break;

        case ELEMENT_DIV:
            if (context->renderer->interface->create_container) {
                widget = context->renderer->interface->create_container(context->renderer, 0, context->current_y,
                                                           context->max_width, 50); // Default height
            }
            break;

        default:
            // Default to label
            if (context->renderer->interface->create_label) {
                widget = context->renderer->interface->create_label(context->renderer, text, 0, context->current_y);
            }
            break;
    }

    return widget;
}

static void dom_renderer_apply_styles(void* widget, RenderContext* context, const char* style) {
    parse_inline_style(style, context, widget);
}

// Global DOM renderer instance
DomRendererInterface dom_renderer = {
    .render_document = dom_renderer_render_document,
    .render_node = dom_renderer_render_node,
    .create_element_widget = dom_renderer_create_element_widget,
    .apply_styles = dom_renderer_apply_styles
};

// Initialize DOM renderer
bool dom_renderer_init(void) {
    return true;
}

// Cleanup DOM renderer
void dom_renderer_cleanup(void) {
    // Cleanup if needed
}

// Main rendering function
RenderResult render_html_to_container(const char* url, RenderContext* context) {
    if (!url || !context) return RENDER_ERROR_UNKNOWN;

    // Download HTML
    MemoryBuffer buffer = {0};
    RenderResult download_result = html_parser.download_html(url, &buffer);
    if (download_result != RENDER_SUCCESS) {
        return download_result;
    }

    if (!buffer.data || buffer.size == 0) {
        return RENDER_ERROR_NETWORK;
    }

    // Parse HTML
    lxb_html_document_t* document = html_parser.parse_html(buffer.data, buffer.size);
    if (!document) {
        free(buffer.data);
        return RENDER_ERROR_PARSE;
    }

    // Render document
    RenderResult render_result = dom_renderer.render_document(document, context);

    // Cleanup
    lxb_html_document_destroy(document);
    free(buffer.data);

    return render_result;
}