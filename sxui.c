#include "sxui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "dynamic_list.h"

#define INPUT_MAX 256
#define SCROLL_FADE_MS 1500
#define DROPDOWN_Z_INDEX 10000

typedef struct {
    Uint32 primary;
    Uint32 on_primary;
    Uint32 secondary;
    Uint32 on_secondary;
    Uint32 tertiary;
    Uint32 on_tertiary;
    Uint32 background;
    Uint32 on_background;
    Uint32 surface;
    Uint32 on_surface;
    Uint32 primary_container;
    Uint32 on_primary_container;
    Uint32 outline;
    Uint32 seed;
    UIThemeMode mode;
} UITheme;

typedef struct {
    int id;
    void* callback;
} BoundCallback;

struct UIElement {
    int x, y, w, h;
    int target_w, target_h;
    UIType type;
    int flags;
    UIElement* parent;
    list* children;
    int _is_hovered;
    int _is_hovered_prev;
    int _is_dragging;
    int z_index;
    Uint32 creation_time;
    float transparency;
    Uint32 custom_color;
    int has_custom_color;
    UIEffects effects;
    
    list* onMouseEnter;
    list* onMouseLeave;
};

typedef struct {
    UIElement el;
    char* text;
    list* onClick;
    int _pressed;
    Uint32 _lastClickTime;
} UIButton;

typedef struct {
    UIElement el;
    char* text;
} UILabel;

typedef struct {
    UIElement el;
    char text[INPUT_MAX];
    char placeholder[INPUT_MAX];
    size_t len;
    int scrollOffset;
    int cursorPosition;
    int selectionAnchor;
    list* onFocusChanged;
    list* onTextChanged;
    list* onSubmit;
} UITextInput;

typedef struct {
    UIElement el;
    int value;
    char* text;
    list* onValueChanged;
} UICheckBox;

typedef struct {
    UIElement el;
    float value;
    list* onValueChanged;
} UISlider;

typedef struct {
    UIElement el;
    char** options;
    int option_count;
    int selected_index;
    int is_open;
    list* onSelectionChanged;
} UIDropdown;

typedef struct {
    UIElement el;
    SDL_Texture* texture;
    Uint32* pixels;
    int pitch;
} UICanvas;

typedef struct {
    UIElement el;
    int padding, spacing;
    int scroll_y, content_height;
    Uint32 last_scroll_time;
    int max_grid_cols;
    int scroll_bar_width;
} UIFrame;

typedef struct {
    list* pages;
    int current_page;
    int initialized;
} PageManager;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    list* root;
    UITheme theme;
    UIElement* focused;
    UIElement* dragging_el;
    int drag_off_x, drag_off_y;
    int running;
    Uint32 last_frame_time;
    TTF_Font* default_font;
    TTF_Font* custom_font;
    PageManager page_manager;
    FileDropCallback file_drop_callback;
} SXUI_Engine;

static SXUI_Engine engine;
static int GLOBAL_CONN_ID = 0;

TTF_Font* _get_active_font() {
    if (engine.custom_font) return engine.custom_font;
    return engine.default_font;
}

Uint32 rgba_to_uint(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    return (r << 24) | (g << 16) | (b << 8) | a;
}

void uint_to_rgba(Uint32 c, Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a) {
    *r = (c >> 24) & 0xFF;
    *g = (c >> 16) & 0xFF;
    *b = (c >> 8) & 0xFF;
    *a = c & 0xFF;
}

Uint32 shift_color(Uint32 c, float factor) {
    Uint8 r, g, b, a;
    uint_to_rgba(c, &r, &g, &b, &a);
    return rgba_to_uint(
        (Uint8)(r * factor > 255 ? 255 : r * factor),
        (Uint8)(g * factor > 255 ? 255 : g * factor),
        (Uint8)(b * factor > 255 ? 255 : b * factor),
        a
    );
}

float calculate_luminance(Uint32 color) {
    Uint8 r, g, b, a;
    uint_to_rgba(color, &r, &g, &b, &a);
    float rf = r / 255.0f;
    float gf = g / 255.0f;
    float bf = b / 255.0f;
    
    rf = (rf <= 0.03928f) ? rf / 12.92f : powf((rf + 0.055f) / 1.055f, 2.4f);
    gf = (gf <= 0.03928f) ? gf / 12.92f : powf((gf + 0.055f) / 1.055f, 2.4f);
    bf = (bf <= 0.03928f) ? bf / 12.92f : powf((bf + 0.055f) / 1.055f, 2.4f);
    
    return 0.2126f * rf + 0.7152f * gf + 0.0722f * bf;
}

Uint32 get_on_color(Uint32 background) {
    float lum = calculate_luminance(background);
    return (lum > 0.5f) ? SX_COLOR_BLACK : SX_COLOR_WHITE;
}

Uint32 blend_colors(Uint32 base, Uint32 overlay, float overlay_alpha) {
    Uint8 br, bg, bb, ba, or, og, ob, oa;
    uint_to_rgba(base, &br, &bg, &bb, &ba);
    uint_to_rgba(overlay, &or, &og, &ob, &oa);
    
    float alpha = overlay_alpha * (oa / 255.0f);
    Uint8 r = (Uint8)(br * (1.0f - alpha) + or * alpha);
    Uint8 g = (Uint8)(bg * (1.0f - alpha) + og * alpha);
    Uint8 b = (Uint8)(bb * (1.0f - alpha) + ob * alpha);
    
    return rgba_to_uint(r, g, b, ba);
}

UITheme sx_generate_palette(Uint32 seed, UIThemeMode mode) {
    UITheme t;
    t.seed = seed;
    t.mode = mode;
    t.primary = seed;
    t.on_primary = get_on_color(seed);
    
    Uint8 r, g, b, a;
    uint_to_rgba(seed, &r, &g, &b, &a);
    
    t.secondary = rgba_to_uint(
        (Uint8)((r * 0.7f + 50) > 255 ? 255 : (r * 0.7f + 50)),
        (Uint8)((g * 0.7f + 50) > 255 ? 255 : (g * 0.7f + 50)),
        (Uint8)((b * 0.7f + 50) > 255 ? 255 : (b * 0.7f + 50)),
        255
    );
    t.on_secondary = get_on_color(t.secondary);
    
    t.tertiary = rgba_to_uint(
        (Uint8)((r * 0.5f + b * 0.3f) > 255 ? 255 : (r * 0.5f + b * 0.3f)),
        (Uint8)((g * 0.8f) > 255 ? 255 : (g * 0.8f)),
        (Uint8)((b * 0.5f + r * 0.3f) > 255 ? 255 : (b * 0.5f + r * 0.3f)),
        255
    );
    t.on_tertiary = get_on_color(t.tertiary);
    
    if (mode == THEME_DARK) {
        t.background = rgba_to_uint(15, 15, 18, 255);
        t.on_background = rgba_to_uint(230, 230, 235, 255);
        t.surface = rgba_to_uint(30, 30, 35, 255);
        t.on_surface = rgba_to_uint(230, 230, 235, 255);
        t.primary_container = blend_colors(t.surface, t.primary, 0.3f);
        t.on_primary_container = get_on_color(t.primary_container);
        t.outline = rgba_to_uint(70, 70, 75, 255);
    } else {
        t.background = rgba_to_uint(250, 250, 252, 255);
        t.on_background = rgba_to_uint(26, 26, 30, 255);
        t.surface = rgba_to_uint(255, 255, 255, 255);
        t.on_surface = rgba_to_uint(26, 26, 30, 255);
        t.primary_container = blend_colors(t.surface, t.primary, 0.15f);
        t.on_primary_container = get_on_color(t.primary_container);
        t.outline = rgba_to_uint(120, 120, 125, 255);
    }
    
    return t;
}

