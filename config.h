// IMU
#define QMI_ADRESS 0x6b  // SA0 pin connected to VCC (use 0x6a if connected to GND)
#define QMI8658C_IIC_FREQUENCY 80*1000

// I2C Pins (shared bus for IMU and BH1750)
#define I2C_SDA 12
#define I2C_SCL 11

// BH1750 Light Sensor
#define BH1750_ADDR 0x23  // ADDR pin to GND (use 0x5C if ADDR to VCC)

// LED Strip Control (via MOSFET IRLZ44N)
#define LED_MOSFET_PIN 41  // PWM output to MOSFET gate

// RGB Led
#define BTN_R 48
#define BTN_C 47
#define BTN_L 0

// Define the pins for your display (adjust these to your setup)
#define TFT_CS     35  // Chip select pin
#define TFT_RST    34  // Reset pin
#define TFT_DC     36  // Data/Command pin
#define TFT_BL     33  // Backlight pin (TFT_BL/IO33)

#define RGB_PW 7
#define RGB_BRIGHTNESS 64