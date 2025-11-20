#ifndef MOTION_DETECTOR_H
#define MOTION_DETECTOR_H

#include "Qmi8658c.h"

class MotionDetector {
public:
    // Constructor
    MotionDetector(Qmi8658c* imu);
    
    // Initialization
    bool begin(qmi8658_cfg_t* config);
    
    // Calibration
    void calibrate(int samples = 100);
    bool isCalibrated() const { return _isCalibrated; }
    
    // Motion detection
    bool detectMotion();
    bool isMoving() const { return _isMoving; }
    
    // Threshold setters
    void setAccThreshold(float threshold) { _accMotionThreshold = threshold; }
    void setGyroThreshold(float threshold) { _gyroMotionThreshold = threshold; }
    void setMotionWindowMs(unsigned long ms) { _motionWindowMs = ms; }
    void setMotionStopDelayMs(unsigned long ms) { _motionStopDelayMs = ms; }
    void setMotionPulseCount(int count) { _motionPulseCount = count; }
    
    // Threshold getters
    float getAccThreshold() const { return _accMotionThreshold; }
    float getGyroThreshold() const { return _gyroMotionThreshold; }
    unsigned long getMotionWindowMs() const { return _motionWindowMs; }
    unsigned long getMotionStopDelayMs() const { return _motionStopDelayMs; }
    int getMotionPulseCount() const { return _motionPulseCount; }
    
    // Statistics
    float getMaxAccDeviation() const { return _maxAccDeviation; }
    float getMaxGyroDeviation() const { return _maxGyroDeviation; }
    void resetStatistics();
    
    // Current deviation values
    float getCurrentAccDeviation() const;
    float getCurrentGyroDeviation() const;
    
    // Baseline values
    void getAccBaseline(float& x, float& y, float& z) const;
    void getGyroBaseline(float& x, float& y, float& z) const;
    
    // Current pulse count
    int getCurrentPulseCount() const { return _motionPulseCounter; }

private:
    // IMU reference
    Qmi8658c* _imu;
    qmi_data_t _data;
    
    // Tunable thresholds
    float _accMotionThreshold;
    float _gyroMotionThreshold;
    unsigned long _motionWindowMs;
    unsigned long _motionStopDelayMs;
    int _motionPulseCount;
    
    // Baseline calibration
    float _accBaselineX, _accBaselineY, _accBaselineZ;
    float _gyroBaselineX, _gyroBaselineY, _gyroBaselineZ;
    bool _isCalibrated;
    
    // Motion state
    bool _isMoving;
    unsigned long _lastMotionTime;
    unsigned long _motionWindowStart;
    int _motionPulseCounter;
    unsigned long _lastPulseTime;
    
    // Statistics
    float _maxAccDeviation;
    float _maxGyroDeviation;
    
    // Helper methods
    float calculateAccDeviation() const;
    float calculateGyroDeviation() const;
};

#endif // MOTION_DETECTOR_H