void _draw_rect(int x, int y, int w, int h, Uint32 c) {
    Uint8 r, g, b, a;
    uint_to_rgba(c, &r, &g, &b, &a);
    SDL_SetRenderDrawBlendMode(engine.renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(engine.renderer, r, g, b, a);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(engine.renderer, &rect);
}

void _draw_rect_outline(int x, int y, int w, int h, Uint32 c) {
    Uint8 r, g, b, a;
    uint_to_rgba(c, &r, &g, &b, &a);
    SDL_SetRenderDrawBlendMode(engine.renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(engine.renderer, r, g, b, a);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderDrawRect(engine.renderer, &rect);
}

void _draw_rounded_rect(int x, int y, int w, int h, int radius, Uint32 c) {
    int r = radius < (w / 2) ? radius : (w / 2);
    r = r < (h / 2) ? r : (h / 2);
    
    _draw_rect(x + r, y, w - 2 * r, h, c);
    _draw_rect(x, y + r, r, h - 2 * r, c);
    _draw_rect(x + w - r, y + r, r, h - 2 * r, c);
    
    Uint8 red, green, blue, alpha;
    uint_to_rgba(c, &red, &green, &blue, &alpha);
    SDL_SetRenderDrawColor(engine.renderer, red, green, blue, alpha);
    
    for (int dy = 0; dy < r; dy++) {
        for (int dx = 0; dx < r; dx++) {
            if (dx * dx + dy * dy <= r * r) {
                SDL_RenderDrawPoint(engine.renderer, x + r - dx, y + r - dy);
                SDL_RenderDrawPoint(engine.renderer, x + w - r + dx - 1, y + r - dy);
                SDL_RenderDrawPoint(engine.renderer, x + r - dx, y + h - r + dy - 1);
                SDL_RenderDrawPoint(engine.renderer, x + w - r + dx - 1, y + h - r + dy - 1);
            }
        }
    }
}

void _draw_gradient_rect(int x, int y, int w, int h, UIGradient* grad) {
    if (!grad->enabled || !grad->stops || list_length(grad->stops) < 2) return;
    
    GradientStop* first = (GradientStop*)list_get(grad->stops, 0);
    // REMOVED unused variable: last
    
    float angle_rad = grad->angle * 3.14159265f / 180.0f;
    // REMOVED unused variables: vertical, horizontal
    
    if (fabs(sinf(angle_rad)) > fabs(cosf(angle_rad))) {
        for (int py = 0; py < h; py++) {
            float t = (float)py / h;
            Uint32 color = first->color;
            
            for (size_t i = 0; i < list_length(grad->stops) - 1; i++) {
                GradientStop* s1 = (GradientStop*)list_get(grad->stops, i);
                GradientStop* s2 = (GradientStop*)list_get(grad->stops, i + 1);
                
                if (t >= s1->position && t <= s2->position) {
                    float local_t = (t - s1->position) / (s2->position - s1->position);
                    Uint8 r1, g1, b1, a1, r2, g2, b2, a2;
                    uint_to_rgba(s1->color, &r1, &g1, &b1, &a1);
                    uint_to_rgba(s2->color, &r2, &g2, &b2, &a2);
                    
                    color = rgba_to_uint(
                        (Uint8)(r1 + (r2 - r1) * local_t),
                        (Uint8)(g1 + (g2 - g1) * local_t),
                        (Uint8)(b1 + (b2 - b1) * local_t),
                        (Uint8)(a1 + (a2 - a1) * local_t)
                    );
                    break;
                }
            }
            
            _draw_rect(x, y + py, w, 1, color);
        }
    } else {
        for (int px = 0; px < w; px++) {
            float t = (float)px / w;
            Uint32 color = first->color;
            
            for (size_t i = 0; i < list_length(grad->stops) - 1; i++) {
                GradientStop* s1 = (GradientStop*)list_get(grad->stops, i);
                GradientStop* s2 = (GradientStop*)list_get(grad->stops, i + 1);
                
                if (t >= s1->position && t <= s2->position) {
                    float local_t = (t - s1->position) / (s2->position - s1->position);
                    Uint8 r1, g1, b1, a1, r2, g2, b2, a2;
                    uint_to_rgba(s1->color, &r1, &g1, &b1, &a1);
                    uint_to_rgba(s2->color, &r2, &g2, &b2, &a2);
                    
                    color = rgba_to_uint(
                        (Uint8)(r1 + (r2 - r1) * local_t),
                        (Uint8)(g1 + (g2 - g1) * local_t),
                        (Uint8)(b1 + (b2 - b1) * local_t),
                        (Uint8)(a1 + (a2 - a1) * local_t)
                    );
                    break;
                }
            }
            
            _draw_rect(x + px, y, 1, h, color);
        }
    }
}

int _measure_text(const char* text) {
    int w = 0;
    if (text && *text && TTF_SizeText(_get_active_font(), text, &w, NULL) != 0) return 0;
    return w;
}

int _measure_text_len(const char* text, int len) {
    char temp[INPUT_MAX];
    if (len >= INPUT_MAX) len = INPUT_MAX - 1;
    strncpy(temp, text, len);
    temp[len] = '\0';
    return _measure_text(temp);
}

void _draw_text(const char* text, int x, int y, Uint32 c, int center, int pass) {
    if (!text || !*text) return;
    
    char buffer[INPUT_MAX];
    if (pass) {
        int i = 0;
        for (; text[i] && i < INPUT_MAX - 1; i++) buffer[i] = '*';
        buffer[i] = '\0';
        text = buffer;
    }
    
    Uint8 r, g, b, a;
    uint_to_rgba(c, &r, &g, &b, &a);
    SDL_Color color = {r, g, b, a};
    SDL_Surface* s = TTF_RenderText_Blended(_get_active_font(), text, color);
    if (!s) return;
    
    SDL_Texture* t = SDL_CreateTextureFromSurface(engine.renderer, s);
    SDL_Rect dst = {x - (center ? s->w/2 : 0), y - s->h/2, s->w, s->h};
    SDL_RenderCopy(engine.renderer, t, NULL, &dst);
    SDL_DestroyTexture(t);
    SDL_FreeSurface(s);
}

UIConnection bind_event(list* handler_list, void* callback) {
    BoundCallback* bc = malloc(sizeof(BoundCallback));
    bc->id = ++GLOBAL_CONN_ID;
    bc->callback = callback;
    list_add(handler_list, bc);
    
    UIConnection conn;
    conn.id = bc->id;
    conn.parent_list = handler_list;
    return conn;
}

void disconnect_binding(UIConnection conn) {
    if (!conn.parent_list) return;
    for (size_t i = 0; i < list_length(conn.parent_list); i++) {
        BoundCallback* bc = (BoundCallback*)list_get(conn.parent_list, i);
        if (bc->id == conn.id) {
            list_remove_at(conn.parent_list, i);
            free(bc);
            return;
        }
    }
}

void trigger_click(UIButton* btn) {
    for (size_t i = 0; i < list_length(btn->onClick); i++) {
        BoundCallback* bc = (BoundCallback*)list_get(btn->onClick, i);
        ((ClickCallback)bc->callback)(btn);
    }
}

void trigger_focus(UITextInput* input, int focused) {
    for (size_t i = 0; i < list_length(input->onFocusChanged); i++) {
        BoundCallback* bc = (BoundCallback*)list_get(input->onFocusChanged, i);
        ((FocusCallback)bc->callback)(input, focused);
    }
}

void trigger_hover(UIElement* elem, int hovered) {
    list* l = hovered ? elem->onMouseEnter : elem->onMouseLeave;
    for (size_t i = 0; i < list_length(l); i++) {
        BoundCallback* bc = (BoundCallback*)list_get(l, i);
        ((HoverCallback)bc->callback)(elem, hovered);
    }
}

void trigger_text_changed(UITextInput* input) {
    for (size_t i = 0; i < list_length(input->onTextChanged); i++) {
        BoundCallback* bc = (BoundCallback*)list_get(input->onTextChanged, i);
        ((TextCallback)bc->callback)(input, input->text);
    }
}

void trigger_submit(UITextInput* input) {
    for (size_t i = 0; i < list_length(input->onSubmit); i++) {
        BoundCallback* bc = (BoundCallback*)list_get(input->onSubmit, i);
        ((TextCallback)bc->callback)(input, input->text);
    }
}

void trigger_value_changed(void* element, float value) {
    list* handlers = NULL;
    if (((UIElement*)element)->type == UI_SLIDER) {
        handlers = ((UISlider*)element)->onValueChanged;
    } else if (((UIElement*)element)->type == UI_CHECKBOX) {
        handlers = ((UICheckBox*)element)->onValueChanged;
    }
    
    if (handlers) {
        for (size_t i = 0; i < list_length(handlers); i++) {
            BoundCallback* bc = (BoundCallback*)list_get(handlers, i);
            ((ValueCallback)bc->callback)(element, value);
        }
    }
}

void trigger_dropdown_changed(UIDropdown* dd, int index, const char* value) {
    for (size_t i = 0; i < list_length(dd->onSelectionChanged); i++) {
        BoundCallback* bc = (BoundCallback*)list_get(dd->onSelectionChanged, i);
        ((DropdownCallback)bc->callback)(dd, index, value);
    }
}

int clamp(int val, int min, int max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

int is_delimiter(char c) {
    return !isalnum(c);
}

int find_next_word_boundary(const char* text, int current_pos, int len) {
    int i = current_pos;
    if (i >= len) return len;
    while (i < len && is_delimiter(text[i])) i++;
    while (i < len && !is_delimiter(text[i])) i++;
    return i;
}

int find_prev_word_boundary(const char* text, int current_pos) {
    int i = current_pos;
    if (i <= 0) return 0;
    i--;
    while (i > 0 && is_delimiter(text[i])) i--;
    while (i > 0 && !is_delimiter(text[i-1])) i--;
    return i;
}

void delete_selection(UITextInput* input) {
    if (input->cursorPosition == input->selectionAnchor) return;

    int start = (input->cursorPosition < input->selectionAnchor) ? 
                input->cursorPosition : input->selectionAnchor;
    int end = (input->cursorPosition > input->selectionAnchor) ? 
              input->cursorPosition : input->selectionAnchor;
    int diff = end - start;

    memmove(&input->text[start], &input->text[end], input->len - end + 1);
    input->len -= diff;
    input->cursorPosition = start;
    input->selectionAnchor = start;
    trigger_text_changed(input);
}

void read_input(UITextInput* input, SDL_Event* event) {
    if (event->type == SDL_TEXTINPUT) {
        delete_selection(input);
        size_t add_len = strlen(event->text.text);
        if (input->len + add_len < INPUT_MAX - 1) {
            memmove(&input->text[input->cursorPosition + add_len], 
                    &input->text[input->cursorPosition], 
                    input->len - input->cursorPosition + 1);
            memcpy(&input->text[input->cursorPosition], event->text.text, add_len);
            input->len += add_len;
            input->cursorPosition += add_len;
            input->selectionAnchor = input->cursorPosition;
            trigger_text_changed(input);
        }
    } 
    else if (event->type == SDL_KEYDOWN) {
        SDL_Keycode key = event->key.keysym.sym;
        Uint16 mod = event->key.keysym.mod;
        int is_shift = (mod & KMOD_SHIFT);
        int is_ctrl = (mod & KMOD_CTRL);

        if (key == SDLK_BACKSPACE) {
            if (input->cursorPosition != input->selectionAnchor) {
                delete_selection(input);
            } else if (input->cursorPosition > 0) {
                if (is_ctrl) {
                    int target = find_prev_word_boundary(input->text, input->cursorPosition);
                    input->selectionAnchor = target;
                    delete_selection(input);
                } else {
                    memmove(&input->text[input->cursorPosition - 1], 
                            &input->text[input->cursorPosition], 
                            input->len - input->cursorPosition + 1);
                    input->len--;
                    input->cursorPosition--;
                    input->selectionAnchor = input->cursorPosition;
                    trigger_text_changed(input);
                }
            }
        } 
        else if (key == SDLK_DELETE) {
            if (input->cursorPosition != input->selectionAnchor) {
                delete_selection(input);
            } else if (input->cursorPosition < (int)input->len) {
                if (is_ctrl) {
                    int target = find_next_word_boundary(input->text, input->cursorPosition, input->len);
                    input->selectionAnchor = target;
                    delete_selection(input);
                } else {
                    memmove(&input->text[input->cursorPosition], 
                            &input->text[input->cursorPosition + 1], 
                            input->len - input->cursorPosition);
                    input->len--;
                    input->selectionAnchor = input->cursorPosition;
                    trigger_text_changed(input);
                }
            }
        }
        else if (key == SDLK_LEFT) {
            int target = input->cursorPosition;
            if (is_ctrl) target = find_prev_word_boundary(input->text, target);
            else target--;
            input->cursorPosition = clamp(target, 0, input->len);
            if (!is_shift) input->selectionAnchor = input->cursorPosition;
        }
        else if (key == SDLK_RIGHT) {
            int target = input->cursorPosition;
            if (is_ctrl) target = find_next_word_boundary(input->text, target, input->len);
            else target++;
            input->cursorPosition = clamp(target, 0, input->len);
            if (!is_shift) input->selectionAnchor = input->cursorPosition;
        }
        else if (key == SDLK_HOME) {
            input->cursorPosition = 0;
            if (!is_shift) input->selectionAnchor = 0;
        }
        else if (key == SDLK_END) {
            input->cursorPosition = input->len;
            if (!is_shift) input->selectionAnchor = input->len;
        }
        else if (key == SDLK_a && is_ctrl) {
            input->selectionAnchor = 0;
            input->cursorPosition = input->len;
        }
        else if (key == SDLK_c && is_ctrl) {
            if (input->cursorPosition != input->selectionAnchor) {
                int start = (input->cursorPosition < input->selectionAnchor) ? 
                            input->cursorPosition : input->selectionAnchor;
                int end = (input->cursorPosition > input->selectionAnchor) ? 
                          input->cursorPosition : input->selectionAnchor;
                char buf[INPUT_MAX];
                int n = end - start;
                strncpy(buf, &input->text[start], n);
                buf[n] = '\0';
                SDL_SetClipboardText(buf);
            }
        } 
        else if (key == SDLK_x && is_ctrl) {
            if (input->cursorPosition != input->selectionAnchor) {
                int start = (input->cursorPosition < input->selectionAnchor) ? 
                            input->cursorPosition : input->selectionAnchor;
                int end = (input->cursorPosition > input->selectionAnchor) ? 
                          input->cursorPosition : input->selectionAnchor;
                char buf[INPUT_MAX];
                int n = end - start;
                strncpy(buf, &input->text[start], n);
                buf[n] = '\0';
                SDL_SetClipboardText(buf);
                delete_selection(input);
            }
        }
        else if (key == SDLK_v && is_ctrl) {
            if (SDL_HasClipboardText()) {
                char* clip = SDL_GetClipboardText();
                if (clip) {
                    delete_selection(input);
                    int n = strlen(clip);
                    if (input->len + n < INPUT_MAX - 1) {
                        memmove(&input->text[input->cursorPosition + n], 
                                &input->text[input->cursorPosition], 
                                input->len - input->cursorPosition + 1);
                        memcpy(&input->text[input->cursorPosition], clip, n);
                        input->len += n;
                        input->cursorPosition += n;
                        input->selectionAnchor = input->cursorPosition;
                        trigger_text_changed(input);
                    }
                    SDL_free(clip);
                }
            }
        }
        else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            trigger_submit(input);
            trigger_focus(input, 0);
            engine.focused = NULL;
            SDL_StopTextInput();
        }
    }

    int cw = input->el.w - 10;
    int is_pass = (input->el.flags & UI_FLAG_PASSWORD);
    
    int cx;
    if (is_pass) {
        cx = input->cursorPosition * _measure_text("*");
    } else {
        cx = _measure_text_len(input->text, input->cursorPosition);
    }
    
    int rel_cx = cx - input->scrollOffset;

    if (rel_cx < 0) {
        input->scrollOffset = cx;
    } else if (rel_cx > cw) {
        input->scrollOffset = cx - cw;
    }
    
    int total_w;
    if (is_pass) {
        total_w = input->len * _measure_text("*");
    } else {
        total_w = _measure_text(input->text);
    }
    
    if (total_w < cw) input->scrollOffset = 0;
    else if (input->scrollOffset > total_w - cw) input->scrollOffset = total_w - cw;
    if (input->scrollOffset < 0) input->scrollOffset = 0;
}

void sx_update_layout(UIFrame* f) {
    int cx = f->padding, cy = f->padding;
    int max_row_h = 0;
    int col_count = 0;

    for (size_t i = 0; i < list_length(f->el.children); i++) {
        UIElement* c = list_get(f->el.children, i);
        if (c->flags & UI_FLAG_HIDDEN) continue;

        if (c->w == 0) c->w = (f->el.target_w > 0) ? f->el.target_w : 100;
        if (c->h == 0) c->h = (f->el.target_h > 0) ? f->el.target_h : 30;

        if (f->el.flags & UI_LAYOUT_GRID) {
            if (cx + c->w + f->padding > f->el.w || 
                (f->max_grid_cols > 0 && col_count >= f->max_grid_cols)) {
                cx = f->padding;
                cy += max_row_h + f->spacing;
                max_row_h = 0;
                col_count = 0;
            }
            c->x = cx;
            c->y = cy;
            cx += c->w + f->spacing;
            if (c->h > max_row_h) max_row_h = c->h;
            col_count++;
        } else if (f->el.flags & UI_LAYOUT_HORIZONTAL) {
            c->x = cx;
            c->y = cy;
            cx += c->w + f->spacing;
            if (c->h > max_row_h) max_row_h = c->h;
        } else {
            c->x = cx;
            c->y = cy;
            cy += c->h + f->spacing;
        }
    }
    f->content_height = cy + max_row_h + f->padding;
}

int compare_elements_desc(const void* a, const void* b) {
    UIElement* ea = *(UIElement**)a;
    UIElement* eb = *(UIElement**)b;
    
    if (ea->z_index != eb->z_index) {
        return eb->z_index - ea->z_index;
    }
    
    return (int)(eb->creation_time - ea->creation_time);
}

UIElement* _get_hit(list* l, int mx, int my, int px, int py) {
    int count = list_length(l);
    if (count == 0) return NULL;

    UIElement** sorted = malloc(count * sizeof(UIElement*));
    if (!sorted) return NULL;

    for (int i = 0; i < count; i++) {
        sorted[i] = list_get(l, i);
    }
    
    qsort(sorted, count, sizeof(UIElement*), compare_elements_desc);
    
    UIElement* result = NULL;

    for (int i = 0; i < count; i++) {
        UIElement* e = sorted[i];
        if (e->flags & UI_FLAG_HIDDEN) continue;
        
        int wx = px + e->x;
        int wy = py + e->y;
        
        int hit_h = e->h;
        
        if (e->type == UI_DROPDOWN) {
            UIDropdown* dd = (UIDropdown*)e;
            if (dd->is_open) {
                hit_h = 30 + (dd->option_count * 30);
            }
        }

        if (mx >= wx && mx <= wx + e->w && my >= wy && my <= wy + hit_h) {
            int scroll = (e->type == UI_FRAME) ? ((UIFrame*)e)->scroll_y : 0;
            
            UIElement* child = _get_hit(e->children, mx, my, wx, wy - scroll);
            
            result = child ? child : e;
            break; 
        }
    }

    free(sorted);
    return result;
}

void init_base(UIElement* el, int x, int y, int w, int h, UIType t) {
    el->x = x;
    el->y = y;
    el->w = w;
    el->h = h;
    el->type = t;
    el->flags = UI_FLAG_NONE;
    el->parent = NULL;
    el->children = list_new();
    el->_is_hovered = 0;
    el->_is_hovered_prev = 0;
    el->_is_dragging = 0;
    el->z_index = 0;
    el->creation_time = SDL_GetTicks();
    el->transparency = 1.0f;
    el->custom_color = SX_COLOR_NONE;
    el->has_custom_color = 0;
    el->onMouseEnter = list_new();
    el->onMouseLeave = list_new();
    
    el->effects.gradient.enabled = 0;
    el->effects.gradient.stops = NULL;
    el->effects.gradient.angle = 90.0f;
    el->effects.outline.enabled = 0;
    el->effects.outline.width = 1;
    el->effects.outline.color = SX_COLOR_BLACK;
    el->effects.outline.alpha = 255;
    el->effects.rounded.enabled = 0;
    el->effects.rounded.radius = 0;
}

void _add_to_parent(UIElement* p, UIElement* c) {
    if (p) {
        c->parent = p;
        list_add(p->children, c);
        if (p->type == UI_FRAME) sx_update_layout((UIFrame*)p);
    } else {
        list_add(engine.root, c);
    }
}

void sxui_init(const char* title, int w, int h, Uint32 seed) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    
    engine.window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, 
                                     SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_SHOWN);
    engine.renderer = SDL_CreateRenderer(engine.window, -1, 
                                        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

    engine.default_font = TTF_OpenFont("fonts/Montserrat-Regular.ttf", 16);
    if (!engine.default_font) {
        printf("Warning: Could not load fallback font Montserrat.\n");
    }
    engine.custom_font = NULL;
    
    engine.root = list_new();
    engine.theme = sx_generate_palette(seed, THEME_DARK);
    engine.running = 1;
    engine.focused = NULL;
    engine.dragging_el = NULL;
    engine.page_manager.pages = NULL;
    engine.page_manager.current_page = -1;
    engine.page_manager.initialized = 0;
    engine.file_drop_callback = NULL;
}

int sxui_load_font(const char* path, int size) {
    if (engine.custom_font) {
        TTF_CloseFont(engine.custom_font);
    }
    engine.custom_font = TTF_OpenFont(path, size);
    return (engine.custom_font != NULL);
}

void sxui_set_theme(Uint32 seed, UIThemeMode mode) {
    engine.theme = sx_generate_palette(seed, mode);
}

void _delete_element_recursive(UIElement* el);

void sxui_cleanup(void) {
    if (engine.root) {
        for (size_t i = 0; i < list_length(engine.root); i++) {
            UIElement* el = list_get(engine.root, i);
            _delete_element_recursive(el);
        }
        list_free(engine.root);
    }
    
    if (engine.page_manager.pages) {
        list_free(engine.page_manager.pages);
    }
    
    if (engine.default_font) {
        TTF_CloseFont(engine.default_font);
        engine.default_font = NULL;
    }
    if (engine.custom_font) {
        TTF_CloseFont(engine.custom_font);
        engine.custom_font = NULL;
    }
    SDL_DestroyRenderer(engine.renderer);
    SDL_DestroyWindow(engine.window);
    TTF_Quit();
    SDL_Quit();
}

int sxui_should_quit(void) {
    return !engine.running;
}

void sxui_poll_events(void) {
    SDL_Event e;
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            engine.running = 0;
        }
        
        if (e.type == SDL_DROPFILE) {
            char* dropped_file = e.drop.file;
            UIElement* hit = _get_hit(engine.root, mx, my, 0, 0);
            
            if (engine.file_drop_callback) {
                engine.file_drop_callback(hit, dropped_file);
            }
            
            SDL_free(dropped_file);
        }

        if (e.type == SDL_MOUSEBUTTONDOWN) {
            UIElement* hit = _get_hit(engine.root, mx, my, 0, 0);
            int clicked_ui = (hit != NULL);

            if (engine.focused && engine.focused != hit) {
                if (engine.focused->type == UI_INPUT) {
                    trigger_focus((UITextInput*)engine.focused, 0);
                    SDL_StopTextInput();
                }
                if (engine.focused->type == UI_DROPDOWN) {
                    ((UIDropdown*)engine.focused)->is_open = 0;
                }
                engine.focused = NULL;
            }
            
            if (hit) {
                if (hit->flags & UI_FLAG_DRAGGABLE) {
                    engine.dragging_el = hit;
                    engine.drag_off_x = mx - hit->x;
                    engine.drag_off_y = my - hit->y;
                }
                
                if (hit->type == UI_BUTTON) {
                    UIButton* b = (UIButton*)hit;
                    b->_pressed = 1;
                    b->_lastClickTime = SDL_GetTicks();
                    trigger_click(b);
                }
                else if (hit->type == UI_CHECKBOX) {
                    UICheckBox* cb = (UICheckBox*)hit;
                    cb->value = !cb->value;
                    trigger_value_changed(cb, (float)cb->value);
                }
                else if (hit->type == UI_SLIDER) {
                    engine.focused = hit;
                }
                else if (hit->type == UI_DROPDOWN) {
                    UIDropdown* dd = (UIDropdown*)hit;
                    int header_h = 30;
                    int wy = hit->y;
                    UIElement* p = hit->parent;
                    while (p) {
                        wy += p->y;
                        if (p->type == UI_FRAME) wy -= ((UIFrame*)p)->scroll_y;
                        p = p->parent;
                    }
                    
                    if (my < wy + header_h) {
                        dd->is_open = !dd->is_open;
                        engine.focused = (UIElement*)dd;
                    } else if (dd->is_open) {
                        int item_idx = (my - (wy + header_h)) / 30;
                        if (item_idx >= 0 && item_idx < dd->option_count) {
                            dd->selected_index = item_idx;
                            dd->is_open = 0;
                            trigger_dropdown_changed(dd, item_idx, dd->options[item_idx]);
                        }
                    }
                }
                else if (hit->type == UI_INPUT) {
                    UITextInput* ti = (UITextInput*)hit;
                    if (engine.focused != (UIElement*)ti) {
                        if (engine.focused && engine.focused->type == UI_INPUT) {
                            trigger_focus((UITextInput*)engine.focused, 0);
                        }
                        engine.focused = (UIElement*)ti;
                        trigger_focus(ti, 1);
                        SDL_StartTextInput();
                    }
                    ti->cursorPosition = ti->len;
                    ti->selectionAnchor = ti->len;
                }
            }
            
            if (!clicked_ui && engine.focused) {
                if (engine.focused->type == UI_INPUT) {
                    trigger_focus((UITextInput*)engine.focused, 0);
                    SDL_StopTextInput();
                }
                if (engine.focused->type == UI_DROPDOWN) {
                    ((UIDropdown*)engine.focused)->is_open = 0;
                }
                engine.focused = NULL;
            }
        }
        
        if (e.type == SDL_MOUSEBUTTONUP) {
            if (engine.dragging_el) {
                engine.dragging_el = NULL;
            }
            if (engine.focused && engine.focused->type == UI_SLIDER) {
                engine.focused = NULL;
            }
        }
        
        if (e.type == SDL_MOUSEWHEEL) {
            UIElement* hit = _get_hit(engine.root, mx, my, 0, 0);
            while (hit) {
                if (hit->type == UI_FRAME && (hit->flags & UI_SCROLLABLE)) {
                    UIFrame* f = (UIFrame*)hit;
                    f->scroll_y -= e.wheel.y * 40;
                    if (f->scroll_y < 0) f->scroll_y = 0;
                    int limit = f->content_height - hit->h;
                    if (f->scroll_y > limit && limit > 0) f->scroll_y = limit;
                    f->last_scroll_time = SDL_GetTicks();
                    break;
                }
                hit = hit->parent;
            }
        }

        if (engine.focused && engine.focused->type == UI_INPUT) {
            if (e.type == SDL_TEXTINPUT || e.type == SDL_KEYDOWN) {
                read_input((UITextInput*)engine.focused, &e);
            }
        }
    }

    if (engine.dragging_el) {
        engine.dragging_el->x = mx - engine.drag_off_x;
        engine.dragging_el->y = my - engine.drag_off_y;
    }

    if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        if (engine.focused && engine.focused->type == UI_SLIDER) {
            UISlider* s = (UISlider*)engine.focused;
            int wx = s->el.x;
            UIElement* p = s->el.parent;
            while (p) {
                wx += p->x;
                if (p->type == UI_FRAME) {
                    wx -= ((UIFrame*)p)->scroll_y;
                }
                p = p->parent;
            }
            float old_val = s->value;
            s->value = (float)(mx - wx) / s->el.w;
            if (s->value < 0) s->value = 0;
            if (s->value > 1) s->value = 1;
            if (fabs(s->value - old_val) > 0.001f) {
                trigger_value_changed(s, s->value);
            }
        }
    }
}

