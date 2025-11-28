# 🔍 KNOMI Visual Debug Guide
**Für Hardware-Tests nach Code-Fixes**

## 🎯 Was solltest du sehen?

---

## View 1: Progress View (10 Sekunden)

```
┌─────────────────────────────────────┐
│                                     │
│              260°C                  │  ← Temperatur (weiß, oben)
│                                     │
│                                     │
│            ░░░░░░░                  │  ← Bunter Ring (teilweise sichtbar)
│          ░░░░░░░░░░░                │
│         ░░█████████░░                │  ← Schwarzer Arc verdeckt Rest
│        ░░███  50%  ███░              │  ← Progress % (groß, Mitte)
│        ░░███       ███░              │
│         ░░█████████░░                │
│          ░░░░░░░░░░░                 │
│            ░░░░░░░                   │
│                                     │
│          Layer 45/120               │  ← Layer Info (unten)
│                                     │
└─────────────────────────────────────┘
     🌈 Glow um Mitte (Temperatur-Farbe)
```

### ✅ RICHTIG:
- Bunter Ring ist **teilweise** sichtbar (nur der bereits erledigte Teil)
- Schwarzer Arc verdeckt den **noch nicht erledigten** Teil
- Arc bewegt sich **smooth** im Uhrzeigersinn vorwärts
- Glow Circle ist sichtbar (Farbe abhängig von Temperatur)
- Progress % **wechselt** alle 3 Sekunden zu Restzeit (z.B. "2h 15m")
- **KEIN Flickering** beim Arc-Update

### ❌ FALSCH:
- Kompletter bunter Ring sichtbar (Arc zu klein oder transparent)
- Arc bewegt sich **rückwärts** oder springt
- Progress % und Restzeit **überlagern sich** (beide gleichzeitig sichtbar)
- Arc **flackert** bei jedem Update
- Ring **scheint durch** den schwarzen Arc

---

## View 2: Temp Graph View (8 Sekunden)

```
┌─────────────────────────────────────┐
│                                     │
│       Nozzle Temperature            │  ← Titel (klein, weiß)
│                                     │
│     ╔═════════════════════╗         │
│  3  ║     ╱╲  ╱╲         ║         │  ← Temperatur-Graph
│  0  ║    ╱  ╲╱  ╲        ║         │     (Cyan-Linie)
│  0  ║   ╱       ╲ ╱╲     ║         │
│     ║  ╱         ╲╱  ╲   ║         │
│  0  ║ ╱              ╲  ║         │
│     ╚═════════════════════╝         │
│                                     │
│       Current: 260°C                │  ← Aktueller Wert
│                                     │
└─────────────────────────────────────┘
     🖤 KOMPLETT schwarzer Hintergrund!
```

### ✅ RICHTIG:
- Background ist **KOMPLETT SCHWARZ** (keine bunten Ringe/GIFs sichtbar!)
- Nur Graph + 2 Labels sichtbar (Titel + Current)
- Graph zeigt **letzte 60 Datenpunkte** (1 Minute Historie)
- Graph-Linie ist **smooth** (keine Sprünge)
- **KEINE** Progress-Elemente sichtbar (Arc, Ring, GIF, %)

### ❌ FALSCH:
- Bunter Ring **scheint durch** schwarzen Hintergrund
- Progress GIF ist im Hintergrund sichtbar
- Arc oder Progress % sind sichtbar
- Background ist **grau** statt schwarz
- Graph hat **weißen Hintergrund** (sollte schwarz sein)

---

## View 3: Main GIF View (8 Sekunden)

```
┌─────────────────────────────────────┐
│                                     │
│                                     │
│         ╔═══════════╗               │
│         ║           ║               │
│         ║  T0 GIF   ║               │  ← Tool-Nummer GIF
│         ║ (animated)║               │     (z.B. Tool 0 = T0)
│         ║           ║               │
│         ╚═══════════╝               │
│                                     │
│                                     │
│                                     │
│                                     │
└─────────────────────────────────────┘
     📹 Nur das GIF, keine anderen Elemente!
```

### ✅ RICHTIG:
- **NUR** das Tool-GIF ist sichtbar (Standard-Ansicht)
- GIF läuft **smooth** ohne Stottern
- **KEINE** Progress-Elemente (Arc, Ring, %, Layer)
- **KEIN** Glow Circle (Standard-Ansicht ohne Extras)
- **KEINE** Temperatur-Anzeige

