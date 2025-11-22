#include "EventLogger.h"
#include <time.h>

EventLogger::EventLogger()
    : _head(0)
    , _count(0)
{
}

bool EventLogger::begin() {
    Serial.println("EventLogger initialized");
    
    // Pulisci eventi vecchi all'avvio
    cleanOldEvents();
    
    return true;
}

void EventLogger::logEvent(bool ledOn, float lux, bool motion, const char* mode) {
    // Crea nuovo evento
    LogEntry& entry = _entries[_head];
    entry.timestamp = (uint32_t)time(nullptr);  // Unix timestamp
    entry.ledOn = ledOn;
    entry.lux = lux;
    entry.motion = motion;
    strncpy(entry.mode, mode, sizeof(entry.mode) - 1);
    entry.mode[sizeof(entry.mode) - 1] = '\0';
    
    // Avanza head (buffer circolare)
    _head = (_head + 1) % MAX_LOG_ENTRIES;
    
    // Incrementa count (max MAX_LOG_ENTRIES)
    if (_count < MAX_LOG_ENTRIES) {
        _count++;
    }
    
    Serial.print("Event logged: ");
    Serial.print(ledOn ? "LED ON" : "LED OFF");
    Serial.print(" | Lux: ");
    Serial.print(lux);
    Serial.print(" | Motion: ");
    Serial.print(motion ? "YES" : "NO");
    Serial.print(" | Mode: ");
    Serial.println(mode);
}

const EventLogger::LogEntry* EventLogger::getEvent(uint16_t index) const {
    if (index >= _count) {
        return nullptr;
    }
    
    uint16_t circularIndex = getCircularIndex(index);
    return &_entries[circularIndex];
}

uint16_t EventLogger::getCircularIndex(uint16_t logicalIndex) const {
    // Calcola l'indice nel buffer circolare
    // logicalIndex 0 = evento più recente
    if (_count < MAX_LOG_ENTRIES) {
        // Buffer non ancora pieno
        if (logicalIndex >= _count) return 0;
        return (_count - 1 - logicalIndex);
    } else {
        // Buffer pieno (circolare)
        int16_t idx = _head - 1 - logicalIndex;
        while (idx < 0) idx += MAX_LOG_ENTRIES;
        return idx % MAX_LOG_ENTRIES;
    }
}

String EventLogger::getEventsJSON() const {
    String json = "{\"logs\":[";
    
    for (uint16_t i = 0; i < _count; i++) {
        const LogEntry* entry = getEvent(i);
        if (!entry) continue;
        
        if (i > 0) json += ",";
        
        json += "{";
        json += "\"timestamp\":" + String(entry->timestamp) + ",";
        json += "\"event\":\"" + String(entry->ledOn ? "on" : "off") + "\",";
        json += "\"lux\":" + String(entry->lux, 1) + ",";
        json += "\"motion\":" + String(entry->motion ? "true" : "false") + ",";
        json += "\"mode\":\"" + String(entry->mode) + "\"";
        json += "}";
    }
    
    json += "],";
    json += "\"total\":" + String(_count);
    json += "}";
    
    return json;
}

uint16_t EventLogger::getEventsLastHours(uint8_t hours) const {
    uint32_t now = (uint32_t)time(nullptr);
    uint32_t cutoff = now - (hours * 3600);
    
    uint16_t count = 0;
    for (uint16_t i = 0; i < _count; i++) {
        const LogEntry* entry = getEvent(i);
        if (entry && entry->timestamp >= cutoff) {
            count++;
        }
    }
    
    return count;
}

void EventLogger::clearAll() {
    _head = 0;
    _count = 0;
    Serial.println("All events cleared");
}

void EventLogger::cleanOldEvents() {
    uint32_t now = (uint32_t)time(nullptr);
    uint32_t cutoff = now - (LOG_RETENTION_DAYS * 24 * 3600);
    
    // Conta quanti eventi da rimuovere
    uint16_t toRemove = 0;
    for (uint16_t i = _count - 1; i > 0; i--) {  // Parti dal più vecchio
        const LogEntry* entry = getEvent(i);
        if (entry && entry->timestamp < cutoff) {
            toRemove++;
        } else {
            break;  // Gli eventi sono ordinati, possiamo fermarci
        }
    }
    
    if (toRemove > 0) {
        _count -= toRemove;
        Serial.print("Cleaned ");
        Serial.print(toRemove);
        Serial.println(" old events");
    }
}

uint16_t EventLogger::getTodayEventCount() const {
    uint16_t count = 0;
    for (uint16_t i = 0; i < _count; i++) {
        const LogEntry* entry = getEvent(i);
        if (entry && isToday(entry->timestamp)) {
            count++;
        }
    }
    return count;
}

bool EventLogger::isToday(uint32_t timestamp) const {
    time_t now = time(nullptr);
    time_t then = timestamp;
    
    struct tm* nowTm = localtime(&now);
    struct tm* thenTm = localtime(&then);
    
    return (nowTm->tm_year == thenTm->tm_year &&
            nowTm->tm_yday == thenTm->tm_yday);
}
