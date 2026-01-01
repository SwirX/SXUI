# Changelog

All notable changes to the SXUI library will be documented in this file.

## [v1.2.0] - 2026-01-01

### Added
- **Advanced Input System**:
  - New mouse state tracking: `sxui_get_mouse_pos`, `sxui_get_mouse_x`, `sxui_get_mouse_y`.
  - Mouse button state detection: `sxui_is_mouse_button_down`, `sxui_is_mouse_button_pressed`, `sxui_is_mouse_button_released`.
  - Comprehensive keyboard state tracking: `sxui_is_key_down`, `sxui_is_key_pressed`, `sxui_is_key_released`.
  - Human-readable key names via `sxui_get_scancode_name`.
  - Strict Click Semantics: `sxui_on_mouse_click` callback triggers only on press-and-release on the same element without dragging.
- **System Utilities**:
  - Window management: `sxui_get_window_size`, `sxui_get_window_width`, `sxui_get_window_height`.
  - Application control: `sxui_quit` and `sxui_should_quit`.
- **API Enhancements**:
  - `sxui_get_width` and `sxui_get_height` getters for UI elements.
  - `sxui_get_parent` and `sxui_get_last_element` helpers.
- **Showcase Overhaul**:
  - New **Input Lab** for real-time event verification.
  - New **Paint Lab** with smooth line interpolation.
  - Enhanced **Canvas Lab** with animation selection and dropdown fixes.
  - Enhanced **Dashboard** with global property controls and theme synchronization.

### Fixed
- **Dropdowns**: Resolved recursive hit detection bugs when dropdowns expanded outside parent frame boundaries.
- **Showcase**: Fixed theme seeds not synchronizing when switching between light and dark modes.
- **Showcase**: Fixed text overflow in Input Lab.
- **Redundancy**: Removed duplicate `SX_COLOR_NONE` and `sxui_get_width` definitions.

## [v1.1.0] - 2025-12-31 (Commit: bf11ce3d)

### Added
- **New UI Elements**: Introduced `UI_DROPDOWN` and `UI_CANVAS`.
- **Visual Effects**: 
  - Implementation of `UIGradient`, `UIOutline`, and `UIRoundedCorners`.
  - Functions: `sxui_set_gradient`, `sxui_set_outline`, `sxui_set_rounded_corners`.
- **Canvas Drawing**: Functions for draw_pixel, draw_line, draw_rect, and draw_circle.
- **Frame Management**: `sxui_frame_get_child_count`, `sxui_frame_get_child`, and `sxui_frame_add_child`.
- **Page Management**: Navigation system with `sxui_switch_page`, `sxui_page_next`, and `sxui_page_previous`.
- **Event System**: Added `sxui_on_dropdown_changed` and `sxui_set_file_drop_callback`.

## [v1.0.1] - 2025-12-31 (Commit: 6b8da041)

### Added
- Value getters for checkboxes and sliders.
### Fixed
- Fixed color synchronization with global theme.
- Fixed slider interaction bugs.

## [v1.0.0] - 2025-12-28 (Commit: 7e1dfc21)

### Added
- Initial release of SXUI core widgets (Buttons, Labels, Inputs, Frames).
- Basic layout engine (Vertical, Horizontal, Grid).
- Basic event system.
