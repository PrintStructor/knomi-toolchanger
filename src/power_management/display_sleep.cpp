#include "display_sleep.h"
#include "../lvgl_hal.h"
#include "../pinout.h"
#include "../moonraker.h"
#include "lvgl.h"
#include <TFT_eSPI.h>

// External TFT instance from lvgl_hal.cpp
extern TFT_eSPI tft_gc9a01;
extern void tft_set_backlight(int8_t level);
extern MOONRAKER moonraker;

// ========================================================================
// State Management
// ========================================================================
static display_sleep_state_t g_display_state = DISPLAY_STATE_ACTIVE;
static display_sleep_mode_t g_sleep_mode = SLEEP_MODE_KLIPPER_SYNC;  // Default: Klipper Sync
static uint32_t g_last_activity_ms = 0;
static bool g_sleep_enabled = true;
static int8_t g_backlight_before_sleep = 16;

// Klipper Integration
static klipper_idle_state_t g_klipper_state = KLIPPER_STATE_READY;
static uint32_t g_klipper_idle_start_ms = 0;

// LED Integration
static bool g_leds_active = true;

// ========================================================================
// GC9A01 Display Commands
// ========================================================================
#define GC9A01_CMD_SLPIN  0x10
#define GC9A01_CMD_SLPOUT 0x11
#define GC9A01_CMD_DISPOFF 0x28
#define GC9A01_CMD_DISPON 0x29

static void gc9a01_sleep_in(void) {
    // Backlight-only mode: No hardware sleep commands
    // Display stays powered, only backlight turns off
    Serial.println("[Display Sleep] Backlight-only sleep (no hardware commands)");
}

static void gc9a01_sleep_out(void) {
    // Backlight-only mode: No hardware wake commands
    // Display was never asleep, just backlight was off
    Serial.println("[Display Sleep] Backlight-only wake (no hardware commands)");
}

// ========================================================================
// Helper: Check if printing
// ========================================================================
static bool is_printer_active(void) {
    // CRITICAL: Never sleep while printing!
    if (moonraker.data.printing) {
        return true;
    }

    // Also not during Homing, Probing, QGL
    if (moonraker.data.homing || moonraker.data.probing || moonraker.data.qgling) {
        return true;
    }

    // Not during heating
    if (moonraker.data.heating_nozzle || moonraker.data.heating_bed) {
        return true;
    }

    return false;
}

// ========================================================================
// Mode-Specific Sleep Logic
// ========================================================================

static bool should_enter_sleep_manual_mode(void) {
    uint32_t now = millis();
    uint32_t inactive_time_sec = (now - g_last_activity_ms) / 1000;
    
    // State Machine for Manual Mode
    if (g_display_state == DISPLAY_STATE_ACTIVE) {
        if (inactive_time_sec >= DISPLAY_IDLE_TIMEOUT_SEC) {
            g_display_state = DISPLAY_STATE_IDLE;
            Serial.println("[Sleep Manual] → IDLE state");
        }
        return false;
    }

    if (g_display_state == DISPLAY_STATE_IDLE) {
        if (inactive_time_sec >= DISPLAY_SLEEP_TIMEOUT_SEC) {
            return true; // Time for sleep
        }
    }
    
    return false;
}

static bool should_enter_sleep_klipper_sync_mode(void) {
    // CRITICAL: Don't sleep if Klipper is not ready!
    // During Klipper restart unready=true, so display stays awake
    if (moonraker.unready || moonraker.unconnected) {
        return false;
    }

    // If Klipper is in IDLE state, start countdown
    if (g_klipper_state == KLIPPER_STATE_IDLE) {
        uint32_t now = millis();
        uint32_t idle_time_sec = (now - g_klipper_idle_start_ms) / 1000;

        if (idle_time_sec >= DISPLAY_SLEEP_DELAY_AFTER_KLIPPER_IDLE_SEC) {
            return true;
        }
    }

    return false;
}

static bool should_enter_sleep_led_sync_mode(void) {
    // If LEDs are off → Display Sleep
    if (!g_leds_active) {
        return true;
    }
    return false;
}

// ========================================================================
// Public API Implementation
// ========================================================================

