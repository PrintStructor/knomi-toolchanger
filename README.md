# 🎯 KNOMI V2 - 6-Toolhead VORON Display System

[![Platform](https://img.shields.io/badge/platform-ESP32--S3-blue.svg)](https://www.espressif.com/en/products/socs/esp32-s3)
[![Framework](https://img.shields.io/badge/framework-Arduino-00979D.svg)](https://www.arduino.cc/)
[![License](https://img.shields.io/badge/license-GPL--3.0-green.svg)](LICENSE)
[![LVGL](https://img.shields.io/badge/LVGL-8.3.7-orange.svg)](https://lvgl.io/)
[![Buy Me A Coffee](https://img.shields.io/badge/Buy%20Me%20A%20Coffee-☕-yellow.svg?style=flat&logo=buy-me-a-coffee)](https://buymeacoffee.com/printstructor)

Advanced firmware for BigTreeTech KNOMI displays with multi-toolhead support, intelligent power management, and Klipper integration.

> **🔧 Part of the [Klipper Toolchanger Extended](https://github.com/PrintStructor/klipper-toolchanger-extended) ecosystem**
>
> This firmware is specifically designed for 6-tool VORON printers using the Klipper Toolchanger Extended framework. Each KNOMI display provides real-time status visualization for its respective toolhead, including:
> - Active tool indication and tool changes
> - Individual extruder temperatures and heating status
> - Print progress with per-tool statistics
> - Synchronized sleep/wake across all 6 displays
> - Seamless integration with toolchanger macros and safety features

---

## 🌟 Key Features

### 🔋 Intelligent Power Management
- **Three Sleep Modes:**
  - **Manual Mode:** Time-based sleep (60s idle → 5min sleep)
  - **Klipper Sync:** Follows Klipper's idle state
  - **LED Sync:** Mirrors case LED status
- **85% power savings** in sleep mode (~50mA vs ~300mA)
- **Automatic wake-up** on touch, print start, or status changes
- **Backlight-only sleep** for instant wake-up (no hardware re-init delays)

### 🎨 Multi-Toolhead Support
- **6 independent displays** (one per toolhead)
- **Tool-specific GIFs** loaded from filesystem
- **Hostname-based tool detection** (`knomi-t0` through `knomi-t5`)
- **Synchronized sleep/wake** across all displays

### 🌐 HTTP API Endpoints
```
POST /api/sleep          - Enter sleep mode
POST /api/wake           - Wake from sleep
GET  /api/sleep/status   - Get sleep status
```

### 🔒 Security & Reliability
- **WiFi password masking** in serial logs
- **Retry logic** for multi-display commands (3 attempts)
- **Timeout management** for network operations
- **95%+ success rate** for sleep/wake commands

### 📊 Advanced UI Features
- **Print progress ring** with ETA calculation
- **Temperature graphs** with real-time monitoring
- **Tool-specific displays** with auto-detection
- **Animated status GIFs** (homing, probing, QGL, etc.)
- **Touch-responsive** interface

---

## 🏗️ Hardware

### Board
- **ESP32-S3-R8** (16MB Flash, 8MB PSRAM)
- **GC9A01** Round TFT Display (240x240)
- **CST816S** Capacitive Touch Controller
- **LIS2DW12** Accelerometer
- **SHT4x** Temperature/Humidity Sensor

### Display
- **Backlight:** PWM-controlled via AW9346 driver
- **Touch:** Multi-gesture support
- **Framerate:** 80MHz SPI, ~60fps

---

## 📦 Project Structure

```
KNOMI_6_VORON/
├── src/
│   ├── power_management/      # Display sleep system
│   │   ├── display_sleep.cpp
│   │   ├── display_sleep.h
│   │   └── README.md
│   ├── ui/                     # LVGL UI components
│   ├── ui_overlay/             # UI overlays & logic
│   ├── gif/                    # Built-in animations
│   └── main.cpp
├── data/
│   └── gifs/                   # Tool-specific GIFs (filesystem)
│       ├── tool_0.gif
│       ├── tool_1.gif
│       └── ...
├── docs/                       # Documentation
├── platformio.ini              # Build configuration
└── README.md
```

---

## 🚀 Quick Start

### 1. Prerequisites

**Software:**
- [PlatformIO](https://platformio.org/) (VSCode extension recommended)
- [Python 3.x](https://www.python.org/) (for upload tools)

**Hardware:**
- BTT KNOMI V2 display(s)
- USB-C cable for flashing
- Klipper-based 3D printer

### 2. Clone & Build

```bash
# Clone repository
git clone https://github.com/PrintStructor/knomi-toolchanger.git
cd knomi-toolchanger

# Switch to firmware branch (contains source code)
git checkout firmware

# Build firmware
pio run -e knomiv2

# Upload to display
pio run -e knomiv2 -t upload

# Upload filesystem (GIFs)
pio run -e knomiv2 -t uploadfs
```

> **Note:** This repository uses a dual-branch structure:
> - `master` branch: Documentation only
> - `firmware` branch: Complete source code and documentation

### 3. Configure WiFi

**Method 1: Web Portal (First Boot)**
1. Display boots into AP mode: `KNOMI_AP_XXXXX`
2. Connect and navigate to `192.168.4.1`
3. Enter WiFi credentials
4. Display auto-restarts

**Method 2: Edit `src/config.h`**
```cpp
#define DEFAULT_STA_SSID "YourWiFiName"
#define DEFAULT_STA_PWD  "YourPassword"
```

### 4. Configure Klipper

**Option 1: Copy config files to Klipper directory**
```bash
# Copy to your Klipper config directory (from firmware branch)
cp knomi.cfg macros.cfg ~/printer_data/config/
```

Then add to your `printer.cfg`:
```ini
[include knomi.cfg]
[include macros.cfg]
```

**Option 2: Use absolute path**
```ini
[include /path/to/knomi-toolchanger/knomi.cfg]
[include /path/to/knomi-toolchanger/macros.cfg]
```

> **Note:** The `knomi.cfg` file contains G-code macro overrides for display integration.
> The `macros.cfg` file includes toolchanger-specific macros and QGL integration.

For detailed Klipper setup, see [DISPLAY_SLEEP_KLIPPER_INTEGRATION.md](docs/DISPLAY_SLEEP_KLIPPER_INTEGRATION.md).

---

## 🔧 Configuration

### Display Sleep Modes

Edit `src/power_management/display_sleep.h`:

```cpp
// Sleep Timeouts (Manual Mode)
#define DISPLAY_IDLE_TIMEOUT_SEC   60    // Idle after 60s
#define DISPLAY_SLEEP_TIMEOUT_SEC  300   // Sleep after 5min

// Klipper Sync Mode
#define DISPLAY_SLEEP_DELAY_AFTER_KLIPPER_IDLE_SEC 30  // Sleep 30s after Klipper idle
```

**Choose mode in `lvgl_usr.cpp`:**
```cpp
display_sleep_init(SLEEP_MODE_KLIPPER_SYNC);  // Recommended
// or
display_sleep_init(SLEEP_MODE_MANUAL);
// or
display_sleep_init(SLEEP_MODE_LED_SYNC);
```

### Display Hostname

**For auto-detection, name displays:**
```
knomi-t0.local  → Tool 0
knomi-t1.local  → Tool 1
...
knomi-t5.local  → Tool 5
```

Set via web interface or mDNS configuration.

---

## 📡 Klipper Integration

### Basic Macros

```gcode
KNOMI_SLEEP          # Put all displays to sleep
KNOMI_WAKE           # Wake all displays
```

### Auto-Wake on Events

The firmware automatically wakes displays when:
- ✅ Print starts
- ✅ Homing begins
- ✅ Bed/nozzle heating starts
- ✅ Probing/QGL active
- ✅ Touch detected

### Advanced Multi-Display Control

For **reliable 6-display setups**, use retry logic:

```cfg
[gcode_shell_command knomi_sleep_all_retry]
command: sh -c 'for i in 0 1 2 3 4 5; do (for retry in 1 2 3; do curl -X POST --connect-timeout 2 --max-time 5 http://knomi-t$i.local/api/sleep 2>/dev/null && break || sleep 0.3; done) & done; wait'
timeout: 30.0
verbose: False
```

For complete multi-display setup, see `docs/DISPLAY_SLEEP_KLIPPER_INTEGRATION.md`.

---

## 🎨 Customization

### Add Your Own Tool GIFs

1. Create GIFs (240x240, optimize for size)
2. Name them `tool_0.gif` through `tool_5.gif`
3. Place in `data/gifs/` folder
4. Upload: `pio run -e knomiv2 -t uploadfs`

**Recommended specs:**
- Resolution: 240x240px
- Frame rate: 15-30fps
- Duration: 2-5 seconds (looping)
- File size: <200KB per GIF

### Custom Animations

Built-in animations in `src/gif/`:
- `gif_homing.c` - Homing sequence
- `gif_probing.c` - Bed leveling
- `gif_qgling.c` - Quad gantry leveling
- `gif_print_ok.c` - Print complete

Edit C arrays and rebuild to change.

---

## 📊 Performance

### Power Consumption

| State | Current | Description |
|-------|---------|-------------|
| Active | ~300mA | Display on, LVGL rendering |
| Idle | ~300mA | Showing tool GIF |
| **Sleep** | **~50mA** | Backlight off, timers paused |

**Savings:** 85% reduction in sleep mode

### Multi-Display Reliability

| Metric | Without Retry | With Retry | Improvement |
|--------|---------------|------------|-------------|
| Sleep Success | 50% | 95% | +45% |
| Wake Success | 40% | 95% | +55% |
| **Overall** | **45%** | **95%** | **+50%** |

---

## 🛠️ Troubleshooting

### Display Not Waking Up

**Check:**
1. Serial logs for `[Display Sleep] WAKING FROM SLEEP`
2. Touch sensor is responding (`[Touch] Event detected`)
3. Klipper connection (`moonraker.unconnected = false`)

**Solution:**
```bash
# Test manual wake via API
curl -X POST http://knomi-t0.local/api/wake
```

### Not All Displays Respond

**Common causes:**
- mDNS resolution issues
- Network congestion
- Display busy with rendering

**Solution:** Increase retries or use sequential mode (see docs).

### GIFs Not Loading

**Check:**
1. Filesystem uploaded: `pio run -t uploadfs`
2. Files in correct path: `/gifs/tool_X.gif`
3. File sizes reasonable (<500KB)

**Debug:**
```
[FS] Loading GIF for tool 0 from /gifs/tool_0.gif
[FS] Loaded tool 0 GIF: 123456 bytes into PSRAM
```

---

## 📚 Documentation

### Ecosystem Integration

> **🔧 This firmware is part of the [Klipper Toolchanger Extended](https://github.com/PrintStructor/klipper-toolchanger-extended) ecosystem**

For full multi-tool integration, see:
- **[Klipper Toolchanger Extended](https://github.com/PrintStructor/klipper-toolchanger-extended)** - Safety logic, tool management, collision detection
- **KNOMI Toolchanger** (this repo) - Display status, power management, per-tool visualization

### Core Documentation

Key documents:
- [CHANGELOG.md](CHANGELOG.md) - Version history and release notes
- [FEATURES.md](FEATURES.md) - Complete feature list
- [CONTRIBUTING.md](CONTRIBUTING.md) - Contribution guidelines
- [RELEASE_NOTES_v1.0.0.md](RELEASE_NOTES_v1.0.0.md) - Release highlights

### Installation & Setup
- [DISPLAY_SLEEP_INSTALLATION.md](docs/DISPLAY_SLEEP_INSTALLATION.md) - Sleep system installation
- [DISPLAY_SLEEP_KLIPPER_INTEGRATION.md](docs/DISPLAY_SLEEP_KLIPPER_INTEGRATION.md) - Klipper macros and G-code integration
- [TEMP_GRAPH_FIX.md](docs/TEMP_GRAPH_FIX.md) - Optional temp graph cleanup

### Technical Documentation
- [docs/dev/](docs/dev/README.md) - Developer documentation overview
- [docs/dev/UI_CUSTOMIZATION.md](docs/dev/UI_CUSTOMIZATION.md) - LVGL layer architecture, widgets, and GIF integration
- [docs/dev/PRINT_PROGRESS_FEATURE.md](docs/dev/PRINT_PROGRESS_FEATURE.md) - Print progress system, PSRAM management, overlays
- [docs/dev/HYBRID_DISPLAY.md](docs/dev/HYBRID_DISPLAY.md) - State machine, screen transitions, Klipper integration
- [docs/dev/DISPLAY_SLEEP_IMPLEMENTATION.md](docs/dev/DISPLAY_SLEEP_IMPLEMENTATION.md) - Power management hooks and timers
- [docs/FLASH_TOOL_GUIDE.md](docs/FLASH_TOOL_GUIDE.md) - ESP32 Flash Download Tool guide
- [docs/TEMP_GRAPH_FIX.md](docs/TEMP_GRAPH_FIX.md) - Temperature display fixes
- [src/power_management/](https://github.com/PrintStructor/knomi-toolchanger/tree/firmware/src/power_management) - Sleep API reference (firmware branch)

---

## 🔄 Changelog

### Version 1.0.0 (2025-01-28)

#### Added
- ✅ Three sleep modes (Manual, Klipper Sync, LED Sync)
- ✅ Backlight-only sleep for instant wake-up
- ✅ HTTP API endpoints for remote control
- ✅ Retry logic for multi-display setups
- ✅ WiFi password masking in logs

#### Changed
- 🔧 Simplified sleep commands (no hardware re-init)
- 🔧 Improved wake-up reliability (95% success rate)
- 🔧 Enhanced Klipper state tracking

#### Fixed
- 🐛 Black screen after wake-up
- 🐛 Inconsistent multi-display behavior
- 🐛 Race conditions in LVGL rendering
- 🐛 Password leaks in serial console

---

## 🤝 Contributing

Contributions welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Test thoroughly (all 6 displays if possible)
4. Submit a pull request

**Areas for contribution:**
- Additional sleep modes
- UI themes
- More tool GIFs
- Documentation translations
- Bug reports

---

## 📄 License

Based on [BTT KNOMI](https://github.com/bigtreetech/KNOMI) open-source firmware.

This project extends the original with:
- Multi-toolhead support (6x displays)
- Advanced power management
- Klipper integration
- Enhanced UI features

**License:** GPL-3.0 (same as original)

---

## 🙏 Credits

**Original Firmware:** BigTreeTech (BTT)
**Enhancements:** [PrintStructor](https://github.com/PrintStructor) - KNOMI 6-Toolhead Project
**Hardware:** ESP32-S3, GC9A01, LVGL
**Ecosystem:** [Klipper Toolchanger Extended](https://github.com/PrintStructor/klipper-toolchanger-extended)

**Special Thanks:**
- BTT for open-sourcing KNOMI firmware
- LVGL community for UI framework
- Klipper community for integration support
- VORON Design for the amazing printer platform

---

## 📞 Support

### Documentation
- [Installation Guide](docs/DISPLAY_SLEEP_INSTALLATION.md) - Sleep system installation
- [Klipper Integration](docs/DISPLAY_SLEEP_KLIPPER_INTEGRATION.md) - G-code macros and setup
- [Developer Details](docs/dev/DISPLAY_SLEEP_IMPLEMENTATION.md) - Hooks, timers, and porting notes

### Debug Tools
```bash
# Serial monitor (115200 baud)
pio device monitor -b 115200

# Test API endpoint
curl -X GET http://knomi-t0.local/api/sleep/status

# Check mDNS resolution
ping knomi-t0.local
```

### Issue Reporting
Please include:
- Hardware version (KNOMI V1/V2)
- Firmware version
- Serial logs
- Number of displays
- Klipper version

---

## 🔮 Roadmap

**Planned Features:**
- [ ] Deep sleep mode for ESP32 (WiFi off)
- [ ] Adaptive sleep timeouts (learning mode)
- [ ] MQTT integration
- [ ] OTA updates over network
- [ ] Multi-language UI
- [ ] Custom themes via web interface

---

## ☕ Support This Project

If you find this firmware useful for your toolchanger setup, consider supporting the development:

[![Buy Me A Coffee](https://img.shields.io/badge/Buy%20Me%20A%20Coffee-Support%20Development-yellow.svg?style=for-the-badge&logo=buy-me-a-coffee)](https://buymeacoffee.com/printstructor)

Your support helps maintain and improve both this firmware and the [Klipper Toolchanger Extended](https://github.com/PrintStructor/klipper-toolchanger-extended) ecosystem!

---

**⭐ Star this project if you find it useful!**

**📢 Share your KNOMI setups on [Voron Discord](https://discord.gg/voron)!**

---

**Last Updated:** January 28, 2025
**Version:** 1.0.0
**Status:** ✅ Production Ready
