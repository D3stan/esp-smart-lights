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
    , _movementBypass(false)
    , _timeWindowEnabled(false)
    , _timeWindowInverted(false)
    , _timeWindowStart(DEFAULT_TIME_WINDOW_START)
    , _timeWindowEnd(DEFAULT_TIME_WINDOW_END)
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
    _movementBypass = false;
    
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
    // Core logic: LED should be ON if:
    // 1. It's night (or light sensor bypassed)
    // 2. There's movement (or movement sensor bypassed)
    // 3. Current time is within allowed window (if time window enabled)
    
    bool isNightCondition = _lightSensorBypass ? true : _lightSensor.isNight();
    bool isMovingCondition = _movementBypass ? true : _motionDetector.isMoving();
    bool isTimeWindowOk = isWithinTimeWindow();
    
    return isNightCondition && isMovingCondition && isTimeWindowOk;
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
    
    // Load time window configuration
    _timeWindowEnabled = prefs.getBool(CONFIG_TIME_WINDOW_ENABLED_KEY, DEFAULT_TIME_WINDOW_ENABLED);
    _timeWindowInverted = prefs.getBool(CONFIG_TIME_WINDOW_INVERTED_KEY, false);
    _timeWindowStart = prefs.getUChar(CONFIG_TIME_WINDOW_START_KEY, DEFAULT_TIME_WINDOW_START);
    _timeWindowEnd = prefs.getUChar(CONFIG_TIME_WINDOW_END_KEY, DEFAULT_TIME_WINDOW_END);
    
    // Load bypass states
    _movementBypass = prefs.getBool(CONFIG_MOVEMENT_BYPASS_KEY, false);
    
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
    Serial.print("  Time window enabled: ");
    Serial.println(_timeWindowEnabled ? "YES" : "NO");
    if (_timeWindowEnabled) {
        Serial.print("  Time window: ");
        Serial.print(_timeWindowStart);
        Serial.print(":00 - ");
        Serial.print(_timeWindowEnd);
        Serial.print(":00 (");
        Serial.print(_timeWindowInverted ? "INVERTITO" : "NORMALE");
        Serial.println(")");
    }
    Serial.print("  Movement bypass: ");
    Serial.println(_movementBypass ? "YES" : "NO");
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
    
    // Save time window configuration
    prefs.putBool(CONFIG_TIME_WINDOW_ENABLED_KEY, _timeWindowEnabled);
    prefs.putBool(CONFIG_TIME_WINDOW_INVERTED_KEY, _timeWindowInverted);
    prefs.putUChar(CONFIG_TIME_WINDOW_START_KEY, _timeWindowStart);
    prefs.putUChar(CONFIG_TIME_WINDOW_END_KEY, _timeWindowEnd);
    
    // Save bypass states
    prefs.putBool(CONFIG_MOVEMENT_BYPASS_KEY, _movementBypass);
    
    prefs.end();
    
    Serial.println("Configuration saved to Preferences");
}

void SmartLightController::setTimeWindow(uint8_t startHour, uint8_t endHour) {
    // Validate hours (0-23)
    if (startHour > 23) startHour = 23;
    if (endHour > 23) endHour = 23;
    
    _timeWindowStart = startHour;
    _timeWindowEnd = endHour;
    
    Serial.print("Time window set: ");
    Serial.print(_timeWindowStart);
    Serial.print(":00 - ");
    Serial.print(_timeWindowEnd);
    Serial.println(":00");
}

bool SmartLightController::isWithinTimeWindow() const {
    // If time window is disabled, always return true
    if (!_timeWindowEnabled) {
        return true;
    }
    
    // Get current time
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        // If time is not available, allow operation (fail-safe)
        Serial.println("WARNING: Time not available, ignoring time window");
        return true;
    }
    
    uint8_t currentHour = timeinfo.tm_hour;
    bool isInWindow = false;
    
    // Calculate if current time is within the configured window
    // Normal case: start < end (e.g. 7:00 - 17:00)
    if (_timeWindowStart < _timeWindowEnd) {
        isInWindow = (currentHour >= _timeWindowStart && currentHour < _timeWindowEnd);
    }
    // Wrap-around case: start > end (e.g. 22:00 - 6:00 = nighttime only)
    else if (_timeWindowStart > _timeWindowEnd) {
        isInWindow = (currentHour >= _timeWindowStart || currentHour < _timeWindowEnd);
    }
    // Edge case: start == end (window disabled or 24h)
    else {
        isInWindow = true;
    }
    
    // Apply inversion logic if enabled
    // If inverted: return true when OUTSIDE window, false when INSIDE
    // If normal: return true when INSIDE window, false when OUTSIDE
    return _timeWindowInverted ? !isInWindow : isInWindow;
}
