# Pre-compiled Firmware Releases

This folder contains pre-compiled firmware binaries for KNOMI V2 displays, ready to flash without needing to compile from source.

## 📦 Available Releases

| Version | Release Date | Key Features | Download |
|---------|--------------|--------------|----------|
| **v1.1.0** | Dec 3, 2025 | WiFi auto-reconnect, remote restart API | [Download](v1.1.0/) |
| v1.0.0 | Dec 2, 2025 | Initial stable release with 6-tool support | See GitHub releases |

## 🚀 Quick Start

1. **Download** the latest version folder
2. **Flash** using esptool.py or BTT Flash Tool
3. **Configure** WiFi and Moonraker connection
4. **Enjoy** your synchronized 6-display toolchanger system!

## 🔧 Flashing Instructions

### Quick Flash (All Components)

```bash
cd releases/v1.1.0/
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 921600 \
  --before default_reset --after hard_reset write_flash -z \
  --flash_mode qio --flash_freq 80m --flash_size 16MB \
  0x0000 bootloader.bin \
  0x8000 partitions.bin \
  0x10000 firmware.bin \
  0x710000 littlefs.bin
```

### Firmware-Only Update

```bash
cd releases/v1.1.0/
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 921600 \
  write_flash -z 0x10000 firmware.bin
```

**Note:** Firmware-only updates preserve your WiFi credentials and Moonraker settings.

## 📖 Documentation

For detailed instructions, see:
- [Flash Tool Guide](../docs/FLASH_TOOL_GUIDE.md) - Step-by-step flashing with BTT tool
- [User Guide](../README.md) - Setup and configuration
- [Developer Documentation](../docs/dev/README.md) - Technical details

## 🆚 Version Comparison

### v1.1.0 (Latest)
✅ Automatic WiFi reconnection (3 attempts, 5s interval)
✅ Remote restart API (`/api/restart`)
✅ Improved WiFi stability
✅ AP mode fallback after failed reconnects

### v1.0.0
✅ 6-tool multi-display support
✅ Tool-specific GIFs and colors
✅ Print progress overlay with PSRAM optimization
✅ Hybrid display state machine
✅ Display sleep management
⚠️ Manual power cycle required for WiFi recovery

## 💾 File Sizes

Each release contains:
- **bootloader.bin** (~15KB) - ESP32-S3 bootloader
- **partitions.bin** (~3KB) - Partition table
- **firmware.bin** (~2.8MB) - Main application
- **littlefs.bin** (~8.9MB) - Filesystem with GIFs

**Total download:** ~12MB per release

## 🔍 Which Files Do I Need?

**First-time setup or full reflash:**
- Flash all 4 files (bootloader, partitions, firmware, littlefs)

**Updating existing installation:**
- Flash only `firmware.bin` (preserves WiFi settings)

**Changing GIF animations:**
- Flash only `littlefs.bin` (requires custom build)

## 🛠️ Hardware Requirements

- **Display:** BTT KNOMI V2 (ESP32-S3-R8)
- **Flash:** 16MB (minimum)
- **PSRAM:** 8MB (required for GIF playback)
- **Cable:** USB-C data cable (not charge-only)

## ⚠️ Important Notes

1. **Backup your configuration** before flashing if you have custom settings
2. **Use quality USB cables** - poor cables cause flashing failures
3. **Do not disconnect power** during flashing process
4. **Verify flash success** by checking serial output after boot

## 🔗 External Resources

- [ESP32 esptool Documentation](https://docs.espressif.com/projects/esptool/en/latest/)
- [PlatformIO Documentation](https://docs.platformio.org/)
- [LVGL Documentation](https://docs.lvgl.io/8.3/)

## 🐛 Troubleshooting

**Flashing fails with "Timed out waiting for packet header":**
- Check USB cable quality
- Reduce baud rate to 115200
- Try different USB port
- Press and hold BOOT button during connection

**Display shows blank screen after flash:**
- Verify all 4 files were flashed to correct addresses
- Check serial output for error messages
- Reflash bootloader and partitions

**WiFi not connecting after flash:**
- Display will auto-switch to AP mode after 3 failed attempts
- Connect to `KNOMI_AP_XXXXX` network
- Configure via web interface at `http://192.168.4.1`

## 📧 Support

- **Issues:** [GitHub Issues](https://github.com/PrintStructor/knomi-toolchanger/issues)
- **Discussions:** [GitHub Discussions](https://github.com/PrintStructor/knomi-toolchanger/discussions)
- **Discord:** [VORON Discord](https://discord.gg/voron) (#toolchangers channel)

---

**Last Updated:** December 3, 2025
