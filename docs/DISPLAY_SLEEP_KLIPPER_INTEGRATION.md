# 🌙 Display Sleep mit Klipper & LED Synchronisation

## 🎯 Neue Features

Das Display Sleep System wurde erweitert mit:

### ✅ Kein Sleep während Druck
- Display schläft **NIEMALS** während:
  - Druck läuft
  - Homing/Probing/QGL aktiv
  - Bed oder Nozzle heizen

### ✅ Drei Sleep-Modi

**1. MANUAL Mode** (klassisch)
- Feste Timeouts: 60s Idle → 5 Min Sleep
- Unabhängig von Klipper

**2. KLIPPER_SYNC Mode** (empfohlen) ⭐
- Display folgt Klipper `idle_timeout` Status
- Wenn Klipper in "Idle" geht → Display schläft 10s später
- Perfekt synchronisiert mit deinem Drucker

**3. LED_SYNC Mode** (für LED-Nutzer)
- Display folgt LED-Status
- LEDs aus → Display schläft
- LEDs an → Display wacht auf

---

## 🔧 Setup: Klipper Synchronisation

### Schritt 1: Klipper Macro erstellen

Erstelle eine neue Datei `knomi_sleep.cfg` oder füge zu deiner `printer.cfg` hinzu:

```ini
# ========================================================================
# KNOMI Display Sleep - Klipper Integration
# ========================================================================

[gcode_macro _KNOMI_UPDATE_IDLE_STATE]
description: Update KNOMI Displays mit Klipper idle_timeout Status
gcode:
    {% set idle_state = printer.idle_timeout.state %}
    
    # Sende Status an alle KNOMI Displays
    # Format: "state=Ready|Printing|Idle"
    SET_GCODE_VARIABLE MACRO=_KNOMI_IDLE_STATE VARIABLE=state VALUE='"{idle_state}"'
    
    # Debug output
    RESPOND MSG="KNOMI: Klipper idle_timeout state = {idle_state}"

[gcode_macro _KNOMI_IDLE_STATE]
variable_state: "Ready"
gcode:
    # Dummy macro für State Storage

# ========================================================================
# Auto-Update bei idle_timeout Änderungen
# ========================================================================

[delayed_gcode _KNOMI_IDLE_MONITOR]
initial_duration: 5.0
gcode:
    _KNOMI_UPDATE_IDLE_STATE
    UPDATE_DELAYED_GCODE ID=_KNOMI_IDLE_MONITOR DURATION=2.0
```

### Schritt 2: Moonraker Integration

In `moonraker.cpp` müssen wir den `idle_timeout.state` abfragen.

**Füge zu `moonraker.cpp` hinzu:**

```cpp
// In get_printer_info() oder neue Funktion get_idle_timeout():

void MOONRAKER::get_idle_timeout(void) {
    String path = "/printer/objects/query?idle_timeout";
    String response = send_request("GET", path);
    
    if (response.length() > 0) {
        // Parse JSON response
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, response);
        
        // Extrahiere idle_timeout.state
        const char* state = doc["result"]["status"]["idle_timeout"]["state"];
        
        if (state) {
            // Update Display Sleep System
            extern void display_update_klipper_idle_state(const char* state);
            display_update_klipper_idle_state(state);
        }
    }
}

// In http_get_loop() hinzufügen:
void MOONRAKER::http_get_loop(void) {
    // ... bestehender Code ...
    get_idle_timeout();  // ← NEU
}
```

---

## 🔧 Setup: LED Synchronisation

### Option A: LED-Status über Klipper Macro

