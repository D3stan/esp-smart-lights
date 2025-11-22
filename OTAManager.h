#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <Update.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

/**
 * @brief OTA Manager - Gestione aggiornamenti firmware Over-The-Air
 * 
 * Questa classe gestisce gli aggiornamenti OTA del firmware con le seguenti funzionalità:
 * 
 * 1. **Aggiornamento da URL Remoto**: Scarica firmware da URL HTTP/HTTPS specificato
 * 2. **Upload Locale**: Carica firmware direttamente tramite interfaccia web
 * 3. **Rollback Automatico**: Utilizza le partizioni OTA per rollback in caso di errore
 * 4. **Preservazione Configurazione**: Le credenziali Wi-Fi e configurazioni vengono preservate
 * 5. **Progress Tracking**: Monitoraggio in tempo reale del progresso (0-100%)
 * 6. **Error Logging**: Gestione dettagliata degli errori con codici specifici
 * 
 * Il modulo si integra con WebServer esistente per fornire interfaccia web dedicata.
 */
class OTAManager {
public:
    /**
     * @brief Stato del processo OTA
     */
    enum class OTAState {
        IDLE,           // Pronto per aggiornamento
        CONNECTING,     // Connessione al server remoto
        DOWNLOADING,    // Download in corso da URL
        UPLOADING,      // Upload in corso da browser
        WRITING,        // Scrittura flash in corso
        VERIFYING,      // Verifica integrità
        REBOOTING,      // Riavvio in corso
        SUCCESS,        // Completato con successo
        ERROR           // Errore durante il processo
    };
    
    /**
     * @brief Codici di errore OTA
     */
    enum class OTAError {
        NONE,
        CONNECTION_REFUSED,
        TIMEOUT,
        DNS_FAILED,
        SSL_FAILED,
        HTTP_404,
        HTTP_500,
        INVALID_RESPONSE,
        FILE_TOO_LARGE,
        PARTITION_NOT_FOUND,
        FLASH_WRITE_FAILED,
        FLASH_VERIFY_FAILED,
        INSUFFICIENT_SPACE,
        OTA_BEGIN_FAILED,
        OTA_END_FAILED,
        ROLLBACK_FAILED,
        INVALID_URL,
        INVALID_FILE,
        UNKNOWN
    };
    
    /**
     * @brief Constructor
     * @param webServer Riferimento al WebServer esistente per integrare gli endpoint
     */
    explicit OTAManager(WebServer& webServer);
    
    /**
     * @brief Destructor
     */
    ~OTAManager();
    
    /**
     * @brief Inizializza il manager OTA
     * 
     * Registra gli endpoint web e prepara il sistema per gli aggiornamenti.
     * 
     * @return true se l'inizializzazione è riuscita
     */
    bool begin();
    
    /**
     * @brief Update loop - deve essere chiamato frequentemente
     * 
     * Gestisce il web server e lo stato degli aggiornamenti in background.
     */
    void update();
    
    /**
     * @brief Avvia aggiornamento da URL remoto
     * 
     * Scarica il firmware dall'URL specificato e lo installa.
     * Supporta sia HTTP che HTTPS (senza verifica certificati).
     * 
     * @param url URL del file firmware.bin
     * @return true se il processo è iniziato con successo
     */
    bool updateFromURL(const String& url);
    
    /**
     * @brief Ottieni lo stato corrente dell'aggiornamento
     * @return Stato corrente
     */
    OTAState getState() const { return _state; }
    
    /**
     * @brief Ottieni il nome dello stato corrente (per debug)
     * @return Nome dello stato come stringa
     */
    const char* getStateString() const;
    
    /**
     * @brief Ottieni il progresso dell'aggiornamento
     * @return Percentuale 0-100, o -1 se non applicabile
     */
    int getProgress() const { return _progress; }
    
    /**
     * @brief Ottieni l'ultimo errore
     * @return Codice errore
     */
    OTAError getLastError() const { return _lastError; }
    
    /**
     * @brief Ottieni la descrizione dell'ultimo errore
     * @return Stringa descrittiva dell'errore
     */
    String getLastErrorString() const;
    
    /**
     * @brief Ottieni il nome della partizione attualmente in esecuzione
     * @return Nome partizione (es. "OTA_0", "OTA_1", "factory")
     */
    String getCurrentPartition() const;
    
    /**
     * @brief Ottieni lo spazio disponibile per il firmware nella partizione OTA
     * @return Dimensione in bytes
     */
    size_t getAvailableSpace() const;
    
    /**
     * @brief Ottieni la dimensione massima del firmware supportata
     * @return Dimensione in bytes
     */
    size_t getMaxFirmwareSize() const;
    
    /**
     * @brief Verifica se è disponibile una partizione per il rollback
     * @return true se il rollback è possibile
     */
    bool canRollback() const;
    
    /**
     * @brief Forza il rollback manuale alla partizione precedente
     * 
     * Attenzione: Questo riavvierà il dispositivo!
     * 
     * @return true se il rollback è stato avviato
     */
    bool rollback();
    
    /**
     * @brief Ottieni informazioni dettagliate sulle partizioni OTA
     * @return Stringa JSON con informazioni
     */
    String getPartitionInfo() const;

private:
    // Web server reference
    WebServer& _webServer;
    
    // State
    OTAState _state;
    OTAError _lastError;
    int _progress;
    
    // Timing
    unsigned long _startTime;
    unsigned long _lastProgressUpdate;
    
    // Statistics
    size_t _totalSize;
    size_t _writtenSize;
    
    // Flags
    bool _updateInProgress;
    
    // HTTP client for downloads
    HTTPClient* _httpClient;
    
    // Helper methods
    void setState(OTAState state);
    void setError(OTAError error);
    void updateProgress(size_t current, size_t total);
    bool validateURL(const String& url);
    bool checkSpace(size_t requiredSize);
    void cleanup();
    
    // Web server handlers
    void setupWebHandlers();
    void handleOTAPage();
    void handleOTAInfo();
    void handleOTAUpload();
    void handleOTAUploadData();
    void handleOTAURL();
    void handleOTAStatus();
    void handleOTARollback();
    
    // OTA operations
    bool beginOTA(size_t size);
    bool writeOTA(uint8_t* data, size_t len);
    bool endOTA();
    void reboot();
    
    // Error code to string conversion
    static const char* errorToString(OTAError error);
};

#endif // OTA_MANAGER_H
