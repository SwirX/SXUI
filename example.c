#include "sxui.h"
#include <stdio.h>

// Simple counter application demonstrating basic SXUI usage

static int counter = 0;
static UIElement* counter_label = NULL;

void update_counter_display() {
    char buf[32];
    sprintf(buf, "Count: %d", counter);
    sxui_set_text(counter_label, buf);
}

void on_increment(void* element) {
    counter++;
    update_counter_display();
    printf("Counter incremented to %d\n", counter);
}

void on_decrement(void* element) {
    counter--;
    update_counter_display();
    printf("Counter decremented to %d\n", counter);
}

void on_reset(void* element) {
    counter = 0;
    update_counter_display();
    printf("Counter reset!\n");
}

void on_input_submit(void* element, const char* text) {
    printf("You typed: %s\n", text);
}

void on_checkbox_changed(void* element, float value) {
    printf("Checkbox is now: %s\n", value > 0.5f ? "CHECKED" : "UNCHECKED");
}

void on_slider_changed(void* element, float value) {
    printf("Slider value: %.2f\n", value);
}

int main() {
    // Initialize the UI system
    sxui_init("Simple Counter App", 600, 500, 0x4A90E2FF);
    
    printf("╔════════════════════════════════════╗\n");
    printf("║  Simple Counter App with SXUI      ║\n");
    printf("║  Demonstrating basic features      ║\n");
    printf("╚════════════════════════════════════╝\n\n");
    
    // Create a main container frame
    UIElement* main_frame = sxui_frame(NULL, 50, 50, 500, 400, 0);
    sxui_frame_set_padding(main_frame, 20);
    sxui_frame_set_spacing(main_frame, 15);
    sxui_frame_set_default_child_size(main_frame, 460, 50);
    
    // Title
    UIElement* title = sxui_label(main_frame, "Counter Demo");
    sxui_set_size(title, 460, 40);
    
    // Counter display
    counter_label = sxui_label(main_frame, "Count: 0");
    sxui_set_size(counter_label, 460, 60);
    
    // Buttons
    sxui_button(main_frame, "Increment", on_increment);
    sxui_button(main_frame, "Decrement", on_decrement);
    sxui_button(main_frame, "Reset", on_reset);
    
    // Text input demo
    UIElement* input = sxui_input(main_frame, "Type and press Enter...", 0);
    sxui_on_submit(input, on_input_submit);
    
    // Checkbox demo
    UIElement* checkbox = sxui_checkbox(main_frame, "Enable something");
    sxui_on_value_changed(checkbox, on_checkbox_changed);
    
    // Slider demo
    UIElement* slider = sxui_slider(main_frame, 0.5f);
    sxui_set_size(slider, 460, 30);
    sxui_on_value_changed(slider, on_slider_changed);
    
    printf("UI ready! Try clicking buttons, typing, and moving the slider.\n");
    printf("Press Ctrl+C to exit.\n\n");
    
    // Main loop
    while (!sxui_should_quit()) {
        sxui_poll_events();
        sxui_render();
    }
    
    // Cleanup
    sxui_cleanup();
    printf("\nGoodbye!\n");
    return 0;
}