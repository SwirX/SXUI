# SXUI Quick Reference

## Basic Setup

```c
#include "sxui.h"

int main() {
    // title, width, height, theme_seed
    sxui_init("Title", 800, 600, 0x00FF7AFF);
    
    // UI construction goes here
    
    while (!sxui_should_quit()) {
        sxui_poll_events();
        sxui_render();
    }
    
    sxui_cleanup();
    return 0;
}

```

## Widget Creation

```c
UIElement* btn = sxui_button(parent, "Text", callback);
UIElement* lbl = sxui_label(parent, "Text");
UIElement* inp = sxui_input(parent, "Placeholder", 0); // 1 for password
UIElement* chk = sxui_checkbox(parent, "Label");
UIElement* sld = sxui_slider(parent, 0.5f); // Range 0.0 - 1.0
UIElement* frm = sxui_frame(parent, x, y, w, h, flags);

```

## Layout & Logic Flags

* `UI_SCROLLABLE`: Enables vertical scrolling.
* `UI_LAYOUT_HORIZONTAL`: Arranges children left-to-right.
* `UI_LAYOUT_GRID`: Arranges children in a wrapping grid.
* `UI_FLAG_CLIP`: Clips children to frame boundaries.
* `UI_FLAG_DRAGGABLE`: Allows manual repositioning.
* `UI_FLAG_HIDDEN`: Prevents rendering and interaction.
* `UI_FLAG_PASSWORD`: Masks text input.

## Common API Calls

### Element Control

```c
sxui_set_position(el, x, y);
sxui_set_size(el, w, h);
sxui_set_visible(el, 1);    // 1=Show, 0=Hide
sxui_set_text(el, "String");
const char* txt = sxui_get_text(el);
sxui_set_value(el, 0.75f);  // For sliders/checkboxes

```

### Frame & Layout

```c
sxui_frame_set_padding(frame, 10);
sxui_frame_set_spacing(frame, 8);
sxui_frame_set_default_child_size(frame, 200, 50);
sxui_frame_set_grid_columns(frame, 4);
sxui_frame_update_layout(frame); // Call after dynamic changes

```

## Event Handling

### Connection Management

```c
// Binding events
UIConnection c1 = sxui_on_click(btn, on_click);
UIConnection c2 = sxui_on_value_changed(sld, on_change);
UIConnection c3 = sxui_on_submit(inp, on_submit);

// Disconnecting
sxui_disconnect(c1);

```

### Callback Signatures

```c
void on_click(void* el);
void on_hover(void* el, int hovered);
void on_text(void* el, const char* text);
void on_value(void* el, float value);

```

## Shortcuts (Text Input)

* **Ctrl + A**: Select All
* **Ctrl + C / X / V**: Copy / Cut / Paste
* **Ctrl + Left / Right**: Word navigation
* **Shift + Left / Right**: Text selection
* **Enter**: Trigger Submit callback and lose focus

## Compilation

```bash
# Build the library and showcase using the provided Makefile
make

# Manual compilation (if not using Makefile)
gcc -c sxui.c -Ideps -o sxui.o
gcc myapp.c sxui.o deps/dynamic_list.c -lSDL2 -lSDL2_ttf -lm -o myapp
```