void display_sleep_init(display_sleep_mode_t mode) {
    g_last_activity_ms = millis();
    g_display_state = DISPLAY_STATE_ACTIVE;
    g_sleep_mode = mode;
    g_klipper_state = KLIPPER_STATE_READY;
    g_leds_active = true;
    
    Serial.println("========================================");
    Serial.println("[Display Sleep] Initialized");
    Serial.printf("[Display Sleep] Mode: %s\n", 
        mode == SLEEP_MODE_MANUAL ? "MANUAL" :
        mode == SLEEP_MODE_KLIPPER_SYNC ? "KLIPPER_SYNC" :
        "LED_SYNC");
    
    if (mode == SLEEP_MODE_MANUAL) {
        Serial.printf("[Display Sleep] Timeouts: Idle=%ds, Sleep=%ds\n", 
                      DISPLAY_IDLE_TIMEOUT_SEC, DISPLAY_SLEEP_TIMEOUT_SEC);
    } else if (mode == SLEEP_MODE_KLIPPER_SYNC) {
        Serial.printf("[Display Sleep] Klipper sync delay: %ds after Klipper IDLE\n",
                      DISPLAY_SLEEP_DELAY_AFTER_KLIPPER_IDLE_SEC);
    }
    Serial.println("========================================");
}

void display_sleep_set_mode(display_sleep_mode_t mode) {
    if (g_sleep_mode != mode) {
        g_sleep_mode = mode;
        Serial.printf("[Display Sleep] Mode changed to: %s\n",
            mode == SLEEP_MODE_MANUAL ? "MANUAL" :
            mode == SLEEP_MODE_KLIPPER_SYNC ? "KLIPPER_SYNC" :
            "LED_SYNC");
        
        // Reset state
        display_sleep_reset_timer();
    }
}

display_sleep_mode_t display_sleep_get_mode(void) {
    return g_sleep_mode;
}

void display_sleep_update(void) {
    if (!g_sleep_enabled) return;
    
    // CRITICAL: Never sleep while printer is active!
    if (is_printer_active()) {
        if (g_display_state != DISPLAY_STATE_ACTIVE) {
            g_display_state = DISPLAY_STATE_ACTIVE;
            g_last_activity_ms = millis();

            // Wake up if sleeping
            if (display_is_sleeping()) {
                display_wake_up();
            }
        }
        return;
    }

    // Check if sleep should be initiated (mode-dependent)
    bool should_sleep = false;
    
    switch (g_sleep_mode) {
        case SLEEP_MODE_MANUAL:
            should_sleep = should_enter_sleep_manual_mode();
            break;
            
        case SLEEP_MODE_KLIPPER_SYNC:
            should_sleep = should_enter_sleep_klipper_sync_mode();
            break;
            
        case SLEEP_MODE_LED_SYNC:
            should_sleep = should_enter_sleep_led_sync_mode();
            break;
    }
    
    if (should_sleep && g_display_state != DISPLAY_STATE_SLEEPING) {
        display_enter_sleep();
    }
}

void display_sleep_reset_timer(void) {
    uint32_t now = millis();
    bool was_sleeping = (g_display_state == DISPLAY_STATE_SLEEPING);
    
    g_last_activity_ms = now;
    
    if (was_sleeping) {
        display_wake_up();
    } else if (g_display_state == DISPLAY_STATE_IDLE) {
        g_display_state = DISPLAY_STATE_ACTIVE;
        Serial.println("[Display Sleep] Returned to ACTIVE (was IDLE)");
    }
}

void display_enter_sleep(void) {
    if (g_display_state == DISPLAY_STATE_SLEEPING) {
        return;
    }
    
    // SAFETY CHECK: Double-check that printer is really inactive
    if (is_printer_active()) {
        Serial.println("[Display Sleep] ⚠️ Sleep BLOCKED - Printer is active!");
        return;
    }
    
    Serial.println("========================================");
    Serial.println("[Display Sleep] ENTERING BACKLIGHT-ONLY SLEEP");
    Serial.printf("[Display Sleep] Reason: %s\n",
        g_sleep_mode == SLEEP_MODE_MANUAL ? "Manual timeout" :
        g_sleep_mode == SLEEP_MODE_KLIPPER_SYNC ? "Klipper IDLE" :
        "LEDs OFF");
    Serial.println("========================================");
    
    g_backlight_before_sleep = 16;
    
    // Turn off backlight
    tft_set_backlight(0);
    Serial.println("[Display Sleep] Backlight OFF");
    
    // Log backlight-only mode (no hardware commands)
    gc9a01_sleep_in();
    
    g_display_state = DISPLAY_STATE_SLEEPING;
    
    Serial.println("[Display Sleep] ✅ Backlight-only sleep active (~7% power saving)");
    Serial.println("========================================");
}

