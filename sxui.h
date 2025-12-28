#ifndef SXUI_H
#define SXUI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "dynamic_list.h"

// ============================================================================
// CORE TYPES & ENUMS
// ============================================================================

typedef enum { THEME_DARK, THEME_LIGHT } UIThemeMode;

typedef enum {
    UI_FLAG_NONE          = 0,
    UI_FLAG_HIDDEN        = 1 << 0,
    UI_FLAG_PASSWORD      = 1 << 1,
    UI_FLAG_DRAGGABLE     = 1 << 2,
    UI_FLAG_CLIP          = 1 << 3,
    UI_LAYOUT_HORIZONTAL  = 1 << 4,
    UI_LAYOUT_GRID        = 1 << 5,
    UI_SCROLLABLE         = 1 << 6,
    UI_LAYOUT_VERTICAL    = 1 << 7  // Explicit vertical for clarity
} UIFlags;

typedef enum { 
    UI_BUTTON, 
    UI_LABEL, 
    UI_INPUT, 
    UI_CHECKBOX, 
    UI_FRAME, 
    UI_SLIDER 
} UIType;

// Forward declarations
typedef struct UIElement UIElement;

typedef struct UIConnection {
    int id;
    list* parent_list;
} UIConnection;

// Callback types
typedef void (*ClickCallback)(void* element);
typedef void (*FocusCallback)(void* element, int is_focused);
typedef void (*HoverCallback)(void* element, int is_hovered);
typedef void (*TextCallback)(void* element, const char* text);
typedef void (*ValueCallback)(void* element, float value);

// ============================================================================
// PUBLIC API - INITIALIZATION & CORE
// ============================================================================

void sxui_init(const char* title, int width, int height, Uint32 seed_color);
void sxui_set_theme(Uint32 seed_color, UIThemeMode mode);
void sxui_poll_events(void);
void sxui_render(void);
int sxui_should_quit(void);
void sxui_cleanup(void);

// ============================================================================
// PUBLIC API - WIDGET CREATION
// ============================================================================

UIElement* sxui_frame(UIElement* parent, int x, int y, int w, int h, int flags);
UIElement* sxui_button(UIElement* parent, const char* label, ClickCallback callback);
UIElement* sxui_label(UIElement* parent, const char* text);
UIElement* sxui_input(UIElement* parent, const char* placeholder, int is_password);
UIElement* sxui_checkbox(UIElement* parent, const char* label);
UIElement* sxui_slider(UIElement* parent, float initial_value);

// ============================================================================
// PUBLIC API - ELEMENT MANIPULATION
// ============================================================================

void sxui_set_position(UIElement* el, int x, int y);
void sxui_set_size(UIElement* el, int w, int h);
void sxui_set_visible(UIElement* el, int visible);
void sxui_set_draggable(UIElement* el, int draggable);
void sxui_set_flags(UIElement* el, int flags);
int sxui_get_flags(UIElement* el);

// Text/Value getters and setters
const char* sxui_get_text(UIElement* el);
void sxui_set_text(UIElement* el, const char* text);
float sxui_get_value(UIElement* el); // For sliders/checkboxes
void sxui_set_value(UIElement* el, float value);

// ============================================================================
// PUBLIC API - LAYOUT CONTROL
// ============================================================================

void sxui_frame_set_padding(UIElement* frame, int padding);
void sxui_frame_set_spacing(UIElement* frame, int spacing);
void sxui_frame_set_default_child_size(UIElement* frame, int w, int h);
void sxui_frame_set_grid_columns(UIElement* frame, int max_cols);
void sxui_frame_set_scrollbar_width(UIElement* frame, int width);
void sxui_frame_update_layout(UIElement* frame);

// ============================================================================
// PUBLIC API - EVENT SYSTEM
// ============================================================================

UIConnection sxui_on_click(UIElement* el, ClickCallback callback);
UIConnection sxui_on_hover_enter(UIElement* el, HoverCallback callback);
UIConnection sxui_on_hover_leave(UIElement* el, HoverCallback callback);
UIConnection sxui_on_focus_changed(UIElement* el, FocusCallback callback);
UIConnection sxui_on_text_changed(UIElement* el, TextCallback callback);
UIConnection sxui_on_submit(UIElement* el, TextCallback callback);
UIConnection sxui_on_value_changed(UIElement* el, ValueCallback callback);
void sxui_disconnect(UIConnection conn);

#endif // SXUI_H