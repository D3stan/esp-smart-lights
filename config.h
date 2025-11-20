// IMU
#define QMI_ADRESS 0x6b  // SA0 pin connected to VCC (use 0x6a if connected to GND)
#define QMI8658C_IIC_FREQUENCY 80*1000

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