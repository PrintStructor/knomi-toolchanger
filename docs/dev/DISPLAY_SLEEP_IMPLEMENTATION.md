# KNOMI Display Sleep Implementation (Developers)

For firmware developers and porters. This documents the low-level hooks, touched files, and expected control flow for the two-stage sleep system (Idle → Sleep with hardware display sleep and LVGL pause).

---

## Touched Files
- `src/power_management/display_sleep.{h,cpp}` - Sleep state machine, timers, hardware sleep/wake, logging.
- `src/ui_overlay/lv_auto_goto_idle.cpp` - Calls `display_sleep_update()` every loop and skips UI work while sleeping.
- `src/lvgl_hal.cpp` - Touch handler resets the sleep timer on any finger event.
- `src/ui_overlay/lv_moonraker_change_screen.cpp` - Tracks printer status changes and triggers wake on activity.
- `src/lvgl_usr.cpp` - Initializes the sleep system during UI startup.
- `platformio.ini` - `build_src_filter` must include `power_management/*.cpp`.

---

## Sleep Engine Overview
- States: `ACTIVE` → `IDLE` (tool GIF) → `SLEEP` (backlight off, GC9A01 SLPIN, LVGL timers paused).
- Timers (seconds) in `display_sleep.h`:
  - `DISPLAY_IDLE_TIMEOUT_SEC` - Transition ACTIVE → IDLE.
  - `DISPLAY_SLEEP_TIMEOUT_SEC` - Transition IDLE → SLEEP.
- Wake sources:
  - Touch input (resets timers and wakes if sleeping).
  - Printer activity via Moonraker: printing, homing, probing, QGL, nozzle heating, bed heating.
  - Direct API calls (`display_force_wake()` etc., if used by host code).
- Logs: `[Display Sleep] ...` emitted for state changes and blocking conditions.

---

## Integration Points (code-level)

### UI loop guard
File: `src/ui_overlay/lv_auto_goto_idle.cpp`
```cpp
#include "../power_management/display_sleep.h"

void lv_loop_auto_idle(wifi_status_t status) {
    display_sleep_update();          // handles idle/sleep transitions
    if (display_is_sleeping()) {
        return;                      // skip UI work while asleep
    }
    // existing UI logic continues...
}
```

### Touch wake
File: `src/lvgl_hal.cpp`
```cpp
#include "power_management/display_sleep.h"

void usr_touchpad_read(struct _lv_indev_drv_t * indev_drv, lv_indev_data_t * data) {
    // ... existing touch read ...
    if (event.finger) {
        touch_idle_time_clear();
        display_sleep_reset_timer(); // clears timers + wakes if needed
    }
}
```

### Printer activity wake
File: `src/ui_overlay/lv_moonraker_change_screen.cpp`
```cpp
#include "../power_management/display_sleep.h"

void lv_loop_moonraker_change_screen(void) {
    static bool last_printing = false;
    static bool last_homing = false;
    static bool last_probing = false;
    static bool last_qgling = false;
    static bool last_heating_nozzle = false;
    static bool last_heating_bed = false;

    const bool status_changed =
        moonraker.data.printing != last_printing ||
        moonraker.data.homing != last_homing ||
        moonraker.data.probing != last_probing ||
        moonraker.data.qgling != last_qgling ||
        moonraker_nozzle_is_heating() != last_heating_nozzle ||
        moonraker_bed_is_heating() != last_heating_bed;

    last_printing = moonraker.data.printing;
    last_homing = moonraker.data.homing;
    last_probing = moonraker.data.probing;
    last_qgling = moonraker.data.qgling;
    last_heating_nozzle = moonraker_nozzle_is_heating();
    last_heating_bed = moonraker_bed_is_heating();

    display_check_wake_condition(status_changed);
    if (display_is_sleeping()) {
        return;                      // block screen updates while asleep
    }
    // existing screen update logic continues...
}
```

### Initialization
File: `src/lvgl_usr.cpp`
```cpp
#include "power_management/display_sleep.h"

void lvgl_ui_task(void * parameter) {
    lv_btn_init();
    lvgl_hal_init();
    display_sleep_init();            // sets up timers + hardware state
    ui_init();
    // ...
}
```

### Build inclusion
File: `platformio.ini`
```ini
build_src_filter =
    +<*>
    +<gif/*.c>
    +<ui/**/*.c>
    +<power_management/*.cpp>
```

---

## Loop Sequence (LVGL task)
1. `display_sleep_update()` evaluates timers and transitions states.
2. On SLEEP: LVGL timers pause, backlight off, GC9A01 SLPIN, UI loop returns early.
3. On WAKE: sleep module re-inits display, resumes LVGL timers, clears timers.
4. UI logic runs only while not sleeping.

---

## Developer Notes
- Wake checks rely on Moonraker fields: `printing`, `homing`, `probing`, `qgling`, `moonraker_nozzle_is_heating()`, `moonraker_bed_is_heating()`.
- Touch events must always call `display_sleep_reset_timer()` to avoid sticky idle timers.
- If you change LVGL loop structure, keep the early-return guard close to the top to minimize render work while sleeping.
- If porting to other boards, verify GC9A01 timing in `display_sleep.cpp` (SLPIN/SLPOUT delays and re-init).
- Default timeouts are intentionally conservative; expose overrides via `display_sleep.h` only for user-level tuning.

---

## Verification Checklist
- Build includes `power_management/*.cpp` (no missing-symbol errors).
- Logs show `[Display Sleep] Idle timeout: ...` on boot.
- Touch wake works from SLEEP state.
- Printer activity wake works when any tracked flag toggles.
- UI loop does not render while sleeping (CPU drop, backlight off).
