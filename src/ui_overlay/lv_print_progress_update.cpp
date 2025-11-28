#include "ui/ui.h"
#include "ui_overlay/lv_overlay.h"
#include "lvgl.h"
#include <LittleFS.h>  // For direct file loading
#include <cstring>

// Extern UI objects created elsewhere
extern lv_obj_t * ui_ScreenPrinting;
extern lv_obj_t * ui_arc_progress_cover;
extern lv_obj_t * ui_label_printing_progress;
extern lv_obj_t * ui_label_progress_eta;
extern lv_obj_t * ui_label_progress_layer;
extern lv_obj_t * ui_tool_bg_circle;        // center glow circle (defined in ui_ScreenPrinting.c)
extern lv_obj_t * ui_label_tool_indicator;  // center label (defined in ui_ScreenPrinting.c)
// Screen Rotation Views
extern lv_obj_t * ui_temp_chart;
extern lv_obj_t * ui_label_temp_title;
extern lv_obj_t * ui_label_temp_current;
extern lv_obj_t * ui_main_screen_gif;

// PNG ring from C-array (defined in src/ui/images/ui_img_progress_ring.c)
extern const lv_img_dsc_t ui_img_progress_ring;

// Background layer handles
static lv_obj_t * ui_bg_gif = nullptr;       // furthest back: animated glow from PSRAM
static lv_obj_t * ui_bg_ring_img = nullptr;  // above GIF: static colorful ring PNG

// Getter functions to access static variables from other files
lv_obj_t * get_ui_bg_gif(void) { return ui_bg_gif; }
lv_obj_t * get_ui_bg_ring_img(void) { return ui_bg_ring_img; }

