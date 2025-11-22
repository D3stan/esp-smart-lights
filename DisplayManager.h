#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "WiFiManager.h"
#include "LightSensor.h"
#include "MotionDetector.h"
#include "LEDController.h"

/**
 * @brief Display Manager - Gestione del display TFT
 * 
 * Questa classe gestisce la visualizzazione delle informazioni sul display TFT integrato.
 * 
 * Layout del display (128x128 pixel):
 * - Area superiore: Stato Wi-Fi e IP
 * - Area centrale: Stato sensori (Lux, Movimento)
 * - Area inferiore: Stato LED e sistema
 * 
 * La classe supporta:
 * - Aggiornamento automatico delle informazioni
 * - Visualizzazione di icone e simboli
 * - Gestione del backlight
 * - Layout ottimizzato per display piccolo
 */
class DisplayManager {
public:
    /**
     * @brief Constructor
     * @param tft Reference to TFT display instance
     */
    explicit DisplayManager(Adafruit_ST7735& tft);
    
    /**
     * @brief Inizializza il display manager
     * @param updateIntervalMs Intervallo di aggiornamento in millisecondi (default: 1000ms)
     */
    void begin(unsigned long updateIntervalMs = 1000);
    
    /**
     * @brief Update loop - aggiorna il display se necessario
     * 
     * Questo metodo controlla se è il momento di aggiornare il display
     * e aggiorna solo le informazioni cambiate per ridurre flickering.
     * 
     * @param wifiManager Reference al WiFiManager per stato connessione
     * @param lightSensor Reference al LightSensor per valore lux
     * @param motionDetector Reference al MotionDetector per stato movimento
     * @param ledController Reference al LEDController per stato LED
     */
    void update(
        const WiFiManager& wifiManager,
        const LightSensor& lightSensor,
        const MotionDetector& motionDetector,
        const LEDController& ledController
    );
    
    /**
     * @brief Forza l'aggiornamento completo del display
     */
    void forceUpdate();
    
    /**
     * @brief Mostra un messaggio di benvenuto
     */
    void showWelcomeScreen();
    
    /**
     * @brief Mostra un messaggio di errore
     * @param errorMsg Messaggio di errore da visualizzare
     */
    void showError(const String& errorMsg);
    
    /**
     * @brief Pulisce il display
     */
    void clear();
    
    /**
     * @brief Abilita/disabilita il backlight
     * @param enabled true per accendere, false per spegnere
     */
    void setBacklight(bool enabled);
    
    /**
     * @brief Imposta l'intervallo di aggiornamento
     * @param intervalMs Intervallo in millisecondi
     */
    void setUpdateInterval(unsigned long intervalMs) { _updateIntervalMs = intervalMs; }
    
    /**
     * @brief Ottieni l'intervallo di aggiornamento corrente
     * @return Intervallo in millisecondi
     */
    unsigned long getUpdateInterval() const { return _updateIntervalMs; }

private:
    // TFT display reference
    Adafruit_ST7735& _tft;
    
    // Update timing
    unsigned long _updateIntervalMs;
    unsigned long _lastUpdate;
    
    // Previous values for change detection
    String _prevWiFiState;
    String _prevIP;
    float _prevLux;
    bool _prevMoving;
    bool _prevLEDOn;
    
    // Display areas (Y coordinates)
    static constexpr uint8_t AREA_HEADER_Y = 0;
    static constexpr uint8_t AREA_HEADER_HEIGHT = 30;
    static constexpr uint8_t AREA_SENSORS_Y = 32;
    static constexpr uint8_t AREA_SENSORS_HEIGHT = 60;
    static constexpr uint8_t AREA_STATUS_Y = 94;
    static constexpr uint8_t AREA_STATUS_HEIGHT = 34;
    
    // Colors
    static constexpr uint16_t COLOR_BACKGROUND = ST77XX_BLACK;
    static constexpr uint16_t COLOR_TEXT = ST77XX_WHITE;
    static constexpr uint16_t COLOR_CONNECTED = ST77XX_GREEN;
    static constexpr uint16_t COLOR_DISCONNECTED = ST77XX_RED;
    static constexpr uint16_t COLOR_AP_MODE = ST77XX_YELLOW;
    static constexpr uint16_t COLOR_CONNECTING = ST77XX_CYAN;
    static constexpr uint16_t COLOR_HEADER_BG = 0x2124; // Dark gray
    static constexpr uint16_t COLOR_SENSOR_BG = 0x1082; // Darker gray
    
    // Helper methods
    void drawHeader(const WiFiManager& wifiManager);
    void drawSensors(const LightSensor& lightSensor, const MotionDetector& motionDetector);
    void drawStatus(const LEDController& ledController);
    void drawWiFiIcon(uint8_t x, uint8_t y, uint16_t color);
    void drawCenteredText(const String& text, uint8_t y, uint8_t fontSize, uint16_t color);
    uint16_t getWiFiStateColor(WiFiManager::ConnectionState state);
    String formatIP(const IPAddress& ip);
    String shortenSSID(const String& ssid, uint8_t maxLen = 16);
};

#endif // DISPLAY_MANAGER_H
