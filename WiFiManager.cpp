#include "WiFiManager.h"
#include "WiFiPages.h"
#include "config.h"
#include "SmartLightController.h"
#include "LightSensor.h"
#include "LEDController.h"
#include "MotionDetector.h"
#include "EventLogger.h"

WiFiManager::WiFiManager()
    : _webServer(nullptr)
    , _dnsServer(nullptr)
    , _state(ConnectionState::DISCONNECTED)
    , _hostname(WIFI_HOSTNAME)
    , _connectionStartTime(0)
    , _lastReconnectAttempt(0)
    , _retryIntervalMs(WIFI_RETRY_INTERVAL_MS)
    , _resetButtonPressStart(0)
    , _apModeActive(false)
    , _resetButtonPressed(false)
    , _smartLightController(nullptr)
    , _lightSensor(nullptr)
    , _motionDetector(nullptr)
    , _ledController(nullptr)
    , _eventLogger(nullptr)
    , _rgbBrightness(nullptr)
{
}

WiFiManager::~WiFiManager() {
    if (_webServer) {
        _webServer->stop();
        delete _webServer;
    }
    if (_dnsServer) {
        _dnsServer->stop();
        delete _dnsServer;
    }
    _preferences.end();
}

bool WiFiManager::begin() {
    Serial.println("\n========== WIFI MANAGER INITIALIZATION ==========");
    
    // Initialize preferences
    if (!_preferences.begin(WIFI_PREFS_NAMESPACE, false)) {
        Serial.println("ERROR: Failed to initialize Preferences!");
        return false;
    }
    
    // Load stored credentials and settings
    if (loadCredentials()) {
        Serial.println("Stored credentials found, attempting connection...");
        startStationMode();
    } else {
        Serial.println("No stored credentials found, starting AP mode...");
        startAPMode();
    }
    
    Serial.println("=================================================\n");
    return true;
}

void WiFiManager::update() {
    // Check factory reset button
    checkResetButton();
    
    // Handle connection state
    switch (_state) {
        case ConnectionState::CONNECTING:
            checkConnection();
            break;
            
        case ConnectionState::CONNECTED:
            // Handle web server even in connected state
            if (_webServer) {
                _webServer->handleClient();
            }
            
            // Check if still connected
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("Wi-Fi connection lost!");
                _state = ConnectionState::DISCONNECTED;
                _lastError = "Connection lost";
            }
            break;
            
        case ConnectionState::DISCONNECTED:
        case ConnectionState::CONNECTION_FAILED:
            handleReconnection();
            break;
            
        case ConnectionState::AP_MODE:
            // Handle web server and DNS
            if (_webServer) {
                _webServer->handleClient();
            }
            if (_dnsServer) {
                _dnsServer->processNextRequest();
            }
            break;
            
        case ConnectionState::RECONNECTING:
            checkConnection();
            break;
    }
}

bool WiFiManager::loadCredentials() {
    _ssid = _preferences.getString(WIFI_PREFS_SSID_KEY, "");
    _password = _preferences.getString(WIFI_PREFS_PASSWORD_KEY, "");
    _retryIntervalMs = _preferences.getULong(WIFI_PREFS_RETRY_KEY, WIFI_RETRY_INTERVAL_MS);
    
    Serial.print("Loaded SSID: ");
    Serial.println(_ssid.isEmpty() ? "(none)" : _ssid);
    Serial.print("Retry Interval: ");
    Serial.print(_retryIntervalMs / 1000);
    Serial.println(" seconds");
    
    return !_ssid.isEmpty();
}

void WiFiManager::startStationMode() {
    Serial.println("\n--- Starting Station Mode ---");
    
    // Stop AP mode if active
    if (_apModeActive) {
        stopAPMode();
    }
    
    // Configure WiFi
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(_hostname.c_str());
    
    // Start connection
    Serial.print("Connecting to: ");
    Serial.println(_ssid);
    WiFi.begin(_ssid.c_str(), _password.c_str());
    
    _state = ConnectionState::CONNECTING;
    _connectionStartTime = millis();
    _lastError = "";
}

