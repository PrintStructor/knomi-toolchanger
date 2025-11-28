# 🔧 KNOMI Fix Status Report
**Erstellt:** 18. Oktober 2025  
**Fortsetzung von:** Chat "Debugging ChatGPT configuration"

## 📊 Status-Übersicht

| Fix | Status | Priorität | Details |
|-----|--------|-----------|---------|
| Arc nur Winkel ändern | ✅ OK | HOCH | Korrekt - nur `lv_arc_set_angles()` |
| Z-Order nur einmal | ✅ OK | HOCH | `static bool layers_ordered` vorhanden |
| Temp View Background | ⚠️ ISSUE | KRITISCH | `LV_OPA_TRANSP` → `LV_OPA_COVER` |
| Doppeltes GIF-System | ⚠️ ISSUE | MITTEL | 2 konkurrierende Main-GIFs |
| View Clear Funktion | ⚠️ IMPROVE | MITTEL | Verbesserungen möglich |
| Arc Size | ✅ OK | HOCH | 240×240 korrekt |

---

## 🔴 KRITISCHE ISSUES

### **Issue #1: Temp View Background nicht deckend** 
**Datei:** `src/lvgl_usr.cpp` Zeile ~175  
**Problem:** Schwarzer Background-Rect hat `LV_OPA_TRANSP` → bunte Ringe scheinen durch

**Aktueller Code:**
```cpp
static void ensure_temp_bg_rect(void) {
    if (s_temp_bg_rect || !ui_ScreenPrinting) return;
    s_temp_bg_rect = lv_obj_create(ui_ScreenPrinting);
    lv_obj_remove_style_all(s_temp_bg_rect);
    lv_obj_set_size(s_temp_bg_rect, LV_PCT(100), LV_PCT(100));
    lv_obj_center(s_temp_bg_rect);
    lv_obj_set_style_bg_color(s_temp_bg_rect, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_temp_bg_rect, LV_OPA_COVER, 0);  // ✅ RICHTIG
    lv_obj_add_flag(s_temp_bg_rect, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_background(s_temp_bg_rect);
}
```

**ABER:** In `ui_set_cycle_mode()` Zeile ~320 wird Screen Background verwendet:
```cpp
// MODE 1: TEMP GRAPH
lv_obj_set_style_bg_color(ui_ScreenPrinting, lv_color_black(), 0);
lv_obj_set_style_bg_opa(ui_ScreenPrinting, LV_OPA_COVER, 0);  // ✅ COVER ist richtig!
```

**Status:** ✅ **BEREITS KORREKT!** Background ist `LV_OPA_COVER`

**Aber:** Alternative Lösung mit `s_temp_bg_rect` wird nicht genutzt!

---

### **Issue #2: Doppeltes Main-GIF-System**
**Dateien:** 
- `src/lvgl_usr.cpp` → `s_main_gif_on_printing` 
- `src/ui_overlay/lv_print_progress_update.cpp` → `ui_main_screen_gif`

**Problem:** Zwei konkurrierende GIF-Objekte für den gleichen Zweck!

**Lösung:** Nur EINES verwenden, das andere deaktivieren

**Empfehlung:** 
- ✅ NUTZE: `s_main_gif_on_printing` in `lvgl_usr.cpp` (aktiv gesteuert)
- ❌ DEAKTIVIERE: `ui_main_screen_gif` in `lv_print_progress_update.cpp` (auskommentiert)

**Status:** ⚠️ **CODE IN lv_print_progress_update.cpp BEREITS DEAKTIVIERT:**
```cpp
// DEACTIVATED: Main Screen GIF is now managed by lvgl_usr.cpp!
// ensure_main_screen_gif_created(tool_number);
```

✅ **FIX BEREITS VORHANDEN!**

---

### **Issue #3: View-Wechsel Reihenfolge**
**Datei:** `src/lvgl_usr.cpp` Funktion `ui_set_cycle_mode()`

**Problem:** Screen Background MUSS VOR dem Verstecken der Elemente gesetzt werden

**Aktueller Code (MODE 1 - TEMP):**
```cpp
if (mode == 1) {
    Serial.println("[View] Activating TEMP GRAPH mode (minimal)");
    
    // CRITICAL: Hide all Progress elements FIRST
    if (bg_gif)             lv_obj_add_flag(bg_gif, LV_OBJ_FLAG_HIDDEN);
    if (bg_ring)            lv_obj_add_flag(bg_ring, LV_OBJ_FLAG_HIDDEN);
    if (ui_arc_progress_cover) lv_obj_add_flag(ui_arc_progress_cover, LV_OBJ_FLAG_HIDDEN);
    if (ui_tool_bg_circle)  lv_obj_add_flag(ui_tool_bg_circle, LV_OBJ_FLAG_HIDDEN);

    // THEN set black background
    lv_obj_set_style_bg_color(ui_ScreenPrinting, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(ui_ScreenPrinting, LV_OPA_COVER, 0);
    
    // Enable temp view elements
    if (ui_temp_chart)       lv_obj_clear_flag(ui_temp_chart, LV_OBJ_FLAG_HIDDEN);
    if (ui_label_temp_title) lv_obj_clear_flag(ui_label_temp_title, LV_OBJ_FLAG_HIDDEN);
    if (ui_label_temp_current) lv_obj_clear_flag(ui_label_temp_current, LV_OPT_FLAG_HIDDEN);
```

**Problem:** Reihenfolge ist eigentlich OK! Background wird NACH dem Verstecken gesetzt.

**Aber:** In `clear_all_view_elements()` fehlt das Setzen des Backgrounds!

