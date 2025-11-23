#include <Adafruit_GFX.h>        // Core graphics library
#include <Adafruit_ST7735.h>     // Library for the ST7735 TFT screen
#include <Adafruit_NeoPixel.h>   // RGB Led
#include <SPI.h>
#include <Wire.h>                // I2C library

#include "bitmap.h"
#include "Qmi8658c.h"
#include "MotionDetector.h"
#include "LightSensor.h"
#include "LEDController.h"
#include "SmartLightController.h"
#include "WiFiManager.h"
#include "DisplayManager.h"
#include "EventLogger.h"
#include "OTAManager.h"
#include "DebugHelper.h"
#include "config.h"

// Create an instance of the display
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// IMU
Qmi8658c qmi8658c(QMI_ADRESS, QMI8658C_IIC_FREQUENCY);

// Configuration optimized for slow-moving grass cutting robot
qmi8658_cfg_t qmi8658_cfg = {
  .qmi8658_mode = qmi8658_mode_dual,
  
  // Use most sensitive scales for detecting slow movement
  .acc_scale = acc_scale_2g,      // Most sensitive (change to acc_scale_4g if too sensitive)
  .acc_odr = acc_odr_250,          // Lower sampling rate (125-250 Hz is enough)
  
  .gyro_scale = gyro_scale_32dps,  // Very sensitive rotation detection (try 16dps for even more sensitivity)
  .gyro_odr = gyro_odr_250,
};

// Motion detector instance
MotionDetector motionDetector(&qmi8658c);

// Light sensor instance (BH1750)
LightSensor lightSensor(BH1750_ADDR);

// LED strip controller instance
LEDController ledController(LED_MOSFET_PIN);

// Event logger instance
EventLogger eventLogger;

// Smart light controller instance (main logic)
SmartLightController smartLight(motionDetector, lightSensor, ledController, &eventLogger);

// WiFi manager instance
WiFiManager wifiManager;

// OTA manager instance (will be initialized after WiFi manager)
OTAManager* otaManager = nullptr;

// Display manager instance
DisplayManager displayManager(tft);

// Global variables for brightness control
uint8_t currentRgbBrightness = RGB_BRIGHTNESS;  // Current RGB LED brightness

