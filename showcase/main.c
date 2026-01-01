#include "pages.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Uint32 current_seed = 0x3F51B5FF;
static UIThemeMode current_mode = THEME_DARK;
static UIElement *sidebar_title;
static UIElement *page_list[8];
static UIElement *sidebar_el;
static UIElement *exit_btn;
static int sidebar_collapsed = 0;
int corner_radius = 25;
int esc_to_exit = 1;

void update_global_theme(Uint32 seed, UIThemeMode mode) {
  current_seed = seed;
  current_mode = mode;
  sxui_set_theme(seed, mode);
}

int get_sidebar_width(void) { return sidebar_collapsed ? 70 : 240; }

static void on_nav_click(void *element) {
  UIElement *btn = (UIElement *)element;
  const char *label = sxui_get_text(btn);
  if (!label)
    return;

  if (strstr(label, "Dashboard") || strstr(label, "DSH"))
    sxui_switch_page(0);
  else if (strstr(label, "Editor") || strstr(label, "EDT"))
    sxui_switch_page(1);
  else if (strstr(label, "Canvas Lab") || strstr(label, "LAB"))
    sxui_switch_page(2);
  else if (strstr(label, "Paint") || strstr(label, "PNT"))
    sxui_switch_page(3);
  else if (strstr(label, "File Drop") || strstr(label, "D&D"))
    sxui_switch_page(4);
  else if (strstr(label, "Stress Test") || strstr(label, "STR"))
    sxui_switch_page(5);
  else if (strstr(label, "Gradient") || strstr(label, "GRD"))
    sxui_switch_page(6);
  else if (strstr(label, "Input Lab") || strstr(label, "INP"))
    sxui_switch_page(7);
}

static void on_toggle_sidebar(void *el) {
  sidebar_collapsed = !sidebar_collapsed;
  int w = sidebar_collapsed ? 70 : 240;
  sxui_set_size(sidebar_el, w, 800);

  const char *labels_full[] = {"Dashboard",    "Editor",    "Canvas Lab",
                               "Paint",        "File Drop", "Stress Test",
                               "Gradient Lab", "Input Lab"};
  const char *labels_mini[] = {"DSH", "EDT", "LAB", "PNT",
                               "D&D", "STR", "GRD", "INP"};

  sxui_set_text(el, sidebar_collapsed ? ">" : "Toggle Sidebar");
  sxui_set_size(el, w - 20, 50);

  sxui_set_text(sidebar_title, sidebar_collapsed ? "SXUI" : "SXUI SHOWCASE");
  sxui_set_size(sidebar_title, w - 20, 50);

  sxui_set_text(exit_btn, sidebar_collapsed ? "X" : "Exit");
  sxui_set_size(exit_btn, w - 20, 50);

  for (int i = 0; i < 8; i++) {
    UIElement *btn = sxui_frame_get_child(sidebar_el, i + 2);
    if (btn) {
      sxui_set_text(btn, sidebar_collapsed ? labels_mini[i] : labels_full[i]);
      sxui_set_size(btn, w - 20, 50);
    }
  }

  for (int i = 0; i < 8; i++) {
    if (page_list[i]) {
      sxui_set_position(page_list[i], w, 0);
      sxui_set_size(page_list[i], 1280 - w, 800);
      sxui_frame_update_layout(page_list[i]);
    }
  }
}

static void quit_app(void *el) {
  (void)el;
  sxui_quit();
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  sxui_init("SXUI Technical Showcase", 1280, 800, current_seed);
  sxui_set_theme(current_seed, current_mode);

  sidebar_el =
      sxui_frame(NULL, 0, 0, 240, 800, UI_LAYOUT_VERTICAL | UI_FLAG_NONE);
  // sxui_set_custom_color(sidebar_el, 0x141414FF);
  sxui_frame_set_padding(sidebar_el, 10);
  sxui_frame_set_spacing(sidebar_el, 10);
  sxui_frame_set_default_child_size(sidebar_el, 220, 50);

  sidebar_title = sxui_label(sidebar_el, "SXUI SHOWCASE");
  UIElement *toggle =
      sxui_button(sidebar_el, "Toggle Sidebar", on_toggle_sidebar);
  sxui_set_custom_color(toggle, 0x333333FF);
  sxui_set_rounded_corners(toggle, corner_radius);

  const char *labels_full[] = {"Dashboard",    "Editor",    "Canvas Lab",
                               "Paint",        "File Drop", "Stress Test",
                               "Gradient Lab", "Input Lab"};
  for (int i = 0; i < 8; i++) {
    UIElement *btn = sxui_button(sidebar_el, labels_full[i], on_nav_click);
    sxui_set_rounded_corners(btn, corner_radius);
  }

  exit_btn = sxui_button(sidebar_el, "Exit", quit_app);
  sxui_set_custom_color(exit_btn, SX_COLOR_RED);
  sxui_set_rounded_corners(exit_btn, corner_radius);

  sxui_page_init();

  page_list[0] = create_dashboard_page(NULL);
  page_list[1] = create_editor_page(NULL);
  page_list[2] = create_canvas_lab_page(NULL);
  page_list[3] = create_paint_page(NULL);
  page_list[4] = create_files_page(NULL);
  page_list[5] = create_stress_page(NULL);
  page_list[6] = create_gradient_lab_page(NULL);
  page_list[7] = create_input_lab_page(NULL);

  for (int i = 0; i < 8; i++)
    sxui_add_page(page_list[i]);

  sxui_switch_page(0);

  while (!sxui_should_quit()) {
    sxui_poll_events();

    if (esc_to_exit && sxui_is_key_pressed(SX_KEY_ESCAPE)) {
      sxui_quit();
    }

    int cp = sxui_get_current_page();
    if (cp == 2) {
      update_canvas_lab_animation();
    } else if (cp == 3) {
      update_paint_animation();
    } else if (cp == 7) {
      update_input_lab();
    }

    sxui_render();
  }

  sxui_cleanup();
  return 0;
}
