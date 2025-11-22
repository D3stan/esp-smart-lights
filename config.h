// IMU
#define QMI_ADRESS 0x6b  // SA0 pin connected to VCC (use 0x6a if connected to GND)
#define QMI8658C_IIC_FREQUENCY 80*1000

// I2C Pins (shared bus for IMU and BH1750)
#define I2C_SDA 12
#define I2C_SCL 11

// BH1750 Light Sensor
#define BH1750_ADDR 0x23  // ADDR pin to GND (use 0x5C if ADDR to VCC)

// LED Strip Control (via MOSFET IRLZ44N)
#define LED_MOSFET_PIN 14  // PWM output to MOSFET gate
#define LED_SHUTOFF_DELAY_MS 10000  // Default shutoff delay (10 seconds)

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

// ========== Wi-Fi Configuration ==========
// These are DEFAULT values, can be overridden via web portal and stored in Preferences

// Wi-Fi Station Mode (connection to existing network)
#define WIFI_HOSTNAME "Centralina-Luci"  // mDNS hostname for the device
#define WIFI_CONNECTION_TIMEOUT_MS 15000  // Time to wait for connection before switching to AP mode
#define WIFI_RETRY_INTERVAL_MS 300000     // Time between reconnection attempts (5 minutes)

// Wi-Fi AP Mode (fallback mode for configuration)
#define WIFI_AP_SSID "Centralina-Luci-Setup"  // SSID when in AP mode
#define WIFI_AP_PASSWORD "setup123"           // AP password (min 8 chars, use empty string for open network)
#define WIFI_AP_CHANNEL 1                      // Wi-Fi channel for AP
#define WIFI_AP_MAX_CONNECTIONS 2              // Max simultaneous connections in AP mode
#define WIFI_AP_HIDDEN false                   // Hide SSID broadcast

// Captive Portal Configuration
#define WIFI_CAPTIVE_PORTAL_ENABLED true       // Enable captive portal redirect in AP mode
#define WIFI_WEB_SERVER_PORT 80                // HTTP server port

// Preferences Storage
#define WIFI_PREFS_NAMESPACE "wifi_config"     // Namespace for Preferences storage
#define WIFI_PREFS_SSID_KEY "ssid"             // Key for stored SSID
#define WIFI_PREFS_PASSWORD_KEY "password"     // Key for stored password
#define WIFI_PREFS_RETRY_KEY "retry_interval"  // Key for retry interval (ms)

// Factory Reset (hold button to reset Wi-Fi credentials)
#define WIFI_RESET_BUTTON_PIN BTN_C            // Button to hold for factory reset (center button)
#define WIFI_RESET_HOLD_TIME_MS 5000           // Hold time required for reset (5 seconds)

// ========== Smart Light Configuration ==========
// User configurable parameters (stored in Preferences)

#define CONFIG_PREFS_NAMESPACE "light_config"  // Namespace for light configuration

// Light Sensor Thresholds
#define DEFAULT_LUX_THRESHOLD 10.0             // Default threshold for night detection (lux)
#define CONFIG_LUX_THRESHOLD_KEY "lux_thresh"  // Preferences key

// IMU Motion Detection Thresholds
#define DEFAULT_ACCEL_THRESHOLD 0.15           // Default acceleration threshold (g) for motion detection
#define DEFAULT_GYRO_THRESHOLD 15.0            // Default gyroscope threshold (deg/s) for motion detection
#define DEFAULT_MOTION_DEBOUNCE_MS 1000        // Default minimum motion time before LED on (ms)

#define CONFIG_ACCEL_THRESHOLD_KEY "accel_th"  // Preferences key
#define CONFIG_GYRO_THRESHOLD_KEY "gyro_th"    // Preferences key
#define CONFIG_MOTION_DEBOUNCE_KEY "motion_db" // Preferences key

// LED Control
#define DEFAULT_LED_SHUTOFF_DELAY_MS 30000     // Default delay before LED off after motion stops (30 seconds)
#define CONFIG_LED_SHUTOFF_KEY "led_shutoff"   // Preferences key

// Event Logging
#define LOG_PREFS_NAMESPACE "event_logs"       // Namespace for event logs
#define MAX_LOG_ENTRIES 100                    // Maximum number of log entries to store
#define LOG_RETENTION_DAYS 7                   // Keep logs for 7 days