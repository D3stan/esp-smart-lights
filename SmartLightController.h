#ifndef SMART_LIGHT_CONTROLLER_H
#define SMART_LIGHT_CONTROLLER_H

#include <Arduino.h>
#include "MotionDetector.h"
#include "LightSensor.h"
#include "LEDController.h"

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
     */
    SmartLightController(
        MotionDetector& motionDetector,
        LightSensor& lightSensor,
        LEDController& ledController
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

private:
    // Sensor and actuator references
    MotionDetector& _motionDetector;
    LightSensor& _lightSensor;
    LEDController& _ledController;
    
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
    
    // Override management
    bool _manualOverride;
    
    // Bypass for testing
    bool _lightSensorBypass;
    
    // Helper methods
    void transitionTo(State newState);
    void handleStateOff();
    void handleStateOn();
    void handleStateCountdown();
};

#endif // SMART_LIGHT_CONTROLLER_H
