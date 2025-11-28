# 🌟 KNOMI 6-Toolhead Features

Complete feature list for the advanced KNOMI multi-display system.

---

## 🔋 Power Management

### Three Sleep Modes

#### 1. Manual Mode (Time-Based)
```
Active → 60s idle → Idle GIF → 5min → Sleep
```
- **Best for:** Standalone displays
- **Configuration:** Adjustable timeouts
- **Wake:** Touch or status change

#### 2. Klipper Sync Mode (Recommended)
```
Klipper Active → Klipper Idle → 30s delay → Display Sleep
```
- **Best for:** Integrated printer systems
- **Configuration:** Delay after Klipper idle
- **Wake:** Automatic on print/heating/homing
- **Safety:** Never sleeps during restart or errors

#### 3. LED Sync Mode
```
LEDs ON → Display Active
LEDs OFF → Display Sleep
```
- **Best for:** Synchronized lighting systems
- **Configuration:** External LED controller
- **Wake:** Instant when LEDs turn on

### Power Savings
- **Active:** ~300mA per display
- **Sleep:** ~50mA per display
- **Savings:** 83% reduction
- **6 displays:** Save ~1500mA (1.5A) when sleeping!

### Wake Triggers
✅ Touch input  
✅ Print start  
✅ Homing begins  
✅ Bed heating starts  
✅ Nozzle heating starts  
✅ Probing/QGL active  
✅ Klipper state change  
✅ Manual command (API)  

---

## 🎨 Display Features

### Multi-Toolhead Support
- **6 independent displays** (expandable)
- **Auto-detection** via hostname (`knomi-t0` to `knomi-t5`)
- **Tool-specific GIFs** from filesystem
- **Individual control** or synchronized
- **No master/slave** - fully distributed

### UI Components

#### Print Progress View
- **Circular progress ring** (animated)
- **Percentage display** (0-100%)
- **Layer counter** (current/total)
- **ETA calculation** with adaptive buffering
- **Tool indicator** showing active toolhead
- **Temperature display** for current tool

#### Temperature Graph
- **Real-time plotting** of nozzle/bed temps
- **Auto-scaling Y-axis** (0-300°C)
- **120-second history** window
- **Smooth curves** with data interpolation
- **Clean black background** (no artifacts)

#### Animated Status Screens
- **Homing** - Animated axis movement
- **Probing** - Bed leveling visualization
- **QGL** - Quad gantry leveling
- **Heating** - Progress bars with temps
- **Print Complete** - Success animation (10s)
- **Tool Standby** - Tool-specific GIF loop

### Touch Interface
- **Swipe gestures** for navigation
- **Tap** for menu access
- **Long press** for settings
- **Multi-touch** capable
- **Instant wake** from sleep

---

## 🌐 Network Features

### WiFi Connectivity
- **Station Mode** (STA) - Connect to existing WiFi
- **Access Point Mode** (AP) - Create hotspot for setup
- **Web Portal** for configuration (192.168.4.1)
- **mDNS support** (`knomi-t0.local`)
- **Automatic reconnection** with exponential backoff

### HTTP API

#### Endpoints
```
POST /api/sleep          - Put display to sleep
POST /api/wake           - Wake display up
GET  /api/sleep/status   - Get current sleep state
```

#### Example Usage
```bash
# Sleep display
curl -X POST http://knomi-t0.local/api/sleep

# Wake display
curl -X POST http://knomi-t0.local/api/wake

# Check status
curl http://knomi-t0.local/api/sleep/status
# Returns: {"sleeping": false, "mode": "klipper_sync"}
```

### Security
- **Password masking** in logs (WiFi credentials)
- **No plaintext passwords** in serial output
- **Secure web interface** for configuration
- **Rate limiting** on API endpoints

---

## 🔌 Klipper Integration

### G-Code Macros

#### Basic Commands
```gcode
KNOMI_SLEEP          # Sleep all displays
KNOMI_WAKE           # Wake all displays
```

#### Advanced Multi-Display (Retry Logic)
```gcode
KNOMI_SLEEP_ALL_RETRY    # 3 retries per display
KNOMI_WAKE_ALL_RETRY     # Parallel execution
```

