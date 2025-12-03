# WiFi Troubleshooting & Recovery

This guide covers WiFi connection issues, automatic reconnection logic, and remote recovery options for KNOMI V2 displays.

---

## 🔍 Common WiFi Issues

### Display Becomes Unresponsive

**Symptoms:**
- Display loses WiFi connection after some time
- Cannot ping hostname (`knomi-t0.local` not responding)
- Web interface unreachable
- API endpoints timeout

**Root Cause:**
- Router drops connection
- WiFi interference
- Power saving mode in router
- DHCP lease expiration
- Signal quality issues

---

## ✅ Automatic Reconnection (v1.1.0+)

The firmware now includes automatic WiFi reconnection logic that activates when a disconnect is detected.

### How It Works

```
1. WiFi Disconnect Detected
   ↓
2. Immediate Reconnect Attempt (0s delay)
   ↓
3. If failed → Wait 5s → Retry (Attempt 2/3)
   ↓
4. If failed → Wait 5s → Retry (Attempt 3/3)
   ↓
5. If all failed → Switch to AP Mode for manual intervention
```

### Configuration

**Default Settings** (in [src/wifi_setup.cpp](../../src/wifi_setup.cpp:336)):
```cpp
const uint32_t RECONNECT_INTERVAL = 5000;  // 5 seconds between attempts
const uint8_t MAX_RECONNECT_ATTEMPTS = 3;  // 3 total attempts
```

### Serial Monitor Output

When WiFi disconnect occurs, you'll see:
```
WiFi disconnected! Starting auto-reconnect...
WiFi reconnect attempt 1/3...
WiFi reconnect attempt 2/3...
WiFi reconnect attempt 3/3...
```

**On success:**
```
Connected!
```

**On failure:**
```
Max reconnect attempts reached. Switching to AP mode.
```

### Fallback to AP Mode

After 3 failed reconnect attempts, the display automatically:
1. Switches to **Access Point mode**
2. Creates WiFi network: `KNOMI_AP_XXXXX`
3. Allows manual WiFi reconfiguration via web interface
4. Display remains functional but not connected to network

**To reconfigure:**
1. Connect to `KNOMI_AP_XXXXX` WiFi
2. Open `http://192.168.4.1` in browser
3. Enter new WiFi credentials
4. Display reconnects to network

---

## 🔧 Manual Recovery Options

### Option 1: Remote Restart (Recommended)

**New in v1.1.0:** Restart display remotely without power cycling.

#### Single Display
```bash
curl -X POST http://knomi-t0.local/api/restart
```

**Response:**
```json
{"status":"restarting"}
```

Display restarts in ~2 seconds and reconnects to WiFi automatically.

#### All 6 Displays (Parallel)
```bash
for i in {0..5}; do
  curl -X POST http://knomi-t$i.local/api/restart &
done
wait
echo "All displays restarting..."
```

#### From Klipper Macro
```gcode
[gcode_shell_command restart_knomi]
command: curl -X POST http://knomi-t0.local/api/restart
timeout: 5.0

[gcode_macro RESTART_KNOMI]
description: Restart KNOMI display via HTTP API
gcode:
    {% set tool = params.TOOL|default(0)|int %}
    RUN_SHELL_COMMAND CMD=restart_knomi PARAMS="http://knomi-t{tool}.local/api/restart"
    M117 Restarting KNOMI T{tool}
```

**Usage:**
```gcode
RESTART_KNOMI TOOL=0
```

### Option 2: Power Cycle

If remote restart fails (display completely unresponsive):

1. **Physical power disconnect**
   - Remove USB-C cable or cut 5V power
   - Wait 5 seconds
   - Reconnect power

2. **Via Klipper power management** (if wired to relay):
   ```gcode
   SET_PIN PIN=knomi_power VALUE=0  # Power off
   G4 P5000                          # Wait 5 seconds
   SET_PIN PIN=knomi_power VALUE=1  # Power on
   ```

---

## 🌐 HTTP API Reference

### POST `/api/restart`

Restart the ESP32 without power cycling.

**Request:**
```bash
curl -X POST http://knomi-t0.local/api/restart
```

**Response:**
```json
{
  "status": "restarting"
}
```

**Status Codes:**
- `200 OK` - Restart initiated successfully
- `504 Gateway Timeout` - Display already offline (use power cycle)

