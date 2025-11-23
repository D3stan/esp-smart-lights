#ifndef SMART_LIGHT_CONTROLLER_H
#define SMART_LIGHT_CONTROLLER_H

#include <Arduino.h>
#include <Preferences.h>
#include "MotionDetector.h"
#include "LightSensor.h"
#include "LEDController.h"
#include "EventLogger.h"

/**
 * @brief Smart Light Controller - Main logic controller
 * 
 * Implements the core logic for automatic LED control based on:
 * - Night condition (low ambient light)
 * - Motion condition (robot movement detected)
 * 
 * LED turns ON when: isNight() AND isMoving()
 * LED turns OFF when: either condition becomes false, with configurable debounce delay
 */
class SmartLightController {
public:
    /**
     * @brief Constructor
     * @param motionDetector Reference to motion detector instance
     * @param lightSensor Reference to light sensor instance
     * @param ledController Reference to LED controller instance
     * @param eventLogger Pointer to event logger (optional)
     */
    SmartLightController(
        MotionDetector& motionDetector,
        LightSensor& lightSensor,
        LEDController& ledController,
        EventLogger* eventLogger = nullptr
    );
    
    /**
     * @brief Initialize the smart light controller
     * @param shutoffDelayMs Delay in milliseconds before turning off LED after conditions are no longer met (default: 30000ms = 30s)
     */
    void begin(unsigned long shutoffDelayMs = 30000);
    
    /**
     * @brief Main update loop - must be called frequently
     * 
     * Reads sensors and updates LED state according to the logic:
     * - If (Night AND Moving) → Turn ON LED
     * - If NOT (Night AND Moving) → Start countdown to turn OFF
     * - If countdown expires → Turn OFF LED
     */
    void update();
    
    /**
     * @brief Enable or disable automatic control
     * @param enabled true to enable automatic control, false to disable
     */
    void setAutoMode(bool enabled) { _autoModeEnabled = enabled; }
    
    /**
     * @brief Check if automatic mode is enabled
     * @return true if auto mode is active
     */
    bool isAutoModeEnabled() const { return _autoModeEnabled; }
    
    /**
     * @brief Force LED on (overrides automatic control temporarily)
     * @param brightness LED brightness (0-255)
     */
    void forceOn(uint8_t brightness = 255);
    
    /**
     * @brief Force LED off (overrides automatic control temporarily)
     */
    void forceOff();
    
    /**
     * @brief Return to automatic control mode
     */
    void returnToAuto();
    
    /**
     * @brief Enable/disable light sensor bypass (for testing)
     * When bypassed, light sensor always returns "night" condition
     * @param bypass true to bypass light sensor, false for normal operation
     */
    void setLightSensorBypass(bool bypass) { _lightSensorBypass = bypass; }
    
    /**
     * @brief Check if light sensor is bypassed
     * @return true if bypassed
     */
    bool isLightSensorBypassed() const { return _lightSensorBypass; }
    
    /**
     * @brief Enable/disable movement sensor bypass (for testing)
     * When bypassed, movement sensor always returns "moving" condition
     * @param bypass true to bypass movement sensor, false for normal operation
     */
    void setMovementBypass(bool bypass) { _movementBypass = bypass; }
    
    /**
     * @brief Check if movement sensor is bypassed
     * @return true if bypassed
     */
    bool isMovementBypassed() const { return _movementBypass; }
    
    /**
     * @brief Enable/disable time window restriction
     * @param enabled true to enable time window, false to disable
     */
    void setTimeWindowEnabled(bool enabled) { _timeWindowEnabled = enabled; }
    
    /**
     * @brief Check if time window is enabled
     * @return true if enabled
     */
    bool isTimeWindowEnabled() const { return _timeWindowEnabled; }
    
    /**
     * @brief Set time window for LED operation
     * @param startHour Hour to start allowing LED (0-23)
     * @param endHour Hour to stop allowing LED (0-23)
     */
    void setTimeWindow(uint8_t startHour, uint8_t endHour);
    
    /**
     * @brief Enable/disable time window inversion
     * When inverted, LED operates OUTSIDE the configured window
     * @param inverted true to invert logic, false for normal
     */
    void setTimeWindowInverted(bool inverted) { _timeWindowInverted = inverted; }
    
    /**
     * @brief Check if time window is inverted
     * @return true if inverted
     */
    bool isTimeWindowInverted() const { return _timeWindowInverted; }
    
    /**
     * @brief Get time window start hour
     * @return Start hour (0-23)
     */
    uint8_t getTimeWindowStart() const { return _timeWindowStart; }
    
    /**
     * @brief Get time window end hour
     * @return End hour (0-23)
     */
    uint8_t getTimeWindowEnd() const { return _timeWindowEnd; }
    
    /**
     * @brief Check if current time is within the allowed window
     * @return true if within window or window disabled
     */
    bool isWithinTimeWindow() const;
    
    /**
     * @brief Check if manual override is active
     * @return true if manual override is active
     */
    bool isManualOverride() const { return _manualOverride; }
    
    /**
     * @brief Set the shutoff delay time
     * @param delayMs Delay in milliseconds
     */
    void setShutoffDelay(unsigned long delayMs) { _shutoffDelayMs = delayMs; }
    
    /**
     * @brief Get the current shutoff delay
     * @return Delay in milliseconds
     */
    unsigned long getShutoffDelay() const { return _shutoffDelayMs; }
    
    /**
     * @brief Check if LED should be on according to conditions
     * @return true if conditions are met for LED to be on
     */
    bool shouldLEDBeOn() const;
    
    /**
     * @brief Get countdown time remaining before shutoff
     * @return Milliseconds remaining, or 0 if not in countdown
     */
    unsigned long getCountdownRemaining() const;
    
    /**
     * @brief Check if currently in shutoff countdown
     * @return true if countdown is active
     */
    bool isInCountdown() const { return _countdownActive; }
    
    /**
     * @brief Get current operational state as string (for debugging)
     * @return State description
     */
    const char* getStateString() const;
    
    /**
     * @brief Load configuration from Preferences
     * Loads thresholds and delays from non-volatile memory
     */
    void loadConfiguration();
    
    /**
     * @brief Save current configuration to Preferences
     */
    void saveConfiguration();

private:
    // Sensor and actuator references
    MotionDetector& _motionDetector;
    LightSensor& _lightSensor;
    LEDController& _ledController;
    EventLogger* _eventLogger;
    
    // Configuration
    unsigned long _shutoffDelayMs;
    bool _autoModeEnabled;
    
    // State management
    enum class State {
        OFF,              // LED is off, conditions not met
        ON,               // LED is on, conditions met
        COUNTDOWN         // Conditions no longer met, countdown before shutoff
    };
    
    State _currentState;
    unsigned long _countdownStartTime;
    bool _countdownActive;
    bool _lastLEDState;  // Track LED state changes for logging
    
    // Override management
    bool _manualOverride;
    
    // Bypass for testing
    bool _lightSensorBypass;
    bool _movementBypass;
    
    // Time window configuration
    bool _timeWindowEnabled;
    bool _timeWindowInverted;  // If true, operate OUTSIDE window
    uint8_t _timeWindowStart;  // 0-23 hour
    uint8_t _timeWindowEnd;    // 0-23 hour
    
    // Helper methods
    void transitionTo(State newState);
    void handleStateOff();
    void handleStateOn();
    void handleStateCountdown();
};

#endif // SMART_LIGHT_CONTROLLER_H