void display_wake_up(void) {
    if (g_display_state != DISPLAY_STATE_SLEEPING) {
        return;
    }
    
    Serial.println("========================================");
    Serial.println("[Display Sleep] WAKING FROM BACKLIGHT-ONLY SLEEP");
    Serial.println("========================================");
    
    // Log backlight-only mode (no hardware commands)
    gc9a01_sleep_out();
    
    // Restore backlight immediately (no delays needed)
    tft_set_backlight(g_backlight_before_sleep);
    Serial.printf("[Display Sleep] Backlight restored to level %d\n", g_backlight_before_sleep);
    
    // Force screen refresh
    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(NULL);
    Serial.println("[Display Sleep] Screen refreshed");
    
    g_display_state = DISPLAY_STATE_ACTIVE;
    g_last_activity_ms = millis();
    
    Serial.println("[Display Sleep] ✅ Fully awake (backlight-only mode)");
    Serial.println("========================================");
}

bool display_is_sleeping(void) {
    return (g_display_state == DISPLAY_STATE_SLEEPING);
}

display_sleep_state_t display_get_state(void) {
    return g_display_state;
}

void display_check_wake_condition(bool status_changed) {
    if (!status_changed) return;
    
    if (g_display_state == DISPLAY_STATE_SLEEPING) {
        Serial.println("[Display Sleep] Status change → WAKE UP");
        display_wake_up();
    } else if (g_display_state == DISPLAY_STATE_IDLE) {
        display_sleep_reset_timer();
    }
}

// ========================================================================
// Klipper Integration
// ========================================================================

void display_update_klipper_idle_state(const char* state) {
    klipper_idle_state_t new_state = KLIPPER_STATE_READY;
    
    if (strcmp(state, "Idle") == 0) {
        new_state = KLIPPER_STATE_IDLE;
    } else if (strcmp(state, "Printing") == 0) {
        new_state = KLIPPER_STATE_PRINTING;
    } else {
        new_state = KLIPPER_STATE_READY;
    }
    
    display_update_klipper_idle_state_enum(new_state);
}

void display_update_klipper_idle_state_enum(klipper_idle_state_t state) {
    if (g_klipper_state != state) {
        klipper_idle_state_t old_state = g_klipper_state;
        g_klipper_state = state;
        
        Serial.printf("[Klipper Idle] State change: %s → %s\n",
            old_state == KLIPPER_STATE_IDLE ? "IDLE" :
            old_state == KLIPPER_STATE_PRINTING ? "PRINTING" : "READY",
            state == KLIPPER_STATE_IDLE ? "IDLE" :
            state == KLIPPER_STATE_PRINTING ? "PRINTING" : "READY");
        
        // On transition to IDLE: Start timer
        if (state == KLIPPER_STATE_IDLE) {
            g_klipper_idle_start_ms = millis();
            Serial.printf("[Klipper Idle] IDLE detected → Display will sleep in %ds\n",
                         DISPLAY_SLEEP_DELAY_AFTER_KLIPPER_IDLE_SEC);
        }

        // On transition from IDLE to active: Wake display
        if (old_state == KLIPPER_STATE_IDLE && state != KLIPPER_STATE_IDLE) {
            if (display_is_sleeping()) {
                Serial.println("[Klipper Idle] Activity detected → Wake up display");
                display_wake_up();
            }
            display_sleep_reset_timer();
        }
    }
}

klipper_idle_state_t display_get_klipper_idle_state(void) {
    return g_klipper_state;
}

void display_reset_klipper_idle_timer(void) {
    g_klipper_idle_start_ms = millis();
    Serial.println("[Klipper Idle] Timer RESET (Klipper ready after restart)");
    
    // If display is sleeping, wake it up
    if (display_is_sleeping()) {
        Serial.println("[Klipper Idle] Waking display after Klipper restart");
        display_wake_up();
    }
}

// ========================================================================
// LED Integration
// ========================================================================

void display_update_led_status(bool leds_active) {
    if (g_leds_active != leds_active) {
        g_leds_active = leds_active;
        
        Serial.printf("[LED Sync] LEDs %s\n", leds_active ? "ON" : "OFF");
        
        if (!leds_active && g_sleep_mode == SLEEP_MODE_LED_SYNC) {
            Serial.println("[LED Sync] LEDs OFF → Entering Sleep");
            display_enter_sleep();
        } else if (leds_active && display_is_sleeping()) {
            Serial.println("[LED Sync] LEDs ON → Waking up");
            display_wake_up();
        }
    }
}

bool display_get_led_status(void) {
    return g_leds_active;
}

// ========================================================================
// Enable/Disable
// ========================================================================

void display_sleep_enable(bool enabled) {
    if (g_sleep_enabled != enabled) {
        g_sleep_enabled = enabled;
        Serial.printf("[Display Sleep] Auto-Sleep %s\n", enabled ? "ENABLED" : "DISABLED");
        
        if (!enabled && display_is_sleeping()) {
            display_wake_up();
        }
    }
}

bool display_sleep_is_enabled(void) {
    return g_sleep_enabled;
}
