#include <Adafruit_GFX.h>        // Core graphics library
#include <Adafruit_ST7735.h>     // Library for the ST7735 TFT screen
#include <Adafruit_NeoPixel.h>   // RGB Led
#include <SPI.h>

#include "bitmap.h"
#include "Qmi8658c.h"
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
qmi_data_t data;

// ========== MOTION DETECTION SETTINGS ==========
// Tunable thresholds - adjust these based on testing
float ACC_MOTION_THRESHOLD = 0.10;    // g units (start with 0.10, lower = more sensitive)
float GYRO_MOTION_THRESHOLD = 5.0;    // degrees/sec (start with 5.0, lower = more sensitive)
unsigned long MOTION_WINDOW_MS = 500; // Time window to look for motion pulses (milliseconds)
unsigned long MOTION_STOP_DELAY_MS = 1000; // Time to wait before considering stopped (milliseconds)
int MOTION_PULSE_COUNT = 3;           // Number of threshold crossings needed to confirm motion

// Baseline calibration (at rest values)
float acc_baseline_x = 0, acc_baseline_y = 0, acc_baseline_z = 0;
float gyro_baseline_x = 0, gyro_baseline_y = 0, gyro_baseline_z = 0;
bool isCalibrated = false;

// Motion state
bool isMoving = false;
unsigned long lastMotionTime = 0;
unsigned long motionWindowStart = 0;
int motionPulseCounter = 0;

// Statistics for tuning
float max_acc_deviation = 0;
float max_gyro_deviation = 0;

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

  qmi8658_result_t qmi8658_result = qmi8658c.open(&qmi8658_cfg);
	delay(1000);
	Serial.print("open result : ");
	Serial.println(qmi8658c.resultToString(qmi8658_result));
	Serial.print("deviceID = 0x");
	Serial.println(qmi8658c.deviceID, HEX);
	Serial.print("deviceRevisionID = 0x");
	Serial.println(qmi8658c.deviceRevisionID, HEX);
	
	// Check if device ID is valid (should be 0x05 for QMI8658)
	if (qmi8658c.deviceID != 0x05) {
	  Serial.println("WARNING: Device ID mismatch! Expected 0x05");
	  Serial.println("Check I2C connections and address");
	}
	
	// Wait for sensor to stabilize after configuration
	delay(100);
	
	// Perform a few dummy reads to flush initial values
	for(int i = 0; i < 5; i++) {
	  qmi8658c.read(&data);
	  delay(20);
	}
	
	Serial.println("IMU initialized and ready");
	
	// Perform baseline calibration
	Serial.println("\n========== CALIBRATING IMU ==========");
	Serial.println("Keep the device STILL for 3 seconds...");
	delay(1000);
	
	calibrateIMU();
	Serial.println("Calibration complete!");
	Serial.println("====================================\n");
	printCalibrationValues();
	printInstructions();

}

