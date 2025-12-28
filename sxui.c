#include "sxui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "dynamic_list.h"

#define INPUT_MAX 256
#define SCROLL_FADE_MS 1500

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

typedef struct {
    Uint32 primary;
    Uint32 secondary;
    Uint32 background;
    Uint32 surface;
    Uint32 text_p;
    Uint32 text_s;
    Uint32 border;
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
    
    // Event lists
    list* onMouseEnter;
    list* onMouseLeave;
};

typedef struct {
    UIElement el;
    char* text;
    Uint32 textColor;
    list* onClick;
    int _pressed;
    Uint32 _lastClickTime;
} UIButton;

typedef struct {
    UIElement el;
    char* text;
    Uint32 textColor;
} UILabel;

typedef struct {
    UIElement el;
    char text[INPUT_MAX];
    char placeholder[INPUT_MAX];
    Uint32 textColor;
    Uint32 placeholderColor;
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
    Uint32 textColor;
    list* onValueChanged;
} UICheckBox;

typedef struct {
    UIElement el;
    float value;
    list* onValueChanged;
} UISlider;

typedef struct {
    UIElement el;
    int padding, spacing;
    int scroll_y, content_height;
    Uint32 last_scroll_time;
    int max_grid_cols;
    int scroll_bar_width;
} UIFrame;

// ============================================================================
// ENGINE STATE
// ============================================================================

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
} SXUI_Engine;

static SXUI_Engine engine;
static int GLOBAL_CONN_ID = 0;

TTF_Font* _get_active_font() {
    if (engine.custom_font) return engine.custom_font;
    return engine.default_font;
}

// ============================================================================
// COLOR UTILITIES
// ============================================================================

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

UITheme sx_generate_palette(Uint32 seed, UIThemeMode mode) {
    UITheme t;
    t.seed = seed;
    t.mode = mode;
    t.primary = seed;
    t.secondary = shift_color(seed, 0.7f);
    
    if (mode == THEME_DARK) {
        t.background = rgba_to_uint(15, 15, 18, 255);
        t.surface = rgba_to_uint(30, 30, 35, 255);
        t.text_p = 0xFFFFFFFF;
        t.text_s = 0xAAAAAAFF;
        t.border = 0x444444FF;
    } else {
        t.background = rgba_to_uint(240, 240, 245, 255);
        t.surface = 0xFFFFFFFF;
        t.text_p = 0x1A1A1AFF;
        t.text_s = 0x666666FF;
        t.border = 0xCCCCCCFF;
    }
    return t;
}

// ============================================================================
// RENDERING UTILITIES
// ============================================================================

