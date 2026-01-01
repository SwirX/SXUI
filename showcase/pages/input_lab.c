#include "../../sxui.h"
#include <stdio.h>

static UIElement *last_key_label;
static UIElement *mouse_test_btn;
static UIElement *key_grid;
static UIElement *key_rects[12]; // Just for show a few important keys

// Mapping some keys to show in the grid
static int watch_keys[] = {SX_KEY_UP,     SX_KEY_DOWN,      SX_KEY_LEFT,
                           SX_KEY_RIGHT,  SX_KEY_SPACE,     SX_KEY_ENTER,
                           SX_KEY_ESCAPE, SX_KEY_BACKSPACE, SX_KEY_LSHIFT,
                           SX_KEY_LCTRL,  SX_KEY_LALT,      SX_KEY_TAB};

static void on_mouse_test_click(UIElement *el, int button) {
  char buf[64];
  const char *btn_name = "UNKNOWN";

  if (button == SX_MOUSE_LEFT)
    btn_name = "LEFT";
  else if (button == SX_MOUSE_RIGHT)
    btn_name = "RIGHT";
  else if (button == SX_MOUSE_MIDDLE)
    btn_name = "MIDDLE";

  sprintf(buf, "CLICKED WITH: %s", btn_name);
  sxui_set_text(el, buf);
}

void update_input_lab(void) {
  // Update key grid brightness based on state
  for (int i = 0; i < 12; i++) {
    if (sxui_is_key_down(watch_keys[i])) {
      sxui_set_custom_color(key_rects[i], 0x00FF7AFF); // Highlight green
    } else {
      sxui_set_custom_color(key_rects[i], 0x333333FF); // Default dark
    }
  }

  // Detect any key pressed to show label
  for (int i = 0; i < 512; i++) {
    if (sxui_is_key_pressed(i)) {
      char buf[128];
      const char *name = sxui_get_scancode_name(i);
      sprintf(buf, "LAST KEY PRESSED: %s (Scancode: %d)", name, i);
      sxui_set_text(last_key_label, buf);
      break;
    }
  }
}

UIElement *create_input_lab_page(UIElement *parent) {
  int sb_w = 240; // Default sidebar width
  int pw = 1280 - sb_w;

  UIElement *page = sxui_frame(parent, sb_w, 0, pw, 800, UI_LAYOUT_VERTICAL);
  sxui_frame_set_padding(page, 40);
  sxui_frame_set_spacing(page, 20);

  sxui_label(page, "INPUT & EVENT LAB");

  last_key_label = sxui_label(page, "LAST KEY PRESSED: NONE");
  sxui_set_size(last_key_label, pw - 80, 40);
  sxui_set_custom_color(last_key_label, 0xAAAAAAFF);

  UIElement *grid_title = sxui_label(page, "KEYBOARD REAL-TIME TRACKING:");
  sxui_set_size(grid_title, pw - 80, 40);
  key_grid = sxui_frame(page, 0, 0, pw - 80, 150, UI_LAYOUT_GRID);
  sxui_frame_set_grid_columns(key_grid, 4);
  sxui_frame_set_spacing(key_grid, 10);
  sxui_frame_set_default_child_size(key_grid, (pw - 120) / 4, 40);

  for (int i = 0; i < 12; i++) {
    key_rects[i] = sxui_frame(key_grid, 0, 0, 100, 40, UI_FLAG_NONE);
    sxui_set_outline(key_rects[i], 1, 0xFFFFFFFF, 50);
    sxui_set_rounded_corners(key_rects[i], 5);
    sxui_label(key_rects[i], sxui_get_scancode_name(watch_keys[i]));
  }

  UIElement *mouse_title =
      sxui_label(page, "MOUSE EVENT TESTING (Refined Semantics):");
  sxui_set_size(mouse_title, pw - 80, 40);
  mouse_test_btn = sxui_button(page, "CLICK ME (ANY BUTTON)", NULL);
  sxui_set_size(mouse_test_btn, 300, 60);
  sxui_on_mouse_click(mouse_test_btn, on_mouse_test_click);

  UIElement *tip =
      sxui_label(page, "Tip: Right-click or Middle-click the button above. It "
                       "uses the new sxui_on_mouse_click callback.");
  sxui_set_size(tip, pw - 80, 60);

  return page;
}
