# ESP32 Flash Download Tool - Anleitung für KNOMI V2

Detaillierte Anleitung zum Flashen der KNOMI V2 Firmware mit dem Espressif Flash Download Tool.

---

## 📋 Voraussetzungen

### Software
- **ESP32 Flash Download Tool v3.9.5** (oder neuer)
  - Download: [Espressif Official](https://www.espressif.com/en/support/download/other-tools)
  - Alternativ: [GitHub Releases](https://github.com/espressif/esptool/releases)

### Hardware
- **BTT KNOMI V2 Display** mit ESP32-S3-R8
- **USB-C Kabel** (Daten, nicht nur Laden)
- **Windows PC** (Flash Download Tool läuft nur auf Windows)

---

## 🖼️ Screenshot-Referenz

![ESP32 Flash Tool Settings](https://raw.githubusercontent.com/PrintStructor/knomi-toolchanger/firmware/docs/images/flash_tool_example.png)

> **Hinweis:** Die oben gezeigte Konfiguration ist **funktional**, aber nicht **optimal**. Siehe unten für empfohlene Einstellungen.

### Firmware-Dateien
Die folgenden 4 Dateien benötigst du aus dem [Release v1.0.0](https://github.com/PrintStructor/knomi-toolchanger/releases/tag/v1.0.0):

1. `bootloader.bin` (15 KB)
2. `partitions.bin` (3 KB)
3. `firmware_knomiv2_v1.0.0.bin` (2.93 MB)
4. `littlefs.bin` (9.37 MB)

---

## 🔧 Flash Download Tool Konfiguration

### Schritt 1: Tool starten

1. Entpacke das ESP32 Flash Download Tool
2. Starte `flash_download_tool_3.9.5.exe`
3. Wähle **ChipType: ESP32-S3**
4. Wähle **WorkMode: Develop**

### Schritt 2: Dateien und Offsets konfigurieren

**WICHTIG:** Die Offsets müssen exakt übereinstimmen!

| Datei | Offset (Hex) | Checkbox | Beschreibung |
|-------|-------------|----------|--------------|
| `bootloader.bin` | `0x1000` ⚠️ | ✅ | ESP32-S3 Bootloader |
| `partitions.bin` | `0x8000` | ✅ | Partition Table |
| `firmware_knomiv2_v1.0.0.bin` | `0x10000` | ✅ | Hauptfirmware (10 MB) |
| `littlefs.bin` | `0x710000` | ✅ | Dateisystem (GIFs) |

⚠️ **WICHTIG - Bootloader Offset:**
- **Standard ESP32-S3:** `0x1000` (empfohlen)
- **BTT KNOMI V2:** Kann auch `0x0000` funktionieren (wie im Screenshot)
- **Empfehlung:** Verwende `0x1000` für maximale Kompatibilität

**Screenshot-Referenz:**
```
[✅] bootloader.bin                    @ 0x1000  (oder 0x0000 bei KNOMI V2)
[✅] partitions.bin                    @ 0x8000
[✅] firmware_knomiv2_v1.0.0.bin      @ 0x10000
[✅] littlefs.bin                      @ 0x710000
```

### Schritt 3: Flash-Einstellungen

**Empfohlene Einstellungen (optimal):**

| Einstellung | Wert | Beschreibung |
|-------------|------|--------------|
| **SPI SPEED** | `80MHz` | Maximale Geschwindigkeit für KNOMI V2 |
| **SPI MODE** | `QIO` | Quad I/O (4 Datenleitungen) |
| **FLASH SIZE** | `16MB` | Automatisch erkannt (QUAD: 16MB) |
| **COM Port** | `COM3` (erkannt) | Dein KNOMI-Display |
| **BAUD** | `921600` | Schnellste Übertragung |

**Alternative Einstellungen (funktional, aber langsamer):**

| Einstellung | Wert | Warum funktioniert es? |
|-------------|------|------------------------|
| **SPI SPEED** | `40MHz` | Sicherer, aber 50% langsamer |
| **SPI MODE** | `DIO` | Dual I/O statt Quad I/O |

> **Hinweis:** Dein Screenshot zeigt `40MHz` und `DIO` - das funktioniert, ist aber nicht optimal. Für beste Performance verwende `80MHz` und `QIO`.

### Schritt 4: Flash-Optionen

| Option | Empfohlen | Dein Screenshot | Beschreibung |
|--------|-----------|-----------------|--------------|
| **DoNotChgBin** | ☐ | ✅ | Lässt Binary unverändert (meist nicht nötig) |
| **CombineBin** | ☐ | ☐ | Erstellt kombinierte Binary |
| **LockSettings** | ☐ | ☐ | Sperrt Flash-Einstellungen |

> **Hinweis:** `DoNotChgBin` ist bei dir aktiviert - das ist okay, aber normalerweise nicht erforderlich.

---

## ⚡ Flash-Prozess

### 1. KNOMI Display vorbereiten

1. **Verbinde das KNOMI** per USB-C mit deinem PC
2. **Halte den BOOT-Button** gedrückt (falls vorhanden)
3. **Drücke kurz den RESET-Button** (während BOOT gedrückt ist)
4. **Lasse BOOT los** → ESP32 ist jetzt im Download-Modus

> **Alternativ:** Einige KNOMI-Boards starten automatisch im Download-Modus, wenn USB verbunden wird.

### 2. COM-Port identifizieren

**Windows Geräte-Manager:**
- Öffne `Geräte-Manager` (Win+X → Geräte-Manager)
- Unter "Anschlüsse (COM & LPT)" sollte `USB Serial Port (COMx)` erscheinen
- Notiere die COM-Nummer (z.B. COM7)

**Im Flash Tool:**
- Wähle den erkannten COM-Port aus dem Dropdown

### 3. Flash starten

1. Klicke auf **START**
2. Der Flash-Prozess beginnt:
   ```
   [0%] Connecting...
   [5%] Erasing flash...
   [10%] Writing bootloader.bin @ 0x1000...
   [20%] Writing partitions.bin @ 0x8000...
   [30%] Writing firmware.bin @ 0x10000...
   [90%] Writing littlefs.bin @ 0x710000...
   [100%] Verifying...
   FINISH
   ```

3. **Dauer:** ca. 2-4 Minuten (abhängig von Baudrate und USB-Port)

### 4. Fertigstellung

Wenn "**FINISH**" in grün erscheint:
1. ✅ Flash erfolgreich
2. Trenne USB-Kabel
3. Verbinde erneut → KNOMI startet mit neuer Firmware

---

## 🐛 Problemlösungen

### Problem 1: "Connecting... timeout"

**Ursache:** ESP32 nicht im Download-Modus

**Lösung:**
1. USB-Kabel trennen
2. BOOT-Button gedrückt halten
3. USB-Kabel verbinden (während BOOT gedrückt)
4. Nach 2 Sekunden BOOT loslassen
5. Im Flash Tool erneut auf START klicken

### Problem 2: "Flash size mismatch"

**Ursache:** Falsche Flash-Größe eingestellt

**Lösung:**
- Stelle sicher, dass **FLASH SIZE: 16 MB** ausgewählt ist
- KNOMI V2 hat immer 16 MB Flash (ESP32-S3-R8)

### Problem 3: "Write failed at 0xXXXXX"

**Ursache:** Defektes USB-Kabel oder Port-Probleme

**Lösung:**
1. Verwende ein anderes USB-Kabel (Datenkabel, nicht nur Ladekabel)
2. Wechsle den USB-Port (bevorzugt USB 2.0 statt 3.0)
3. Reduziere Baudrate auf `460800` oder `115200`

### Problem 4: "COM Port not found"

**Ursache:** Treiber fehlen

**Lösung:**
- Installiere **CP210x USB to UART Bridge Driver**
  - Download: [Silicon Labs](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
- Oder **CH340 Driver** (je nach KNOMI-Variante)
  - Download: [WCH Official](http://www.wch.cn/downloads/CH341SER_EXE.html)

### Problem 5: "Invalid partition table"

**Ursache:** Falsche Offset-Adresse für `partitions.bin`

**Lösung:**
- **partitions.bin MUSS bei 0x8000 sein** (nicht 0x8000**0** oder 0x**0**8000)
- Prüfe, dass alle Offsets korrekt sind (siehe Tabelle oben)

---

## 📊 Verifikation nach dem Flash

### 1. Serieller Monitor (optional)

Verbinde mit 115200 Baud und prüfe die Boot-Logs:

```bash
# Mit PlatformIO
pio device monitor -b 115200

# Mit Arduino IDE Serial Monitor
Tools → Serial Monitor → 115200 baud
```

**Erwartete Ausgabe:**
```
[Boot] KNOMI V2 Firmware v1.0.0
[Boot] ESP32-S3-R8 (16MB Flash, 8MB PSRAM)
[WiFi] Booting into AP mode: KNOMI_AP_XXXXX
[Display] GC9A01 initialized
[FS] LittleFS mounted successfully
[FS] Loading GIF for tool X from /gifs/tool_X.gif
```

### 2. WiFi Konfiguration

Nach erfolgreichem Flash:
1. KNOMI startet im **AP-Modus**: `KNOMI_AP_XXXXX`
2. Verbinde dich mit diesem WLAN
3. Navigiere zu `192.168.4.1`
4. Gib deine WiFi-Credentials ein
5. KNOMI startet neu und verbindet sich mit deinem WLAN

### 3. Funktionstest

**Display-Test:**
- ✅ Touchscreen reagiert
- ✅ Standby-GIF wird angezeigt
- ✅ Temperaturanzeige funktioniert

**Netzwerk-Test:**
```bash
# Ping auf Hostname (nach WiFi-Setup)
ping knomi-t0.local

# API-Test
curl http://knomi-t0.local/api/sleep/status
```

---

## 🔄 Alternative: Combined Binary Flash

Wenn du häufiger flashst, kannst du ein **Combined Binary** erstellen:

### Combined Binary erstellen

1. Im Flash Download Tool:
   - Aktiviere **CombineBin** ☑
   - Konfiguriere alle 4 Dateien wie oben
   - Klicke auf **CombineBin** (unten rechts)

2. Es wird eine Datei erstellt:
   - `combined_0x0.bin` (ca. 16 MB)

3. Flashen mit einem einzigen Befehl:
   ```bash
   esptool.py --chip esp32s3 --port COM7 write_flash 0x0 combined_0x0.bin
   ```

**Vorteil:** Schnelleres Flashen, nur eine Datei
**Nachteil:** Größere Datei, immer kompletter Flash nötig

---

## 📁 Verzeichnisstruktur für Flash-Dateien

Empfohlene Organisation:

```
KNOMI_Flash/
├── v1.0.0/
│   ├── bootloader.bin
│   ├── partitions.bin
│   ├── firmware_knomiv2_v1.0.0.bin
│   └── littlefs.bin
├── flash_download_tool_3.9.5.exe
└── README.txt (diese Anleitung)
```

---

## ⚙️ Nur Firmware updaten (ohne littlefs)

Wenn du nur die Firmware aktualisierst und die GIFs behalten willst:

**Flash nur diese Datei:**
- `firmware_knomiv2_v1.0.0.bin` @ `0x10000`

**Deaktiviere die anderen:**
- ☐ bootloader.bin
- ☐ partitions.bin
- ☐ littlefs.bin

**Vorteil:** Schneller Update (~30 Sekunden)
**Nutzen:** WiFi-Einstellungen und GIFs bleiben erhalten

---

## 🚀 Fortgeschrittene Optionen

### DoNotChgBin Erklärung

**Wenn aktiviert (☑):**
- Originale Binary wird verwendet (keine Modifikation)
- Für signierte Firmwares erforderlich

**Für KNOMI V2:** Normalerweise deaktiviert lassen (☐)

### SPI Mode Erklärung

| Modus | Beschreibung | Geschwindigkeit |
|-------|--------------|-----------------|
| QIO | Quad I/O (4 Datenleitungen) | **Schnellste** |
| QOUT | Quad Output | Schnell |
| DIO | Dual I/O | Standard |
| DOUT | Dual Output | Langsam |

**Empfehlung:** QIO verwenden (wie in platformio.ini konfiguriert)

### Baud Rate Wahl

| Baudrate | Dauer | Stabilität |
|----------|-------|------------|
| 921600 | ~2 min | Gut (empfohlen) |
| 460800 | ~3 min | Sehr gut |
| 115200 | ~10 min | Exzellent (bei Problemen) |

---

## 📞 Support

**Bei Problemen:**
1. Prüfe, dass alle Offsets korrekt sind
2. Verwende ein gutes USB-Kabel
3. Reduziere Baudrate auf 115200
4. Prüfe Geräte-Manager für COM-Port

**GitHub Issues:** https://github.com/PrintStructor/knomi-toolchanger/issues

---

## 📄 Referenzen

- [Espressif Flash Download Tool Dokumentation](https://www.espressif.com/sites/default/files/tools/flash_download_tool_v3.9.5_0.pdf)
- [ESP32-S3 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- [KNOMI V2 GitHub Repository](https://github.com/PrintStructor/knomi-toolchanger)

---

**Version:** 1.0.0
**Letztes Update:** 29. November 2024
**Autor:** PrintStructor
