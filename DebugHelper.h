#ifndef DEBUG_HELPER_H
#define DEBUG_HELPER_H

#include <Arduino.h>
#include "MotionDetector.h"
#include "LightSensor.h"
#include "LEDController.h"
#include "SmartLightController.h"
#include "WiFiManager.h"
#include "EventLogger.h"
#include "OTAManager.h"

// ========== DEBUG AND HELPER FUNCTIONS ==========
// These functions are used for debugging and displaying information
// on the Serial Monitor during development and testing.

namespace DebugHelper {

  // Print IMU calibration baseline values
  void printCalibrationValues(MotionDetector& motionDetector) {
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

  // Print motion detection statistics
  void printStatistics(MotionDetector& motionDetector) {
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

  // Print light sensor status
  void printLightStatus(LightSensor& lightSensor) {
    Serial.println("\n--- Light Sensor Status ---");
    Serial.print("Current Lux: "); Serial.print(lightSensor.getLastLux(), 1); Serial.println(" lux");
    Serial.print("Night Threshold: "); Serial.print(lightSensor.getNightThreshold(), 1); Serial.println(" lux");
    Serial.print("Is Night: "); Serial.println(lightSensor.isNight() ? "YES" : "NO");
    Serial.print("Sensor Ready: "); Serial.println(lightSensor.isReady() ? "YES" : "NO");
    Serial.println("---------------------------\n");
  }

  // Test LED strip with brightness sequence
  void testLED(LEDController& ledController) {
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

  // Print WiFi status details
  void printWiFiStatus(WiFiManager& wifiManager) {
    Serial.println("\n========== WIFI STATUS ==========");
    Serial.print("State: ");
    Serial.println(wifiManager.getStateString());
    Serial.print("SSID: ");
    Serial.println(wifiManager.getSSID());
    Serial.print("IP Address: ");
    Serial.println(wifiManager.getIPAddress());
    Serial.print("Hostname: ");
    Serial.println(wifiManager.getHostname());
    
    if (wifiManager.isConnected()) {
      Serial.print("Signal Strength (RSSI): ");
      Serial.print(wifiManager.getRSSI());
      Serial.println(" dBm");
    }
    
    if (!wifiManager.getLastError().isEmpty()) {
      Serial.print("Last Error: ");
      Serial.println(wifiManager.getLastError());
    }
    
    Serial.print("Retry Interval: ");
    Serial.print(wifiManager.getRetryInterval() / 1000);
    Serial.println(" seconds");
    
    if (!wifiManager.isConnected() && !wifiManager.isAPMode()) {
      unsigned long remaining = wifiManager.getReconnectTimeRemaining();
      if (remaining > 0) {
        Serial.print("Next reconnection attempt in: ");
        Serial.print(remaining / 1000);
        Serial.println(" seconds");
      }
    }
    
    Serial.println("=================================\n");
  }

  // Print event logs
  void printEventLogs(EventLogger& eventLogger) {
    Serial.println("\n========== EVENT LOGS ==========");
    Serial.print("Total events: "); Serial.println(eventLogger.getEventCount());
    Serial.print("Events today: "); Serial.println(eventLogger.getTodayEventCount());
    Serial.print("Events last 24h: "); Serial.println(eventLogger.getEventsLastHours(24));
    Serial.println("\nRecent events (newest first):");
    Serial.println("--------------------------------");
    
    uint16_t count = eventLogger.getEventCount();
    uint16_t maxShow = count > 20 ? 20 : count; // Show max 20 events
    
    for (uint16_t i = 0; i < maxShow; i++) {
      const EventLogger::LogEntry* entry = eventLogger.getEvent(i);
      if (!entry) continue;
      
      // Format timestamp
      time_t timestamp = entry->timestamp;
      struct tm* timeinfo = localtime(&timestamp);
      char timeStr[20];
      strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
      
      Serial.print(timeStr);
      Serial.print(" | ");
      Serial.print(entry->ledOn ? "LED ON " : "LED OFF");
      Serial.print(" | Lux:");
      Serial.print(entry->lux, 1);
      Serial.print(" | Motion:");
      Serial.print(entry->motion ? "YES" : "NO ");
      Serial.print(" | Mode:");
      Serial.println(entry->mode);
    }
    
    if (count > maxShow) {
      Serial.print("... and "); Serial.print(count - maxShow); Serial.println(" more events");
    }
    
    Serial.println("================================\n");
  }

  // Print OTA status
  void printOTAStatus(OTAManager* otaManager, WiFiManager& wifiManager) {
    Serial.println("\n========== OTA STATUS ==========");
    
    if (!otaManager) {
      Serial.println("OTA Manager: NOT INITIALIZED");
      Serial.println("OTA will be available when WiFi connects");
    } else {
      Serial.print("State: ");
      Serial.println(otaManager->getStateString());
      
      Serial.print("Current Partition: ");
      Serial.println(otaManager->getCurrentPartition());
      
      Serial.print("Available Space: ");
      Serial.print(otaManager->getAvailableSpace());
      Serial.println(" bytes");
      
      Serial.print("Max Firmware Size: ");
      Serial.print(otaManager->getMaxFirmwareSize());
      Serial.println(" bytes");
      
      Serial.print("Can Rollback: ");
      Serial.println(otaManager->canRollback() ? "YES" : "NO");
      
      if (otaManager->getLastError() != OTAManager::OTAError::NONE) {
        Serial.print("Last Error: ");
        Serial.println(otaManager->getLastErrorString());
      }
      
      if (wifiManager.isConnected()) {
        Serial.print("\nOTA Web Interface: http://");
        Serial.print(wifiManager.getIPAddress().toString());
        Serial.println("/ota");
      }
    }
    
    Serial.println("================================\n");
  }

} // namespace DebugHelper

#endif // DEBUG_HELPER_H
