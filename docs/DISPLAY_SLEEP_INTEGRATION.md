# Display Sleep Mode Integration Guide

## Übersicht

Das Display Sleep System wurde implementiert und muss nun in die bestehenden Dateien integriert werden.

## Änderungen erforderlich in folgenden Dateien:

### 1. `src/ui_overlay/lv_auto_goto_idle.cpp`

**Änderung:** Display Sleep Update hinzufügen

```cpp
// Am Anfang der Datei hinzufügen:
#include "../power_management/display_sleep.h"

// In der Funktion lv_loop_auto_idle():
void lv_loop_auto_idle(wifi_status_t status) {
    // *** NEU: Display Sleep Management ***
    display_sleep_update();  // ← Prüft Sleep-Timer
    
    // Wenn Display schläft, keine weiteren Aktionen
    if (display_is_sleeping()) {
        return;
    }
    
    // ... rest des bestehenden Codes bleibt unverändert ...
}
```

---

### 2. `src/lvgl_hal.cpp`

**Änderung:** Touch-Events wecken Display auf

```cpp
// Am Anfang der Datei hinzufügen:
#include "power_management/display_sleep.h"

// In der Funktion usr_touchpad_read() ändern:
#ifdef CST816S_SUPPORT
void touch_idle_time_clear(void);
void usr_touchpad_read(struct _lv_indev_drv_t * indev_drv, lv_indev_data_t * data) {
    static touch_event_t event;
    if(ts_cst816s.ready()) {
        ts_cst816s.getTouch(&event);
    }
    if(event.finger) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = event.x;
        data->point.y = event.y;
        
        touch_idle_time_clear();
        display_sleep_reset_timer();  // ← NEU: Display aufwecken bei Touch
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}
#endif
```

---

### 3. `src/ui_overlay/lv_moonraker_change_screen.cpp`

**Änderung:** Display aufwecken bei Statusänderungen

```cpp
// Am Anfang der Datei hinzufügen:
#include "../power_management/display_sleep.h"

// In der Funktion lv_loop_moonraker_change_screen():
void lv_loop_moonraker_change_screen(void) {
    // *** NEU: Status-Tracking für Wake-up ***
    static bool last_printing = false;
    static bool last_homing = false;
    static bool last_probing = false;
    static bool last_qgling = false;
    static bool last_heating_nozzle = false;
    static bool last_heating_bed = false;
    
    // Prüfe ob sich relevanter Status geändert hat
    bool status_changed = (
        moonraker.data.printing != last_printing ||
        moonraker.data.homing != last_homing ||
        moonraker.data.probing != last_probing ||
        moonraker.data.qgling != last_qgling ||
        moonraker_nozzle_is_heating() != last_heating_nozzle ||
        moonraker_bed_is_heating() != last_heating_bed
    );
    
    // Status speichern
    last_printing = moonraker.data.printing;
    last_homing = moonraker.data.homing;
    last_probing = moonraker.data.probing;
    last_qgling = moonraker.data.qgling;
    last_heating_nozzle = moonraker_nozzle_is_heating();
    last_heating_bed = moonraker_bed_is_heating();
    
    // Display aufwecken bei Statusänderung
    display_check_wake_condition(status_changed);
    
    // Wenn Display schläft, keine Screen-Updates
    if (display_is_sleeping()) {
        return;
    }
    
    // ... rest des bestehenden Codes bleibt unverändert ...
}
```

---

### 4. `src/lvgl_usr.cpp`

**Änderung:** Display Sleep initialisieren

```cpp
// Am Anfang der Datei hinzufügen:
#include "power_management/display_sleep.h"

// In der Funktion lvgl_ui_task(), nach lvgl_hal_init():
void lvgl_ui_task(void * parameter) {
    lv_btn_init();
    lvgl_hal_init();
    
    // *** NEU: Display Sleep System initialisieren ***
    display_sleep_init();
    Serial.println("[INIT] Display Sleep Management ready!");
    
    ui_init();
    
    // ... rest des Codes ...
}
```

---

### 5. `platformio.ini`

**Änderung:** Neue Source-Dateien zum Build hinzufügen

```ini
[env:knomiv2]
# ... bestehende Konfiguration ...

build_flags = 
    ${env.build_flags}
    -D FW_VERSION=\"v2.0.0\"
    # ... andere flags ...

# WICHTIG: Stelle sicher dass alle .cpp Dateien kompiliert werden
build_src_filter = 
    +<*>
    +<gif/*.c>
    +<power_management/*.cpp>  # ← NEU
```

---

## Test-Prozedur

### 1. Display Sleep Timing testen:

```
1. System starten und warten
2. Nach 60 Sekunden: Tool-GIF wird angezeigt (Idle State)
3. Nach 5 Minuten: Display wird schwarz (Sleep Mode)
4. Touch das Display → Sofortiges Aufwachen
```

### 2. Status-basiertes Wake-up testen:

```
1. Warte bis Display schläft (5 Min)
2. Starte Druck über Klipper → Display wacht automatisch auf
3. Warte bis Display schläft
4. Starte Bed-Heizen → Display wacht auf
```

### 3. Serial Monitor Logs prüfen:

```
[Display Sleep] Initialized
[Display Sleep] Idle timeout: 60s, Sleep timeout: 300s
[Display Sleep] → IDLE state (showing standby GIF)
========================================
[Display Sleep] ENTERING SLEEP MODE
========================================
[Display Sleep] LVGL timers paused
[Display Sleep] Backlight OFF
[Display Sleep] GC9A01 entered sleep mode
[Display Sleep] ✅ Sleep mode active - system idle
```

---

## Konfiguration anpassen

In `display_sleep.h` können die Timeouts angepasst werden:

```cpp
// Zeitkonstanten in Sekunden
#define DISPLAY_IDLE_TIMEOUT_SEC   60    // Nach 60s → Idle-GIF
#define DISPLAY_SLEEP_TIMEOUT_SEC  300   // Nach 5 Min → Display Sleep
```

**Beispiele:**
- Schnellerer Sleep: `#define DISPLAY_SLEEP_TIMEOUT_SEC  120` (2 Minuten)
- Langsamerer Sleep: `#define DISPLAY_SLEEP_TIMEOUT_SEC  600` (10 Minuten)
- Sleep deaktivieren: Kommentiere `display_sleep_update();` Aufrufe aus

---

## Vorteile

✅ **Energiesparen**: Display komplett aus (Backlight + Hardware Sleep)
✅ **Automatisches Aufwachen**: Bei Touch oder Statusänderungen
✅ **CPU-Entlastung**: LVGL Timer pausiert während Sleep
✅ **Zweistufig**: Erst Idle-GIF (60s), dann Sleep (5 Min)
✅ **Konfigurierbar**: Timeouts einfach anpassbar

---

## Bekannte Einschränkungen

- **Erste Wake-up-Verzögerung**: GC9A01 braucht ~120ms zum Aufwachen
- **Moonraker-Polling**: System muss Moonraker-Status regelmäßig abfragen für Wake-up
- **WiFi bleibt aktiv**: Nur Display schläft, ESP32 bleibt wach für Moonraker-Updates
