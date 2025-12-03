# Developer Documentation

Welcome to the KNOMI V2 developer documentation. This directory contains technical deep-dives, implementation details, and customization guides for developers who want to understand, modify, or extend the firmware.

---

## 📚 Documentation Overview

### Core Architecture

| Document | Description | Topics Covered |
|----------|-------------|----------------|
| **[UI_CUSTOMIZATION.md](UI_CUSTOMIZATION.md)** | Complete guide to LVGL UI customization | Layer architecture, widgets, GIF integration, color themes, performance optimization |
| **[PRINT_PROGRESS_FEATURE.md](PRINT_PROGRESS_FEATURE.md)** | Print progress system deep-dive | PSRAM management, layer compositing, Moonraker integration, tool indicators |
| **[HYBRID_DISPLAY.md](HYBRID_DISPLAY.md)** | State machine & screen switching | Display states, state transitions, GIF management, Klipper integration |
| **[DISPLAY_SLEEP_IMPLEMENTATION.md](DISPLAY_SLEEP_IMPLEMENTATION.md)** | Power management system | Sleep modes, hooks, timer management, API implementation |
| **[WIFI_TROUBLESHOOTING.md](WIFI_TROUBLESHOOTING.md)** | WiFi connectivity & recovery | Auto-reconnect logic, remote restart API, debugging WiFi issues |

---

## 🎯 Quick Start for Developers

### New to KNOMI Development?

**Start here:**

1. **[UI_CUSTOMIZATION.md](UI_CUSTOMIZATION.md)** - Learn the layer architecture and LVGL basics
2. **[HYBRID_DISPLAY.md](HYBRID_DISPLAY.md)** - Understand how screens switch automatically
3. **[PRINT_PROGRESS_FEATURE.md](PRINT_PROGRESS_FEATURE.md)** - See how dynamic overlays work

### Want to Add Features?

