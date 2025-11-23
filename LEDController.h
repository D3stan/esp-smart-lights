#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>

/**
 * @brief LED strip controller using PWM and MOSFET
 * 
 * Controls LED strip brightness through a MOSFET (IRLZ44N) using PWM.
 * Implements smooth transitions and power management.
 */
class LEDController {
public:
    /**
     * @brief Constructor
     * @param pin GPIO pin connected to MOSFET gate
     * @param invertPwm Set to true if MOSFET logic is inverted (default: false)
     */
    explicit LEDController(uint8_t pin, bool invertPwm = false);
    
    /**
     * @brief Initialize the LED controller
     * @param frequency PWM frequency in Hz (default: 5000 Hz)
     * @param resolution PWM resolution in bits (default: 8 bits = 0-255)
     * @return true if initialization successful
     */
    bool begin(uint32_t frequency = 5000, uint8_t resolution = 8);
    
    /**
     * @brief Set LED brightness
     * @param brightness Brightness value (0 = off, 255 = max with 8-bit resolution)
     */
    void setBrightness(uint8_t brightness);
    
    /**
     * @brief Turn LED strip on at specified brightness
     * @param brightness Target brightness (default: maximum)
     */
    void turnOn(uint8_t brightness = 255);
    
    /**
     * @brief Turn LED strip off
     */
    void turnOff();
    
    /**
     * @brief Get current brightness level
     * @return Current brightness value
     */
    uint8_t getBrightness() const { return _currentBrightness; }
    
    /**
     * @brief Check if LED strip is on
     * @return true if brightness > 0
     */
    bool isOn() const { return _currentBrightness > 0; }
    
    /**
     * @brief Fade to target brightness smoothly
     * @param targetBrightness Target brightness level
     * @param durationMs Duration of fade in milliseconds
     */
    void fadeTo(uint8_t targetBrightness, uint16_t durationMs = 500);
    
private:
    uint8_t _pin;
    uint8_t _pwmChannel;
    uint8_t _pwmResolution;
    uint8_t _currentBrightness;
    uint16_t _maxValue;  // Maximum PWM value based on resolution
    bool _isInitialized;
    bool _invertPwm;     // Invert PWM output (for inverted MOSFET logic)
    
    // Convert brightness to PWM duty cycle
    uint16_t brightnessToDuty(uint8_t brightness) const;
};

#endif // LED_CONTROLLER_H