```ini
[gcode_macro _KNOMI_LED_STATUS]
description: Sende LED Status an KNOMI
variable_leds_active: True
gcode:
    {% set leds = params.ACTIVE|default(1)|int %}
    SET_GCODE_VARIABLE MACRO=_KNOMI_LED_STATUS VARIABLE=leds_active VALUE={leds}
    
    # Wenn deine LED-Effekte sich ausschalten:
    {% if leds == 0 %}
        # Alle KNOMI Displays sollen schlafen
        RESPOND MSG="KNOMI: LEDs OFF - Displays entering sleep"
    {% else %}
        RESPOND MSG="KNOMI: LEDs ON - Displays active"
    {% endif %}

# In deinem LED-Off Macro aufrufen:
[gcode_macro LEDS_OFF]
gcode:
    # Deine LED-Befehle...
    SET_LED LED=my_leds RED=0 GREEN=0 BLUE=0
    _KNOMI_LED_STATUS ACTIVE=0  # ← KNOMI informieren
```

### Option B: LED-Status direkt aus led_effect

Wenn du `[led_effect]` nutzt, kannst du den Status abfragen:

```cpp
// In moonraker.cpp:

void MOONRAKER::get_led_status(void) {
    String path = "/printer/objects/query?led_effect";
    String response = send_request("GET", path);
    
    // Parse und prüfe ob LEDs aktiv sind
    // ... dann:
    extern void display_update_led_status(bool active);
    display_update_led_status(leds_active);
}
```

---

## 🎛️ Konfiguration in Code

In `lvgl_usr.cpp` beim Init:

### Option 1: Klipper Sync (empfohlen)

```cpp
void lvgl_ui_task(void * parameter) {
    lv_btn_init();
    lvgl_hal_init();
    
    // KLIPPER_SYNC Mode - Display folgt Klipper idle_timeout
    display_sleep_init(SLEEP_MODE_KLIPPER_SYNC);
    Serial.println("[INIT] Display Sleep: KLIPPER_SYNC Mode");
    
    ui_init();
    // ...
}
```

### Option 2: LED Sync

```cpp
// LED_SYNC Mode - Display folgt LED-Status
display_sleep_init(SLEEP_MODE_LED_SYNC);
Serial.println("[INIT] Display Sleep: LED_SYNC Mode");
```

### Option 3: Manual (klassisch)

```cpp
// MANUAL Mode - Feste Timeouts
display_sleep_init(SLEEP_MODE_MANUAL);
Serial.println("[INIT] Display Sleep: MANUAL Mode");
```

---

## 🔄 Runtime Mode-Switching

Du kannst den Modus auch zur Laufzeit wechseln:

```cpp
// Über Moonraker Command oder Button:
display_sleep_set_mode(SLEEP_MODE_KLIPPER_SYNC);
```

**Beispiel: Web-Interface Button**

In `webserver.cpp` kannst du Endpoints hinzufügen:

```cpp
server.on("/sleep/mode", HTTP_GET, [](AsyncWebServerRequest *request){
    if(request->hasParam("mode")) {
        String mode = request->getParam("mode")->value();
        
        if (mode == "manual") {
            display_sleep_set_mode(SLEEP_MODE_MANUAL);
        } else if (mode == "klipper") {
            display_sleep_set_mode(SLEEP_MODE_KLIPPER_SYNC);
        } else if (mode == "led") {
            display_sleep_set_mode(SLEEP_MODE_LED_SYNC);
        }
        
        request->send(200, "text/plain", "Mode changed");
    }
});
```

---

## ⚙️ Timeouts anpassen

In `display_sleep.h`:

```cpp
// Für KLIPPER_SYNC: Delay nach Klipper IDLE
#define DISPLAY_SLEEP_DELAY_AFTER_KLIPPER_IDLE_SEC  10  // 10 Sekunden

// Für MANUAL Mode:
#define DISPLAY_IDLE_TIMEOUT_SEC   60    // Idle nach 60s
#define DISPLAY_SLEEP_TIMEOUT_SEC  300   // Sleep nach 5 Min
```

---

## 🧪 Testing

### Test 1: Klipper Sync Mode

```
1. Setze Mode auf KLIPPER_SYNC
2. Warte bis Klipper in IDLE geht (normalerweise nach 600s = 10 Min)
3. Display sollte 10s später schlafen
4. Bewege eine Achse → Display wacht auf
```