**Implementation:** [src/webserver.cpp:268-272](../../src/webserver.cpp#L268-L272)

### GET `/api/sleep/status`

Check if display is sleeping (useful for diagnostics).

**Request:**
```bash
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

**States:**
- `active` - Display is on and responsive
- `idle` - Display idle but awake
- `sleeping` - Display in sleep mode

**Modes:**
- `manual` - Time-based sleep
- `klipper_sync` - Follows Klipper idle state
- `led_sync` - Mirrors case LED status

---

## 🔬 Debugging WiFi Issues

### Serial Monitor Diagnostics

Connect to ESP32 serial port at **115200 baud**:

```bash
# Using PlatformIO
pio device monitor -b 115200

# Using screen (macOS/Linux)
screen /dev/ttyUSB0 115200

# Using Arduino IDE
Tools → Serial Monitor → 115200 baud
```

**Key messages to watch:**

```
✅ Good:
WiFi connected!
sta ip: 192.168.1.100
Moonraker connection established

❌ Problems:
WiFi disconnected! Starting auto-reconnect...
Max reconnect attempts reached. Switching to AP mode.
sta connect failed!!!
```

### Network Diagnostics

#### 1. Check if display is reachable
```bash
ping knomi-t0.local
```

**Expected:** `64 bytes from ...` (alive)
**Problem:** `Request timeout` (offline)

#### 2. Check DNS resolution
```bash
# macOS/Linux
avahi-browse -r _http._tcp

# Windows (requires Bonjour)
dns-sd -B _http._tcp
```

Should show: `knomi-t0._http._tcp`

#### 3. Check WiFi signal strength
```bash
curl http://knomi-t0.local/api/sleep/status
```

If this works but display is slow → WiFi signal issue.

#### 4. Router logs

Check DHCP server logs for:
- MAC address: `CC:BA:97:19:DA:D4` (example from your setup)
- Hostname: `knomi-t0`
- IP assignment conflicts

---

## 🛠️ Advanced Troubleshooting

### Issue: Display reconnects but immediately disconnects again

**Cause:** Router-side issue (MAC filtering, client isolation, band steering)

**Fix:**
1. **Disable band steering** - Force 2.4GHz only
2. **Add MAC to whitelist** if MAC filtering enabled
3. **Disable client isolation** on router
4. **Reserve DHCP IP** for each KNOMI MAC address

### Issue: All 6 displays disconnect at the same time

**Cause:** Router overload or DHCP pool exhaustion

**Fix:**
1. Increase DHCP pool size (need 6+ IPs for KNOMIs)
2. Use static IP assignment for KNOMIs
3. Upgrade router firmware
4. Consider separate VLAN for toolchanger devices

### Issue: Auto-reconnect works but Moonraker connection fails

**Cause:** Moonraker IP/port changed or became unreachable

**Fix:**
1. Check Moonraker IP in web config: `http://knomi-t0.local`
2. Update IP if changed
3. Verify Moonraker is running: `curl http://192.168.1.10/server/info`

---

## 📊 Reconnection Statistics

Monitor reconnection behavior via serial output:

```cpp
// In wifi_task() main loop
static uint32_t last_reconnect_attempt = 0;
static uint8_t reconnect_attempts = 0;
```

**Healthy network:**
- `reconnect_attempts` stays at `0`
- No "WiFi disconnected!" messages

**Unstable network:**
- Frequent reconnect attempts
- `reconnect_attempts` > 0 often
- May need to improve WiFi signal or router settings

---

## 🔗 Related Documentation

- [DISPLAY_SLEEP_IMPLEMENTATION.md](DISPLAY_SLEEP_IMPLEMENTATION.md) - Power management and sleep API
- [src/wifi_setup.cpp](../../src/wifi_setup.cpp) - WiFi connection logic
- [src/webserver.cpp](../../src/webserver.cpp) - HTTP API endpoints

---

## 📝 Version History

| Version | Changes |
|---------|---------|
| v1.1.0 | Added automatic WiFi reconnection (3 attempts, 5s interval) |
| v1.1.0 | Added `/api/restart` endpoint for remote restart |
| v1.0.0 | Initial release - manual power cycle required |

---

**Last Updated:** December 3, 2025
**Author:** PrintStructor
