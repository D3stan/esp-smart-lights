#include "LEDController.h"

LEDController::LEDController(uint8_t pin, uint8_t pwmChannel)
    : _pin(pin)
    , _pwmChannel(pwmChannel)
    , _pwmResolution(8)
    , _currentBrightness(0)
    , _maxValue(255)
    , _isInitialized(false)
{
}

bool LEDController::begin(uint32_t frequency, uint8_t resolution) {
    _pwmResolution = resolution;
    _maxValue = (1 << resolution) - 1;  // 2^resolution - 1

    // Attach pin to PWM channel
    ledcAttach(_pin, frequency, resolution);
    
    // Start with LED off
    ledcWrite(_pwmChannel, 0);
    _currentBrightness = 0;
    
    _isInitialized = true;
    return true;
}

void LEDController::setBrightness(uint8_t brightness) {
    if (!_isInitialized) {
        return;
    }
    
    _currentBrightness = brightness;
    uint16_t duty = brightnessToDuty(brightness);
    ledcWrite(_pwmChannel, duty);
}

void LEDController::turnOn(uint8_t brightness) {
    setBrightness(brightness);
}

void LEDController::turnOff() {
    setBrightness(0);
}

void LEDController::fadeTo(uint8_t targetBrightness, uint16_t durationMs) {
    if (!_isInitialized) {
        return;
    }
    
    int16_t startBrightness = _currentBrightness;
    int16_t brightnessRange = targetBrightness - startBrightness;
    
    // If already at target, nothing to do
    if (brightnessRange == 0) {
        return;
    }
    
    // Calculate step delay for smooth transition
    constexpr uint16_t steps = 50;
    uint16_t stepDelay = durationMs / steps;
    
    for (uint16_t i = 0; i <= steps; i++) {
        uint8_t newBrightness = startBrightness + (brightnessRange * i / steps);
        setBrightness(newBrightness);
        delay(stepDelay);
    }
    
    // Ensure we reach exact target
    setBrightness(targetBrightness);
}

uint16_t LEDController::brightnessToDuty(uint8_t brightness) const {
    // Map brightness (0-255) to duty cycle (0-_maxValue)
    // Using 32-bit arithmetic to avoid overflow
    return static_cast<uint16_t>((static_cast<uint32_t>(brightness) * _maxValue) / 255);
}
