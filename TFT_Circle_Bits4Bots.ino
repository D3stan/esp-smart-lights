#include <Adafruit_GFX.h>        // Core graphics library
#include <Adafruit_ST7735.h>     // Library for the ST7735 TFT screen
#include <Adafruit_NeoPixel.h>   // RGB Led
#include <SPI.h>

#include "bitmap.h"
#include "Qmi8658c.h"
#include "MotionDetector.h"
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

void setup() {

	Serial.begin(115200);

  while (!Serial) { 
    delay(10); 
  }
  
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

}

void loop() {
  // Check for serial commands
  handleSerialCommands();
  
  // Detect motion
  bool isMoving = motionDetector.detectMotion();
  
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
      lastButtonPress = millis();
    } else if (digitalRead(BTN_C) == LOW) {
      Serial.println("\n[GREEN BTN] Resetting statistics...");
      motionDetector.resetStatistics();
      lastButtonPress = millis();
    }
  }
  
  // Update LED based on motion (this should always reflect motion state)
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
  Serial.println("  BLUE (BTN_L): Show statistics");
  Serial.println("  GREEN (BTN_C): Reset statistics");
  Serial.println("\nSerial Commands:");
  Serial.println("  a<value> - Set acc threshold (e.g., a0.05)");
  Serial.println("  g<value> - Set gyro threshold (e.g., g3.0)");
  Serial.println("  w<value> - Set motion window ms (e.g., w300)");
  Serial.println("  p<value> - Set pulse count needed (e.g., p2)");
  Serial.println("  s - Show statistics");
  Serial.println("  c - Recalibrate");
  Serial.println("  h - Show this help");
  Serial.println("========================================\n");
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
      case 's': // Show statistics
        printStatistics();
        break;
      case 'c': // Calibrate
        Serial.println("Recalibrating... keep still!");
        delay(500);
        motionDetector.calibrate();
        printCalibrationValues();
        break;
      case 'h': // Help
        printInstructions();
        break;
    }
    // Clear remaining buffer
    while(Serial.available()) Serial.read();
  }
}
