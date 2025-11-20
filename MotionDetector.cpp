#include "MotionDetector.h"
#include <Arduino.h>
#include <math.h>

MotionDetector::MotionDetector(Qmi8658c* imu) 
    : _imu(imu),
      _accMotionThreshold(0.10),
      _gyroMotionThreshold(5.0),
      _motionWindowMs(500),
      _motionStopDelayMs(1000),
      _motionPulseCount(3),
      _accBaselineX(0), _accBaselineY(0), _accBaselineZ(0),
      _gyroBaselineX(0), _gyroBaselineY(0), _gyroBaselineZ(0),
      _isCalibrated(false),
      _isMoving(false),
      _lastMotionTime(0),
      _motionWindowStart(0),
      _motionPulseCounter(0),
      _lastPulseTime(0),
      _maxAccDeviation(0),
      _maxGyroDeviation(0) {
}

bool MotionDetector::begin(qmi8658_cfg_t* config) {
    qmi8658_result_t result = _imu->open(config);
    
    if (result != qmi8658_result_open_success) {
        return false;
    }
    
    // Wait for sensor to stabilize
    delay(100);
    
    // Perform dummy reads to flush initial values
    for(int i = 0; i < 5; i++) {
        _imu->read(&_data);
        delay(20);
    }
    
    return true;
}

void MotionDetector::calibrate(int samples) {
    float accSumX = 0, accSumY = 0, accSumZ = 0;
    float gyroSumX = 0, gyroSumY = 0, gyroSumZ = 0;
    
    for (int i = 0; i < samples; i++) {
        _imu->read(&_data);
        accSumX += _data.acc_xyz.x;
        accSumY += _data.acc_xyz.y;
        accSumZ += _data.acc_xyz.z;
        gyroSumX += _data.gyro_xyz.x;
        gyroSumY += _data.gyro_xyz.y;
        gyroSumZ += _data.gyro_xyz.z;
        delay(10);
    }
    
    _accBaselineX = accSumX / samples;
    _accBaselineY = accSumY / samples;
    _accBaselineZ = accSumZ / samples;
    _gyroBaselineX = gyroSumX / samples;
    _gyroBaselineY = gyroSumY / samples;
    _gyroBaselineZ = gyroSumZ / samples;
    
    _isCalibrated = true;
    _maxAccDeviation = 0;
    _maxGyroDeviation = 0;
    _motionPulseCounter = 0;
    _motionWindowStart = 0;
}

bool MotionDetector::detectMotion() {
    if (!_isCalibrated) return false;
    
    // Read current IMU data
    _imu->read(&_data);
    
    // Calculate deviations
    float accTotalDev = calculateAccDeviation();
    float gyroTotalDev = calculateGyroDeviation();
    
    // Track maximum deviations for tuning
    if (accTotalDev > _maxAccDeviation) _maxAccDeviation = accTotalDev;
    if (gyroTotalDev > _maxGyroDeviation) _maxGyroDeviation = gyroTotalDev;
    
    // Check if motion exceeds threshold
    bool currentMotion = (accTotalDev > _accMotionThreshold) || (gyroTotalDev > _gyroMotionThreshold);
    
    unsigned long now = millis();
    
    if (currentMotion) {
        _lastMotionTime = now;
        
        // Start a new motion detection window if needed
        if (_motionWindowStart == 0) {
            _motionWindowStart = now;
            _motionPulseCounter = 1;
        } else {
            // Check if we're still within the detection window
            if (now - _motionWindowStart <= _motionWindowMs) {
                // Count this pulse (but avoid counting continuously - needs gap)
                if (now - _lastPulseTime > 50) { // At least 50ms between pulses
                    _motionPulseCounter++;
                    _lastPulseTime = now;
                }
            } else {
                // Window expired, restart
                _motionWindowStart = now;
                _motionPulseCounter = 1;
            }
        }
        
        // If we've detected enough motion pulses, declare moving
        if (_motionPulseCounter >= _motionPulseCount) {
            _isMoving = true;
        }
    } else {
        // No motion detected right now
        // Check if motion window has expired without enough pulses
        if (!_isMoving && _motionWindowStart != 0 && (now - _motionWindowStart > _motionWindowMs)) {
            // Reset if we didn't get enough pulses
            _motionWindowStart = 0;
            _motionPulseCounter = 0;
        }
        
        // If already moving, wait for stop delay
        if (_isMoving && (now - _lastMotionTime > _motionStopDelayMs)) {
            _isMoving = false;
            _motionWindowStart = 0;
            _motionPulseCounter = 0;
        }
    }
    
    return _isMoving;
}

void MotionDetector::resetStatistics() {
    _maxAccDeviation = 0;
    _maxGyroDeviation = 0;
}

float MotionDetector::calculateAccDeviation() const {
    float devX = abs(_data.acc_xyz.x - _accBaselineX);
    float devY = abs(_data.acc_xyz.y - _accBaselineY);
    float devZ = abs(_data.acc_xyz.z - _accBaselineZ);
    
    return sqrt(devX*devX + devY*devY + devZ*devZ);
}

float MotionDetector::calculateGyroDeviation() const {
    float devX = abs(_data.gyro_xyz.x - _gyroBaselineX);
    float devY = abs(_data.gyro_xyz.y - _gyroBaselineY);
    float devZ = abs(_data.gyro_xyz.z - _gyroBaselineZ);
    
    return sqrt(devX*devX + devY*devY + devZ*devZ);
}

float MotionDetector::getCurrentAccDeviation() const {
    if (!_isCalibrated) return 0;
    return calculateAccDeviation();
}

float MotionDetector::getCurrentGyroDeviation() const {
    if (!_isCalibrated) return 0;
    return calculateGyroDeviation();
}

void MotionDetector::getAccBaseline(float& x, float& y, float& z) const {
    x = _accBaselineX;
    y = _accBaselineY;
    z = _accBaselineZ;
}

void MotionDetector::getGyroBaseline(float& x, float& y, float& z) const {
    x = _gyroBaselineX;
    y = _gyroBaselineY;
    z = _gyroBaselineZ;
}
