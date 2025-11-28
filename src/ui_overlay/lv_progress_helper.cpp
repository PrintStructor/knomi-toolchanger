#include "lvgl.h"

// External UI objects
extern lv_obj_t * ui_temp_chart;

// Helper function to check if progress mode is enabled
// Progress mode is enabled when the temp chart is NOT visible
bool ui_get_progress_enabled(void) {
    // If temp_chart doesn't exist, we're in progress mode
    if (!ui_temp_chart) {
        return true;
    }
    
    // If temp_chart is hidden, we're in progress mode
    if (lv_obj_has_flag(ui_temp_chart, LV_OBJ_FLAG_HIDDEN)) {
        return true;
    }
    
    // Otherwise, we're in Temp Graph mode
    return false;
}
