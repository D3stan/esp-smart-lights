#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <vector>

/**
 * @brief WiFi Manager - Gestione connessione Wi-Fi con captive portal
 * 
 * Questa classe gestisce la connettività Wi-Fi del dispositivo con le seguenti funzionalità:
 * 
 * 1. **Persistenza Credenziali**: Le credenziali Wi-Fi (SSID, password) vengono salvate
 *    nella memoria non volatile (Preferences) e caricate automaticamente all'avvio.
 * 
 * 2. **Modalità Station**: Tenta di connettersi alla rete Wi-Fi configurata.
 *    Se la connessione fallisce entro il timeout, passa automaticamente in modalità AP.
 * 
 * 3. **Modalità AP (Access Point)**: Crea un hotspot Wi-Fi per la configurazione.
 *    In questa modalità viene attivato un captive portal che permette di:
 *    - Scansionare le reti disponibili
 *    - Inserire SSID e password
 *    - Configurare l'intervallo di riconnessione
 *    - Visualizzare eventuali errori di connessione
 * 
 * 4. **Reconnection Automatica**: Se la connessione viene persa, il sistema riprova
 *    automaticamente a connettersi secondo l'intervallo configurato.
 * 
 * 5. **Factory Reset**: Tenendo premuto il pulsante configurato per un tempo definito,
 *    è possibile resettare le credenziali Wi-Fi e tornare alla modalità di configurazione.
 * 
 * @note La classe utilizza le costanti definite in config.h per la configurazione.
 */
class WiFiManager {
public:
    /**
     * @brief Stato della connessione Wi-Fi
     */
    enum class ConnectionState {
        DISCONNECTED,       // Non connesso
        CONNECTING,         // Tentativo di connessione in corso
        CONNECTED,          // Connesso con successo
        AP_MODE,           // Modalità Access Point attiva
        CONNECTION_FAILED, // Connessione fallita
        RECONNECTING       // Riconnessione in corso
    };
    
    /**
     * @brief Constructor
     */
    WiFiManager();
    
    /**
     * @brief Destructor
     */
    ~WiFiManager();
    
    /**
     * @brief Inizializza il WiFi manager
     * 
     * Carica le credenziali salvate e tenta la connessione.
     * Se non ci sono credenziali salvate o la connessione fallisce,
     * attiva la modalità AP con captive portal.
     * 
     * @return true se l'inizializzazione è riuscita
     */
    bool begin();
    
    /**
     * @brief Update loop - deve essere chiamato frequentemente
     * 
     * Gestisce:
     * - Riconnessione automatica se disconnesso
     * - Server web per il captive portal
     * - DNS server per il redirect del captive portal
     * - Rilevamento pulsante per factory reset
     */
    void update();
    
    /**
     * @brief Ottieni lo stato corrente della connessione
     * @return Stato corrente
     */
    ConnectionState getState() const { return _state; }
    
    /**
     * @brief Ottieni il nome dello stato corrente (per debug)
     * @return Nome dello stato come stringa
     */
    const char* getStateString() const;
    
    /**
     * @brief Verifica se è connesso a una rete Wi-Fi
     * @return true se connesso
     */
    bool isConnected() const { return _state == ConnectionState::CONNECTED; }
    
    /**
     * @brief Verifica se è in modalità AP
     * @return true se in modalità AP
     */
    bool isAPMode() const { return _state == ConnectionState::AP_MODE; }
    
    /**
     * @brief Ottieni l'indirizzo IP corrente
     * @return IP address (in Station mode) o AP IP (in AP mode)
     */
    IPAddress getIPAddress() const;
    
    /**
     * @brief Ottieni l'SSID a cui è connesso (o quello configurato)
     * @return SSID corrente
     */
    String getSSID() const { return _ssid; }
    
    /**
     * @brief Ottieni l'hostname del dispositivo
     * @return Hostname
     */
    String getHostname() const { return _hostname; }
    
    /**
     * @brief Ottieni la potenza del segnale RSSI
     * @return RSSI in dBm (disponibile solo quando connesso)
     */
    int32_t getRSSI() const;
    
