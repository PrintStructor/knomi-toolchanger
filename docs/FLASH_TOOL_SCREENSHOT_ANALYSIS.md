# Flash Tool Screenshot Analysis

## ✅ Status: Flash successful!

Your screenshot shows **"FINISH"** (完成) – the KNOMI V2 flashed correctly.

---

## 📊 Your configuration

### File offsets

| File | Your Offset | Recommended | Status |
|------|-------------|-------------|--------|
| `bootloader.bin` | `0x0000` | `0x1000` | ⚠️ Works, but not the standard offset |
| File 2 (partitions?) | `0x8000` | `0x8000` | ✅ Correct |
| File 3 (firmware?) | `0x10000` | `0x10000` | ✅ Correct |
| File 4 (littlefs?) | `0x710000` | `0x710000` | ✅ Correct |

### Flash settings

| Setting | Yours | Recommended | Performance impact |
|---------|-------|-------------|--------------------|
| **SPI SPEED** | `40MHz` | `80MHz` | ⚠️ ~50% slower |
| **SPI MODE** | `DIO` | `QIO` | ⚠️ Half the data lines |
| **BAUD** | `921600` | `921600` | ✅ Optimal |
| **DoNotChgBin** | ✅ On | ☐ Off | ⚠️ Usually not needed |

### Detected hardware

```
Flash Vendor: C8h (GigaDevice)
Flash DevID: 4018h
Flash Size: QUAD 16MB ✅
Crystal: 40MHz
```

**MAC addresses:**
- AP: `CCBA9719DAD5`
- STA: `CCBA9719DAD4`
- BT: `CCBA9719DAD6`
- ETHERNET: `CCBA9719DAD7`

---

## 🎯 Why it works despite differences

### 1) Bootloader at 0x0000 instead of 0x1000

Normally:
- ESP32-S3 reserves 0x0000–0x0FFF
- Bootloader typically starts at 0x1000

On KNOMI V2:
- BTT may ship a layout that also works from 0x0000, or an existing bootloader at 0x1000 remains intact.

Recommendation:
- If it works, you can leave it ✅
- If you hit boot issues later, try placing bootloader at 0x1000

### 2) SPI SPEED 40MHz instead of 80MHz

Effect:
- Boot: slightly slower (not really noticeable)
- Runtime: none (runtime executes from RAM)

Pros of 40MHz:
- ✅ More tolerant of cables/EMI
- ✅ Compatible with more flash chips

Recommendation:
- For stability: 40MHz ✅
- For max speed: 80MHz

### 3) SPI MODE DIO instead of QIO

DIO:
- Uses 2 data lines; stable; fast enough for KNOMI.

QIO:
- Uses 4 data lines; about 2x faster; requires compatible flash.

Your chip (GigaDevice C8h) supports QIO ✅

Recommendation:
- For stability: DIO ✅
- For max speed: QIO

---

## 🚀 Optimized settings (optional)

If you reflash and want maximum speed:
```
SPI SPEED: 80MHz  (instead of 40MHz)
SPI MODE: QIO     (instead of DIO)
DoNotChgBin: ☐    (turn off)

Offsets stay the same:
0x0000  - bootloader.bin  (works for your board)
0x8000  - partitions.bin
0x10000 - firmware.bin
0x710000 - littlefs.bin
```

**Gain:**
- Flash reads ~2x faster
- Boot ~0.5s faster
- Runtime unchanged

---

## 🔍 Verification

### 1) Display test
- ✅ Boots and shows logo/GIF
- ✅ Touch responds
- ✅ WiFi AP mode starts (`KNOMI_AP_XXXXX`)
- ✅ Temps display after Klipper connection

### 2) Serial output
Connect at 115200 baud and expect:
```
[Boot] KNOMI V2 Firmware v1.0.0
[Boot] ESP32-S3-R8 (16MB Flash, 8MB PSRAM)
[WiFi] Starting AP mode
```

### 3) WiFi setup
1. Connect to `KNOMI_AP_XXXXX`
2. Open `192.168.4.1`
3. Enter WiFi credentials
4. KNOMI reboots and joins

### 4) Network test (after WiFi setup)

```bash
# Hostname test
ping knomi-t0.local

# API test
curl http://knomi-t0.local/api/sleep/status
```

---

## ⚡ Summary

Your config:
- ✅ Works (FINISH confirmed)
- ⚠️ Not performance-optimal
- ✅ Very stable (conservative settings)

Next steps:
1. ✅ Flash succeeded – display should boot
2. ✅ Configure WiFi via AP mode
3. ✅ Connect to Moonraker/Klipper
4. (Optional) Next flash: 80MHz/QIO for speed

If issues:
- No boot → check power
- No AP → check serial output
- Touch issues → use web UI to calibrate

---

## 📞 Support

If problems persist:
1. Check serial output (115200 baud)
2. Try optimized settings (80MHz/QIO)
3. Open a GitHub issue with screenshot and serial log

**GitHub:** https://github.com/PrintStructor/knomi-toolchanger/issues

---

**Created:** 28 January 2025  
**Based on:** ESP32 Flash Download Tool v3.9.5 screenshot
