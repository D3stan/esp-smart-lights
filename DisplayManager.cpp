#include "DisplayManager.h"
#include "config.h"

DisplayManager::DisplayManager(Adafruit_ST7735& tft)
    : _tft(tft)
    , _updateIntervalMs(1000)
    , _lastUpdate(0)
    , _prevWiFiState("")
    , _prevIP("")
    , _prevLux(-1.0f)
    , _prevMoving(false)
    , _prevLEDOn(false)
{
}

void DisplayManager::begin(unsigned long updateIntervalMs) {
    _updateIntervalMs = updateIntervalMs;
    _lastUpdate = 0;
    
    // Initialize backlight
    pinMode(TFT_BL, OUTPUT);
    setBacklight(true);
    
    // Clear display
    clear();
    
    // Show welcome screen
    showWelcomeScreen();
    
    Serial.println("Display Manager initialized");
}

void DisplayManager::update(
    const WiFiManager& wifiManager,
    const LightSensor& lightSensor,
    const MotionDetector& motionDetector,
    const LEDController& ledController
) {
    // Check if it's time to update
    unsigned long now = millis();
    if (now - _lastUpdate < _updateIntervalMs) {
        return;
    }
    _lastUpdate = now;
    
    // Get current values
    String currentWiFiState = wifiManager.getStateString();
    String currentIP = formatIP(wifiManager.getIPAddress());
    float currentLux = lightSensor.getLastLux();
    bool currentMoving = motionDetector.isMoving();
    bool currentLEDOn = ledController.isOn();
    
    // Update header if WiFi state or IP changed
    if (currentWiFiState != _prevWiFiState || currentIP != _prevIP) {
        drawHeader(wifiManager);
        _prevWiFiState = currentWiFiState;
        _prevIP = currentIP;
    }
    
    // Update sensors if values changed significantly
    if (abs(currentLux - _prevLux) > 1.0f || currentMoving != _prevMoving) {
        drawSensors(lightSensor, motionDetector);
        _prevLux = currentLux;
        _prevMoving = currentMoving;
    }
    
    // Update status if LED state changed
    if (currentLEDOn != _prevLEDOn) {
        drawStatus(ledController);
        _prevLEDOn = currentLEDOn;
    }
}

void DisplayManager::forceUpdate() {
    _lastUpdate = 0;
    _prevWiFiState = "";
    _prevIP = "";
    _prevLux = -1.0f;
    _prevMoving = false;
    _prevLEDOn = false;
}

void DisplayManager::showWelcomeScreen() {
    clear();
    
    _tft.setTextSize(2);
    _tft.setTextColor(ST77XX_WHITE);
    
    // Title
    _tft.setCursor(10, 30);
    _tft.println("Centralina");
    _tft.setCursor(28, 50);
    _tft.println("Luci");
    
    // Version/subtitle
    _tft.setTextSize(1);
    _tft.setCursor(15, 80);
    _tft.println("Robot Tosaerba");
    
    // Initialization message
    _tft.setTextColor(ST77XX_CYAN);
    _tft.setCursor(8, 105);
    _tft.println("Inizializzazione..");
    
    delay(2000);
    clear();
}

void DisplayManager::showError(const String& errorMsg) {
    clear();
    
    _tft.fillScreen(ST77XX_RED);
    _tft.setTextSize(1);
    _tft.setTextColor(ST77XX_WHITE);
    
    _tft.setCursor(20, 40);
    _tft.println("ERRORE:");
    
    _tft.setCursor(5, 60);
    _tft.println(errorMsg);
}

void DisplayManager::clear() {
    _tft.fillScreen(COLOR_BACKGROUND);
}

void DisplayManager::setBacklight(bool enabled) {
    digitalWrite(TFT_BL, enabled ? HIGH : LOW);
}

