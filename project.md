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