# Hybrid Display Architecture - State Machine System

## Overview

The KNOMI V2 uses a sophisticated **state machine** that automatically switches between different display modes based on real-time printer status from Moonraker/Klipper. This creates a "hybrid" display that adapts to what the printer is doing.

---

## Table of Contents

1. [State Machine Overview](#state-machine-overview)
2. [Display States](#display-states)
3. [State Transitions](#state-transitions)
4. [Screen Management](#screen-management)
5. [GIF Management](#gif-management)
6. [Klipper Integration](#klipper-integration)
7. [Power Management Integration](#power-management-integration)
8. [Customization Guide](#customization-guide)

---

## State Machine Overview

### Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    Moonraker API                            │
│  (printer_state, temperatures, homing_flags, print_stats)   │
└──────────────────┬──────────────────────────────────────────┘
                   │
                   ▼
┌──────────────────────────────────────────────────────────────┐
│          lv_moonraker_change_screen.cpp                      │
│                 State Machine Logic                          │
│  - Analyze Klipper flags                                     │
│  - Determine current state                                   │
│  - Trigger screen transitions                                │
└──────────────────┬───────────────────────────────────────────┘
                   │
                   ▼
┌──────────────────────────────────────────────────────────────┐
│                  Screen Rendering                            │
│  ┌────────────┬─────────────┬──────────────┬──────────────┐ │
│  │ Idle       │ Homing      │ Heating      │ Printing     │ │
│  │ (Tool GIF) │ (Homing GIF)│ (Temp Slider)│ (Progress)   │ │
│  └────────────┴─────────────┴──────────────┴──────────────┘ │
└──────────────────────────────────────────────────────────────┘
```

### Key Concept: Hybrid Display

**"Hybrid"** means the display dynamically combines:

1. **Static States** (Idle) - Tool-specific GIF from filesystem
2. **Dynamic States** (Printing) - Real-time progress overlay on animated background
3. **Busy States** (Homing) - Full-screen GIF animation
4. **Heating States** - Temperature sliders with live updates

The system **automatically** switches between these based on printer status—**no user interaction needed**.

---

## Display States

### State Enumeration

**File:** `src/ui_overlay/lv_moonraker_change_screen.cpp:30-43`

```cpp
typedef enum {
    LV_MOONRAKER_STATE_IDLE = 0,      // Idle, showing tool GIF
    LV_MOONRAKER_STATE_HOMING,        // Homing in progress
    LV_MOONRAKER_STATE_PROBING,       // Bed probing (mesh leveling)
    LV_MOONRAKER_STATE_QGLING,        // Quad Gantry Leveling
    LV_MOONRAKER_STATE_NOZZLE_HEATING,// Nozzle heating
    LV_MOONRAKER_STATE_BED_HEATING,   // Bed heating
    LV_MOONRAKER_STATE_PRINTING,      // Actively printing
    LV_SCREEN_STATE_INIT,             // Initial boot
    LV_SCREEN_STATE_IDLE,             // Fallback idle
    LV_SCREEN_PRINT_OK,               // Print complete (success)
    LV_SCREEN_PRINTED,                // Post-print cooldown
    LV_SCREEN_STATE_PLAYING,          // Legacy state
} lv_screen_state_t;
```

---

### State Descriptions

#### 1. IDLE State

**Trigger:**
- `printer_state == "ready"` or `"standby"`
- No homing/heating/printing active

**Display:**
- **Background:** Tool-specific GIF (e.g., `tool_0.gif` for T0)
- **Overlay:** None (just GIF)
- **Touch:** Swipe gestures enabled

**Code:**

```cpp
void lv_goto_idle_screen(void) {
    lv_screen_state = LV_MOONRAKER_STATE_IDLE;

    // Load tool-specific GIF from filesystem
    fs_set_tool_gif(ui_img_main_gif, detect_my_tool_number());

    // Enable clickable for gestures
    lv_obj_add_flag(ui_img_main_gif, LV_OBJ_FLAG_CLICKABLE);

    // Switch to idle screen
    _ui_screen_change(&ui_ScreenIdle, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, NULL);
}
```

**Tool Detection:**

```cpp
int detect_my_tool_number(void) {
    String hostname = WiFi.getHostname();

    // Extract tool number from hostname (knomi-t0 → 0)
    if (hostname.startsWith("knomi-t")) {
        return hostname.substring(7).toInt();  // 0-5
    }

    return 0;  // Default T0
}
```

---

#### 2. HOMING State

**Trigger:**
- `homing_axes != ""`
- Klipper reports homing in progress

**Display:**
- **Background:** Full-screen homing GIF (C-array)
- **GIF:** `gif_homing` (spinning logo animation)
- **Duration:** Until homing complete

**Code:**

```cpp
void lv_goto_busy_screen(lv_obj_t * screen,
                         lv_screen_state_t state,
                         const lv_img_dsc_t * gif) {
    lv_screen_state = state;

    // Set GIF source to built-in C-array
    lv_gif_set_src(ui_img_main_gif, gif);
    lv_obj_clear_flag(ui_img_main_gif, LV_OBJ_FLAG_CLICKABLE);

    // Switch screen with animation
    _ui_screen_change(&screen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 500, 0, NULL);
}

// Usage:
lv_goto_busy_screen(ui_ScreenBusy, LV_MOONRAKER_STATE_HOMING, &gif_homing);
```

**GIF Source:** `src/gif/gif_homing.c` (~800KB, embedded in firmware)

---

#### 3. PROBING State

**Trigger:**
- `bed_mesh.probing == true`
- Or custom Klipper variable: `probe_active`

**Display:**
- **GIF:** `gif_probing` (bed mesh visualization)
- **Overlay:** None

**Code:**

```cpp
if (klipper_vars.probe_active) {
    lv_goto_busy_screen(ui_ScreenBusy, LV_MOONRAKER_STATE_PROBING, &gif_probing);
}
```

---

#### 4. QGLING State (Quad Gantry Leveling)

**Trigger:**
- Custom Klipper variable: `qgl_active`

**Display:**
- **GIF:** `gif_qgling` (quad gantry animation)

**Klipper Macro:**

```gcode
[gcode_macro QUAD_GANTRY_LEVEL]
rename_existing: _QUAD_GANTRY_LEVEL
gcode:
    SET_GCODE_VARIABLE MACRO=qgl_status VARIABLE=active VALUE=1
    _QUAD_GANTRY_LEVEL
    SET_GCODE_VARIABLE MACRO=qgl_status VARIABLE=active VALUE=0

[gcode_macro qgl_status]
variable_active: 0
gcode:
```

**Code:**

```cpp
if (klipper_vars.qgl_active) {
    lv_goto_busy_screen(ui_ScreenBusy, LV_MOONRAKER_STATE_QGLING, &gif_qgling);
}
```

---

#### 5. HEATING States

**Triggers:**

**Nozzle Heating:**
- `extruder.temperature < extruder.target - TEMPERATURE_ERROR_RANGE`
- `extruder.target > 0`

**Bed Heating:**
- `heater_bed.temperature < heater_bed.target - TEMPERATURE_ERROR_RANGE`
- `heater_bed.target > 0`

**Display:**

**Nozzle Heating Screen:**
- **Slider:** Visual temperature bar
- **Labels:**
  - Actual temp: "210°C"
  - Target temp: "240°C"
- **Color:** Temperature-based gradient

**Bed Heating Screen:**
- Same layout as nozzle

**Code:**

```cpp
// Check nozzle heating
int nozzle_temp = (int)moonraker.extruder.temperature;
int nozzle_target = (int)moonraker.extruder.target;

if (nozzle_target > 0 && nozzle_temp < (nozzle_target - TEMPERATURE_ERROR_RANGE)) {
    // Switch to nozzle heating screen
    _ui_screen_change(&ui_ScreenHeatingNozzle, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, NULL);

    // Update slider and labels
    lv_slider_set_value(ui_slider_heating_nozzle, nozzle_temp, LV_ANIM_ON);
    lv_label_set_text_fmt(ui_label_heating_nozzle_actual, "%d°C", nozzle_temp);
    lv_label_set_text_fmt(ui_label_heating_nozzle_target, "%d°C", nozzle_target);
}
```

**Slider Range:** 0-300°C (configurable)

---

#### 6. PRINTING State

**Trigger:**
- `print_stats.state == "printing"`

**Display:**
- **Background:** Animated GIF (print_progress_bg.gif)
- **Layer 1:** Colorful progress ring (PNG)
- **Layer 2:** Black progress arc (reveals ring)
- **Overlays:**
  - Progress percentage
  - Layer count
  - ETA
  - Tool indicator

**Code:**

```cpp
if (moonraker.state == "printing") {
    _ui_screen_change(&ui_ScreenPrinting, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, NULL);

    update_print_progress(
        (int)(moonraker.progress * 100),
        moonraker.current_layer,
        moonraker.total_layers,
        moonraker.eta,
        moonraker.active_tool,
        (int)moonraker.extruder.temperature
    );
}
```

**See:** [PRINT_PROGRESS_FEATURE.md](PRINT_PROGRESS_FEATURE.md) for details

---

#### 7. PRINT_OK State (Complete)

**Trigger:**
- `print_stats.state == "complete"`

**Display:**
- **GIF:** `gif_print_ok` (checkmark animation)
- **Recolor Filter:** Green tint
- **Duration:** 10 seconds, then return to Idle

**Code:**

```cpp
if (moonraker.state == "complete" && !print_complete_shown) {
    lv_screen_state = LV_SCREEN_PRINT_OK;

    // Apply green recolor filter
    lv_obj_set_style_img_recolor(ui_img_main_gif, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_img_recolor_opa(ui_img_main_gif, LV_OPA_50, 0);

    // Set print complete GIF
    lv_gif_set_src(ui_img_main_gif, &gif_print_ok);

    // Start 10-second timer
    print_complete_timer_active = true;
    print_complete_time = millis();
    print_complete_shown = true;
}

// In main loop:
if (print_complete_timer_active &&
    (millis() - print_complete_time > PRINT_COMPLETE_DISPLAY_TIME)) {
    // Return to idle after 10 seconds
    lv_goto_idle_screen();
    print_complete_timer_active = false;
}
```

---

## State Transitions

### Transition Priority

The state machine checks conditions in **priority order** (highest first):

```cpp
void update_screen_state(void) {
    // Priority 1: Homing (most critical)
    if (is_homing()) {
        lv_goto_busy_screen(ui_ScreenBusy, LV_MOONRAKER_STATE_HOMING, &gif_homing);
        return;
    }

    // Priority 2: Probing
    if (is_probing()) {
        lv_goto_busy_screen(ui_ScreenBusy, LV_MOONRAKER_STATE_PROBING, &gif_probing);
        return;
    }

    // Priority 3: QGL
    if (is_qgling()) {
        lv_goto_busy_screen(ui_ScreenBusy, LV_MOONRAKER_STATE_QGLING, &gif_qgling);
        return;
    }

    // Priority 4: Heating
    if (is_heating_nozzle()) {
        goto_heating_screen(NOZZLE);
        return;
    }
    if (is_heating_bed()) {
        goto_heating_screen(BED);
        return;
    }

    // Priority 5: Printing
    if (is_printing()) {
        goto_printing_screen();
        return;
    }

    // Priority 6: Print Complete
    if (is_print_complete()) {
        goto_print_complete_screen();
        return;
    }

    // Default: Idle
    lv_goto_idle_screen();
}
```

---

### Transition Animations

**Available Animations:**

```cpp
LV_SCR_LOAD_ANIM_NONE          // Instant switch
LV_SCR_LOAD_ANIM_OVER_LEFT     // Slide from right
LV_SCR_LOAD_ANIM_OVER_RIGHT    // Slide from left
LV_SCR_LOAD_ANIM_OVER_TOP      // Slide from bottom
LV_SCR_LOAD_ANIM_OVER_BOTTOM   // Slide from top
LV_SCR_LOAD_ANIM_MOVE_LEFT     // Push current screen left
LV_SCR_LOAD_ANIM_MOVE_RIGHT    // Push current screen right
LV_SCR_LOAD_ANIM_MOVE_TOP      // Push current screen up
LV_SCR_LOAD_ANIM_MOVE_BOTTOM   // Push current screen down
LV_SCR_LOAD_ANIM_FADE_ON       // Fade in (recommended)
```

**Usage:**

```cpp
_ui_screen_change(&ui_ScreenPrinting,
                  LV_SCR_LOAD_ANIM_FADE_ON,  // Animation type
                  500,                        // Duration (ms)
                  0,                          // Delay (ms)
                  NULL);                      // Callback
```

**Recommended Transitions:**

| From | To | Animation | Duration |
|------|-----|-----------|----------|
| Idle | Homing | MOVE_BOTTOM | 500ms |
| Idle | Printing | FADE_ON | 500ms |
| Heating | Idle | FADE_ON | 500ms |
| Any | Idle | FADE_ON | 500ms |

---

## Screen Management

### Screen Backup System

**Problem:** LVGL's `lv_scr_act()` is delayed—doesn't return new screen immediately after transition.

**Solution:** Manual screen tracking with backup variables.

```cpp
static lv_obj_t * ui_ScreenIdle = NULL;  // Backup of idle screen
static lv_obj_t * ui_ScreenNow = NULL;   // Current screen

void lv_goto_busy_screen(...) {
    // Backup current screen before switching
    if (ui_ScreenNow == NULL) {
        ui_ScreenNow = lv_scr_act();
    }

    // Backup idle screen
    if (ui_ScreenIdle == NULL) {
        ui_ScreenIdle = ui_ScreenNow;
    }

    // Switch to new screen
    ui_ScreenNow = screen;
    _ui_screen_change(&screen, anim, duration, 0, NULL);
}

void lv_goto_idle_screen(void) {
    // Restore backed-up idle screen
    if (ui_ScreenIdle) {
        _ui_screen_change(&ui_ScreenIdle, anim, duration, 0, NULL);
        ui_ScreenNow = ui_ScreenIdle;
    }
}
```

**Why Needed:**

```cpp
// WRONG: lv_scr_act() returns OLD screen during transition
_ui_screen_change(&ui_ScreenPrinting, ...);
if (lv_scr_act() == ui_ScreenPrinting) {  // FALSE for 500ms!
    // This won't execute immediately
}

// CORRECT: Track manually
ui_ScreenNow = ui_ScreenPrinting;
_ui_screen_change(&ui_ScreenPrinting, ...);
if (ui_ScreenNow == ui_ScreenPrinting) {  // TRUE immediately
    // This executes right away
}
```

---

### Background Color Enforcement

**Critical:** Ensure black background to avoid "white flash" during GIF loading.

```cpp
void lv_goto_busy_screen(lv_obj_t * screen, ...) {
    // CRITICAL: Always set screen background to black
    if (screen) {
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    }

    // Set GIF and switch screen
    ...
}
```

**Without this:** White screen flashes before GIF loads (looks broken).

**With this:** Smooth black background while GIF initializes.

---

## GIF Management

### Built-in GIFs (C-Arrays)

**Location:** `src/gif/*.c`

**Declare:**

```cpp
extern const lv_img_dsc_t gif_homing;
extern const lv_img_dsc_t gif_probing;
extern const lv_img_dsc_t gif_qgling;
extern const lv_img_dsc_t gif_print_ok;
```

**Usage:**

```cpp
lv_gif_set_src(ui_img_main_gif, &gif_homing);
```

**Pros:**
- ✅ Fast (compiled into firmware)
- ✅ Always available (no filesystem dependency)
- ✅ Reliable

**Cons:**
- ❌ Large firmware size (~2-3MB with GIFs)
- ❌ Can't change without reflashing

---

### Filesystem GIFs (LittleFS)

**Location:** `data/gifs/tool_0.gif` - `tool_5.gif`

**Load Function:** `fs_set_tool_gif()` from `src/fs_gif_loader.cpp`

```cpp
void fs_set_tool_gif(lv_obj_t * gif_obj, int tool_number) {
    // Build file path
    char path[32];
    snprintf(path, sizeof(path), "/gifs/tool_%d.gif", tool_number);

    // Check if file exists
    if (!LittleFS.exists(path)) {
        Serial.printf("[FS] Tool GIF not found: %s\n", path);
        // Fallback to default GIF
        lv_gif_set_src(gif_obj, &gif_voron);
        return;
    }

    // Load file
    File f = LittleFS.open(path, "r");
    size_t size = f.size();

    // Allocate PSRAM
    uint8_t * data = (uint8_t*)ps_malloc(size);
    f.read(data, size);
    f.close();

    // Create descriptor
    static lv_img_dsc_t descriptor;
    descriptor.header.always_zero = 0;
    descriptor.header.w = 240;
    descriptor.header.h = 240;
    descriptor.data_size = size;
    descriptor.header.cf = LV_IMG_CF_RAW_ALPHA;
    descriptor.data = data;

    // Set GIF source
    lv_gif_set_src(gif_obj, &descriptor);

    Serial.printf("[FS] Loaded tool %d GIF: %u bytes\n", tool_number, size);
}
```

**Pros:**
- ✅ Customizable per tool (6 different GIFs)
- ✅ Smaller firmware size
- ✅ Can update via filesystem upload

**Cons:**
- ❌ Slower initial load (filesystem read)
- ❌ Requires PSRAM
- ❌ Possible corruption if filesystem damaged

---

### Hybrid Approach (Recommended)

**Strategy:** Use **built-in GIFs** for critical states (homing, probing) and **filesystem GIFs** for idle/tool-specific.

```cpp
void set_gif_for_state(lv_screen_state_t state) {
    switch (state) {
        case LV_MOONRAKER_STATE_HOMING:
            lv_gif_set_src(ui_img_main_gif, &gif_homing);  // Built-in
            break;

        case LV_MOONRAKER_STATE_PROBING:
            lv_gif_set_src(ui_img_main_gif, &gif_probing);  // Built-in
            break;

        case LV_MOONRAKER_STATE_IDLE:
            fs_set_tool_gif(ui_img_main_gif, detect_my_tool_number());  // Filesystem
            break;

        default:
            lv_gif_set_src(ui_img_main_gif, &gif_voron);  // Fallback built-in
    }
}
```

---

## Klipper Integration

### Required Moonraker Objects

**Query:**

```http
GET /printer/objects/query?print_stats&toolhead&extruder&heater_bed&gcode_move
```

**Response Fields:**

```json
{
  "result": {
    "status": {
      "print_stats": {
        "state": "printing",           // "standby", "printing", "paused", "complete"
        "filename": "test.gcode",
        "total_duration": 1234.5,
        "print_duration": 1200.0,
        "filament_used": 1234.5,
        "info": {
          "current_layer": 42,
          "total_layer_count": 120
        }
      },
      "toolhead": {
        "homed_axes": "xyz",           // "" = not homed
        "position": [120, 120, 10, 0],
        "active_extruder": 0           // 0-5 for multi-tool
      },
      "extruder": {
        "temperature": 210.5,
        "target": 240.0,
        "power": 0.42
      },
      "heater_bed": {
        "temperature": 60.2,
        "target": 60.0
      },
      "gcode_move": {
        "homing_origin": [0, 0, 0, 0],
        "gcode_position": [100, 100, 5, 0]
      }
    }
  }
}
```

---

### Custom Klipper Variables

**For advanced state detection (probing, QGL), define custom variables:**

**File:** `printer.cfg`

```ini
[gcode_macro probe_status]
variable_active: 0
gcode:

[gcode_macro qgl_status]
variable_active: 0
gcode:

[gcode_macro BED_MESH_CALIBRATE]
rename_existing: _BED_MESH_CALIBRATE
gcode:
    SET_GCODE_VARIABLE MACRO=probe_status VARIABLE=active VALUE=1
    KNOMI_WAKE  # Wake display before probing
    _BED_MESH_CALIBRATE {rawparams}
    SET_GCODE_VARIABLE MACRO=probe_status VARIABLE=active VALUE=0
```

**Query:**

```http
GET /printer/objects/query?gcode_macro probe_status&gcode_macro qgl_status
```

**KNOMI Check:**

```cpp
bool is_probing() {
    return klipper_vars["gcode_macro probe_status"]["active"] == 1;
}
```

---

## Power Management Integration

### Display Sleep Hooks

The hybrid display system integrates with [display sleep](DISPLAY_SLEEP_IMPLEMENTATION.md):

**Auto-Wake Triggers:**

```cpp
// Wake display on state changes
void update_screen_state(void) {
    lv_screen_state_t new_state = determine_state();

    if (new_state != lv_screen_state) {
        // State changed - wake display if sleeping
        display_sleep_wake();  // From display_sleep.cpp

        // Then switch screen
        switch_to_state(new_state);
    }
}
```

**Sleep Prevention:**

```cpp
// Don't sleep during critical operations
bool should_allow_sleep(void) {
    if (lv_screen_state == LV_MOONRAKER_STATE_HOMING) return false;
    if (lv_screen_state == LV_MOONRAKER_STATE_PROBING) return false;
    if (lv_screen_state == LV_MOONRAKER_STATE_PRINTING) return false;

    return true;  // Allow sleep in idle/heating states
}
```

---

## Customization Guide

### Add New State

**Example:** Add "Bed Mesh Visualization" state

#### Step 1: Add Enum

```cpp
// In lv_moonraker_change_screen.cpp
typedef enum {
    // ... existing states ...
    LV_MOONRAKER_STATE_BED_MESH,  // NEW
} lv_screen_state_t;
```

#### Step 2: Create GIF

**Option A:** C-Array

```cpp
// src/gif/gif_bed_mesh.c (create with LVGL Image Converter)
const lv_img_dsc_t gif_bed_mesh = { ... };
```

**Option B:** Filesystem

```
data/gifs/bed_mesh.gif  (240x240 pixels)
```

#### Step 3: Add Screen (Optional)

Use SquareLine Studio to create `ui_ScreenBedMesh` or reuse existing busy screen.

#### Step 4: Add Transition Logic

```cpp
void update_screen_state(void) {
    // Check for bed mesh active
    if (klipper_vars.bed_mesh_active) {
        lv_goto_busy_screen(ui_ScreenBusy,
                            LV_MOONRAKER_STATE_BED_MESH,
                            &gif_bed_mesh);
        return;
    }

    // ... rest of state checks ...
}
```

#### Step 5: Klipper Variable

```ini
[gcode_macro bed_mesh_status]
variable_active: 0
gcode:

[gcode_macro BED_MESH_CALIBRATE]
rename_existing: _BED_MESH_CALIBRATE
gcode:
    SET_GCODE_VARIABLE MACRO=bed_mesh_status VARIABLE=active VALUE=1
    _BED_MESH_CALIBRATE {rawparams}
    SET_GCODE_VARIABLE MACRO=bed_mesh_status VARIABLE=active VALUE=0
```

---

### Modify State Priority

**Example:** Show printing screen instead of heating if printing

```cpp
void update_screen_state(void) {
    // Priority 1: Printing (moved up from Priority 5)
    if (is_printing()) {
        goto_printing_screen();
        return;
    }

    // Priority 2: Homing
    if (is_homing()) {
        lv_goto_busy_screen(...);
        return;
    }

    // ... rest ...
}
```

---

## Resources

- [LVGL Screen Management](https://docs.lvgl.io/8.3/overview/display.html#screens)
- [Moonraker API](https://moonraker.readthedocs.io/en/latest/web_api/)
- [Klipper Macros](https://www.klipper3d.org/Config_Reference.html#gcode_macro)

---

## See Also

- [UI_CUSTOMIZATION.md](UI_CUSTOMIZATION.md) - Widget styling and layer architecture
- [PRINT_PROGRESS_FEATURE.md](PRINT_PROGRESS_FEATURE.md) - Print progress screen details
- [DISPLAY_SLEEP_IMPLEMENTATION.md](DISPLAY_SLEEP_IMPLEMENTATION.md) - Power management integration

---

**Last Updated:** December 2, 2025
**Version:** 1.0.0