void setup() {

	Serial.begin(115200);

  //while (!Serial) { 
  //  delay(10); 
  //}
  
  Serial.println("Hello, I am using Native USB-CDC!");

    // Initialize button pins
    pinMode(BTN_R, INPUT_PULLUP);
    pinMode(BTN_C, INPUT_PULLUP);
    pinMode(BTN_L, INPUT_PULLUP);

	// RGB led
	pinMode(RGB_PW, OUTPUT);
	digitalWrite(RGB_PW, HIGH);
	neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0); 

	// Initialize the backlight pin
	pinMode(TFT_BL, OUTPUT);
	digitalWrite(TFT_BL, HIGH);  // Turn on the backlight

	// Initialize the display
	tft.initR(INITR_BLACKTAB);  // Specific initialization for the ST7735 screen
	tft.fillScreen(ST77XX_WHITE); // Clear the screen with black background
	
	// Initialize Display Manager first (shows welcome screen)
	displayManager.begin(1000); // Update every 1 second
	
	// Set LCD timeout from config (30 seconds default)
	displayManager.setLCDTimeout(DEFAULT_LCD_TIMEOUT_MS);
	
	// Draw the bitmap at position (0, 0) - removed to use display manager instead
	// tft.drawBitmap(0, 0, myBitmap, 128, 128, ST77XX_BLACK);

	// Initialize motion detector
	if (!motionDetector.begin(&qmi8658_cfg)) {
		Serial.println("Failed to initialize IMU!");
		while(1) delay(10);
	}
	
	delay(1000);
	Serial.print("deviceID = 0x");
	Serial.println(qmi8658c.deviceID, HEX);
	Serial.print("deviceRevisionID = 0x");
	Serial.println(qmi8658c.deviceRevisionID, HEX);
	
	// Check if device ID is valid (should be 0x05 for QMI8658)
	if (qmi8658c.deviceID != 0x05) {
	  Serial.println("WARNING: Device ID mismatch! Expected 0x05");
	  Serial.println("Check I2C connections and address");
	}
	
	Serial.println("IMU initialized and ready");
	
	// Perform baseline calibration
	Serial.println("\n========== CALIBRATING IMU ==========");
	Serial.println("Keep the device STILL for 3 seconds...");
	delay(1000);
	
	motionDetector.calibrate();
	Serial.println("Calibration complete!");
	Serial.println("====================================\n");
	DebugHelper::printCalibrationValues(motionDetector);
	
	Serial.println("\n====== SYSTEM READY ======");
	Serial.println("Buttons: RED=Calibrate | BLUE=Toggle Light Bypass | GREEN=Test LED");
	Serial.println("All configuration available via Web Dashboard");
	Serial.println("=========================\n");
	
	// Initialize Light Sensor (BH1750)
	Serial.println("\n========== INITIALIZING LIGHT SENSOR ==========");
	if (!lightSensor.begin(I2C_SDA, I2C_SCL)) {
		Serial.println("ERROR: Failed to initialize BH1750!");
		Serial.println("Check I2C connections and address");
	} else {
		Serial.println("BH1750 initialized successfully");
		Serial.print("I2C SDA: GPIO"); Serial.println(I2C_SDA);
		Serial.print("I2C SCL: GPIO"); Serial.println(I2C_SCL);
		Serial.print("BH1750 Address: 0x"); Serial.println(BH1750_ADDR, HEX);
		
		// Read initial lux value
		float lux = lightSensor.readLux();
		Serial.print("Initial Light Level: "); Serial.print(lux); Serial.println(" lux");
		Serial.print("Night Threshold: "); Serial.print(lightSensor.getNightThreshold()); Serial.println(" lux");
	}
	Serial.println("===============================================\n");
	
	// Initialize LED Controller
	Serial.println("\n========== INITIALIZING LED CONTROLLER ==========");
	if (!ledController.begin(5000, 8)) { // 5 kHz, 8-bit resolution
		Serial.println("ERROR: Failed to initialize LED Controller!");
	} else {
		Serial.println("LED Controller initialized successfully");
		Serial.print("MOSFET Pin: GPIO"); Serial.println(LED_MOSFET_PIN);
		Serial.println("PWM Frequency: 5000 Hz");
		Serial.println("PWM Resolution: 8-bit (0-255)");
		
		// Test LED with a quick blink
		Serial.println("Testing LED... (quick blink)");
		ledController.turnOn(255);
		delay(300);
		ledController.turnOff();
		delay(300);
		Serial.println("LED test complete");
	}
	Serial.println("=================================================\n");
	
	// Initialize Smart Light Controller
	Serial.println("\n========== INITIALIZING SMART LIGHT CONTROLLER ==========");
	smartLight.begin(LED_SHUTOFF_DELAY_MS); // Load config and set shutoff delay
    // Light sensor bypass is OFF by default (normal operation)
	
	// Load saved brightness values from Preferences
	Preferences prefs;
	prefs.begin(CONFIG_PREFS_NAMESPACE, true);  // Read-only
	uint8_t savedLedBrightness = prefs.getUChar(CONFIG_LED_BRIGHTNESS_KEY, DEFAULT_LED_BRIGHTNESS);
	currentRgbBrightness = prefs.getUChar(CONFIG_RGB_BRIGHTNESS_KEY, DEFAULT_RGB_BRIGHTNESS);
	prefs.end();
	
	ledController.setBrightness(savedLedBrightness);
	Serial.print("Loaded LED brightness: "); Serial.println(savedLedBrightness);
	Serial.print("Loaded RGB brightness: "); Serial.println(currentRgbBrightness);
	Serial.println("Smart Light Controller initialized");
	Serial.print("Auto Mode: "); Serial.println(smartLight.isAutoModeEnabled() ? "ENABLED" : "DISABLED");
	Serial.print("Shutoff Delay: "); Serial.print(smartLight.getShutoffDelay() / 1000); Serial.println(" seconds");
	Serial.println("Logic: LED ON when (Night AND Movement)");
	Serial.println("=========================================================\n");
	
	// Initialize Event Logger
	Serial.println("\n========== INITIALIZING EVENT LOGGER ==========");
	if (!eventLogger.begin()) {
		Serial.println("ERROR: Failed to initialize Event Logger!");
	} else {
		Serial.println("Event Logger initialized successfully");
		Serial.print("Max log entries: "); Serial.println(MAX_LOG_ENTRIES);
		Serial.print("Retention days: "); Serial.println(LOG_RETENTION_DAYS);
	}
	Serial.println("================================================\n");
	
	// Initialize WiFi Manager
	Serial.println("\n========== INITIALIZING WIFI MANAGER ==========");
	if (!wifiManager.begin()) {
		Serial.println("ERROR: Failed to initialize WiFi Manager!");
		displayManager.showError("WiFi Init Failed");
		delay(3000);
	} else {
		Serial.println("WiFi Manager initialized successfully");
	}
	
	// Link system components to WiFi Manager for API
	wifiManager.setSystemComponents(&smartLight, &lightSensor, &motionDetector, &ledController, &eventLogger, &currentRgbBrightness);
	Serial.println("System components linked to WiFi Manager API");
	Serial.println("===============================================\n");
	
	// Initialize OTA Manager (after WiFi Manager is ready)
	Serial.println("\n========== INITIALIZING OTA MANAGER ==========");
	// Wait for WebServer to be available
	WebServer* webServer = wifiManager.getWebServer();
	if (webServer) {
		otaManager = new OTAManager(*webServer);
		if (!otaManager->begin()) {
			Serial.println("ERROR: Failed to initialize OTA Manager!");
			delete otaManager;
			otaManager = nullptr;
		} else {
			Serial.println("OTA Manager initialized successfully");
			Serial.println("Access OTA interface at: http://<device-ip>/ota");
		}
	} else {
		Serial.println("WARNING: WebServer not available yet, OTA will be initialized when WiFi connects");
	}
	Serial.println("==============================================\n");
	
	// Configure NTP for timestamps (will sync when WiFi connects)
	configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");  // GMT+1 (Italy), DST +1h
	Serial.println("NTP time sync configured (will sync when WiFi connected)");

}

