#include "SmartLightController.h"
#include "config.h"

SmartLightController::SmartLightController(
    MotionDetector& motionDetector,
    LightSensor& lightSensor,
    LEDController& ledController,
    EventLogger* eventLogger
)
    : _motionDetector(motionDetector)
    , _lightSensor(lightSensor)
    , _ledController(ledController)
    , _eventLogger(eventLogger)
    , _shutoffDelayMs(DEFAULT_LED_SHUTOFF_DELAY_MS)
    , _autoModeEnabled(true)
    , _currentState(State::OFF)
    , _countdownStartTime(0)
    , _countdownActive(false)
    , _lastLEDState(false)
    , _manualOverride(false)
    , _lightSensorBypass(false)
{
}

void SmartLightController::begin(unsigned long shutoffDelayMs) {
    // Load configuration from Preferences
    loadConfiguration();
    
    // Override with parameter if provided
    if (shutoffDelayMs > 0) {
        _shutoffDelayMs = shutoffDelayMs;
    }
    
    // Reset all state variables to defaults
    _currentState = State::OFF;
    _countdownActive = false;
    _manualOverride = false;
    _autoModeEnabled = true;
    _lastLEDState = false;
    _lightSensorBypass = false;
    
    // Force LED off at startup
    _ledController.turnOff();
}

void SmartLightController::update() {
    // If manual override is active, don't update automatically
    if (_manualOverride || !_autoModeEnabled) {
        return;
    }
    
    // Process current state
    switch (_currentState) {
        case State::OFF:
            handleStateOff();
            break;
        case State::ON:
            handleStateOn();
            break;
        case State::COUNTDOWN:
            handleStateCountdown();
            break;
    }
}

bool SmartLightController::shouldLEDBeOn() const {
    // Core logic: LED should be ON if it's night AND there's movement
    // If light sensor is bypassed, always consider it "night"
    bool isNightCondition = _lightSensorBypass ? true : _lightSensor.isNight();
    return isNightCondition && _motionDetector.isMoving();
}

void SmartLightController::handleStateOff() {
    // Check if conditions are met to turn on
    if (shouldLEDBeOn()) {
        transitionTo(State::ON);
    }
}

void SmartLightController::handleStateOn() {
    // Check if conditions are no longer met
    if (!shouldLEDBeOn()) {
        transitionTo(State::COUNTDOWN);
    }
    // Otherwise stay on (LED already on from transition)
}

void SmartLightController::handleStateCountdown() {
    // Check if conditions are met again (cancel countdown)
    if (shouldLEDBeOn()) {
        transitionTo(State::ON);
        return;
    }
    
    // Check if countdown has expired
    unsigned long elapsed = millis() - _countdownStartTime;
    if (elapsed >= _shutoffDelayMs) {
        transitionTo(State::OFF);
    }
    // Otherwise keep LED on and continue countdown
}

void SmartLightController::transitionTo(State newState) {
    if (_currentState == newState) {
        return;  // No transition needed
    }
    
    State oldState = _currentState;
    _currentState = newState;
    
    // Handle state entry actions
    switch (newState) {
        case State::OFF:
            _ledController.turnOff();
            _countdownActive = false;
            
            // Log OFF event if LED was on
            if (_lastLEDState && _eventLogger) {
                const char* mode = _manualOverride ? (_autoModeEnabled ? "auto" : "manual") : "auto";
                _eventLogger->logEvent(false, _lightSensor.getLastLux(), 
                                      _motionDetector.isMoving(), mode);
            }
            _lastLEDState = false;
            break;
            
        case State::ON:
            {
                // Load saved brightness from Preferences
                Preferences prefs;
                prefs.begin(CONFIG_PREFS_NAMESPACE, true);
                uint8_t brightness = prefs.getUChar(CONFIG_LED_BRIGHTNESS_KEY, DEFAULT_LED_BRIGHTNESS);
                prefs.end();
                
                _ledController.turnOn(brightness);
                _countdownActive = false;
                
                // Log ON event if LED was off
                if (!_lastLEDState && _eventLogger) {
                    const char* mode = _manualOverride ? (_autoModeEnabled ? "auto" : "manual") : "auto";
                    _eventLogger->logEvent(true, _lightSensor.getLastLux(), 
                                          _motionDetector.isMoving(), mode);
                }
                _lastLEDState = true;
            }
            break;
            
        case State::COUNTDOWN:
            // LED stays on during countdown
            _countdownStartTime = millis();
            _countdownActive = true;
            break;
    }
}

