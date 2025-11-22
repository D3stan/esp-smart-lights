#include "OTAManager.h"
#include "OTAPages.h"
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

// Constructor
OTAManager::OTAManager(WebServer& webServer)
    : _webServer(webServer)
    , _state(OTAState::IDLE)
    , _lastError(OTAError::NONE)
    , _progress(0)
    , _startTime(0)
    , _lastProgressUpdate(0)
    , _totalSize(0)
    , _writtenSize(0)
    , _updateInProgress(false)
    , _httpClient(nullptr)
{
}

// Destructor
OTAManager::~OTAManager() {
    cleanup();
}

// Initialize OTA manager
bool OTAManager::begin() {
    Serial.println("[OTA] Initializing OTA Manager...");
    
    // Setup web handlers
    setupWebHandlers();
    
    // Get partition info
    const esp_partition_t* partition = esp_ota_get_running_partition();
    if (partition) {
        Serial.printf("[OTA] Running from partition: %s (type=%d, subtype=%d)\n", 
                      partition->label, partition->type, partition->subtype);
        Serial.printf("[OTA] Partition size: %d bytes\n", partition->size);
    }
    
    // Check if we can rollback
    if (canRollback()) {
        Serial.println("[OTA] Rollback is available");
    } else {
        Serial.println("[OTA] Rollback not available");
    }
    
    Serial.println("[OTA] OTA Manager initialized");
    return true;
}

// Update loop
void OTAManager::update() {
    // Nothing to do in loop for now
    // Upload handling is done by WebServer callbacks
}

// Setup web handlers
void OTAManager::setupWebHandlers() {
    _webServer.on("/ota", HTTP_GET, [this]() { handleOTAPage(); });
    _webServer.on("/api/ota/info", HTTP_GET, [this]() { handleOTAInfo(); });
    _webServer.on("/api/ota/url", HTTP_POST, [this]() { handleOTAURL(); });
    _webServer.on("/api/ota/status", HTTP_GET, [this]() { handleOTAStatus(); });
    _webServer.on("/api/ota/rollback", HTTP_POST, [this]() { handleOTARollback(); });
    
    // Upload handler with data callback
    _webServer.on("/api/ota/upload", HTTP_POST, 
        [this]() { handleOTAUpload(); },
        [this]() { handleOTAUploadData(); }
    );
}

// Update from URL
bool OTAManager::updateFromURL(const String& url) {
    if (_updateInProgress) {
        Serial.println("[OTA] Update already in progress");
        setError(OTAError::UNKNOWN);
        return false;
    }
    
    if (!validateURL(url)) {
        Serial.println("[OTA] Invalid URL");
        setError(OTAError::INVALID_URL);
        return false;
    }
    
    Serial.printf("[OTA] Starting update from URL: %s\n", url.c_str());
    
    setState(OTAState::CONNECTING);
    _updateInProgress = true;
    _startTime = millis();
    _writtenSize = 0;
    
    // Create HTTP client
    _httpClient = new HTTPClient();
    
    WiFiClient* client;
    if (url.startsWith("https://")) {
        WiFiClientSecure* secureClient = new WiFiClientSecure();
        secureClient->setInsecure(); // Don't verify certificates
        client = secureClient;
    } else {
        client = new WiFiClient();
    }
    
    // Begin HTTP connection
    if (!_httpClient->begin(*client, url)) {
        Serial.println("[OTA] Failed to begin HTTP connection");
        setError(OTAError::CONNECTION_REFUSED);
        cleanup();
        return false;
    }
    
    _httpClient->setTimeout(60000); // 60 second timeout
    
    // Send GET request
    int httpCode = _httpClient->GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[OTA] HTTP error: %d\n", httpCode);
        if (httpCode == 404) {
            setError(OTAError::HTTP_404);
        } else if (httpCode >= 500) {
            setError(OTAError::HTTP_500);
        } else {
            setError(OTAError::INVALID_RESPONSE);
        }
        cleanup();
        return false;
    }
    
    // Get content length
    _totalSize = _httpClient->getSize();
    
    if (_totalSize <= 0) {
        Serial.println("[OTA] Invalid content length");
        setError(OTAError::INVALID_RESPONSE);
        cleanup();
        return false;
    }
    
    Serial.printf("[OTA] Firmware size: %d bytes\n", _totalSize);
    
    // Check available space
    if (!checkSpace(_totalSize)) {
        setError(OTAError::FILE_TOO_LARGE);
        cleanup();
        return false;
    }
    
    // Begin OTA
    if (!beginOTA(_totalSize)) {
        setError(OTAError::OTA_BEGIN_FAILED);
        cleanup();
        return false;
    }
    
    setState(OTAState::DOWNLOADING);
    
    // Get stream
    WiFiClient* stream = _httpClient->getStreamPtr();
    
    // Download and write
    uint8_t buffer[1024];
    size_t bytesRead = 0;
    
    while (_httpClient->connected() && bytesRead < _totalSize) {
        size_t available = stream->available();
        
        if (available) {
            size_t toRead = min(available, sizeof(buffer));
            size_t read = stream->readBytes(buffer, toRead);
            
            if (read > 0) {
                if (!writeOTA(buffer, read)) {
                    setError(OTAError::FLASH_WRITE_FAILED);
                    cleanup();
                    return false;
                }
                
                bytesRead += read;
                updateProgress(bytesRead, _totalSize);
            }
        } else {
            delay(1);
        }
    }
    
    // End OTA
    if (!endOTA()) {
        setError(OTAError::OTA_END_FAILED);
        cleanup();
        return false;
    }
    
    setState(OTAState::SUCCESS);
    cleanup();
    
    Serial.println("[OTA] Update successful! Rebooting in 3 seconds...");
    delay(3000);
    reboot();
    
    return true;
}

