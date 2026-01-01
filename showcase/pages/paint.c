#include "../../sxui.h"
#include "../pages.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

static UIElement *paint_canvas;
static Uint32 current_color = 0xFF0000FF;
static int current_tool = 0; // 0=Brush, 1=Rect, 2=Circle
static int last_pencil_x = -1, last_pencil_y = -1;

static void on_tool_brush(void *el) {
  (void)el;
  current_tool = 0;
}
static void on_tool_rect(void *el) {
  (void)el;
  current_tool = 1;
}
static void on_tool_circle(void *el) {
  (void)el;
  current_tool = 2;
}
static void on_clear_canvas(void *el) {
  (void)el;
  sxui_canvas_clear(paint_canvas, 0xFFFFFFFF);
}

void update_paint_animation(void) {
  if (!paint_canvas)
    return;

  int mx, my;
  Uint32 buttons = SDL_GetMouseState(&mx, &my);
  if (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) {
    int sb_w = get_sidebar_width();
    int lx = mx - sb_w;
    int ly = my - 60;

    if (lx < 0 || ly < 0 || lx > 1040 || ly > 740)
      return;

    if (current_tool == 0) {
      if (last_pencil_x == -1) {
        sxui_canvas_draw_circle(paint_canvas, lx, ly, 8, current_color, 1);
      } else {
        // Draw multiple circles to fill gaps
        int dx = lx - last_pencil_x;
        int dy = ly - last_pencil_y;
        float steps = sqrtf(dx * dx + dy * dy) / 2.0f;
        if (steps < 1)
          steps = 1;
        for (float i = 0; i <= steps; i++) {
          int px = last_pencil_x + (int)(dx * (i / steps));
          int py = last_pencil_y + (int)(dy * (i / steps));
          sxui_canvas_draw_circle(paint_canvas, px, py, 8, current_color, 1);
        }
      }
      last_pencil_x = lx;
      last_pencil_y = ly;
    } else {
      last_pencil_x = -1; // Reset for shape tools
      static Uint32 last_shape_t = 0;
      if (SDL_GetTicks() - last_shape_t > 150) {
        if (current_tool == 1) {
          sxui_canvas_draw_rect(paint_canvas, lx - 40, ly - 30, 80, 60,
                                current_color, 0);
        } else if (current_tool == 2) {
          sxui_canvas_draw_circle(paint_canvas, lx, ly, 40, current_color, 0);
        }
        last_shape_t = SDL_GetTicks();
      }
    }
  } else {
    last_pencil_x = -1;
    last_pencil_y = -1;
  }
}

static void on_color(void *el) {
  UIElement *btn = (UIElement *)el;
  char *color_name = sxui_get_text(btn);
  if (strcmp(color_name, "RED") == 0)
    current_color = SX_COLOR_RED;
  else if (strcmp(color_name, "GREEN") == 0)
    current_color = SX_COLOR_GREEN;
  else if (strcmp(color_name, "BLUE") == 0)
    current_color = SX_COLOR_BLUE;
  else if (strcmp(color_name, "YELLOW") == 0)
    current_color = SX_COLOR_YELLOW;
  else if (strcmp(color_name, "MAGENTA") == 0)
    current_color = SX_COLOR_MAGENTA;
  else if (strcmp(color_name, "CYAN") == 0)
    current_color = SX_COLOR_CYAN;
  else if (strcmp(color_name, "WHITE") == 0)
    current_color = SX_COLOR_WHITE;
  else if (strcmp(color_name, "BLACK") == 0)
    current_color = SX_COLOR_BLACK;
  else if (strcmp(color_name, "ORANGE") == 0)
    current_color = SX_COLOR_ORANGE;
  else if (strcmp(color_name, "PINK") == 0)
    current_color = SX_COLOR_PINK;
  else if (strcmp(color_name, "PURPLE") == 0)
    current_color = SX_COLOR_PURPLE;
  else if (strcmp(color_name, "GRAY") == 0)
    current_color = SX_COLOR_GRAY;
}

UIElement *create_paint_page(UIElement *parent) {
  UIElement *page = sxui_frame(parent, 240, 0, 1040, 800, UI_LAYOUT_VERTICAL);
  sxui_frame_set_padding(page, 0);

  UIElement *toolbar = sxui_frame(page, 0, 0, 1040, 60, UI_LAYOUT_HORIZONTAL);
  sxui_set_custom_color(toolbar, 0x222222FF);
  sxui_frame_set_padding(toolbar, 10);
  sxui_frame_set_spacing(toolbar, 10);

  sxui_button(toolbar, "BRUSH", on_tool_brush);
  // sxui_button(toolbar, "RECT", on_tool_rect);
  // sxui_button(toolbar, "CIRCLE", on_tool_circle);
  sxui_button(toolbar, "CLEAR", on_clear_canvas);

  sxui_label(toolbar, " COLORS:");
  sxui_set_size(sxui_button(toolbar, "BLACK", on_color), 50, 50);
  sxui_set_size(sxui_button(toolbar, "RED", on_color), 50, 50);
  sxui_set_size(sxui_button(toolbar, "GREEN", on_color), 50, 50);
  sxui_set_size(sxui_button(toolbar, "BLUE", on_color), 50, 50);
  sxui_set_size(sxui_button(toolbar, "YELLOW", on_color), 50, 50);
  sxui_set_size(sxui_button(toolbar, "MAGENTA", on_color), 50, 50);
  sxui_set_size(sxui_button(toolbar, "CYAN", on_color), 50, 50);
  sxui_set_size(sxui_button(toolbar, "GRAY", on_color), 50, 50);
  sxui_set_size(sxui_button(toolbar, "ORANGE", on_color), 50, 50);
  sxui_set_size(sxui_button(toolbar, "PINK", on_color), 50, 50);
  sxui_set_size(sxui_button(toolbar, "PURPLE", on_color), 50, 50);
  sxui_set_size(sxui_button(toolbar, "WHITE", on_color), 50, 50);

  paint_canvas = sxui_canvas(page, 0, 0, 1040, 740);
  sxui_canvas_clear(paint_canvas, 0xFFFFFFFF);

  return page;
}