    /**
     * @brief Salva nuove credenziali Wi-Fi
     * @param ssid SSID della rete
     * @param password Password della rete
     * @return true se salvate con successo
     */
    bool saveCredentials(const String& ssid, const String& password);
    
    /**
     * @brief Imposta l'intervallo di riconnessione
     * @param intervalMs Intervallo in millisecondi
     */
    void setRetryInterval(unsigned long intervalMs);
    
    /**
     * @brief Ottieni l'intervallo di riconnessione configurato
     * @return Intervallo in millisecondi
     */
    unsigned long getRetryInterval() const { return _retryIntervalMs; }
    
    /**
     * @brief Resetta le credenziali Wi-Fi (factory reset)
     * 
     * Cancella SSID e password salvati e riavvia in modalità AP.
     */
    void resetCredentials();
    
    /**
     * @brief Forza la riconnessione
     * 
     * Disconnette e tenta immediatamente di riconnettersi.
     */
    void reconnect();
    
    /**
     * @brief Ottieni l'ultimo errore di connessione
     * @return Descrizione dell'errore
     */
    String getLastError() const { return _lastError; }
    
    /**
     * @brief Verifica se ci sono credenziali salvate
     * @return true se esistono credenziali salvate
     */
    bool hasStoredCredentials() const;
    
    /**
     * @brief Ottieni il tempo rimasto prima del prossimo tentativo di riconnessione
     * @return Millisecondi rimanenti, 0 se non in attesa
     */
    unsigned long getReconnectTimeRemaining() const;
    
    /**
     * @brief Imposta i riferimenti ai componenti per l'API dashboard
     * @param controller Puntatore allo SmartLightController
     * @param lightSensor Puntatore al LightSensor
     * @param motionDetector Puntatore al MotionDetector
     * @param ledController Puntatore al LEDController
     * @param eventLogger Puntatore all'EventLogger
     */
    void setSystemComponents(void* controller, void* lightSensor, 
                              void* motionDetector, void* ledController,
                              void* eventLogger, uint8_t* rgbBrightness = nullptr);    /**
     * @brief Ottieni il riferimento al WebServer interno
     * @return Puntatore al WebServer (può essere nullptr se non inizializzato)
     */
    WebServer* getWebServer() { return _webServer; }

private:
    // Preferences storage
    Preferences _preferences;
    
    // Web server and DNS for captive portal
    WebServer* _webServer;
    DNSServer* _dnsServer;
    
    // Connection state
    ConnectionState _state;
    String _ssid;
    String _password;
    String _hostname;
    String _lastError;
    
    // Timing
    unsigned long _connectionStartTime;
    unsigned long _lastReconnectAttempt;
    unsigned long _retryIntervalMs;
    unsigned long _resetButtonPressStart;
    
    // Flags
    bool _apModeActive;
    bool _resetButtonPressed;
    
    // System component references (for API)
    void* _smartLightController;
    void* _lightSensor;
    void* _motionDetector;
    void* _ledController;
    void* _eventLogger;
    uint8_t* _rgbBrightness;  // Pointer to RGB brightness variable
    
    // Helper methods
    bool loadCredentials();
    void startStationMode();
    void startAPMode();
    void stopAPMode();
    void checkConnection();
    void handleReconnection();
    void checkResetButton();
    
    // Web server handlers
    void setupWebServer();
    void handleRoot();
    void handleScan();
    void handleSave();
    void handleStatus();
    void handleNotFound();
    void handleDashboard();
    void handleLogs();
    void handleApiStatus();
    void handleApiConfig();
    void handleApiConfigPost();
    void handleApiLedOverride();
    void handleApiBrightnessGet();
    void handleApiBrightness();
    void handleApiLogs();
    void handleApiLogsDelete();
    void handleApiBypassGet();
    void handleApiBypassLight();
    void handleApiBypassMovement();
    void handleApiTimeWindowGet();
    void handleApiTimeWindowPost();
    void handleApiTimeWindowEnable();
    void handleApiTimeWindowInvert();
    
    // HTML pages (stored in PROGMEM to save RAM)
    static const char* getConfigPageHTML();
    static const char* getSuccessPageHTML();
    static const char* getStatusPageJSON(const String& status, const String& message);
};

#endif // WIFI_MANAGER_H
