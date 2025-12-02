# Print Progress Feature - Technical Documentation

## Overview

The KNOMI V2 print progress system provides real-time visual feedback during printing with a sophisticated layer-based architecture. This document explains the technical implementation, PSRAM optimization, and integration points.

---

## Table of Contents

1. [Architecture](#architecture)
2. [Layer System](#layer-system)
3. [PSRAM Management](#psram-management)
4. [Progress Update Flow](#progress-update-flow)
5. [Tool Integration](#tool-integration)
6. [Temperature-Based Styling](#temperature-based-styling)
7. [Performance Optimization](#performance-optimization)
8. [Integration Guide](#integration-guide)

---

## Architecture

### Component Overview

```
┌──────────────────────────────────────────┐
│  Moonraker API (Klipper Data)            │
│  - Progress %                            │
│  - Layer count                           │
│  - ETA                                   │
│  - Tool number                           │
│  - Temperatures                          │
└──────────────┬───────────────────────────┘
               │
               ▼
┌──────────────────────────────────────────┐
│  lv_print_progress_update.cpp            │
│  - update_print_progress()               │
│  - Layer management                      │
│  - PSRAM GIF handling                    │
└──────────────┬───────────────────────────┘
               │
               ▼
┌──────────────────────────────────────────┐
│  LVGL Display (240x240)                  │
│  Layer 0: Animated GIF (PSRAM)           │
│  Layer 1: Static PNG Ring                │
│  Layer 2: Progress Arc (covers ring)     │
│  Layer 3: Text Labels                    │
└──────────────────────────────────────────┘
```

### Key Files

| File | Purpose | Lines |
|------|---------|-------|
| `src/ui_overlay/lv_print_progress_update.cpp` | Main implementation | ~400 |
| `src/ui_overlay/lv_progress_helper.cpp` | Helper functions | ~20 |
| `src/ui/images/ui_img_progress_ring.c` | Colorful ring PNG | ~1500 |
| `data/gifs/print_progress_bg.gif` | Background animation | 240x240 |
| `src/ui/screens/ui_ScreenPrinting.c` | SquareLine UI definition | ~300 |

---

## Layer System

### Layer Architecture

The print progress screen uses a **4-layer compositing system**:

#### Layer 0: Background GIF (Animated)

**Source:** `data/gifs/print_progress_bg.gif` or fallback C-array

**Properties:**
- **Size:** 240x240 pixels
- **Storage:** PSRAM (8MB available)
- **Animation:** Continuous loop (glow effect)
- **Z-Index:** 0 (furthest back)

**Code:**

```cpp
// From lv_print_progress_update.cpp:92-110
ui_bg_gif = lv_gif_create(ui_ScreenPrinting);
lv_obj_clear_flag(ui_bg_gif, LV_OBJ_FLAG_CLICKABLE);
lv_obj_set_size(ui_bg_gif, 240, 240);
lv_obj_center(ui_bg_gif);
lv_obj_set_style_opa(ui_bg_gif, LV_OPA_COVER, 0);
lv_obj_move_background(ui_bg_gif);  // Push to back
lv_obj_move_to_index(ui_bg_gif, 0); // Z-index 0

// Set source from PSRAM descriptor
lv_gif_set_src(ui_bg_gif, &progress_gif_descriptor);
```

---

#### Layer 1: Static Ring (Colorful PNG)

**Source:** `src/ui/images/ui_img_progress_ring.c`

**Properties:**
- **Format:** PNG (RGB565 with alpha)
- **Size:** 240x240 pixels (~50KB compressed)
- **Purpose:** Decorative ring revealed by progress arc
- **Z-Index:** 1 (above GIF, below arc)

**Code:**

```cpp
// Create static ring
ui_bg_ring_img = lv_img_create(ui_ScreenPrinting);
lv_img_set_src(ui_bg_ring_img, &ui_img_progress_ring);
lv_obj_set_size(ui_bg_ring_img, 240, 240);
lv_obj_center(ui_bg_ring_img);
lv_obj_set_style_opa(ui_bg_ring_img, LV_OPA_COVER, 0);
lv_obj_move_to_index(ui_bg_ring_img, 1);  // Above GIF
```

**Critical:** Ring is **static** (not animated). The visual effect comes from the arc **revealing** it.

---

#### Layer 2: Progress Arc (Cover)

**Widget:** LVGL Arc (`ui_arc_progress_cover`)

**Purpose:** Black arc that **covers** the colorful ring. As progress increases, arc shrinks, revealing ring.

**Properties:**
- **Range:** 0-100 (percentage)
- **Color:** Black (matches screen background)
- **Width:** 12px
- **Z-Index:** 2 (above ring)

**Code:**

```cpp
// Update progress (0-100%)
lv_arc_set_value(ui_arc_progress_cover, progress_percent);

// Arc styling (from SquareLine export)
lv_obj_set_style_arc_color(ui_arc_progress_cover,
                            lv_color_hex(0x000000),  // Black
                            LV_PART_INDICATOR);
lv_obj_set_style_arc_width(ui_arc_progress_cover, 12, LV_PART_INDICATOR);
```

**Visual Effect:**

```
Progress = 0%   → Arc fully covers ring (360°) → Ring hidden
Progress = 50%  → Arc covers 180° → Half ring visible
Progress = 100% → Arc minimal (only start/end) → Full ring visible
```

---

#### Layer 3: Text Labels (Foreground)

**Labels:**
- `ui_label_printing_progress` - "42%" (center, large)
- `ui_label_progress_eta` - "2h 15m" (below percentage)
- `ui_label_progress_layer` - "Layer 42/120" (top)
- `ui_label_tool_indicator` - "T0" (center glow circle)

**Properties:**
- **Font:** InterSemiBold28 or InterSemiBold16
- **Color:** White (#FFFFFF) or tool-specific
- **Opacity:** Fully opaque (LV_OPA_COVER)
- **Z-Index:** 3 (topmost)

**Code:**

```cpp
// Update labels
lv_label_set_text_fmt(ui_label_printing_progress, "%d%%", progress);
lv_label_set_text_fmt(ui_label_progress_layer, "%d/%d", current, total);

// ETA formatting
int hours = eta_seconds / 3600;
int minutes = (eta_seconds % 3600) / 60;
lv_label_set_text_fmt(ui_label_progress_eta, "%dh %dm", hours, minutes);
```

---

## PSRAM Management

### Why PSRAM?

**ESP32-S3-R8 Memory:**
- **Internal SRAM:** 512KB (shared with WiFi, LVGL, heap)
- **PSRAM:** 8MB (external, slower but huge)

**Problem:** Large GIF files (100-500KB) don't fit in internal RAM.

**Solution:** Load GIF into PSRAM, keep LVGL descriptor in RAM.

---

### GIF Loading Pipeline

#### Step 1: Allocate PSRAM

```cpp
// From lv_print_progress_update.cpp:66-69
static uint8_t * progress_gif_data = nullptr;  // PSRAM pointer
static size_t progress_gif_size = 0;
static lv_img_dsc_t progress_gif_descriptor;  // RAM descriptor
```

#### Step 2: Load from Filesystem

```cpp
void load_progress_gif_to_psram(void) {
    // Open file from LittleFS
    File f = LittleFS.open("/gifs/print_progress_bg.gif", "r");
    if (!f) {
        Serial.println("[Progress] ERROR: GIF file not found!");
        return;
    }

    // Get file size
    size_t size = f.size();

    // Allocate PSRAM (ps_malloc = PSRAM malloc)
    progress_gif_data = (uint8_t*)ps_malloc(size);
    if (!progress_gif_data) {
        Serial.println("[Progress] ERROR: PSRAM allocation failed!");
        f.close();
        return;
    }

    // Read entire file into PSRAM
    size_t bytes_read = f.read(progress_gif_data, size);
    f.close();

    if (bytes_read != size) {
        Serial.println("[Progress] ERROR: Incomplete read!");
        free(progress_gif_data);
        progress_gif_data = nullptr;
        return;
    }

    progress_gif_size = size;
    Serial.printf("[Progress] Loaded GIF: %u bytes into PSRAM\n", size);
}
```

#### Step 3: Create LVGL Descriptor

```cpp
void create_gif_descriptor(void) {
    // Populate LVGL image descriptor
    progress_gif_descriptor.header.always_zero = 0;
    progress_gif_descriptor.header.w = 240;
    progress_gif_descriptor.header.h = 240;
    progress_gif_descriptor.data_size = progress_gif_size;
    progress_gif_descriptor.header.cf = LV_IMG_CF_RAW_ALPHA;
    progress_gif_descriptor.data = progress_gif_data;  // Point to PSRAM
}
```

#### Step 4: Create LVGL GIF Object

```cpp
void show_ui_bg_gif(void) {
    if (!progress_gif_data) {
        load_progress_gif_to_psram();
        create_gif_descriptor();
    }

    ui_bg_gif = lv_gif_create(ui_ScreenPrinting);
    lv_gif_set_src(ui_bg_gif, &progress_gif_descriptor);  // Use PSRAM data
    lv_obj_set_size(ui_bg_gif, 240, 240);
    lv_obj_center(ui_bg_gif);
    lv_obj_move_background(ui_bg_gif);

    Serial.println("[Progress] GIF displayed from PSRAM");
}
```

---

### Memory Optimization

**Key Insight:** GIF data stays in PSRAM **permanently** after first load.

**Benefits:**
1. **Fast recreation:** Switching back to print screen is instant
2. **No re-read:** Filesystem only accessed once
3. **Low RAM usage:** Only small descriptor in RAM (~20 bytes)

**Cleanup (optional):**

```cpp
void free_progress_gif_psram(void) {
    if (progress_gif_data) {
        free(progress_gif_data);  // Free PSRAM
        progress_gif_data = nullptr;
        progress_gif_size = 0;
        Serial.println("[Progress] PSRAM freed");
    }
}
```

**Recommendation:** Don't free unless critically low on PSRAM. Keeping data allows instant mode switching.

---

## Progress Update Flow

### Main Function Signature

```cpp
void update_print_progress(int progress_percent,
                           int current_layer,
                           int total_layers,
                           int eta_seconds,
                           int tool_number,
                           int tool_temp);
```

**Parameters:**
- `progress_percent` - 0-100 (print completion)
- `current_layer` - Current layer number
- `total_layers` - Total layer count
- `eta_seconds` - Estimated time remaining
- `tool_number` - Active tool (0-5)
- `tool_temp` - Current extruder temperature (°C)

---

### Update Logic

```cpp
void update_print_progress(int progress_percent,
                           int current_layer,
                           int total_layers,
                           int eta_seconds,
                           int tool_number,
                           int tool_temp) {
    // 1. Ensure GIF and ring are visible
    show_ui_bg_gif();    // Create/show background GIF
    show_ui_bg_ring();   // Create/show colorful ring

    // 2. Update progress arc (covers ring)
    lv_arc_set_value(ui_arc_progress_cover, progress_percent);

    // 3. Update percentage label
    lv_label_set_text_fmt(ui_label_printing_progress, "%d%%", progress_percent);

    // 4. Update layer count
    lv_label_set_text_fmt(ui_label_progress_layer,
                          "Layer %d/%d",
                          current_layer,
                          total_layers);

    // 5. Format and update ETA
    int hours = eta_seconds / 3600;
    int minutes = (eta_seconds % 3600) / 60;

    if (hours > 0) {
        lv_label_set_text_fmt(ui_label_progress_eta, "%dh %dm", hours, minutes);
    } else {
        lv_label_set_text_fmt(ui_label_progress_eta, "%dm", minutes);
    }

    // 6. Update tool indicator with color
    update_tool_indicator(tool_number, tool_temp);
}
```

---

### Integration Example

**From Moonraker API:**

```cpp
// In moonraker.cpp or main loop
if (moonraker.state == PRINTING) {
    update_print_progress(
        (int)(moonraker.print_stats.progress * 100),  // 0.42 → 42%
        moonraker.print_stats.info.current_layer,     // e.g., 42
        moonraker.print_stats.info.total_layer_count, // e.g., 120
        moonraker.print_stats.eta,                    // seconds remaining
        moonraker.toolhead.active_extruder,           // 0-5
        (int)moonraker.extruder.temperature           // current temp
    );
}
```

---

## Tool Integration

### Tool Detection

**Function:** `detect_my_tool_number()` (from `src/knomi.h`)

**Logic:**

```cpp
int detect_my_tool_number(void) {
    // Get hostname (e.g., "knomi-t0", "knomi-t1")
    String hostname = WiFi.getHostname();

    // Extract tool number from hostname
    if (hostname.startsWith("knomi-t")) {
        int tool = hostname.substring(7).toInt();  // Extract after "knomi-t"
        return (tool >= 0 && tool <= 5) ? tool : 0;
    }

    return 0;  // Default to T0
}
```

**Hostname Format:**
- `knomi-t0.local` → Tool 0
- `knomi-t1.local` → Tool 1
- ...
- `knomi-t5.local` → Tool 5

---

### Tool Indicator (Glow Circle)

**Components:**
- `ui_tool_bg_circle` - Circular background with glow effect
- `ui_label_tool_indicator` - "T0" - "T5" text

**Update Function:**

```cpp
void update_tool_indicator(int tool, int temp) {
    // Get tool-specific color
    lv_color_t color = get_tool_color(tool);

    // Update circle background
    lv_obj_set_style_bg_color(ui_tool_bg_circle, color, 0);
    lv_obj_set_style_bg_opa(ui_tool_bg_circle, LV_OPA_70, 0);

    // Add glow effect (shadow)
    lv_obj_set_style_shadow_color(ui_tool_bg_circle, color, 0);
    lv_obj_set_style_shadow_width(ui_tool_bg_circle, 30, 0);
    lv_obj_set_style_shadow_opa(ui_tool_bg_circle, LV_OPA_70, 0);

    // Update label text
    lv_label_set_text_fmt(ui_label_tool_indicator, "T%d", tool);
    lv_obj_set_style_text_color(ui_label_tool_indicator, color, 0);

    // Optionally adjust color based on temperature
    if (temp > 200) {
        // Hot extruder - red tint
        lv_obj_set_style_shadow_color(ui_tool_bg_circle,
                                       lv_color_hex(0xFF0000), 0);
    }
}
```

---

## Temperature-Based Styling

### Color Gradient

**Implementation:** Dynamic color based on extruder temperature

```cpp
lv_color_t get_temp_color(int temp) {
    if (temp < 150) {
        // Cool - Cyan/Blue
        return lv_color_hex(0x4ECDC4);
    }
    else if (temp < 200) {
        // Warm - Orange
        return lv_color_hex(0xFF6B35);
    }
    else {
        // Hot - Red
        return lv_color_hex(0xFF0000);
    }
}
```

**Application:**

```cpp
// Apply temperature-based color to glow
lv_color_t temp_color = get_temp_color(tool_temp);
lv_obj_set_style_shadow_color(ui_tool_bg_circle, temp_color, 0);
```

---

## Performance Optimization

### 1. Minimize Redraws

**Problem:** Updating labels every frame (60fps) wastes CPU.

**Solution:** Only update when values change.

```cpp
static int last_progress = -1;

void update_print_progress(int progress_percent, ...) {
    // Only update if changed
    if (progress_percent != last_progress) {
        lv_arc_set_value(ui_arc_progress_cover, progress_percent);
        lv_label_set_text_fmt(ui_label_printing_progress, "%d%%", progress_percent);
        last_progress = progress_percent;
    }
}
```

---

### 2. Lazy GIF Loading

**Strategy:** Load GIF only when entering print screen.

```cpp
void show_ui_bg_gif(void) {
    // Return early if already exists
    if (ui_bg_gif) {
        lv_obj_clear_flag(ui_bg_gif, LV_OBJ_FLAG_HIDDEN);
        return;  // Fast path - no filesystem access
    }

    // First time: load from PSRAM (data already loaded at boot)
    if (!progress_gif_data) {
        load_progress_gif_to_psram();  // Only on first call
    }

    // Create widget (fast, no disk I/O)
    ui_bg_gif = lv_gif_create(ui_ScreenPrinting);
    lv_gif_set_src(ui_bg_gif, &progress_gif_descriptor);
    ...
}
```

---

### 3. Delete on Mode Switch

**Strategy:** Free CPU resources when leaving print screen.

```cpp
void delete_ui_bg_gif(void) {
    if (ui_bg_gif) {
        lv_obj_del(ui_bg_gif);  // Free LVGL object
        ui_bg_gif = nullptr;
        // NOTE: Data stays in PSRAM for fast recreation
    }
}
```

**When to delete:**
- User switches to Temp screen
- Print finishes (goes to Idle screen)
- Display enters sleep mode

**When NOT to delete:**
- During print (obviously)
- Brief screen switches (< 1 second)

---

### 4. Double Buffering

**Configuration:** `include/lv_conf.h`

```c
#define LV_COLOR_DEPTH 16          // RGB565
#define LV_VDB_SIZE    (240 * 40)  // Partial buffer (40 lines)

// Double buffering for smooth rendering
#define LV_VDB_DOUBLE  1
```

**Impact:**
- **Single buffer:** 40-50fps, visible tearing
- **Double buffer:** 55-60fps, smooth rendering

---

## Integration Guide

### Step 1: Enable Print Progress Screen

**File:** `src/ui_overlay/lv_moonraker_change_screen.cpp`

```cpp
// In state machine update function
if (moonraker.state == "printing") {
    // Switch to printing screen
    _ui_screen_change(&ui_ScreenPrinting, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, NULL);

    // Update progress
    update_print_progress(
        moonraker.progress_percent,
        moonraker.current_layer,
        moonraker.total_layers,
        moonraker.eta_seconds,
        moonraker.active_tool,
        moonraker.tool_temp
    );
}
```

---

### Step 2: Moonraker Data Mapping

**Required Fields:**

| Moonraker Field | Type | Example | Purpose |
|----------------|------|---------|---------|
| `print_stats.state` | String | "printing" | Trigger screen switch |
| `virtual_sdcard.progress` | Float | 0.42 | Progress percentage (0-1) |
| `print_stats.info.current_layer` | Int | 42 | Current layer |
| `print_stats.info.total_layer_count` | Int | 120 | Total layers |
| `print_stats.eta` | Int | 7800 | Seconds remaining |
| `toolhead.active_extruder` | Int | 0 | Tool number (0-5) |
| `extruder.temperature` | Float | 210.5 | Extruder temp |

---

### Step 3: Poll Moonraker API

**Example:**

```cpp
void update_moonraker_data(void) {
    // HTTP GET to Moonraker API
    String url = "http://" + MOONRAKER_HOST + "/printer/objects/query?";
    url += "print_stats&virtual_sdcard&toolhead&extruder";

    HTTPClient http;
    http.begin(url);
    int code = http.GET();

    if (code == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(4096);
        deserializeJson(doc, payload);

        // Extract data
        moonraker.state = doc["result"]["status"]["print_stats"]["state"].as<String>();
        moonraker.progress = doc["result"]["status"]["virtual_sdcard"]["progress"].as<float>();
        moonraker.current_layer = doc["result"]["status"]["print_stats"]["info"]["current_layer"];
        moonraker.total_layers = doc["result"]["status"]["print_stats"]["info"]["total_layer_count"];
        moonraker.eta = doc["result"]["status"]["print_stats"]["eta"];
        moonraker.active_tool = doc["result"]["status"]["toolhead"]["active_extruder"];
        moonraker.tool_temp = doc["result"]["status"]["extruder"]["temperature"];
    }

    http.end();
}
```

---

### Step 4: Create Print Progress GIF

**Requirements:**
- **Size:** 240x240 pixels
- **Format:** GIF (animated)
- **Theme:** Glow/pulse effect
- **File:** `data/gifs/print_progress_bg.gif`
- **Recommended:** <200KB, 15-30fps

**Tools:**
- [Pandamation](https://pandamation.techjeeper.com) - KNOMI-specific generator
- [EZGIF](https://ezgif.com/optimize) - Optimize file size
- [Photoshop](https://www.adobe.com/products/photoshop.html) - Professional creation

**Upload:**

```bash
pio run -e knomiv2 -t uploadfs
```

---

## Advanced Customization

### Custom Progress Ring

**Replace:** `src/ui/images/ui_img_progress_ring.c`

**Steps:**
1. Create 240x240 PNG with transparent center
2. Convert to C array: [LVGL Image Converter](https://lvgl.io/tools/imageconverter)
   - **Format:** C array
   - **Color:** RGB565
   - **Alpha:** Yes
3. Replace `ui_img_progress_ring.c`
4. Rebuild firmware

---

### Alternative: Gradient Arc

**Instead of static ring, use gradient arc:**

```cpp
// Set arc to use gradient instead of solid color
lv_obj_set_style_arc_color(ui_arc_progress_cover,
                            lv_color_hex(0x00FF00),  // Green
                            LV_PART_INDICATOR);

// Add gradient (LVGL 8.3+)
static lv_grad_dsc_t grad;
grad.dir = LV_GRAD_DIR_HOR;
grad.stops[0].color = lv_color_hex(0x00FF00);  // Green (start)
grad.stops[1].color = lv_color_hex(0xFF0000);  // Red (end)
grad.stops_count = 2;

lv_obj_set_style_arc_grad(ui_arc_progress_cover, &grad, LV_PART_INDICATOR);
```

---

## Resources

- [LVGL Arc Widget](https://docs.lvgl.io/8.3/widgets/arc.html)
- [LVGL GIF Support](https://docs.lvgl.io/8.3/libs/gif.html)
- [ESP32 PSRAM Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/external-ram.html)
- [Moonraker API Docs](https://moonraker.readthedocs.io/en/latest/web_api/)

---

## See Also

- [UI_CUSTOMIZATION.md](UI_CUSTOMIZATION.md) - LVGL widget customization
- [HYBRID_DISPLAY.md](HYBRID_DISPLAY.md) - State machine & screen switching
- [DISPLAY_SLEEP_IMPLEMENTATION.md](DISPLAY_SLEEP_IMPLEMENTATION.md) - Power management

---

**Last Updated:** December 2, 2025
**Version:** 1.0.0