void loop() {
  // Check for serial commands
  handleSerialCommands();
  
  // Read IMU data
  qmi8658c.read(&data);
  
  // Detect motion
  bool motionDetected = detectMotion();
  
  // Button controls (check without blocking LED updates)
  static unsigned long lastButtonPress = 0;
  if (millis() - lastButtonPress > 500) { // Debounce buttons
    if (digitalRead(BTN_R) == LOW) {
      Serial.println("\n[RED BTN] Recalibrating IMU...");
      calibrateIMU();
      printCalibrationValues();
      lastButtonPress = millis();
    } else if (digitalRead(BTN_L) == LOW) {
      Serial.println("\n[BLUE BTN] Printing statistics...");
      printStatistics();
      lastButtonPress = millis();
    } else if (digitalRead(BTN_C) == LOW) {
      Serial.println("\n[GREEN BTN] Resetting statistics...");
      max_acc_deviation = 0;
      max_gyro_deviation = 0;
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

// ========== CALIBRATION FUNCTION ==========
void calibrateIMU() {
  const int samples = 100;
  float acc_sum_x = 0, acc_sum_y = 0, acc_sum_z = 0;
  float gyro_sum_x = 0, gyro_sum_y = 0, gyro_sum_z = 0;
  
  for (int i = 0; i < samples; i++) {
    qmi8658c.read(&data);
    acc_sum_x += data.acc_xyz.x;
    acc_sum_y += data.acc_xyz.y;
    acc_sum_z += data.acc_xyz.z;
    gyro_sum_x += data.gyro_xyz.x;
    gyro_sum_y += data.gyro_xyz.y;
    gyro_sum_z += data.gyro_xyz.z;
    delay(10);
  }
  
  acc_baseline_x = acc_sum_x / samples;
  acc_baseline_y = acc_sum_y / samples;
  acc_baseline_z = acc_sum_z / samples;
  gyro_baseline_x = gyro_sum_x / samples;
  gyro_baseline_y = gyro_sum_y / samples;
  gyro_baseline_z = gyro_sum_z / samples;
  
  isCalibrated = true;
  max_acc_deviation = 0;
  max_gyro_deviation = 0;
  motionPulseCounter = 0;
  motionWindowStart = 0;
}

// ========== MOTION DETECTION FUNCTION ==========
bool detectMotion() {
  if (!isCalibrated) return false;
  
  // Calculate deviations from baseline
  float acc_dev_x = abs(data.acc_xyz.x - acc_baseline_x);
  float acc_dev_y = abs(data.acc_xyz.y - acc_baseline_y);
  float acc_dev_z = abs(data.acc_xyz.z - acc_baseline_z);
  
  float gyro_dev_x = abs(data.gyro_xyz.x - gyro_baseline_x);
  float gyro_dev_y = abs(data.gyro_xyz.y - gyro_baseline_y);
  float gyro_dev_z = abs(data.gyro_xyz.z - gyro_baseline_z);
  
  // Calculate total deviations
  float acc_total_dev = sqrt(acc_dev_x*acc_dev_x + acc_dev_y*acc_dev_y + acc_dev_z*acc_dev_z);
  float gyro_total_dev = sqrt(gyro_dev_x*gyro_dev_x + gyro_dev_y*gyro_dev_y + gyro_dev_z*gyro_dev_z);
  
  // Track maximum deviations for tuning
  if (acc_total_dev > max_acc_deviation) max_acc_deviation = acc_total_dev;
  if (gyro_total_dev > max_gyro_deviation) max_gyro_deviation = gyro_total_dev;
  
  // Check if motion exceeds threshold
  bool currentMotion = (acc_total_dev > ACC_MOTION_THRESHOLD) || (gyro_total_dev > GYRO_MOTION_THRESHOLD);
  
  unsigned long now = millis();
  
  if (currentMotion) {
    lastMotionTime = now;
    
    // Start a new motion detection window if needed
    if (motionWindowStart == 0) {
      motionWindowStart = now;
      motionPulseCounter = 1;
    } else {
      // Check if we're still within the detection window
      if (now - motionWindowStart <= MOTION_WINDOW_MS) {
        // Count this pulse (but avoid counting continuously - needs gap)
        static unsigned long lastPulseTime = 0;
        if (now - lastPulseTime > 50) { // At least 50ms between pulses
          motionPulseCounter++;
          lastPulseTime = now;
        }
      } else {
        // Window expired, restart
        motionWindowStart = now;
        motionPulseCounter = 1;
      }
    }
    
    // If we've detected enough motion pulses, declare moving
    if (motionPulseCounter >= MOTION_PULSE_COUNT) {
      isMoving = true;
    }
  } else {
    // No motion detected right now
    // Check if motion window has expired without enough pulses
    if (!isMoving && motionWindowStart != 0 && (now - motionWindowStart > MOTION_WINDOW_MS)) {
      // Reset if we didn't get enough pulses
      motionWindowStart = 0;
      motionPulseCounter = 0;
    }
    
    // If already moving, wait for stop delay
    if (isMoving && (now - lastMotionTime > MOTION_STOP_DELAY_MS)) {
      isMoving = false;
      motionWindowStart = 0;
      motionPulseCounter = 0;
    }
  }
  
  return isMoving;
}

// ========== PRINTING FUNCTIONS ==========
void printMotionStatus() {
  if (!isCalibrated) return;
  
  // Calculate current deviations
  float acc_dev = sqrt(
    pow(data.acc_xyz.x - acc_baseline_x, 2) +
    pow(data.acc_xyz.y - acc_baseline_y, 2) +
    pow(data.acc_xyz.z - acc_baseline_z, 2)
  );
  
  float gyro_dev = sqrt(
    pow(data.gyro_xyz.x - gyro_baseline_x, 2) +
    pow(data.gyro_xyz.y - gyro_baseline_y, 2) +
    pow(data.gyro_xyz.z - gyro_baseline_z, 2)
  );
  
  // For Serial Plotter (comment out Serial.print text to use plotter)
  Serial.print("AccDev:");
  Serial.print(acc_dev, 4);
  Serial.print(" AccThreshold:");
  Serial.print(ACC_MOTION_THRESHOLD, 4);
  Serial.print(" GyroDev:");
  Serial.print(gyro_dev, 2);
  Serial.print(" GyroThreshold:");
  Serial.print(GYRO_MOTION_THRESHOLD, 2);
  Serial.print(" Moving:");
  Serial.println(isMoving ? 1 : 0);
}

void printCalibrationValues() {
  Serial.println("\n--- Baseline Values (at rest) ---");
  Serial.print("Acc: X="); Serial.print(acc_baseline_x, 4);
  Serial.print(" Y="); Serial.print(acc_baseline_y, 4);
  Serial.print(" Z="); Serial.println(acc_baseline_z, 4);
  Serial.print("Gyro: X="); Serial.print(gyro_baseline_x, 2);
  Serial.print(" Y="); Serial.print(gyro_baseline_y, 2);
  Serial.print(" Z="); Serial.println(gyro_baseline_z, 2);
  Serial.println("---------------------------------\n");
}

void printStatistics() {
  Serial.println("\n========== MOTION STATISTICS ==========");
  Serial.print("Max Acc Deviation: "); Serial.print(max_acc_deviation, 4); Serial.println(" g");
  Serial.print("Max Gyro Deviation: "); Serial.print(max_gyro_deviation, 2); Serial.println(" dps");
  Serial.println("\nCurrent Thresholds:");
  Serial.print("  ACC_MOTION_THRESHOLD = "); Serial.print(ACC_MOTION_THRESHOLD, 4); Serial.println(" g");
  Serial.print("  GYRO_MOTION_THRESHOLD = "); Serial.print(GYRO_MOTION_THRESHOLD, 2); Serial.println(" dps");
  Serial.print("  MOTION_WINDOW_MS = "); Serial.print(MOTION_WINDOW_MS); Serial.println(" ms");
  Serial.print("  MOTION_PULSE_COUNT = "); Serial.print(MOTION_PULSE_COUNT); Serial.println(" pulses");
  Serial.print("  Current Pulse Count = "); Serial.println(motionPulseCounter);
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
        ACC_MOTION_THRESHOLD = Serial.parseFloat();
        Serial.print("ACC_MOTION_THRESHOLD set to: "); Serial.println(ACC_MOTION_THRESHOLD, 4);
        break;
      case 'g': // Gyroscope threshold
        GYRO_MOTION_THRESHOLD = Serial.parseFloat();
        Serial.print("GYRO_MOTION_THRESHOLD set to: "); Serial.println(GYRO_MOTION_THRESHOLD, 2);
        break;
      case 'w': // Motion window time
        MOTION_WINDOW_MS = Serial.parseInt();
        Serial.print("MOTION_WINDOW_MS set to: "); Serial.println(MOTION_WINDOW_MS);
        break;
      case 'p': // Pulse count
        MOTION_PULSE_COUNT = Serial.parseInt();
        Serial.print("MOTION_PULSE_COUNT set to: "); Serial.println(MOTION_PULSE_COUNT);
        break;
      case 's': // Show statistics
        printStatistics();
        break;
      case 'c': // Calibrate
        Serial.println("Recalibrating... keep still!");
        delay(500);
        calibrateIMU();
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
