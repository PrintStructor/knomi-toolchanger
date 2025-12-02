# UI Customization Guide - KNOMI V2

## Overview

This guide explains how to customize the KNOMI V2 user interface, including LVGL widget modification, GIF creation, and layer architecture.

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [LVGL Layer System](#lvgl-layer-system)
3. [UI Components](#ui-components)
4. [Custom GIF Integration](#custom-gif-integration)
5. [Widget Customization](#widget-customization)
6. [Color Themes](#color-themes)
7. [Performance Optimization](#performance-optimization)

---

## Architecture Overview

The KNOMI V2 UI uses a **layered architecture** combining static/animated backgrounds with transparent LVGL overlays:

```
┌─────────────────────────────────────┐
│  LVGL Labels (Transparent)          │  ← Top layer: Text, progress
├─────────────────────────────────────┤
│  LVGL Arcs/Charts (Semi-transparent)│  ← Middle: Interactive widgets
├─────────────────────────────────────┤
│  Static PNG Ring (Colorful)         │  ← Background decoration
├─────────────────────────────────────┤
│  Animated GIF (Tool/State specific) │  ← Bottom: Dynamic background
└─────────────────────────────────────┘
```

### Key Files

| Path | Purpose |
|------|---------|
| `src/ui/` | LVGL UI definitions (SquareLine Studio exports) |
| `src/ui_overlay/` | Custom logic & overlays (C++ implementations) |
| `src/gif/` | Built-in GIF animations (C arrays) |
| `data/gifs/` | Tool-specific GIF files (filesystem, 240x240px) |
| `src/ui/images/` | Static assets (PNG ring, icons) |

---

## LVGL Layer System

### Layer Order (Z-Index)

LVGL objects are layered using `lv_obj_move_background()` and `lv_obj_move_foreground()`:

```cpp
// Example from lv_print_progress_update.cpp:100-110
ui_bg_gif = lv_gif_create(ui_ScreenPrinting);
lv_obj_move_background(ui_bg_gif);  // Push to back
lv_obj_move_to_index(ui_bg_gif, 0); // Ensure furthest back

ui_bg_ring_img = lv_img_create(ui_ScreenPrinting);
lv_img_set_src(ui_bg_ring_img, &ui_img_progress_ring);
lv_obj_move_to_index(ui_bg_ring_img, 1); // Above GIF, below text
```

**Critical:** Labels must be created **last** or moved to front:

```cpp
lv_obj_move_foreground(ui_label_printing_progress);
```

---

## UI Components

### 1. Print Progress Screen

**File:** `src/ui_overlay/lv_print_progress_update.cpp`

**Components:**
- **Background GIF** (`ui_bg_gif`) - Animated glow from PSRAM
- **Progress Ring** (`ui_bg_ring_img`) - Static colorful PNG (240x240)
- **Progress Arc** (`ui_arc_progress_cover`) - Black arc covering ring
- **Labels:**
  - `ui_label_printing_progress` - Percentage (e.g., "42%")
  - `ui_label_progress_eta` - Time remaining
  - `ui_label_progress_layer` - Layer count
  - `ui_label_tool_indicator` - Tool number (T0-T5)

**Update Function:**

```cpp
void update_print_progress(int progress_percent,
                           int current_layer,
                           int total_layers,
                           int eta_seconds,
                           int tool_number,
                           int tool_temp);
```

**Usage Example:**

```cpp
// In moonraker.cpp or main loop:
if (printer_state == PRINTING) {
    update_print_progress(
        moonraker.progress_percent,  // 0-100
        moonraker.current_layer,     // e.g., 42
        moonraker.total_layers,      // e.g., 120
        moonraker.eta_seconds,       // time remaining
        moonraker.active_tool,       // 0-5
        moonraker.tool_temp          // current extruder temp
    );
}
```

---

### 2. Temperature Graphs

**File:** `src/ui/screens/ui_ScreenTemp.c` (SquareLine export)

**Overlay Logic:** `src/ui_overlay/lv_theme_color.cpp`

**Chart Styling:**

```cpp
// Create temperature chart
lv_obj_t * chart = lv_chart_create(parent);
lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
lv_chart_set_point_count(chart, 60);  // 60 seconds of data

// Add series
lv_chart_series_t * series_bed = lv_chart_add_series(
    chart,
    lv_color_hex(0xFF6B35),  // Orange for bed
    LV_CHART_AXIS_PRIMARY_Y
);

lv_chart_series_t * series_nozzle = lv_chart_add_series(
    chart,
    lv_color_hex(0x4ECDC4),  // Cyan for nozzle
    LV_CHART_AXIS_PRIMARY_Y
);
```

**Add Data Points:**

```cpp
lv_chart_set_next_value(chart, series_bed, bed_temp);
lv_chart_set_next_value(chart, series_nozzle, nozzle_temp);
lv_chart_refresh(chart);
```

---

### 3. Heating Screens

**Files:**
- `src/ui/screens/ui_ScreenHeatingBed.c`
- `src/ui/screens/ui_ScreenHeatingNozzle.c`

**State Machine:** `src/ui_overlay/lv_moonraker_change_screen.cpp:30-43`

```cpp
typedef enum {
    LV_MOONRAKER_STATE_IDLE = 0,
    LV_MOONRAKER_STATE_HOMING,
    LV_MOONRAKER_STATE_PROBING,
    LV_MOONRAKER_STATE_QGLING,
    LV_MOONRAKER_STATE_NOZZLE_HEATING,
    LV_MOONRAKER_STATE_BED_HEATING,
    LV_MOONRAKER_STATE_PRINTING,
} lv_screen_state_t;
```

**Heating Update:**

```cpp
// Update heating progress
lv_slider_set_value(ui_slider_heating_nozzle, current_temp, LV_ANIM_ON);
lv_label_set_text_fmt(ui_label_heating_nozzle_actual, "%d°C", current_temp);
lv_label_set_text_fmt(ui_label_heating_nozzle_target, "%d°C", target_temp);
```

---

## Custom GIF Integration

### Tool-Specific GIFs (Filesystem)

**Location:** `data/gifs/tool_0.gif` through `tool_5.gif`

**Specifications:**
- **Resolution:** 240x240 pixels
- **Format:** GIF (animated or static)
- **File Size:** <500KB (recommend <200KB)
- **Frame Rate:** 15-30fps
- **Duration:** 2-5 seconds (looping)

**How It Works:**

1. **Tool Detection** (`src/knomi.h` → `detect_my_tool_number()`):
   ```cpp
   // Detects tool number from hostname (knomi-t0, knomi-t1, etc.)
   int tool = detect_my_tool_number();  // Returns 0-5
   ```

2. **GIF Loading** (`src/fs_gif_loader.cpp`):
   ```cpp
   // Load tool-specific GIF from LittleFS
   fs_set_tool_gif(ui_img_main_gif, tool_number);
   ```

3. **Display:**
   - Idle state shows tool GIF
   - Heating/Printing states show built-in GIFs
   - Returns to tool GIF after operation

**Upload GIFs:**

```bash
# Place GIFs in data/gifs/ folder
cp my_custom_tool_0.gif data/gifs/tool_0.gif

# Upload filesystem to KNOMI
pio run -e knomiv2 -t uploadfs
```

---

### Built-in GIFs (C Arrays)

**Location:** `src/gif/*.c`

**Available GIFs:**
- `gif_homing.c` - Homing animation
- `gif_probing.c` - Bed leveling
- `gif_qgling.c` - Quad gantry leveling
- `gif_print_ok.c` - Print complete (checkmark)
- `gif_voron.c` - VORON logo (default idle)
- `gif_standby.c` - Standby animation
- `gif_print.c` - Printing animation (fallback)

**Usage:**

```cpp
extern const lv_img_dsc_t gif_homing;

lv_obj_t * gif_obj = lv_gif_create(parent);
lv_gif_set_src(gif_obj, &gif_homing);
lv_obj_center(gif_obj);
```

**Create Custom C-Array GIF:**

1. Use [LVGL Image Converter](https://lvgl.io/tools/imageconverter)
2. Settings:
   - **Output format:** C array
   - **Color format:** RGB565
   - **Compressed:** Yes (if < 50KB)
3. Save as `src/gif/gif_mycustom.c`
4. Declare in header:
   ```cpp
   extern const lv_img_dsc_t gif_mycustom;
   ```

---

## Widget Customization

### Labels

**Create:**

```cpp
lv_obj_t * label = lv_label_create(parent);
lv_label_set_text(label, "Hello KNOMI");
lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
```

**Styling:**

```cpp
// Font size (from src/ui/fonts/)
lv_obj_set_style_text_font(label, &ui_font_InterSemiBold28, 0);

// Color
lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);

// Opacity (0 = transparent, 255 = opaque)
lv_obj_set_style_text_opa(label, LV_OPA_COVER, 0);

// Alignment
lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
```

**Dynamic Text:**

```cpp
// Format string with values
lv_label_set_text_fmt(label, "T%d: %d°C", tool, temp);

// Update progress percentage
lv_label_set_text_fmt(ui_label_printing_progress, "%d%%", progress);
```

---

### Arcs (Progress Rings)

**Example:** Print progress arc (`ui_arc_progress_cover`)

```cpp
// Create arc
lv_obj_t * arc = lv_arc_create(parent);
lv_obj_set_size(arc, 200, 200);
lv_obj_center(arc);

// Configure range
lv_arc_set_range(arc, 0, 100);

// Set value (0-100%)
lv_arc_set_value(arc, 42);

// Styling
lv_obj_set_style_arc_width(arc, 10, LV_PART_MAIN);
lv_obj_set_style_arc_color(arc, lv_color_hex(0x000000), LV_PART_INDICATOR);
lv_obj_set_style_arc_width(arc, 12, LV_PART_INDICATOR);

// Make non-clickable
lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
```

**Critical for Overlay Effect:**

The progress arc **covers** the colorful ring beneath it. As progress increases, the black arc shrinks, revealing the color underneath:

```cpp
// Start: 0% (arc fully covers ring) → ring hidden
// Progress: 50% → half of ring visible
// End: 100% (arc at minimum) → full ring visible
```

---

### Spinners

**Create:**

```cpp
lv_obj_t * spinner = lv_spinner_create(parent, 1000, 60);
lv_obj_set_size(spinner, 100, 100);
lv_obj_center(spinner);
```

**Parameters:**
- `1000` = rotation speed (ms per revolution)
- `60` = arc angle (degrees)

---

## Color Themes

### Tool-Specific Colors

**File:** `src/ui_overlay/lv_theme_color.cpp`

**Color Palette:**

```cpp
// Tool 0 - Red
lv_color_hex(0xFF0000)

// Tool 1 - Green
lv_color_hex(0x00FF00)

// Tool 2 - Blue
lv_color_hex(0x0000FF)

// Tool 3 - Yellow
lv_color_hex(0xFFFF00)

// Tool 4 - Magenta
lv_color_hex(0xFF00FF)

// Tool 5 - Cyan
lv_color_hex(0x00FFFF)
```

**Apply Tool Color:**

```cpp
int tool = detect_my_tool_number();
lv_color_t tool_color = get_tool_color(tool);

lv_obj_set_style_text_color(ui_label_tool_indicator, tool_color, 0);
lv_obj_set_style_bg_color(ui_tool_bg_circle, tool_color, 0);
```

**Temperature-Based Gradient:**

From `lv_print_progress_update.cpp`:

```cpp
// Cool (< 150°C) → Blue/Cyan
if (tool_temp < 150) {
    color = lv_color_hex(0x4ECDC4);
}
// Warm (150-200°C) → Orange
else if (tool_temp < 200) {
    color = lv_color_hex(0xFF6B35);
}
// Hot (> 200°C) → Red
else {
    color = lv_color_hex(0xFF0000);
}
```

---

## Performance Optimization

### PSRAM Usage

**GIF Loading:** Large GIFs loaded into PSRAM (8MB available)

```cpp
// From lv_print_progress_update.cpp:68-69
static uint8_t * progress_gif_data = nullptr;  // PSRAM buffer
static lv_img_dsc_t progress_gif_descriptor;  // LVGL descriptor
```

**Why PSRAM?**
- **Internal RAM:** 512KB (used for LVGL rendering, stack, heap)
- **PSRAM:** 8MB (perfect for large assets like GIFs)

**Load GIF to PSRAM:**

```cpp
// Allocate PSRAM
progress_gif_data = (uint8_t*)ps_malloc(gif_size);

// Load from filesystem
File f = LittleFS.open("/gifs/print_progress_bg.gif", "r");
f.read(progress_gif_data, gif_size);
f.close();

// Create LVGL descriptor
progress_gif_descriptor.header.always_zero = 0;
progress_gif_descriptor.header.w = 240;
progress_gif_descriptor.header.h = 240;
progress_gif_descriptor.data_size = gif_size;
progress_gif_descriptor.header.cf = LV_IMG_CF_RAW_ALPHA;
progress_gif_descriptor.data = progress_gif_data;
```

---

### GIF Pause/Resume

**LVGL 8.3.7 (KNOMI V2):**

```cpp
// Pause not available in v8, use hide instead
lv_obj_add_flag(ui_bg_gif, LV_OBJ_FLAG_HIDDEN);

// Resume = restart
lv_gif_restart(ui_bg_gif);
```

**LVGL 9+ (future):**

```cpp
lv_gif_pause(ui_bg_gif);
lv_gif_resume(ui_bg_gif);
```

---

### Framerate Optimization

**Target:** 60fps @ 80MHz SPI

**Tips:**
1. **Minimize redraws:** Only update changed widgets
2. **Use buffers:** Double buffering in `lv_conf.h`
3. **Optimize GIFs:** <200KB, 15-30fps
4. **Reduce transparency:** Fully opaque backgrounds render faster
5. **Batch updates:**
   ```cpp
   lv_obj_invalidate(parent);  // Redraw parent once
   ```

**Monitor Performance:**

```cpp
lv_refr_now(NULL);  // Force immediate refresh
uint32_t fps = lv_refr_get_fps_avg();
Serial.printf("FPS: %d\n", fps);
```

---

## Advanced Examples

### Custom Overlay with GIF Background

```cpp
// 1. Load custom background GIF
lv_obj_t * bg_gif = lv_gif_create(ui_ScreenMyCustom);
lv_gif_set_src(bg_gif, &my_custom_gif);
lv_obj_set_size(bg_gif, 240, 240);
lv_obj_center(bg_gif);
lv_obj_move_background(bg_gif);

// 2. Add semi-transparent overlay
lv_obj_t * overlay = lv_obj_create(ui_ScreenMyCustom);
lv_obj_set_size(overlay, 240, 240);
lv_obj_center(overlay);
lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000), 0);
lv_obj_set_style_bg_opa(overlay, LV_OPA_50, 0);  // 50% black

// 3. Add text label
lv_obj_t * label = lv_label_create(ui_ScreenMyCustom);
lv_label_set_text(label, "Custom Screen");
lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
lv_obj_set_style_text_font(label, &ui_font_InterSemiBold28, 0);
lv_obj_align(label, LV_ALIGN_CENTER, 0, -50);
lv_obj_move_foreground(label);  // Ensure on top
```

---

### Dynamic Tool Indicator

```cpp
void update_tool_indicator(int tool, int temp) {
    // Get tool color
    lv_color_t color = get_tool_color(tool);

    // Update glow circle
    lv_obj_set_style_bg_color(ui_tool_bg_circle, color, 0);
    lv_obj_set_style_shadow_color(ui_tool_bg_circle, color, 0);
    lv_obj_set_style_shadow_width(ui_tool_bg_circle, 30, 0);
    lv_obj_set_style_shadow_opa(ui_tool_bg_circle, LV_OPA_70, 0);

    // Update label
    lv_label_set_text_fmt(ui_label_tool_indicator, "T%d", tool);
    lv_obj_set_style_text_color(ui_label_tool_indicator, color, 0);

    // Add pulsing animation
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui_tool_bg_circle);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_values(&a, LV_OPA_70, LV_OPA_30);
    lv_anim_set_time(&a, 1000);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_playback_time(&a, 1000);
    lv_anim_start(&a);
}
```

---

## Resources

### SquareLine Studio

**Website:** https://squareline.io

**Export Settings:**
- **LVGL Version:** 8.3.7 (match firmware)
- **Color Depth:** 16-bit RGB565
- **Export Path:** `src/ui/`

**Workflow:**
1. Design UI in SquareLine Studio
2. Export to `src/ui/`
3. Add custom logic in `src/ui_overlay/`
4. Build and flash

---

### LVGL Documentation

- **Main Docs:** https://docs.lvgl.io/8.3/
- **Widgets:** https://docs.lvgl.io/8.3/widgets/index.html
- **Styles:** https://docs.lvgl.io/8.3/overview/style.html
- **Animations:** https://docs.lvgl.io/8.3/overview/animation.html

---

### GIF Tools

- **LVGL Image Converter:** https://lvgl.io/tools/imageconverter
- **GIF Optimizer:** https://ezgif.com/optimize
- **GIF Resizer:** https://ezgif.com/resize
- **Pandamation:** https://pandamation.techjeeper.com (KNOMI-optimized)

---

## See Also

- [PRINT_PROGRESS_FEATURE.md](PRINT_PROGRESS_FEATURE.md) - Print progress system deep-dive
- [HYBRID_DISPLAY.md](HYBRID_DISPLAY.md) - State machine & multi-mode display
- [DISPLAY_SLEEP_IMPLEMENTATION.md](DISPLAY_SLEEP_IMPLEMENTATION.md) - Power management hooks
- [../README.md](../../README.md) - Main project documentation

---

**Last Updated:** December 2, 2025
**Version:** 1.0.0