### Auto-Wake Integration
Displays automatically wake on:
```gcode
G28          # Homing
M109         # Wait for nozzle temp
M190         # Wait for bed temp
QUAD_GANTRY_LEVEL
BED_MESH_CALIBRATE
PRINT_START
```

### Status Reporting
- **Real-time print progress** (via Moonraker)
- **Temperature updates** (1Hz)
- **Layer information** from slicer
- **Print time remaining** (ETA)
- **Tool changes** reflected instantly

---

## 🎞️ Media Management

### Built-in GIFs (Firmware)
Located in `src/gif/`:
- `gif_homing.c` - Axes moving
- `gif_probing.c` - Bed mesh
- `gif_qgling.c` - Gantry leveling
- `gif_heated.c` - Heating indicator
- `gif_print.c` - Printing animation
- `gif_print_ok.c` - Success checkmark
- `gif_printed.c` - Completion screen
- `gif_voron.c` - VORON logo
- `gif_wifi.c` - WiFi connecting

### Filesystem GIFs (Runtime)
Located in `data/gifs/`:
- `tool_0.gif` through `tool_5.gif` - Tool-specific
- `print_progress_bg.gif` - Background animation

### Custom GIF Guidelines
**Specifications:**
- Resolution: 240x240 pixels
- Frame rate: 15-30fps recommended
- Duration: 2-5 seconds (looping)
- Format: GIF89a with transparency
- Size: <200KB recommended (<500KB max)