UIElement* sxui_frame(UIElement* p, int x, int y, int w, int h, int flags) {
    UIFrame* f = calloc(1, sizeof(UIFrame));
    init_base(&f->el, x, y, w, h, UI_FRAME);
    f->el.flags = flags;
    f->padding = 10;
    f->spacing = 8;
    f->scroll_bar_width = 6;
    _add_to_parent(p, (UIElement*)f);
    return (UIElement*)f;
}

UIElement* sxui_button(UIElement* p, const char* label, ClickCallback cb) {
    UIButton* b = calloc(1, sizeof(UIButton));
    init_base(&b->el, 0, 0, 0, 0, UI_BUTTON);
    b->text = strdup(label);
    b->onClick = list_new();
    if (cb) bind_event(b->onClick, cb);
    _add_to_parent(p, (UIElement*)b);
    return (UIElement*)b;
}

UIElement* sxui_label(UIElement* p, const char* text) {
    UILabel* l = calloc(1, sizeof(UILabel));
    init_base(&l->el, 0, 0, 0, 0, UI_LABEL);
    l->text = strdup(text);
    _add_to_parent(p, (UIElement*)l);
    return (UIElement*)l;
}

UIElement* sxui_input(UIElement* p, const char* placeholder, int is_pass) {
    UITextInput* i = calloc(1, sizeof(UITextInput));
    init_base(&i->el, 0, 0, 0, 0, UI_INPUT);
    if (is_pass) i->el.flags |= UI_FLAG_PASSWORD;
    strncpy(i->placeholder, placeholder, INPUT_MAX - 1);
    i->onFocusChanged = list_new();
    i->onTextChanged = list_new();
    i->onSubmit = list_new();
    _add_to_parent(p, (UIElement*)i);
    return (UIElement*)i;
}

