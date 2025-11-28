#ifndef DISPLAY_SLEEP_H
#define DISPLAY_SLEEP_H

#include <Arduino.h>

// ========================================================================
// Display Sleep Management for KNOMI - Extended Version
// ========================================================================
// Intelligent power management with Klipper synchronization:
//
// Features:
// - No sleep while printing
// - Synchronization with Klipper idle_timeout
// - Integration with LED effects
// - Configurable modes
// ========================================================================

// Sleep Modes
typedef enum {
    SLEEP_MODE_MANUAL = 0,       // Fixed timeouts (as before)
    SLEEP_MODE_KLIPPER_SYNC,     // Sync with Klipper idle_timeout
    SLEEP_MODE_LED_SYNC          // Sync with LED effects (when LEDs off → Display Sleep)
} display_sleep_mode_t;

// Display Sleep States
typedef enum {
    DISPLAY_STATE_ACTIVE = 0,    // Normal active
    DISPLAY_STATE_IDLE,          // Idle-GIF is shown
    DISPLAY_STATE_SLEEPING       // Display in Sleep Mode
} display_sleep_state_t;

// Klipper Idle States (from idle_timeout)
typedef enum {
    KLIPPER_STATE_READY = 0,     // Ready, but inactive
    KLIPPER_STATE_PRINTING,      // Currently printing
    KLIPPER_STATE_IDLE           // Idle (motors off)
} klipper_idle_state_t;

// ========================================================================
// Configuration
// ========================================================================

// Time constants in seconds (only for MANUAL mode)
#define DISPLAY_IDLE_TIMEOUT_SEC   60    // After 60s → Idle-GIF
#define DISPLAY_SLEEP_TIMEOUT_SEC  300   // After 5 min → Display Sleep

// For KLIPPER_SYNC mode: How long after Klipper-Idle until Display-Sleep?
#define DISPLAY_SLEEP_DELAY_AFTER_KLIPPER_IDLE_SEC  10  // 10s after Klipper Idle

// ========================================================================
// Public API
// ========================================================================

/**
 * @brief Initialize Display Sleep Management
 * @param mode Sleep mode (MANUAL, KLIPPER_SYNC, LED_SYNC)
 */
void display_sleep_init(display_sleep_mode_t mode);

/**
 * @brief Set Sleep mode at runtime
 * @param mode New sleep mode
 */
void display_sleep_set_mode(display_sleep_mode_t mode);

/**
 * @brief Get current sleep mode
 * @return display_sleep_mode_t
 */
display_sleep_mode_t display_sleep_get_mode(void);

/**
 * @brief Update Sleep Timer (Call in main loop)
 * Checks timer and performs sleep/wake
 * IMPORTANT: Automatically blocks sleep while printing!
 */
void display_sleep_update(void);

/**
 * @brief Reset inactivity timer
 * Call on touch events or user interaction
 */
void display_sleep_reset_timer(void);

/**
 * @brief Put display into sleep mode
 */
void display_enter_sleep(void);

/**
 * @brief Wake display from sleep
 */
void display_wake_up(void);

/**
 * @brief Check if display is sleeping
 * @return true if display is in sleep mode
 */
bool display_is_sleeping(void);

/**
 * @brief Get current display state
 * @return display_sleep_state_t
 */
display_sleep_state_t display_get_state(void);

/**
 * @brief Wake-up on Moonraker status changes
 * @param status_changed true if relevant status has changed
 */
void display_check_wake_condition(bool status_changed);

// ========================================================================
// Klipper Integration API
// ========================================================================

/**
 * @brief Update Klipper Idle Status (for KLIPPER_SYNC mode)
 * Call when idle_timeout state from Klipper is received
 * @param state Klipper idle_timeout state ("Ready", "Printing", "Idle")
 */
void display_update_klipper_idle_state(const char* state);

/**
 * @brief Update Klipper Idle Status (enum)
 * @param state klipper_idle_state_t
 */
void display_update_klipper_idle_state_enum(klipper_idle_state_t state);

/**
 * @brief Get current Klipper Idle State
 * @return klipper_idle_state_t
 */
klipper_idle_state_t display_get_klipper_idle_state(void);

/**
 * @brief Reset Klipper Idle Timer (e.g. after restart)
 * Called when Klipper becomes ready again after unready
 */
void display_reset_klipper_idle_timer(void);

// ========================================================================
// LED Integration API
// ========================================================================

/**
 * @brief Update LED Status (for LED_SYNC mode)
 * When LEDs turn off, display also goes to sleep
 * @param leds_active true if LEDs are on
 */
void display_update_led_status(bool leds_active);

/**
 * @brief Get LED Status
 * @return true if LEDs are active
 */
bool display_get_led_status(void);

// ========================================================================
// Force Sleep/Wake (for manual control)
// ========================================================================

/**
 * @brief Enable/Disable automatic sleep management
 * @param enabled true = Auto-Sleep on, false = off
 */
void display_sleep_enable(bool enabled);

/**
 * @brief Check if auto-sleep is enabled
 * @return true if enabled
 */
bool display_sleep_is_enabled(void);

#endif // DISPLAY_SLEEP_H