**Optimization:**
- Use [ezgif.com](https://ezgif.com/optimize) for compression
- Reduce colors to 64-128 palette
- Remove dithering for cleaner look
- Test on actual hardware before deploying

---

## 🛠️ Hardware Features

### Sensors

#### LIS2DW12 Accelerometer
- **Purpose:** Input shaper, vibration analysis
- **Axes:** X, Y, Z
- **Range:** ±2g / ±4g / ±8g / ±16g (configurable)
- **Sample rate:** Up to 1600Hz
- **Interface:** I2C
- **Status:** Displayed during printing

#### SHT4x Temp/Humidity
- **Temperature:** -40°C to +125°C (±0.2°C accuracy)
- **Humidity:** 0-100% RH (±1.8% accuracy)
- **Update rate:** 1Hz
- **Use cases:** 
  - Chamber monitoring
  - Filament drying tracking
  - Enclosure temperature

### Display Hardware

#### GC9A01 TFT Controller
- **Resolution:** 240x240 (round)
- **Colors:** 65K (16-bit RGB565)
- **Interface:** SPI (80MHz)
- **Refresh rate:** ~60fps
- **Viewing angle:** 170° (all directions)
- **Brightness:** 400 cd/m² typical

#### CST816S Touch Controller
- **Type:** Capacitive
- **Points:** 1 simultaneous touch
- **Gestures:** Swipe, tap, long-press
- **Response time:** <10ms
- **Interface:** I2C
- **Sleep current:** <2µA

### LEDs & Buttons (Optional)
**Note:** Camera and Buttons/LEDs share pins - choose one!

#### With `KNOMI_USE_BUTTONS_LEDS` defined:
- **Buttons:** 3x tactile switches
- **LEDs:** 3x RGB LEDs
- **Use cases:** Tool status, manual control

#### With `KNOMI_USE_CAMERA` defined:
- **Camera:** OV2640 (2MP)
- **Use cases:** Time-lapse, monitoring

---

## 📊 Performance

### Rendering
- **Framerate:** ~60fps typical
- **UI thread:** 10ms budget (LVGL)
- **Buffer:** Full-screen double-buffer (PSRAM)
- **Animations:** Smooth 30fps GIF playback

### Network
- **API latency:** <50ms (local network)
- **Moonraker polling:** 1Hz (configurable)
- **Multi-display sync:** <500ms (6 displays)
- **Retry success:** 95% (3 attempts)

### Memory Usage
- **Flash:** ~4MB firmware + 6MB filesystem
- **PSRAM:** ~2MB (GIFs, UI buffers)
- **SRAM:** ~200KB (stacks, variables)
- **Heap fragmentation:** <5% typical

---

## 🔧 Configuration

### Compile-Time Options

#### In `platformio.ini`:
```ini
build_flags = 
    -DKNOMIV2                    # Hardware version
    -DBOARD_HAS_PSRAM            # Enable PSRAM
    -Os                          # Optimize for size
```

#### In `src/pinout_knomi_v2.h`:
```cpp
#define KNOMI_USE_BUTTONS_LEDS   // Enable buttons/LEDs
// OR
#define KNOMI_USE_CAMERA         // Enable camera (mutually exclusive)
```

#### In `src/power_management/display_sleep.h`:
```cpp
#define DISPLAY_IDLE_TIMEOUT_SEC   60    // Idle after 60s
#define DISPLAY_SLEEP_TIMEOUT_SEC  300   // Sleep after 5min
#define DISPLAY_SLEEP_DELAY_AFTER_KLIPPER_IDLE_SEC 30  // Klipper sync delay
```

### Runtime Configuration

#### Via Web Interface:
- WiFi credentials
- Display hostname
- Backlight brightness
- Theme colors

#### Via G-Code:
```gcode
SET_KNOMI_MODE MODE=manual        # Set sleep mode
SET_KNOMI_TIMEOUT IDLE=30 SLEEP=120  # Adjust timeouts
KNOMI_ENABLE_SLEEP ENABLE=0       # Disable auto-sleep
```

---

## 🚀 Advanced Features

### Debug & Development

#### Serial Console
- **Baud rate:** 115200
- **Logs:** Color-coded by severity
- **Categories:** 
  - `[Display Sleep]` - Power management
  - `[FS]` - Filesystem operations
  - `[Moonraker]` - API communication
  - `[Touch]` - Input events
  - `[LVGL]` - UI rendering

#### OTA Updates (Optional)
```cpp
// Uncomment in platformio.ini
upload_protocol = custom
upload_url = http://knomi-t0.local/update
```

### Multi-Language Support (Planned)
- English (current)
- German (planned)
- French (planned)
- Spanish (planned)
- Chinese (planned)

### Themes (Planned)
- VORON Red (default)
- Classic Blue
- Midnight Dark
- Custom RGB

---

## 📈 Roadmap

### v3.2 (Q1 2026)
- [ ] Deep sleep mode (WiFi off)
- [ ] Adaptive timeouts (learning)
- [ ] MQTT integration
- [ ] Multi-language UI

### v3.3 (Q2 2026)
- [ ] OTA updates over WiFi
- [ ] Theme editor (web interface)
- [ ] Custom status screens
- [ ] Widget system

### v4.0 (Q3 2026)
- [ ] Voice control integration
- [ ] Camera support (time-lapse)
- [ ] Mobile app companion
- [ ] Cloud sync (optional)

---

## 🎯 Use Cases

### 1. Multi-Color Printing
- Display per toolhead shows active tool
- Independent status for each color
- Track temperatures per extruder
- Visual confirmation of tool changes

### 2. Print Farm
- Monitor multiple printers from one location
- Synchronized sleep saves power
- Remote wake via API
- Consistent UI across all machines

### 3. Development & Testing
- Serial debugging with live UI
- API testing without Klipper
- Rapid prototyping of new screens
- Isolated testing per display

### 4. Integration Projects
- Home automation (Home Assistant)
- LED synchronization
- Custom notification system
- Event-driven displays

---

## 💡 Tips & Tricks

### Optimization
1. **Reduce GIF sizes** for faster loading
2. **Use Klipper sync mode** for best power savings
3. **Adjust brightness** to reduce eye strain
4. **Enable serial logs** only when debugging

### Troubleshooting
1. **mDNS not working?** Use IP addresses directly
2. **Displays not syncing?** Increase retry count
3. **Wake-up unreliable?** Check Moonraker connection
4. **High power usage?** Verify sleep is actually active

### Best Practices
- Name displays consistently (`knomi-t0`, `knomi-t1`, etc.)
- Keep WiFi signal strong (avoid metal enclosures blocking)
- Update all displays to same firmware version
- Backup configuration before major changes
- Test sleep/wake after firmware updates

---

**For detailed implementation and setup, see:**
- [README.md](README.md) - Main documentation
- [CHANGELOG.md](CHANGELOG.md) - Version history
- [docs/](docs/) - Detailed guides

---

**Last Updated:** January 28, 2025  
**Version:** 3.1.0