void loop() {
  // Update WiFi Manager (handles reconnection, captive portal, etc.)
  wifiManager.update();
  
  // Initialize OTA Manager if it wasn't initialized and WiFi is now connected
  static bool otaInitAttempted = false;
  if (!otaManager && wifiManager.isConnected() && !otaInitAttempted) {
    Serial.println("\n[OTA] WiFi connected, initializing OTA Manager...");
    WebServer* webServer = wifiManager.getWebServer();
    if (webServer) {
      otaManager = new OTAManager(*webServer);
      if (otaManager->begin()) {
        Serial.println("[OTA] OTA Manager initialized successfully");
        Serial.println("[OTA] Access OTA at: http://" + wifiManager.getIPAddress().toString() + "/ota");
      } else {
        delete otaManager;
        otaManager = nullptr;
      }
    }
    otaInitAttempted = true;
  }
  
  // Update OTA Manager if initialized
  if (otaManager) {
    otaManager->update();
  }
  
  // Read light level every 500ms
  static unsigned long lastLightRead = 0;
  if (millis() - lastLightRead > 500) {
    float lux = lightSensor.readLux();
    lastLightRead = millis();
  }
  
  // Detect motion
  bool isMoving = motionDetector.detectMotion();
  
  // Update smart light controller (main automatic logic)
  smartLight.update();
  
  // Update display with all current information
  displayManager.update(wifiManager, lightSensor, motionDetector, ledController);
  
  // Button controls with wake-on-press functionality
  static unsigned long lastButtonPress = 0;
  if (millis() - lastButtonPress > 500) { // Debounce buttons
    bool btnPressed = false;
    
    if (digitalRead(BTN_R) == LOW) {
      btnPressed = true;
      if (!displayManager.isDisplayOn()) {
        // Just wake display, don't execute action
        displayManager.wakeDisplay();
        Serial.println("[BTN] Display woken by RED button");
      } else {
        // Display is on, execute action
        Serial.println("\n[RED BTN] Recalibrating IMU...");
        displayManager.showMessage("Calibrating IMU...", 2000);
        motionDetector.calibrate();
        DebugHelper::printCalibrationValues(motionDetector);
        smartLight.saveConfiguration();
        Serial.println("Configuration saved after calibration");
        displayManager.showMessage("IMU Calibrated!", 2000);
      }
      lastButtonPress = millis();
    } else if (digitalRead(BTN_L) == LOW) {
      btnPressed = true;
      if (!displayManager.isDisplayOn()) {
        // Just wake display, don't execute action
        displayManager.wakeDisplay();
        Serial.println("[BTN] Display woken by BLUE button");
      } else {
        // Display is on, execute action
        bool currentBypass = smartLight.isLightSensorBypassed();
        smartLight.setLightSensorBypass(!currentBypass);
        Serial.print("\n[BLUE BTN] Light sensor bypass: "); 
        Serial.println(smartLight.isLightSensorBypassed() ? "ENABLED" : "DISABLED");
        if (smartLight.isLightSensorBypassed()) {
          displayManager.showMessage("Light Bypass: ON", 2000);
          Serial.println("*** Testing mode: LED control based on IMU ONLY ***");
        } else {
          displayManager.showMessage("Light Bypass: OFF", 2000);
        }
        DebugHelper::printStatistics(motionDetector);
        DebugHelper::printLightStatus(lightSensor);
      }
      lastButtonPress = millis();
    } else if (digitalRead(BTN_C) == LOW) {
      btnPressed = true;
      if (!displayManager.isDisplayOn()) {
        // Just wake display, don't execute action
        displayManager.wakeDisplay();
        Serial.println("[BTN] Display woken by GREEN button");
      } else {
        // Display is on, execute action
        Serial.println("\n[GREEN BTN] Testing LED...");
        displayManager.showMessage("Testing LED...", 2000);
        DebugHelper::testLED(ledController);
        displayManager.showMessage("Test Complete!", 2000);
      }
      lastButtonPress = millis();
    }
  }
  
  // Update RGB LED based on motion (this should always reflect motion state)
  // Use currentRgbBrightness instead of RGB_BRIGHTNESS constant
  if (isMoving) {
    neopixelWrite(RGB_BUILTIN, 0, currentRgbBrightness, 0); // Green = moving
  } else {
    neopixelWrite(RGB_BUILTIN, currentRgbBrightness, 0, 0); // Red = stationary
  }
  
  delay(20); // ~50 Hz loop rate
}



