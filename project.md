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

## 7. Gestione dell'Alimentazione

Il sistema sarà alimentato da un **powerbank a 5V**.

* **Efficienza:** Il consumo energetico deve essere ottimizzato. Quando la condizione $Stato\_Luce\_Ambiente$ è FALSO (giorno), il sistema può entrare in una modalità di **deep sleep ciclico**, risvegliandosi periodicamente (es. ogni 10 minuti) per controllare la luminosità ambientale, riducendo drasticamente il consumo.
* **Controllo Tensione:** Assicurarsi che il powerbank sia in grado di fornire corrente sufficiente per la striscia LED alla massima luminosità, oltre che per l'ESP32.
* **Monitoraggio:** Utilizzare l'ADC dell'ESP32 (se disponibile e opportunamente configurato con un partitore di tensione) per monitorare lo stato di carica residua del powerbank, visualizzandolo sul display LCD.