void DisplayManager::drawHeader(const WiFiManager& wifiManager) {
    // Clear header area
    _tft.fillRect(0, AREA_HEADER_Y, 128, AREA_HEADER_HEIGHT, COLOR_HEADER_BG);
    
    // Get WiFi info
    WiFiManager::ConnectionState state = wifiManager.getState();
    uint16_t stateColor = getWiFiStateColor(state);
    String stateText = wifiManager.getStateString();
    
    // Draw WiFi icon
    drawWiFiIcon(5, 5, stateColor);
    
    // Draw state text
    _tft.setTextSize(1);
    _tft.setTextColor(stateColor);
    _tft.setCursor(22, 7);
    
    if (state == WiFiManager::ConnectionState::CONNECTED) {
        String ssid = shortenSSID(wifiManager.getSSID(), 10);
        _tft.println(ssid);
    } else if (state == WiFiManager::ConnectionState::AP_MODE) {
        _tft.println("CONFIG MODE");
    } else {
        _tft.println(stateText);
    }
    
    // Draw IP address
    _tft.setTextColor(COLOR_TEXT);
    _tft.setCursor(5, 19);
    
    if (state == WiFiManager::ConnectionState::CONNECTED) {
        String ip = formatIP(wifiManager.getIPAddress());
        _tft.print("IP: ");
        _tft.println(ip);
    } else if (state == WiFiManager::ConnectionState::AP_MODE) {
        String apIP = formatIP(wifiManager.getIPAddress());
        _tft.print("AP: ");
        _tft.println(apIP);
    } else if (state == WiFiManager::ConnectionState::CONNECTING || 
               state == WiFiManager::ConnectionState::RECONNECTING) {
        _tft.println("Connessione...");
    } else {
        unsigned long remaining = wifiManager.getReconnectTimeRemaining();
        if (remaining > 0) {
            _tft.print("Retry: ");
            _tft.print(remaining / 1000);
            _tft.println("s");
        } else {
            _tft.println("Disconnesso");
        }
    }
}

void DisplayManager::drawSensors(const LightSensor& lightSensor, const MotionDetector& motionDetector) {
    // Clear sensors area
    _tft.fillRect(0, AREA_SENSORS_Y, 128, AREA_SENSORS_HEIGHT, COLOR_SENSOR_BG);
    
    // Draw light sensor info
    _tft.setTextSize(1);
    _tft.setTextColor(ST77XX_YELLOW);
    _tft.setCursor(5, AREA_SENSORS_Y + 5);
    _tft.println("LUCE:");
    
    _tft.setTextSize(2);
    _tft.setCursor(5, AREA_SENSORS_Y + 17);
    float lux = lightSensor.getLastLux();
    if (lux >= 0) {
        if (lux < 100) {
            _tft.print(lux, 1);
        } else {
            _tft.print((int)lux);
        }
        _tft.println(" lux");
    } else {
        _tft.println("N/A");
    }
    
    // Night indicator
    _tft.setTextSize(1);
    if (lightSensor.isNight()) {
        _tft.setTextColor(ST77XX_BLUE);
        _tft.setCursor(5, AREA_SENSORS_Y + 36);
        _tft.println("(Notte)");
    } else {
        _tft.setTextColor(ST77XX_WHITE);
        _tft.setCursor(5, AREA_SENSORS_Y + 36);
        _tft.println("(Giorno)");
    }
    
    // Draw divider line
    _tft.drawFastHLine(5, AREA_SENSORS_Y + 48, 118, COLOR_TEXT);
    
    // Draw motion detector info
    _tft.setTextSize(1);
    _tft.setTextColor(ST77XX_GREEN);
    _tft.setCursor(65, AREA_SENSORS_Y + 5);
    _tft.println("MOVIMENTO:");
    
    // Motion indicator
    bool moving = motionDetector.isMoving();
    if (moving) {
        // Draw animated motion icon
        _tft.fillCircle(95, AREA_SENSORS_Y + 25, 12, ST77XX_GREEN);
        _tft.setTextColor(ST77XX_BLACK);
        _tft.setTextSize(2);
        _tft.setCursor(90, AREA_SENSORS_Y + 19);
        _tft.println(">");
    } else {
        // Draw static icon
        _tft.drawCircle(95, AREA_SENSORS_Y + 25, 12, ST77XX_RED);
        _tft.setTextColor(ST77XX_RED);
        _tft.setTextSize(2);
        _tft.setCursor(90, AREA_SENSORS_Y + 19);
        _tft.println("-");
    }
    
    // Motion text
    _tft.setTextSize(1);
    _tft.setTextColor(moving ? ST77XX_GREEN : ST77XX_RED);
    _tft.setCursor(70, AREA_SENSORS_Y + 42);
    _tft.println(moving ? "ATTIVO" : "FERMO");
}

