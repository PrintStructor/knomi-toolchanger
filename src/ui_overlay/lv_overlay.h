#ifndef _LV_OVERLAY_H
#define _LV_OVERLAY_H

#include "lvgl.h"
#include "knomi.h"  // ← For wifi_status_t
#include <stdint.h>
#include <stdbool.h>

extern lv_obj_t * ui_img_main_gif;
extern const lv_img_dsc_t * gif_idle[];
extern lv_obj_t * ui_temp_bg_gif; // animated BG for Temp Graph (loaded via lv_gif_set_src from LittleFS)

#ifdef __cplusplus
extern "C" {
#endif

// Toggle Temp-Graph visibility on the Printing screen
void ui_set_temp_view(bool on);
void ui_progress_enable(bool on);
bool ui_get_progress_enabled(void);  // Get current progress updates state

// Auto-cycle API (0: Progress, 1: Temp Graph, 2: Main GIF)
void ui_set_cycle_mode(uint8_t mode);
void ui_auto_cycle_start(void);
void ui_auto_cycle_stop(void);

// Query currently active tool index from Moonraker
int  knomi_active_tool_index(void);

#ifdef __cplusplus
}
#endif

LV_IMG_DECLARE(gif_welcome);
LV_IMG_DECLARE(gif_voron);
LV_IMG_DECLARE(gif_standby);
LV_IMG_DECLARE(gif_homing);
LV_IMG_DECLARE(gif_probing);
LV_IMG_DECLARE(gif_qgling);
LV_IMG_DECLARE(gif_heated);
LV_IMG_DECLARE(gif_print);
LV_IMG_DECLARE(gif_print_ok);
LV_IMG_DECLARE(gif_printed);

// lv_button_style.cpp
void lv_btn_add_style(void);
// lv_theme_color.cpp
lv_color_t lv_theme_color(void);
void lv_theme_update_color(lv_color_t c);
void lv_theme_color_style(void);
//lv_wifi_change_screen.cpp
void lv_loop_wifi_change_screen(wifi_status_t status);
// lv_moonraker_change_screen.cpp
void lv_loop_moonraker_change_screen(void);
void lv_loop_moonraker_change_screen_value(void);
// lv_set_temp.cpp
void lv_loop_set_temp_screen(void);
// lv_popup.cpp
void lv_loop_popup_screen(void);
void lv_popup_warning(const char * warning, bool clickable);
// lv_dialog.cpp
void lv_dialog_goto_print(void);
void lv_dialog_goto_reset_wifi(void);
// lv_btn_event.cpp
void lv_btn_init(void);
void lv_loop_btn_event(void);
// lv_auto_goto_idle.cpp
void touch_idle_time_clear(void);
void lv_loop_auto_idle(wifi_status_t status);
// lv_print_progress_update.cpp - NEU!
void update_print_progress(int progress_percent, int current_layer, int total_layers, int eta_seconds, int tool_number, int tool_temp);
void delete_ui_bg_ring(void);
void delete_ui_bg_gif(void);  // Hide/pause GIF (doesn't actually delete for performance)
void show_ui_bg_gif(void);    // Show/resume GIF
void preload_progress_gif_data(void);  // Preload GIF at startup for faster display

#endif
