#include "LightSensor.h"

LightSensor::LightSensor(uint8_t address)
    : _address(address)
    , _nightThreshold(10.0f)  // Default: 10 lux
    , _lastLux(0.0f)
    , _isNight(false)
    , _isInitialized(false)
{
}

bool LightSensor::begin(int sda, int scl) {
    // Initialize I2C with specified pins
    Wire.begin(sda, scl);
    
    // Initialize BH1750 sensor with the specified address
    _isInitialized = _sensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, _address);
    
    if (_isInitialized) {
        // Perform initial reading
        readLux();
    }
    
    return _isInitialized;
}

float LightSensor::readLux() {
    if (!_isInitialized) {
        return -1.0f;
    }
    
    // Read light level from sensor
    _lastLux = _sensor.readLightLevel();
    
    // Update night detection status
    updateNightStatus();
    
    return _lastLux;
}

void LightSensor::updateNightStatus() {
    // Determine if it's night based on threshold
    _isNight = (_lastLux < _nightThreshold);
}