void WiFiManager::startAPMode() {
    Serial.println("\n--- Starting AP Mode ---");
    
    _apModeActive = true;
    _state = ConnectionState::AP_MODE;
    
    // Disconnect from station if connected
    WiFi.disconnect();
    
    // Configure AP
    WiFi.mode(WIFI_AP);
    
    bool apStarted;
    if (strlen(WIFI_AP_PASSWORD) >= 8) {
        apStarted = WiFi.softAP(
            WIFI_AP_SSID,
            WIFI_AP_PASSWORD,
            WIFI_AP_CHANNEL,
            WIFI_AP_HIDDEN,
            WIFI_AP_MAX_CONNECTIONS
        );
    } else {
        // Open network (no password)
        apStarted = WiFi.softAP(
            WIFI_AP_SSID,
            NULL,
            WIFI_AP_CHANNEL,
            WIFI_AP_HIDDEN,
            WIFI_AP_MAX_CONNECTIONS
        );
    }
    
    if (!apStarted) {
        Serial.println("ERROR: Failed to start AP!");
        _lastError = "Failed to start AP";
        return;
    }
    
    IPAddress apIP = WiFi.softAPIP();
    Serial.print("AP Started: ");
    Serial.println(WIFI_AP_SSID);
    Serial.print("AP IP: ");
    Serial.println(apIP);
    
    // Setup DNS server for captive portal
    if (WIFI_CAPTIVE_PORTAL_ENABLED) {
        _dnsServer = new DNSServer();
        _dnsServer->start(53, "*", apIP); // Redirect all DNS requests to AP IP
        Serial.println("DNS Server started (Captive Portal enabled)");
    }
    
    // Setup web server
    setupWebServer();
    
    Serial.println("AP Mode ready - Connect to configure Wi-Fi");
    Serial.println("-------------------------------\n");
}

void WiFiManager::stopAPMode() {
    Serial.println("Stopping AP mode...");
    
    // Keep web server running, only stop DNS and AP
    // Web server will continue to serve in Station mode
    
    if (_dnsServer) {
        _dnsServer->stop();
        delete _dnsServer;
        _dnsServer = nullptr;
    }
    
    WiFi.softAPdisconnect(true);
    _apModeActive = false;
}

void WiFiManager::checkConnection() {
    wl_status_t status = WiFi.status();
    
    // Check timeout
    if (millis() - _connectionStartTime > WIFI_CONNECTION_TIMEOUT_MS) {
        Serial.println("Connection timeout!");
        _state = ConnectionState::CONNECTION_FAILED;
        _lastError = "Connection timeout";
        
        // If no credentials or connection failed, start AP mode
        if (!hasStoredCredentials() || _state == ConnectionState::CONNECTING) {
            startAPMode();
        }
        return;
    }
    
    // Check connection status
    switch (status) {
        case WL_CONNECTED:
            _state = ConnectionState::CONNECTED;
            Serial.println("\n✓ Wi-Fi Connected!");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            Serial.print("Hostname: ");
            Serial.println(_hostname);
            Serial.print("RSSI: ");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm\n");
            _lastError = "";
            
            // Start web server if not already running
            if (!_webServer) {
                setupWebServer();
                Serial.println("Web server started for Station mode");
            }
            break;
            
        case WL_CONNECT_FAILED:
            _state = ConnectionState::CONNECTION_FAILED;
            _lastError = "Wrong password or SSID not found";
            Serial.println("Connection failed: Wrong password or SSID not found");
            break;
            
        case WL_NO_SSID_AVAIL:
            _state = ConnectionState::CONNECTION_FAILED;
            _lastError = "SSID not available";
            Serial.println("Connection failed: SSID not available");
            break;
            
        case WL_DISCONNECTED:
        case WL_IDLE_STATUS:
            // Still connecting...
            break;
            
        default:
            Serial.print("Connection status: ");
            Serial.println(status);
            break;
    }
}

