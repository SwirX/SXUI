#include "../../sxui.h"
#include "../pages.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  UIElement *frame;
  Uint32 color;
} GradientNode;

static list *gradient_nodes;
static UIElement *gradient_preview;
static UIElement *node_container;

static void update_preview(void) {
  int count = list_length(gradient_nodes);
  if (count < 2)
    return;

  float *positions = malloc(sizeof(float) * count);
  Uint32 *colors = malloc(sizeof(Uint32) * count);

  for (int i = 0; i < count; i++) {
    GradientNode *gn = list_get(gradient_nodes, i);
    positions[i] = (float)i / (count - 1);
    colors[i] = gn->color;
  }

  sxui_set_gradient(gradient_preview, positions, colors, count, 90.0f);

  free(positions);
  free(colors);
}

static void on_color_slider(void *el, float val) {
  UIElement *slider = (UIElement *)el;
  // We need to find which node this slider belongs to.
  // For simplicity in this demo, we'll just look at the parent
  UIElement *node_frame = sxui_get_parent(slider);

  for (size_t i = 0; i < list_length(gradient_nodes); i++) {
    GradientNode *gn = list_get(gradient_nodes, i);
    if (gn->frame == node_frame) {
      Uint8 hue = (Uint8)(val * 255);
      // Simple hue to RGB
      gn->color = (hue << 24) | ((255 - hue) << 16) | (128 << 8) | 0xFF;
      sxui_set_custom_color(node_frame, gn->color);
      update_preview();
      break;
    }
  }
}

static void on_remove_node(void *el) {
  UIElement *btn = (UIElement *)el;
  UIElement *node_frame = sxui_get_parent(btn);

  for (size_t i = 0; i < list_length(gradient_nodes); i++) {
    GradientNode *gn = list_get(gradient_nodes, i);
    if (gn->frame == node_frame) {
      sxui_delete(node_frame);
      list_remove(gradient_nodes, gn);
      free(gn);
      break;
    }
  }
  sxui_frame_update_layout(node_container);
  update_preview();
}

static void on_add_node(void *el) {
  (void)el;
  if (list_length(gradient_nodes) >= 10)
    return;

  GradientNode *gn = malloc(sizeof(GradientNode));
  gn->color = 0xFFFFFFFF;

  gn->frame = sxui_frame(node_container, 0, 0, 200, 60, UI_LAYOUT_HORIZONTAL);
  sxui_set_custom_color(gn->frame, gn->color);
  sxui_frame_set_padding(gn->frame, 5);
  sxui_frame_set_spacing(gn->frame, 10);

  UIElement *s = sxui_slider(gn->frame, 0.5f);
  sxui_set_size(s, 120, 30);
  sxui_on_value_changed(s, on_color_slider);

  sxui_button(gn->frame, "X", on_remove_node);

  list_add(gradient_nodes, gn);
  sxui_frame_update_layout(node_container);
  update_preview();
}

UIElement *create_gradient_lab_page(UIElement *parent) {
  int sb_w = get_sidebar_width();
  int pw = 1280 - sb_w;

  UIElement *page = sxui_frame(parent, sb_w, 0, pw, 800, UI_LAYOUT_VERTICAL);
  sxui_frame_set_padding(page, 40);
  sxui_frame_set_spacing(page, 20);

  sxui_label(page, "GRADIENT GENERATOR LAB");

  gradient_preview = sxui_frame(page, 0, 0, pw - 80, 200, UI_FLAG_NONE);
  sxui_set_outline(gradient_preview, 2, 0xFFFFFFFF, 150);
  sxui_set_rounded_corners(gradient_preview, 15);
  sxui_label(gradient_preview, "GRADIENT PREVIEW");

  UIElement *ctrls = sxui_frame(page, 0, 0, pw - 80, 50, UI_LAYOUT_HORIZONTAL);
  sxui_button(ctrls, "ADD COLOR STOP", on_add_node);

  node_container = sxui_frame(page, 0, 0, pw - 80, 400, UI_LAYOUT_GRID);
  sxui_frame_set_grid_columns(node_container, 3);
  sxui_frame_set_spacing(node_container, 15);
  sxui_frame_set_default_child_size(node_container, 250, 60);

  if (!gradient_nodes) {
    gradient_nodes = list_new();
  } else {
    // Clear old state if re-creating
    while (list_length(gradient_nodes) > 0) {
      free(list_remove_at(gradient_nodes, 0));
    }
  }

  // Initial nodes
  on_add_node(NULL);
  on_add_node(NULL);

  return page;
}
