#include <stdio.h> // sprintf
#include <string.h>
#include <Arduino.h>

#include "lvgl_hal.h"
#include "lvgl_usr.h"
#include "ui/ui.h"
#include "moonraker.h"
#include "ui_overlay/lv_overlay.h"
#include "fs_gif_loader.h"
#include "power_management/display_sleep.h"

// ========================================================================
// GLOBAL: GIF object for Main Screen (WITHOUT static!)
// ========================================================================

lv_obj_t * ui_img_main_gif;  // ← NEU!

extern "C" void ui_set_temp_view(bool on); // implemented in ui_ScreenPrinting.c


// ==== Extern UI objects from UI (needed for toggling visibility) =================
extern lv_obj_t * ui_temp_bg_gif;
extern lv_obj_t * ui_temp_chart;
extern lv_obj_t * ui_label_temp_title;
extern lv_obj_t * ui_label_temp_current;
extern lv_obj_t * ui_label_printing_progress;
extern lv_obj_t * ui_label_progress_eta;
extern lv_obj_t * ui_label_progress_layer;
extern lv_obj_t * ui_label_tool_indicator;
extern lv_obj_t * ui_tool_bg_circle;
extern lv_obj_t * ui_ScreenPrinting;
extern lv_obj_t * ui_gif_print_progress_bg;  // SECOND ring from ui_ScreenPrinting.c!

// ==== Background elements from lv_print_progress_update.cpp ================
extern lv_obj_t * ui_arc_progress_cover;  // Black arc that covers undone progress
extern lv_obj_t * get_ui_bg_gif(void);      // Background GIF (animated glow)
extern lv_obj_t * get_ui_bg_ring_img(void); // PNG ring (static colorful ring)
extern void delete_ui_bg_ring(void);        // Delete the ring completely
extern void delete_ui_bg_gif(void);         // Delete the background GIF
extern lv_obj_t * ui_main_screen_gif;       // Main screen GIF from lv_print_progress_update.cpp!

// We re-use the existing FS helper to load a tool GIF
extern void fs_set_tool_gif(lv_obj_t * obj, int tool_index);
extern int  detect_my_tool_number(void);
// Progress updates enable/disable flag
static bool g_ui_progress_updates_enabled = true;

// Enable/disable progress updates (replaces weak stub)
extern "C" void ui_progress_enable(bool on) {
    g_ui_progress_updates_enabled = on;
    Serial.printf("[Progress] Updates %s\n", on ? "ENABLED" : "DISABLED");
}

// Getter for progress enabled state
extern "C" bool ui_progress_is_enabled(void) {
    return g_ui_progress_updates_enabled;
}

// External declaration for lv_print_progress_update.cpp
extern "C" bool ui_get_progress_enabled(void) {
    return g_ui_progress_updates_enabled;
}

#if LV_USE_IMG
extern const lv_obj_class_t lv_img_class;
#endif
#if LV_USE_GIF
extern const lv_obj_class_t lv_gif_class;
// --- GIF pause/resume compatibility (v8/v9) -------------------------------
static inline void gif_pause_compat(lv_obj_t *o) {
#if defined(LVGL_VERSION_MAJOR) && (LVGL_VERSION_MAJOR >= 9)
    lv_gif_pause(o);
#else
    // LVGL v8: no pause API; hiding is done by caller
    (void)o;
#endif
}
static inline void gif_resume_compat(lv_obj_t *o) {
#if defined(LVGL_VERSION_MAJOR) && (LVGL_VERSION_MAJOR >= 9)
    lv_gif_resume(o);
#else
    // LVGL v8: restart to simulate resume
    lv_gif_restart(o);
#endif
}
#endif

// Recursively hide all objects of a given LVGL class under a root object
static void hide_by_class_recursive(lv_obj_t *root, const lv_obj_class_t *klass) {
    if (!root) return;
    if (lv_obj_check_type(root, klass)) {
        lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN);
    }
    uint32_t n = lv_obj_get_child_cnt(root);
    for (uint32_t i = 0; i < n; i++) {
        hide_by_class_recursive(lv_obj_get_child(root, i), klass);
    }
}

