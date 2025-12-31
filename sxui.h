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
    UI_LAYOUT_VERTICAL    = 1 << 7
} UIFlags;

typedef enum { 
    UI_BUTTON, 
    UI_LABEL, 
    UI_INPUT, 
    UI_CHECKBOX, 
    UI_FRAME, 
    UI_SLIDER,
    UI_DROPDOWN,
    UI_CANVAS
} UIType;

typedef struct UIElement UIElement;

typedef struct UIConnection {
    int id;
    list* parent_list;
} UIConnection;

typedef struct {
    float position;
    Uint32 color;
} GradientStop;

typedef struct {
    int enabled;
    list* stops;
    float angle;
} UIGradient;

typedef struct {
    int enabled;
    int width;
    Uint32 color;
    Uint8 alpha;
} UIOutline;

typedef struct {
    int enabled;
    int radius;
} UIRoundedCorners;

typedef struct {
    UIGradient gradient;
    UIOutline outline;
    UIRoundedCorners rounded;
} UIEffects;

typedef void (*ClickCallback)(void* element);
typedef void (*FocusCallback)(void* element, int is_focused);
typedef void (*HoverCallback)(void* element, int is_hovered);
typedef void (*TextCallback)(void* element, const char* text);
typedef void (*ValueCallback)(void* element, float value);
typedef void (*DropdownCallback)(void* element, int index, const char* value);
typedef void (*FileDropCallback)(UIElement* element, const char* filepath);

// ============================================================================
// COLOR CONSTANTS
// ============================================================================

#define SX_COLOR_GREEN     0x00FF7AFF
#define SX_COLOR_RED       0xFF5A5AFF
#define SX_COLOR_BLUE      0x5A9FFFFF
#define SX_COLOR_ORANGE    0xFFAA00FF
#define SX_COLOR_PURPLE    0xAA5AFFFF
#define SX_COLOR_MAGENTA   0xFF00AAFF
#define SX_COLOR_YELLOW    0xFFEB3BFF
#define SX_COLOR_CYAN      0x00BCD4FF
#define SX_COLOR_PINK      0xFF4081FF
#define SX_COLOR_TEAL      0x009688FF
#define SX_COLOR_LIME      0xCDDC39FF
#define SX_COLOR_INDIGO    0x3F51B5FF

#define SX_COLOR_WHITE     0xFFFFFFFF
#define SX_COLOR_BLACK     0x000000FF
#define SX_COLOR_GRAY      0x808080FF
#define SX_COLOR_LIGHT     0xDCDCDCFF
#define SX_COLOR_DARK      0x101010FF
#define SX_COLOR_CHARCOAL  0x2A2A2AFF
#define SX_COLOR_SMOKE     0xF5F5F5FF

#define SX_COLOR_NONE      0xFFFFFF00

// ============================================================================
// PUBLIC API - INITIALIZATION & CORE
// ============================================================================

void sxui_init(const char* title, int width, int height, Uint32 seed_color);
void sxui_set_theme(Uint32 seed_color, UIThemeMode mode);
void sxui_poll_events(void);
void sxui_render(void);
int sxui_should_quit(void);
void sxui_cleanup(void);
int sxui_load_font(const char* path, int size);

// ============================================================================
// PUBLIC API - WIDGET CREATION
// ============================================================================

UIElement* sxui_frame(UIElement* parent, int x, int y, int w, int h, int flags);
UIElement* sxui_button(UIElement* parent, const char* label, ClickCallback callback);
UIElement* sxui_label(UIElement* parent, const char* text);
UIElement* sxui_input(UIElement* parent, const char* placeholder, int is_password);
UIElement* sxui_checkbox(UIElement* parent, const char* label);
UIElement* sxui_slider(UIElement* parent, float initial_value);
UIElement* sxui_dropdown(UIElement* parent, const char** options, int option_count, int default_index);
UIElement* sxui_canvas(UIElement* parent, int x, int y, int w, int h);

UIElement* sxui_clone(UIElement* element);
void sxui_delete(UIElement* element);

// ============================================================================
// PUBLIC API - ELEMENT MANIPULATION
// ============================================================================

void sxui_set_position(UIElement* el, int x, int y);
void sxui_set_size(UIElement* el, int w, int h);
void sxui_set_visible(UIElement* el, int visible);
void sxui_set_draggable(UIElement* el, int draggable);
void sxui_set_flags(UIElement* el, int flags);
int sxui_get_flags(UIElement* el);
void sxui_set_z_index(UIElement* el, int z);
int sxui_get_z_index(UIElement* el);
void sxui_set_transparency(UIElement* el, float alpha);
float sxui_get_transparency(UIElement* el);
void sxui_set_custom_color(UIElement* el, Uint32 color);
Uint32 sxui_get_custom_color(UIElement* el);

const char* sxui_get_text(UIElement* el);
void sxui_set_text(UIElement* el, const char* text);
float sxui_get_value(UIElement* el);
void sxui_set_value(UIElement* el, float value);
int sxui_get_dropdown_index(UIElement* el);

// ============================================================================
// PUBLIC API - EFFECTS
// ============================================================================

void sxui_set_gradient(UIElement* el, float* positions, Uint32* colors, int count, float angle);
void sxui_clear_gradient(UIElement* el);
void sxui_set_outline(UIElement* el, int width, Uint32 color, Uint8 alpha);
void sxui_clear_outline(UIElement* el);
void sxui_set_rounded_corners(UIElement* el, int radius);
void sxui_clear_rounded_corners(UIElement* el);

// ============================================================================
// PUBLIC API - CANVAS DRAWING
// ============================================================================

void sxui_canvas_clear(UIElement* canvas, Uint32 color);
void sxui_canvas_draw_pixel(UIElement* canvas, int x, int y, Uint32 color);
void sxui_canvas_draw_line(UIElement* canvas, int x1, int y1, int x2, int y2, Uint32 color);
void sxui_canvas_draw_rect(UIElement* canvas, int x, int y, int w, int h, Uint32 color, int filled);
void sxui_canvas_draw_circle(UIElement* canvas, int cx, int cy, int radius, Uint32 color, int filled);

// ============================================================================
// PUBLIC API - LAYOUT CONTROL
// ============================================================================

void sxui_frame_set_padding(UIElement* frame, int padding);
void sxui_frame_set_spacing(UIElement* frame, int spacing);
void sxui_frame_set_default_child_size(UIElement* frame, int w, int h);
void sxui_frame_set_grid_columns(UIElement* frame, int max_cols);
void sxui_frame_set_scrollbar_width(UIElement* frame, int width);
void sxui_frame_update_layout(UIElement* frame);
// New helpers to fix compilation error
int sxui_frame_get_child_count(UIElement* frame);
UIElement* sxui_frame_get_child(UIElement* frame, int index);
void sxui_frame_add_child(UIElement* frame, UIElement* child);


// ============================================================================
// PUBLIC API - PAGE MANAGER
// ============================================================================

void sxui_page_init(void);
void sxui_add_page(UIElement* frame);
void sxui_add_page_at(UIElement* frame, int position);
void sxui_page_next(void);
void sxui_page_previous(void);
void sxui_switch_page(int index);
int sxui_get_current_page(void);
int sxui_get_page_count(void);

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
UIConnection sxui_on_dropdown_changed(UIElement* el, DropdownCallback callback);
void sxui_disconnect(UIConnection conn);

void sxui_set_file_drop_callback(FileDropCallback callback);

#endif