void SmartLightController::forceOn(uint8_t brightness) {
    _manualOverride = true;
    _ledController.turnOn(brightness);
    
    // Log event if state changed
    if (!_lastLEDState && _eventLogger) {
        _eventLogger->logEvent(true, _lightSensor.getLastLux(), 
                              _motionDetector.isMoving(), "on");
    }
    _lastLEDState = true;
}

void SmartLightController::forceOff() {
    _manualOverride = true;
    _ledController.turnOff();
    
    // Log event if state changed
    if (_lastLEDState && _eventLogger) {
        _eventLogger->logEvent(false, _lightSensor.getLastLux(), 
                              _motionDetector.isMoving(), "off");
    }
    _lastLEDState = false;
}

void SmartLightController::returnToAuto() {
    _manualOverride = false;
    // Reset to OFF state and let automatic control take over
    _currentState = State::OFF;
    _countdownActive = false;
    _ledController.turnOff();
}

unsigned long SmartLightController::getCountdownRemaining() const {
    if (!_countdownActive) {
        return 0;
    }
    
    unsigned long elapsed = millis() - _countdownStartTime;
    if (elapsed >= _shutoffDelayMs) {
        return 0;
    }
    
    return _shutoffDelayMs - elapsed;
}

const char* SmartLightController::getStateString() const {
    switch (_currentState) {
        case State::OFF:
            return "OFF";
        case State::ON:
            return "ON";
        case State::COUNTDOWN:
            return "COUNTDOWN";
        default:
            return "UNKNOWN";
    }
}

void SmartLightController::loadConfiguration() {
    Preferences prefs;
    if (!prefs.begin(CONFIG_PREFS_NAMESPACE, true)) {  // Read-only
        Serial.println("Failed to open config preferences for reading");
        return;
    }
    
    // Load LED shutoff delay
    _shutoffDelayMs = prefs.getULong(CONFIG_LED_SHUTOFF_KEY, DEFAULT_LED_SHUTOFF_DELAY_MS);
    
    // Load thresholds and apply to sensors
    float luxThresh = prefs.getFloat(CONFIG_LUX_THRESHOLD_KEY, DEFAULT_LUX_THRESHOLD);
    float accelThresh = prefs.getFloat(CONFIG_ACCEL_THRESHOLD_KEY, DEFAULT_ACCEL_THRESHOLD);
    float gyroThresh = prefs.getFloat(CONFIG_GYRO_THRESHOLD_KEY, DEFAULT_GYRO_THRESHOLD);
    
    prefs.end();
    
    // Apply loaded values
    _lightSensor.setNightThreshold(luxThresh);
    _motionDetector.setAccThreshold(accelThresh);
    _motionDetector.setGyroThreshold(gyroThresh);
    
    Serial.println("Configuration loaded from Preferences:");
    Serial.print("  Lux threshold: ");
    Serial.println(luxThresh);
    Serial.print("  Accel threshold: ");
    Serial.println(accelThresh);
    Serial.print("  Gyro threshold: ");
    Serial.println(gyroThresh);
    Serial.print("  Shutoff delay: ");
    Serial.print(_shutoffDelayMs / 1000);
    Serial.println(" seconds");
}

void SmartLightController::saveConfiguration() {
    Preferences prefs;
    if (!prefs.begin(CONFIG_PREFS_NAMESPACE, false)) {  // Read-write
        Serial.println("Failed to open config preferences for writing");
        return;
    }
    
    prefs.putULong(CONFIG_LED_SHUTOFF_KEY, _shutoffDelayMs);
    prefs.putFloat(CONFIG_LUX_THRESHOLD_KEY, _lightSensor.getNightThreshold());
    prefs.putFloat(CONFIG_ACCEL_THRESHOLD_KEY, _motionDetector.getAccThreshold());
    prefs.putFloat(CONFIG_GYRO_THRESHOLD_KEY, _motionDetector.getGyroThreshold());
    
    prefs.end();
    
    Serial.println("Configuration saved to Preferences");
}
