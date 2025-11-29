# 🌙 KNOMI Display Sleep Mode

Intelligent power management system for KNOMI V2 6-Toolhead Displays

## 🎯 Features

### Two-Stage Power Management

**Stage 1: Idle Mode (60 seconds)**
- Display shows tool-specific standby GIF
- All systems remain active
- Full touch responsiveness

**Stage 2: Sleep Mode (5 minutes)**
- Display completely black (backlight off)
- Hardware Sleep Mode (GC9A01 SLPIN)
- LVGL timers paused
- **85% less power consumption**

### Intelligent Wake-up

The display wakes up automatically when:
- ✅ Touch input
- ✅ Print starts
- ✅ Heating begins (Bed/Nozzle)
- ✅ Homing/Probing/QGL active

## 📁 Files

```
power_management/
├── display_sleep.h       # Public API & Configuration
└── display_sleep.cpp     # Implementation
```

## 🔧 API Overview

```cpp
// Initialization (in lvgl_ui_task)
void display_sleep_init(void);

// Update Loop (in lv_loop_auto_idle)
void display_sleep_update(void);

// Touch Handler (in usr_touchpad_read)
void display_sleep_reset_timer(void);

// Status Changes (in lv_loop_moonraker_change_screen)
void display_check_wake_condition(bool status_changed);

// Manual Sleep/Wake
void display_enter_sleep(void);
void display_wake_up(void);

// Status Queries
bool display_is_sleeping(void);
display_sleep_state_t display_get_state(void);
```

## ⚙️ Configuration

In `display_sleep.h`:

```cpp
// Time constants (in seconds)
#define DISPLAY_IDLE_TIMEOUT_SEC   60    // Idle after 60s
#define DISPLAY_SLEEP_TIMEOUT_SEC  300   // Sleep after 5 min
```

### Recommended Configurations

| Use Case | Idle | Sleep | Description |
|----------|------|-------|--------------|
| Standard | 60s | 300s | Balanced |
| Aggressive | 30s | 120s | Max power saving |
| Relaxed | 120s | 600s | Infrequent sleep |
| Idle Only | 60s | 999999s | Sleep disabled |

## 📊 State Machine

```
ACTIVE (normal)
    ↓ 60s inactivity
IDLE (Tool-GIF)
    ↓ additional 240s (= 5 min total)
SLEEPING (Display off)
    ↓ Touch or status change
ACTIVE (awake)
```

## 🔌 Hardware Sleep Details

### GC9A01 Commands
- `0x10` SLPIN - Enter Sleep (120ms delay)
- `0x11` SLPOUT - Exit Sleep (120ms delay)
- `0x28` DISPOFF - Display Output OFF
- `0x29` DISPON - Display Output ON

### Power Consumption

| State | Backlight | Display | LVGL | CPU | Current |
|-------|-----------|---------|------|-----|---------|
| Active | 100% | ON | ✓ | ~30% | ~300mA |
| Idle | 100% | ON | ✓ | ~25% | ~300mA |
| Sleep | 0% | Sleep | ✗ | ~5% | ~50mA |

**Savings: ~85% in Sleep Mode**

## 🚀 Integration

Complete guides:
- `docs/DISPLAY_SLEEP_INSTALLATION.md` - User build/flash steps and timeouts
- `docs/dev/DISPLAY_SLEEP_IMPLEMENTATION.md` - Developer hooks and integration points

**Quick Start:**

These files are already part of the firmware. To use the sleep system:
1. Build/flash the firmware as in `docs/DISPLAY_SLEEP_INSTALLATION.md`
2. Keep Moonraker reachable so wake-on-activity works
3. If you are porting or modifying: follow the hook list in `docs/dev/DISPLAY_SLEEP_IMPLEMENTATION.md`

## 🧪 Testing

### Basic Test
```
1. Start system
2. Wait 60s → Idle-GIF
3. Wait 5 min → Display black
4. Touch → Display wakes up
```

### Status Test
```
1. Let display sleep
2. Start print in Klipper
3. Display wakes up automatically
```

### Serial Monitor
```
[Display Sleep] Initialized
[Display Sleep] → IDLE state
[Display Sleep] ENTERING SLEEP MODE
[Display Sleep] Backlight OFF
[Display Sleep] GC9A01 entered sleep mode
[Display Sleep] ✅ Sleep mode active
```

## 🐛 Debugging

**Problem:** Display doesn't sleep
```cpp
// Check in lv_auto_goto_idle.cpp:
display_sleep_update();  // Present?
Serial.println(display_get_state());  // Output?
```

**Problem:** Wake-up doesn't work
```cpp
// Check in lv_moonraker_change_screen.cpp:
display_check_wake_condition(status_changed);  // Present?
// Check in lvgl_hal.cpp:
display_sleep_reset_timer();  // On touch present?
```

**Problem:** Compile error
```bash
pio run -e knomiv2 --target clean
pio run -e knomiv2
```

## 📝 Changelog

### v1.0.0 (2025-01-24)
- Initial release
- Two-stage power management (Idle + Sleep)
- Automatic wake-up on status changes
- Touch-based wake-up
- Configurable timeouts
- GC9A01 Hardware Sleep Support

## 🔮 Future Ideas

Possible enhancements:
- [ ] Deep Sleep Mode for ESP32 (WiFi off)
- [ ] Adaptive timeouts based on usage patterns
- [ ] Wake-up schedule (e.g. office hours)
- [ ] MQTT-controlled sleep commands
- [ ] Display brightness dimming before sleep

## 📄 License

Part of the KNOMI V2 6-Toolhead VORON Project

## 👨‍💻 Author

Developed for the KNOMI 6-Toolhead Display System
