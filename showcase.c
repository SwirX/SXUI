#include "sxui.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// ============================================================================
// APPLICATION STATE
// ============================================================================

typedef struct {
    UIElement* popup;
    UIElement* password_input;
    UIElement* grid_frame;
    UIElement* list_frame;
    UIElement* slider_value_label;
    UIElement* checkbox_status_label;
    UIElement* theme_seed_label;
    int current_seed_index;
    Uint32 seed_colors[6];
} AppState;

static AppState app;

// ============================================================================
// CALLBACK FUNCTIONS
// ============================================================================

void on_toggle_popup(void* element) {
    if (app.popup) {
        int visible = !(sxui_get_flags(app.popup) & UI_FLAG_HIDDEN);
        sxui_set_visible(app.popup, !visible);
    }
}

void on_change_seed(void* element) {
    app.current_seed_index = (app.current_seed_index + 1) % 6;
    Uint32 new_seed = app.seed_colors[app.current_seed_index];
    
    // Get current theme mode
    UIThemeMode mode = THEME_DARK; // You'd need to expose this from the engine
    sxui_set_theme(new_seed, mode);
    
    char buf[64];
    sprintf(buf, "Seed: #%08X", new_seed);
    sxui_set_text(app.theme_seed_label, buf);
}

void on_toggle_theme_mode(void* element) {
    // Toggle between light and dark
    static UIThemeMode current_mode = THEME_DARK;
    current_mode = (current_mode == THEME_DARK) ? THEME_LIGHT : THEME_DARK;
    
    Uint32 current_seed = app.seed_colors[app.current_seed_index];
    sxui_set_theme(current_seed, current_mode);
}

void on_toggle_password(void* element) {
    if (app.password_input) {
        int flags = sxui_get_flags(app.password_input);
        flags ^= UI_FLAG_PASSWORD;
        sxui_set_flags(app.password_input, flags);
    }
}

void on_input_submit(void* element, const char* text) {
    printf("✓ Input Submitted: '%s'\n", text);
}

void on_input_changed(void* element, const char* text) {
    // Uncomment to see live typing (can be noisy)
    // printf("Text Changed: '%s'\n", text);
}

void on_checkbox_toggled(void* element, float value) {
    if (app.checkbox_status_label) {
        char buf[32];
        sprintf(buf, "Status: %s", value > 0.5f ? "CHECKED" : "UNCHECKED");
        sxui_set_text(app.checkbox_status_label, buf);
    }
}

void on_slider_changed(void* element, float value) {
    if (app.slider_value_label) {
        char buf[32];
        sprintf(buf, "Value: %.2f", value);
        sxui_set_text(app.slider_value_label, buf);
    }
}

void on_toggle_grid_layout(void* element) {
    if (app.grid_frame) {
        int flags = sxui_get_flags(app.grid_frame);
        
        if (flags & UI_LAYOUT_GRID) {
            // Switch to horizontal list
            flags &= ~UI_LAYOUT_GRID;
            flags |= UI_LAYOUT_HORIZONTAL;
            printf("Switched to HORIZONTAL layout\n");
        } else if (flags & UI_LAYOUT_HORIZONTAL) {
            // Switch to vertical list
            flags &= ~UI_LAYOUT_HORIZONTAL;
            printf("Switched to VERTICAL layout\n");
        } else {
            // Switch back to grid
            flags |= UI_LAYOUT_GRID;
            printf("Switched to GRID layout\n");
        }
        
        sxui_set_flags(app.grid_frame, flags);
        sxui_frame_update_layout(app.grid_frame);
    }
}

void on_add_grid_item(void* element) {
    if (app.grid_frame) {
        static int item_count = 12;
        char buf[32];
        sprintf(buf, "Item %d", ++item_count);
        sxui_button(app.grid_frame, buf, NULL);
        sxui_frame_update_layout(app.grid_frame);
        printf("Added new item: %s\n", buf);
    }
}