// Function to delete the ring when switching to Temp mode
void delete_ui_bg_ring(void) {
    if (ui_bg_ring_img) {
        // Force immediate hide before delete
        lv_obj_add_flag(ui_bg_ring_img, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_opa(ui_bg_ring_img, LV_OPA_TRANSP, 0);
        lv_obj_invalidate(ui_bg_ring_img);  // Force redraw
        lv_obj_del(ui_bg_ring_img);
        ui_bg_ring_img = nullptr;
        Serial.println("[Progress] Background Ring DELETED (forced hidden first)");
    }
}

// GIF pause/resume compatibility helpers (inline for this file)
#if LV_USE_GIF
static inline void gif_pause_compat(lv_obj_t *o) {
#if defined(LVGL_VERSION_MAJOR) && (LVGL_VERSION_MAJOR >= 9)
    lv_gif_pause(o);
#else
    // LVGL v8: no pause API; just hiding is sufficient
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

// PSRAM buffer + descriptor for progress GIF (declare early for use in functions below)
static uint8_t * progress_gif_data = nullptr;
static size_t progress_gif_size = 0;
static lv_img_dsc_t progress_gif_descriptor;  // IMPORTANT: Descriptor for LVGL

// Function to delete the background GIF when switching modes
// OPTIMIZATION: Data stays in PSRAM for fast recreation
void delete_ui_bg_gif(void) {
    if (ui_bg_gif) {
        // Actually delete to free CPU resources
        lv_obj_del(ui_bg_gif);
        ui_bg_gif = nullptr;
        Serial.println("[Progress] Background GIF DELETED (data stays in PSRAM)");
    }
}

// Function to show/create the background GIF from preloaded data
void show_ui_bg_gif(void) {
    // If already exists, just ensure it's visible
    if (ui_bg_gif) {
        lv_obj_clear_flag(ui_bg_gif, LV_OBJ_FLAG_HIDDEN);
        Serial.println("[Progress] Background GIF shown (already exists)");
        return;
    }
    
    // Create from preloaded data - FAST because data is already in PSRAM
    if (progress_gif_data && ui_ScreenPrinting) {
        ui_bg_gif = lv_gif_create(ui_ScreenPrinting);
        lv_obj_clear_flag(ui_bg_gif, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_size(ui_bg_gif, 240, 240);
        lv_obj_center(ui_bg_gif);
        lv_obj_set_style_opa(ui_bg_gif, LV_OPA_COVER, 0);
        lv_obj_move_background(ui_bg_gif);
        lv_obj_move_to_index(ui_bg_gif, 0);
        
        // Set source from preloaded descriptor
        lv_gif_set_src(ui_bg_gif, &progress_gif_descriptor);
        
        Serial.println("[Progress] Background GIF CREATED from preloaded PSRAM data!");
    } else {
        Serial.println("[Progress] ERROR: Cannot create GIF - data not preloaded!");
    }
}

// PSRAM buffer + descriptor for main screen GIF
static uint8_t * main_screen_gif_data = nullptr;
static size_t main_screen_gif_size = 0;
static lv_img_dsc_t main_screen_gif_descriptor;
static int current_tool_gif = -1;  // Track which tool GIF is loaded

// Preload GIF data into PSRAM for fast creation when needed
// OPTIMIZATION: Only preload data, create object on-demand for best CPU performance
void preload_progress_gif_data(void)
{
    if (progress_gif_data) return;  // Already loaded
    
    // Load data into PSRAM
    File file = LittleFS.open("/gifs/print_progress_bg.gif", "r");
    if (file) {
        progress_gif_size = file.size();
        progress_gif_data = (uint8_t*)ps_malloc(progress_gif_size);
        if (progress_gif_data) {
            file.read(progress_gif_data, progress_gif_size);
            Serial.printf("[Progress] PRELOADED background GIF data: %u bytes in PSRAM\n", progress_gif_size);
            
            // Initialize descriptor structure
            progress_gif_descriptor.header.always_zero = 0;
            progress_gif_descriptor.header.w = 0;
            progress_gif_descriptor.header.h = 0;
            progress_gif_descriptor.data_size = progress_gif_size;
            progress_gif_descriptor.header.cf = LV_IMG_CF_RAW;
            progress_gif_descriptor.data = progress_gif_data;
        }
        file.close();
    }
    
    // Note: GIF object will be created on-demand by show_ui_bg_gif() for optimal performance
}

// Create animated background GIF (once) - uses preloaded object
static void ensure_bg_gif_created()
{
    // GIF should already exist from preload - just ensure it's there
    if (!ui_bg_gif && progress_gif_data && ui_ScreenPrinting) {
        Serial.println("[Progress] GIF object missing, recreating...");
        preload_progress_gif_data();
    }
}

// Create static PNG ring (once) - ONLY IF PROGRESS MODE IS ENABLED!
static void ensure_bg_ring_created()
{
    if (ui_bg_ring_img || !ui_ScreenPrinting) return;
    
    // CRITICAL: Only create ring if progress mode is enabled!
    if (!ui_get_progress_enabled()) {
        return;  // Don't create ring in non-progress modes
    }
    
    // OPTIMIZATION: Ensure screen stays black
    lv_obj_set_style_bg_color(ui_ScreenPrinting, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(ui_ScreenPrinting, 255, 0);
    
    ui_bg_ring_img = lv_img_create(ui_ScreenPrinting);
    lv_img_set_src(ui_bg_ring_img, &ui_img_progress_ring);
    lv_obj_set_size(ui_bg_ring_img, 240, 240);
    lv_obj_center(ui_bg_ring_img);
    lv_obj_set_style_opa(ui_bg_ring_img, LV_OPA_100, 0);
    // Keep it truly behind arc/text but above GIF
    lv_obj_move_background(ui_bg_ring_img);
    lv_obj_move_to_index(ui_bg_ring_img, 1);
}

// REMOVED: Arc styling moved to ui_ScreenPrinting.c where it belongs!
// Arc should ONLY be styled during initialization, NOT in update loop

// Static variable for layer ordering - MUST be accessible for reset
static bool layers_ordered = false;

// Function to reset layer ordering when switching modes
void reset_layer_ordering(void) {
    layers_ordered = false;
}

// Ensure both background layers exist and are ordered correctly - ONLY ONCE!
// Z-Index Order (from back to front):
// 0: GIF Animation (background glow)
// 1: PNG Ring (static colorful ring)
// 2: Black Cover Arc (hides undone progress) - MUST be above PNG!
// 10: Tool Glow Circle (MUST be above Arc!)
// 11-14: All Labels (must be on top!)
static void ensure_bg_layers_created()
{
    // CRITICAL: Only create background layers if progress mode is enabled!
    if (!ui_get_progress_enabled()) {
        // If switching away from progress mode, clean up and reset
        if (layers_ordered) {
            delete_ui_bg_ring();
            delete_ui_bg_gif();
            layers_ordered = false;
        }
        return;  // Don't create any background in non-progress modes
    }
    
    ensure_bg_gif_created();
    ensure_bg_ring_created();
    
    // CRITICAL: Only order layers ONCE to avoid constant repaints!
    if (!layers_ordered) {
        // Keep background layers truly at the back
        if (ui_bg_gif) {
            lv_obj_move_to_index(ui_bg_gif, 0);
        }
        if (ui_bg_ring_img) {
            lv_obj_move_to_index(ui_bg_ring_img, 1);
        }

        // Arc MUST live on the screen-level parent and above the ring
        if (ui_arc_progress_cover) {
            if (lv_obj_get_parent(ui_arc_progress_cover) != ui_ScreenPrinting) {
                lv_obj_set_parent(ui_arc_progress_cover, ui_ScreenPrinting);
            }
            // CRITICAL: Arc must be ABOVE the ring!
            lv_obj_move_to_index(ui_arc_progress_cover, 2);
            // FORCE arc to foreground with move_foreground
            lv_obj_move_foreground(ui_arc_progress_cover);
            lv_obj_clear_flag(ui_arc_progress_cover, LV_OBJ_FLAG_HIDDEN);
        }

        if (ui_tool_bg_circle)           lv_obj_move_to_index(ui_tool_bg_circle, 10);

        // Ensure all labels are on ABSOLUTE TOP (highest z-index)
        if (ui_label_tool_indicator)     lv_obj_move_to_index(ui_label_tool_indicator, 50);
        if (ui_label_printing_progress)  lv_obj_move_to_index(ui_label_printing_progress, 51);
        if (ui_label_progress_eta)       lv_obj_move_to_index(ui_label_progress_eta, 52);
        if (ui_label_progress_layer)     lv_obj_move_to_index(ui_label_progress_layer, 53);
        
        // CRITICAL: Force labels to foreground as final step
        if (ui_label_tool_indicator)     lv_obj_move_foreground(ui_label_tool_indicator);
        if (ui_label_printing_progress)  lv_obj_move_foreground(ui_label_printing_progress);
        if (ui_label_progress_eta)       lv_obj_move_foreground(ui_label_progress_eta);
        if (ui_label_progress_layer)     lv_obj_move_foreground(ui_label_progress_layer);
        
        layers_ordered = true;
        Serial.printf("[Progress Layers] ✅ Layers ordered ONCE - GIF:%p Ring:%p Arc:%p Glow:%p\n",
            ui_bg_gif, ui_bg_ring_img, ui_arc_progress_cover, ui_tool_bg_circle);
    }
}

// ------------------------------------------------------------------------
// Progress % / Restzeit Toggle System (3 Sekunden)
// ------------------------------------------------------------------------
static bool show_progress_now = true;  // Toggle zwischen Progress % und Restzeit
static lv_timer_t *toggle_timer = NULL;
static int g_tool_temp = 0;      // Cached for toggle
static int g_eta_seconds = 0;    // Cached for toggle

// Temperature history for graph (last 60 values = 1 minute at 1s update)
static uint32_t last_temp_update_ms = 0;
#define TEMP_UPDATE_INTERVAL_MS 1000  // 1 Sekunde

#define TEMP_HISTORY_SIZE 60
static int16_t temp_history[TEMP_HISTORY_SIZE] = {0};
static uint8_t temp_history_index = 0;
static bool temp_history_filled = false;

// Toggle callback - wechselt zwischen Progress % und Restzeit
static void toggle_progress_eta(lv_timer_t *t) {
    LV_UNUSED(t);
    show_progress_now = !show_progress_now;
    
    // Update the label immediately
    if (ui_label_printing_progress) {
        if (show_progress_now) {
            // Will be updated in next update_print_progress call
            // (Shows Progress % - will be set by update_print_progress)
        } else {
            // Show ETA (Remaining Time)
            if (g_eta_seconds == -1) {
                // Special case: Calculating (not enough data yet)
                lv_label_set_text(ui_label_printing_progress, "---");
            } else if (g_eta_seconds <= 0) {
                lv_label_set_text(ui_label_printing_progress, "Done!");
            } else {
                int hours = g_eta_seconds / 3600;
                int minutes = (g_eta_seconds % 3600) / 60;
                if (hours > 0) {
                    lv_label_set_text_fmt(ui_label_printing_progress, "%dh %dm", hours, minutes);
                } else {
                    lv_label_set_text_fmt(ui_label_printing_progress, "%dm", minutes);
                }
            }
            
        }
    }
}

// Ensure toggle timer is running
static void ensure_toggle_timer(void) {
    if (!toggle_timer) {
        toggle_timer = lv_timer_create(toggle_progress_eta, 3000, NULL);  // Toggle alle 3 Sekunden
    }
}

static void add_temp_to_history(int temp) {
    temp_history[temp_history_index] = temp;
    temp_history_index++;
    if (temp_history_index >= TEMP_HISTORY_SIZE) {
        temp_history_index = 0;
        temp_history_filled = true;
    }
}

// Load and create Main Screen GIF for the current tool
static void ensure_main_screen_gif_created(int tool_number) {
    if (!ui_ScreenPrinting) return;
    
    // If GIF already created and correct tool, just make it visible
    if (ui_main_screen_gif && current_tool_gif == tool_number) {
        lv_obj_clear_flag(ui_main_screen_gif, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    
    // Need to load different tool GIF
    if (current_tool_gif != tool_number) {
        // Free old GIF data if exists
        if (main_screen_gif_data) {
            free(main_screen_gif_data);
            main_screen_gif_data = nullptr;
            main_screen_gif_size = 0;
        }
        
        // Delete old GIF object if exists
        if (ui_main_screen_gif) {
            lv_obj_del(ui_main_screen_gif);
            ui_main_screen_gif = nullptr;
        }
        
        // Load new GIF from LittleFS
        char filename[32];
        snprintf(filename, sizeof(filename), "/gifs/tool_%d.gif", tool_number);
        
        File file = LittleFS.open(filename, "r");
        if (file) {
            main_screen_gif_size = file.size();
            main_screen_gif_data = (uint8_t*)ps_malloc(main_screen_gif_size);
            if (main_screen_gif_data) {
                file.read(main_screen_gif_data, main_screen_gif_size);
                Serial.printf("[MainScreen] Loaded %s: %u bytes\n", filename, main_screen_gif_size);
                
                // Initialize descriptor
                main_screen_gif_descriptor.header.always_zero = 0;
                main_screen_gif_descriptor.header.w = 0;
                main_screen_gif_descriptor.header.h = 0;
                main_screen_gif_descriptor.data_size = main_screen_gif_size;
                main_screen_gif_descriptor.header.cf = LV_IMG_CF_RAW;
                main_screen_gif_descriptor.data = main_screen_gif_data;
                
                current_tool_gif = tool_number;
            }
            file.close();
        } else {
            Serial.printf("[MainScreen] ERROR: Could not open %s\n", filename);
        }
    }
    
    // Create GIF object if we have data
    if (main_screen_gif_data && !ui_main_screen_gif) {
        ui_main_screen_gif = lv_gif_create(ui_ScreenPrinting);
        lv_obj_clear_flag(ui_main_screen_gif, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_size(ui_main_screen_gif, 240, 240);
        lv_obj_center(ui_main_screen_gif);
        lv_gif_set_src(ui_main_screen_gif, &main_screen_gif_descriptor);
        lv_obj_move_to_index(ui_main_screen_gif, 5);  // Between background and text
        Serial.println("[MainScreen] GIF animation started!");
    }
}

// Screen Rotation wird jetzt von lvgl_usr.cpp gesteuert via ui_set_cycle_mode()

// Smooth temperature → color using HSV interpolation
// New color palette: Cold (dark blue) → Warm (orange) → Hot (red)
static lv_color_t temp_to_color(int temp_c) {
    uint16_t h; // Hue (0-359)
    uint8_t  s; // Saturation (0-255)
    uint8_t  v; // Value/Brightness (0-255)

    if (temp_c < 40) {
        // Very cold: Dark blue (almost black)
        h = 240;  // Blue
        s = 255;
        v = 80;   // Dark
    } else if (temp_c < 160) {
        // Cold → Warm: Dark blue → Light blue → Cyan → Orange
        float ratio = (float)(temp_c - 40) / 120.0f;  // 0.0 at 40°C, 1.0 at 160°C

        if (ratio < 0.5f) {
            // 40°C - 100°C: Dark blue → Light blue
            float sub_ratio = ratio * 2.0f;
            h = 240;  // Blue
            s = 255;
            v = (uint8_t)(80 + (sub_ratio * 120));  // 80 → 200 (darker → lighter)
        } else {
            // 100°C - 160°C: Light blue → Cyan → Orange
            float sub_ratio = (ratio - 0.5f) * 2.0f;
            h = (uint16_t)(240 - (sub_ratio * 210));  // 240° (Blue) → 30° (Orange)
            s = 255;
            v = 220;
        }
    } else if (temp_c < 220) {
        // Warm → Hot: Orange → Red
        float ratio = (float)(temp_c - 160) / 60.0f;  // 0.0 at 160°C, 1.0 at 220°C
        h = (uint16_t)(30 - (ratio * 30));  // 30° (Orange) → 0° (Red)
        s = 255;
        v = 240;
    } else {
        // Very hot: Intense red
        h = 0;    // Red
        s = 255;
        v = 255;  // Full brightness
    }

    return lv_color_hsv_to_rgb(h, s, v);
}

static void apply_tool_glow(int temp_c) {
    if (!ui_tool_bg_circle) return;
    lv_obj_set_style_bg_opa(ui_tool_bg_circle, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(ui_tool_bg_circle, LV_OPA_0, 0);
    
    lv_color_t c = temp_to_color(temp_c);
    
    // More subtle glow for lower load and less flickering
    int sw = 24;       // Subtle glow
    int sp = 6;        // Smaller spread
    uint8_t opa = 140; // Lower opacity
    
    lv_obj_set_style_shadow_width (ui_tool_bg_circle, sw, 0);
    lv_obj_set_style_shadow_spread(ui_tool_bg_circle, sp, 0);
    lv_obj_set_style_shadow_color (ui_tool_bg_circle, c, 0);
    lv_obj_set_style_shadow_opa   (ui_tool_bg_circle, opa, 0);
    
    // Ensure that the glow is above the arc
    lv_obj_move_to_index(ui_tool_bg_circle, 10);
}

// ========================================================================
// Print Progress Update Funktion (expanded)
// ========================================================================
// Parameters:
// - progress_percent: 0..100
// - current_layer / total_layers
// - eta_seconds
// - tool_number / tool_temp (°C)
// ========================================================================
void update_print_progress(int progress_percent, int current_layer, int total_layers, int eta_seconds, int tool_number, int tool_temp) {
    // Safety: Is printing screen active?
    if (lv_scr_act() != ui_ScreenPrinting) {
        return;
    }

    // Clamp progress
    if (progress_percent < 0)   progress_percent = 0;
    if (progress_percent > 100) progress_percent = 100;

    // CRITICAL: Check progress_enabled status FIRST!
    const bool progress_enabled = ui_get_progress_enabled();
    
    // Track previous state to detect mode changes
    static bool prev_progress_enabled = true;
    
    // If mode changed, ensure proper cleanup/reset
    if (prev_progress_enabled != progress_enabled) {
        if (!progress_enabled) {
            // Switching FROM Progress TO Temp/Main mode - clean up everything
            delete_ui_bg_ring();
            delete_ui_bg_gif();
            reset_layer_ordering();
            if (ui_arc_progress_cover) {
                lv_obj_add_flag(ui_arc_progress_cover, LV_OBJ_FLAG_HIDDEN);
            }
        }
        prev_progress_enabled = progress_enabled;
    }
    
    // If progress is disabled (Temp Graph or Main GIF mode), skip ALL background operations!
    if (!progress_enabled) {
        // CRITICAL: Keep everything hidden and deleted in non-Progress modes!
        if (ui_arc_progress_cover) {
            lv_obj_add_flag(ui_arc_progress_cover, LV_OBJ_FLAG_HIDDEN);
        }
        
        // AGGRESSIVE deletion - ensure ring and gif are REALLY gone
        if (ui_bg_ring_img) {
            Serial.println("[Progress] WARNING: Ring still exists in non-progress mode! Force deleting...");
            delete_ui_bg_ring();
        }
        if (ui_bg_gif) {
            Serial.println("[Progress] WARNING: GIF still exists in non-progress mode! Force deleting...");
            delete_ui_bg_gif();
        }
        
        // Still update temperature history for when we switch back to Temp Graph
        // OPTIMIZATION: Only update temperature once per second for proper 60-second history
        uint32_t current_ms = millis();
        if (current_ms - last_temp_update_ms >= TEMP_UPDATE_INTERVAL_MS) {
            add_temp_to_history(tool_temp);
            last_temp_update_ms = current_ms;
        }
        
        // Update chart data (color is fixed in ui_ScreenPrinting.c)
        if (ui_temp_chart) {
            static bool chart_styled = false;
            if (!chart_styled) {
                lv_obj_set_style_bg_opa(ui_temp_chart, LV_OPA_TRANSP, 0);
                lv_obj_set_style_border_opa(ui_temp_chart, LV_OPA_TRANSP, 0);
                chart_styled = true;
            }
            
            lv_chart_series_t * ser = lv_chart_get_series_next(ui_temp_chart, NULL);
            if (ser && !lv_obj_has_flag(ui_temp_chart, LV_OBJ_FLAG_HIDDEN)) {
                // Update data points
                int start_idx = temp_history_filled ? temp_history_index : 0;
                int count = temp_history_filled ? TEMP_HISTORY_SIZE : temp_history_index;
                
                for (int i = 0; i < count; i++) {
                    int idx = (start_idx + i) % TEMP_HISTORY_SIZE;
                    ser->y_points[i] = temp_history[idx];
                }
                
                lv_chart_refresh(ui_temp_chart);
            }
        }
        
        if (ui_label_temp_current) {
            lv_label_set_text_fmt(ui_label_temp_current, "Current: %d°C", tool_temp);
        }
        
        // Early return - don't update Progress elements!
        return;
    }

    // === PROGRESS MODE ACTIVE - Continue with normal updates ===

    // Ensure background PNG ring and animated GIF are present & ordered
    if (progress_enabled) {
        // Only create/order BG when the Progress view is active
        ensure_bg_layers_created();
        if (ui_bg_gif)      lv_obj_clear_flag(ui_bg_gif, LV_OBJ_FLAG_HIDDEN);
        if (ui_bg_ring_img) lv_obj_clear_flag(ui_bg_ring_img, LV_OBJ_FLAG_HIDDEN);
        if (ui_arc_progress_cover) lv_obj_clear_flag(ui_arc_progress_cover, LV_OBJ_FLAG_HIDDEN);
    } else {
        // In non-Progress views keep these fully hidden
        if (ui_arc_progress_cover) lv_obj_add_flag(ui_arc_progress_cover, LV_OBJ_FLAG_HIDDEN);
        if (ui_bg_ring_img)        lv_obj_add_flag(ui_bg_ring_img, LV_OBJ_FLAG_HIDDEN);
        if (ui_bg_gif)             lv_obj_add_flag(ui_bg_gif, LV_OBJ_FLAG_HIDDEN);
    }

    // Oben: Temperatur (konstant)
    if (ui_label_tool_indicator) {
        char buf[12];
        lv_snprintf(buf, sizeof(buf), "%d°C", tool_temp);
        lv_label_set_text(ui_label_tool_indicator, buf);
    }

    // 2) Cover-Arc (clockwise from 6 o'clock) - ONLY UPDATE ANGLES!
    // The black arc covers the NOT YET DONE part
    // At 0%: Arc from 450° to 810° (full circle = nothing done)
    // At 50%: Arc from 630° to 810° (half arc = half done)
    // At 100%: Arc from 810° to 810° (no arc = all done)
    if (progress_enabled && ui_arc_progress_cover) {
        static int last_pct = -1;
        if (progress_percent != last_pct) {
            last_pct = progress_percent;
            const int32_t base  = 270 + 180;                                      // 6 o'clock anchor (450°)
            const int32_t start = base + (progress_percent * 360) / 100;          // Moves forward
            const int32_t end   = base + 360;                                     // Stays fixed at 810° (6 o'clock)
            
            // CRITICAL: ONLY set angles - NO styling, NO z-order changes!
            lv_arc_set_angles(ui_arc_progress_cover, start, end);
        }
    } else if (!ui_arc_progress_cover) {
        Serial.println("[Progress] ERROR: ui_arc_progress_cover is NULL!");
    }

    // 3) ETA Label verstecken (wird jetzt in der Mitte im Toggle angezeigt)
    if (ui_label_progress_eta) {
        lv_obj_add_flag(ui_label_progress_eta, LV_OBJ_FLAG_HIDDEN);
    }

    // 4) Layer info
    if (ui_label_progress_layer) {
        if (total_layers > 0) lv_label_set_text_fmt(ui_label_progress_layer, "Layer %d/%d", current_layer, total_layers);
        else                  lv_label_set_text_fmt(ui_label_progress_layer, "Layer %d", current_layer);
    }

    // 5) Center Toggle (Progress <-> Restzeit) & Glow
    g_tool_temp = tool_temp;
    g_eta_seconds = eta_seconds;
    ensure_toggle_timer();
    
    // Mitte: Progress % / Restzeit Toggle
    if (progress_enabled) {
        if (ui_label_printing_progress) {
            if (show_progress_now) {
                // Zeige Progress %
                lv_label_set_text_fmt(ui_label_printing_progress, "%d%%", progress_percent);
            } else {
                // Zeige Restzeit
                if (eta_seconds == -1) {
                    // Special case: Calculating (not enough data yet)
                    lv_label_set_text(ui_label_printing_progress, "---");
                } else if (eta_seconds <= 0) {
                    lv_label_set_text(ui_label_printing_progress, "Done!");
                } else {
                    int hours = eta_seconds / 3600;
                    int minutes = (eta_seconds % 3600) / 60;
                    if (hours > 0) {
                        lv_label_set_text_fmt(ui_label_printing_progress, "%dh %dm", hours, minutes);
                    } else {
                        lv_label_set_text_fmt(ui_label_printing_progress, "%dm", minutes);
                    }
                }
            }
            // Shadow for better readability
            lv_obj_set_style_shadow_width(ui_label_printing_progress, 10, 0);
            lv_obj_set_style_shadow_color(ui_label_printing_progress, lv_color_black(), 0);
            lv_obj_set_style_shadow_opa(ui_label_printing_progress, LV_OPA_80, 0);
        }
    }

    if (progress_enabled) apply_tool_glow(tool_temp);
    
    // ====================================================================
    // Update Temp Graph Data (for screen rotation in lvgl_usr.cpp)
    // ====================================================================
    
    // Add temperature to history for graph
    // OPTIMIZATION: Only update temperature once per second for proper 60-second history
    uint32_t current_ms = millis();
    if (current_ms - last_temp_update_ms >= TEMP_UPDATE_INTERVAL_MS) {
        add_temp_to_history(tool_temp);
        last_temp_update_ms = current_ms;
    }
    
    // Update chart data (color is fixed in ui_ScreenPrinting.c as Neon Green)
    if (ui_temp_chart) {
        static bool chart_styled = false;
        if (!chart_styled) {
            lv_obj_set_style_bg_opa(ui_temp_chart, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_opa(ui_temp_chart, LV_OPA_TRANSP, 0);
            chart_styled = true;
        }
        
        lv_chart_series_t * ser = lv_chart_get_series_next(ui_temp_chart, NULL);
        if (ser && !lv_obj_has_flag(ui_temp_chart, LV_OBJ_FLAG_HIDDEN)) {
            // Update data points
            int start_idx = temp_history_filled ? temp_history_index : 0;
            int count = temp_history_filled ? TEMP_HISTORY_SIZE : temp_history_index;
            
            for (int i = 0; i < count; i++) {
                int idx = (start_idx + i) % TEMP_HISTORY_SIZE;
                ser->y_points[i] = temp_history[idx];
            }
            
            lv_chart_refresh(ui_temp_chart);
        }
    }
    
    if (ui_label_temp_current) {
        lv_label_set_text_fmt(ui_label_temp_current, "Current: %d°C", tool_temp);
    }
    
    // DEACTIVATED: Main Screen GIF is now managed by lvgl_usr.cpp!
    // ensure_main_screen_gif_created(tool_number);
    
    // Screen Rotation wird von lvgl_usr.cpp gesteuert via ui_set_cycle_mode()
}