**Verbesserung:**
```cpp
static void clear_all_view_elements(void)
{
    // ... [alle Hide-Befehle] ...
    
    // WICHTIG: Background auf TRANSPARENT zurücksetzen für Progress View!
    lv_obj_set_style_bg_opa(ui_ScreenPrinting, LV_OPA_TRANSP, 0);
}
```

---

## ✅ BEREITS KORREKT IMPLEMENTIERT

### **Fix #1: Arc Update nur Winkel** ✅
**Datei:** `src/ui_overlay/lv_print_progress_update.cpp` Zeile ~280

```cpp
// CRITICAL: ONLY set angles - NO styling, NO z-order changes!
lv_arc_set_angles(ui_arc_progress_cover, start, end);
```

✅ **Perfekt!** Keine Styling-Änderungen im Update-Loop

---

### **Fix #2: Z-Order nur einmal** ✅
**Datei:** `src/ui_overlay/lv_print_progress_update.cpp` Zeile ~150

```cpp
static void ensure_bg_layers_created()
{
    static bool layers_ordered = false;
    
    ensure_bg_gif_created();
    ensure_bg_ring_created();
    
    // CRITICAL: Only order layers ONCE to avoid constant repaints!
    if (!layers_ordered) {
        // ... [Z-Order setup] ...
        layers_ordered = true;
        Serial.printf("[Progress Layers] ✅ Layers ordered ONCE\n");
    }
}
```

✅ **Perfekt!** Z-Order wird nur einmal gesetzt

---

### **Fix #3: Arc Size** ✅
**Datei:** `src/ui/screens/ui_ScreenPrinting.c` Zeile ~80

```cpp
ui_arc_progress_cover = lv_arc_create(ui_ScreenPrinting);
lv_obj_set_size(ui_arc_progress_cover, 240, 240);  // CRITICAL: Must be 240x240!
```

✅ **Perfekt!** Arc ist 240×240 Pixel

---

## 🔧 EMPFOHLENE VERBESSERUNGEN

### **Improvement #1: View Clear mit Background Reset**

**Datei:** `src/lvgl_usr.cpp` Funktion `clear_all_view_elements()`

**Hinzufügen am Ende:**
```cpp
static void clear_all_view_elements(void)
{
    Serial.println("[View Clear] Starting AGGRESSIVE full clear...");
    
    // ... [bestehender Code] ...
    
    // WICHTIG: Background auf TRANSPARENT zurücksetzen!
    lv_obj_set_style_bg_opa(ui_ScreenPrinting, LV_OPA_TRANSP, 0);
    
    Serial.println("[View Clear] ✅ All elements hidden + background reset");
}
```

**Vorteil:** Sauberer Reset zwischen Views

---

### **Improvement #2: Dedizierter Temp Background Rect nutzen**

**Option:** Statt Screen Background zu ändern, nutze `s_temp_bg_rect`

**Datei:** `src/lvgl_usr.cpp` Funktion `ui_set_cycle_mode()`

**Ändern:**
```cpp
if (mode == 1) {
    // TEMP GRAPH MODE
    
    // Option A: Screen Background (aktuell)
    lv_obj_set_style_bg_color(ui_ScreenPrinting, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(ui_ScreenPrinting, LV_OPA_COVER, 0);
    
    // Option B: Dedizierter Rect (alternat)
    if (s_temp_bg_rect) {
        lv_obj_clear_flag(s_temp_bg_rect, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_to_index(s_temp_bg_rect, 1);  // Above GIF/Ring, below Chart
    }
}
```

**Empfehlung:** Option A (Screen Background) ist einfacher und funktioniert bereits!

---

## 🎯 ZUSAMMENFASSUNG

### Was bereits funktioniert ✅
1. Arc ändert nur Winkel (kein Styling im Update)
2. Z-Order wird nur einmal gesetzt
3. Arc ist korrekt 240×240 Pixel groß
4. Main-GIF-System ist dedupliziert (nur ein GIF aktiv)
5. Temp View Background ist `LV_OPA_COVER`

### Was verbessert werden kann ⚠️
1. `clear_all_view_elements()` sollte Background auf TRANSPARENT zurücksetzen
2. Eventuell dedizierter Temp Background Rect statt Screen Background

### Was getestet werden muss 🧪
1. View-Wechsel: Progress → Temp → Main GIF → Progress
2. Keine bunten Ringe im Temp View sichtbar?
3. Smooth Wechsel ohne Flickering?
4. Performance stabil >20 FPS?

---

## 📝 NÄCHSTE SCHRITTE

1. **Testen** - Kompilieren und auf Hardware testen:
   ```bash
   cd /Users/ShotsOfReality/Downloads/KNOMI_6_VORON
   pio run -t upload -t monitor
   ```

2. **Beobachten** im Serial Monitor:
   - `[Progress Layers] ✅ Layers ordered ONCE` - nur EINMAL!
   - `[View] Activating X mode` - bei jedem Wechsel
   - `[View Clear] ✅ All elements hidden` - vor jedem Wechsel

3. **Visuell prüfen:**
   - [ ] Progress View: Schwarzer Arc sichtbar und maskiert Ring?
   - [ ] Temp View: KOMPLETT schwarzer Hintergrund ohne Ringe?
   - [ ] Main GIF: Tool-Nummer GIF läuft smooth?
   - [ ] Wechsel: Keine Überlagerungen zwischen Views?

4. **Optional verbessern:**
   - Background Reset in `clear_all_view_elements()` hinzufügen
   - Performance-Logs erweitern für besseres Debugging

---

**Fazit:** Die meisten Fixes sind bereits korrekt implementiert! 🎉  
Die Code-Qualität ist deutlich besser als nach den ChatGPT-Änderungen.  
Hauptsächlich sind jetzt Tests auf echter Hardware nötig.

**Erstellt von Claude - 18. Oktober 2025**
