# ESP32 Flash Download Tool - KNOMI V2 Guide

Detailed steps to flash KNOMI V2 firmware with the Espressif Flash Download Tool (Windows-only).
> Note: OTA/web upload is not supported with LittleFS. Use USB flashing (this tool on Windows, or `pio run -e knomiv2 --target upload/uploadfs` on macOS/Linux).

---

## 📋 Prerequisites

### Software
- **ESP32 Flash Download Tool v3.9.5** (or newer)
  - Download: [Espressif Official](https://www.espressif.com/en/support/download/other-tools)
  - Alternative: [GitHub Releases](https://github.com/espressif/esptool/releases)

### Hardware
- **BTT KNOMI V2 display** with ESP32-S3-R8
- **USB-C data cable**
- **Windows PC** (the Flash Download Tool is Windows-only)

---

## 🖼️ Screenshot Reference

![ESP32 Flash Tool Settings](https://raw.githubusercontent.com/PrintStructor/knomi-toolchanger/firmware/docs/images/flash_tool_example.png)

> The shown configuration works but is not optimal. See recommended settings below.

### Firmware files
Obtain these 4 files from [Release v1.1.0](https://github.com/PrintStructor/knomi-toolchanger/releases/tag/v1.1.0):

1. `bootloader.bin`
2. `partitions.bin`
3. `firmware.bin`
4. `littlefs.bin`

---

## 🔧 Flash Download Tool Configuration

### Step 1: Start the tool
1. Extract the ESP32 Flash Download Tool.
2. Launch `flash_download_tool_3.9.5.exe`.
3. Select **ChipType: ESP32-S3**.
4. Select **WorkMode: Develop**.

### Step 2: Configure files and offsets
**Important:** Offsets must match exactly.

| File | Offset (Hex) | Checkbox | Description |
|------|--------------|----------|-------------|
| `bootloader.bin` | `0x0000` | ✅ | ESP32-S3 bootloader |
| `partitions.bin` | `0x8000` | ✅ | Partition table |
| `firmware.bin` | `0x10000` | ✅ | Main firmware |
| `littlefs.bin` | `0x710000` | ✅ | Filesystem (GIFs) |

⚠️ **Critical:** These offsets are required for KNOMI V2 to boot correctly.

Reference layout:
```
[✅] bootloader.bin  @ 0x0000
[✅] partitions.bin  @ 0x8000
[✅] firmware.bin    @ 0x10000
[✅] littlefs.bin    @ 0x710000
```

### Step 3: Flash settings

**Recommended (optimal performance):**
| Setting | Value | Why |
|---------|-------|-----|
| **SPI SPEED** | `80MHz` | Fastest for KNOMI V2 |
| **SPI MODE** | `QIO` | ⚠️ **REQUIRED for full speed** - 4 data lines |
| **FLASH SIZE** | `16MB` | Detected automatically |
| **BAUD** | `921600` | Fast upload |

**Alternative (slower display performance):**
| Setting | Value | Notes |
|---------|-------|-------|
| **SPI SPEED** | `40MHz` | Works but slower |
| **SPI MODE** | `DIO` | Works but display runs slower (2 data lines) |

> **Important:** DIO mode works but causes reduced display performance. Always use QIO for best results!

### Step 4: Flash options

| Option | Recommended | Notes |
|--------|-------------|-------|
| **DoNotChgBin** | ☐ | Usually leave off |
| **CombineBin** | ☐ | Not needed here |
| **LockSettings** | ☐ | Leave off |

---

## ⚡ Flash Process

### 1) Prepare the KNOMI
1. Connect KNOMI via USB-C to your PC.
2. Hold **BOOT** (if present).
3. Tap **RESET** while holding BOOT.
4. Release BOOT → ESP32 enters download mode.  
   (Some boards enter download mode automatically when USB is connected.)

### 2) Identify COM port
- Windows Device Manager → “Ports (COM & LPT)” → note the COM number.
- Select that COM port in the Flash Tool dropdown.

### 3) Start flashing
1. Click **START**.
2. Progress should show writing at each offset and finish with **FINISH**.
3. Typical duration: ~2–4 minutes depending on baud/USB.

### 4) Finish
1. When you see **FINISH** in green, flashing succeeded.
2. Unplug USB, plug back in → KNOMI boots the new firmware.

---

## 🐛 Troubleshooting

- **"Connecting... timeout":** ESP32 not in download mode. Unplug USB, hold BOOT, plug in, release BOOT after ~2s, click START again.
- **"Flash size mismatch":** Set **FLASH SIZE: 16MB** (KNOMI V2 is 16MB).
- **"Write failed at 0xXXXXX":** Try another USB data cable/port; lower baud to `460800` or `115200`.
- **"COM Port not found":** Install the USB-UART driver for your board (CP210x or CH340, depending on variant).
- **"Invalid partition table":** Ensure `partitions.bin` is at `0x8000` and other offsets match the table above.

---

## 📊 Post-Flash Verification

### 1) Serial monitor (optional)
Connect at 115200 baud and check boot logs:

```bash
# Using PlatformIO
pio device monitor -b 115200

# Using Arduino IDE
Tools → Serial Monitor → 115200 baud
```

Expected output:
```
[Boot] KNOMI V2 Firmware v1.0.0
[Boot] ESP32-S3-R8 (16MB Flash, 8MB PSRAM)
[WiFi] Booting into AP mode: KNOMI_AP_XXXXX
[Display] GC9A01 initialized
[FS] LittleFS mounted successfully
[FS] Loading GIF for tool X from /gifs/tool_X.gif
```

### 2) WiFi setup
1. After flashing, KNOMI starts in **AP mode**: `KNOMI_AP_XXXXX`.
2. Connect to that WiFi.
3. Open `192.168.4.1`.
4. Enter your WiFi credentials.
5. KNOMI reboots and joins your network.

### 3) Functional check

Display:
- ✅ Touch responds
- ✅ Standby GIF shows
- ✅ Temperature display works

Network:
```bash
# Ping hostname (after WiFi setup)
ping knomi-t0.local

# API test
curl http://knomi-t0.local/api/sleep/status
```

---

## 🔄 Alternative: Combined Binary Flash

If you flash often, you can create a combined binary:

### Create combined binary
1. In the Flash Download Tool:
   - Enable **CombineBin** ☑
   - Configure all 4 files as above
   - Click **CombineBin** (bottom right)
2. It creates a file like:
   - `combined_0x0.bin` (~16 MB)
3. Flash in one go:
   ```bash
   esptool.py --chip esp32s3 --port COM7 write_flash 0x0 combined_0x0.bin
   ```

**Pros:** Faster flashing, single file  
**Cons:** Larger file, always a full flash

---

## 📁 Recommended File Organization

Suggested folder structure:

```
KNOMI_Flash/
├── v1.1.0/
│   ├── bootloader.bin
│   ├── partitions.bin
│   ├── firmware.bin
│   └── littlefs.bin
├── flash_download_tool_3.9.5.exe
└── README.txt (this guide)
```

---

## ⚙️ Firmware-Only Update (Keep LittleFS)

If you only want to update firmware and keep existing GIFs/WiFi settings:

**Flash only this file:**
- `firmware.bin` @ `0x10000`

**Disable the others:**
- ☐ bootloader.bin
- ☐ partitions.bin
- ☐ littlefs.bin

**Benefit:** Faster update (~30 seconds)
**Preserves:** WiFi credentials and custom GIFs

---

## 🚀 Advanced Options

### DoNotChgBin Explained

**When enabled (☑):**
- Original binary is used (no modification)
- Required for signed firmware

**For KNOMI V2:** Usually leave disabled (☐)

### SPI Mode Explained

| Mode | Description | Speed |
|------|-------------|-------|
| QIO | Quad I/O (4 data lines) | **Fastest** |
| QOUT | Quad Output | Fast |
| DIO | Dual I/O | Standard |
| DOUT | Dual Output | Slow |

**Recommendation:** Use QIO (as configured in platformio.ini)

### Baud Rate Selection

| Baud Rate | Duration | Stability |
|-----------|----------|-----------|
| 921600 | ~2 min | Good (recommended) |
| 460800 | ~3 min | Very good |
| 115200 | ~10 min | Excellent (for troubleshooting) |

---

## 📞 Support

**If you encounter issues:**
1. Verify all offsets are correct
2. Use a quality USB data cable
3. Reduce baud rate to 115200
4. Check Device Manager for COM port

**GitHub Issues:** https://github.com/PrintStructor/knomi-toolchanger/issues

---

## 📄 References

- [Espressif Flash Download Tool Documentation](https://www.espressif.com/sites/default/files/tools/flash_download_tool_v3.9.5_0.pdf)
- [ESP32-S3 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- [KNOMI V2 GitHub Repository](https://github.com/PrintStructor/knomi-toolchanger)

---

**Version:** 1.1.0
**Last Updated:** December 5, 2025
**Author:** PrintStructor
