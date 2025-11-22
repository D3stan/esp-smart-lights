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

// Smart light controller instance (main logic)
SmartLightController smartLight(motionDetector, lightSensor, ledController);

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
	
	// Draw the bitmap at position (0, 0)
	tft.drawBitmap(0, 0, myBitmap, 128, 128, ST77XX_BLACK); // Change color as needed

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
	printCalibrationValues();
	printInstructions();
	
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
	smartLight.begin(LED_SHUTOFF_DELAY_MS); // 30 seconds shutoff delay
    smartLight.setLightSensorBypass(true);
	Serial.println("Smart Light Controller initialized");
	Serial.print("Auto Mode: "); Serial.println(smartLight.isAutoModeEnabled() ? "ENABLED" : "DISABLED");
	Serial.print("Shutoff Delay: "); Serial.print(smartLight.getShutoffDelay() / 1000); Serial.println(" seconds");
	Serial.println("Logic: LED ON when (Night AND Movement)");
	Serial.println("=========================================================\n");

}

void loop() {
  // Check for serial commands
  handleSerialCommands();
  
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
  
  // Button controls (check without blocking LED updates)
  static unsigned long lastButtonPress = 0;
  if (millis() - lastButtonPress > 500) { // Debounce buttons
    if (digitalRead(BTN_R) == LOW) {
      Serial.println("\n[RED BTN] Recalibrating IMU...");
      motionDetector.calibrate();
      printCalibrationValues();
      lastButtonPress = millis();
    } else if (digitalRead(BTN_L) == LOW) {
      Serial.println("\n[BLUE BTN] Printing statistics...");
      printStatistics();
      printLightStatus();
      bool currentBypass = smartLight.isLightSensorBypassed();
      smartLight.setLightSensorBypass(!currentBypass);
      Serial.print("Light sensor bypass: "); 
      Serial.println(smartLight.isLightSensorBypassed() ? "ENABLED (always night)" : "DISABLED (normal)");
      if (smartLight.isLightSensorBypassed()) {
        Serial.println("*** Testing mode: LED control based on IMU ONLY ***");
      }
      lastButtonPress = millis();
    } else if (digitalRead(BTN_C) == LOW) {
      Serial.println("\n[GREEN BTN] Testing LED...");
      testLED();
      lastButtonPress = millis();
    }
  }
  
  // Update RGB LED based on motion (this should always reflect motion state)
  if (isMoving) {
    neopixelWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, 0); // Green = moving
  } else {
    neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0); // Red = stationary
  }
  
  // Print motion status (every 100ms to avoid flooding)
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 100) {
    printMotionStatus();
    lastPrint = millis();
  }
  
  // Print smart light status (every 1000ms)
  static unsigned long lastSmartPrint = 0;
  if (millis() - lastSmartPrint > 1000) {
    printSmartLightStatus();
    lastSmartPrint = millis();
  }
  
  delay(20); // ~50 Hz loop rate
}

// ========== PRINTING FUNCTIONS ==========
void printMotionStatus() {
  if (!motionDetector.isCalibrated()) return;
  
  float acc_dev = motionDetector.getCurrentAccDeviation();
  float gyro_dev = motionDetector.getCurrentGyroDeviation();
  
  // For Serial Plotter (comment out Serial.print text to use plotter)
  Serial.print("AccDev:");
  Serial.print(acc_dev, 4);
  Serial.print(" AccThreshold:");
  Serial.print(motionDetector.getAccThreshold(), 4);
  Serial.print(" GyroDev:");
  Serial.print(gyro_dev, 2);
  Serial.print(" GyroThreshold:");
  Serial.print(motionDetector.getGyroThreshold(), 2);
  Serial.print(" Moving:");
  Serial.println(motionDetector.isMoving() ? 1 : 0);
}

void printCalibrationValues() {
  float acc_x, acc_y, acc_z;
  float gyro_x, gyro_y, gyro_z;
  
  motionDetector.getAccBaseline(acc_x, acc_y, acc_z);
  motionDetector.getGyroBaseline(gyro_x, gyro_y, gyro_z);
  
  Serial.println("\n--- Baseline Values (at rest) ---");
  Serial.print("Acc: X="); Serial.print(acc_x, 4);
  Serial.print(" Y="); Serial.print(acc_y, 4);
  Serial.print(" Z="); Serial.println(acc_z, 4);
  Serial.print("Gyro: X="); Serial.print(gyro_x, 2);
  Serial.print(" Y="); Serial.print(gyro_y, 2);
  Serial.print(" Z="); Serial.println(gyro_z, 2);
  Serial.println("---------------------------------\n");
}