### ❌ FALSCH:
- **Zwei GIFs** überlagern sich (Doppeltes GIF-System!)
- Bunter Ring ist sichtbar
- Progress % oder Layer-Info sind sichtbar
- GIF **stottert** oder lädt nicht
- Glow Circle ist sichtbar (sollte versteckt sein)

---

## 🔄 View-Wechsel Checks

### Progress → Temp:
```
[t=0s]   Progress View ✓
[t=10s]  🔄 Wechsel...
         [Serial] "[View Clear] ✅ All elements hidden + background reset"
         [Serial] "[View] Activating TEMP GRAPH mode"
[t=10.5s] Temp View ✓
```

✅ **RICHTIG:**
- Smooth Übergang (< 500ms)
- Keine Überlagerung während des Wechsels
- Schwarzer Background erscheint SOFORT (keine bunte Ringe sichtbar)

❌ **FALSCH:**
- Bunter Ring ist **kurz** im Temp View sichtbar
- Arc **bleibt sichtbar** während Wechsel
- **Flickering** oder weißer Blitz beim Wechsel
- View wechselt nicht automatisch

### Temp → Main GIF:
```
[t=18s]  Temp View ✓
[t=18s]  🔄 Wechsel...
         [Serial] "[View Clear] ✅ All elements hidden + background reset"
         [Serial] "[View] Activating MAIN GIF mode"
[t=18.5s] Main GIF View ✓
```

✅ **RICHTIG:**
- GIF startet smooth
- Kein schwarzer Hintergrund mehr (transparent)
- Temp-Graph verschwindet komplett

❌ **FALSCH:**
- Temp-Graph **bleibt sichtbar** über dem GIF
- GIF lädt **nicht** oder startet nicht
- Schwarzer Background bleibt (GIF nicht sichtbar)

### Main GIF → Progress:
```
[t=26s]  Main GIF View ✓
[t=26s]  🔄 Wechsel...
         [Serial] "[View Clear] ✅ All elements hidden + background reset"
         [Serial] "[View] Activating PROGRESS mode"
[t=26.5s] Progress View ✓
```

✅ **RICHTIG:**
- Arc + Ring erscheinen smooth
- Progress % zeigt korrekten Wert
- Glow Circle ist wieder sichtbar

❌ **FALSCH:**
- Main GIF **bleibt sichtbar** hinter Progress View
- Arc startet bei **0%** statt bei aktuellem Progress
- Progress-Elemente **fehlen** (nur GIF sichtbar)

---

## 📊 Serial Monitor Messages

### ✅ RICHTIGE Output-Sequenz beim Start:

```
[INIT] LittleFS ready!
[Progress] Loaded background GIF: 123456 bytes
[Progress] Background GIF animation started!
[Progress Layers] ✅ Layers ordered ONCE - GIF:0xXXXX Ring:0xYYYY Arc:0xZZZZ Glow:0xAAAA
[Progress] Arc: 0% -> angles 450° to 810°
[View] Activating PROGRESS mode
[Progress] Layer: 1/120
```

### ✅ RICHTIGE Output beim View-Wechsel:

```
[View Clear] Starting AGGRESSIVE full clear...
[View Clear] Background GIF paused+hidden
[View Clear] Background Ring hidden
[View Clear] Progress Arc hidden
[View Clear] Glow Circle hidden
[View Clear] Main GIF (s_main_gif_on_printing) paused+hidden
[View Clear] ✅ All elements hidden + background reset
[View] Activating TEMP GRAPH mode (minimal)
```

### ❌ FALSCHE Output (Probleme):

```
[Progress Layers] ✅ Layers ordered ONCE    ← sollte nur EINMAL erscheinen!
[Progress Layers] ✅ Layers ordered ONCE    ← FEHLER: erscheint mehrfach!
[Progress Layers] ✅ Layers ordered ONCE
[Progress Layers] ✅ Layers ordered ONCE
```
→ **Problem:** Z-Order wird bei jedem Update neu gesetzt (Flickering!)

```
[Progress] Arc: 50% -> angles 630° to 810°
[Progress] Arc: 50% -> angles 630° to 810°    ← Doppelt!
[Progress] Arc: 50% -> angles 630° to 810°
```
→ **Problem:** Arc wird zu oft geupdated

