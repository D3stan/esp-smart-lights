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

// Corrected Configuration
qmi8658_cfg_t qmi8658_cfg = {
  // 1. Enable both sensors (6-Axis)
  .qmi8658_mode = qmi8658_mode_dual, // Check your .h for the exact enum name (e.g. qmi8658_mode_dual)
  
  // 2. Use valid Scale
  .acc_scale = acc_scale_4g,
  
  // 3. Use a valid ODR (1000Hz is safe for almost all modes)
  .acc_odr = acc_odr_1000, 
  
  .gyro_scale = gyro_scale_256dps,
  .gyro_odr = gyro_odr_1000,
};
qmi_data_t data;

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

}

void loop() {
  // handle button-driven neopixel colors
  if (digitalRead(BTN_R) == LOW) {
    neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0); 
    Serial.println("RED");
  } else if (digitalRead(BTN_L) == LOW) {
    neopixelWrite(RGB_BUILTIN, 0, 0, RGB_BRIGHTNESS); 
    Serial.println("BLUE");
  } else if (digitalRead(BTN_C) == LOW) {
    neopixelWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, 0); 
    Serial.println("GREEN - Testing register read");
    
    // Read raw register values to diagnose the issue
    Wire.beginTransmission(QMI_ADRESS);
    Wire.write(0x35); // ACC_X_L register
    Wire.endTransmission(false);
    Wire.requestFrom(QMI_ADRESS, 6); // Read 6 bytes (X, Y, Z - low and high)
    
    if (Wire.available() >= 6) {
      uint8_t xl = Wire.read();
      uint8_t xh = Wire.read();
      uint8_t yl = Wire.read();
      uint8_t yh = Wire.read();
      uint8_t zl = Wire.read();
      uint8_t zh = Wire.read();
      
      int16_t x = (int16_t)((xh << 8) | xl);
      int16_t y = (int16_t)((yh << 8) | yl);
      int16_t z = (int16_t)((zh << 8) | zl);
      
      Serial.print("Raw values: X=");
      Serial.print(x);
      Serial.print(" Y=");
      Serial.print(y);
      Serial.print(" Z=");
      Serial.println(z);
    }
    Wire.endTransmission();
  }
  
  // Read IMU data
  qmi8658c.read(&data);

  // Print as space-separated values for Serial Plotter
  // Accelerometer (g)
  Serial.print("AccX:");
  Serial.print(data.acc_xyz.x, 3);
  Serial.print(" AccY:");
  Serial.print(data.acc_xyz.y, 3);
  Serial.print(" AccZ:");
  Serial.print(data.acc_xyz.z, 3);
  
  // Gyroscope (dps)
  Serial.print(" GyroX:");
  Serial.print(data.gyro_xyz.x, 3);
  Serial.print(" GyroY:");
  Serial.print(data.gyro_xyz.y, 3);
  Serial.print(" GyroZ:");
  Serial.print(data.gyro_xyz.z, 3);
  
  // Temperature (°C)
  Serial.print(" Temp:");
  Serial.println(data.temperature, 2);

  // Short delay controls plot sample rate (adjust 10-50 ms as needed)
  delay(50);
}