void printStatistics() {
  Serial.println("\n========== MOTION STATISTICS ==========");
  Serial.print("Max Acc Deviation: "); Serial.print(motionDetector.getMaxAccDeviation(), 4); Serial.println(" g");
  Serial.print("Max Gyro Deviation: "); Serial.print(motionDetector.getMaxGyroDeviation(), 2); Serial.println(" dps");
  Serial.println("\nCurrent Thresholds:");
  Serial.print("  ACC_MOTION_THRESHOLD = "); Serial.print(motionDetector.getAccThreshold(), 4); Serial.println(" g");
  Serial.print("  GYRO_MOTION_THRESHOLD = "); Serial.print(motionDetector.getGyroThreshold(), 2); Serial.println(" dps");
  Serial.print("  MOTION_WINDOW_MS = "); Serial.print(motionDetector.getMotionWindowMs()); Serial.println(" ms");
  Serial.print("  MOTION_PULSE_COUNT = "); Serial.print(motionDetector.getMotionPulseCount()); Serial.println(" pulses");
  Serial.print("  Current Pulse Count = "); Serial.println(motionDetector.getCurrentPulseCount());
  Serial.println("========================================\n");
}

void printInstructions() {
  Serial.println("\n====== IMU MOTION DETECTION READY ======");
  Serial.println("LED: RED=Stationary, GREEN=Moving");
  Serial.println("\nButtons:");
  Serial.println("  RED (BTN_R): Recalibrate baseline");
  Serial.println("  BLUE (BTN_L): Show statistics & light status");
  Serial.println("  GREEN (BTN_C): Test LED strip");
  Serial.println("\nSerial Commands:");
  Serial.println("  a<value> - Set acc threshold (e.g., a0.05)");
  Serial.println("  g<value> - Set gyro threshold (e.g., g3.0)");
  Serial.println("  w<value> - Set motion window ms (e.g., w300)");
  Serial.println("  p<value> - Set pulse count needed (e.g., p2)");
  Serial.println("  l<value> - Set night threshold lux (e.g., l10.0)");
  Serial.println("  L<value> - Set LED brightness (e.g., L255) [manual mode]");
  Serial.println("  d<value> - Set shutoff delay seconds (e.g., d30)");
  Serial.println("  A - Toggle auto mode (enable/disable)");
  Serial.println("  B - Toggle light sensor bypass (test with IMU only)");
  Serial.println("  O - Force LED ON (manual override)");
  Serial.println("  F - Force LED OFF (manual override)");
  Serial.println("  R - Return to auto mode");
  Serial.println("  s - Show statistics");
  Serial.println("  c - Recalibrate");
  Serial.println("  h - Show this help");
  Serial.println("========================================\n");
}

void printLightStatus() {
  Serial.println("\n--- Light Sensor Status ---");
  Serial.print("Current Lux: "); Serial.print(lightSensor.getLastLux(), 1); Serial.println(" lux");
  Serial.print("Night Threshold: "); Serial.print(lightSensor.getNightThreshold(), 1); Serial.println(" lux");
  Serial.print("Is Night: "); Serial.println(lightSensor.isNight() ? "YES" : "NO");
  Serial.print("Sensor Ready: "); Serial.println(lightSensor.isReady() ? "YES" : "NO");
  Serial.println("---------------------------\n");
}

void testLED() {
  Serial.println("LED Test Sequence:");
  Serial.println("  25% brightness...");
  ledController.setBrightness(64);
  delay(500);
  Serial.println("  50% brightness...");
  ledController.setBrightness(128);
  delay(500);
  Serial.println("  100% brightness...");
  ledController.setBrightness(255);
  delay(500);
  Serial.println("  Fading off...");
  ledController.fadeTo(0, 1000);
  Serial.println("LED Test Complete");
}

