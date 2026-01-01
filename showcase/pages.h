#ifndef SHOWCASE_PAGES_H
#define SHOWCASE_PAGES_H

#include "../sxui.h"

UIElement *create_dashboard_page(UIElement *parent);
UIElement *create_editor_page(UIElement *parent);
UIElement *create_canvas_lab_page(UIElement *parent);
UIElement *create_paint_page(UIElement *parent);
UIElement *create_files_page(UIElement *parent);
UIElement *create_stress_page(UIElement *parent);
UIElement *create_gradient_lab_page(UIElement *parent);
UIElement *create_input_lab_page(UIElement *parent);

void update_global_theme(Uint32 seed, UIThemeMode mode);

void update_canvas_lab_animation(void);
void update_paint_animation(void);
void update_input_lab(void);
int get_sidebar_width(void);

#endif