#if LV_USE_GIF
static void gif_pause_recursive(lv_obj_t *root) {
    if (!root) return;
    if (lv_obj_check_type(root, &lv_gif_class)) gif_pause_compat(root);
    uint32_t n = lv_obj_get_child_cnt(root);
    for (uint32_t i = 0; i < n; i++) gif_pause_recursive(lv_obj_get_child(root, i));
}
static void gif_resume_recursive(lv_obj_t *root) {
    if (!root) return;
    if (lv_obj_check_type(root, &lv_gif_class)) gif_resume_compat(root);
    uint32_t n = lv_obj_get_child_cnt(root);
    for (uint32_t i = 0; i < n; i++) gif_resume_recursive(lv_obj_get_child(root, i));
}
#endif


static void ui_drop_image_caches(void) {
#if defined(LVGL_VERSION_MAJOR) && (LVGL_VERSION_MAJOR >= 9)
    lv_image_cache_drop(NULL);
    lv_image_header_cache_drop(NULL);
#else
    /* v8 API */
    lv_img_cache_invalidate_src(NULL);
#endif
}

// --- TEMP view solid black background (overlay rect) ---
static lv_obj_t * s_temp_bg_rect = NULL;

static void ensure_temp_bg_rect(void) {
    if (s_temp_bg_rect || !ui_ScreenPrinting) return;
    s_temp_bg_rect = lv_obj_create(ui_ScreenPrinting);
    lv_obj_remove_style_all(s_temp_bg_rect);
    lv_obj_set_size(s_temp_bg_rect, LV_PCT(100), LV_PCT(100));
    lv_obj_center(s_temp_bg_rect);
    lv_obj_set_style_bg_color(s_temp_bg_rect, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_temp_bg_rect, LV_OPA_COVER, 0);
    lv_obj_add_flag(s_temp_bg_rect, LV_OBJ_FLAG_HIDDEN);
    // keep it behind other widgets by default
    lv_obj_move_background(s_temp_bg_rect);
}