UIElement* sxui_checkbox(UIElement* p, const char* label) {
    UICheckBox* c = calloc(1, sizeof(UICheckBox));
    init_base(&c->el, 0, 0, 0, 0, UI_CHECKBOX);
    c->text = strdup(label);
    c->onValueChanged = list_new();
    _add_to_parent(p, (UIElement*)c);
    return (UIElement*)c;
}

UIElement* sxui_slider(UIElement* p, float initial) {
    UISlider* s = calloc(1, sizeof(UISlider));
    init_base(&s->el, 0, 0, 0, 0, UI_SLIDER);
    s->value = initial;
    s->onValueChanged = list_new();
    _add_to_parent(p, (UIElement*)s);
    return (UIElement*)s;
}

UIElement* sxui_dropdown(UIElement* parent, const char** options, int option_count, int default_index) {
    UIDropdown* dd = calloc(1, sizeof(UIDropdown));
    init_base(&dd->el, 0, 0, 0, 0, UI_DROPDOWN);
    
    dd->option_count = option_count;
    dd->options = malloc(sizeof(char*) * option_count);
    for (int i = 0; i < option_count; i++) {
        dd->options[i] = strdup(options[i]);
    }
    dd->selected_index = (default_index >= 0 && default_index < option_count) ? default_index : 0;
    dd->is_open = 0;
    dd->el.z_index = DROPDOWN_Z_INDEX;
    dd->onSelectionChanged = list_new();
    
    _add_to_parent(parent, (UIElement*)dd);
    return (UIElement*)dd;
}

