# 💡 Documento di Progetto: Luci Smart per Robot Tosaerba
**Piattaforma di Sviluppo:** Arduino IDE (Arduino Framework)
**Microcontrollore:** ESP32-S3FH4R2 (Wemos S3 Mini Pro)

---

## 1. Introduzione e Obiettivo del Progetto

L'obiettivo di questo progetto è implementare un **sistema di illuminazione "smart" e autonomo** per un robot tosaerba da giardino. Il sistema deve accendere una striscia LED esterna solo quando vengono soddisfatte due condizioni fondamentali:

* **Condizione Ambientale (Notte):** L'illuminazione ambientale è bassa, indicando che è notte o crepuscolo.
* **Condizione Operativa (Movimento):** L'unità robotica è in movimento, suggerendo che sta per iniziare o è in corso l'operazione di taglio dell'erba.

Il sistema dovrà inoltre offrire **connettività Wi-Fi** per il monitoraggio e il controllo remoto, con l'ambizione di integrarsi con piattaforme di domotica esistenti (Google Home).

### 1.1. Principi di Sviluppo Software

Il progetto deve seguire rigorosamente i seguenti principi:

* **Programmazione Orientata agli Oggetti (OOP):** Ogni componente hardware e logico deve essere incapsulato in classi dedicate con interfacce chiare.
* **C++ Moderno:** Utilizzo di feature C++11/14/17 (dove supportato dall'Arduino Framework):
  * `constexpr` per costanti compile-time
  * `enum class` per type-safe enumerations
  * Reference semantics (`const T&`) per evitare copie non necessarie
  * Inizializzazione uniforme con `{}`
  * `nullptr` invece di `NULL`
* **No Allocazione Dinamica Non Necessaria:** Evitare `new`/`delete` e heap allocation quando possibile, preferendo oggetti stack-allocated e composizione.
* **RAII (Resource Acquisition Is Initialization):** Gestione automatica delle risorse tramite costruttori/distruttori.
* **Separazione Responsabilità:** Ogni classe ha una singola responsabilità ben definita (Single Responsibility Principle).

---

## 2. Specifiche della Piattaforma Hardware

Il progetto sarà basato sulla scheda di sviluppo **Wemos S3 Mini Pro**, che offre un potente set di funzionalità IoT in un formato compatto.

| Componente | Specifiche Chiave | Funzione nel Progetto |
| :--- | :--- | :--- |
| **Microcontrollore** | ESP32-S3FH4R2 | Controllo logico centrale, Wi-Fi, gestione I/O e periferiche. |
| **Connettività** | 2.4 GHz Wi-Fi, Bluetooth LE | Connessione alla rete domestica, integrazione IoT/Web Server. |
| **Memoria** | 4MB Flash, 2MB PSRAM | Archiviazione del firmware, gestione del buffer per il web server. |
| **Display** | 0.85" 128x128 LCD TFT (ST7735) | Debug, visualizzazione dello stato di connessione e del livello di luce/movimento. **Nota:** Il display integrato è attualmente gestito con la libreria `Adafruit_ST7735`. Originariamente si prevedeva l'uso di `TFT_eSPI` con driver GC9107/GC9A01, ma tale migrazione è rimandata a fase successiva. |
| **Sensore di Movimento** | 6D MEMS IMU (QMI8658C) | Rilevamento del movimento (accelerazione/rotazione) del robot. |
| **GPIO Disponibili** | 12x IO, 3x Button (IO0, IO47, IO48) | Connessione a sensori esterni (BH1750) e attuatori (IRLZ44N/LED). |
| **Periferiche** | ADC, DAC, I2C, SPI, UART, USB OTG | Interfaccia I2C utilizzata per la comunicazione con il BH1750. |

---

## 3. Componenti Periferiche e Cablaggio

| Componente | Tipo di Interfaccia | Scopo | Cablaggio Wemos S3 Mini Pro |
| :--- | :--- | :--- | :--- |
| **Sensore di Luminosità** | I2C (BH1750) | Rilevamento del livello di lux ambientale. | SDA (GPIO), SCL (GPIO) |
| **Striscia LED** | Attuatore PWM | Sorgente luminosa (5V). | Alimentazione esterna (Powerbank) |
| **MOSFET** | PWM (IRLZ44N) | Interruttore di potenza a bassa soglia per la striscia LED. | Gate (GPIO configurato PWM, es. IO10), Source (GND), Drain (LED Strip GND) |
| **Alimentazione** | Powerbank | Fornitura di alimentazione 5V stabile al Wemos e alla striscia LED. | VCC 5V |

> **Nota sul MOSFET:** L'**IRLZ44N** è un **MOSFET Logic-Level**, ideale per essere pilotato direttamente dalle uscite logiche a 3.3V dell'ESP32-S3. La striscia LED a 5V sarà alimentata direttamente dal powerbank, con il MOSFET che commuta il polo negativo (**GND**).

---

## 4. Architettura Funzionale e Logica Operativa

La logica del sistema si basa sulla fusione dei dati provenienti da due sensori: il **BH1750** e l'**IMU integrata**.

### 4.1. Diagramma a Blocchi Funzionale


### 4.2. Algoritmo di Accensione della Luce

1.  **Inizializzazione:** Connessione a Wi-Fi, calibrazione dell'IMU (determinazione dello stato di riposo) e inizializzazione del BH1750.
2.  **Monitoraggio Luminosità:** Leggere il valore in **Lux** dal BH1750.
3.  **Condizione Notte:** Se $Lux < Soglia\_Notte$ (es. 5-10 Lux), impostare $Stato\_Luce\_Ambiente = VERO$.
4.  **Monitoraggio Movimento:** Leggere l'accelerazione totale (e/o variazione angolare) dall'IMU **QMI8658C**.
5.  **Condizione Movimento:** Se $Accelerazione\_Totale > Soglia\_Movimento$ per un periodo minimo (es. 1 secondo), impostare $Stato\_Movimento = VERO$.
6.  **Logica di Controllo Principale:**
    * SE ($Stato\_Luce\_Ambiente$ è VERO) E ($Stato\_Movimento$ è VERO):
        * Accendere la striscia LED usando il **PWM** (es. 255/255) per la massima luminosità.
    * ALTRIMENTI:
        * Spegnere la striscia LED (PWM a 0).
7.  **Modalità Standby:** Se il robot si ferma ($Stato\_Movimento = FALSO$), la luce rimane accesa per un breve periodo di **debounce** (es. 30 secondi) prima di spegnersi, per evitare lo sfarfallio.

---

## 5. Stack Software e Pianificazione dello Sviluppo

Lo sviluppo avverrà tramite **Arduino IDE** con l'**Arduino Framework**, sfruttando le librerie specifiche per le periferiche e la connettività.

### 5.1. Librerie Necessarie

| Funzionalità | Libreria (Esempi) | Note |
| :--- | :--- | :--- |
| Gestione I2C | `Wire` (inclusa) | Standard I2C per BH1750. |
| Sensore BH1750 | `BH1750` | Per la lettura dei Lux. |
| IMU QMI8658C | Libreria specifica per la QMI8658C (potrebbe richiedere una libreria I2C personalizzata o l'uso di registri grezzi). | Rilevazione dell'accelerazione per il movimento. |
| Connettività Wi-Fi | `WiFi` (inclusa) | Gestione della connessione di rete. |
| Controllo PWM | `ledcSetup`, `ledcAttachPin` (Funzioni native ESP32) | Per modulare l'intensità luminosa del LED. |
| Integrazione IoT | `ESPAsyncWebServer`, `AsyncTCP` | Per la pagina web locale e la gestione asincrona. |
| Integrazione Google Home | `SinricPro`, `FauxmoESP` o integrazione MQTT personalizzata. | Implementazione del bridge per la domotica. |
| Display LCD | `TFT_eSPI` (Adattato per GC9107/GC9A01) | Per il feedback visivo e il debug. |

### 5.2. Fasi di Sviluppo (Roadmap)

1.  **Fase 1: Configurazione di Base (Arduino IDE e I/O)**
    * Configurazione del progetto Arduino IDE per Wemos S3 Mini Pro.
    * Test PWM sul pin del MOSFET (IRLZ44N).
    * Test di lettura dal BH1750 via I2C.
2.  **Fase 2: Logica Sensori e Firmware Core**
    * Implementazione del driver IMU e calibrazione della soglia di movimento.
    * Sviluppo dell'algoritmo di accensione basato sulla logica **AND** (Notte E Movimento).
    * Implementazione del debounce (ritardo allo spegnimento).
3.  **Fase 3: Connettività e Debug**
    * Implementazione della connessione Wi-Fi persistente.
    * Utilizzo del display LCD per mostrare lo stato (IP, Lux, Movimento).
4.  **Fase 4: Integrazione IoT (Opzionale/Alternativa)**
    * Implementazione dell'integrazione Google Home (priorità) **OPPURE**
    * Sviluppo della pagina Web Server semplice per monitoraggio/override manuale.

---

## 6. Integrazione e Connettività

### 6.1. Priorità: Integrazione con Google Home

L'integrazione con Google Home richiede un bridge software. Le opzioni da esplorare sono:

* **FauxmoESP:** Emula un dispositivo Philips Hue. Metodo semplice che non richiede servizi cloud esterni, ma è spesso limitato alle funzioni On/Off/Luminosità.
* **Servizio MQTT con Bridge:** Utilizzare un broker MQTT locale o cloud (es. Mosquitto) e un bridge come Node-RED o Home Assistant per esporre il dispositivo a Google Home. Offre la massima flessibilità.
* **SinricPro / IFTTT:** Piattaforme cloud che forniscono un ponte tra ESP32 e assistenti vocali, richiedendo una chiave API.

### 6.2. Alternativa: Pagina Web Server Semplice

Se l'integrazione diretta con Google Home si rivela complessa, verrà implementato un semplice **Web Server asincrono** (`ESPAsyncWebServer`) sull'ESP32-S3.

**Funzionalità del Web Server:**

* Visualizzazione dello stato attuale (**Luminosità in Lux**, Stato Movimento, Stato Luce ON/OFF).
* Campo per impostare la **Soglia\_Notte** (Lux).
* Pulsante per l'**Override Manuale** (Forza ON/Forza OFF).

---

## 8. Fase 3: Connettività Wi-Fi e Debug Visivo - Implementazione Completata

### 8.1. Architettura del Sistema Wi-Fi

Il sistema Wi-Fi implementato è basato su tre componenti principali che lavorano in sinergia:

#### 8.1.1. WiFiManager (WiFiManager.h/cpp)

La classe `WiFiManager` è il cuore della gestione della connettività e implementa un sistema robusto e user-friendly per la configurazione e il mantenimento della connessione Wi-Fi.

**Funzionalità Principali:**

1. **Persistenza delle Credenziali**
   - Le credenziali Wi-Fi (SSID e password) vengono salvate nella memoria non volatile ESP32 utilizzando la libreria `Preferences`
   - Namespace utilizzato: `wifi_config` (configurabile in `config.h`)
   - Le credenziali vengono caricate automaticamente all'avvio
   - Factory reset disponibile tramite pressione prolungata del pulsante centrale (5 secondi)

2. **Modalità Operativa Dual-Mode**

   **Modalità Station (STA):**
   - Il dispositivo si connette alla rete Wi-Fi domestica esistente
   - Timeout di connessione configurabile (default: 15 secondi)
   - Hostname mDNS: `Centralina-Luci.local` per un facile accesso in rete locale
   - Se la connessione fallisce, il sistema passa automaticamente in modalità AP

   **Modalità Access Point (AP):**
   - Il dispositivo crea un hotspot Wi-Fi per la configurazione
   - SSID predefinito: `Centralina-Luci-Setup`
   - Password AP: `setup123` (configurabile, o disabilitabile per rete aperta)
   - IP statico AP: assegnato automaticamente da ESP32 (tipicamente 192.168.4.1)

3. **Captive Portal per Configurazione**
   - Quando il dispositivo è in modalità AP, viene attivato un **DNS server** che reindirizza tutte le richieste DNS verso l'IP del dispositivo
   - Questo crea un "captive portal" che si apre automaticamente quando ci si connette alla rete
   - L'utente viene guidato attraverso un'interfaccia web per:
     * Scansionare le reti Wi-Fi disponibili
     * Selezionare l'SSID dalla lista o inserirlo manualmente
     * Inserire la password della rete
     * Configurare l'intervallo di riconnessione automatica
   - La pagina web è completamente embedded nel firmware (stored in PROGMEM) e non richiede file esterni

4. **Riconnessione Automatica**
   - Se la connessione viene persa, il sistema riprova automaticamente a connettersi
   - Intervallo configurabile tra i tentativi (default: 5 minuti, modificabile via web portal)
   - Durante l'attesa, il dispositivo continua a funzionare offline con i sensori locali
   - Il countdown al prossimo tentativo è visualizzabile sul display e via seriale

5. **Gestione Errori Avanzata**
   - Il sistema rileva e memorizza l'ultimo errore di connessione:
     * `WL_CONNECT_FAILED`: Password errata o SSID non trovato
     * `WL_NO_SSID_AVAIL`: Rete non disponibile
     * `Connection timeout`: Router irraggiungibile
   - Gli errori vengono mostrati sul display TFT e nella pagina web di configurazione
   - Log dettagliato via Serial Monitor per debugging

#### 8.1.2. DisplayManager (DisplayManager.h/cpp)

La classe `DisplayManager` gestisce la visualizzazione delle informazioni sul display TFT ST7735 integrato (0.85", 128x128 pixel).

**Layout del Display:**

Il display è diviso in tre aree funzionali:

```
┌────────────────────────────┐
│  AREA HEADER (30px)        │
│  Wi-Fi Icon + Stato        │
│  IP Address / AP Info      │
├────────────────────────────┤
│  AREA SENSORI (60px)       │
│  Luce: XX lux (Notte)      │
│  Movimento: ATTIVO/FERMO   │
├────────────────────────────┤
│  AREA STATUS (34px)        │
│  LED Strip: ON/OFF         │
└────────────────────────────┘
```

**Funzionalità:**

1. **Aggiornamento Intelligente**
   - Il display si aggiorna solo quando i dati cambiano, riducendo flickering
   - Frequenza di aggiornamento configurabile (default: 1 secondo)
   - Ogni area viene ridisegnata indipendentemente per ottimizzare le prestazioni

2. **Visualizzazione Wi-Fi**
   - Icona Wi-Fi colorata in base allo stato:
     * Verde: Connesso
     * Giallo: Modalità AP
     * Ciano: Connessione in corso
     * Rosso: Disconnesso/Errore
   - SSID della rete connessa (accorciato se troppo lungo)
   - Indirizzo IP o stato della connessione
   - Countdown per la riconnessione se disconnesso

3. **Visualizzazione Sensori**
   - Livello di luce in lux con precisione decimale
   - Indicatore Notte/Giorno
   - Stato movimento con icona animata (cerchio verde = movimento, cerchio rosso = fermo)

4. **Visualizzazione LED**
   - Stato LED strip (ACCESO/SPENTO)
   - Livello di luminosità quando acceso

5. **Schermata di Benvenuto**
   - Mostrata all'avvio con il nome del dispositivo
   - Transizione fluida verso la schermata principale

#### 8.1.3. Pagina Web di Configurazione

La pagina web embedded offre un'interfaccia moderna e responsive:

**Design e UX:**
- Design moderno con gradiente viola/blu
- Completamente responsive (funziona su smartphone, tablet, desktop)
- Font system nativi per caricamento istantaneo
- Animazioni fluide e feedback visivo

**Funzionalità:**
- **Scansione Automatica**: La pagina esegue automaticamente la scansione delle reti all'apertura
- **Lista Reti**: Mostra tutte le reti trovate con:
  * Nome SSID
  * Potenza del segnale (RSSI in dBm)
  * Indicatore di cifratura (🔒 per reti protette, 🔓 per reti aperte)
- **Input Manuale**: Possibilità di inserire SSID manualmente per reti nascoste
- **Configurazione Avanzata**: Campo per impostare l'intervallo di riconnessione (30-3600 secondi)
- **Validazione Client-Side**: Controlli JavaScript per input validi prima dell'invio
- **Feedback Visivo**: Loading spinner durante la connessione, messaggi di successo/errore
- **Auto-Close**: La pagina si chiude automaticamente dopo la configurazione riuscita

### 8.2. Configurazione e Costanti (config.h)

Tutte le configurazioni Wi-Fi sono centralizzate in `config.h`:

### 8.3. Flusso Operativo del Sistema Wi-Fi

#### All'Avvio:

1. **Inizializzazione Preferences**: Apertura del namespace per leggere le credenziali salvate
2. **Verifica Credenziali**:
   - Se credenziali presenti → Avvia modalità **Station** e tenta la connessione
   - Se credenziali assenti → Avvia modalità **AP** con captive portal
3. **Tentativo di Connessione** (se in modalità Station):
   - Timeout: 15 secondi
   - Se successo → Stampa IP e transita a stato CONNECTED
   - Se fallisce → Passa in modalità AP per configurazione

#### Durante l'Esecuzione:

**Loop di Update (chiamato in `loop()`):**

**Gestione Stati:**

1. **CONNECTED**: Monitora continuamente lo stato della connessione
2. **DISCONNECTED/FAILED**: Avvia countdown per riconnessione automatica
3. **RECONNECTING**: Riprova connessione dopo intervallo configurato
4. **AP_MODE**: Gestisce web server e DNS server per captive portal

#### Factory Reset:

- Tenere premuto il pulsante **centrale** (BTN_C) per **5 secondi**
- Il sistema cancella tutte le credenziali salvate
- Riavvia automaticamente in modalità AP per nuova configurazione
- LED RGB può lampeggiare per conferma visiva

### 8.4. Comandi Seriali Aggiunti

Nuovi comandi disponibili via Serial Monitor:

```
W - Mostra stato dettagliato Wi-Fi (stato, SSID, IP, RSSI, errori)
X - Factory reset credenziali Wi-Fi (entra in modalità AP)
Z - Forza riconnessione immediata (bypassa countdown)
```

### 8.5. Integrazione con il Sistema Esistente

Il WiFiManager è completamente integrato con il resto del sistema:

- **Non bloccante**: Tutte le operazioni sono asincrone, il controllo LED continua anche senza Wi-Fi
- **Modalità Offline**: Il robot funziona normalmente anche senza connessione (sensori e LED operativi)
- **Display in Tempo Reale**: Lo stato Wi-Fi è sempre visibile sul TFT
- **Logging Completo**: Tutti gli eventi Wi-Fi sono loggati via Serial per debugging

### 8.6. Librerie Richieste

Per compilare il progetto con le nuove funzionalità, assicurarsi di avere installate:

| Libreria | Versione | Note |
|----------|----------|------|
| `WiFi` | Built-in ESP32 | Gestione Wi-Fi |
| `WebServer` | Built-in ESP32 | Web server HTTP |
| `DNSServer` | Built-in ESP32 | DNS per captive portal |
| `Preferences` | Built-in ESP32 | Storage non volatile |
| `Adafruit_GFX` | Latest | Grafiche per TFT |
| `Adafruit_ST7735` | Latest | Driver TFT |

### 8.7. Test e Debugging

**Scenario di Test Consigliati:**

1. **Prima Configurazione**:
   - Flash del firmware su ESP32 nuovo (senza credenziali)
   - Verifica attivazione AP mode automatica
   - Connessione alla rete AP e test captive portal
   - Configurazione rete e verifica salvataggio

2. **Riconnessione Automatica**:
   - Disconnessione del router Wi-Fi
   - Verifica tentativo di riconnessione
   - Riattivazione router e verifica reconnect automatico

3. **Factory Reset**:
   - Pressione prolungata pulsante centrale
   - Verifica cancellazione credenziali
   - Verifica ritorno a modalità AP

4. **Display**:
   - Verifica aggiornamento dinamico delle informazioni
   - Test transizioni di stato (CONNECTING → CONNECTED → DISCONNECTED)
   - Verifica visualizzazione errori

### 8.8. Prossimi Passi

Con la **Fase 3 completata**, il sistema ora dispone di:
- ✅ Connettività Wi-Fi persistente e configurabile
- ✅ Captive portal user-friendly
- ✅ Display TFT con informazioni in tempo reale
- ✅ Gestione errori robusta
- ✅ Riconnessione automatica

La **Fase 4** (Integrazione IoT) potrà ora concentrarsi su:
- Integrazione con Google Home (FauxmoESP o SinricPro)
- Web dashboard avanzata per controllo remoto
- API REST per integrazione con sistemi domotici
- MQTT broker per Home Assistant

---

## 9. Gestione dell'Alimentazione

Il sistema sarà alimentato da un **powerbank a 5V**.

* **Efficienza:** Il consumo energetico deve essere ottimizzato. Quando la condizione $Stato\_Luce\_Ambiente$ è FALSO (giorno), il sistema può entrare in una modalità di **deep sleep ciclico**, risvegliandosi periodicamente (es. ogni 10 minuti) per controllare la luminosità ambientale, riducendo drasticamente il consumo.
* **Controllo Tensione:** Assicurarsi che il powerbank sia in grado di fornire corrente sufficiente per la striscia LED alla massima luminosità, oltre che per l'ESP32.
* **Monitoraggio:** Utilizzare l'ADC dell'ESP32 (se disponibile e opportunamente configurato con un partitore di tensione) per monitorare lo stato di carica residua del powerbank, visualizzandolo sul display LCD.

---

## 10. Aggiornamento Firmware Remoto (OTA)

Il sistema implementa un modulo dedicato per l'**aggiornamento Over-The-Air (OTA)** del firmware, permettendo di aggiornare il software del dispositivo senza necessità di connessione fisica USB.

### 10.1. Caratteristiche del Sistema OTA

Il sistema OTA offre le seguenti funzionalità:

* **Aggiornamento da URL Remoto**: Possibilità di specificare un URL HTTPS per scaricare il firmware (es. `https://server.example.com/firmware.bin`)
* **Upload Locale**: Caricamento diretto di un file `.bin` tramite interfaccia web
* **Rollback Automatico**: Utilizzo delle partizioni OTA dell'ESP32 per permettere il rollback automatico in caso di firmware corrotto
* **Preservazione Configurazione**: Le credenziali Wi-Fi e tutte le configurazioni salvate vengono preservate durante l'aggiornamento
* **Interfaccia Web Dedicata**: Pagina web responsive con progress bar e log degli errori in tempo reale
* **Nessuna Autenticazione**: Sistema semplificato per uso interno senza protezione password

### 10.2. Architettura delle Partizioni OTA

L'ESP32-S3 utilizza uno schema di partizioni con due slot OTA:

```
┌─────────────────────────┐
│   Bootloader            │  Immutabile
├─────────────────────────┤
│   Partition Table       │  Immutabile
├─────────────────────────┤
│   OTA Slot 0 (app0)    │  ← Firmware Attivo
│   1.4 MB                │
├─────────────────────────┤
│   OTA Slot 1 (app1)    │  ← Firmware Nuovo
│   1.4 MB                │
├─────────────────────────┤
│   NVS (Preferences)     │  ← Credenziali/Config
│   20 KB                 │    (Preservato)
└─────────────────────────┘
```

**Processo di Aggiornamento:**

1. Il firmware nuovo viene scaricato e scritto nella partizione OTA inattiva (es. app1)
2. Se lo scaricamento e la scrittura hanno successo, il bootloader viene configurato per avviare dalla nuova partizione
3. Il dispositivo si riavvia
4. Se il nuovo firmware parte correttamente e viene confermato, l'aggiornamento è completato
5. Se il nuovo firmware non parte o è corrotto, il bootloader riavvia automaticamente dalla partizione precedente (rollback)

### 10.3. Interfaccia Web OTA

L'interfaccia web per l'OTA è accessibile all'indirizzo:

```
http://<device-ip>/ota
o
http://<hostname>.local/ota
```

**Funzionalità dell'Interfaccia:**

* **Pannello Informazioni Correnti**:
  - Versione firmware attuale (se implementata)
  - Partizione attiva (OTA_0 o OTA_1)
  - Spazio disponibile per l'aggiornamento
  - Dimensione massima firmware supportata

* **Metodo 1 - URL Remoto**:
  - Campo di input per inserire URL del firmware (supporta HTTP e HTTPS)
  - Pulsante "Scarica e Aggiorna"
  - Progress bar per il download
  - Validazione URL prima del download

* **Metodo 2 - Upload Locale**:
  - Drag & drop o selezione file `.bin`
  - Verifica dimensione file prima dell'upload
  - Progress bar per l'upload
  - Validazione formato file

* **Log in Tempo Reale**:
  - Stato connessione al server remoto
  - Progress download/upload (byte scaricati/caricati)
  - Stato scrittura flash
  - Messaggi di errore dettagliati
  - Conferma successo o fallimento

### 10.4. Gestione Errori

Il sistema OTA implementa una gestione completa degli errori con logging dettagliato:

**Errori di Connessione:**
- `ERR_CONNECTION_REFUSED`: Server non raggiungibile
- `ERR_TIMEOUT`: Timeout durante il download (>60 secondi)
- `ERR_DNS_FAILED`: Impossibile risolvere l'hostname
- `ERR_SSL_FAILED`: Errore certificato SSL (per HTTPS)

**Errori di Download:**
- `ERR_HTTP_404`: File firmware non trovato sul server
- `ERR_HTTP_500`: Errore server remoto
- `ERR_INVALID_RESPONSE`: Risposta HTTP non valida
- `ERR_FILE_TOO_LARGE`: File supera lo spazio disponibile

**Errori di Scrittura:**
- `ERR_PARTITION_NOT_FOUND`: Partizione OTA non disponibile
- `ERR_FLASH_WRITE_FAILED`: Errore scrittura memoria flash
- `ERR_FLASH_VERIFY_FAILED`: Verifica integrità fallita dopo scrittura
- `ERR_INSUFFICIENT_SPACE`: Spazio insufficiente nella partizione

**Errori di Sistema:**
- `ERR_OTA_BEGIN_FAILED`: Impossibile inizializzare il processo OTA
- `ERR_OTA_END_FAILED`: Impossibile finalizzare l'aggiornamento
- `ERR_ROLLBACK_FAILED`: Rollback automatico fallito
- `ERR_REBOOT_REQUIRED`: Riavvio necessario ma non eseguito

### 10.5. Sicurezza e Best Practices

Nonostante il sistema sia semplificato per uso interno, vengono implementate alcune best practices:

* **Checksum MD5**: Verifica basilare dell'integrità del file scaricato
* **Validazione Dimensione**: Controllo che il firmware non superi lo spazio disponibile prima di iniziare
* **Timeout Configurabili**: Timeout di rete per evitare blocchi indefiniti
* **Logging Completo**: Tracciamento di ogni operazione per debugging
* **Preservazione NVS**: La partizione NVS (credenziali, configurazioni) non viene mai toccata
* **Rollback Automatico**: Protezione contro brick del dispositivo

### 10.6. Modulo Software - OTAManager

Il modulo `OTAManager` (classe C++) incapsula tutta la logica OTA:

**File del Modulo:**
- `OTAManager.h` - Definizione classe e interfaccia pubblica
- `OTAManager.cpp` - Implementazione logica OTA
- `OTAPages.h` - Pagine HTML/CSS/JavaScript per interfaccia web (PROGMEM)

**Metodi Principali:**

```cpp
class OTAManager {
public:
    // Inizializzazione
    bool begin();
    
    // Update loop (gestisce il web server)
    void update();
    
    // Aggiornamento da URL
    bool updateFromURL(const String& url);
    
    // Aggiornamento da upload
    bool updateFromUpload(Stream& stream, size_t size);
    
    // Stato aggiornamento
    enum class OTAState {
        IDLE,           // Pronto per aggiornamento
        DOWNLOADING,    // Download in corso
        UPLOADING,      // Upload in corso
        WRITING,        // Scrittura flash in corso
        VERIFYING,      // Verifica integrità
        SUCCESS,        // Completato con successo
        ERROR           // Errore
    };
    
    OTAState getState() const;
    int getProgress() const; // 0-100%
    String getLastError() const;
    
    // Informazioni partizioni
    String getCurrentPartition() const;
    size_t getFreeSpace() const;
    String getFirmwareVersion() const;
};
```

### 10.7. Integrazione con il Sistema Esistente

Il modulo OTA si integra perfettamente con il sistema esistente:

* **Non Bloccante**: L'aggiornamento avviene in background, il sistema rimane responsive
* **Modalità Offline**: Durante l'aggiornamento, le funzionalità di controllo LED rimangono attive fino al riavvio
* **Display TFT**: Visualizzazione dello stato OTA sul display (opzionale)
* **Log Serial**: Tutti gli eventi OTA vengono loggati per debugging
* **EventLogger Integration**: Gli aggiornamenti OTA vengono registrati nell'EventLogger

### 10.8. Workflow Tipico di Aggiornamento

**Scenario 1 - Aggiornamento da URL Remoto:**

1. L'utente accede a `http://device.local/ota`
2. Inserisce l'URL: `https://myserver.com/firmware/v2.0.bin`
3. Clicca su "Scarica e Aggiorna"
4. Il sistema scarica il file mostrando il progress
5. Una volta scaricato, scrive nella partizione OTA inattiva
6. Completa la scrittura e riavvia automaticamente
7. Il bootloader carica il nuovo firmware
8. Il nuovo firmware viene confermato dopo 30 secondi di uptime stabile

**Scenario 2 - Upload Locale:**

1. L'utente accede a `http://device.local/ota`
2. Trascina il file `firmware.bin` nell'area di upload (oppure clicca e seleziona)
3. Il sistema verifica la dimensione del file
4. Avvia l'upload mostrando il progress
5. Scrive nella partizione OTA inattiva
6. Completa e riavvia automaticamente
7. Rollback automatico se il firmware è corrotto

**Scenario 3 - Errore e Rollback:**

1. Aggiornamento iniziato (da URL o upload)
2. Nuovo firmware scritto correttamente
3. Riavvio eseguito
4. Il nuovo firmware non parte o crasha durante l'inizializzazione
5. Il bootloader ESP32 rileva il problema
6. Rollback automatico alla partizione precedente (funzionante)
7. Il dispositivo riavvia con il firmware originale
8. Log dell'errore disponibile per analisi

### 10.9. Endpoint API REST

Il modulo OTA espone i seguenti endpoint:

```
GET  /ota              → Interfaccia web OTA
GET  /api/ota/info     → Info partizioni e versione corrente (JSON)
POST /api/ota/url      → Avvia aggiornamento da URL
POST /api/ota/upload   → Avvia aggiornamento da file upload
GET  /api/ota/status   → Stato aggiornamento in corso (JSON)
POST /api/ota/rollback → Forza rollback manuale (se disponibile)
```

### 10.10. Librerie Necessarie

Per il modulo OTA sono necessarie le seguenti librerie (tutte built-in ESP32):

| Libreria | Versione | Note |
|----------|----------|------|
| `Update` | Built-in ESP32 | Gestione aggiornamenti OTA |
| `HTTPClient` | Built-in ESP32 | Download firmware da URL |
| `esp_ota_ops` | Built-in ESP32 | API partizioni OTA |
| `esp_partition` | Built-in ESP32 | Informazioni partizioni |
| `WiFiClientSecure` | Built-in ESP32 | HTTPS support (opzionale) |

### 10.11. Note Tecniche

**Dimensione Massima Firmware:**
- Con 4MB di flash, ogni partizione OTA ha circa 1.4-1.8 MB disponibili
- Il firmware deve essere compilato con schema di partizioni appropriato

**Compatibilità HTTPS:**
- Per URL HTTPS è richiesto `WiFiClientSecure`
- I certificati SSL non vengono verificati (mode insecure) per semplicità
- Supporto solo TLS 1.2+

**Performance:**
- Download da URL: ~100-200 KB/s (dipende dalla rete)
- Upload locale: ~300-500 KB/s
- Scrittura flash: ~100 KB/s
- Tempo totale tipico: 30-60 secondi per firmware di 1MB

**Limitazioni:**
- Durante l'aggiornamento il consumo di memoria RAM aumenta (~40KB buffer)
- Connessione Wi-Fi deve rimanere stabile durante tutto il processo
- Non è possibile annullare un aggiornamento una volta iniziata la scrittura flash

---

## 11. Ottimizzazioni e Pulizia del Codice (Novembre 2025)

### 11.1. Riorganizzazione della Struttura del Progetto

Il progetto è stato ottimizzato per migliorare la leggibilità, manutenibilità e separazione delle responsabilità.

#### 11.1.1. Creazione di DebugHelper.h

**Motivazione**: Il file principale `esp-smart-lights.ino` conteneva numerose funzioni di stampa e debug che intasavano il codice principale, rendendo difficile la lettura della logica di business.

**Soluzione**: Creazione di un modulo helper dedicato (`DebugHelper.h`) che incapsula tutte le funzioni di debugging e stampa:

**Funzioni spostate:**
- `printCalibrationValues()` - Stampa valori di calibrazione IMU
- `printStatistics()` - Stampa statistiche di movimento
- `printLightStatus()` - Stampa stato sensore di luce
- `testLED()` - Sequenza di test per LED strip
- `printWiFiStatus()` - Stampa stato dettagliato Wi-Fi
- `printEventLogs()` - Stampa log degli eventi
- `printOTAStatus()` - Stampa stato aggiornamento OTA

**Namespace**: Tutte le funzioni sono racchiuse nel namespace `DebugHelper::` per evitare conflitti di nomi.

#### 11.1.2. Rimozione Comandi Seriali

**Motivazione**: L'interfaccia seriale con comandi interattivi era utile durante lo sviluppo ma non necessaria in produzione. Tutti i parametri sono ora configurabili tramite la dashboard web.

**Rimosso**:
- Funzione `handleSerialCommands()` e tutto il relativo switch-case
- Funzione `printInstructions()` con la lista dei comandi
- Print continui di `printMotionStatus()` (ogni 100ms) che intasavano il Serial Monitor
- Print continui di `printSmartLightStatus()` (ogni 1000ms)

**Mantenuto**:
- Log essenziali durante l'inizializzazione dei componenti
- Log delle azioni dei bottoni fisici
- Log degli eventi critici (connessione Wi-Fi, errori, ecc.)

#### 11.1.3. Miglioramento Gestione Bottoni Fisici

**Problema**: I bottoni eseguivano sempre le loro azioni, anche quando il display era spento, e non c'era feedback visivo chiaro delle azioni.

**Soluzione implementata:**

**1. Wake-on-Press**: 
- Se il display è spento, la pressione di qualsiasi bottone lo risveglia SENZA eseguire l'azione
- L'azione viene eseguita solo se il display è già acceso
- Previene esecuzioni accidentali quando il dispositivo è in standby

**2. Feedback Visivo su Display**:
- Utilizzo del nuovo metodo `displayManager.showMessage()` per mostrare messaggi temporanei
- Ogni azione del bottone mostra un messaggio di conferma sul display
- Esempi:
  * RED BTN: "Calibrating IMU..." → "IMU Calibrated!"
  * BLUE BTN: "Light Bypass: ON" / "Light Bypass: OFF"
  * GREEN BTN: "Testing LED..." → "Test Complete!"

**3. Etichette Permanenti sul Display**:
- Aggiunto metodo `drawButtonLabels()` in DisplayManager
- Le etichette vengono disegnate permanentemente nella riga superiore del display:
  * Sinistra (BTN_L/0) - "Lgt" (Light bypass) - Colore CIANO
  * Centro (BTN_C/47) - "Tst" (Test LED) - Colore VERDE
  * Destra (BTN_R/48) - "Cal" (Calibrate) - Colore ROSSO
- Le etichette vengono ridisegnate automaticamente durante `forceUpdate()`

**Mappatura Bottoni**:
```
      [Cal]      [Tst]      [Lgt]
        RED      GREEN      BLUE
     (BTN_R)    (BTN_C)    (BTN_L)
       Pin48      Pin47      Pin0
```

### 11.2. Nuove Funzionalità Dashboard Web

#### 11.2.1. Controllo Luminosità LED

**Problema**: Non era possibile regolare la luminosità dei LED dalla dashboard web, solo accenderli/spegnerli.

**Soluzione**:

**1. Slider per LED Strip**:
- Range: 0-255 (valore PWM)
- Aggiornamento in tempo reale del valore visualizzato
- Pulsante "Applica Luminosità" per inviare le modifiche

**2. Slider per RGB LED Integrato**:
- Range: 0-255
- Controllo indipendente dalla striscia LED
- Utile per regolare la luminosità del LED di stato

**3. Nuovo Endpoint API**: `/api/brightness`
- Metodo: POST
- Payload JSON: 
  ```json
  {
    "led_brightness": 255,
    "rgb_brightness": 64
  }
  ```
- Risposta:
  ```json
  {
    "success": true,
    "message": "Brightness updated"
  }
  ```

**Implementazione**:
- Aggiunto metodo `handleApiBrightness()` in `WiFiManager.cpp`
- Funzioni JavaScript `updateBrightnessDisplay()` e `saveBrightness()` in `WiFiPages.h`
- Slider HTML5 con input type="range" per un'interfaccia touch-friendly

#### 11.2.2. Interfaccia Dashboard Migliorata

**Layout Sezione LED Control**:
```
┌─────────────────────────────────────┐
│ 🎮 Controllo LED                    │
├─────────────────────────────────────┤
│ Modalità: [AUTO] [ON] [OFF]        │
│                                      │
│ Luminosità LED Strip:                │
│ ━━━━━━━━●━━━━━━━━━━━━━━━ 255       │
│                                      │
│ Luminosità RGB LED:                  │
│ ━━━━●━━━━━━━━━━━━━━━━━━━ 64        │
│                                      │
│ [💡 Applica Luminosità]             │
└─────────────────────────────────────┘
```

### 11.3. DisplayManager - Nuove Funzionalità

#### 11.3.1. Metodo showMessage()

**Firma**: `void showMessage(const String& message, unsigned long durationMs = 2000)`

**Funzionalità**:
- Mostra un messaggio temporaneo al centro del display
- Risveglia automaticamente il display se spento
- Il messaggio rimane visualizzato per il tempo specificato
- Al termine, il display torna alla visualizzazione normale
- Utile per feedback delle azioni utente

**Implementazione**:
- Variabili di stato: `_tempMessage`, `_tempMessageEndTime`, `_showingTempMessage`
- Durante la visualizzazione del messaggio, gli aggiornamenti normali vengono sospesi
- Controllo automatico della scadenza nel metodo `update()`

#### 11.3.2. Metodo isDisplayOn()

**Firma**: `bool isDisplayOn() const`

**Funzionalità**:
- Ritorna lo stato corrente del backlight del display
- Utilizzato per implementare il wake-on-press dei bottoni
- Permette di verificare se il display è in modalità power saving

### 11.4. Struttura File Aggiornata

**File Aggiunti**:
- `DebugHelper.h` - Funzioni helper per debugging (namespace DebugHelper)

**File Modificati**:
- `esp-smart-lights.ino` - Pulizia codice, rimozione comandi seriali, miglioramento gestione bottoni
- `DisplayManager.h/.cpp` - Aggiunta `showMessage()`, `drawButtonLabels()`, `isDisplayOn()`
- `WiFiManager.h/.cpp` - Aggiunta `handleApiBrightness()` per controllo luminosità
- `WiFiPages.h` - Aggiunta sezione brightness control con slider
- `project.md` - Aggiornamento documentazione

### 11.5. Vantaggi delle Modifiche

**Manutenibilità**:
- Codice più pulito e organizzato
- Funzioni di debug separate dalla logica di business
- Più facile navigare e comprendere il codice

**User Experience**:
- Feedback visivo chiaro per tutte le azioni
- Etichette permanenti rendono i bottoni auto-esplicativi
- Wake-on-press previene azioni accidentali
- Controllo completo dalla dashboard web

**Produzione**:
- Nessuna dipendenza da interfaccia seriale
- Tutte le configurazioni accessibili via web
- Sistema completamente standalone

**Performance**:
- Ridotto traffico sulla seriale (meno print continui)
- Display aggiornato solo quando necessario
- Gestione intelligente dei messaggi temporanei

### 11.6. Comandi Seriali Rimossi

I seguenti comandi sono stati rimossi (funzionalità ora disponibili solo via web):

| Comando | Funzione | Alternativa Web |
|---------|----------|-----------------|
| `a<val>` | Imposta soglia accelerometro | Dashboard → Configurazione |
| `g<val>` | Imposta soglia giroscopio | Dashboard → Configurazione |
| `w<val>` | Imposta finestra movimento | *Non più necessario* |
| `p<val>` | Imposta conteggio impulsi | *Non più necessario* |
| `l<val>` | Imposta soglia lux | Dashboard → Configurazione |
| `L<val>` | Imposta luminosità LED | Dashboard → Controllo LED → Slider |
| `d<val>` | Imposta ritardo spegnimento | Dashboard → Configurazione |
| `A` | Toggle auto mode | Dashboard → Controllo LED → Mode |
| `B` | Toggle light sensor bypass | Bottone fisico BLUE |
| `O` | Forza LED ON | Dashboard → Controllo LED → ON |
| `F` | Forza LED OFF | Dashboard → Controllo LED → OFF |
| `R` | Ritorna a modo auto | Dashboard → Controllo LED → AUTO |
| `s` | Mostra statistiche | *Rimosso* |
| `c` | Ricalibra IMU | Bottone fisico RED |
| `t<val>` | Imposta timeout LCD | *Non più necessario* |
| `W` | Mostra stato WiFi | Dashboard → Info |
| `X` | Reset credenziali WiFi | Bottone fisico CENTER (hold 5s) |
| `Z` | Forza riconnessione WiFi | *Automatico* |
| `E` | Mostra event logs | Dashboard → Logs |
| `C` | Cancella event logs | Dashboard → Logs |
| `U` | Mostra stato OTA | Dashboard → OTA |
| `T` | Trigger OTA rollback | Dashboard → OTA |
| `h` | Mostra help | *Rimosso* |

---