# Geänderte Dateien - Übersicht

## 📝 Modifizierte Dateien (3 Stück)

### 1. `src/ui/screens/ui_ScreenPrinting.c`
**Änderungen:**
- ✅ Arc-Größe von 230x230 auf **240x240** erhöht (Zeile 88)

**Grund:** Arc muss gleich groß sein wie der PNG-Ring, sonst scheint Ring durch!

---

### 2. `src/ui_overlay/lv_print_progress_update.cpp`
**Änderungen:**
- ✅ `ensure_bg_layers_created()` - Z-Order nur noch **einmal** setzen (Zeile 121-167)
  - Neue static Variable `layers_ordered` statt `layers_created`
  - Entfernt: `lv_obj_move_foreground()` / `lv_obj_move_background()` im Loop
  - Nur noch `lv_obj_move_to_index()` beim ersten Durchlauf
  
- ✅ Arc-Update optimiert (Zeile 411-421)
  - **Entfernt:** `lv_obj_move_foreground()` bei jedem Update
  - **Entfernt:** `lv_obj_move_background()` für Ring bei jedem Update
  - **Behalten:** Nur Winkel setzen + HIDDEN-Flag clearen
  
- ✅ Chart-Update optimiert (Zeile 488-503)
  - Style nur **einmal** setzen (neue static Variable `chart_styled`)
  - Chart nur updaten wenn **wirklich sichtbar**

**Grund:** Performance! Ständige Z-Order-Updates verursachten Repaint-Orgie

---

### 3. `src/lvgl_usr.cpp`
**Änderungen:**
- ✅ Temp-Graph Mode (Zeile 265-285)
  - **NEU:** Explizites Verstecken von Background-GIF, Ring, Arc, Glow
  - **NEU:** Screen-Background auf **schwarz + deckend** setzen
  - Entfernt: `s_temp_bg_rect` (nicht mehr nötig)
  
- ✅ Progress Mode (Zeile 327-335)
  - **NEU:** Screen-Background auf **transparent** zurücksetzen
  - Entfernt: Überflüssige `move_foreground()` / `move_background()` Calls

**Grund:** Echter schwarzer Hintergrund für Temp-View + keine Z-Order-Updates

---

## 📋 Unveränderte Dateien

Diese Dateien wurden **NICHT** geändert:
- ❌ `src/gif/gif_print_progress.c` - **NICHT IM PROJEKT**
- ✅ `src/ui_overlay/lv_moonraker_change_screen.cpp` - Keine Änderung nötig
- ✅ `src/ui_overlay/lv_overlay.h` - Keine Änderung nötig
- ✅ `src/fs_gif_loader.cpp` - Funktioniert bereits korrekt
- ✅ `lv_conf.h` - Keine LVGL-Settings geändert
- ✅ Alle anderen Dateien

---

## 🔍 Zeilen-Übersicht (für Review)

| Datei | Zeilen | Änderung |
|-------|--------|----------|
| `ui_ScreenPrinting.c` | 88 | Arc-Größe: 230 → 240 |
| `lv_print_progress_update.cpp` | 121-167 | Z-Order nur einmal |
| `lv_print_progress_update.cpp` | 411-421 | Arc-Update ohne Z-Order |
| `lv_print_progress_update.cpp` | 488-503 | Chart-Style nur einmal |
| `lvgl_usr.cpp` | 265-285 | Temp-Mode: Schwarz + Hide All |
| `lvgl_usr.cpp` | 327-335 | Progress-Mode: Reset Background |

---

## 🧪 Test-Checklist

Nach dem Flashen testen:

```
[ ] Arc ist sichtbar und 240x240 groß
[ ] Bunter Ring wird komplett maskiert
[ ] Temp-Graph hat schwarzen Hintergrund
[ ] Kein Ruckeln beim Druck-Start
[ ] View-Wechsel sind flüssig
[ ] Logs zeigen "Layers ordered ONCE"
```

---

## 📦 Nächste Schritte

1. **Kompilieren** (PlatformIO)
2. **Flashen** auf ESP32
3. **Power-Cycle** (Strom trennen, neu starten)
4. **Testen** (siehe Checklist)
5. **Logs prüfen** (Serial Monitor 115200 baud)

Bei Erfolg:
- ✅ Commit mit Message: "fix: Arc visibility + performance improvements"
- ✅ Backup erstellen

Bei Problemen:
- 🔴 Rollback zu vorheriger Version
- 📧 Logs hochladen für Debugging
