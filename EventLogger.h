#ifndef EVENT_LOGGER_H
#define EVENT_LOGGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

/**
 * @brief Event Logger - Sistema di logging per eventi LED
 * 
 * Questa classe gestisce un log circolare degli eventi di accensione/spegnimento
 * del LED con timestamp, memorizzato in RAM per prestazioni e persistenza opzionale.
 * 
 * Funzionalità:
 * - Log circolare con max MAX_LOG_ENTRIES eventi
 * - Timestamp UNIX (epoch)
 * - Retention di LOG_RETENTION_DAYS giorni
 * - Pulizia automatica dei log vecchi
 */
class EventLogger {
public:
    /**
     * @brief Struttura per un singolo evento
     */
    struct LogEntry {
        uint32_t timestamp;    // Unix timestamp (epoch)
        bool ledOn;            // true = accensione, false = spegnimento
        float lux;             // Valore lux al momento dell'evento
        bool motion;           // Stato movimento
        char mode[8];          // Modalità LED: "auto", "on", "off"
        
        LogEntry() : timestamp(0), ledOn(false), lux(0), motion(false) {
            strcpy(mode, "auto");
        }
    };
    
    /**
     * @brief Constructor
     */
    EventLogger();
    
    /**
     * @brief Inizializza il logger
     * @return true se inizializzato correttamente
     */
    bool begin();
    
    /**
     * @brief Aggiungi un nuovo evento al log
     * @param ledOn true per accensione, false per spegnimento
     * @param lux Valore lux corrente
     * @param motion Stato movimento corrente
     * @param mode Modalità LED corrente ("auto", "on", "off")
     */
    void logEvent(bool ledOn, float lux, bool motion, const char* mode = "auto");
    
    /**
     * @brief Ottieni il numero di eventi nel log
     * @return Numero di eventi
     */
    uint16_t getEventCount() const { return _count; }
    
    /**
     * @brief Ottieni un evento specifico (0 = più recente)
     * @param index Indice dell'evento (0-based)
     * @return Puntatore all'evento o nullptr se index non valido
     */
    const LogEntry* getEvent(uint16_t index) const;
    
    /**
     * @brief Ottieni tutti gli eventi in formato JSON
     * @return Stringa JSON con array di eventi
     */
    String getEventsJSON() const;
    
    /**
     * @brief Ottieni eventi delle ultime N ore
     * @param hours Numero di ore
     * @return Numero di eventi trovati
     */
    uint16_t getEventsLastHours(uint8_t hours) const;
    
    /**
     * @brief Cancella tutti gli eventi
     */
    void clearAll();
    
    /**
     * @brief Rimuovi eventi più vecchi di retention_days
     */
    void cleanOldEvents();
    
    /**
     * @brief Ottieni statistiche giornaliere
     * @return Numero di eventi oggi
     */
    uint16_t getTodayEventCount() const;

private:
    LogEntry _entries[MAX_LOG_ENTRIES];
    uint16_t _head;           // Indice prossima scrittura
    uint16_t _count;          // Numero di eventi (max MAX_LOG_ENTRIES)
    
    // Helper per gestione buffer circolare
    uint16_t getCircularIndex(uint16_t logicalIndex) const;
    bool isToday(uint32_t timestamp) const;
};

#endif // EVENT_LOGGER_H
