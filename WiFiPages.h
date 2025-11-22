#ifndef WIFI_PAGES_H
#define WIFI_PAGES_H

#include <Arduino.h>

// HTML Configuration Page for AP Mode (stored in PROGMEM)
const char WIFI_CONFIG_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Configurazione Wi-Fi - Centralina Luci</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 12px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            max-width: 500px;
            width: 100%;
            padding: 30px;
        }
        h1 {
            color: #333;
            font-size: 24px;
            margin-bottom: 10px;
            text-align: center;
        }
        .subtitle {
            color: #666;
            text-align: center;
            margin-bottom: 30px;
            font-size: 14px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: 500;
            font-size: 14px;
        }
        input, select {
            width: 100%;
            padding: 12px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            font-size: 16px;
            transition: border-color 0.3s;
        }
        input:focus, select:focus {
            outline: none;
            border-color: #667eea;
        }
        .btn {
            width: 100%;
            padding: 14px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s, box-shadow 0.2s;
            margin-top: 10px;
        }
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(102, 126, 234, 0.4);
        }
        .btn:active {
            transform: translateY(0);
        }
        .btn-secondary {
            background: linear-gradient(135deg, #868f96 0%, #596164 100%);
        }
        .info-box {
            background: #f0f4ff;
            border-left: 4px solid #667eea;
            padding: 15px;
            border-radius: 6px;
            margin-bottom: 20px;
            font-size: 14px;
            color: #555;
        }
        .error-box {
            background: #fff0f0;
            border-left: 4px solid #e74c3c;
            padding: 15px;
            border-radius: 6px;
            margin-bottom: 20px;
            font-size: 14px;
            color: #c0392b;
            display: none;
        }
        .loading {
            display: none;
            text-align: center;
            margin: 20px 0;
        }
        .spinner {
            border: 3px solid #f3f3f3;
            border-top: 3px solid #667eea;
            border-radius: 50%;
            width: 40px;
            height: 40px;
            animation: spin 1s linear infinite;
            margin: 0 auto;
        }
        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
        .advanced {
            margin-top: 15px;
            padding-top: 15px;
            border-top: 1px solid #e0e0e0;
        }
        .small-text {
            font-size: 12px;
            color: #888;
            margin-top: 5px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔧 Configurazione Wi-Fi</h1>
        <p class="subtitle">Centralina Luci - Robot Tosaerba</p>
        
        <div class="info-box">
            <strong>📶 Modalità Access Point attiva</strong><br>
            Connesso a: <strong>%DEVICE_AP_SSID%</strong><br>
            Inserisci le credenziali della tua rete Wi-Fi per connettere il dispositivo.
        </div>
        
        <div id="errorBox" class="error-box"></div>
        
        <form id="wifiForm" onsubmit="return submitForm(event)">
            <div class="form-group">
                <label for="ssid">Nome Rete (SSID) *</label>
                <select id="ssid" name="ssid" onchange="updateSSIDField()">
                    <option value="">-- Scansiona reti disponibili --</option>
                </select>
                <input type="text" id="ssid_custom" name="ssid_custom" placeholder="Oppure inserisci SSID manualmente" style="margin-top: 10px;">
                <div class="small-text">Usa il menu a tendina o inserisci l'SSID manualmente</div>
            </div>
            
            <div class="form-group">
                <label for="password">Password *</label>
                <input type="password" id="password" name="password" required placeholder="Inserisci la password Wi-Fi">
                <div class="small-text">La password verrà salvata in modo sicuro nella memoria del dispositivo</div>
            </div>
            
            <div class="advanced">
                <div class="form-group">
                    <label for="retry">Intervallo Riconnessione (secondi)</label>
                    <input type="number" id="retry" name="retry" value="300" min="30" max="3600">
                    <div class="small-text">Tempo di attesa tra i tentativi di riconnessione (default: 300s = 5 min)</div>
                </div>
            </div>
            
            <button type="button" class="btn btn-secondary" onclick="scanNetworks()">🔍 Scansiona Reti</button>
            <button type="submit" class="btn">💾 Salva e Connetti</button>
        </form>
        
        <div id="loading" class="loading">
            <div class="spinner"></div>
            <p style="margin-top: 10px; color: #666;">Connessione in corso...</p>
        </div>
    </div>
    
    <script>
        function showError(message) {
            const errorBox = document.getElementById('errorBox');
            errorBox.textContent = '⚠️ ' + message;
            errorBox.style.display = 'block';
        }
        
        function hideError() {
            document.getElementById('errorBox').style.display = 'none';
        }
        
        function showLoading(show) {
            document.getElementById('wifiForm').style.display = show ? 'none' : 'block';
            document.getElementById('loading').style.display = show ? 'block' : 'none';
        }
        
        function updateSSIDField() {
            const select = document.getElementById('ssid');
            const custom = document.getElementById('ssid_custom');
            if (select.value) {
                custom.value = '';
                custom.required = false;
            } else {
                custom.required = true;
            }
        }
        
        async function scanNetworks() {
            hideError();
            const select = document.getElementById('ssid');
            const btn = event.target;
            btn.disabled = true;
            btn.textContent = '🔄 Scansione in corso...';
            
            try {
                const response = await fetch('/scan');
                const data = await response.json();
                
                select.innerHTML = '<option value="">-- Seleziona una rete --</option>';
                
                if (data.networks && data.networks.length > 0) {
                    data.networks.forEach(network => {
                        const option = document.createElement('option');
                        option.value = network.ssid;
                        option.textContent = `${network.ssid} (${network.rssi} dBm) ${network.encrypted ? '🔒' : '🔓'}`;
                        select.appendChild(option);
                    });
                } else {
                    showError('Nessuna rete Wi-Fi trovata. Verifica che il router sia acceso.');
                }
            } catch (error) {
                showError('Errore durante la scansione: ' + error.message);
            } finally {
                btn.disabled = false;
                btn.textContent = '🔍 Scansiona Reti';
            }
        }
        
        async function submitForm(event) {
            event.preventDefault();
            hideError();
            
            const ssid = document.getElementById('ssid').value || document.getElementById('ssid_custom').value;
            const password = document.getElementById('password').value;
            const retry = document.getElementById('retry').value;
            
            if (!ssid) {
                showError('Inserisci un SSID valido');
                return false;
            }
            
            if (!password) {
                showError('Inserisci la password Wi-Fi');
                return false;
            }
            
            showLoading(true);
            
            try {
                const response = await fetch('/save', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}&retry=${retry}`
                });
                
                const data = await response.json();
                
                if (data.success) {
                    document.querySelector('.container').innerHTML = `
                        <div style="text-align: center; padding: 20px;">
                            <div style="font-size: 60px; margin-bottom: 20px;">✅</div>
                            <h1>Configurazione Salvata!</h1>
                            <p style="margin: 20px 0; color: #666;">
                                Il dispositivo si sta connettendo alla rete <strong>${ssid}</strong>.<br>
                                Questa pagina si chiuderà automaticamente tra qualche secondo.
                            </p>
                            <p style="font-size: 14px; color: #888;">
                                Se la connessione ha successo, potrai accedere al dispositivo<br>
                                tramite l'indirizzo: <strong>http://${data.hostname}.local</strong>
                            </p>
                        </div>
                    `;
                    setTimeout(() => window.close(), 5000);
                } else {
                    showError(data.message || 'Errore durante il salvataggio');
                    showLoading(false);
                }
            } catch (error) {
                showError('Errore di comunicazione: ' + error.message);
                showLoading(false);
            }
            
            return false;
        }
        
        // Auto-scan on page load
        window.addEventListener('load', () => {
            setTimeout(scanNetworks, 1000);
        });
    </script>
</body>
</html>
)rawliteral";

// HTML Status Page for Station Mode (stored in PROGMEM)
const char WIFI_STATUS_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Centralina Luci - Status</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 12px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            max-width: 600px;
            width: 100%;
            padding: 30px;
        }
        h1 {
            color: #333;
            font-size: 28px;
            margin-bottom: 10px;
            text-align: center;
        }
        .subtitle {
            text-align: center;
            color: #666;
            margin-bottom: 30px;
            font-size: 14px;
        }
        .status-box {
            background: #f0f4ff;
            border-left: 4px solid #667eea;
            padding: 20px;
            border-radius: 8px;
            margin: 20px 0;
        }
        .status-item {
            display: flex;
            justify-content: space-between;
            padding: 12px 0;
            border-bottom: 1px solid #e0e0e0;
        }
        .status-item:last-child {
            border-bottom: none;
        }
        .label {
            font-weight: 600;
            color: #555;
            font-size: 14px;
        }
        .value {
            color: #333;
            font-size: 14px;
            text-align: right;
        }
        .success {
            color: #27ae60;
            font-weight: 600;
        }
        .badge {
            display: inline-block;
            padding: 4px 12px;
            border-radius: 12px;
            font-size: 12px;
            font-weight: 600;
            background: #27ae60;
            color: white;
        }
        .info-banner {
            margin-top: 20px;
            padding: 20px;
            background: linear-gradient(135deg, #fff9e6 0%, #ffe9b8 100%);
            border-radius: 8px;
            text-align: center;
            border: 1px solid #f5d888;
        }
        .info-banner p {
            color: #856404;
            font-size: 14px;
            margin: 5px 0;
        }
        .info-banner strong {
            color: #664d03;
        }
        .nav-footer {
            position: fixed;
            bottom: 0;
            left: 0;
            right: 0;
            background: white;
            box-shadow: 0 -2px 10px rgba(0,0,0,0.1);
            display: flex;
            justify-content: space-around;
            padding: 12px 0;
        }
        .nav-item {
            flex: 1;
            text-align: center;
            padding: 8px;
            color: #666;
            text-decoration: none;
            font-size: 12px;
            font-weight: 600;
            transition: color 0.2s;
        }
        .nav-item.active {
            color: #667eea;
        }
        .nav-icon {
            font-size: 20px;
            display: block;
            margin-bottom: 4px;
        }
        @media (min-width: 768px) {
            .nav-footer {
                position: relative;
                max-width: 600px;
                margin: 20px auto 0;
                border-radius: 12px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🤖 Centralina Luci</h1>
        <p class="subtitle">Robot Tosaerba - Status Dashboard</p>
        
        <div class="status-box">
            <div class="status-item">
                <span class="label">Stato Connessione:</span>
                <span class="value"><span class="badge">✓ CONNESSO</span></span>
            </div>
            <div class="status-item">
                <span class="label">Rete Wi-Fi:</span>
                <span class="value">%WIFI_SSID%</span>
            </div>
            <div class="status-item">
                <span class="label">Indirizzo IP:</span>
                <span class="value">%WIFI_IP%</span>
            </div>
            <div class="status-item">
                <span class="label">Hostname:</span>
                <span class="value">%WIFI_HOSTNAME%.local</span>
            </div>
            <div class="status-item">
                <span class="label">Potenza Segnale:</span>
                <span class="value">%WIFI_RSSI% dBm</span>
            </div>
            <div class="status-item">
                <span class="label">MAC Address:</span>
                <span class="value">%WIFI_MAC%</span>
            </div>
        </div>
        <div class="info-banner">
            <p>✨ <strong>Sistema Operativo</strong></p>
            <p>Il dispositivo è connesso alla rete e pronto per l'uso</p>
            <p style="margin-top: 15px;"><a href="/dashboard" style="color: #664d03; font-weight: 600; text-decoration: none;">📊 Apri Dashboard →</a></p>
        </div>
    </div>
    
    <nav class="nav-footer">
        <a href="/dashboard" class="nav-item">
            <span class="nav-icon">📊</span>
            Dashboard
        </a>
        <a href="/logs" class="nav-item">
            <span class="nav-icon">📋</span>
            Logs
        </a>
        <a href="/ota" class="nav-item">
            <span class="nav-icon">🔄</span>
            OTA
        </a>
        <a href="/" class="nav-item active">
            <span class="nav-icon">ℹ️</span>
            Info
        </a>
    </nav>
</body>
</html>
)rawliteral";

// HTML Dashboard Page for Station Mode (stored in PROGMEM)
const char WIFI_DASHBOARD_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Dashboard - Centralina Luci</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 15px;
            padding-bottom: 80px;
        }
        .header {
            text-align: center;
            color: white;
            margin-bottom: 20px;
            padding: 15px;
        }
        .header h1 {
            font-size: 24px;
            margin-bottom: 5px;
        }
        .header .subtitle {
            font-size: 14px;
            opacity: 0.9;
        }
        .card {
            background: white;
            border-radius: 12px;
            box-shadow: 0 4px 20px rgba(0,0,0,0.15);
            margin-bottom: 15px;
            padding: 20px;
        }
        .card-header {
            font-size: 16px;
            font-weight: 600;
            color: #333;
            margin-bottom: 15px;
            display: flex;
            align-items: center;
            gap: 8px;
        }
        .status-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
        }
        .status-item {
            text-align: center;
        }
        .status-value {
            font-size: 28px;
            font-weight: 700;
            color: #667eea;
            margin-bottom: 5px;
        }
        .status-label {
            font-size: 12px;
            color: #666;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        .badge {
            display: inline-block;
            padding: 6px 12px;
            border-radius: 20px;
            font-size: 13px;
            font-weight: 600;
        }
        .badge-success { background: #27ae60; color: white; }
        .badge-warning { background: #f39c12; color: white; }
        .badge-danger { background: #e74c3c; color: white; }
        .badge-info { background: #3498db; color: white; }
        .control-group {
            margin: 15px 0;
        }
        .control-label {
            font-size: 14px;
            font-weight: 600;
            color: #333;
            margin-bottom: 8px;
            display: block;
        }
        .btn-group {
            display: flex;
            gap: 8px;
            margin-bottom: 10px;
        }
        .btn {
            flex: 1;
            padding: 12px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            background: white;
            cursor: pointer;
            font-size: 14px;
            font-weight: 600;
            transition: all 0.2s;
        }
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(0,0,0,0.1);
        }
        .btn.active {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border-color: #667eea;
        }
        .btn-primary {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            width: 100%;
            margin-top: 10px;
        }
        .input-group {
            display: flex;
            align-items: center;
            gap: 10px;
            margin-bottom: 10px;
        }
        .input-group input {
            flex: 1;
            padding: 10px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            font-size: 14px;
        }
        .input-group .unit {
            font-size: 13px;
            color: #666;
            min-width: 40px;
        }
        .info-text {
            font-size: 12px;
            color: #666;
            margin-top: 5px;
        }
        .nav-footer {
            position: fixed;
            bottom: 0;
            left: 0;
            right: 0;
            background: white;
            box-shadow: 0 -2px 10px rgba(0,0,0,0.1);
            display: flex;
            justify-content: space-around;
            padding: 12px 0;
        }
        .nav-item {
            flex: 1;
            text-align: center;
            padding: 8px;
            color: #666;
            text-decoration: none;
            font-size: 12px;
            font-weight: 600;
            transition: color 0.2s;
        }
        .nav-item.active {
            color: #667eea;
        }
        .nav-icon {
            font-size: 20px;
            display: block;
            margin-bottom: 4px;
        }
        .section {
            margin-bottom: 20px;
            padding-bottom: 20px;
            border-bottom: 1px solid #e0e0e0;
        }
        .section:last-child {
            border-bottom: none;
            margin-bottom: 0;
            padding-bottom: 0;
        }
        @media (min-width: 768px) {
            body {
                padding: 30px;
                display: flex;
                flex-direction: column;
                align-items: center;
            }
            .header, .card {
                max-width: 600px;
                width: 100%;
            }
            .nav-footer {
                position: relative;
                max-width: 600px;
                margin: 20px auto 0;
                border-radius: 12px;
            }
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>🤖 Dashboard Centralina</h1>
        <p class="subtitle">Robot Tosaerba - Controllo Luci</p>
    </div>
    
    <div class="card">
        <div class="card-header">📊 Stato Sistema</div>
        <div class="status-grid">
            <div class="status-item">
                <div class="status-value" id="ledStatus">--</div>
                <div class="status-label">LED</div>
            </div>
            <div class="status-item">
                <div class="status-value" id="luxValue">--</div>
                <div class="status-label">Luminosità (lux)</div>
            </div>
            <div class="status-item">
                <div class="status-value" id="motionStatus">--</div>
                <div class="status-label">Movimento</div>
            </div>
            <div class="status-item">
                <div class="status-value" id="wifiRSSI">--</div>
                <div class="status-label">Wi-Fi (dBm)</div>
            </div>
        </div>
    </div>
    
    <div class="card">
        <div class="card-header">🎮 Controllo LED</div>
        <div class="control-group">
            <label class="control-label">Modalità</label>
            <div class="btn-group">
                <button class="btn" id="btnAuto" onclick="setLEDMode('auto')">AUTO</button>
                <button class="btn" id="btnOn" onclick="setLEDMode('on')">ON</button>
                <button class="btn" id="btnOff" onclick="setLEDMode('off')">OFF</button>
            </div>
            <div class="info-text" id="modeInfo">Modalità automatica: LED si accende quando è notte e c'è movimento</div>
        </div>
        
        <div class="section" style="margin-top: 20px;">
            <div class="control-group">
                <label class="control-label">Luminosità LED Strip</label>
                <div class="input-group">
                    <input type="range" id="ledBrightness" min="0" max="255" value="255" 
                           style="flex: 1;" oninput="updateBrightnessDisplay('led', this.value)">
                    <span class="unit" id="ledBrightnessValue">255</span>
                </div>
                <div class="info-text">Regola la luminosità della striscia LED (0-255)</div>
            </div>
            
            <div class="control-group">
                <label class="control-label">Luminosità RGB LED</label>
                <div class="input-group">
                    <input type="range" id="rgbBrightness" min="0" max="255" value="64" 
                           style="flex: 1;" oninput="updateBrightnessDisplay('rgb', this.value)">
                    <span class="unit" id="rgbBrightnessValue">64</span>
                </div>
                <div class="info-text">Regola la luminosità del LED RGB integrato (0-255)</div>
            </div>
            
            <button class="btn btn-primary" onclick="saveBrightness()">💡 Applica Luminosità</button>
        </div>
    </div>
    
    <div class="card">
        <div class="card-header">⚙️ Configurazione</div>
        
        <div class="section">
            <div class="control-group">
                <label class="control-label">Soglia Notte</label>
                <div class="input-group">
                    <input type="number" id="luxThreshold" step="0.1" min="0" max="100">
                    <span class="unit">lux</span>
                </div>
                <div class="info-text">LED si accende quando la luminosità scende sotto questa soglia</div>
            </div>
        </div>
        
        <div class="section">
            <div class="control-group">
                <label class="control-label">Soglia Accelerazione</label>
                <div class="input-group">
                    <input type="number" id="accelThreshold" step="0.01" min="0.01" max="2">
                    <span class="unit">g</span>
                </div>
                <div class="info-text">Sensibilità rilevamento movimento (accelerazione)</div>
            </div>
            
            <div class="control-group">
                <label class="control-label">Soglia Giroscopio</label>
                <div class="input-group">
                    <input type="number" id="gyroThreshold" step="1" min="1" max="100">
                    <span class="unit">°/s</span>
                </div>
                <div class="info-text">Sensibilità rilevamento rotazione</div>
            </div>
        </div>
        
        <div class="section">
            <div class="control-group">
                <label class="control-label">Ritardo Spegnimento</label>
                <div class="input-group">
                    <input type="number" id="shutoffDelay" step="1" min="1" max="300">
                    <span class="unit">sec</span>
                </div>
                <div class="info-text">Tempo prima di spegnere il LED dopo che si ferma</div>
            </div>
        </div>
        
        <button class="btn btn-primary" onclick="saveConfig()">💾 Salva Configurazione</button>
    </div>
    
    <nav class="nav-footer">
        <a href="/dashboard" class="nav-item active">
            <span class="nav-icon">📊</span>
            Dashboard
        </a>
        <a href="/logs" class="nav-item">
            <span class="nav-icon">📋</span>
            Logs
        </a>
        <a href="/ota" class="nav-item">
            <span class="nav-icon">🔄</span>
            OTA
        </a>
        <a href="/" class="nav-item">
            <span class="nav-icon">ℹ️</span>
            Info
        </a>
    </nav>
    
    <script>
        let currentMode = 'auto';
        let pollInterval = null;
        let userInteracting = false;
        let interactionTimeout = null;
        
        // Poll status from server
        async function pollStatus() {
            try {
                const response = await fetch('/api/status');
                if (response.ok) {
                    const data = await response.json();
                    updateStatus(data);
                }
            } catch (error) {
                console.error('Error polling status:', error);
                // Continue polling even on error
            }
        }
        
        // Start polling every 2 seconds
        function startPolling() {
            // Initial poll
            pollStatus();
            
            // Poll every 2 seconds
            if (pollInterval) {
                clearInterval(pollInterval);
            }
            pollInterval = setInterval(pollStatus, 2000);
        }
        
        // Stop polling
        function stopPolling() {
            if (pollInterval) {
                clearInterval(pollInterval);
                pollInterval = null;
            }
        }
        
        // Update dashboard with new data
        function updateStatus(data) {
            // LED status
            const ledEl = document.getElementById('ledStatus');
            if (data.led_on) {
                ledEl.innerHTML = '<span class="badge badge-success">ACCESO</span>';
            } else {
                ledEl.innerHTML = '<span class="badge badge-danger">SPENTO</span>';
            }
            
            // Lux value
            document.getElementById('luxValue').textContent = data.lux.toFixed(1);
            
            // Motion status
            const motionEl = document.getElementById('motionStatus');
            if (data.motion) {
                motionEl.innerHTML = '<span class="badge badge-warning">ATTIVO</span>';
            } else {
                motionEl.innerHTML = '<span class="badge badge-info">FERMO</span>';
            }
            
            // Wi-Fi RSSI
            document.getElementById('wifiRSSI').textContent = data.rssi;
            
            // Update mode buttons
            updateModeButtons(data.led_mode);
        }
        
        // Update mode button states
        function updateModeButtons(mode) {
            // Don't update if user is currently interacting
            if (userInteracting) {
                return;
            }
            
            currentMode = mode;
            document.getElementById('btnAuto').classList.toggle('active', mode === 'auto');
            document.getElementById('btnOn').classList.toggle('active', mode === 'on');
            document.getElementById('btnOff').classList.toggle('active', mode === 'off');
            
            const infoTexts = {
                auto: 'Modalità automatica: LED si accende quando è notte e c\'è movimento',
                on: 'LED forzato ACCESO (modalità manuale)',
                off: 'LED forzato SPENTO (modalità manuale)'
            };
            document.getElementById('modeInfo').textContent = infoTexts[mode] || '';
        }
        
        // Set LED mode
        async function setLEDMode(mode) {
            // Mark as user interaction
            userInteracting = true;
            
            // Clear any existing timeout
            if (interactionTimeout) {
                clearTimeout(interactionTimeout);
            }
            
            // Update UI immediately
            currentMode = mode;
            document.getElementById('btnAuto').classList.toggle('active', mode === 'auto');
            document.getElementById('btnOn').classList.toggle('active', mode === 'on');
            document.getElementById('btnOff').classList.toggle('active', mode === 'off');
            
            const infoTexts = {
                auto: 'Modalità automatica: LED si accende quando è notte e c\'è movimento',
                on: 'LED forzato ACCESO (modalità manuale)',
                off: 'LED forzato SPENTO (modalità manuale)'
            };
            document.getElementById('modeInfo').textContent = infoTexts[mode] || '';
            
            try {
                const response = await fetch('/api/led/override', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ mode: mode })
                });
                
                const data = await response.json();
                if (!data.success) {
                    alert('Errore: ' + data.message);
                }
            } catch (error) {
                alert('Errore di comunicazione: ' + error.message);
            } finally {
                // Allow updates again after 3 seconds
                interactionTimeout = setTimeout(() => {
                    userInteracting = false;
                }, 3000);
            }
        }
        
        // Update brightness display value
        function updateBrightnessDisplay(type, value) {
            if (type === 'led') {
                document.getElementById('ledBrightnessValue').textContent = value;
            } else if (type === 'rgb') {
                document.getElementById('rgbBrightnessValue').textContent = value;
            }
        }
        
        // Save brightness settings
        async function saveBrightness() {
            const brightness = {
                led_brightness: parseInt(document.getElementById('ledBrightness').value),
                rgb_brightness: parseInt(document.getElementById('rgbBrightness').value)
            };
            
            try {
                const response = await fetch('/api/brightness', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(brightness)
                });
                
                const data = await response.json();
                if (data.success) {
                    alert('✓ Luminosità aggiornata con successo!');
                } else {
                    alert('Errore: ' + data.message);
                }
            } catch (error) {
                alert('Errore di comunicazione: ' + error.message);
            }
        }
        
        // Load configuration
        async function loadConfig() {
            try {
                const response = await fetch('/api/config');
                const data = await response.json();
                
                document.getElementById('luxThreshold').value = data.lux_threshold;
                document.getElementById('accelThreshold').value = data.accel_threshold;
                document.getElementById('gyroThreshold').value = data.gyro_threshold;
                document.getElementById('shutoffDelay').value = data.shutoff_delay / 1000; // Convert to seconds
            } catch (error) {
                console.error('Error loading config:', error);
            }
        }
        
        // Save configuration
        async function saveConfig() {
            const config = {
                lux_threshold: parseFloat(document.getElementById('luxThreshold').value),
                accel_threshold: parseFloat(document.getElementById('accelThreshold').value),
                gyro_threshold: parseFloat(document.getElementById('gyroThreshold').value),
                shutoff_delay: parseInt(document.getElementById('shutoffDelay').value) * 1000 // Convert to ms
            };
            
            try {
                const response = await fetch('/api/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(config)
                });
                
                const data = await response.json();
                if (data.success) {
                    alert('✓ Configurazione salvata con successo!');
                } else {
                    alert('Errore: ' + data.message);
                }
            } catch (error) {
                alert('Errore di comunicazione: ' + error.message);
            }
        }
        
        // Initialize
        window.addEventListener('load', () => {
            startPolling();
            loadConfig();
        });
        
        // Cleanup on page unload
        window.addEventListener('beforeunload', () => {
            stopPolling();
        });
        
        // Stop polling when page is hidden, resume when visible
        document.addEventListener('visibilitychange', () => {
            if (document.hidden) {
                stopPolling();
            } else {
                startPolling();
            }
        });
    </script>
</body>
</html>
)rawliteral";

// HTML Logs Page (stored in PROGMEM)
const char WIFI_LOGS_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Logs - Centralina Luci</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 15px;
            padding-bottom: 80px;
        }
        .header {
            text-align: center;
            color: white;
            margin-bottom: 20px;
            padding: 15px;
        }
        .header h1 {
            font-size: 24px;
            margin-bottom: 5px;
        }
        .header .subtitle {
            font-size: 14px;
            opacity: 0.9;
        }
        .card {
            background: white;
            border-radius: 12px;
            box-shadow: 0 4px 20px rgba(0,0,0,0.15);
            margin-bottom: 15px;
            padding: 20px;
        }
        .card-header {
            font-size: 16px;
            font-weight: 600;
            color: #333;
            margin-bottom: 15px;
            display: flex;
            align-items: center;
            justify-content: space-between;
        }
        .filter-group {
            display: flex;
            gap: 8px;
            margin-bottom: 15px;
        }
        .filter-btn {
            flex: 1;
            padding: 8px 12px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            background: white;
            cursor: pointer;
            font-size: 12px;
            font-weight: 600;
            transition: all 0.2s;
        }
        .filter-btn.active {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border-color: #667eea;
        }
        .log-list {
            max-height: 60vh;
            overflow-y: auto;
        }
        .log-entry {
            padding: 12px;
            border-left: 4px solid #ccc;
            margin-bottom: 10px;
            border-radius: 4px;
            background: #f8f9fa;
            transition: all 0.2s;
        }
        .log-entry:hover {
            background: #e9ecef;
            transform: translateX(4px);
        }
        .log-entry.on {
            border-left-color: #27ae60;
            background: #d4edda;
        }
        .log-entry.off {
            border-left-color: #e74c3c;
            background: #f8d7da;
        }
        .log-time {
            font-size: 11px;
            color: #666;
            margin-bottom: 4px;
        }
        .log-event {
            font-size: 14px;
            font-weight: 600;
            color: #333;
        }
        .log-details {
            font-size: 12px;
            color: #666;
            margin-top: 4px;
        }
        .empty-state {
            text-align: center;
            padding: 40px 20px;
            color: #999;
        }
        .empty-state-icon {
            font-size: 48px;
            margin-bottom: 16px;
        }
        .stats {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin-bottom: 20px;
        }
        .stat-box {
            text-align: center;
            padding: 15px;
            background: #f0f4ff;
            border-radius: 8px;
        }
        .stat-value {
            font-size: 24px;
            font-weight: 700;
            color: #667eea;
        }
        .stat-label {
            font-size: 12px;
            color: #666;
            margin-top: 4px;
        }
        .nav-footer {
            position: fixed;
            bottom: 0;
            left: 0;
            right: 0;
            background: white;
            box-shadow: 0 -2px 10px rgba(0,0,0,0.1);
            display: flex;
            justify-content: space-around;
            padding: 12px 0;
        }
        .nav-item {
            flex: 1;
            text-align: center;
            padding: 8px;
            color: #666;
            text-decoration: none;
            font-size: 12px;
            font-weight: 600;
            transition: color 0.2s;
        }
        .nav-item.active {
            color: #667eea;
        }
        .nav-icon {
            font-size: 20px;
            display: block;
            margin-bottom: 4px;
        }
        .btn-clear {
            padding: 8px 16px;
            background: #e74c3c;
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 12px;
            font-weight: 600;
            cursor: pointer;
        }
        @media (min-width: 768px) {
            body {
                padding: 30px;
                display: flex;
                flex-direction: column;
                align-items: center;
            }
            .header, .card {
                max-width: 600px;
                width: 100%;
            }
            .nav-footer {
                position: relative;
                max-width: 600px;
                margin: 20px auto 0;
                border-radius: 12px;
            }
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>📋 Log Eventi</h1>
        <p class="subtitle">Storico Accensioni LED - Ultimi 7 giorni</p>
    </div>
    
    <div class="card">
        <div class="stats">
            <div class="stat-box">
                <div class="stat-value" id="totalEvents">0</div>
                <div class="stat-label">EVENTI TOTALI</div>
            </div>
            <div class="stat-box">
                <div class="stat-value" id="todayEvents">0</div>
                <div class="stat-label">OGGI</div>
            </div>
        </div>
    </div>
    
    <div class="card">
        <div class="card-header">
            <span>Filtro Eventi</span>
            <button class="btn-clear" onclick="clearLogs()">🗑️ Cancella</button>
        </div>
        <div class="filter-group">
            <button class="filter-btn active" onclick="filterLogs('all')">TUTTI</button>
            <button class="filter-btn" onclick="filterLogs('on')">ACCENSIONI</button>
            <button class="filter-btn" onclick="filterLogs('off')">SPEGNIMENTI</button>
        </div>
        
        <div id="logList" class="log-list">
            <div class="empty-state">
                <div class="empty-state-icon">📭</div>
                <p>Caricamento eventi...</p>
            </div>
        </div>
    </div>
    
    <nav class="nav-footer">
        <a href="/dashboard" class="nav-item">
            <span class="nav-icon">📊</span>
            Dashboard
        </a>
        <a href="/logs" class="nav-item active">
            <span class="nav-icon">📋</span>
            Logs
        </a>
        <a href="/ota" class="nav-item">
            <span class="nav-icon">🔄</span>
            OTA
        </a>
        <a href="/" class="nav-item">
            <span class="nav-icon">ℹ️</span>
            Info
        </a>
    </nav>
    
    <script>
        let allLogs = [];
        let currentFilter = 'all';
        
        // Load logs from server
        async function loadLogs() {
            try {
                const response = await fetch('/api/logs');
                const data = await response.json();
                allLogs = data.logs || [];
                updateStats();
                displayLogs();
            } catch (error) {
                console.error('Error loading logs:', error);
                document.getElementById('logList').innerHTML = `
                    <div class="empty-state">
                        <div class="empty-state-icon">⚠️</div>
                        <p>Errore nel caricamento dei log</p>
                    </div>
                `;
            }
        }
        
        // Update statistics
        function updateStats() {
            document.getElementById('totalEvents').textContent = allLogs.length;
            
            const today = new Date().toDateString();
            const todayLogs = allLogs.filter(log => {
                const logDate = new Date(log.timestamp * 1000).toDateString();
                return logDate === today;
            });
            document.getElementById('todayEvents').textContent = todayLogs.length;
        }
        
        // Display logs with current filter
        function displayLogs() {
            const logList = document.getElementById('logList');
            
            let filteredLogs = allLogs;
            if (currentFilter !== 'all') {
                filteredLogs = allLogs.filter(log => log.event === currentFilter);
            }
            
            if (filteredLogs.length === 0) {
                logList.innerHTML = `
                    <div class="empty-state">
                        <div class="empty-state-icon">📭</div>
                        <p>Nessun evento trovato</p>
                    </div>
                `;
                return;
            }
            
            logList.innerHTML = filteredLogs.map(log => {
                const date = new Date(log.timestamp * 1000);
                const dateStr = date.toLocaleDateString('it-IT');
                const timeStr = date.toLocaleTimeString('it-IT');
                
                const eventText = log.event === 'on' ? '💡 LED ACCESO' : '🌙 LED SPENTO';
                const eventClass = log.event;
                
                return `
                    <div class="log-entry ${eventClass}">
                        <div class="log-time">${dateStr} • ${timeStr}</div>
                        <div class="log-event">${eventText}</div>
                        <div class="log-details">
                            Lux: ${log.lux.toFixed(1)} • 
                            Movimento: ${log.motion ? 'Attivo' : 'Fermo'} • 
                            Modo: ${log.mode || 'auto'}
                        </div>
                    </div>
                `;
            }).join('');
        }
        
        // Filter logs
        function filterLogs(filter) {
            currentFilter = filter;
            
            // Update filter buttons
            document.querySelectorAll('.filter-btn').forEach(btn => {
                btn.classList.remove('active');
            });
            event.target.classList.add('active');
            
            displayLogs();
        }
        
        // Clear all logs
        async function clearLogs() {
            if (!confirm('Sei sicuro di voler cancellare tutti i log?')) {
                return;
            }
            
            try {
                const response = await fetch('/api/logs', {
                    method: 'DELETE'
                });
                
                const data = await response.json();
                if (data.success) {
                    allLogs = [];
                    updateStats();
                    displayLogs();
                } else {
                    alert('Errore nella cancellazione: ' + data.message);
                }
            } catch (error) {
                alert('Errore di comunicazione: ' + error.message);
            }
        }
        
        // Auto-refresh every 30 seconds
        setInterval(loadLogs, 30000);
        
        // Initial load
        window.addEventListener('load', loadLogs);
    </script>
</body>
</html>
)rawliteral";

#endif // WIFI_PAGES_H
