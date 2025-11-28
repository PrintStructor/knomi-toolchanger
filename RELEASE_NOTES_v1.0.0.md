# KNOMI V2 - 6-Toolhead VORON v1.0.0 Release Notes

## 🎯 Overview

Initial release of the KNOMI display firmware for 6-tool VORON printers, featuring intelligent power management, multi-display support, and seamless Klipper integration.

**Part of the [Klipper Toolchanger Extended](https://github.com/PrintStructor/klipper-toolchanger-extended) ecosystem**

---

## ✨ Features in v1.0.0

### 🔋 Power Management System
- **Three Sleep Modes:**
  - Manual Mode: Time-based (60s idle → 5min sleep)
  - Klipper Sync: Follows Klipper's idle state
  - LED Sync: Mirrors case LED status
- **85% power savings** (~50mA in sleep vs ~300mA active)
- **Backlight-only sleep** for instant wake-up (<100ms)
- **Automatic wake** on touch, print start, or status changes

### 🌐 Network & Reliability
- **HTTP API endpoints** for remote control
- **Retry logic** for multi-display commands (3 attempts)
- **95% success rate** for sleep/wake operations (up from 45%)
- **WiFi password masking** in serial logs

### 🎨 UI Improvements
- **Temperature-based color gradient** on progress ring
- **Fixed black screen on wake-up** issue
- **Improved arc visibility** during print progress
- **Clean temperature graph** rendering

### 📚 Documentation
- **Complete English translation** of all code comments
- **Comprehensive README** with installation guide
- **Detailed API documentation** for power management
- **Klipper integration guide** with macros

---

## 📦 Installation

### Quick Start

1. **Download the firmware:**
   - `firmware_knomiv2_v1.0.0.bin` - Main firmware
   - `littlefs_v1.0.0.bin` - Filesystem with GIFs

2. **Flash via USB:**
   ```bash
   esptool.py --chip esp32s3 --port /dev/ttyUSB0 write_flash 0x0 firmware_knomiv2_v1.0.0.bin
   esptool.py --chip esp32s3 --port /dev/ttyUSB0 write_flash 0xa00000 littlefs_v1.0.0.bin
   ```

3. **Or use PlatformIO:**
   ```bash
   pio run -e knomiv2 -t upload
   pio run -e knomiv2 -t uploadfs
   ```

### Klipper Configuration

Add to your `printer.cfg`:

```ini
[include knomi.cfg]
[include macros.cfg]
```

See [Installation Guide](docs/DISPLAY_SLEEP_INSTALLATION.md) for detailed setup.

---

## 🔧 Configuration

### Choose Sleep Mode

Edit `src/lvgl_usr.cpp` before compiling:

```cpp
display_sleep_init(SLEEP_MODE_KLIPPER_SYNC);  // Recommended
// or
display_sleep_init(SLEEP_MODE_MANUAL);
// or
display_sleep_init(SLEEP_MODE_LED_SYNC);
```

### Adjust Timeouts

Edit `src/power_management/display_sleep.h`:

```cpp
#define DISPLAY_IDLE_TIMEOUT_SEC   60    // Idle after 60s
#define DISPLAY_SLEEP_TIMEOUT_SEC  300   // Sleep after 5min
#define DISPLAY_SLEEP_DELAY_AFTER_KLIPPER_IDLE_SEC 30  // Klipper sync delay
```

---

## 🐛 Bug Fixes

- **Fixed:** Black screen after wake-up (display register loss)
- **Fixed:** Inconsistent multi-display sleep/wake behavior
- **Fixed:** Race conditions in LVGL rendering
- **Fixed:** Progress arc not covering colored ring properly
- **Fixed:** Temperature graph background artifacts
- **Fixed:** WiFi passwords visible in serial logs

---

## 📊 Performance Improvements

| Metric | Before | After | Improvement |
|--------|--------|-------|-------------|
| Multi-display success rate | 45% | 95% | **+50%** |
| Wake-up time | 510ms | <100ms | **80% faster** |
| Power in sleep | 280mA | 50mA | **82% reduction** |

---

## 📚 Documentation

- [README.md](README.md) - Main documentation
- [FEATURES.md](FEATURES.md) - Complete feature list
- [CHANGELOG.md](CHANGELOG.md) - Detailed version history
- [CONTRIBUTING.md](CONTRIBUTING.md) - Contribution guidelines
- [DISPLAY_SLEEP_README.md](DISPLAY_SLEEP_README.md) - Power management deep-dive

---

## 🛠️ Hardware Requirements

- **Board:** BTT KNOMI V2 (ESP32-S3-R8)
- **Display:** GC9A01 Round TFT (240x240)
- **Memory:** 16MB Flash, 8MB PSRAM
- **Printer:** Klipper-based toolchanger (6 tools)

---

## ⚠️ Known Limitations

- Camera support disabled in 6-toolhead mode (pins shared with buttons/LEDs)
- OTA updates not supported (use USB flashing)
- mDNS required for multi-display commands (fallback to IP addresses possible)

---

## 🙏 Credits

**Original Firmware:** BigTreeTech (BTT)
**Enhancements:** [PrintStructor](https://github.com/PrintStructor)
**Ecosystem:** [Klipper Toolchanger Extended](https://github.com/PrintStructor/klipper-toolchanger-extended)

**Special Thanks:**
- BTT for open-sourcing KNOMI firmware
- LVGL community for UI framework
- Klipper community for integration support
- VORON Design for the amazing printer platform

---

## ☕ Support This Project

If you find this firmware useful, consider supporting the development:

[![Buy Me A Coffee](https://img.shields.io/badge/Buy%20Me%20A%20Coffee-Support%20Development-yellow.svg?style=for-the-badge&logo=buy-me-a-coffee)](https://buymeacoffee.com/printstructor)

---

## 📄 License

GPL-3.0 (same as original BTT KNOMI firmware)

---

**Full Changelog:** [CHANGELOG.md](CHANGELOG.md)
**Installation Guide:** [docs/DISPLAY_SLEEP_INSTALLATION.md](docs/DISPLAY_SLEEP_INSTALLATION.md)
**Klipper Integration:** [docs/DISPLAY_SLEEP_KLIPPER_INTEGRATION.md](docs/DISPLAY_SLEEP_KLIPPER_INTEGRATION.md)