void _draw_rect(int x, int y, int w, int h, Uint32 c) {
    Uint8 r, g, b, a;
    uint_to_rgba(c, &r, &g, &b, &a);
    SDL_SetRenderDrawBlendMode(engine.renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(engine.renderer, r, g, b, a);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(engine.renderer, &rect);
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

// ============================================================================
// EVENT SYSTEM IMPLEMENTATION
// ============================================================================

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

// ============================================================================
// TEXT INPUT HELPERS
// ============================================================================

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

    // Auto-scroll management
    int cw = input->el.w - 10;
    int cx = _measure_text_len(input->text, input->cursorPosition);
    int rel_cx = cx - input->scrollOffset;

    if (rel_cx < 0) {
        input->scrollOffset = cx;
    } else if (rel_cx > cw) {
        input->scrollOffset = cx - cw;
    }
    
    int total_w = _measure_text(input->text);
    if (total_w < cw) input->scrollOffset = 0;
    else if (input->scrollOffset > total_w - cw) input->scrollOffset = total_w - cw;
    if (input->scrollOffset < 0) input->scrollOffset = 0;
}

// ============================================================================
// LAYOUT ENGINE
// ============================================================================

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

// ============================================================================
// HIT TESTING
// ============================================================================

UIElement* _get_hit(list* l, int mx, int my, int px, int py) {
    for (int i = list_length(l) - 1; i >= 0; i--) {
        UIElement* e = list_get(l, i);
        if (e->flags & UI_FLAG_HIDDEN) continue;
        
        int wx = px + e->x, wy = py + e->y;
        if (mx >= wx && mx <= wx + e->w && my >= wy && my <= wy + e->h) {
            int scroll = (e->type == UI_FRAME) ? ((UIFrame*)e)->scroll_y : 0;
            UIElement* child = _get_hit(e->children, mx, my, wx, wy - scroll);
            return child ? child : e;
        }
    }
    return NULL;
}

// ============================================================================
// INITIALIZATION BASE
// ============================================================================

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
    el->onMouseEnter = list_new();
    el->onMouseLeave = list_new();
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

// ============================================================================
// PUBLIC API: CORE
// ============================================================================

void sxui_init(const char* title, int w, int h, Uint32 seed) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    
    engine.window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, 
                                     SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_SHOWN);
    engine.renderer = SDL_CreateRenderer(engine.window, -1, 
                                        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    // engine.font = TTF_OpenFont("font.ttf", 16);
    // if (!engine.font) {
    //     printf("Warning: font.ttf not found. Text will not render.\n");
    // }

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

void sxui_cleanup(void) {
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

// ============================================================================
// PUBLIC API: EVENT POLLING
// ============================================================================

void sxui_poll_events(void) {
    SDL_Event e;
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            engine.running = 0;
        }

        if (e.type == SDL_MOUSEBUTTONDOWN) {
            UIElement* hit = _get_hit(engine.root, mx, my, 0, 0);
            int clicked_ui = (hit != NULL);

            // if we click anything other than the focused input then we should infocus duh
            if (engine.focused && engine.focused != hit) {
                if (engine.focused->type == UI_INPUT) {
                    trigger_focus((UITextInput*)engine.focused, 0);
                    SDL_StopTextInput();
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
            
            if (!clicked_ui && engine.focused && engine.focused->type == UI_INPUT) {
                trigger_focus((UITextInput*)engine.focused, 0);
                engine.focused = NULL;
                SDL_StopTextInput();
            }
        }
        
        if (e.type == SDL_MOUSEBUTTONUP) {
            engine.dragging_el = NULL;
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

// ============================================================================
// PUBLIC API: WIDGET CREATION
// ============================================================================

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
    b->textColor = engine.theme.text_p;
    b->onClick = list_new();
    if (cb) bind_event(b->onClick, cb);
    _add_to_parent(p, (UIElement*)b);
    return (UIElement*)b;
}

UIElement* sxui_label(UIElement* p, const char* text) {
    UILabel* l = calloc(1, sizeof(UILabel));
    init_base(&l->el, 0, 0, 0, 0, UI_LABEL);
    l->text = strdup(text);
    l->textColor = engine.theme.text_p;
    _add_to_parent(p, (UIElement*)l);
    return (UIElement*)l;
}

UIElement* sxui_input(UIElement* p, const char* placeholder, int is_pass) {
    UITextInput* i = calloc(1, sizeof(UITextInput));
    init_base(&i->el, 0, 0, 0, 0, UI_INPUT);
    if (is_pass) i->el.flags |= UI_FLAG_PASSWORD;
    strncpy(i->placeholder, placeholder, INPUT_MAX - 1);
    i->textColor = engine.theme.text_p;
    i->placeholderColor = engine.theme.text_s;
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
    c->textColor = engine.theme.text_p;
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

// ============================================================================
// PUBLIC API: ELEMENT MANIPULATION
// ============================================================================

void sxui_set_position(UIElement* el, int x, int y) {
    if (el) { el->x = x; el->y = y; }
}

void sxui_set_size(UIElement* el, int w, int h) {
    if (el) { el->w = w; el->h = h; }
}

void sxui_set_visible(UIElement* el, int visible) {
    if (el) {
        if (visible) el->flags &= ~UI_FLAG_HIDDEN;
        else el->flags |= UI_FLAG_HIDDEN;
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

// ============================================================================
// PUBLIC API: LAYOUT CONTROL
// ============================================================================

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

// ============================================================================
// PUBLIC API: EVENT SYSTEM
// ============================================================================

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

void sxui_disconnect(UIConnection conn) {
    disconnect_binding(conn);
}

// ============================================================================
// RENDERING PIPELINE
// ============================================================================

void sx_render_recursive(list* l, int mx, int my, int px, int py) {
    for (size_t i = 0; i < list_length(l); i++) {
        UIElement* e = list_get(l, i);
        if (e->flags & UI_FLAG_HIDDEN) continue;
        
        int wx = px + e->x, wy = py + e->y;
        int is_hovered = (mx >= wx && mx <= wx + e->w && my >= wy && my <= wy + e->h);
        
        if (is_hovered && !e->_is_hovered_prev) trigger_hover(e, 1);
        else if (!is_hovered && e->_is_hovered_prev) trigger_hover(e, 0);
        e->_is_hovered_prev = is_hovered;

        switch (e->type) {
            case UI_FRAME: {
                UIFrame* f = (UIFrame*)e;
                _draw_rect(wx, wy, e->w, e->h, engine.theme.surface);
                
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
                    Uint32 s_col = rgba_to_uint(150, 150, 150, (Uint8)(200 * alpha));
                    int bh = (int)((float)e->h / f->content_height * e->h);
                    int by = wy + (int)((float)f->scroll_y / f->content_height * e->h);
                    _draw_rect(wx + e->w - f->scroll_bar_width - 2, by, f->scroll_bar_width, bh, s_col);
                }
                break;
            }
            case UI_BUTTON: {
                UIButton* b = (UIButton*)e;
                Uint32 col = engine.theme.primary;
                if (is_hovered) col = shift_color(col, 1.2f);
                if (b->_pressed && SDL_GetTicks() - b->_lastClickTime < 100) {
                    col = shift_color(col, 0.8f);
                } else {
                    b->_pressed = 0;
                }
                _draw_rect(wx, wy, e->w, e->h, col);
                _draw_text(b->text, wx + e->w/2, wy + e->h/2, b->textColor, 1, 0);
                break;
            }
            case UI_LABEL: {
                UILabel* l = (UILabel*)e;
                _draw_text(l->text, wx + e->w/2, wy + e->h/2, l->textColor, 1, 0);
                break;
            }
            case UI_INPUT: {
                UITextInput* ti = (UITextInput*)e;
                int is_focused = (engine.focused == e);
                _draw_rect(wx, wy, e->w, e->h, engine.theme.background);
                
                SDL_Rect clip = {wx + 5, wy + 5, e->w - 10, e->h - 10};
                SDL_RenderSetClipRect(engine.renderer, &clip);

                if (ti->cursorPosition != ti->selectionAnchor) {
                    int start = (ti->cursorPosition < ti->selectionAnchor) ? 
                                ti->cursorPosition : ti->selectionAnchor;
                    int end = (ti->cursorPosition > ti->selectionAnchor) ? 
                              ti->cursorPosition : ti->selectionAnchor;
                    int x1 = _measure_text_len(ti->text, start) - ti->scrollOffset;
                    int x2 = _measure_text_len(ti->text, end) - ti->scrollOffset;
                    _draw_rect(wx + 5 + x1, wy + 5, x2 - x1, e->h - 10, 0x0078D788);
                }

                if (ti->len == 0 && !is_focused) {
                    _draw_text(ti->placeholder, wx + e->w/2, wy + e->h/2, ti->placeholderColor, 1, 0);
                } else {
                    int is_pass = (e->flags & UI_FLAG_PASSWORD);
                    if (!is_focused && _measure_text(ti->text) > e->w - 10) {
                        char temp[INPUT_MAX];
                        strcpy(temp, ti->text);
                        while (strlen(temp) > 0 && _measure_text(temp) > e->w - 30) {
                            temp[strlen(temp) - 1] = 0;
                        }
                        strcat(temp, "...");
                        _draw_text(temp, wx + 5, wy + e->h/2, ti->textColor, 0, is_pass);
                    } else {
                        _draw_text(ti->text, wx + 5 - ti->scrollOffset, wy + e->h/2, 
                                 ti->textColor, 0, is_pass);
                    }
                }

                if (is_focused && (SDL_GetTicks() % 1000) < 500) {
                    int cx = _measure_text_len(ti->text, ti->cursorPosition) - ti->scrollOffset;
                    _draw_rect(wx + 5 + cx, wy + 5, 2, e->h - 10, ti->textColor);
                }

                SDL_RenderSetClipRect(engine.renderer, NULL);
                
                if (is_focused) {
                    _draw_rect(wx, wy + e->h - 2, e->w, 2, engine.theme.primary);
                }
                break;
            }
            case UI_CHECKBOX: {
                UICheckBox* cb = (UICheckBox*)e;
                int boxSize = e->h;
                Uint32 boxCol = is_hovered ? shift_color(engine.theme.border, 1.2f) : engine.theme.border;
                _draw_rect(wx, wy, boxSize, boxSize, boxCol);
                
                if (cb->value) {
                    _draw_rect(wx + 4, wy + 4, boxSize - 8, boxSize - 8, engine.theme.primary);
                }
                
                _draw_text(cb->text, wx + boxSize + 10, wy + boxSize/2, cb->textColor, 0, 0);
                break;
            }
            case UI_SLIDER: {
                UISlider* s = (UISlider*)e;
                _draw_rect(wx, wy + e->h/2 - 2, e->w, 4, engine.theme.background);
                int handle_x = (int)(s->value * (e->w - 12));
                Uint32 handle_col = is_hovered ? shift_color(engine.theme.primary, 1.2f) : engine.theme.primary;
                _draw_rect(wx + handle_x, wy, 12, e->h, handle_col);
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