void printSmartLightStatus() {
  Serial.print("[SMART] State:");
  Serial.print(smartLight.getStateString());
  Serial.print(" | AutoMode:");
  Serial.print(smartLight.isAutoModeEnabled() ? "ON" : "OFF");
  
  if (smartLight.isLightSensorBypassed()) {
    Serial.print(" | Night:BYPASS");
  } else {
    Serial.print(" | Night:");
    Serial.print(lightSensor.isNight() ? "YES" : "NO");
  }
  
  Serial.print(" | Motion:");
  Serial.print(motionDetector.isMoving() ? "YES" : "NO");
  Serial.print(" | LED:");
  Serial.print(ledController.isOn() ? "ON" : "OFF");
  
  if (smartLight.isInCountdown()) {
    Serial.print(" | Countdown:");
    Serial.print(smartLight.getCountdownRemaining() / 1000);
    Serial.print("s");
  }
  
  Serial.println();
}

void handleSerialCommands() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    switch(cmd) {
      case 'a': // Accelerometer threshold
        {
          float threshold = Serial.parseFloat();
          motionDetector.setAccThreshold(threshold);
          Serial.print("ACC_MOTION_THRESHOLD set to: "); Serial.println(threshold, 4);
        }
        break;
      case 'g': // Gyroscope threshold
        {
          float threshold = Serial.parseFloat();
          motionDetector.setGyroThreshold(threshold);
          Serial.print("GYRO_MOTION_THRESHOLD set to: "); Serial.println(threshold, 2);
        }
        break;
      case 'w': // Motion window time
        {
          unsigned long windowMs = Serial.parseInt();
          motionDetector.setMotionWindowMs(windowMs);
          Serial.print("MOTION_WINDOW_MS set to: "); Serial.println(windowMs);
        }
        break;
      case 'p': // Pulse count
        {
          int pulseCount = Serial.parseInt();
          motionDetector.setMotionPulseCount(pulseCount);
          Serial.print("MOTION_PULSE_COUNT set to: "); Serial.println(pulseCount);
        }
        break;
      case 'l': // Light threshold
        {
          float threshold = Serial.parseFloat();
          lightSensor.setNightThreshold(threshold);
          Serial.print("NIGHT_THRESHOLD set to: "); Serial.print(threshold, 1); Serial.println(" lux");
        }
        break;
      case 'L': // LED brightness
        {
          int brightness = Serial.parseInt();
          if (brightness >= 0 && brightness <= 255) {
            ledController.setBrightness(brightness);
            Serial.print("LED brightness set to: "); Serial.println(brightness);
          } else {
            Serial.println("ERROR: Brightness must be 0-255");
          }
        }
        break;
      case 's': // Show statistics
        printStatistics();
        printLightStatus();
        break;
      case 'c': // Calibrate
        Serial.println("Recalibrating... keep still!");
        delay(500);
        motionDetector.calibrate();
        printCalibrationValues();
        break;
      case 'd': // Shutoff delay
        {
          unsigned long delaySec = Serial.parseInt();
          if (delaySec > 0) {
            smartLight.setShutoffDelay(delaySec * 1000);
            Serial.print("Shutoff delay set to: "); Serial.print(delaySec); Serial.println(" seconds");
          } else {
            Serial.println("ERROR: Delay must be > 0");
          }
        }
        break;
      case 'A': // Toggle auto mode
        {
          bool currentAuto = smartLight.isAutoModeEnabled();
          smartLight.setAutoMode(!currentAuto);
          Serial.print("Auto mode: "); Serial.println(smartLight.isAutoModeEnabled() ? "ENABLED" : "DISABLED");
        }
        break;
      case 'B': // Toggle light sensor bypass
        {
          bool currentBypass = smartLight.isLightSensorBypassed();
          smartLight.setLightSensorBypass(!currentBypass);
          Serial.print("Light sensor bypass: "); 
          Serial.println(smartLight.isLightSensorBypassed() ? "ENABLED (always night)" : "DISABLED (normal)");
          if (smartLight.isLightSensorBypassed()) {
            Serial.println("*** Testing mode: LED control based on IMU ONLY ***");
          }
        }
        break;
      case 'O': // Force LED ON
        Serial.println("Forcing LED ON (manual override)");
        smartLight.forceOn(255);
        Serial.println("Use 'R' to return to auto mode");
        break;
      case 'F': // Force LED OFF
        Serial.println("Forcing LED OFF (manual override)");
        smartLight.forceOff();
        Serial.println("Use 'R' to return to auto mode");
        break;
      case 'R': // Return to auto
        Serial.println("Returning to automatic control mode");
        smartLight.returnToAuto();
        break;
      case 'h': // Help
        printInstructions();
        break;
    }
    // Clear remaining buffer
    while(Serial.available()) Serial.read();
  }
}