// Validate URL
bool OTAManager::validateURL(const String& url) {
    if (url.length() == 0) {
        return false;
    }
    
    if (!url.startsWith("http://") && !url.startsWith("https://")) {
        return false;
    }
    
    if (!url.endsWith(".bin")) {
        return false;
    }
    
    return true;
}

// Check available space
bool OTAManager::checkSpace(size_t requiredSize) {
    size_t available = getAvailableSpace();
    
    if (requiredSize > available) {
        Serial.printf("[OTA] Insufficient space: required=%d, available=%d\n", 
                      requiredSize, available);
        return false;
    }
    
    return true;
}

// Begin OTA
bool OTAManager::beginOTA(size_t size) {
    Serial.println("[OTA] Beginning OTA update...");
    
    if (!Update.begin(size)) {
        Serial.printf("[OTA] Begin failed: %s\n", Update.errorString());
        return false;
    }
    
    setState(OTAState::WRITING);
    return true;
}

// Write OTA data
bool OTAManager::writeOTA(uint8_t* data, size_t len) {
    size_t written = Update.write(data, len);
    
    if (written != len) {
        Serial.printf("[OTA] Write failed: written=%d, expected=%d\n", written, len);
        Serial.printf("[OTA] Error: %s\n", Update.errorString());
        return false;
    }
    
    _writtenSize += written;
    return true;
}

// End OTA
bool OTAManager::endOTA() {
    Serial.println("[OTA] Finalizing OTA update...");
    
    if (!Update.end(true)) {
        Serial.printf("[OTA] End failed: %s\n", Update.errorString());
        return false;
    }
    
    Serial.println("[OTA] OTA update finalized successfully");
    return true;
}

// Reboot device
void OTAManager::reboot() {
    Serial.println("[OTA] Rebooting...");
    Serial.flush();
    ESP.restart();
}

// Cleanup resources
void OTAManager::cleanup() {
    if (_httpClient) {
        _httpClient->end();
        delete _httpClient;
        _httpClient = nullptr;
    }
    
    _updateInProgress = false;
}

// Set state
void OTAManager::setState(OTAState state) {
    _state = state;
    Serial.printf("[OTA] State changed: %s\n", getStateString());
}

// Set error
void OTAManager::setError(OTAError error) {
    _lastError = error;
    _state = OTAState::ERROR;
    Serial.printf("[OTA] Error: %s\n", getLastErrorString().c_str());
}

// Update progress
void OTAManager::updateProgress(size_t current, size_t total) {
    if (total == 0) {
        _progress = 0;
        return;
    }
    
    _progress = (current * 100) / total;
    
    // Log progress every 10%
    unsigned long now = millis();
    if (now - _lastProgressUpdate > 1000) {
        Serial.printf("[OTA] Progress: %d%% (%d / %d bytes)\n", _progress, current, total);
        _lastProgressUpdate = now;
    }
}

