#include "ui/ui.h"
#include "knomi.h"
#include "moonraker.h"
#include "ui_overlay/lv_overlay.h"
#include "fs_gif_loader.h"  // Include FS-Loader
#include "power_management/display_sleep.h"  // Display Sleep Management

// forward declaration from lv_print_progress_update.cpp
void update_print_progress(int progress_percent,
                           int current_layer,
                           int total_layers,
                           int eta_seconds,
                           int tool_number,
                           int tool_temp);

// Fallback if Stock-BTT does not define this constant:
#ifndef TEMPERATURE_ERROR_RANGE
#define TEMPERATURE_ERROR_RANGE 3
#endif

static int to_percent(float p) {
    // accept 0..1 or 0..100; clamp to [0,100]
    float v = p;
    if (v <= 1.01f) v *= 100.0f;
    if (v < 0.0f) v = 0.0f;
    if (v > 100.0f) v = 100.0f;
    return (int)(v + 0.5f);
}

typedef enum {
    LV_MOONRAKER_STATE_IDLE = 0,
    LV_MOONRAKER_STATE_HOMING,
    LV_MOONRAKER_STATE_PROBING,
    LV_MOONRAKER_STATE_QGLING,
    LV_MOONRAKER_STATE_NOZZLE_HEATING,
    LV_MOONRAKER_STATE_BED_HEATING,
    LV_MOONRAKER_STATE_PRINTING,
    LV_SCREEN_STATE_INIT,
    LV_SCREEN_STATE_IDLE,
    LV_SCREEN_PRINT_OK,
    LV_SCREEN_PRINTED,
    LV_SCREEN_STATE_PLAYING,
} lv_screen_state_t;

static char string_buffer[8];
static lv_screen_state_t lv_screen_state = LV_MOONRAKER_STATE_IDLE;
static lv_obj_t * ui_ScreenIdle = NULL;
static lv_obj_t * ui_ScreenNow = NULL;

// ========================================================================
// Print Complete Delay System
// ========================================================================
static bool print_complete_shown = false;  // Has Complete Screen been shown?
static bool print_complete_timer_active = false;  // Is timer active?
static uint32_t print_complete_time = 0;
#define PRINT_COMPLETE_DISPLAY_TIME 10000  // 10 seconds in milliseconds

// ========================================================================
// IMPORTANT: ui_img_main_gif is defined externally (in lvgl_usr.cpp)
// Only declare here, do NOT create new instance!
// ========================================================================
extern lv_obj_t * ui_img_main_gif;  // Important: extern!

// ========================================================================
// External GIF declarations (defined in gif/*.c)
// ========================================================================
extern const lv_img_dsc_t gif_homing;
extern const lv_img_dsc_t gif_probing;
extern const lv_img_dsc_t gif_qgling;
extern const lv_img_dsc_t gif_print_ok;  // Print Complete GIF (checkmark)

// ========================================================================
// Heating Screens (defined in ui/screens/)
// ========================================================================
extern lv_obj_t * ui_ScreenHeatingBed;
extern lv_obj_t * ui_ScreenHeatingNozzle;
extern lv_obj_t * ui_slider_heating_bed;
extern lv_obj_t * ui_slider_heating_nozzle;
extern lv_obj_t * ui_label_heating_bed_target;
extern lv_obj_t * ui_label_heating_bed_actual;
extern lv_obj_t * ui_label_heating_nozzle_target;
extern lv_obj_t * ui_label_heating_nozzle_actual;

static void lv_goto_busy_screen(lv_obj_t * screen, lv_screen_state_t state, const lv_img_dsc_t * gif) {
    if (lv_screen_state == state) return;
    
    // Safety check for ui_img_main_gif
    if (!ui_img_main_gif) {
        Serial.println("ERROR: ui_img_main_gif is NULL in lv_goto_busy_screen!");
        return;
    }
    
    lv_screen_state = state;

    // CRITICAL: Always ensure screen background is black
    if (screen) {
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    }

    // If a C-Array-GIF is provided (old states like Homing)
    if (gif) {
        lv_gif_set_src(ui_img_main_gif, gif);
    }
    lv_obj_clear_flag(ui_img_main_gif, LV_OBJ_FLAG_CLICKABLE);

    // backup now screen, cause lv_scr_act() is delayed updates
    if (ui_ScreenNow == NULL) ui_ScreenNow = lv_scr_act();
    if (screen && screen != ui_ScreenNow) {
        // backup screen before jump
        if (ui_ScreenIdle == NULL) ui_ScreenIdle = ui_ScreenNow;
        ui_ScreenNow = screen;
        _ui_screen_change(&screen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 500, 0, NULL);
    }
}