UIElement* sxui_canvas(UIElement* parent, int x, int y, int w, int h) {
    UICanvas* c = calloc(1, sizeof(UICanvas));
    init_base(&c->el, x, y, w, h, UI_CANVAS);
    
    c->texture = SDL_CreateTexture(engine.renderer, SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_STREAMING, w, h);
    c->pixels = calloc(w * h, sizeof(Uint32));
    c->pitch = w * sizeof(Uint32);
    
    _add_to_parent(parent, (UIElement*)c);
    return (UIElement*)c;
}

void _delete_element_recursive(UIElement* el) {
    if (!el) return;
    
    for (size_t i = 0; i < list_length(el->children); i++) {
        _delete_element_recursive(list_get(el->children, i));
    }
    
    list_free(el->children);
    list_free(el->onMouseEnter);
    list_free(el->onMouseLeave);
    
    if (el->effects.gradient.stops) {
        for (size_t i = 0; i < list_length(el->effects.gradient.stops); i++) {
            free(list_get(el->effects.gradient.stops, i));
        }
        list_free(el->effects.gradient.stops);
    }
    
    switch (el->type) {
        case UI_BUTTON:
            free(((UIButton*)el)->text);
            list_free(((UIButton*)el)->onClick);
            break;
        case UI_LABEL:
            free(((UILabel*)el)->text);
            break;
        case UI_INPUT:
            list_free(((UITextInput*)el)->onFocusChanged);
            list_free(((UITextInput*)el)->onTextChanged);
            list_free(((UITextInput*)el)->onSubmit);
            break;
        case UI_CHECKBOX:
            free(((UICheckBox*)el)->text);
            list_free(((UICheckBox*)el)->onValueChanged);
            break;
        case UI_SLIDER:
            list_free(((UISlider*)el)->onValueChanged);
            break;
        case UI_DROPDOWN:
            for (int i = 0; i < ((UIDropdown*)el)->option_count; i++) {
                free(((UIDropdown*)el)->options[i]);
            }
            free(((UIDropdown*)el)->options);
            list_free(((UIDropdown*)el)->onSelectionChanged);
            break;
        case UI_CANVAS:
            if (((UICanvas*)el)->texture) {
                SDL_DestroyTexture(((UICanvas*)el)->texture);
            }
            free(((UICanvas*)el)->pixels);
            break;
        default:
            break;
    }
    
    free(el);
}

UIElement* sxui_clone(UIElement* element) {
    if (!element) return NULL;
    
    UIElement* clone = NULL;
    
    switch (element->type) {
        case UI_BUTTON: {
            UIButton* src = (UIButton*)element;
            clone = sxui_button(NULL, src->text, NULL);
            break;
        }
        case UI_LABEL: {
            UILabel* src = (UILabel*)element;
            clone = sxui_label(NULL, src->text);
            break;
        }
        case UI_INPUT: {
            UITextInput* src = (UITextInput*)element;
            clone = sxui_input(NULL, src->placeholder, (element->flags & UI_FLAG_PASSWORD));
            sxui_set_text(clone, src->text);
            break;
        }
        case UI_CHECKBOX: {
            UICheckBox* src = (UICheckBox*)element;
            clone = sxui_checkbox(NULL, src->text);
            ((UICheckBox*)clone)->value = src->value;
            break;
        }
        case UI_SLIDER: {
            UISlider* src = (UISlider*)element;
            clone = sxui_slider(NULL, src->value);
            break;
        }
        case UI_DROPDOWN: {
            UIDropdown* src = (UIDropdown*)element;
            clone = sxui_dropdown(NULL, (const char**)src->options, src->option_count, src->selected_index);
            break;
        }
        case UI_FRAME: {
            clone = sxui_frame(NULL, element->x, element->y, element->w, element->h, element->flags);
            UIFrame* src = (UIFrame*)element;
            UIFrame* dst = (UIFrame*)clone;
            dst->padding = src->padding;
            dst->spacing = src->spacing;
            dst->max_grid_cols = src->max_grid_cols;
            break;
        }
        case UI_CANVAS: {
            clone = sxui_canvas(NULL, element->x, element->y, element->w, element->h);
            UICanvas* src = (UICanvas*)element;
            UICanvas* dst = (UICanvas*)clone;
            memcpy(dst->pixels, src->pixels, element->w * element->h * sizeof(Uint32));
            break;
        }
    }
    
    if (clone) {
        clone->x = element->x;
        clone->y = element->y;
        clone->w = element->w;
        clone->h = element->h;
        clone->flags = element->flags;
        clone->z_index = element->z_index;
        clone->transparency = element->transparency;
        clone->custom_color = element->custom_color;
        clone->has_custom_color = element->has_custom_color;
        clone->effects = element->effects;
        
        if (element->effects.gradient.stops) {
            clone->effects.gradient.stops = list_new();
            for (size_t i = 0; i < list_length(element->effects.gradient.stops); i++) {
                GradientStop* orig = list_get(element->effects.gradient.stops, i);
                GradientStop* copy = malloc(sizeof(GradientStop));
                *copy = *orig;
                list_add(clone->effects.gradient.stops, copy);
            }
        }
    }
    
    return clone;
}

void sxui_delete(UIElement* element) {
    if (!element) return;
    
    if (element == engine.focused) {
        engine.focused = NULL;
    }
    if (element == engine.dragging_el) {
        engine.dragging_el = NULL;
    }
    
    if (element->parent) {
        list_remove(element->parent->children, element);
        if (element->parent->type == UI_FRAME) {
            sx_update_layout((UIFrame*)element->parent);
        }
    } else {
        list_remove(engine.root, element);
    }
    
    _delete_element_recursive(element);
}

void sxui_set_position(UIElement* el, int x, int y) {
    if (el) { el->x = x; el->y = y; }
}

void sxui_set_size(UIElement* el, int w, int h) {
    if (el) { 
        el->w = w; 
        el->h = h;
        
        if (el->type == UI_CANVAS) {
            UICanvas* c = (UICanvas*)el;
            if (c->texture) SDL_DestroyTexture(c->texture);
            if (c->pixels) free(c->pixels);
            
            c->texture = SDL_CreateTexture(engine.renderer, SDL_PIXELFORMAT_RGBA8888,
                                          SDL_TEXTUREACCESS_STREAMING, w, h);
            c->pixels = calloc(w * h, sizeof(Uint32));
            c->pitch = w * sizeof(Uint32);
        }
    }
}

void sxui_set_visible(UIElement* el, int visible) {
    if (el) {
        if (visible) el->flags &= ~UI_FLAG_HIDDEN;
        else el->flags |= UI_FLAG_HIDDEN;
        
        for (size_t i = 0; i < list_length(el->children); i++) {
            sxui_set_visible(list_get(el->children, i), visible);
        }
    }
}

void sxui_set_draggable(UIElement* el, int draggable) {
    if (el) {
        if (draggable) el->flags |= UI_FLAG_DRAGGABLE;
        else el->flags &= ~UI_FLAG_DRAGGABLE;
    }
}

void sxui_set_flags(UIElement* el, int flags) {
    if (el) el->flags = flags;
}

int sxui_get_flags(UIElement* el) {
    return el ? el->flags : 0;
}

void sxui_set_z_index(UIElement* el, int z) {
    if (el) el->z_index = z;
}

int sxui_get_z_index(UIElement* el) {
    return el ? el->z_index : 0;
}

void sxui_set_transparency(UIElement* el, float alpha) {
    if (el) el->transparency = (alpha < 0.0f) ? 0.0f : (alpha > 1.0f ? 1.0f : alpha);
}

float sxui_get_transparency(UIElement* el) {
    return el ? el->transparency : 1.0f;
}

void sxui_set_custom_color(UIElement* el, Uint32 color) {
    if (el) {
        el->custom_color = color;
        el->has_custom_color = 1;
    }
}

Uint32 sxui_get_custom_color(UIElement* el) {
    return el ? el->custom_color : SX_COLOR_NONE;
}

const char* sxui_get_text(UIElement* el) {
    if (!el) return NULL;
    if (el->type == UI_BUTTON) return ((UIButton*)el)->text;
    if (el->type == UI_LABEL) return ((UILabel*)el)->text;
    if (el->type == UI_INPUT) return ((UITextInput*)el)->text;
    if (el->type == UI_CHECKBOX) return ((UICheckBox*)el)->text;
    return NULL;
}

void sxui_set_text(UIElement* el, const char* text) {
    if (!el || !text) return;
    if (el->type == UI_BUTTON) {
        free(((UIButton*)el)->text);
        ((UIButton*)el)->text = strdup(text);
    }
    else if (el->type == UI_LABEL) {
        free(((UILabel*)el)->text);
        ((UILabel*)el)->text = strdup(text);
    }
    else if (el->type == UI_INPUT) {
        UITextInput* inp = (UITextInput*)el;
        strncpy(inp->text, text, INPUT_MAX - 1);
        inp->len = strlen(inp->text);
        inp->cursorPosition = inp->len;
        inp->selectionAnchor = inp->len;
    }
    else if (el->type == UI_CHECKBOX) {
        free(((UICheckBox*)el)->text);
        ((UICheckBox*)el)->text = strdup(text);
    }
}
float sxui_get_value(UIElement* el) {
    if (!el) return 0.0f;
    if (el->type == UI_SLIDER) return ((UISlider*)el)->value;
    if (el->type == UI_CHECKBOX) return (float)((UICheckBox*)el)->value;
    return 0.0f;
}

void sxui_set_value(UIElement* el, float value) {
    if (!el) return;
    if (el->type == UI_SLIDER) {
        UISlider* s = (UISlider*)el;
        s->value = value;
        if (s->value < 0) s->value = 0;
        if (s->value > 1) s->value = 1;
    }
    else if (el->type == UI_CHECKBOX) {
        ((UICheckBox*)el)->value = (int)value;
    }
}

int sxui_get_dropdown_index(UIElement* el) {
    if (!el || el->type != UI_DROPDOWN) return -1;
    return ((UIDropdown*)el)->selected_index;
}

