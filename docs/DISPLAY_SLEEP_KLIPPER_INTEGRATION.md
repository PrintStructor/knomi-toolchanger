# KNOMI Display Sleep - Klipper & Moonraker Integration (Power Users)

How the sleep system interacts with Klipper/Moonraker and what you need to configure so wake-on-activity works reliably.

---

## What the Firmware Reads from Moonraker
- `/api/printer` → `state.flags.printing|cancelling|paused` and heater temps (used to detect active printing and active heating).
- `/printer/objects/query?idle_timeout` → `idle_timeout.state` (used when you enable Klipper-synced sleep).
- `/printer/objects/query?gcode_macro _KNOMI_STATUS` → custom flags the macros expose:
  - `homing`, `probing`, `qgling`
  - `heating_nozzle`, `heating_bed`
- Diagram placeholder: `docs/images/klipper_flags.png`

These fields drive wake decisions in `lv_moonraker_change_screen.cpp`.

---

## Required Klipper Config
1) **Include the KNOMI macro file**
- Copy `KNOMI.cfg` into your Klipper config directory and include it from `printer.cfg`:
  ```ini
  [include KNOMI.cfg]
  ```
- `_KNOMI_STATUS` is defined in `KNOMI.cfg` and receives updates from the macro overrides below.

2) **Macro overrides that set the flags**
- `G28` sets `homing` (defined in KNOMI.cfg)
- `BED_MESH_CALIBRATE` sets `probing` (defined in KNOMI.cfg)
- `M109` sets `heating_nozzle` (defined in KNOMI.cfg)
- `M190` sets `heating_bed` (defined in KNOMI.cfg)
- For `QUAD_GANTRY_LEVEL`, `PRINT_START`, `PRINT_END`, etc.: See `klipper_integration_example.cfg` for integration examples to add to your existing macros.
- Printing state is also read directly from Moonraker.

3) **Network reachability**
- KNOMI must reach Moonraker over HTTP. Verify with:
  ```bash
  curl http://<printer-host>/printer/objects/query?webhooks
  ```
- If you use hostnames (`knomi-t0.local`, etc.), ensure mDNS works on your network.

---

## Optional: Klipper Idle Sync
- The firmware polls `idle_timeout.state`; when it sees `Idle`, it can enter sleep after the configured delay.
- Ensure Klipper has an `idle_timeout` section (default 600s). Example:
  ```ini
  [idle_timeout]
  timeout: 300
  ```
- This is independent of the manual idle/sleep timers in firmware; it simply provides an extra wake/sleep hint.

---

## How to Verify the Flags
- Query the macro status directly:
  ```bash
  curl "http://<printer-host>/printer/objects/query?gcode_macro%20_KNOMI_STATUS"
  ```
  You should see `true/false` for `homing`, `probing`, `qgling`, `heating_nozzle`, `heating_bed`.
- Run each action (home, QGL, heat bed/nozzle) and confirm the flag flips and the display wakes if it was sleeping.
- Start a print and ensure `printing` is `true` in `/api/printer`; the display should never enter sleep while printing.

---

## Common Issues
- **Display never wakes on printer activity:** `_KNOMI_STATUS` is missing (files not included) or Moonraker URL/hostname is wrong.
- **Sleep triggers immediately after Klipper restart:** `idle_timeout` state may be stale; ensure the display can query it and that Klipper is `ready`.
- **Heater wake unreliable:** Check `M109`/`M190` overrides are active and that the HTTP query above shows `heating_nozzle` / `heating_bed` toggling.
- **Moonraker unreachable from KNOMI:** Fix WiFi credentials on the display, ensure the printer and KNOMI are on the same network, and test with `curl` or ping.

---

For deeper firmware hooks or porting notes, see `docs/dev/DISPLAY_SLEEP_IMPLEMENTATION.md`.