void WiFiManager::handleReconnection() {
    // Only try to reconnect if we have credentials and enough time has passed
    if (!hasStoredCredentials()) {
        if (_state != ConnectionState::AP_MODE) {
            startAPMode();
        }
        return;
    }
    
    unsigned long now = millis();
    if (now - _lastReconnectAttempt >= _retryIntervalMs) {
        Serial.println("Attempting reconnection...");
        _lastReconnectAttempt = now;
        _state = ConnectionState::RECONNECTING;
        startStationMode();
    }
}

void WiFiManager::checkResetButton() {
    bool buttonPressed = (digitalRead(WIFI_RESET_BUTTON_PIN) == LOW);
    
    if (buttonPressed && !_resetButtonPressed) {
        // Button just pressed
        _resetButtonPressed = true;
        _resetButtonPressStart = millis();
    } else if (!buttonPressed && _resetButtonPressed) {
        // Button just released
        _resetButtonPressed = false;
    } else if (buttonPressed && _resetButtonPressed) {
        // Button held down
        unsigned long holdTime = millis() - _resetButtonPressStart;
        if (holdTime >= WIFI_RESET_HOLD_TIME_MS) {
            Serial.println("\n*** FACTORY RESET TRIGGERED ***");
            resetCredentials();
            _resetButtonPressed = false;
        }
    }
}

void WiFiManager::setupWebServer() {
    _webServer = new WebServer(WIFI_WEB_SERVER_PORT);
    
    // Register handlers
    _webServer->on("/", [this]() { handleRoot(); });
    _webServer->on("/dashboard", [this]() { handleDashboard(); });
    _webServer->on("/logs", [this]() { handleLogs(); });
    _webServer->on("/scan", [this]() { handleScan(); });
    _webServer->on("/save", [this]() { handleSave(); });
    _webServer->on("/status", [this]() { handleStatus(); });
    
    // API endpoints
    _webServer->on("/api/status", HTTP_GET, [this]() { handleApiStatus(); });
    _webServer->on("/api/config", HTTP_GET, [this]() { handleApiConfig(); });
    _webServer->on("/api/config", HTTP_POST, [this]() { handleApiConfigPost(); });
    _webServer->on("/api/led/override", HTTP_POST, [this]() { handleApiLedOverride(); });
    _webServer->on("/api/brightness", HTTP_GET, [this]() { handleApiBrightnessGet(); });
    _webServer->on("/api/brightness", HTTP_POST, [this]() { handleApiBrightness(); });
    _webServer->on("/api/logs", HTTP_GET, [this]() { handleApiLogs(); });
    _webServer->on("/api/logs", HTTP_DELETE, [this]() { handleApiLogsDelete(); });
    
    _webServer->onNotFound([this]() { handleNotFound(); });
    
    _webServer->begin();
    Serial.println("Web server started on port 80");
}

void WiFiManager::handleRoot() {
    if (_state == ConnectionState::AP_MODE) {
        // In AP mode, show configuration page
        String html = FPSTR(WIFI_CONFIG_PAGE);
        html.replace("%DEVICE_AP_SSID%", WIFI_AP_SSID);
        _webServer->send(200, "text/html", html);
    } else {
        // In Station mode, show status/dashboard page
        String html = FPSTR(WIFI_STATUS_PAGE);
        html.replace("%WIFI_SSID%", _ssid);
        html.replace("%WIFI_IP%", WiFi.localIP().toString());
        html.replace("%WIFI_HOSTNAME%", _hostname);
        html.replace("%WIFI_RSSI%", String(WiFi.RSSI()));
        html.replace("%WIFI_MAC%", WiFi.macAddress());
        _webServer->send(200, "text/html", html);
    }
}

void WiFiManager::handleScan() {
    Serial.println("Scanning networks...");
    int n = WiFi.scanNetworks();
    
    String json = "{\"networks\":[";
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"encrypted\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false");
        json += "}";
    }
    json += "]}";
    
    _webServer->send(200, "application/json", json);
    WiFi.scanDelete();
}

