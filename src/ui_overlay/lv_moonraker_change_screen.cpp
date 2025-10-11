#include "ui/ui.h"
#include "knomi.h"
#include "moonraker.h"
#include "lv_overlay.h"
#include "fs_gif_loader.h"  // ← FS-Loader einbinden

// Fallback, falls Stock-BTT diese Konstante nicht definiert:
#ifndef TEMPERATURE_ERROR_RANGE
#define TEMPERATURE_ERROR_RANGE 3
#endif

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
// WICHTIG: ui_img_main_gif wird extern definiert (in lvgl_usr.cpp)
// Hier nur deklarieren, NICHT neu erstellen!
// ========================================================================
extern lv_obj_t * ui_img_main_gif;  // ← Wichtig: extern!

static void lv_goto_busy_screen(lv_obj_t * screen, lv_screen_state_t state, const lv_img_dsc_t * gif) {
    if (lv_screen_state == state) return;
    lv_screen_state = state;

    // Wenn ein C-Array-GIF übergeben wird (alte States wie Homing)
    if (gif) {
        lv_gif_set_src(ui_img_main_gif, gif);
    }
    lv_obj_clear_flag(ui_ScreenMainGif, LV_OBJ_FLAG_CLICKABLE);

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
    lv_screen_state = LV_MOONRAKER_STATE_IDLE;

    // ========================================================================
    // FS-GIF-Integration: Toolnummer erkennen & GIF aus LittleFS laden
    // ========================================================================
    fs_set_tool_gif(ui_img_main_gif, detect_my_tool_number());
    
    lv_obj_add_flag(ui_ScreenMainGif, LV_OBJ_FLAG_CLICKABLE);

    // goto the screen backed up before
    if (ui_ScreenIdle) {
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
void lv_loop_moonraker_change_screen(void) {

    static lv_screen_state_t screen_state = LV_SCREEN_STATE_INIT;
    
    // Printing State (optional - wenn ihr einen Printing-Screen habt)
    // if (moonraker.data.printing) {
    //     if (lv_screen_state != LV_MOONRAKER_STATE_PRINTING) {
    //         lv_goto_busy_screen(ui_ScreenPrinting, LV_MOONRAKER_STATE_PRINTING, NULL);
    //         return;
    //     }
    // }
    
    // Homing State
    if (moonraker.data.homing) {
        // WICHTIG: Hier muss gif_homing eingebunden sein!
        // LV_IMG_DECLARE(gif_homing); sollte in knomi.h oder hier oben stehen
        extern const lv_img_dsc_t gif_homing;  // Falls nicht in Header
        lv_goto_busy_screen(ui_ScreenMainGif, LV_MOONRAKER_STATE_HOMING, &gif_homing);
        return;
    }
    
    // Probing State
    if (moonraker.data.probing) {
        extern const lv_img_dsc_t gif_probing;
        lv_goto_busy_screen(ui_ScreenMainGif, LV_MOONRAKER_STATE_PROBING, &gif_probing);
        return;
    }
    
    // QGL State
    if (moonraker.data.qgling) {
        extern const lv_img_dsc_t gif_qgl;
        lv_goto_busy_screen(ui_ScreenMainGif, LV_MOONRAKER_STATE_QGLING, &gif_qgl);
        return;
    }
    
    // Nozzle Heating State
    if (moonraker_nozzle_is_heating()) {
        extern const lv_img_dsc_t gif_heating_nozzle;
        lv_goto_busy_screen(ui_ScreenMainGif, LV_MOONRAKER_STATE_NOZZLE_HEATING, &gif_heating_nozzle);
        return;
    }
    
    // Bed Heating State
    if (moonraker_bed_is_heating()) {
        extern const lv_img_dsc_t gif_heating_bed;
        lv_goto_busy_screen(ui_ScreenMainGif, LV_MOONRAKER_STATE_BED_HEATING, &gif_heating_bed);
        return;
    }
    
    // Idle State - Lädt Tool-GIF aus LittleFS
    lv_goto_idle_screen();
}
