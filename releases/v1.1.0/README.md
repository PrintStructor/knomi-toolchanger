# KNOMI V2 Firmware v1.1.0 - Pre-compiled Binaries

This folder contains pre-compiled firmware binaries for KNOMI V2 displays (ESP32-S3-R8).

## 📦 What's Included

| File | Size | Description |
|------|------|-------------|
| `firmware.bin` | 2.8MB | Main application firmware |
| `bootloader.bin` | 15KB | ESP32-S3 bootloader |
| `partitions.bin` | 3.0KB | Partition table (16MB flash layout) |
| `littlefs.bin` | 8.9MB | Filesystem with GIF animations |

## ✨ What's New in v1.1.0

### WiFi Connectivity Improvements

**Automatic Reconnection:**
- Display automatically reconnects when WiFi connection is lost
- 3 reconnection attempts with 5-second intervals
- Falls back to AP mode after max attempts for manual configuration

**Remote Restart API:**
- New `/api/restart` endpoint for remote display restart
- No power cycling required when display becomes unresponsive
- Useful for batch operations across all 6 displays

See [WIFI_TROUBLESHOOTING.md](../../docs/dev/WIFI_TROUBLESHOOTING.md) for detailed documentation.

## 🔧 How to Flash

### Option 1: Using esptool.py (Recommended)

**Install esptool:**
```bash
pip install esptool
```

**Flash all files at once:**
```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 921600 \
  --before default_reset --after hard_reset write_flash -z \
  --flash_mode dio --flash_freq 80m --flash_size 16MB \
  0x0 bootloader.bin \
  0x8000 partitions.bin \
  0x10000 firmware.bin \
  0x310000 littlefs.bin
```

**Note:** Replace `/dev/ttyUSB0` with your serial port:
- macOS: `/dev/cu.usbserial-*` or `/dev/tty.usbserial-*`
- Windows: `COM3`, `COM4`, etc.
- Linux: `/dev/ttyUSB0` or `/dev/ttyACM0`

### Option 2: Using BTT Flash Tool

See [FLASH_TOOL_GUIDE.md](../../docs/FLASH_TOOL_GUIDE.md) for graphical flashing instructions.

### Option 3: Firmware-Only Update (Keep WiFi Settings)

If you already have KNOMI configured and only want to update the firmware:

```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 921600 \
  write_flash -z 0x10000 firmware.bin
```

This preserves your WiFi credentials and Moonraker configuration.

## 📍 Memory Map

| Address | Size | Contents |
|---------|------|----------|
| 0x0 | 15KB | Bootloader |
| 0x8000 | 3KB | Partition Table |
| 0x10000 | 2.8MB | Firmware Application |
| 0x310000 | 8.9MB | LittleFS Filesystem (GIFs) |

## 🔍 Verification

After flashing, the display should:

1. Boot and show BTT logo
2. Connect to WiFi (or create AP if not configured)
3. Display hostname via mDNS: `knomi-t0.local` (or t1-t5)
4. Respond to HTTP API: `curl http://knomi-t0.local/api/sleep/status`

## 🆘 Troubleshooting

**Display stuck on boot:**
- Re-flash bootloader and partitions
- Check USB cable quality (use data cable, not charge-only)
- Reduce baud rate to 115200 if flashing fails

**WiFi not connecting:**
- Display will automatically switch to AP mode after 3 failed attempts
- Connect to `KNOMI_AP_XXXXX` WiFi network
- Configure via web interface at `http://192.168.4.1`

**Remote restart not working:**
- Display must be reachable on network
- Use power cycle as fallback: disconnect 5V for 5 seconds

## 🔗 Related Documentation

- [User Guide](../../README.md) - Setup and configuration
- [Developer Documentation](../../docs/dev/README.md) - Technical deep-dive
- [WiFi Troubleshooting](../../docs/dev/WIFI_TROUBLESHOOTING.md) - Connection issues
- [Flash Tool Guide](../../docs/FLASH_TOOL_GUIDE.md) - Detailed flashing instructions

## 📝 Changelog

See [CHANGELOG.md](../../CHANGELOG.md) for complete version history.

---

**Build Information:**
- Platform: ESP32-S3-R8 (16MB Flash, 8MB PSRAM)
- Framework: Arduino-ESP32 2.0.11
- LVGL: 8.3.7
- Build Date: December 3, 2025
- Compiler: GCC 8.4.0

---

**Questions or Issues?** Visit [GitHub Discussions](https://github.com/PrintStructor/knomi-toolchanger/discussions)