void sxui_set_gradient(UIElement* el, float* positions, Uint32* colors, int count, float angle) {
    if (!el || count < 2) return;
    
    if (el->effects.gradient.stops) {
        for (size_t i = 0; i < list_length(el->effects.gradient.stops); i++) {
            free(list_get(el->effects.gradient.stops, i));
        }
        list_free(el->effects.gradient.stops);
    }
    
    el->effects.gradient.stops = list_new();
    el->effects.gradient.enabled = 1;
    el->effects.gradient.angle = angle;
    
    int valid_count = 0;
    for (int i = 0; i < count; i++) {
        if (positions && colors) {
            GradientStop* stop = malloc(sizeof(GradientStop));
            stop->position = positions[i];
            stop->color = colors[i];
            list_add(el->effects.gradient.stops, stop);
            valid_count++;
        } else if (positions && !colors) {
            GradientStop* stop = malloc(sizeof(GradientStop));
            stop->position = positions[i];
            stop->color = SX_COLOR_WHITE;
            list_add(el->effects.gradient.stops, stop);
            valid_count++;
            printf("Warning: Missing color for gradient stop %d, using white\n", i);
        }
    }
    
    if (valid_count < 2) {
        printf("Warning: Gradient needs at least 2 valid stops\n");
        sxui_clear_gradient(el);
    }
}

void sxui_clear_gradient(UIElement* el) {
    if (!el) return;
    
    if (el->effects.gradient.stops) {
        for (size_t i = 0; i < list_length(el->effects.gradient.stops); i++) {
            free(list_get(el->effects.gradient.stops, i));
        }
        list_free(el->effects.gradient.stops);
        el->effects.gradient.stops = NULL;
    }
    
    el->effects.gradient.enabled = 0;
}

void sxui_set_outline(UIElement* el, int width, Uint32 color, Uint8 alpha) {
    if (!el) return;
    
    el->effects.outline.enabled = 1;
    el->effects.outline.width = width;
    el->effects.outline.color = color;
    el->effects.outline.alpha = alpha;
}

void sxui_clear_outline(UIElement* el) {
    if (!el) return;
    el->effects.outline.enabled = 0;
}

void sxui_set_rounded_corners(UIElement* el, int radius) {
    if (!el) return;
    
    el->effects.rounded.enabled = 1;
    el->effects.rounded.radius = radius;
}

void sxui_clear_rounded_corners(UIElement* el) {
    if (!el) return;
    el->effects.rounded.enabled = 0;
}

void sxui_canvas_clear(UIElement* canvas, Uint32 color) {
    if (!canvas || canvas->type != UI_CANVAS) return;
    
    UICanvas* c = (UICanvas*)canvas;
    for (int i = 0; i < canvas->w * canvas->h; i++) {
        c->pixels[i] = color;
    }
    
    SDL_UpdateTexture(c->texture, NULL, c->pixels, c->pitch);
}

void sxui_canvas_draw_pixel(UIElement* canvas, int x, int y, Uint32 color) {
    if (!canvas || canvas->type != UI_CANVAS) return;
    
    UICanvas* c = (UICanvas*)canvas;
    if (x < 0 || x >= canvas->w || y < 0 || y >= canvas->h) return;
    
    c->pixels[y * canvas->w + x] = color;
    SDL_UpdateTexture(c->texture, NULL, c->pixels, c->pitch);
}

void sxui_canvas_draw_line(UIElement* canvas, int x1, int y1, int x2, int y2, Uint32 color) {
    if (!canvas || canvas->type != UI_CANVAS) return;
    
    UICanvas* c = (UICanvas*)canvas;
    
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        if (x1 >= 0 && x1 < canvas->w && y1 >= 0 && y1 < canvas->h) {
            c->pixels[y1 * canvas->w + x1] = color;
        }
        
        if (x1 == x2 && y1 == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
    
    SDL_UpdateTexture(c->texture, NULL, c->pixels, c->pitch);
}

void sxui_canvas_draw_rect(UIElement* canvas, int x, int y, int w, int h, Uint32 color, int filled) {
    if (!canvas || canvas->type != UI_CANVAS) return;
    
    UICanvas* c = (UICanvas*)canvas;
    
    if (filled) {
        for (int py = y; py < y + h; py++) {
            for (int px = x; px < x + w; px++) {
                if (px >= 0 && px < canvas->w && py >= 0 && py < canvas->h) {
                    c->pixels[py * canvas->w + px] = color;
                }
            }
        }
    } else {
        for (int px = x; px < x + w; px++) {
            if (px >= 0 && px < canvas->w) {
                if (y >= 0 && y < canvas->h) c->pixels[y * canvas->w + px] = color;
                if (y + h - 1 >= 0 && y + h - 1 < canvas->h) c->pixels[(y + h - 1) * canvas->w + px] = color;
            }
        }
        for (int py = y; py < y + h; py++) {
            if (py >= 0 && py < canvas->h) {
                if (x >= 0 && x < canvas->w) c->pixels[py * canvas->w + x] = color;
                if (x + w - 1 >= 0 && x + w - 1 < canvas->w) c->pixels[py * canvas->w + (x + w - 1)] = color;
            }
        }
    }
    
    SDL_UpdateTexture(c->texture, NULL, c->pixels, c->pitch);
}

void sxui_canvas_draw_circle(UIElement* canvas, int cx, int cy, int radius, Uint32 color, int filled) {
    if (!canvas || canvas->type != UI_CANVAS) return;
    
    UICanvas* c = (UICanvas*)canvas;
    
    if (filled) {
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x * x + y * y <= radius * radius) {
                    int px = cx + x;
                    int py = cy + y;
                    if (px >= 0 && px < canvas->w && py >= 0 && py < canvas->h) {
                        c->pixels[py * canvas->w + px] = color;
                    }
                }
            }
        }
    } else {
        int x = 0;
        int y = radius;
        int d = 3 - 2 * radius;
        
        while (y >= x) {
            int points[8][2] = {
                {cx + x, cy + y}, {cx - x, cy + y},
                {cx + x, cy - y}, {cx - x, cy - y},
                {cx + y, cy + x}, {cx - y, cy + x},
                {cx + y, cy - x}, {cx - y, cy - x}
            };
            
            for (int i = 0; i < 8; i++) {
                int px = points[i][0];
                int py = points[i][1];
                if (px >= 0 && px < canvas->w && py >= 0 && py < canvas->h) {
                    c->pixels[py * canvas->w + px] = color;
                }
            }
            
            x++;
            if (d > 0) {
                y--;
                d = d + 4 * (x - y) + 10;
            } else {
                d = d + 4 * x + 6;
            }
        }
    }
    
    SDL_UpdateTexture(c->texture, NULL, c->pixels, c->pitch);
}

void sxui_frame_set_padding(UIElement* frame, int padding) {
    if (frame && frame->type == UI_FRAME) {
        ((UIFrame*)frame)->padding = padding;
        sx_update_layout((UIFrame*)frame);
    }
}

void sxui_frame_set_spacing(UIElement* frame, int spacing) {
    if (frame && frame->type == UI_FRAME) {
        ((UIFrame*)frame)->spacing = spacing;
        sx_update_layout((UIFrame*)frame);
    }
}

void sxui_frame_set_default_child_size(UIElement* frame, int w, int h) {
    if (frame && frame->type == UI_FRAME) {
        frame->target_w = w;
        frame->target_h = h;
        sx_update_layout((UIFrame*)frame);
    }
}

void sxui_frame_set_grid_columns(UIElement* frame, int max_cols) {
    if (frame && frame->type == UI_FRAME) {
        ((UIFrame*)frame)->max_grid_cols = max_cols;
        sx_update_layout((UIFrame*)frame);
    }
}

void sxui_frame_set_scrollbar_width(UIElement* frame, int width) {
    if (frame && frame->type == UI_FRAME) {
        ((UIFrame*)frame)->scroll_bar_width = width;
    }
}

void sxui_frame_update_layout(UIElement* frame) {
    if (frame && frame->type == UI_FRAME) {
        sx_update_layout((UIFrame*)frame);
    }
}

// Added implementations for the missing functions
int sxui_frame_get_child_count(UIElement* element) {
    if (!element || element->type != UI_FRAME) return 0;
    return list_length(element->children);
}

UIElement* sxui_frame_get_child(UIElement* element, int index) {
    if (!element || element->type != UI_FRAME) return NULL;
    if (index < 0 || index >= list_length(element->children)) return NULL;
    return (UIElement*)list_get(element->children, index);
}

void sxui_frame_add_child(UIElement* parent, UIElement* child) {
    if (parent && child) {
        // If child already has a parent, remove it from there first
        if (child->parent) {
             list_remove(child->parent->children, child);
        }
        child->parent = parent;
        list_add(parent->children, child);
        if (parent->type == UI_FRAME) sx_update_layout((UIFrame*)parent);
    }
}

void sxui_page_init(void) {
    if (!engine.page_manager.initialized) {
        engine.page_manager.pages = list_new();
        engine.page_manager.current_page = -1;
        engine.page_manager.initialized = 1;
    }
}

void sxui_add_page(UIElement* frame) {
    if (!frame) return;
    if (!engine.page_manager.initialized) sxui_page_init();
    
    list_add(engine.page_manager.pages, frame);
    
    if (engine.page_manager.current_page == -1) {
        engine.page_manager.current_page = 0;
        sxui_set_visible(frame, 1);
    } else {
        sxui_set_visible(frame, 0);
    }
}

void sxui_add_page_at(UIElement* frame, int position) {
    if (!frame) return;
    if (!engine.page_manager.initialized) sxui_page_init();
    
    if (position < 0 || position > (int)list_length(engine.page_manager.pages)) {
        sxui_add_page(frame);
        return;
    }
    
    list_insert(engine.page_manager.pages, position, frame);
    
    if (engine.page_manager.current_page == -1) {
        engine.page_manager.current_page = 0;
        sxui_set_visible(frame, 1);
    } else {
        sxui_set_visible(frame, 0);
        if (position <= engine.page_manager.current_page) {
            engine.page_manager.current_page++;
        }
    }
}