void DisplayManager::drawStatus(const LEDController& ledController) {
    // Clear status area
    _tft.fillRect(0, AREA_STATUS_Y, 128, AREA_STATUS_HEIGHT, COLOR_BACKGROUND);
    
    // Draw LED status
    bool ledOn = ledController.isOn();
    uint16_t ledColor = ledOn ? ST77XX_GREEN : ST77XX_RED;
    
    _tft.setTextSize(1);
    _tft.setTextColor(COLOR_TEXT);
    _tft.setCursor(5, AREA_STATUS_Y + 5);
    _tft.println("LED STRIP:");
    
    // LED indicator
    if (ledOn) {
        _tft.fillRect(5, AREA_STATUS_Y + 17, 118, 15, ST77XX_GREEN);
        _tft.setTextColor(ST77XX_BLACK);
        _tft.setTextSize(1);
        _tft.setCursor(40, AREA_STATUS_Y + 21);
        _tft.print("ACCESO (");
        _tft.print(ledController.getBrightness());
        _tft.println(")");
    } else {
        _tft.drawRect(5, AREA_STATUS_Y + 17, 118, 15, ST77XX_RED);
        _tft.setTextColor(ST77XX_RED);
        _tft.setTextSize(1);
        _tft.setCursor(45, AREA_STATUS_Y + 21);
        _tft.println("SPENTO");
    }
}

void DisplayManager::drawWiFiIcon(uint8_t x, uint8_t y, uint16_t color) {
    // Draw simple WiFi icon (3 arcs)
    _tft.drawPixel(x + 6, y + 12, color);
    _tft.drawPixel(x + 7, y + 12, color);
    
    // Small arc
    _tft.drawLine(x + 4, y + 10, x + 9, y + 10, color);
    _tft.drawPixel(x + 3, y + 9, color);
    _tft.drawPixel(x + 10, y + 9, color);
    
    // Medium arc
    _tft.drawLine(x + 2, y + 7, x + 11, y + 7, color);
    _tft.drawPixel(x + 1, y + 6, color);
    _tft.drawPixel(x + 12, y + 6, color);
    
    // Large arc
    _tft.drawLine(x, y + 4, x + 13, y + 4, color);
}

void DisplayManager::drawCenteredText(const String& text, uint8_t y, uint8_t fontSize, uint16_t color) {
    _tft.setTextSize(fontSize);
    _tft.setTextColor(color);
    
    // Calculate text width (approximate)
    uint8_t charWidth = 6 * fontSize;
    uint8_t textWidth = text.length() * charWidth;
    uint8_t x = (128 - textWidth) / 2;
    
    _tft.setCursor(x, y);
    _tft.println(text);
}

uint16_t DisplayManager::getWiFiStateColor(WiFiManager::ConnectionState state) {
    switch (state) {
        case WiFiManager::ConnectionState::CONNECTED:
            return COLOR_CONNECTED;
        case WiFiManager::ConnectionState::CONNECTING:
        case WiFiManager::ConnectionState::RECONNECTING:
            return COLOR_CONNECTING;
        case WiFiManager::ConnectionState::AP_MODE:
            return COLOR_AP_MODE;
        case WiFiManager::ConnectionState::DISCONNECTED:
        case WiFiManager::ConnectionState::CONNECTION_FAILED:
        default:
            return COLOR_DISCONNECTED;
    }
}

String DisplayManager::formatIP(const IPAddress& ip) {
    return ip.toString();
}

String DisplayManager::shortenSSID(const String& ssid, uint8_t maxLen) {
    if (ssid.length() <= maxLen) {
        return ssid;
    }
    return ssid.substring(0, maxLen - 2) + "..";
}
