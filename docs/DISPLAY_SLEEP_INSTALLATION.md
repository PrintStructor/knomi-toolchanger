# KNOMI Display Sleep Mode - Installation & Usage (End Users)

> Goal: simple, repeatable setup for users who just want the sleep feature. No C++ edits required.  
> Note: With LittleFS this firmware does not support web/OTA uploads. Flash via USB using PlatformIO (below) or the Espressif Flash Download Tool on Windows.

---

## What the Sleep System Does
- Two-stage flow: **Idle** (shows tool GIF) → **Sleep** (backlight off, hardware sleep)
- LVGL timers pause to reduce CPU load; WiFi stays alive for Moonraker polling
- Wakes on: touch, print start, homing/probing/QGL, nozzle/bed heating

---

## Requirements
- KNOMI V2 hardware
- Moonraker reachable from the display
- This firmware repository (firmware branch)
- [PlatformIO](https://platformio.org/) installed

---

## Build & Flash

```bash
git clone https://github.com/PrintStructor/knomi-toolchanger.git
cd knomi-toolchanger
git checkout firmware

# Optional but recommended when pulling fresh sources
pio run -e knomiv2 --target clean

# Build firmware
pio run -e knomiv2

# Upload filesystem (GIFs) if you changed data/
pio run -e knomiv2 --target uploadfs

# Flash firmware
pio run -e knomiv2 --target upload
```

---

## Adjust Timeouts (optional)
File: `src/power_management/display_sleep.h`

```cpp
#define DISPLAY_IDLE_TIMEOUT_SEC   60   // Idle after 60s
#define DISPLAY_SLEEP_TIMEOUT_SEC  300  // Sleep after 5min
```

Change only these two values for user-level tuning.

---

## How to Test
- **Idle → Sleep:** Leave the display untouched; expect Idle after the idle timeout, full black screen after the sleep timeout.
- **Wake on touch:** Tap the screen while sleeping; it should wake immediately.
- **Wake on printer activity:** Start a print or heat nozzle/bed; the display should wake without touch.
- **Logs:** Serial monitor at 115200 baud should show `[Display Sleep]` messages for state changes.

---

## Troubleshooting
- **Never enters sleep:** Confirm Moonraker connection, leave the screen untouched for the full timeout, and watch for `[Display Sleep]` logs.
- **Does not wake:** Test a touch wake first; if that works but prints don’t wake it, verify Moonraker URL/connectivity.
- **Build/flash issues:** Run `pio run -e knomiv2 --target clean` before rebuilding and ensure you are on the `firmware` branch.

---

## What’s Next?
- For Klipper/Moonraker integration details (status fields, wake triggers): see `docs/DISPLAY_SLEEP_KLIPPER_INTEGRATION.md`.
- For the temp-graph ring overlay fix (optional): see `docs/TEMP_GRAPH_FIX.md`.
- For developer/porting details (hooks, files, code paths): see `docs/dev/DISPLAY_SLEEP_IMPLEMENTATION.md`.