void WiFiManager::handleSave() {
    if (!_webServer->hasArg("ssid") || !_webServer->hasArg("password")) {
        _webServer->send(400, "application/json", "{\"success\":false,\"message\":\"Missing parameters\"}");
        return;
    }
    
    String ssid = _webServer->arg("ssid");
    String password = _webServer->arg("password");
    unsigned long retryInterval = _webServer->arg("retry").toInt() * 1000; // Convert to ms
    
    if (ssid.isEmpty()) {
        _webServer->send(400, "application/json", "{\"success\":false,\"message\":\"SSID cannot be empty\"}");
        return;
    }
    
    // Save credentials
    if (!saveCredentials(ssid, password)) {
        _webServer->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to save credentials\"}");
        return;
    }
    
    // Save retry interval
    if (retryInterval >= 30000 && retryInterval <= 3600000) {
        _retryIntervalMs = retryInterval;
        _preferences.putULong(WIFI_PREFS_RETRY_KEY, _retryIntervalMs);
    }
    
    String response = "{\"success\":true,\"message\":\"Credentials saved\",\"hostname\":\"" + _hostname + "\"}";
    _webServer->send(200, "application/json", response);
    
    // Restart connection after response is sent
    delay(1000);
    startStationMode();
}

void WiFiManager::handleStatus() {
    String json = "{";
    json += "\"state\":\"" + String(getStateString()) + "\",";
    json += "\"ssid\":\"" + _ssid + "\",";
    json += "\"connected\":" + String(isConnected() ? "true" : "false") + ",";
    json += "\"ip\":\"" + getIPAddress().toString() + "\",";
    json += "\"rssi\":" + String(getRSSI()) + ",";
    json += "\"error\":\"" + _lastError + "\"";
    json += "}";
    _webServer->send(200, "application/json", json);
}

void WiFiManager::handleNotFound() {
    // Captive portal redirect
    if (WIFI_CAPTIVE_PORTAL_ENABLED && _apModeActive) {
        handleRoot();
    } else {
        _webServer->send(404, "text/plain", "Not Found");
    }
}

bool WiFiManager::saveCredentials(const String& ssid, const String& password) {
    _ssid = ssid;
    _password = password;
    
    _preferences.putString(WIFI_PREFS_SSID_KEY, _ssid);
    _preferences.putString(WIFI_PREFS_PASSWORD_KEY, _password);
    
    Serial.println("Credentials saved:");
    Serial.print("  SSID: ");
    Serial.println(_ssid);
    Serial.println("  Password: ********");
    
    return true;
}

void WiFiManager::setRetryInterval(unsigned long intervalMs) {
    _retryIntervalMs = intervalMs;
    _preferences.putULong(WIFI_PREFS_RETRY_KEY, _retryIntervalMs);
    Serial.print("Retry interval set to: ");
    Serial.print(_retryIntervalMs / 1000);
    Serial.println(" seconds");
}

void WiFiManager::resetCredentials() {
    Serial.println("Resetting Wi-Fi credentials...");
    _preferences.clear();
    _ssid = "";
    _password = "";
    _lastError = "";
    
    // Restart in AP mode
    startAPMode();
    Serial.println("Credentials reset. Device in AP mode.");
}

void WiFiManager::reconnect() {
    Serial.println("Manual reconnection triggered");
    _lastReconnectAttempt = 0; // Force immediate reconnection
    _state = ConnectionState::DISCONNECTED;
}

bool WiFiManager::hasStoredCredentials() const {
    return !_ssid.isEmpty() && !_password.isEmpty();
}

IPAddress WiFiManager::getIPAddress() const {
    if (_apModeActive) {
        return WiFi.softAPIP();
    }
    return WiFi.localIP();
}

int32_t WiFiManager::getRSSI() const {
    if (isConnected()) {
        return WiFi.RSSI();
    }
    return 0;
}

unsigned long WiFiManager::getReconnectTimeRemaining() const {
    if (_state == ConnectionState::CONNECTED || _state == ConnectionState::AP_MODE) {
        return 0;
    }
    
    unsigned long elapsed = millis() - _lastReconnectAttempt;
    if (elapsed >= _retryIntervalMs) {
        return 0;
    }
    
    return _retryIntervalMs - elapsed;
}

