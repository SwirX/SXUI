#include "../../sxui.h"
#include "../pages.h"
#include <math.h>
#include <stdio.h>

static UIElement *red_slider;
static UIElement *green_slider;
static UIElement *blue_slider;
static UIElement *radius_slider;
static UIElement *preview_box = NULL;
static UIElement *floating_window = NULL;

extern int corner_radius;
extern int esc_to_exit;

static void on_esc_toggle(void *el, float val) {
  (void)el;
  esc_to_exit = (int)val;
}

static void update_theme_vals(void *el, float val) {
  (void)el;
  (void)val;
  float r = sxui_get_value(red_slider);
  float g = sxui_get_value(green_slider);
  float b = sxui_get_value(blue_slider);

  Uint8 ir = (Uint8)(r * 255);
  Uint8 ig = (Uint8)(g * 255);
  Uint8 ib = (Uint8)(b * 255);
  Uint32 seed = (ir << 24) | (ig << 16) | (ib << 8) | 0xFF;

  update_global_theme(seed, THEME_DARK);
}

static Uint32 get_current_slider_seed(void) {
  float r = sxui_get_value(red_slider);
  float g = sxui_get_value(green_slider);
  float b = sxui_get_value(blue_slider);
  Uint8 ir = (Uint8)(r * 255);
  Uint8 ig = (Uint8)(g * 255);
  Uint8 ib = (Uint8)(b * 255);
  return (ir << 24) | (ig << 16) | (ib << 8) | 0xFF;
}

static void on_toggle_window(void *el) {
  (void)el;
  if (!floating_window) {
    floating_window = sxui_frame(NULL, 400, 200, 400, 300,
                                 UI_FLAG_DRAGGABLE | UI_LAYOUT_VERTICAL);
    sxui_set_custom_color(floating_window, 0x1A1A1AF0);
    sxui_set_outline(floating_window, 2, SX_COLOR_WHITE, 100);
    sxui_set_rounded_corners(floating_window, corner_radius);
    sxui_frame_set_padding(floating_window, 20);

    sxui_label(floating_window, "FLOATING UTILITY");
    sxui_button(floating_window, "CLOSE", on_toggle_window);
    sxui_button(floating_window, "ACTION 1", NULL);
    sxui_button(floating_window, "ACTION 2", NULL);
  } else {
    sxui_delete(floating_window);
    floating_window = NULL;
  }
}

static void on_radius_changed(UIElement *el, float val) {
  (void)el;
  corner_radius = (int)(val * 50.0f);
  if (preview_box) {
    sxui_set_rounded_corners(preview_box, corner_radius);
    sxui_render();
  }
}

static void on_header_mouse_click(UIElement *el, int button) {
  if (button == SX_MOUSE_RIGHT) {
    sxui_set_custom_color(el, SX_COLOR_MAGENTA);
  } else if (button == SX_MOUSE_MIDDLE) {
    sxui_set_custom_color(el, SX_COLOR_YELLOW);
  }
}

static void on_theme_switch(void *el) {
  (void)el;
  static UIThemeMode mode = THEME_DARK;
  mode = (mode == THEME_DARK) ? THEME_LIGHT : THEME_DARK;
  update_global_theme(get_current_slider_seed(), mode);
}

UIElement *create_dashboard_page(UIElement *parent) {
  int sb_w = get_sidebar_width();
  int pw = 1280 - sb_w;
  int ph = 800;

  UIElement *page = sxui_frame(parent, sb_w, 0, pw, ph, UI_FLAG_NONE);
  sxui_frame_set_padding(page, 40);
  sxui_frame_set_spacing(page, 20);

  UIElement *header = sxui_label(page, "DASHBOARD CONTROLS");
  sxui_set_size(header, pw - 80, 60);
  sxui_on_mouse_click(header, on_header_mouse_click);

  UIElement *left_col =
      sxui_frame(page, 40, 100, (pw - 120) / 2, ph - 140, UI_LAYOUT_VERTICAL);
  sxui_frame_set_spacing(left_col, 15);
  sxui_frame_set_default_child_size(left_col, (pw - 120) / 2 - 40, 40);

  sxui_label(left_col, "THEME ENGINE");
  sxui_button(left_col, "TOGGLE LIGHT/DARK", on_theme_switch);
  sxui_button(left_col, "TOGGLE FLOATING WINDOW", on_toggle_window);

  sxui_label(left_col, "ACCENT COLOR (RGB)");
  red_slider = sxui_slider(left_col, 0.25f);
  sxui_on_value_changed(red_slider, update_theme_vals);
  green_slider = sxui_slider(left_col, 0.32f);
  sxui_on_value_changed(green_slider, update_theme_vals);
  blue_slider = sxui_slider(left_col, 0.71f);
  sxui_on_value_changed(blue_slider, update_theme_vals);

  UIElement *right_col =
      sxui_frame(page, (pw - 120) / 2 + 80, 100, (pw - 120) / 2, ph - 140,
                 UI_LAYOUT_VERTICAL);
  sxui_frame_set_spacing(left_col, 15);
  sxui_frame_set_default_child_size(right_col, (pw - 120) / 2 - 40, 40);

  sxui_label(right_col, "UI PROPERTIES");
  sxui_label(right_col, "Global Corner Radius");
  radius_slider = sxui_slider(right_col, (float)corner_radius / 50.0f);
  sxui_on_value_changed(radius_slider, (ValueCallback)on_radius_changed);

  UIElement *esc_box = sxui_checkbox(right_col, "ESC TO EXIT");
  sxui_set_value(esc_box, (float)esc_to_exit);
  sxui_on_value_changed(esc_box, on_esc_toggle);

  preview_box = sxui_frame(right_col, 0, 50, 300, 150, UI_FLAG_NONE);
  sxui_set_rounded_corners(preview_box, corner_radius);
  sxui_set_custom_color(preview_box, 0x333333FF);
  sxui_label(preview_box, "PREVIEW BOX");

  return page;
}