// A dedicated GIF instance shown ON the Printing Screen when cycle mode == 2
static lv_obj_t * s_main_gif_on_printing = NULL;
static void ensure_main_gif_on_printing(void) {
#if LV_USE_GIF
    if (!s_main_gif_on_printing) {
        s_main_gif_on_printing = lv_gif_create(ui_ScreenPrinting);
        lv_obj_set_size(s_main_gif_on_printing, 240, 240);
        lv_obj_center(s_main_gif_on_printing);
        fs_set_tool_gif(s_main_gif_on_printing, detect_my_tool_number());
        lv_obj_add_flag(s_main_gif_on_printing, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_to_index(s_main_gif_on_printing, 1); // hinter Labels, vor BG
    }
#endif
}

// -------------------------- Core view toggles -----------------------------------
extern "C" void ui_set_temp_view(bool on) {
    // Temp-Grafik zeigen/verstecken
    if (ui_temp_bg_gif)      { lv_obj_add_flag(ui_temp_bg_gif, LV_OBJ_FLAG_HIDDEN); }
    if (ui_temp_chart)       { if (on) lv_obj_clear_flag(ui_temp_chart, LV_OBJ_FLAG_HIDDEN);       else lv_obj_add_flag(ui_temp_chart, LV_OBJ_FLAG_HIDDEN); }
    if (ui_label_temp_title) { if (on) lv_obj_clear_flag(ui_label_temp_title, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(ui_label_temp_title, LV_OBJ_FLAG_HIDDEN); }
    if (ui_label_temp_current){if (on) lv_obj_clear_flag(ui_label_temp_current, LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(ui_label_temp_current, LV_OBJ_FLAG_HIDDEN); }

    // Progress-Labels spiegelbildlich
    if (ui_label_printing_progress){ if (on) lv_obj_add_flag(ui_label_printing_progress, LV_OBJ_FLAG_HIDDEN); else lv_obj_clear_flag(ui_label_printing_progress, LV_OBJ_FLAG_HIDDEN); }
    if (ui_label_progress_eta)     { if (on) lv_obj_add_flag(ui_label_progress_eta, LV_OBJ_FLAG_HIDDEN);      else lv_obj_clear_flag(ui_label_progress_eta, LV_OBJ_FLAG_HIDDEN); }
    if (ui_label_progress_layer)   { if (on) lv_obj_add_flag(ui_label_progress_layer, LV_OBJ_FLAG_HIDDEN);    else lv_obj_clear_flag(ui_label_progress_layer, LV_OBJ_FLAG_HIDDEN); }
    if (ui_label_tool_indicator)   { if (on) lv_obj_add_flag(ui_label_tool_indicator, LV_OBJ_FLAG_HIDDEN);    else lv_obj_clear_flag(ui_label_tool_indicator, LV_OBJ_FLAG_HIDDEN); }

    // Glow dezent halten, Z-Order
    if (ui_tool_bg_circle) {
        lv_obj_set_style_shadow_opa(ui_tool_bg_circle, on ? 90 : 140, 0);
        lv_obj_move_background(ui_tool_bg_circle);
    }

    if (on) {
        if (ui_temp_chart)          lv_obj_move_foreground(ui_temp_chart);
        if (ui_label_temp_title)    lv_obj_move_foreground(ui_label_temp_title);
        if (ui_label_temp_current)  lv_obj_move_foreground(ui_label_temp_current);
    } else {
        if (ui_label_printing_progress) lv_obj_move_foreground(ui_label_printing_progress);
        if (ui_label_progress_eta)      lv_obj_move_foreground(ui_label_progress_eta);
        if (ui_label_progress_layer)    lv_obj_move_foreground(ui_label_progress_layer);
    }

#if LV_USE_GIF
    if (s_main_gif_on_printing) lv_obj_add_flag(s_main_gif_on_printing, LV_OBJ_FLAG_HIDDEN);
#endif
}

// ========================================================================
// CLEAR FUNCTION: Hide all elements before switching views
// ========================================================================
static void clear_all_view_elements(void)
{
    Serial.println("[View Clear] Starting AGGRESSIVE full clear...");
    
    // ===== Hide ALL Progress View elements =====
    if (ui_label_printing_progress) lv_obj_add_flag(ui_label_printing_progress, LV_OBJ_FLAG_HIDDEN);
    if (ui_label_progress_eta)      lv_obj_add_flag(ui_label_progress_eta, LV_OBJ_FLAG_HIDDEN);
    if (ui_label_progress_layer)    lv_obj_add_flag(ui_label_progress_layer, LV_OBJ_FLAG_HIDDEN);
    if (ui_label_tool_indicator)    lv_obj_add_flag(ui_label_tool_indicator, LV_OBJ_FLAG_HIDDEN);
    
    // ===== DELETE Progress Background Elements (CRITICAL!) =====
    // IMPORTANT: Actually DELETE them instead of just hiding!
    // This ensures they won't show through in other modes
    delete_ui_bg_ring();  // Completely remove the ring from lv_print_progress_update.cpp
    delete_ui_bg_gif();   // Completely remove the background GIF
    
    // CRITICAL: Also hide the SECOND ring from ui_ScreenPrinting.c!
    if (ui_gif_print_progress_bg) {
        lv_obj_add_flag(ui_gif_print_progress_bg, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_opa(ui_gif_print_progress_bg, LV_OPA_TRANSP, 0);
        Serial.println("[View Clear] SECOND Background Ring (ui_gif_print_progress_bg) hidden + transparent");
    }
    
    Serial.println("[View Clear] ALL Background Rings & GIF processed");
    
    if (ui_arc_progress_cover) {
        lv_obj_add_flag(ui_arc_progress_cover, LV_OBJ_FLAG_HIDDEN);
        Serial.println("[View Clear] Progress Arc hidden");
    }
    
    // ===== Hide Tool Glow Circle =====
    if (ui_tool_bg_circle) {
        lv_obj_add_flag(ui_tool_bg_circle, LV_OBJ_FLAG_HIDDEN);
        Serial.println("[View Clear] Glow Circle hidden");
    }
    
    // ===== Hide ALL Temp Graph elements =====
    if (ui_temp_bg_gif)      lv_obj_add_flag(ui_temp_bg_gif, LV_OBJ_FLAG_HIDDEN);
    if (ui_temp_chart)       lv_obj_add_flag(ui_temp_chart, LV_OBJ_FLAG_HIDDEN);
    if (ui_label_temp_title) lv_obj_add_flag(ui_label_temp_title, LV_OBJ_FLAG_HIDDEN);
    if (ui_label_temp_current) lv_obj_add_flag(ui_label_temp_current, LV_OBJ_FLAG_HIDDEN);
    
    // ===== Hide Main GIF =====
#if LV_USE_GIF
    if (s_main_gif_on_printing) {
        gif_pause_compat(s_main_gif_on_printing);
        lv_obj_add_flag(s_main_gif_on_printing, LV_OBJ_FLAG_HIDDEN);
        Serial.println("[View Clear] Main GIF (s_main_gif_on_printing) paused+hidden");
    }
#endif
    
    // CRITICAL: Hide the SECOND Main GIF from lv_print_progress_update.cpp **only if it lives on the Printing screen**
    if (ui_main_screen_gif) {
        lv_obj_t *scr_of_gif = lv_obj_get_screen(ui_main_screen_gif);
        if (scr_of_gif == ui_ScreenPrinting) {
            lv_obj_add_flag(ui_main_screen_gif, LV_OBJ_FLAG_HIDDEN);
            Serial.println("[View Clear] Main Screen GIF (ui_main_screen_gif) hidden [on Printing]");
        }
    }
    
    // No forced lv_refr_now(NULL) here -- handled in ui_set_cycle_mode
    if (s_temp_bg_rect) lv_obj_add_flag(s_temp_bg_rect, LV_OBJ_FLAG_HIDDEN);
    Serial.println("[View Clear] ✅ All elements hidden");
}

static uint8_t s_current_mode = 255; // dedupe current mode
extern "C" void ui_set_cycle_mode(uint8_t mode)
{
    // Dedupe: do nothing if mode unchanged
    if (mode == s_current_mode) return;
    s_current_mode = mode;
    ensure_temp_bg_rect();

    // ========================================================================
    // STEP 1: CLEAR ALL ELEMENTS - Prevent overlays!
    // ========================================================================
    clear_all_view_elements();
    
    Serial.printf("[View Switch] Switching to mode %d\n", mode);

    // ========================================================================
    // STEP 2: SHOW ELEMENTS FOR SELECTED MODE
    // ========================================================================
    
    // 0: Progress, 1: Temp Graph, 2: Main GIF (auf Printing-Screen)
    if (mode == 1) {
        // ===== MODE 1: TEMP GRAPH =====
        // Minimalistisch: Nur schwarzer Background + Graph + Fonts
        Serial.println("[View] Activating TEMP GRAPH mode (minimal)");

        // CRITICAL: DELETE all Progress elements for clean Temp Graph!
        // This prevents the colorful ring from showing through
        delete_ui_bg_ring();  // DELETE the ring completely!
        delete_ui_bg_gif();   // DELETE the background GIF too!
        
        // CRITICAL: Also ensure SECOND ring stays hidden!
        if (ui_gif_print_progress_bg) {
            lv_obj_add_flag(ui_gif_print_progress_bg, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_opa(ui_gif_print_progress_bg, LV_OPA_TRANSP, 0);
            Serial.println("[View] TEMP MODE: SECOND ring (ui_gif_print_progress_bg) hidden + transparent");
        }
        
        Serial.println("[View] TEMP MODE: All progress backgrounds DELETED/HIDDEN for clean view");
        if (ui_arc_progress_cover) {
            lv_obj_add_flag(ui_arc_progress_cover, LV_OBJ_FLAG_HIDDEN);
            Serial.println("[View] TEMP MODE: Arc hidden");
        }
        if (ui_tool_bg_circle) {
            lv_obj_add_flag(ui_tool_bg_circle, LV_OBJ_FLAG_HIDDEN);
            Serial.println("[View] TEMP MODE: Glow Circle hidden");
        }

        // Solid black screen background - CRITICAL for clean temp view!
        lv_obj_set_style_bg_color(ui_ScreenPrinting, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(ui_ScreenPrinting, LV_OPA_COVER, 0);
        Serial.println("[View] TEMP MODE: Black background set");
        
        // Enable temp view elements
        if (ui_temp_chart) {
            lv_obj_clear_flag(ui_temp_chart, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_to_index(ui_temp_chart, 20);  // ABOVE everything else!
        }
        if (ui_label_temp_title) {
            lv_obj_clear_flag(ui_label_temp_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_to_index(ui_label_temp_title, 21);  // ABOVE everything else!
        }
        if (ui_label_temp_current) {
            lv_obj_clear_flag(ui_label_temp_current, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_to_index(ui_label_temp_current, 22);  // ABOVE everything else!
        }

        // Disable progress updates
        ui_progress_enable(false);

    } else if (mode == 2) {
        // ===== MODE 2: MAIN GIF (Tool Number) =====
        // Standard: Nur das GIF, keine Erweiterungen
        Serial.println("[View] Activating MAIN GIF mode (standard)");
        
        // Show Main GIF
#if LV_USE_GIF
        ensure_main_gif_on_printing();
        if (s_main_gif_on_printing) {
            gif_resume_compat(s_main_gif_on_printing);
            lv_obj_clear_flag(s_main_gif_on_printing, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(s_main_gif_on_printing);
        }
#endif
        
        // NO Glow Circle in Main GIF Mode!
        // Glow Circle stays hidden for standard view
        
        // Disable progress updates
        ui_progress_enable(false);
        
    } else {
        // ===== MODE 0: PROGRESS VIEW (Default) =====
        Serial.println("[View] Activating PROGRESS mode");

        // CRITICAL: Set BLACK background FIRST to prevent white flash!
        lv_obj_set_style_bg_color(ui_ScreenPrinting, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(ui_ScreenPrinting, LV_OPA_COVER, 0);
        
        // OPTIMIZATION: Removed lv_refr_now() - let LVGL scheduler handle it for smoother performance
        Serial.println("[View] PROGRESS MODE: Black background set (scheduler will refresh)");

        // Show Progress Background Elements
        // OPTIMIZATION: GIF already exists from preload, just show it
        show_ui_bg_gif();  // Resume and show GIF
        
        lv_obj_t * bg_ring = get_ui_bg_ring_img();
        if (bg_ring) {
            lv_obj_clear_flag(bg_ring, LV_OBJ_FLAG_HIDDEN);
            Serial.println("[View] Background Ring shown");
        }
        
        // CRITICAL: Also show the SECOND ring in Progress mode!
        if (ui_gif_print_progress_bg) {
            lv_obj_clear_flag(ui_gif_print_progress_bg, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_opa(ui_gif_print_progress_bg, LV_OPA_100, 0);
            Serial.println("[View] SECOND Background Ring (ui_gif_print_progress_bg) shown");
        }
        if (ui_arc_progress_cover) {
            lv_obj_clear_flag(ui_arc_progress_cover, LV_OBJ_FLAG_HIDDEN);
            Serial.println("[View] Progress Arc shown");
        }

        // Enable progress elements
        if (ui_label_printing_progress) lv_obj_clear_flag(ui_label_printing_progress, LV_OBJ_FLAG_HIDDEN);
        if (ui_label_progress_layer)    lv_obj_clear_flag(ui_label_progress_layer, LV_OBJ_FLAG_HIDDEN);
        if (ui_label_tool_indicator)    lv_obj_clear_flag(ui_label_tool_indicator, LV_OBJ_FLAG_HIDDEN);

        // Show Glow Circle
        if (ui_tool_bg_circle) {
            lv_obj_clear_flag(ui_tool_bg_circle, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_shadow_opa(ui_tool_bg_circle, 140, 0);
            Serial.println("[View] Glow Circle shown (strong)");
        }

        // Enable progress updates
        ui_progress_enable(true);
    }

    // Let LVGL schedule the refresh via lv_timer_handler(); avoids blocking spikes
    Serial.println("[View] Mode switch complete");
}

// Define cycle timer tick period in ms
#define UI_CYCLE_TICK_MS 200
// -------------------------- Auto Cycle Timer ------------------------------------
static lv_timer_t * s_cycle_timer = NULL;
static bool      s_cycle_running = false;
static uint8_t   s_cycle_phase   = 0;   // 0: Progress, 1: Temp, 2: Main
static int32_t   s_phase_left_ms = 0;   // Countdown

static inline int32_t cycle_phase_duration_ms(uint8_t p) {
    switch (p) {
        case 0: return 10000; // Progress 10s
        case 1: return  8000; // Temp 8s
        case 2: return  8000; // Main GIF 8s
        default: return 10000;
    }
}

static void cycle_apply(uint8_t p) {
    ui_set_cycle_mode(p);
    s_cycle_phase   = p;
    s_phase_left_ms = cycle_phase_duration_ms(p);
}

static void cycle_timer_cb(lv_timer_t * t) {
    if (!s_cycle_running) return;
    s_phase_left_ms -= UI_CYCLE_TICK_MS;
    if (s_phase_left_ms <= 0) {
        uint8_t next = (s_cycle_phase + 1u) % 3u;
        cycle_apply(next);
    }
}

extern "C" void ui_auto_cycle_start(void) {
    if (s_cycle_running) {
        return; // already running
    }
    if (!s_cycle_timer) {
        s_cycle_timer = lv_timer_create(cycle_timer_cb, UI_CYCLE_TICK_MS, NULL); // 5 Hz
    }
    s_cycle_running = true;
    lv_timer_resume(s_cycle_timer);
    cycle_apply(0); // start with Progress without resetting s_current_mode blindly
}

extern "C" void ui_auto_cycle_stop(void) {
    // Already stopped and already in mode 0? Nothing to do.
    if (!s_cycle_running && s_current_mode == 0) {
        return;
    }

    s_cycle_running = false;
    if (s_cycle_timer) lv_timer_pause(s_cycle_timer);

    // CRITICAL: Delete Progress Background GIF to free CPU!
    // When cycle stops, we no longer need the Progress background
    Serial.println("[Cycle Stop] Deleting Progress Background GIF for better Main GIF performance");
    delete_ui_bg_gif();
    delete_ui_bg_ring();

    // Halt background work
    ui_progress_enable(false);
#if LV_USE_GIF
    // Pause all GIFs on Printing screen
    if (ui_ScreenPrinting) gif_pause_recursive(ui_ScreenPrinting);
#endif
    ui_drop_image_caches();

    // Only switch if not already in Progress view 0
    if (s_current_mode != 0) {
        ui_set_cycle_mode(0);
    }
    
    Serial.println("[Cycle Stop] Main GIF should now run at full speed");
}


/****************** lvgl ui call function ******************/
//
void lv_tft_set_backlight(lv_event_t * e) {
    int32_t light = lv_slider_get_value(ui_slider_backlight);
    tft_set_backlight(light);
}

// extruder speed
void lv_btn_set_extrude(lv_event_t * e) {
    // Initialize parameter values from roller settings
    char roller_str[10];
    lv_roller_get_selected_str(ui_roller_set_extrude_length, roller_str, sizeof(roller_str));
    lv_label_set_text(ui_label_extruder_length, roller_str);
    uint32_t sel = lv_roller_get_selected(ui_roller_set_extrude_length);
    lv_obj_set_user_data(ui_label_extruder_length, (void *)sel);
    lv_roller_get_selected_str(ui_roller_set_extrude_speed, roller_str, sizeof(roller_str));
    lv_label_set_text(ui_label_extruder_speed, roller_str);
    sel = lv_roller_get_selected(ui_roller_set_extrude_speed);
    lv_obj_set_user_data(ui_label_extruder_speed, (void *)sel);
}

// set extruder roller
void lv_roller_set_extrude(lv_event_t * e) {
    // Initialize parameter values from roller settings
    uint32_t sel = (uint32_t)lv_obj_get_user_data(ui_label_extruder_length);
    lv_roller_set_selected(ui_roller_set_extrude_length, sel, LV_ANIM_OFF);

    sel = (uint32_t)lv_obj_get_user_data(ui_label_extruder_speed);
    lv_roller_set_selected(ui_roller_set_extrude_speed, sel, LV_ANIM_OFF);
}
/***********************************************************/


void lv_popup_warning(const char * warning, bool clickable);
void lv_popup_remove(lv_event_t * e) ;
// lvgl ui
void lvgl_ui_task(void * parameter) {
    lv_btn_init();
    lvgl_hal_init();
    ui_init();
    
    // ========================================================================
    // Initialize Display Sleep Management
    // ========================================================================
    display_sleep_init(SLEEP_MODE_KLIPPER_SYNC);
    Serial.println("[INIT] Display Sleep Management initialized!");

#ifndef LIS2DW_SUPPORT
    // progress in center if no lis2dw accelerometer data to display
    lv_obj_set_y(ui_label_printing_progress, 0);
    lv_obj_set_align(ui_label_printing_progress, LV_ALIGN_CENTER);
    // delete unused accelerometer data
    lv_obj_del(ui_slider_printing_acc_x);
    lv_obj_del(ui_slider_printing_acc_y);
    lv_obj_del(ui_slider_printing_acc_z);
    lv_obj_del(ui_label_printing_acc_x);
    lv_obj_del(ui_label_printing_acc_y);
    lv_obj_del(ui_label_printing_acc_z);
#endif

    lv_obj_t * label = lv_label_create(ui_ScreenTestImg);
    lv_obj_set_size(label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_label_set_text_static(label, FW_VERSION);

    // Add all button style
    lv_btn_add_style();

    // Set theme color
    lv_theme_color_style();

    // ========================================================================
    // Mount LittleFS (LVGL driver not needed, as we use PSRAM)
    // ========================================================================
    fs_mount_once();                // LittleFS.begin()
    Serial.println("[INIT] LittleFS ready!");
    
    // ========================================================================
    // PERFORMANCE: Preload Progress Background GIF into PSRAM
    // ========================================================================
    preload_progress_gif_data();    // Load GIF at startup for instant display
    Serial.println("[INIT] Progress GIF preloaded into PSRAM!");
    
    // ========================================================================
    // GIF-Objekt erstellen
    // ========================================================================
    ui_img_main_gif = lv_gif_create(ui_ScreenMainGif);
    if (!ui_img_main_gif) {
        Serial.println("CRITICAL ERROR: Failed to create ui_img_main_gif! Halting.");
        while(1) { delay(1000); }  // System halt - cannot continue without main GIF
    }
    lv_obj_align(ui_img_main_gif, LV_ALIGN_CENTER, 0, 0);
    
    // Initial Tool-GIF aus LittleFS in PSRAM laden
    fs_set_tool_gif(ui_img_main_gif, detect_my_tool_number());

    // Add logo gif

    // Add welcome gif
    lv_obj_t * img_welcome_gif = lv_gif_create(ui_ScreenWelcome);
    lv_gif_set_src(img_welcome_gif, &gif_welcome);
    lv_obj_align(img_welcome_gif, LV_ALIGN_CENTER, 0, -36);

    // Create a QR Code
    lv_obj_t * qr = lv_qrcode_create(ui_ScreenQRCode, 130, LV_COLOR_MAKE(0xff, 0xff, 0xff), LV_COLOR_MAKE(0, 0, 0));
    const char * data = "https://bigtreetech.github.io/docs/KNOMI2.html";
    lv_qrcode_update(qr, data, strlen(data));
    lv_obj_center(qr);

    // Initialize extruder speed/length roller options
    const char *extrude_len = {
        EXTRUDE_MM_0_LABEL "\n"\
        EXTRUDE_MM_1_LABEL "\n"\
        EXTRUDE_MM_2_LABEL "\n"\
        EXTRUDE_MM_3_LABEL "\n"\
        EXTRUDE_MM_4_LABEL
    };
    lv_roller_set_options(ui_roller_set_extrude_length, extrude_len, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(ui_roller_set_extrude_length, 1, LV_ANIM_ON); // 5mm
    const char *EXTRUDE_MM_S = {
        EXTRUDE_MM_S_0_LABEL "\n"\
        EXTRUDE_MM_S_1_LABEL "\n"\
        EXTRUDE_MM_S_2_LABEL "\n"\
        EXTRUDE_MM_S_3_LABEL "\n"\
        EXTRUDE_MM_S_4_LABEL
    };
    lv_roller_set_options(ui_roller_set_extrude_speed, EXTRUDE_MM_S, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(ui_roller_set_extrude_speed, 2, LV_ANIM_ON); // 10mm/s
    // Initialize extruder speed/length values from roller settings
    lv_btn_set_extrude(NULL);

    ui_set_temp_view(false);
    ui_progress_enable(true);

    for(;;) {
        // lvgl task, must run in loop first.
        lv_timer_handler();

        wifi_status_t status = wifi_get_connect_status();

        lv_loop_wifi_change_screen(status);

        if (status == WIFI_STATUS_CONNECTED) {

            lv_loop_popup_screen();
            lv_loop_set_temp_screen();

            if (!moonraker.unconnected && !moonraker.unready) {
                if (moonraker.data_unlock) {
                    lv_loop_moonraker_change_screen();
                }
                lv_loop_moonraker_change_screen_value();
            }
        }

        lv_loop_auto_idle(status);
        lv_loop_btn_event();

        delay(5);
    }
}
