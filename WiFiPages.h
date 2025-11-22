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
        </div>
    </div>
</body>
</html>
)rawliteral";

#endif // WIFI_PAGES_H
