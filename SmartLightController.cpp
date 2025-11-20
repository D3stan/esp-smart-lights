#include "SmartLightController.h"

SmartLightController::SmartLightController(
    MotionDetector& motionDetector,
    LightSensor& lightSensor,
    LEDController& ledController
)
    : _motionDetector(motionDetector)
    , _lightSensor(lightSensor)
    , _ledController(ledController)
    , _shutoffDelayMs(30000)  // Default 30 seconds
    , _autoModeEnabled(true)
    , _currentState(State::OFF)
    , _countdownStartTime(0)
    , _countdownActive(false)
    , _manualOverride(false)
    , _lightSensorBypass(false)
{
}

void SmartLightController::begin(unsigned long shutoffDelayMs) {
    _shutoffDelayMs = shutoffDelayMs;
    _currentState = State::OFF;
    _countdownActive = false;
    _manualOverride = false;
    _autoModeEnabled = true;
    
    // Ensure LED starts off
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
            break;
            
        case State::ON:
            _ledController.turnOn(255);  // Full brightness
            _countdownActive = false;
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
}

void SmartLightController::forceOff() {
    _manualOverride = true;
    _ledController.turnOff();
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