const char* WiFiManager::getStateString() const {
    switch (_state) {
        case ConnectionState::DISCONNECTED: return "DISCONNECTED";
        case ConnectionState::CONNECTING: return "CONNECTING";
        case ConnectionState::CONNECTED: return "CONNECTED";
        case ConnectionState::AP_MODE: return "AP_MODE";
        case ConnectionState::CONNECTION_FAILED: return "FAILED";
        case ConnectionState::RECONNECTING: return "RECONNECTING";
        default: return "UNKNOWN";
    }
}

void WiFiManager::setSystemComponents(void* controller, void* lightSensor, 
                                       void* motionDetector, void* ledController,
                                       void* eventLogger, uint8_t* rgbBrightness) {
    _smartLightController = controller;
    _lightSensor = lightSensor;
    _motionDetector = motionDetector;
    _ledController = ledController;
    _eventLogger = eventLogger;
    _rgbBrightness = rgbBrightness;
    Serial.println("System components linked to WiFiManager");
}

void WiFiManager::handleDashboard() {
    String html = FPSTR(WIFI_DASHBOARD_PAGE);
    _webServer->send(200, "text/html", html);
}

void WiFiManager::handleLogs() {
    String html = FPSTR(WIFI_LOGS_PAGE);
    _webServer->send(200, "text/html", html);
}

void WiFiManager::handleApiStatus() {
    // Need forward declarations to avoid circular dependencies
    // Cast pointers back to proper types
    auto* controller = static_cast<SmartLightController*>(_smartLightController);
    auto* lightSensor = static_cast<LightSensor*>(_lightSensor);
    auto* motionDetector = static_cast<MotionDetector*>(_motionDetector);
    auto* ledController = static_cast<LEDController*>(_ledController);
    
    if (!controller || !lightSensor || !motionDetector || !ledController) {
        _webServer->send(500, "application/json", 
            "{\"error\":\"System components not initialized\"}");
        return;
    }
    
    // Get actual LED state from LED controller
    bool ledIsOn = ledController->isOn();
    String ledMode = "auto";
    
    // Check if manual override is active
    if (controller->isManualOverride()) {
        // In manual override, determine mode from LED state
        ledMode = ledIsOn ? "on" : "off";
    } else if (!controller->isAutoModeEnabled()) {
        // Auto mode disabled (but not manual override)
        ledMode = ledIsOn ? "on" : "off";
    }
    // else: auto mode active, ledMode = "auto"
    
    String json = "{";
    json += "\"led_on\":" + String(ledIsOn ? "true" : "false") + ",";
    json += "\"led_mode\":\"" + ledMode + "\",";
    json += "\"lux\":" + String(lightSensor->getLastLux(), 1) + ",";
    json += "\"motion\":" + String(motionDetector->isMoving() ? "true" : "false") + ",";
    json += "\"rssi\":" + String(getRSSI());
    json += "}";
    
    _webServer->send(200, "application/json", json);
}

void WiFiManager::handleApiConfig() {
    // Read current values from sensors (not from Preferences)
    // This ensures we get the actual current values, including any changes
    // made through buttons or serial commands
    auto* lightSensor = static_cast<LightSensor*>(_lightSensor);
    auto* motionDetector = static_cast<MotionDetector*>(_motionDetector);
    auto* controller = static_cast<SmartLightController*>(_smartLightController);
    
    if (!lightSensor || !motionDetector || !controller) {
        _webServer->send(500, "application/json", 
            "{\"error\":\"System components not initialized\"}");
        return;
    }
    
    // Get live values from sensors
    float luxThresh = lightSensor->getNightThreshold();
    float accelThresh = motionDetector->getAccThreshold();
    float gyroThresh = motionDetector->getGyroThreshold();
    unsigned long shutoff = controller->getShutoffDelay();
    
    String json = "{";
    json += "\"lux_threshold\":" + String(luxThresh, 1) + ",";
    json += "\"accel_threshold\":" + String(accelThresh, 4) + ",";
    json += "\"gyro_threshold\":" + String(gyroThresh, 2) + ",";
    json += "\"shutoff_delay\":" + String(shutoff);
    json += "}";
    
    _webServer->send(200, "application/json", json);
}