static void lv_goto_idle_screen(void) {
    if (lv_screen_state == LV_MOONRAKER_STATE_IDLE) return;
    
    // Safety check for ui_img_main_gif
    if (!ui_img_main_gif) {
        Serial.println("ERROR: ui_img_main_gif is NULL in lv_goto_idle_screen!");
        return;
    }
    
    lv_screen_state = LV_MOONRAKER_STATE_IDLE;

    // ========================================================================
    // CRITICAL: Reset Recolor Filter from previous states (e.g., Print Complete)
    // ========================================================================
    if (ui_img_main_gif) {
        lv_obj_set_style_img_recolor_opa(ui_img_main_gif, LV_OPA_TRANSP, 0);
        Serial.println("[IDLE] Recolor filter reset to transparent");
    }

    // ========================================================================
    // FS-GIF-Integration: Detect tool number & load GIF from LittleFS
    // ========================================================================
    fs_set_tool_gif(ui_img_main_gif, detect_my_tool_number());
    
    lv_obj_add_flag(ui_img_main_gif, LV_OBJ_FLAG_CLICKABLE);

    // goto the screen backed up before
    if (ui_ScreenIdle) {
        // CRITICAL: Ensure idle screen background is black
        lv_obj_set_style_bg_color(ui_ScreenIdle, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(ui_ScreenIdle, LV_OPA_COVER, 0);
        
        if (lv_scr_act() != ui_ScreenIdle && ui_ScreenIdle != ui_ScreenNow) {
            // backup screen before jump
            ui_ScreenNow = ui_ScreenIdle;
            _ui_screen_change(&ui_ScreenIdle, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, NULL);
        }
        ui_ScreenIdle = NULL;
    }
}

static bool moonraker_nozzle_is_heating(void) {
    if (moonraker.data.heating_nozzle)
        return true;
    if (moonraker.data.nozzle_actual + TEMPERATURE_ERROR_RANGE < moonraker.data.nozzle_target)
        return true;
    if ((moonraker.data.nozzle_target != 0) && (moonraker.data.nozzle_target + TEMPERATURE_ERROR_RANGE < moonraker.data.nozzle_actual))
        return true;
    return false;
}

static bool moonraker_bed_is_heating(void) {
    if (moonraker.data.heating_bed)
        return true;
    if (moonraker.data.bed_actual + TEMPERATURE_ERROR_RANGE < moonraker.data.bed_target)
        return true;
    if ((moonraker.data.bed_target != 0) && (moonraker.data.bed_target + TEMPERATURE_ERROR_RANGE < moonraker.data.bed_actual))
        return true;
    return false;
}

// screen change according to moonraker status
// IMPORTANT: Check specific states BEFORE printing state to show preparation GIFs
void lv_loop_moonraker_change_screen(void) {
    // ========================================================================
    // Display Sleep: Wake on status changes
    // ========================================================================
    static bool last_homing = false;
    static bool last_probing = false;
    static bool last_qgling = false;
    static bool last_heating_nozzle = false;
    static bool last_heating_bed = false;
    static bool last_printing = false;
    
    bool status_changed = (moonraker.data.homing != last_homing) ||
                          (moonraker.data.probing != last_probing) ||
                          (moonraker.data.qgling != last_qgling) ||
                          (moonraker.data.heating_nozzle != last_heating_nozzle) ||
                          (moonraker.data.heating_bed != last_heating_bed) ||
                          (moonraker.data.printing != last_printing);
    
    if (status_changed) {
        display_check_wake_condition(true);
        last_homing = moonraker.data.homing;
        last_probing = moonraker.data.probing;
        last_qgling = moonraker.data.qgling;
        last_heating_nozzle = moonraker.data.heating_nozzle;
        last_heating_bed = moonraker.data.heating_bed;
        last_printing = moonraker.data.printing;
    }
    
    // ========================================================================
    // PRIORITY 1: Specific Actions (Homing, Probing, QGL, Heating)
    // These must be checked BEFORE printing state to show preparation GIFs
    // ========================================================================
    
    // Homing State
    if (moonraker.data.homing) {
        ui_auto_cycle_stop();
        ui_set_temp_view(false);
        lv_goto_busy_screen(ui_ScreenMainGif, LV_MOONRAKER_STATE_HOMING, &gif_homing);
        
        // Apply theme color recolor filter to GIF
        if (ui_img_main_gif) {
            lv_obj_set_style_img_recolor(ui_img_main_gif, lv_theme_color(), 0);
            lv_obj_set_style_img_recolor_opa(ui_img_main_gif, LV_OPA_70, 0);
        }
        return;
    }
    
    // Probing State
    if (moonraker.data.probing) {
        ui_auto_cycle_stop();
        ui_set_temp_view(false);
        lv_goto_busy_screen(ui_ScreenMainGif, LV_MOONRAKER_STATE_PROBING, &gif_probing);
        
        // Apply theme color recolor filter to GIF
        if (ui_img_main_gif) {
            lv_obj_set_style_img_recolor(ui_img_main_gif, lv_theme_color(), 0);
            lv_obj_set_style_img_recolor_opa(ui_img_main_gif, LV_OPA_70, 0);
        }
        return;
    }
    
    // QGL State
    if (moonraker.data.qgling) {
        static bool qgl_logged = false;
        if (!qgl_logged) {
            Serial.println("[State] QGL active - showing animation");
            qgl_logged = true;
        }
        
        ui_auto_cycle_stop();
        ui_set_temp_view(false);
        lv_goto_busy_screen(ui_ScreenMainGif, LV_MOONRAKER_STATE_QGLING, &gif_qgling);
        
        // Apply theme color recolor filter to GIF
        if (ui_img_main_gif) {
            lv_obj_set_style_img_recolor(ui_img_main_gif, lv_theme_color(), 0);
            lv_obj_set_style_img_recolor_opa(ui_img_main_gif, LV_OPA_70, 0);
        }
        return;
    } else {
        static bool qgl_logged = false;
        qgl_logged = false;  // Reset for next QGL
    }
    
    // Nozzle Heating State
    if (moonraker_nozzle_is_heating()) {
        ui_auto_cycle_stop();
        ui_set_temp_view(false);
        lv_goto_busy_screen(ui_ScreenHeatingNozzle, LV_MOONRAKER_STATE_NOZZLE_HEATING, NULL);
        
        // Update slider and labels
        if (ui_slider_heating_nozzle && ui_label_heating_nozzle_target && ui_label_heating_nozzle_actual) {
            int slider_val = 0;
            if (moonraker.data.nozzle_target > 0) {
                slider_val = (moonraker.data.nozzle_actual * 100) / moonraker.data.nozzle_target;
                if (slider_val > 100) slider_val = 100;
            }
            lv_slider_set_value(ui_slider_heating_nozzle, slider_val, LV_ANIM_ON);
            lv_label_set_text_fmt(ui_label_heating_nozzle_target, "%d℃", moonraker.data.nozzle_target);
            lv_label_set_text_fmt(ui_label_heating_nozzle_actual, "%d℃", moonraker.data.nozzle_actual);
            
            // Apply theme color to slider indicator image
            lv_obj_set_style_img_recolor(ui_slider_heating_nozzle, lv_theme_color(), LV_PART_INDICATOR);
            lv_obj_set_style_img_recolor_opa(ui_slider_heating_nozzle, LV_OPA_70, LV_PART_INDICATOR);
        }
        return;
    }
    
    // Bed Heating State
    if (moonraker_bed_is_heating()) {
        ui_auto_cycle_stop();
        ui_set_temp_view(false);
        lv_goto_busy_screen(ui_ScreenHeatingBed, LV_MOONRAKER_STATE_BED_HEATING, NULL);
        
        // Update slider and labels
        if (ui_slider_heating_bed && ui_label_heating_bed_target && ui_label_heating_bed_actual) {
            int slider_val = 0;
            if (moonraker.data.bed_target > 0) {
                slider_val = (moonraker.data.bed_actual * 100) / moonraker.data.bed_target;
                if (slider_val > 100) slider_val = 100;
            }
            lv_slider_set_value(ui_slider_heating_bed, slider_val, LV_ANIM_ON);
            lv_label_set_text_fmt(ui_label_heating_bed_target, "%d℃", moonraker.data.bed_target);
            lv_label_set_text_fmt(ui_label_heating_bed_actual, "%d℃", moonraker.data.bed_actual);
            
            // Apply theme color to slider indicator image
            lv_obj_set_style_img_recolor(ui_slider_heating_bed, lv_theme_color(), LV_PART_INDICATOR);
            lv_obj_set_style_img_recolor_opa(ui_slider_heating_bed, LV_OPA_70, LV_PART_INDICATOR);
        }
        return;
    }
    
    // ========================================================================
    // PRIORITY 2: PRINTING STATE
    // Only check this AFTER all preparation states
    // ========================================================================
    // print_stats state is authoritative and already includes paused,
    // pausing, and cancelling states through moonraker.data.printing.
    bool is_printing = moonraker.data.printing;

    if (is_printing) {
        // Reset Print Complete state when new print starts
        print_complete_shown = false;
        print_complete_timer_active = false;
        
        if (lv_screen_state != LV_MOONRAKER_STATE_PRINTING) {
            // Ensure printing screen becomes active
            lv_goto_busy_screen(ui_ScreenPrinting, LV_MOONRAKER_STATE_PRINTING, NULL);
            // Reset overlays and start auto cycle: Progress (10s) → Temp (8s) → Main GIF (8s)
            ui_set_temp_view(false);
            ui_auto_cycle_start();
        }
        return;
    }
    
    // ========================================================================
    // PRIORITY 2.1: PRINT CANCELLED/FAILED STATE
    // If we were printing but stopped without completion, go back to IDLE
    // ========================================================================
    if (!is_printing && lv_screen_state == LV_MOONRAKER_STATE_PRINTING) {
        int progress_percent = to_percent(moonraker.data.progress);
        
        // Check if print was cancelled/failed (not completed)
        // Use 95% threshold as many slicers don't report exactly 100%
        if (progress_percent < 95) {
            Serial.printf("[Print Cancelled/Failed] Progress at %d%% - returning to IDLE\n", progress_percent);
            
            // Reset all print-related states
            print_complete_shown = false;
            print_complete_timer_active = false;
            
            // Immediately go back to Idle Screen
            ui_auto_cycle_stop();
            ui_set_temp_view(false);
            lv_goto_idle_screen();
            return;
        }
        // If progress >= 95, fall through to Print Complete state below
    }
    
    // ========================================================================
    // PRIORITY 2.5: PRINT COMPLETE STATE
    // Show completion screen for 10 seconds after print finishes
    // ========================================================================
    int progress_percent = to_percent(moonraker.data.progress);
    
    // Show Print Complete screen only ONCE per print and for limited time
    // Use 95% threshold as many slicers don't report exactly 100%
    if (!is_printing && progress_percent >= 95 && !print_complete_shown) {
        if (!print_complete_timer_active) {
            // First time showing completion screen
            print_complete_timer_active = true;
            print_complete_time = millis();
            
            ui_auto_cycle_stop();
            ui_set_temp_view(false);
            
            // CRITICAL: Delete Progress Ring before showing Print Complete screen
            delete_ui_bg_ring();
            
            lv_goto_busy_screen(ui_ScreenMainGif, LV_SCREEN_PRINT_OK, &gif_print_ok);
            
            // CRITICAL: Reset Recolor Filter for clean green checkmark (no colored fog)
            if (ui_img_main_gif) {
                lv_obj_set_style_img_recolor_opa(ui_img_main_gif, LV_OPA_TRANSP, 0);
                Serial.println("[Print Complete] Recolor filter reset for clean display");
            }
            
            Serial.println("[Print Complete] Showing completion screen for 10 seconds");
        }
        
        // CRITICAL: Force black background on screen itself
        if (ui_ScreenMainGif && lv_scr_act() == ui_ScreenMainGif) {
            lv_obj_set_style_bg_color(ui_ScreenMainGif, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ScreenMainGif, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        
        // Force GIF container to have black background
        if (ui_img_main_gif) {
            lv_obj_set_style_bg_color(ui_img_main_gif, lv_color_hex(0x000000), 0);
            lv_obj_set_style_bg_opa(ui_img_main_gif, LV_OPA_COVER, 0);
        }
        
        // Check if timer still active
        uint32_t elapsed = millis() - print_complete_time;
        if (elapsed < PRINT_COMPLETE_DISPLAY_TIME) {
            return;  // Stay on completion screen
        }
        
        // Timer expired - mark as shown and allow transition to idle
        Serial.println("[Print Complete] Timer expired, transitioning to idle");
        print_complete_shown = true;
        print_complete_timer_active = false;
        // Now fall through to IDLE state
    }
    
    // ========================================================================
    // PRIORITY 3: IDLE STATE (Default)
    // ========================================================================
    ui_auto_cycle_stop();
    ui_set_temp_view(false);
    lv_goto_idle_screen();
}

// Export function for lvgl_usr.cpp with Print Progress Updates
void lv_loop_moonraker_change_screen_value(void) {
    // Screen State Management
    lv_loop_moonraker_change_screen();

    // ========================================================================
    // PRINT PROGRESS UPDATE (only when Printing Screen is active)
    // ========================================================================
    if (lv_scr_act() == ui_ScreenPrinting) {
        // Moonraker supplies virtual-SD progress as a 0.0-1.0 ratio.
        // Convert it to a whole percentage only for visual display.
        const float progress_ratio = moonraker.data.progress;
        const int progress_percent = to_percent(progress_ratio);

        // Layer information comes directly from print_stats.
        const int current_layer = moonraker.data.current_layer;
        const int total_layers = moonraker.data.total_layers;

        // ====================================================================
        // Smoothed file-progress ETA
        //
        // Estimated total print duration:
        //
        //     estimated total = elapsed printing time / fractional progress
        //
        // The total-time estimate is updated only when G-code file progress
        // advances. This prevents toolchanges, wipes, purge macros, and other
        // long commands from making the ETA increase on every Moonraker poll.
        //
        // Remaining time continues counting downward between progress samples.
        // ====================================================================
        static float smoothed_total_time = 0.0f;
        static float last_sample_progress = 0.0f;
        static int32_t previous_print_duration = 0;

        const int32_t print_duration = moonraker.data.print_duration;
        int32_t eta_seconds = 0;

        // A lower duration or substantial backward progress indicates that
        // Klipper started a new print or reset its print statistics.
        const bool estimate_reset =
            print_duration < previous_print_duration ||
            progress_ratio + 0.01f < last_sample_progress;

        if (estimate_reset) {
            smoothed_total_time = 0.0f;
            last_sample_progress = 0.0f;
        }

        previous_print_duration = print_duration;

        if (progress_ratio >= 1.0f) {
            // Print is complete.
            eta_seconds = 0;

        } else if (progress_ratio < 0.02f || print_duration <= 0) {
            // Startup heating, purging, homing, and other preparation work make
            // a live elapsed/progress estimate unreliable during the first 2%.
            eta_seconds = -1;

        } else {
            // Resample after progress advances by 0.05 percentage points.
            // During stationary file progress, preserve the estimated total
            // while remaining time continues counting down.
            const float minimum_progress_step = 0.0005f;

            const bool progress_advanced =
                progress_ratio >= last_sample_progress + minimum_progress_step;

            if (smoothed_total_time <= 0.0f || progress_advanced) {
                const float instantaneous_total_time =
                    (float)print_duration / progress_ratio;

                if (smoothed_total_time <= 0.0f) {
                    // Seed the filter using the first valid progress sample.
                    smoothed_total_time = instantaneous_total_time;

                } else {
                    // Stronger smoothing is used early because small progress
                    // changes produce large swings in projected total time.
                    float smoothing_alpha = 0.08f;

                    if (progress_ratio >= 0.50f) {
                        smoothing_alpha = 0.25f;
                    } else if (progress_ratio >= 0.10f) {
                        smoothing_alpha = 0.15f;
                    }

                    smoothed_total_time +=
                        smoothing_alpha *
                        (instantaneous_total_time - smoothed_total_time);
                }

                last_sample_progress = progress_ratio;
            }

            eta_seconds =
                (int32_t)(smoothed_total_time - (float)print_duration);

            if (eta_seconds < 0) {
                eta_seconds = 0;
            }
        }

        // Periodic diagnostics for comparing KNOMI with Mainsail/KlipperScreen.
        static int debug_counter = 0;

        if (debug_counter++ % 10 == 0) {
            Serial.printf(
                "[ETA DEBUG] duration=%d progress=%.4f (%d%%) total=%.0f eta=%d\n",
                (int)print_duration,
                progress_ratio,
                progress_percent,
                smoothed_total_time,
                (int)eta_seconds
            );

            if (eta_seconds >= 0) {
                const int hours = eta_seconds / 3600;
                const int minutes = (eta_seconds % 3600) / 60;

                Serial.printf(
                    "[ETA DEBUG] Display will show: %dh %dm\n",
                    hours,
                    minutes
                );
            } else {
                Serial.println(
                    "[ETA DEBUG] Display will show: calculating"
                );
            }
        }

        // Determine which physical tool this KNOMI display belongs to.
        const int my_tool_number = detect_my_tool_number();

        // Show the temperature belonging to this display's physical tool,
        // rather than always showing the currently active extruder.
        int16_t my_tool_temp = 0;

        if (my_tool_number >= 0 && my_tool_number < 6) {
            my_tool_temp =
                moonraker.data.extruder_temps[my_tool_number];
        }

        static int temp_debug_counter = 0;

        if (temp_debug_counter++ % 30 == 0) {
            Serial.printf(
                "[Tool Temp] Display Tool #%d: %d°C\n",
                my_tool_number,
                my_tool_temp
            );
        }

        // Push all calculated values into the active progress-screen widgets.
        update_print_progress(
            progress_percent,
            current_layer,
            total_layers,
            eta_seconds,
            my_tool_number,
            my_tool_temp
        );
    }
}