```
[View] Activating TEMP GRAPH mode
[View Clear] ✅ All elements hidden    ← FEHLER: Reihenfolge falsch!
```
→ **Problem:** Clear kommt NACH Mode-Aktivierung (sollte VOR sein)

---

## 🎮 Performance-Checks

### FPS Monitor (wenn verfügbar):
```
✅ RICHTIG:  FPS: 22-30 (stabil)
⚠️ WARNING: FPS: 15-20 (grenzwertig)
❌ FALSCH:  FPS: <15 (zu niedrig - Performance-Problem!)
```

### Heap Memory:
```
✅ RICHTIG:  Free Heap: >100kB
⚠️ WARNING: Free Heap: 50-100kB
❌ FALSCH:  Free Heap: <50kB (Memory Leak?)
```

### PSRAM Usage:
```
✅ RICHTIG:  PSRAM: ~500kB verwendet (GIF-Daten)
⚠️ WARNING: PSRAM: >1MB verwendet
❌ FALSCH:  PSRAM: >2MB verwendet (Memory Leak!)
```

---

## 🐛 Troubleshooting

### Problem: Arc ist nicht sichtbar
**Mögliche Ursachen:**
1. Arc ist transparent (`LV_OPA_TRANSP` statt `LV_OPA_COVER`)
2. Arc ist hinter dem Ring (Z-Order falsch)
3. Arc ist zu klein (<240×240 Pixel)

**Fix prüfen:**
```cpp
// ui_ScreenPrinting.c
lv_obj_set_size(ui_arc_progress_cover, 240, 240);  // Muss 240×240 sein!
lv_obj_set_style_arc_opa(ui_arc_progress_cover, 255, LV_PART_INDICATOR);
lv_obj_move_to_index(ui_arc_progress_cover, 2);    // Über dem Ring!
```

---

### Problem: Bunter Ring im Temp View sichtbar
**Mögliche Ursachen:**
1. Background ist transparent (`LV_OPA_TRANSP`)
2. Ring wird nicht versteckt beim View-Wechsel
3. Ring hat falsche Z-Order (über dem Background)

**Fix prüfen:**
```cpp
// lvgl_usr.cpp - Mode 1 (TEMP)
lv_obj_set_style_bg_opa(ui_ScreenPrinting, LV_OPA_COVER, 0);  // Muss COVER sein!
if (bg_ring) lv_obj_add_flag(bg_ring, LV_OBJ_FLAG_HIDDEN);   // Muss hidden sein!
```

---

### Problem: View wechselt nicht automatisch
**Mögliche Ursachen:**
1. Auto-Cycle Timer nicht gestartet
2. Cycle-Timer ist pausiert
3. `s_cycle_running` ist false

**Fix prüfen:**
```cpp
// lvgl_usr.cpp
ui_auto_cycle_start();  // Muss beim Start aufgerufen werden!
```

---

### Problem: Zwei GIFs überlagern sich
**Mögliche Ursachen:**
1. Beide GIF-Systeme aktiv (`s_main_gif_on_printing` UND `ui_main_screen_gif`)
2. Eines der GIFs wird nicht versteckt

**Fix prüfen:**
```cpp
// lv_print_progress_update.cpp - Zeile ~368
// DEACTIVATED: Main Screen GIF is now managed by lvgl_usr.cpp!
// ensure_main_screen_gif_created(tool_number);  // MUSS auskommentiert sein!
```

---

## ✅ Final Checklist

Nach allen Tests solltest du sehen:

- [ ] Progress View: Arc sichtbar, maskiert Ring korrekt
- [ ] Temp View: KOMPLETT schwarzer Hintergrund
- [ ] Main GIF: Nur GIF, keine anderen Elemente
- [ ] View-Wechsel smooth ohne Flickering
- [ ] FPS stabil >20
- [ ] Serial: "Layers ordered ONCE" nur EINMAL
- [ ] Kein Memory Leak (Heap stabil)
- [ ] Progress % wechselt zu Restzeit alle 3s

Wenn ALLE Punkte ✅ sind → **System funktioniert perfekt!** 🎉

---

**Erstellt von Claude - 18. Oktober 2025**