void WiFiManager::handleApiConfigPost() {
    if (!_webServer->hasArg("plain")) {
        _webServer->send(400, "application/json", 
            "{\"success\":false,\"message\":\"No body provided\"}");
        return;
    }
    
    // Parse JSON manually (Arduino JSON library might not be available)
    String body = _webServer->arg("plain");
    
    Serial.print("Received config JSON: ");
    Serial.println(body);
    
    // Extract values (simple parsing, assuming valid JSON)
    float luxThresh = -1, accelThresh = -1, gyroThresh = -1;
    unsigned long shutoff = 0;
    
    // Helper lambda to extract numeric value after a key
    auto extractNumeric = [&body](const char* key) -> String {
        int idx = body.indexOf(key);
        if (idx >= 0) {
            int colonIdx = body.indexOf(":", idx);
            if (colonIdx >= 0) {
                String valueStr = body.substring(colonIdx + 1);
                // Skip whitespace
                int startIdx = 0;
                while (startIdx < valueStr.length() && 
                       (valueStr.charAt(startIdx) == ' ' || valueStr.charAt(startIdx) == '\t')) {
                    startIdx++;
                }
                return valueStr.substring(startIdx);
            }
        }
        return "";
    };
    
    String val;
    if ((val = extractNumeric("lux_threshold")) != "") {
        luxThresh = val.toFloat();
    }
    if ((val = extractNumeric("accel_threshold")) != "") {
        accelThresh = val.toFloat();
    }
    if ((val = extractNumeric("gyro_threshold")) != "") {
        gyroThresh = val.toFloat();
    }
    if ((val = extractNumeric("shutoff_delay")) != "") {
        shutoff = val.toInt();
    }
    
    // Save to preferences
    Preferences prefs;
    prefs.begin(CONFIG_PREFS_NAMESPACE, false);  // Read-write
    
    if (luxThresh >= 0) prefs.putFloat(CONFIG_LUX_THRESHOLD_KEY, luxThresh);
    if (accelThresh >= 0) prefs.putFloat(CONFIG_ACCEL_THRESHOLD_KEY, accelThresh);
    if (gyroThresh >= 0) prefs.putFloat(CONFIG_GYRO_THRESHOLD_KEY, gyroThresh);
    if (shutoff > 0) prefs.putULong(CONFIG_LED_SHUTOFF_KEY, shutoff);
    
    prefs.end();
    
    // Apply to system components
    auto* lightSensor = static_cast<LightSensor*>(_lightSensor);
    auto* motionDetector = static_cast<MotionDetector*>(_motionDetector);
    auto* controller = static_cast<SmartLightController*>(_smartLightController);
    
    bool anyChanged = false;
    
    if (lightSensor && luxThresh >= 0) {
        lightSensor->setNightThreshold(luxThresh);
        anyChanged = true;
    }
    if (motionDetector && accelThresh >= 0) {
        motionDetector->setAccThreshold(accelThresh);
        anyChanged = true;
    }
    if (motionDetector && gyroThresh >= 0) {
        motionDetector->setGyroThreshold(gyroThresh);
        anyChanged = true;
    }
    if (controller && shutoff > 0) {
        controller->setShutoffDelay(shutoff);
        anyChanged = true;
    }
    
    // Save configuration to persistent storage
    if (anyChanged && controller) {
        controller->saveConfiguration();
    }
    
    Serial.println("Configuration updated from web dashboard");
    
    _webServer->send(200, "application/json", 
        "{\"success\":true,\"message\":\"Configuration saved\"}");
}

