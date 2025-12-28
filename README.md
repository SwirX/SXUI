# SXUI - Simple eXtensible UI Library

A high-level, plug-and-play UI library for C built on SDL2. SXUI provides a simplified interface for creating responsive desktop-class interfaces without managing low-level SDL state.

## Features

### Core Widgets

- **Buttons**: Click events, hover effects, and press animations.
- **Labels**: Static and dynamic text display.
- **Text Inputs**: Full editing suite including selection, copy/paste, and word navigation.
- **Checkboxes**: State-based toggles with value callbacks.
- **Sliders**: Normalized value selection (0.0 - 1.0).
- **Frames**: Container elements with automated layout management.

### Layout Engine

- **Vertical & Horizontal Lists**: Automated element stacking.
- **Grid Layout**: Auto-wrapping grid with configurable column limits.
- **Scrollable Containers**: Clipping support with automated, fading scrollbars.
- **Dynamic Switching**: Change layout modes at runtime without recreating elements.

### Text Editing Support

- Selection via Shift + Arrow keys.
- Standard shortcuts: Ctrl+A (Select All), Ctrl+C/X/V (Copy/Cut/Paste).
- Navigation: Ctrl+Left/Right (Word-jump), Home/End.
- Word deletion: Ctrl+Backspace and Ctrl+Delete.
- Native mouse selection and horizontal auto-scrolling for long strings.

## Installation

### Dependencies

Ensure you have the SDL2 and SDL2_ttf development headers installed.

**Ubuntu/Debian:**
`sudo apt install libsdl2-dev libsdl2-ttf-dev`

**macOS:**
`brew install sdl2 sdl2_ttf`

### Project Structure

- SXUI depends on the [SXList](https://github.com/SwirX/SXList) library. For best results, place the SXList source files in `deps/` directory within your project.

    ```bash
    git clone --recursive https://github.com/SwirX/SXUI
    cd SXUI
    ```

- Font: Ensure Montserrat-Regular.ttf is in the root directory

## Building the Library

To compile SXUI as a static library:

```bash
# Compile dependencies and library
make lib

```

To link SXUI to your application:

```bash
gcc main.c libsxui.a -lSDL2 -lSDL2_ttf -lm -o my_app

```

Or to run the showcase directly:
```bash
make
./showcase
```

## Quick Start

```c
#include "sxui.h"

void on_button_click(void* element) {
    printf("Button clicked!\n");
}

int main() {
    sxui_init("Application", 800, 600, 0x00FF7AFF);
    
    UIElement* btn = sxui_button(NULL, "Click Me", on_button_click);
    sxui_set_size(btn, 200, 50);
    sxui_set_position(btn, 300, 275);
    
    while (!sxui_should_quit()) {
        sxui_poll_events();
        sxui_render();
    }
    
    sxui_cleanup();
    return 0;
}

```

## API Reference

### Initialization & Core

- `sxui_init(title, w, h, seed)`: Initialize the engine and window.
- `sxui_set_theme(seed, mode)`: Update the procedural theme.
- `sxui_poll_events()`: Process input and internal logic.
- `sxui_render()`: Draw the current frame.
- `sxui_cleanup()`: Free all library resources.

### Layout Flags

- `UI_SCROLLABLE`: Enable mouse-wheel scrolling.
- `UI_LAYOUT_GRID`: Enable auto-grid positioning.
- `UI_FLAG_CLIP`: Clip children to frame boundaries.
- `UI_FLAG_DRAGGABLE`: Allow user to move the element.

## Customization

### Fonts
SXUI ships with Montserrat-Regular as the default fallback. You can load your own .ttf font at any time:

```c
sxui_init("My App", 800, 600, 0x00FF7AFF);
sxui_load_font("path/to/my_font.ttf", 18);
```

If your custom font fails to load or the path is incorrect, the library will automatically fallback to Montserrat to ensure the UI remains usable.

## Licenses

- **SXUI**: MIT License.
- **Montserrat Font**: Licensed under the [SIL Open Font License](fonts/OFL.txt).
- **SXList**: MIT License.