| Goal | Read This | Key Sections |
|------|-----------|--------------|
| **Add a new screen** | [HYBRID_DISPLAY.md](HYBRID_DISPLAY.md#add-new-state) | State machine, transitions |
| **Customize GIFs** | [UI_CUSTOMIZATION.md](UI_CUSTOMIZATION.md#custom-gif-integration) | Filesystem GIFs, C-array GIFs |
| **Modify progress display** | [PRINT_PROGRESS_FEATURE.md](PRINT_PROGRESS_FEATURE.md#layer-system) | Layer architecture, PSRAM |
| **Add power management** | [DISPLAY_SLEEP_IMPLEMENTATION.md](DISPLAY_SLEEP_IMPLEMENTATION.md#hooks) | Sleep hooks, timers |
| **Change color themes** | [UI_CUSTOMIZATION.md](UI_CUSTOMIZATION.md#color-themes) | Tool colors, gradients |
| **Fix WiFi connectivity** | [WIFI_TROUBLESHOOTING.md](WIFI_TROUBLESHOOTING.md) | Auto-reconnect, remote restart, debugging |

---

## 🏗️ Architecture Overview

### System Layers

```
┌─────────────────────────────────────────────────────┐
│  Moonraker API (Klipper Integration)                │
│  - Printer state, temperatures, progress            │
└───────────────────────┬─────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────┐
│  State Machine (Hybrid Display)                     │
│  - Idle, Homing, Heating, Printing, Complete        │
│  - Automatic screen transitions                     │
└───────────────────────┬─────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────┐
│  LVGL UI Layer System                               │
│  - Background GIFs (PSRAM)                          │
│  - Static overlays (PNG rings)                      │
│  - Dynamic widgets (arcs, labels)                   │
└───────────────────────┬─────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────┐
│  Display Hardware (GC9A01)                          │
│  - 240x240 circular TFT                             │
│  - 80MHz SPI, 60fps rendering                       │
└─────────────────────────────────────────────────────┘
```

---

## 🔑 Key Concepts

### 1. Layer Architecture

The KNOMI UI uses a **4-layer compositing system**:

```
Layer 3: Text Labels (Foreground)
Layer 2: Progress Arc (Semi-transparent black)
Layer 1: Static Ring (Colorful PNG)
Layer 0: Animated GIF (Background from PSRAM)
```

**Why?** This allows dynamic overlays on animated backgrounds without redrawing the entire screen—critical for 60fps performance.

**Learn more:** [UI_CUSTOMIZATION.md](UI_CUSTOMIZATION.md#lvgl-layer-system)

---

### 2. Hybrid Display (State Machine)

The display **automatically** switches between modes based on printer state:

- **Idle** → Tool-specific GIF
- **Homing** → Homing animation
- **Heating** → Temperature sliders
- **Printing** → Progress overlay
- **Complete** → Success checkmark

**No user input needed**—the display adapts in real-time.

**Learn more:** [HYBRID_DISPLAY.md](HYBRID_DISPLAY.md#state-machine-overview)

---

### 3. PSRAM Optimization

Large GIF files (100-500KB) are loaded into **PSRAM** (8MB external RAM) instead of internal SRAM:

**Benefits:**
- ✅ Keeps internal RAM free for LVGL rendering
- ✅ Fast recreation (data persists after screen switch)
- ✅ No filesystem re-reads

**Learn more:** [PRINT_PROGRESS_FEATURE.md](PRINT_PROGRESS_FEATURE.md#psram-management)

---

### 4. Tool Detection & Multi-Display

Each KNOMI display detects its tool number from hostname:

```
knomi-t0.local → Tool 0
knomi-t1.local → Tool 1
...
knomi-t5.local → Tool 5
```

This enables:
- **Tool-specific GIFs** (e.g., `tool_0.gif` for T0)
- **Tool-specific colors** (e.g., red for T0, green for T1)
- **Synchronized sleep/wake** across all 6 displays

**Learn more:** [UI_CUSTOMIZATION.md](UI_CUSTOMIZATION.md#tool-specific-colors)

---

## 🌐 HTTP API Reference

The KNOMI V2 exposes several HTTP endpoints for remote control and monitoring.

### Power Management & Sleep

```bash
# Put display to sleep
curl -X POST http://knomi-t0.local/api/sleep

# Wake display
curl -X POST http://knomi-t0.local/api/wake

# Check sleep status
curl http://knomi-t0.local/api/sleep/status
```

**Response:**
```json
{
  "sleeping": false,
  "state": "active",
  "mode": "klipper_sync"
}
```

**Learn more:** [DISPLAY_SLEEP_IMPLEMENTATION.md](DISPLAY_SLEEP_IMPLEMENTATION.md#api-endpoints)

---

### Remote Restart (v1.1.0+)

```bash
# Restart display without power cycling
curl -X POST http://knomi-t0.local/api/restart
```

**Response:**
```json
{"status":"restarting"}
```

**Use case:** When display loses WiFi connection and becomes unresponsive.

**Learn more:** [WIFI_TROUBLESHOOTING.md](WIFI_TROUBLESHOOTING.md#manual-recovery-options)

---

### Batch Operations

```bash
# Restart all 6 displays in parallel
for i in {0..5}; do
  curl -X POST http://knomi-t$i.local/api/restart &
done
wait

# Put all displays to sleep
for i in {0..5}; do
  curl -X POST http://knomi-t$i.local/api/sleep &
done
wait
```

---

## 🛠️ Development Environment

### Prerequisites

- **PlatformIO** (VSCode extension recommended)
- **ESP32 toolchain** (auto-installed by PlatformIO)
- **SquareLine Studio** (optional, for UI design)
- **LVGL 8.3.7** (included in project)

### Build & Flash

```bash
# Clone repository (firmware branch)
git clone -b firmware https://github.com/PrintStructor/knomi-toolchanger.git
cd knomi-toolchanger

# Build firmware
pio run -e knomiv2

# Flash firmware
pio run -e knomiv2 -t upload

# Flash filesystem (GIFs)
pio run -e knomiv2 -t uploadfs

# Monitor serial output
pio device monitor -b 115200
```

---

## 📝 Code Style Guide

### File Organization

```
src/
├── ui/                     # SquareLine Studio exports (auto-generated)
│   ├── screens/            # Screen definitions
│   ├── images/             # Static assets (C-arrays)
│   ├── fonts/              # Custom fonts
│   └── ui.c/h              # Main UI entry point
├── ui_overlay/             # Custom logic & overlays
│   ├── lv_print_progress_update.cpp    # Print progress system
│   ├── lv_moonraker_change_screen.cpp  # State machine
│   └── lv_theme_color.cpp              # Color themes
├── gif/                    # Built-in GIF animations (C-arrays)
├── power_management/       # Display sleep system
└── main.cpp                # Firmware entry point
```

### Naming Conventions

**LVGL Objects:**
- Prefix with `ui_` for SquareLine exports: `ui_label_printing_progress`
- Prefix with `lv_` for custom LVGL code: `lv_goto_idle_screen()`

**Functions:**
- Snake_case for C/C++ functions: `update_print_progress()`
- Verb-first for actions: `delete_ui_bg_gif()`, `show_ui_bg_ring()`

**Constants:**
- ALL_CAPS for defines: `DISPLAY_IDLE_TIMEOUT_SEC`
- Prefix with `LV_` for LVGL enums: `LV_MOONRAKER_STATE_IDLE`

---

## 🧪 Testing & Debugging

### Serial Debugging

**Enable verbose logging:**

```cpp
// In main.cpp or lvgl_usr.cpp
#define DEBUG_PRINT_PROGRESS 1
#define DEBUG_STATE_MACHINE  1
#define DEBUG_PSRAM          1
```

**Monitor output:**

```bash
pio device monitor -b 115200
```

**Expected logs:**

```
[Progress] Loaded GIF: 123456 bytes into PSRAM
[Progress] Background Ring created
[Progress] Update: 42% (Layer 42/120)
[State] Transition: IDLE → PRINTING
[Sleep] WAKING FROM SLEEP (trigger: state_change)
```

---

### Performance Profiling

**Check FPS:**

```cpp
// In lvgl_usr.cpp
void loop() {
    lv_task_handler();
    static uint32_t last_fps_check = 0;
    if (millis() - last_fps_check > 5000) {  // Every 5 seconds
        uint32_t fps = lv_refr_get_fps_avg();
        Serial.printf("LVGL FPS: %d\n", fps);
        last_fps_check = millis();
    }
}
```

**Target:** 55-60 FPS (at 80MHz SPI)

---

### Memory Debugging

**Check PSRAM usage:**

```cpp
Serial.printf("PSRAM Free: %u bytes\n", ESP.getFreePsram());
Serial.printf("PSRAM Total: %u bytes\n", ESP.getPsramSize());
```

**Check heap:**

```cpp
Serial.printf("Free Heap: %u bytes\n", ESP.getFreeHeap());
```

---

## 📦 Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| **LVGL** | 8.3.7 | UI framework |
| **Arduino-ESP32** | 2.0.x | ESP32 core |
| **LittleFS** | (built-in) | Filesystem for GIFs |
| **WiFi** | (built-in) | Network connectivity |
| **HTTPClient** | (built-in) | Moonraker API |
| **ArduinoJson** | 6.x | JSON parsing |

---

## 🔗 Related Documentation

### User Documentation

- [README.md](../../README.md) - Main project documentation
- [FEATURES.md](../../FEATURES.md) - Feature overview
- [CHANGELOG.md](../../CHANGELOG.md) - Version history

### Installation Guides

- [DISPLAY_SLEEP_INSTALLATION.md](../DISPLAY_SLEEP_INSTALLATION.md) - Sleep system setup
- [DISPLAY_SLEEP_KLIPPER_INTEGRATION.md](../DISPLAY_SLEEP_KLIPPER_INTEGRATION.md) - Klipper macros
- [FLASH_TOOL_GUIDE.md](../FLASH_TOOL_GUIDE.md) - ESP32 flashing guide
- [TEMP_GRAPH_FIX.md](../TEMP_GRAPH_FIX.md) - Temperature display fixes

### External Resources

- [LVGL Documentation](https://docs.lvgl.io/8.3/)
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [Moonraker API](https://moonraker.readthedocs.io/en/latest/web_api/)
- [Klipper Config Reference](https://www.klipper3d.org/Config_Reference.html)

---

## 🤝 Contributing

### Before You Start

1. Read the **code style guide** above
2. Review existing **state machine logic** in [HYBRID_DISPLAY.md](HYBRID_DISPLAY.md)
3. Understand the **layer architecture** in [UI_CUSTOMIZATION.md](UI_CUSTOMIZATION.md)

### Making Changes

1. **Fork** the repository
2. Create a **feature branch** (`git checkout -b feature/my-feature`)
3. **Test thoroughly** (all 6 displays if possible)
4. **Document** your changes (update relevant .md files)
5. Submit a **pull request**

### Code Review Checklist

- [ ] Code follows naming conventions
- [ ] No memory leaks (test with `ESP.getFreeHeap()`)
- [ ] FPS remains above 55 (profile with `lv_refr_get_fps_avg()`)
- [ ] Serial logs added for debugging
- [ ] Documentation updated
- [ ] Tested on real hardware (not just emulator)

---

## 💡 Tips & Tricks

### Optimize GIF File Size

```bash
# Use ezgif.com or ImageMagick
convert input.gif -resize 240x240 -colors 256 -coalesce -layers optimize output.gif
```

**Target:** <200KB per GIF

---

### Quick PSRAM Test

```cpp
void test_psram() {
    uint8_t* test = (uint8_t*)ps_malloc(1024 * 1024);  // Allocate 1MB
    if (test) {
        Serial.println("PSRAM allocation OK");
        free(test);
    } else {
        Serial.println("PSRAM allocation FAILED");
    }
}
```

---

### Debug Screen Transitions

```cpp
void _ui_screen_change_debug(lv_obj_t ** target, ...) {
    Serial.printf("[Screen] Changing from %p to %p\n", lv_scr_act(), *target);
    _ui_screen_change(target, anim, time, delay, user_data);
}
```

---

## ❓ FAQ

### Q: How do I add a custom screen?

**A:** See [HYBRID_DISPLAY.md - Add New State](HYBRID_DISPLAY.md#add-new-state)

### Q: Why is my FPS dropping below 50?

**A:** Check:
1. GIF file size (should be <200KB)
2. Double buffering enabled in `lv_conf.h`
3. SPI speed (should be 80MHz, not 40MHz)
4. Minimize label updates (only update on change)

### Q: Can I use LVGL 9?

**A:** Not yet. KNOMI V2 firmware is built for **LVGL 8.3.7**. Porting to LVGL 9 requires significant changes.

### Q: How do I debug PSRAM issues?

**A:** Add this to `setup()`:

```cpp
Serial.printf("PSRAM: %s\n", psramFound() ? "FOUND" : "NOT FOUND");
Serial.printf("PSRAM Size: %u bytes\n", ESP.getPsramSize());
Serial.printf("PSRAM Free: %u bytes\n", ESP.getFreePsram());
```

### Q: Display loses WiFi connection and becomes unresponsive. What do I do?

**A:** Starting with v1.1.0, the display automatically reconnects (3 attempts, 5s interval). If that fails, use:

```bash
# Remote restart (no power cycling needed)
curl -X POST http://knomi-t0.local/api/restart
```

**Learn more:** [WIFI_TROUBLESHOOTING.md](WIFI_TROUBLESHOOTING.md)

---

## 📧 Support

- **Issues:** [GitHub Issues](https://github.com/PrintStructor/knomi-toolchanger/issues)
- **Discussions:** [GitHub Discussions](https://github.com/PrintStructor/knomi-toolchanger/discussions)
- **Discord:** [VORON Discord](https://discord.gg/voron) (#toolchangers channel)

---

**Happy Coding! 🚀**

---

**Last Updated:** December 2, 2025
**Version:** 1.0.0