void WiFiManager::handleApiLedOverride() {
    if (!_webServer->hasArg("plain")) {
        _webServer->send(400, "application/json", 
            "{\"success\":false,\"message\":\"No body provided\"}");
        return;
    }
    
    String body = _webServer->arg("plain");
    String mode = "";
    
    Serial.print("Received LED override JSON: ");
    Serial.println(body);
    
    // Extract mode value (handle both with and without quotes)
    int idx = body.indexOf("mode");
    if (idx >= 0) {
        int colonIdx = body.indexOf(":", idx);
        if (colonIdx >= 0) {
            String valueStr = body.substring(colonIdx + 1);
            // Skip whitespace and quotes
            int startIdx = 0;
            while (startIdx < valueStr.length() && 
                   (valueStr.charAt(startIdx) == ' ' || 
                    valueStr.charAt(startIdx) == '\t' ||
                    valueStr.charAt(startIdx) == '"')) {
                startIdx++;
            }
            // Find end (quote, comma, or brace)
            int endIdx = startIdx;
            while (endIdx < valueStr.length() && 
                   valueStr.charAt(endIdx) != '"' &&
                   valueStr.charAt(endIdx) != ',' &&
                   valueStr.charAt(endIdx) != '}') {
                endIdx++;
            }
            mode = valueStr.substring(startIdx, endIdx);
            Serial.print("Parsed mode: ");
            Serial.println(mode);
        }
    }
    
    auto* controller = static_cast<SmartLightController*>(_smartLightController);
    if (!controller) {
        _webServer->send(500, "application/json", 
            "{\"success\":false,\"message\":\"Controller not initialized\"}");
        return;
    }
    
    if (mode == "auto") {
        controller->returnToAuto();
        Serial.println("LED mode set to AUTO");
    } else if (mode == "on") {
        // Load saved brightness from Preferences
        Preferences prefs;
        prefs.begin(CONFIG_PREFS_NAMESPACE, true);  // Read-only
        uint8_t brightness = prefs.getUChar(CONFIG_LED_BRIGHTNESS_KEY, DEFAULT_LED_BRIGHTNESS);
        prefs.end();
        
        controller->forceOn(brightness);
        Serial.print("LED mode set to FORCED ON with brightness: ");
        Serial.println(brightness);
    } else if (mode == "off") {
        controller->forceOff();
        Serial.println("LED mode set to FORCED OFF");
    } else {
        _webServer->send(400, "application/json", 
            "{\"success\":false,\"message\":\"Invalid mode\"}");
        return;
    }
    
    _webServer->send(200, "application/json", 
        "{\"success\":true,\"message\":\"Mode updated\"}");
}

void WiFiManager::handleApiBrightnessGet() {
    // Read brightness values from Preferences (not from LED controller)
    // This ensures we always get the saved values, even if LED is off
    Preferences prefs;
    prefs.begin(CONFIG_PREFS_NAMESPACE, true);  // Read-only
    
    uint8_t ledBrightness = prefs.getUChar(CONFIG_LED_BRIGHTNESS_KEY, DEFAULT_LED_BRIGHTNESS);
    uint8_t rgbBrightness = prefs.getUChar(CONFIG_RGB_BRIGHTNESS_KEY, RGB_BRIGHTNESS);
    
    prefs.end();
    
    // Override RGB with global variable if available
    if (_rgbBrightness != nullptr) {
        rgbBrightness = *_rgbBrightness;
    }
    
    String json = "{";
    json += "\"led_brightness\":" + String(ledBrightness) + ",";
    json += "\"rgb_brightness\":" + String(rgbBrightness);
    json += "}";
    
    _webServer->send(200, "application/json", json);
}