void on_toggle_list_visibility(void* element) {
    if (app.list_frame) {
        int visible = !(sxui_get_flags(app.list_frame) & UI_FLAG_HIDDEN);
        sxui_set_visible(app.list_frame, !visible);
        printf("List visibility: %s\n", visible ? "HIDDEN" : "VISIBLE");
    }
}

void on_hover_enter(void* element, int hovered) {
    printf("Mouse entered element\n");
}

void on_hover_leave(void* element, int hovered) {
    printf("Mouse left element\n");
}

void on_focus_changed(void* element, int focused) {
    printf("Input focus: %s\n", focused ? "GAINED" : "LOST");
}

void on_exit(void* element) {
    printf("Exiting application...\n");
    exit(0);
}

// ============================================================================
// MAIN APPLICATION
// ============================================================================

int main(int argc, char* argv[]) {
    srand(time(NULL));
    
    // Initialize color seeds for quick switching
    app.seed_colors[0] = 0x00FF7AFF; // Green
    app.seed_colors[1] = 0xFF5A5AFF; // Red
    app.seed_colors[2] = 0x5A9FFFFF; // Blue
    app.seed_colors[3] = 0xFFAA00FF; // Orange
    app.seed_colors[4] = 0xAA5AFFFF; // Purple
    app.seed_colors[5] = 0xFF00AAFF; // Magenta
    app.current_seed_index = 0;
    
    // Initialize UI engine
    sxui_init("SXUI Complete Showcase", 1280, 800, app.seed_colors[0]);
    
    printf("╔════════════════════════════════════════════╗\n");
    printf("║   SXUI COMPLETE SHOWCASE                   ║\n");
    printf("║   Featuring all widgets and capabilities   ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    
    // ========================================================================
    // LEFT SIDEBAR - CONTROLS & SETTINGS
    // ========================================================================
    
    UIElement* sidebar = sxui_frame(NULL, 10, 10, 280, 780, UI_SCROLLABLE);
    sxui_frame_set_padding(sidebar, 15);
    sxui_frame_set_spacing(sidebar, 10);
    sxui_frame_set_default_child_size(sidebar, 250, 45);
    
    // Header label
    UIElement* title = sxui_label(sidebar, "SXUI Showcase");
    sxui_set_size(title, 250, 35);
    
    // Theme controls
    sxui_button(sidebar, "Random Seed", on_change_seed);
    sxui_button(sidebar, "Toggle Light/Dark", on_toggle_theme_mode);
    
    app.theme_seed_label = sxui_label(sidebar, "Seed: #00FF7AFF");
    sxui_set_size(app.theme_seed_label, 250, 30);
    
    // Layout demo controls
    sxui_button(sidebar, "Change Grid Layout", on_toggle_grid_layout);
    sxui_button(sidebar, "Add Grid Item", on_add_grid_item);
    sxui_button(sidebar, "Toggle List Visibility", on_toggle_list_visibility);
    
    // Popup control
    sxui_button(sidebar, "Toggle Popup", on_toggle_popup);
    
    // Checkbox demo
    UIElement* demo_checkbox = sxui_checkbox(sidebar, "Enable Feature X");
    sxui_on_value_changed(demo_checkbox, on_checkbox_toggled);
    
    app.checkbox_status_label = sxui_label(sidebar, "Status: UNCHECKED");
    sxui_set_size(app.checkbox_status_label, 250, 30);
    
    // Slider demo
    UIElement* demo_slider = sxui_slider(sidebar, 0.5f);
    sxui_set_size(demo_slider, 250, 30);
    sxui_on_value_changed(demo_slider, on_slider_changed);
    
    app.slider_value_label = sxui_label(sidebar, "Value: 0.50");
    sxui_set_size(app.slider_value_label, 250, 30);
    
    // Input demos
    UIElement* username_input = sxui_input(sidebar, "Username...", 0);
    sxui_set_size(username_input, 250, 45);
    sxui_on_submit(username_input, on_input_submit);
    sxui_on_focus_changed(username_input, on_focus_changed);
    
    app.password_input = sxui_input(sidebar, "Password...", 1);
    sxui_set_size(app.password_input, 250, 45);
    sxui_on_submit(app.password_input, on_input_submit);
    
    sxui_button(sidebar, "Show/Hide Password", on_toggle_password);
    
    // Hover demo button
    UIElement* hover_btn = sxui_button(sidebar, "Hover Test", NULL);
    sxui_on_hover_enter(hover_btn, on_hover_enter);
    sxui_on_hover_leave(hover_btn, on_hover_leave);
    
    // Exit button
    UIElement* exit_btn = sxui_button(sidebar, "Exit", on_exit);
    sxui_set_size(exit_btn, 250, 50);
    
    // ========================================================================
    // MAIN AREA - GRID LAYOUT DEMO
    // ========================================================================
    
    app.grid_frame = sxui_frame(NULL, 300, 10, 970, 400, 
                                 UI_LAYOUT_GRID | UI_SCROLLABLE);
    sxui_frame_set_padding(app.grid_frame, 15);
    sxui_frame_set_spacing(app.grid_frame, 12);
    sxui_frame_set_default_child_size(app.grid_frame, 180, 100);
    sxui_frame_set_grid_columns(app.grid_frame, 5);
    sxui_frame_set_scrollbar_width(app.grid_frame, 8);
    
    // Create grid items
    for (int i = 0; i < 12; i++) {
        char buf[32];
        sprintf(buf, "Grid Item %d", i + 1);
        UIElement* btn = sxui_button(app.grid_frame, buf, NULL);
        
        // Every 4th item is draggable
        if (i % 4 == 0) {
            sxui_set_draggable(btn, 1);
        }
    }
    
    // ========================================================================
    // VERTICAL LIST DEMO
    // ========================================================================
    
    app.list_frame = sxui_frame(NULL, 300, 420, 970, 370, UI_SCROLLABLE);
    sxui_frame_set_padding(app.list_frame, 15);
    sxui_frame_set_spacing(app.list_frame, 8);
    sxui_frame_set_default_child_size(app.list_frame, 400, 50);
    
    sxui_label(app.list_frame, "Vertical List Demo");
    
    for (int i = 0; i < 8; i++) {
        char buf[64];
        sprintf(buf, "List Item %d - Click me!", i + 1);
        sxui_button(app.list_frame, buf, NULL);
    }
    
    sxui_checkbox(app.list_frame, "List Checkbox Option 1");
    sxui_checkbox(app.list_frame, "List Checkbox Option 2");
    
    UIElement* list_input = sxui_input(app.list_frame, "Type something here...", 0);
    sxui_on_text_changed(list_input, on_input_changed);
    
    // ========================================================================
    // DRAGGABLE POPUP
    // ========================================================================
    
    app.popup = sxui_frame(NULL, 500, 250, 400, 300, UI_FLAG_DRAGGABLE);
    sxui_set_visible(app.popup, 0); // Hidden by default
    sxui_frame_set_padding(app.popup, 20);
    sxui_frame_set_spacing(app.popup, 12);
    sxui_frame_set_default_child_size(app.popup, 360, 50);
    
    UIElement* popup_title = sxui_label(app.popup, "Draggable Popup");
    sxui_set_size(popup_title, 360, 40);
    
    sxui_label(app.popup, "Drag me by the frame!");
    sxui_button(app.popup, "✓ Action Button", NULL);
    sxui_input(app.popup, "Popup Input...", 0);
    sxui_checkbox(app.popup, "Popup Checkbox");
    sxui_button(app.popup, "Close Popup", on_toggle_popup);
    
    // ========================================================================
    // MAIN LOOP
    // ========================================================================
    
    printf("\nTips:\n");
    printf("  • Try changing themes with the theme buttons\n");
    printf("  • Drag the green-bordered items in the grid\n");
    printf("  • Type in inputs - full editing support!\n");
    printf("  • Scroll the frames to see more content\n");
    printf("  • Toggle layouts dynamically\n");
    printf("  • Open the draggable popup\n\n");
    
    while (!sxui_should_quit()) {
        sxui_poll_events();
        sxui_render();
    }
    
    sxui_cleanup();
    printf("\nThanks for using SXUI!\n");
    return 0;
}