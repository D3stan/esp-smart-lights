#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include <Wire.h>
#include <BH1750.h>

/**
 * @brief Light sensor wrapper class for BH1750
 * 
 * Manages the BH1750 ambient light sensor with OOP principles.
 * Provides lux readings and night/day detection based on configurable threshold.
 */
class LightSensor {
public:
    /**
     * @brief Constructor
     * @param address I2C address of the BH1750 sensor
     */
    explicit LightSensor(uint8_t address = 0x23);
    
    /**
     * @brief Initialize the sensor
     * @param sda SDA pin for I2C
     * @param scl SCL pin for I2C
     * @return true if initialization successful, false otherwise
     */
    bool begin(int sda, int scl);
    
    /**
     * @brief Read current light level in lux
     * @return Light level in lux, or -1.0f on error
     */
    float readLux();
    
    /**
     * @brief Check if it's currently night (below threshold)
     * @return true if light level is below night threshold
     */
    bool isNight() const { return _isNight; }
    
    /**
     * @brief Set the night threshold in lux
     * @param threshold Lux value below which it's considered night
     */
    void setNightThreshold(float threshold) { _nightThreshold = threshold; }
    
    /**
     * @brief Get the current night threshold
     * @return Night threshold in lux
     */
    float getNightThreshold() const { return _nightThreshold; }
    
    /**
     * @brief Get the last measured lux value
     * @return Last lux reading
     */
    float getLastLux() const { return _lastLux; }
    
    /**
     * @brief Check if sensor is initialized and working
     * @return true if sensor is ready
     */
    bool isReady() const { return _isInitialized; }

private:
    BH1750 _sensor;
    uint8_t _address;
    float _nightThreshold;
    float _lastLux;
    bool _isNight;
    bool _isInitialized;
    
    // Update night detection based on current lux reading
    void updateNightStatus();
};

#endif // LIGHT_SENSOR_H