void sxui_page_next(void) {
    if (!engine.page_manager.initialized || list_length(engine.page_manager.pages) == 0) return;
    
    int old_page = engine.page_manager.current_page;
    engine.page_manager.current_page = (engine.page_manager.current_page + 1) % list_length(engine.page_manager.pages);
    
    if (old_page >= 0 && old_page < (int)list_length(engine.page_manager.pages)) {
        sxui_set_visible(list_get(engine.page_manager.pages, old_page), 0);
    }
    
    if (engine.page_manager.current_page >= 0 && engine.page_manager.current_page < (int)list_length(engine.page_manager.pages)) {
        sxui_set_visible(list_get(engine.page_manager.pages, engine.page_manager.current_page), 1);
    }
}

void sxui_page_previous(void) {
    if (!engine.page_manager.initialized || list_length(engine.page_manager.pages) == 0) return;
    
    int old_page = engine.page_manager.current_page;
    engine.page_manager.current_page--;
    if (engine.page_manager.current_page < 0) {
        engine.page_manager.current_page = list_length(engine.page_manager.pages) - 1;
    }
    
    if (old_page >= 0 && old_page < (int)list_length(engine.page_manager.pages)) {
        sxui_set_visible(list_get(engine.page_manager.pages, old_page), 0);
    }
    
    if (engine.page_manager.current_page >= 0 && engine.page_manager.current_page < (int)list_length(engine.page_manager.pages)) {
        sxui_set_visible(list_get(engine.page_manager.pages, engine.page_manager.current_page), 1);
    }
}

void sxui_switch_page(int index) {
    if (!engine.page_manager.initialized || index < 0 || index >= (int)list_length(engine.page_manager.pages)) return;
    
    int old_page = engine.page_manager.current_page;
    engine.page_manager.current_page = index;
    
    if (old_page >= 0 && old_page < (int)list_length(engine.page_manager.pages)) {
        sxui_set_visible(list_get(engine.page_manager.pages, old_page), 0);
    }
    
    sxui_set_visible(list_get(engine.page_manager.pages, engine.page_manager.current_page), 1);
}

int sxui_get_current_page(void) {
    if (!engine.page_manager.initialized) return -1;
    return engine.page_manager.current_page;
}

int sxui_get_page_count(void) {
    if (!engine.page_manager.initialized) return 0;
    return list_length(engine.page_manager.pages);
}

UIConnection sxui_on_click(UIElement* el, ClickCallback cb) {
    if (el && el->type == UI_BUTTON && cb) {
        return bind_event(((UIButton*)el)->onClick, cb);
    }
    UIConnection empty = {0, NULL};
    return empty;
}

UIConnection sxui_on_hover_enter(UIElement* el, HoverCallback cb) {
    if (el && cb) return bind_event(el->onMouseEnter, cb);
    UIConnection empty = {0, NULL};
    return empty;
}

UIConnection sxui_on_hover_leave(UIElement* el, HoverCallback cb) {
    if (el && cb) return bind_event(el->onMouseLeave, cb);
    UIConnection empty = {0, NULL};
    return empty;
}

UIConnection sxui_on_focus_changed(UIElement* el, FocusCallback cb) {
    if (el && el->type == UI_INPUT && cb) {
        return bind_event(((UITextInput*)el)->onFocusChanged, cb);
    }
    UIConnection empty = {0, NULL};
    return empty;
}

UIConnection sxui_on_text_changed(UIElement* el, TextCallback cb) {
    if (el && el->type == UI_INPUT && cb) {
        return bind_event(((UITextInput*)el)->onTextChanged, cb);
    }
    UIConnection empty = {0, NULL};
    return empty;
}

UIConnection sxui_on_submit(UIElement* el, TextCallback cb) {
    if (el && el->type == UI_INPUT && cb) {
        return bind_event(((UITextInput*)el)->onSubmit, cb);
    }
    UIConnection empty = {0, NULL};
    return empty;
}

UIConnection sxui_on_value_changed(UIElement* el, ValueCallback cb) {
    if (!el || !cb) {
        UIConnection empty = {0, NULL};
        return empty;
    }
    if (el->type == UI_SLIDER) {
        return bind_event(((UISlider*)el)->onValueChanged, cb);
    }
    if (el->type == UI_CHECKBOX) {
        return bind_event(((UICheckBox*)el)->onValueChanged, cb);
    }
    UIConnection empty = {0, NULL};
    return empty;
}

UIConnection sxui_on_dropdown_changed(UIElement* el, DropdownCallback cb) {
    if (el && el->type == UI_DROPDOWN && cb) {
        return bind_event(((UIDropdown*)el)->onSelectionChanged, cb);
    }
    UIConnection empty = {0, NULL};
    return empty;
}

void sxui_disconnect(UIConnection conn) {
    disconnect_binding(conn);
}

void sxui_set_file_drop_callback(FileDropCallback callback) {
    engine.file_drop_callback = callback;
}

int compare_elements(const void* a, const void* b) {
    UIElement* ea = *(UIElement**)a;
    UIElement* eb = *(UIElement**)b;
    
    if (ea->z_index != eb->z_index) {
        return ea->z_index - eb->z_index;
    }
    
    return (int)(ea->creation_time - eb->creation_time);
}

void sort_elements_by_z(list* elements, UIElement** sorted_array, int* count) {
    *count = 0;
    for (size_t i = 0; i < list_length(elements); i++) {
        UIElement* e = list_get(elements, i);
        if (!(e->flags & UI_FLAG_HIDDEN)) {
            sorted_array[(*count)++] = e;
        }
    }
    
    qsort(sorted_array, *count, sizeof(UIElement*), compare_elements);
}

void render_element_base(UIElement* e, int wx, int wy, Uint32 base_color) {
    Uint8 r, g, b, a;
    uint_to_rgba(base_color, &r, &g, &b, &a);
    a = (Uint8)(a * e->transparency);
    Uint32 final_color = rgba_to_uint(r, g, b, a);
    
    if (e->effects.gradient.enabled) {
        _draw_gradient_rect(wx, wy, e->w, e->h, &e->effects.gradient);
    } else if (e->effects.rounded.enabled) {
        _draw_rounded_rect(wx, wy, e->w, e->h, e->effects.rounded.radius, final_color);
    } else {
        _draw_rect(wx, wy, e->w, e->h, final_color);
    }
    
    if (e->effects.outline.enabled) {
        Uint8 or, og, ob, oa;
        uint_to_rgba(e->effects.outline.color, &or, &og, &ob, &oa);
        oa = (Uint8)((e->effects.outline.alpha / 255.0f) * e->transparency * 255);
        Uint32 outline_color = rgba_to_uint(or, og, ob, oa);
        
        for (int i = 0; i < e->effects.outline.width; i++) {
            _draw_rect_outline(wx + i, wy + i, e->w - 2 * i, e->h - 2 * i, outline_color);
        }
    }
}