void WiFiManager::handleApiBrightness() {
    if (!_webServer->hasArg("plain")) {
        _webServer->send(400, "application/json", 
            "{\"success\":false,\"message\":\"No body provided\"}");
        return;
    }
    
    String body = _webServer->arg("plain");
    int ledBrightness = -1;
    int rgbBrightness = -1;
    
    Serial.print("Received brightness JSON: ");
    Serial.println(body);
    
    // Parse LED strip brightness (handle both with and without quotes in keys)
    int idx = body.indexOf("led_brightness");
    if (idx >= 0) {
        // Find the colon after the key
        int colonIdx = body.indexOf(":", idx);
        if (colonIdx >= 0) {
            // Extract substring after colon and parse
            String valueStr = body.substring(colonIdx + 1);
            // Find first digit or minus sign
            int startIdx = 0;
            while (startIdx < valueStr.length() && 
                   !isdigit(valueStr.charAt(startIdx)) && 
                   valueStr.charAt(startIdx) != '-') {
                startIdx++;
            }
            if (startIdx < valueStr.length()) {
                ledBrightness = valueStr.substring(startIdx).toInt();
                Serial.print("Parsed led_brightness: ");
                Serial.println(ledBrightness);
            }
        }
    }
    
    // Parse RGB LED brightness (handle both with and without quotes in keys)
    idx = body.indexOf("rgb_brightness");
    if (idx >= 0) {
        // Find the colon after the key
        int colonIdx = body.indexOf(":", idx);
        if (colonIdx >= 0) {
            // Extract substring after colon and parse
            String valueStr = body.substring(colonIdx + 1);
            // Find first digit or minus sign
            int startIdx = 0;
            while (startIdx < valueStr.length() && 
                   !isdigit(valueStr.charAt(startIdx)) && 
                   valueStr.charAt(startIdx) != '-') {
                startIdx++;
            }
            if (startIdx < valueStr.length()) {
                rgbBrightness = valueStr.substring(startIdx).toInt();
                Serial.print("Parsed rgb_brightness: ");
                Serial.println(rgbBrightness);
            }
        }
    }
    
    bool updated = false;
    Preferences prefs;
    prefs.begin(CONFIG_PREFS_NAMESPACE, false);  // Read-write
    
    // Cast to get LED controller
    auto* ledController = static_cast<LEDController*>(_ledController);
    
    // Save AND apply LED strip brightness immediately
    if (ledBrightness >= 0 && ledBrightness <= 255) {
        prefs.putUChar(CONFIG_LED_BRIGHTNESS_KEY, ledBrightness);
        
        // Apply brightness if LED is currently on
        if (ledController && ledController->isOn()) {
            ledController->setBrightness(ledBrightness);
            Serial.print("Applied LED strip brightness: ");
            Serial.println(ledBrightness);
        } else {
            Serial.print("Saved LED strip brightness (will apply when LED turns on): ");
            Serial.println(ledBrightness);
        }
        updated = true;
    }
    
    // Save RGB LED brightness (apply immediately to global variable)
    if (rgbBrightness >= 0 && rgbBrightness <= 255 && _rgbBrightness != nullptr) {
        *_rgbBrightness = rgbBrightness;  // Update global variable
        prefs.putUChar(CONFIG_RGB_BRIGHTNESS_KEY, rgbBrightness);
        Serial.print("Saved RGB LED brightness: ");
        Serial.println(rgbBrightness);
        updated = true;
    }
    
    prefs.end();
    
    if (!updated) {
        _webServer->send(400, "application/json", 
            "{\"success\":false,\"message\":\"Invalid brightness values\"}");
        return;
    }
    
    _webServer->send(200, "application/json", 
        "{\"success\":true,\"message\":\"Brightness updated\"}");
}

void WiFiManager::handleApiLogs() {
    auto* logger = static_cast<EventLogger*>(_eventLogger);
    if (!logger) {
        _webServer->send(500, "application/json", 
            "{\"error\":\"Event logger not initialized\"}");
        return;
    }
    
    String json = logger->getEventsJSON();
    _webServer->send(200, "application/json", json);
}

void WiFiManager::handleApiLogsDelete() {
    auto* logger = static_cast<EventLogger*>(_eventLogger);
    if (!logger) {
        _webServer->send(500, "application/json", 
            "{\"success\":false,\"message\":\"Event logger not initialized\"}");
        return;
    }
    
    logger->clearAll();
    Serial.println("All logs cleared via API");
    
    _webServer->send(200, "application/json", 
        "{\"success\":true,\"message\":\"Logs cleared\"}");
}