**Klipper IDLE-Timeout prüfen:**
```ini
[idle_timeout]
timeout: 600  # 10 Minuten (standard)
```

### Test 2: LED Sync Mode

```
1. Setze Mode auf LED_SYNC
2. Schalte deine LEDs aus
3. Display sollte sofort schlafen
4. Schalte LEDs an → Display wacht auf
```

### Test 3: Während Druck

```
1. Starte einen Druck
2. Display sollte NIEMALS schlafen (egal welcher Mode)
3. Druckende → Display kann schlafen
```

---

## 📊 Serial Monitor Output

### Klipper Sync Mode:
```
[Display Sleep] Initialized
[Display Sleep] Mode: KLIPPER_SYNC
[Display Sleep] Klipper sync delay: 10s after Klipper IDLE
[Klipper Idle] State change: READY → IDLE
[Klipper Idle] IDLE detected → Display will sleep in 10s
[Display Sleep] ENTERING SLEEP MODE
[Display Sleep] Reason: Klipper IDLE
```

### LED Sync Mode:
```
[Display Sleep] Mode: LED_SYNC
[LED Sync] LEDs OFF
[LED Sync] LEDs OFF → Entering Sleep
[Display Sleep] ENTERING SLEEP MODE
[Display Sleep] Reason: LEDs OFF
```

### Print Protection:
```
[Display Sleep] ⚠️ Sleep BLOCKED - Printer is active!
```

---

## 🎯 Empfohlene Konfiguration

Für optimale Erfahrung:

```cpp
// In lvgl_usr.cpp:
display_sleep_init(SLEEP_MODE_KLIPPER_SYNC);

// In display_sleep.h:
#define DISPLAY_SLEEP_DELAY_AFTER_KLIPPER_IDLE_SEC  10
```

Dann in Klipper:
```ini
[idle_timeout]
timeout: 300  # 5 Minuten
```

**Ergebnis:**
- Nach 5 Min ohne Aktivität → Klipper geht in IDLE → Motoren aus
- 10 Sekunden später → Display geht in Sleep
- Bei Bewegung/Druck → Alles wacht automatisch auf

---

## 🔍 Troubleshooting

### Problem: Display schläft nicht synchron mit Klipper

**Lösung:**
- Prüfe ob `get_idle_timeout()` in Moonraker implementiert ist
- Serial Monitor: Schaust du `[Klipper Idle]` Messages?
- Teste manuell: `display_update_klipper_idle_state_enum(KLIPPER_STATE_IDLE);`

### Problem: LED Sync funktioniert nicht

**Lösung:**
- Stelle sicher dass `display_update_led_status()` aufgerufen wird
- Prüfe Serial Monitor für `[LED Sync]` Messages
- Teste manuell: `display_update_led_status(false);`

### Problem: Display schläft während Druck

**Lösung:**
- Das sollte NICHT passieren! Bug im Code wenn das auftritt
- Prüfe `is_printer_active()` Funktion
- Serial Monitor sollte zeigen: `[Display Sleep] ⚠️ Sleep BLOCKED`

---

## ✅ Integration Checkliste

- [ ] Klipper Macro `_KNOMI_UPDATE_IDLE_STATE` hinzugefügt
- [ ] Moonraker `get_idle_timeout()` implementiert (optional)
- [ ] LED-Status Integration (falls LED_SYNC gewünscht)
- [ ] Sleep Mode in `lvgl_usr.cpp` konfiguriert
- [ ] Timeouts nach Bedarf angepasst
- [ ] Getestet: Kein Sleep während Druck
- [ ] Getestet: Sleep bei Idle/LED-Off
- [ ] Getestet: Auto Wake-up funktioniert

---

**🎉 Mit diesen Anpassungen hast du ein perfekt integriertes Display-Sleep-System!**