// Get state string
const char* OTAManager::getStateString() const {
    switch (_state) {
        case OTAState::IDLE:        return "IDLE";
        case OTAState::CONNECTING:  return "CONNECTING";
        case OTAState::DOWNLOADING: return "DOWNLOADING";
        case OTAState::UPLOADING:   return "UPLOADING";
        case OTAState::WRITING:     return "WRITING";
        case OTAState::VERIFYING:   return "VERIFYING";
        case OTAState::REBOOTING:   return "REBOOTING";
        case OTAState::SUCCESS:     return "SUCCESS";
        case OTAState::ERROR:       return "ERROR";
        default:                    return "UNKNOWN";
    }
}

// Get error string
String OTAManager::getLastErrorString() const {
    return String(errorToString(_lastError));
}

// Error to string
const char* OTAManager::errorToString(OTAError error) {
    switch (error) {
        case OTAError::NONE:                return "No error";
        case OTAError::CONNECTION_REFUSED:  return "Connection refused";
        case OTAError::TIMEOUT:             return "Timeout";
        case OTAError::DNS_FAILED:          return "DNS resolution failed";
        case OTAError::SSL_FAILED:          return "SSL/TLS error";
        case OTAError::HTTP_404:            return "File not found (HTTP 404)";
        case OTAError::HTTP_500:            return "Server error (HTTP 5xx)";
        case OTAError::INVALID_RESPONSE:    return "Invalid HTTP response";
        case OTAError::FILE_TOO_LARGE:      return "File too large";
        case OTAError::PARTITION_NOT_FOUND: return "OTA partition not found";
        case OTAError::FLASH_WRITE_FAILED:  return "Flash write failed";
        case OTAError::FLASH_VERIFY_FAILED: return "Flash verification failed";
        case OTAError::INSUFFICIENT_SPACE:  return "Insufficient space";
        case OTAError::OTA_BEGIN_FAILED:    return "OTA begin failed";
        case OTAError::OTA_END_FAILED:      return "OTA end failed";
        case OTAError::ROLLBACK_FAILED:     return "Rollback failed";
        case OTAError::INVALID_URL:         return "Invalid URL";
        case OTAError::INVALID_FILE:        return "Invalid file";
        case OTAError::UNKNOWN:             return "Unknown error";
        default:                            return "Undefined error";
    }
}

// Get current partition
String OTAManager::getCurrentPartition() const {
    const esp_partition_t* partition = esp_ota_get_running_partition();
    
    if (!partition) {
        return "unknown";
    }
    
    if (partition->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0) {
        return "OTA_0";
    } else if (partition->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1) {
        return "OTA_1";
    } else if (partition->subtype == ESP_PARTITION_SUBTYPE_APP_FACTORY) {
        return "factory";
    }
    
    return String(partition->label);
}

// Get available space
size_t OTAManager::getAvailableSpace() const {
    const esp_partition_t* partition = esp_ota_get_next_update_partition(nullptr);
    
    if (!partition) {
        return 0;
    }
    
    return partition->size;
}

// Get max firmware size
size_t OTAManager::getMaxFirmwareSize() const {
    return getAvailableSpace();
}

// Can rollback
bool OTAManager::canRollback() const {
    const esp_partition_t* partition = esp_ota_get_last_invalid_partition();
    return (partition != nullptr);
}

// Rollback
bool OTAManager::rollback() {
    Serial.println("[OTA] Performing manual rollback...");
    
    const esp_partition_t* partition = esp_ota_get_last_invalid_partition();
    
    if (!partition) {
        Serial.println("[OTA] No partition available for rollback");
        setError(OTAError::ROLLBACK_FAILED);
        return false;
    }
    
    esp_err_t err = esp_ota_set_boot_partition(partition);
    
    if (err != ESP_OK) {
        Serial.printf("[OTA] Rollback failed: %d\n", err);
        setError(OTAError::ROLLBACK_FAILED);
        return false;
    }
    
    Serial.println("[OTA] Rollback successful, rebooting...");
    delay(1000);
    reboot();
    
    return true;
}

// Get partition info
String OTAManager::getPartitionInfo() const {
    String info = "{";
    
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* next = esp_ota_get_next_update_partition(nullptr);
    
    info += "\"current\":\"" + getCurrentPartition() + "\",";
    info += "\"current_size\":" + String(running ? running->size : 0) + ",";
    info += "\"next\":\"" + String(next ? next->label : "none") + "\",";
    info += "\"next_size\":" + String(next ? next->size : 0) + ",";
    info += "\"max_firmware_size\":" + String(getMaxFirmwareSize()) + ",";
    info += "\"can_rollback\":" + String(canRollback() ? "true" : "false");
    
    info += "}";
    return info;
}