void sx_render_recursive(list* l, int mx, int my, int px, int py) {
    UIElement* sorted[1024];
    int count = 0;
    sort_elements_by_z(l, sorted, &count);
    
    for (int idx = 0; idx < count; idx++) {
        UIElement* e = sorted[idx];
        
        int wx = px + e->x, wy = py + e->y;
        int is_hovered = (mx >= wx && mx <= wx + e->w && my >= wy && my <= wy + e->h);
        
        if (is_hovered && !e->_is_hovered_prev) trigger_hover(e, 1);
        else if (!is_hovered && e->_is_hovered_prev) trigger_hover(e, 0);
        e->_is_hovered_prev = is_hovered;

        Uint32 text_color = e->has_custom_color ? get_on_color(e->custom_color) : engine.theme.on_surface;

        switch (e->type) {
            case UI_FRAME: {
                UIFrame* f = (UIFrame*)e;
                Uint32 frame_color = e->has_custom_color ? e->custom_color : engine.theme.surface;
                render_element_base(e, wx, wy, frame_color);
                
                int should_clip = (e->flags & UI_FLAG_CLIP);
                if (should_clip) {
                    SDL_Rect clip = {wx, wy, e->w, e->h};
                    SDL_RenderSetClipRect(engine.renderer, &clip);
                }
                sx_render_recursive(e->children, mx, my, wx, wy - f->scroll_y);
                if (should_clip) {
                    SDL_RenderSetClipRect(engine.renderer, NULL);
                }
                
                Uint32 elapsed = SDL_GetTicks() - f->last_scroll_time;
                if (elapsed < SCROLL_FADE_MS && f->content_height > e->h) {
                    float alpha = 1.0f - ((float)elapsed / SCROLL_FADE_MS);
                    Uint32 s_col = rgba_to_uint(150, 150, 150, (Uint8)(200 * alpha * e->transparency));
                    int bh = (int)((float)e->h / f->content_height * e->h);
                    int by = wy + (int)((float)f->scroll_y / f->content_height * e->h);
                    _draw_rect(wx + e->w - f->scroll_bar_width - 2, by, f->scroll_bar_width, bh, s_col);
                }
                break;
            }
            case UI_BUTTON: {
                UIButton* b = (UIButton*)e;
                Uint32 col = e->has_custom_color ? e->custom_color : engine.theme.primary;
                if (is_hovered) col = shift_color(col, 1.2f);
                if (b->_pressed && SDL_GetTicks() - b->_lastClickTime < 100) {
                    col = shift_color(col, 0.8f);
                } else {
                    b->_pressed = 0;
                }
                render_element_base(e, wx, wy, col);
                
                Uint8 tr, tg, tb, ta;
                uint_to_rgba(text_color, &tr, &tg, &tb, &ta);
                ta = (Uint8)(ta * e->transparency);
                _draw_text(b->text, wx + e->w/2, wy + e->h/2, rgba_to_uint(tr, tg, tb, ta), 1, 0);
                break;
            }
            case UI_LABEL: {
                UILabel* l = (UILabel*)e;
                Uint8 tr, tg, tb, ta;
                uint_to_rgba(text_color, &tr, &tg, &tb, &ta);
                ta = (Uint8)(ta * e->transparency);
                _draw_text(l->text, wx + e->w/2, wy + e->h/2, rgba_to_uint(tr, tg, tb, ta), 1, 0);
                break;
            }
            case UI_INPUT: {
                UITextInput* ti = (UITextInput*)e;
                int is_focused = (engine.focused == e);
                Uint32 bg_col = e->has_custom_color ? e->custom_color : engine.theme.background;
                render_element_base(e, wx, wy, bg_col);
                
                SDL_Rect clip = {wx + 5, wy + 5, e->w - 10, e->h - 10};
                SDL_RenderSetClipRect(engine.renderer, &clip);

                int is_pass = (e->flags & UI_FLAG_PASSWORD);
                
                if (ti->cursorPosition != ti->selectionAnchor) {
                    int start = (ti->cursorPosition < ti->selectionAnchor) ? 
                                ti->cursorPosition : ti->selectionAnchor;
                    int end = (ti->cursorPosition > ti->selectionAnchor) ? 
                              ti->cursorPosition : ti->selectionAnchor;
                    int x1, x2;
                    if (is_pass) {
                        x1 = start * _measure_text("*") - ti->scrollOffset;
                        x2 = end * _measure_text("*") - ti->scrollOffset;
                    } else {
                        x1 = _measure_text_len(ti->text, start) - ti->scrollOffset;
                        x2 = _measure_text_len(ti->text, end) - ti->scrollOffset;
                    }
                    _draw_rect(wx + 5 + x1, wy + 5, x2 - x1, e->h - 10, 0x0078D788);
                }

                Uint32 placeholder_col = engine.theme.on_surface;
                Uint8 pr, pg, pb, pa;
                uint_to_rgba(placeholder_col, &pr, &pg, &pb, &pa);
                pa = (Uint8)(pa * 0.5f * e->transparency);
                
                if (ti->len == 0 && !is_focused) {
                    _draw_text(ti->placeholder, wx + e->w/2, wy + e->h/2, rgba_to_uint(pr, pg, pb, pa), 1, 0);
                } else {
                    Uint8 tr, tg, tb, ta;
                    uint_to_rgba(text_color, &tr, &tg, &tb, &ta);
                    ta = (Uint8)(ta * e->transparency);
                    
                    if (!is_focused && _measure_text(ti->text) > e->w - 10) {
                        char temp[INPUT_MAX];
                        strcpy(temp, ti->text);
                        while (strlen(temp) > 0 && _measure_text(temp) > e->w - 30) {
                            temp[strlen(temp) - 1] = 0;
                        }
                        strcat(temp, "...");
                        _draw_text(temp, wx + 5, wy + e->h/2, rgba_to_uint(tr, tg, tb, ta), 0, is_pass);
                    } else {
                        _draw_text(ti->text, wx + 5 - ti->scrollOffset, wy + e->h/2, 
                                 rgba_to_uint(tr, tg, tb, ta), 0, is_pass);
                    }
                }

                if (is_focused && (SDL_GetTicks() % 1000) < 500) {
                    int cx;
                    if (is_pass) {
                        cx = ti->cursorPosition * _measure_text("*") - ti->scrollOffset;
                    } else {
                        cx = _measure_text_len(ti->text, ti->cursorPosition) - ti->scrollOffset;
                    }
                    Uint8 tr, tg, tb, ta;
                    uint_to_rgba(text_color, &tr, &tg, &tb, &ta);
                    ta = (Uint8)(ta * e->transparency);
                    _draw_rect(wx + 5 + cx, wy + 5, 2, e->h - 10, rgba_to_uint(tr, tg, tb, ta));
                }

                SDL_RenderSetClipRect(engine.renderer, NULL);
                
                if (is_focused) {
                    Uint32 focus_col = engine.theme.primary;
                    Uint8 fr, fg, fb, fa;
                    uint_to_rgba(focus_col, &fr, &fg, &fb, &fa);
                    fa = (Uint8)(fa * e->transparency);
                    _draw_rect(wx, wy + e->h - 2, e->w, 2, rgba_to_uint(fr, fg, fb, fa));
                }
                break;
            }
            case UI_CHECKBOX: {
                UICheckBox* cb = (UICheckBox*)e;
                int boxSize = e->h;
                Uint32 boxCol = e->has_custom_color ? e->custom_color : engine.theme.outline;
                if (is_hovered) boxCol = shift_color(boxCol, 1.2f);
                
                Uint8 br, bg, bb, ba;
                uint_to_rgba(boxCol, &br, &bg, &bb, &ba);
                ba = (Uint8)(ba * e->transparency);
                _draw_rect(wx, wy, boxSize, boxSize, rgba_to_uint(br, bg, bb, ba));
                
                if (cb->value) {
                    Uint32 check_col = engine.theme.primary;
                    Uint8 cr, cg, cb_b, ca;
                    uint_to_rgba(check_col, &cr, &cg, &cb_b, &ca);
                    ca = (Uint8)(ca * e->transparency);
                    _draw_rect(wx + 4, wy + 4, boxSize - 8, boxSize - 8, rgba_to_uint(cr, cg, cb_b, ca));
                }
                
                Uint8 tr, tg, tb, ta;
                uint_to_rgba(text_color, &tr, &tg, &tb, &ta);
                ta = (Uint8)(ta * e->transparency);
                _draw_text(cb->text, wx + boxSize + 10, wy + boxSize/2, rgba_to_uint(tr, tg, tb, ta), 0, 0);
                break;
            }
            case UI_SLIDER: {
                UISlider* s = (UISlider*)e;
                Uint32 track_col = e->has_custom_color ? shift_color(e->custom_color, 0.7f) : engine.theme.background;
                Uint8 tkr, tkg, tkb, tka;
                uint_to_rgba(track_col, &tkr, &tkg, &tkb, &tka);
                tka = (Uint8)(tka * e->transparency);
                _draw_rect(wx, wy + e->h/2 - 2, e->w, 4, rgba_to_uint(tkr, tkg, tkb, tka));
                
                int handle_x = (int)(s->value * (e->w - 12));
                Uint32 handle_col = e->has_custom_color ? e->custom_color : engine.theme.primary;
                if (is_hovered) handle_col = shift_color(handle_col, 1.2f);
                
                Uint8 hr, hg, hb, ha;
                uint_to_rgba(handle_col, &hr, &hg, &hb, &ha);
                ha = (Uint8)(ha * e->transparency);
                _draw_rect(wx + handle_x, wy, 12, e->h, rgba_to_uint(hr, hg, hb, ha));
                break;
            }
            case UI_DROPDOWN: {
                UIDropdown* dd = (UIDropdown*)e;
                Uint32 bg_col = e->has_custom_color ? e->custom_color : engine.theme.surface;
                if (is_hovered) bg_col = shift_color(bg_col, 1.1f);
                
                int header_h = 30;
                render_element_base(e, wx, wy, bg_col);
                
                Uint8 tr, tg, tb, ta;
                uint_to_rgba(text_color, &tr, &tg, &tb, &ta);
                ta = (Uint8)(ta * e->transparency);
                
                if (dd->selected_index >= 0 && dd->selected_index < dd->option_count) {
                    _draw_text(dd->options[dd->selected_index], wx + 10, wy + header_h/2, 
                             rgba_to_uint(tr, tg, tb, ta), 0, 0);
                }
                
                char arrow = dd->is_open ? '^' : 'v';
                char arrow_str[2] = {arrow, '\0'};
                _draw_text(arrow_str, wx + e->w - 15, wy + header_h/2, rgba_to_uint(tr, tg, tb, ta), 0, 0);
                
                if (dd->is_open) {
                    int dropdown_h = dd->option_count * 30;
                    Uint32 popup_col = engine.theme.surface;
                    Uint8 pr, pg, pb, pa;
                    uint_to_rgba(popup_col, &pr, &pg, &pb, &pa);
                    pa = (Uint8)(pa * e->transparency);
                    _draw_rect(wx, wy + header_h, e->w, dropdown_h, rgba_to_uint(pr, pg, pb, pa));
                    
                    _draw_rect_outline(wx, wy + header_h, e->w, dropdown_h, engine.theme.outline);
                    
                    for (int i = 0; i < dd->option_count; i++) {
                        int item_y = wy + header_h + i * 30;
                        
                        if (mx >= wx && mx < wx + e->w && my >= item_y && my < item_y + 30) {
                            Uint32 hover_col = shift_color(popup_col, 1.2f);
                            Uint8 hvr, hvg, hvb, hva;
                            uint_to_rgba(hover_col, &hvr, &hvg, &hvb, &hva);
                            hva = (Uint8)(hva * e->transparency);
                            _draw_rect(wx, item_y, e->w, 30, rgba_to_uint(hvr, hvg, hvb, hva));
                        }
                        
                        Uint8 itr, itg, itb, ita;
                        uint_to_rgba(text_color, &itr, &itg, &itb, &ita);
                        ita = (Uint8)(ita * e->transparency);
                        _draw_text(dd->options[i], wx + 10, item_y + 15, rgba_to_uint(itr, itg, itb, ita), 0, 0);
                    }
                }
                break;
            }
            case UI_CANVAS: {
                UICanvas* c = (UICanvas*)e;
                SDL_SetTextureBlendMode(c->texture, SDL_BLENDMODE_BLEND);
                SDL_SetTextureAlphaMod(c->texture, (Uint8)(255 * e->transparency));
                SDL_Rect dst = {wx, wy, e->w, e->h};
                SDL_RenderCopy(engine.renderer, c->texture, NULL, &dst);
                break;
            }
        }
    }
}

void sxui_render(void) {
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    
    Uint8 r, g, b, a;
    uint_to_rgba(engine.theme.background, &r, &g, &b, &a);
    SDL_SetRenderDrawColor(engine.renderer, r, g, b, 255);
    SDL_RenderClear(engine.renderer);
    
    sx_render_recursive(engine.root, mx, my, 0, 0);
    SDL_RenderPresent(engine.renderer);
}