// Web handlers
void OTAManager::handleOTAPage() {
    _webServer.send(200, "text/html", FPSTR(OTA_PAGE));
}

void OTAManager::handleOTAInfo() {
    String json = "{";
    json += "\"current_partition\":\"" + getCurrentPartition() + "\",";
    json += "\"available_space\":" + String(getAvailableSpace()) + ",";
    json += "\"max_firmware_size\":" + String(getMaxFirmwareSize()) + ",";
    json += "\"can_rollback\":" + String(canRollback() ? "true" : "false") + ",";
    json += "\"state\":\"" + String(getStateString()) + "\",";
    json += "\"progress\":" + String(_progress);
    json += "}";
    
    _webServer.send(200, "application/json", json);
}

void OTAManager::handleOTAURL() {
    if (!_webServer.hasArg("url")) {
        _webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Missing URL parameter\"}");
        return;
    }
    
    String url = _webServer.arg("url");
    
    // Start update in background (this will block until complete or error)
    bool success = updateFromURL(url);
    
    if (success) {
        _webServer.send(200, "application/json", "{\"success\":true,\"message\":\"Update started\"}");
    } else {
        _webServer.send(500, "application/json", 
                       "{\"success\":false,\"message\":\"" + getLastErrorString() + "\"}");
    }
}

void OTAManager::handleOTAStatus() {
    String json = "{";
    json += "\"state\":\"" + String(getStateString()) + "\",";
    json += "\"progress\":" + String(_progress) + ",";
    json += "\"error\":\"" + getLastErrorString() + "\"";
    json += "}";
    
    _webServer.send(200, "application/json", json);
}

void OTAManager::handleOTARollback() {
    bool success = rollback();
    
    if (success) {
        _webServer.send(200, "application/json", "{\"success\":true,\"message\":\"Rollback initiated\"}");
    } else {
        _webServer.send(500, "application/json", 
                       "{\"success\":false,\"message\":\"" + getLastErrorString() + "\"}");
    }
}

void OTAManager::handleOTAUpload() {
    // This is called after upload completes
    if (Update.hasError()) {
        String error = "{\"success\":false,\"message\":\"" + String(Update.errorString()) + "\"}";
        _webServer.send(500, "application/json", error);
        setState(OTAState::ERROR);
        setError(OTAError::FLASH_WRITE_FAILED);
    } else {
        _webServer.send(200, "application/json", "{\"success\":true,\"message\":\"Update successful\"}");
        setState(OTAState::SUCCESS);
        
        Serial.println("[OTA] Upload successful! Rebooting in 3 seconds...");
        delay(3000);
        reboot();
    }
}

void OTAManager::handleOTAUploadData() {
    // This is called for each chunk of uploaded data
    HTTPUpload& upload = _webServer.upload();
    
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("[OTA] Upload started: %s\n", upload.filename.c_str());
        
        _updateInProgress = true;
        _startTime = millis();
        _writtenSize = 0;
        _totalSize = 0; // Unknown for uploads
        _progress = 0;
        
        setState(OTAState::UPLOADING);
        
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Serial.printf("[OTA] Begin failed: %s\n", Update.errorString());
            setError(OTAError::OTA_BEGIN_FAILED);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Serial.printf("[OTA] Write failed: %s\n", Update.errorString());
            setError(OTAError::FLASH_WRITE_FAILED);
        } else {
            _writtenSize += upload.currentSize;
            
            // Update progress (approximate since we don't know total size)
            if (millis() - _lastProgressUpdate > 1000) {
                Serial.printf("[OTA] Uploaded: %d bytes\n", _writtenSize);
                _lastProgressUpdate = millis();
            }
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.printf("[OTA] Upload complete: %d bytes\n", upload.totalSize);
            _totalSize = upload.totalSize;
            _progress = 100;
        } else {
            Serial.printf("[OTA] End failed: %s\n", Update.errorString());
            setError(OTAError::OTA_END_FAILED);
        }
        
        _updateInProgress = false;
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Serial.println("[OTA] Upload aborted");
        Update.end();
        setError(OTAError::UNKNOWN);
        _updateInProgress = false;
    }
}
