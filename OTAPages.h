#ifndef OTA_PAGES_H
#define OTA_PAGES_H

#include <Arduino.h>

// HTML OTA Page for firmware updates (stored in PROGMEM)
const char OTA_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Aggiornamento Firmware - OTA</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
            display: flex;
            align-items: center;
            justify-content: center;
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
            color: #666;
            text-align: center;
            margin-bottom: 30px;
            font-size: 14px;
        }
        .info-box {
            background: #f0f4ff;
            border-left: 4px solid #667eea;
            padding: 20px;
            border-radius: 8px;
            margin-bottom: 25px;
        }
        .info-row {
            display: flex;
            justify-content: space-between;
            padding: 8px 0;
            border-bottom: 1px solid #e0e0e0;
        }
        .info-row:last-child {
            border-bottom: none;
        }
        .info-label {
            font-weight: 600;
            color: #555;
            font-size: 14px;
        }
        .info-value {
            color: #333;
            font-size: 14px;
        }
        .card {
            background: #f8f9fa;
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 20px;
        }
        .card-title {
            font-size: 18px;
            font-weight: 600;
            color: #333;
            margin-bottom: 15px;
            display: flex;
            align-items: center;
            gap: 8px;
        }
        .form-group {
            margin-bottom: 15px;
        }
        label {
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: 500;
            font-size: 14px;
        }
        input[type="text"], input[type="url"], input[type="file"] {
            width: 100%;
            padding: 12px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            font-size: 16px;
            transition: border-color 0.3s;
        }
        input:focus {
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
        .btn:hover:not(:disabled) {
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(102, 126, 234, 0.4);
        }
        .btn:disabled {
            opacity: 0.6;
            cursor: not-allowed;
        }
        .btn-danger {
            background: linear-gradient(135deg, #e74c3c 0%, #c0392b 100%);
        }
        .progress-container {
            display: none;
            margin-top: 20px;
        }
        .progress-bar {
            width: 100%;
            height: 30px;
            background: #e0e0e0;
            border-radius: 15px;
            overflow: hidden;
            margin-bottom: 10px;
        }
        .progress-fill {
            height: 100%;
            background: linear-gradient(90deg, #667eea 0%, #764ba2 100%);
            width: 0%;
            transition: width 0.3s;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            font-weight: 600;
            font-size: 14px;
        }
        .log-container {
            max-height: 200px;
            overflow-y: auto;
            background: #2c3e50;
            color: #ecf0f1;
            padding: 15px;
            border-radius: 8px;
            font-family: 'Courier New', monospace;
            font-size: 13px;
            line-height: 1.6;
        }
        .log-entry {
            margin-bottom: 5px;
        }
        .log-error {
            color: #e74c3c;
        }
        .log-success {
            color: #27ae60;
        }
        .log-warning {
            color: #f39c12;
        }
        .alert {
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
            font-size: 14px;
        }
        .alert-warning {
            background: #fff3cd;
            border-left: 4px solid #f39c12;
            color: #856404;
        }
        .alert-danger {
            background: #f8d7da;
            border-left: 4px solid #e74c3c;
            color: #721c24;
        }
        .alert-success {
            background: #d4edda;
            border-left: 4px solid #27ae60;
            color: #155724;
        }
        .file-upload-area {
            border: 2px dashed #667eea;
            border-radius: 8px;
            padding: 30px;
            text-align: center;
            cursor: pointer;
            transition: all 0.3s;
            background: #f8f9fa;
        }
        .file-upload-area:hover {
            background: #e9ecef;
            border-color: #764ba2;
        }
        .file-upload-area.drag-over {
            background: #667eea;
            color: white;
        }
        .file-icon {
            font-size: 48px;
            margin-bottom: 10px;
        }
        .small-text {
            font-size: 12px;
            color: #888;
            margin-top: 5px;
        }
        .back-link {
            text-align: center;
            margin-top: 20px;
        }
        .back-link a {
            color: #667eea;
            text-decoration: none;
            font-weight: 600;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔄 Aggiornamento Firmware</h1>
        <p class="subtitle">Sistema OTA (Over-The-Air)</p>
        
        <div class="info-box">
            <div class="info-row">
                <span class="info-label">Partizione Corrente:</span>
                <span class="info-value" id="currentPartition">--</span>
            </div>
            <div class="info-row">
                <span class="info-label">Spazio Disponibile:</span>
                <span class="info-value" id="availableSpace">--</span>
            </div>
            <div class="info-row">
                <span class="info-label">Dimensione Max Firmware:</span>
                <span class="info-value" id="maxFirmwareSize">--</span>
            </div>
            <div class="info-row">
                <span class="info-label">Rollback Disponibile:</span>
                <span class="info-value" id="canRollback">--</span>
            </div>
        </div>
        
        <div id="alertContainer"></div>
        
        <!-- Method 1: URL Remote -->
        <div class="card">
            <div class="card-title">🌐 Metodo 1: Aggiornamento da URL</div>
            <form id="urlForm" onsubmit="return updateFromURL(event)">
                <div class="form-group">
                    <label for="firmwareUrl">URL Firmware (.bin)</label>
                    <input type="url" id="firmwareUrl" name="firmwareUrl" 
                           placeholder="https://example.com/firmware.bin" required>
                    <div class="small-text">Inserisci l'URL completo del file firmware (HTTP o HTTPS)</div>
                </div>
                <button type="submit" class="btn" id="btnUrlUpdate">
                    📥 Scarica e Aggiorna
                </button>
            </form>
        </div>
        
        <!-- Method 2: Local Upload -->
        <div class="card">
            <div class="card-title">📁 Metodo 2: Upload Locale</div>
            <form id="uploadForm" onsubmit="return updateFromFile(event)">
                <div class="form-group">
                    <div class="file-upload-area" id="fileUploadArea" onclick="document.getElementById('firmwareFile').click()">
                        <div class="file-icon">📦</div>
                        <div>Clicca per selezionare o trascina qui il file .bin</div>
                        <div class="small-text" id="fileInfo">Nessun file selezionato</div>
                    </div>
                    <input type="file" id="firmwareFile" accept=".bin" style="display: none;" onchange="fileSelected()">
                </div>
                <button type="submit" class="btn" id="btnFileUpdate" disabled>
                    📤 Carica e Aggiorna
                </button>
            </form>
        </div>
        
        <!-- Progress -->
        <div class="progress-container" id="progressContainer">
            <div class="progress-bar">
                <div class="progress-fill" id="progressFill">0%</div>
            </div>
            <div class="log-container" id="logContainer"></div>
        </div>
        
        <!-- Rollback -->
        <div class="card" id="rollbackCard" style="display: none;">
            <div class="card-title">⚠️ Rollback Firmware</div>
            <div class="alert alert-warning">
                <strong>Attenzione!</strong> Questa operazione ripristinerà la versione precedente del firmware.
                Il dispositivo verrà riavviato immediatamente.
            </div>
            <button type="button" class="btn btn-danger" onclick="performRollback()">
                🔙 Ripristina Versione Precedente
            </button>
        </div>
        
        <div class="back-link">
            <a href="/dashboard">← Torna alla Dashboard</a>
        </div>
    </div>
    
    <script>
        let updateInProgress = false;
        
        // Load OTA info on page load
        async function loadOTAInfo() {
            try {
                const response = await fetch('/api/ota/info');
                const data = await response.json();
                
                document.getElementById('currentPartition').textContent = data.current_partition;
                document.getElementById('availableSpace').textContent = formatBytes(data.available_space);
                document.getElementById('maxFirmwareSize').textContent = formatBytes(data.max_firmware_size);
                document.getElementById('canRollback').textContent = data.can_rollback ? 'Sì ✅' : 'No ❌';
                
                if (data.can_rollback) {
                    document.getElementById('rollbackCard').style.display = 'block';
                }
            } catch (error) {
                console.error('Error loading OTA info:', error);
                showAlert('danger', 'Errore nel caricamento delle informazioni OTA');
            }
        }
        
        // Format bytes to human readable
        function formatBytes(bytes) {
            if (bytes === 0) return '0 Bytes';
            const k = 1024;
            const sizes = ['Bytes', 'KB', 'MB', 'GB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            return Math.round((bytes / Math.pow(k, i)) * 100) / 100 + ' ' + sizes[i];
        }
        
        // Show alert
        function showAlert(type, message) {
            const alertContainer = document.getElementById('alertContainer');
            alertContainer.innerHTML = `<div class="alert alert-${type}">${message}</div>`;
        }
        
        // Clear alert
        function clearAlert() {
            document.getElementById('alertContainer').innerHTML = '';
        }
        
        // Add log entry
        function addLog(message, type = 'info') {
            const logContainer = document.getElementById('logContainer');
            const entry = document.createElement('div');
            entry.className = 'log-entry';
            
            if (type === 'error') entry.classList.add('log-error');
            if (type === 'success') entry.classList.add('log-success');
            if (type === 'warning') entry.classList.add('log-warning');
            
            const timestamp = new Date().toLocaleTimeString();
            entry.textContent = `[${timestamp}] ${message}`;
            
            logContainer.appendChild(entry);
            logContainer.scrollTop = logContainer.scrollHeight;
        }
        
        // Update progress
        function updateProgress(percent, message) {
            const progressFill = document.getElementById('progressFill');
            progressFill.style.width = percent + '%';
            progressFill.textContent = percent + '%';
            
            if (message) {
                addLog(message);
            }
        }
        
        // Show progress
        function showProgress() {
            document.getElementById('progressContainer').style.display = 'block';
            document.getElementById('logContainer').innerHTML = '';
        }
        
        // Hide progress
        function hideProgress() {
            document.getElementById('progressContainer').style.display = 'none';
        }
        
        // Update from URL
        async function updateFromURL(event) {
            event.preventDefault();
            
            if (updateInProgress) {
                showAlert('warning', 'Un aggiornamento è già in corso');
                return false;
            }
            
            const url = document.getElementById('firmwareUrl').value;
            
            if (!url.endsWith('.bin')) {
                showAlert('danger', 'Il file deve avere estensione .bin');
                return false;
            }
            
            if (!confirm('Sei sicuro di voler avviare l\'aggiornamento?\nIl dispositivo verrà riavviato al termine.')) {
                return false;
            }
            
            updateInProgress = true;
            clearAlert();
            showProgress();
            
            document.getElementById('btnUrlUpdate').disabled = true;
            document.getElementById('btnFileUpdate').disabled = true;
            
            addLog('Avvio aggiornamento da URL...', 'info');
            addLog('URL: ' + url, 'info');
            
            try {
                updateProgress(10, 'Connessione al server...');
                
                const response = await fetch('/api/ota/url', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: 'url=' + encodeURIComponent(url)
                });
                
                const data = await response.json();
                
                if (data.success) {
                    updateProgress(100, 'Aggiornamento completato!');
                    addLog('✓ Aggiornamento riuscito!', 'success');
                    addLog('Il dispositivo si sta riavviando...', 'info');
                    
                    showAlert('success', '✓ Aggiornamento completato! Il dispositivo si sta riavviando...');
                    
                    // Monitor status
                    monitorUpdateStatus();
                } else {
                    throw new Error(data.message || 'Aggiornamento fallito');
                }
            } catch (error) {
                addLog('✗ Errore: ' + error.message, 'error');
                showAlert('danger', '✗ Errore durante l\'aggiornamento: ' + error.message);
                updateInProgress = false;
                document.getElementById('btnUrlUpdate').disabled = false;
                document.getElementById('btnFileUpdate').disabled = false;
            }
            
            return false;
        }
        
        // File selected
        function fileSelected() {
            const fileInput = document.getElementById('firmwareFile');
            const fileInfo = document.getElementById('fileInfo');
            const btnUpdate = document.getElementById('btnFileUpdate');
            
            if (fileInput.files.length > 0) {
                const file = fileInput.files[0];
                fileInfo.textContent = file.name + ' (' + formatBytes(file.size) + ')';
                btnUpdate.disabled = false;
            } else {
                fileInfo.textContent = 'Nessun file selezionato';
                btnUpdate.disabled = true;
            }
        }
        
        // Update from file
        async function updateFromFile(event) {
            event.preventDefault();
            
            if (updateInProgress) {
                showAlert('warning', 'Un aggiornamento è già in corso');
                return false;
            }
            
            const fileInput = document.getElementById('firmwareFile');
            
            if (fileInput.files.length === 0) {
                showAlert('danger', 'Seleziona un file .bin');
                return false;
            }
            
            const file = fileInput.files[0];
            
            if (!file.name.endsWith('.bin')) {
                showAlert('danger', 'Il file deve avere estensione .bin');
                return false;
            }
            
            if (!confirm('Sei sicuro di voler avviare l\'aggiornamento?\nIl dispositivo verrà riavviato al termine.')) {
                return false;
            }
            
            updateInProgress = true;
            clearAlert();
            showProgress();
            
            document.getElementById('btnUrlUpdate').disabled = true;
            document.getElementById('btnFileUpdate').disabled = true;
            
            addLog('Avvio upload firmware...', 'info');
            addLog('File: ' + file.name + ' (' + formatBytes(file.size) + ')', 'info');
            
            try {
                const formData = new FormData();
                formData.append('firmware', file);
                
                const xhr = new XMLHttpRequest();
                
                xhr.upload.addEventListener('progress', (e) => {
                    if (e.lengthComputable) {
                        const percent = Math.round((e.loaded / e.total) * 100);
                        updateProgress(percent, 'Upload: ' + formatBytes(e.loaded) + ' / ' + formatBytes(e.total));
                    }
                });
                
                xhr.addEventListener('load', () => {
                    if (xhr.status === 200) {
                        const data = JSON.parse(xhr.responseText);
                        
                        if (data.success) {
                            updateProgress(100, 'Aggiornamento completato!');
                            addLog('✓ Aggiornamento riuscito!', 'success');
                            addLog('Il dispositivo si sta riavviando...', 'info');
                            showAlert('success', '✓ Aggiornamento completato! Il dispositivo si sta riavviando...');
                        } else {
                            throw new Error(data.message || 'Aggiornamento fallito');
                        }
                    } else {
                        throw new Error('HTTP ' + xhr.status);
                    }
                });
                
                xhr.addEventListener('error', () => {
                    throw new Error('Errore di rete durante l\'upload');
                });
                
                xhr.open('POST', '/api/ota/upload');
                xhr.send(formData);
                
            } catch (error) {
                addLog('✗ Errore: ' + error.message, 'error');
                showAlert('danger', '✗ Errore durante l\'aggiornamento: ' + error.message);
                updateInProgress = false;
                document.getElementById('btnUrlUpdate').disabled = false;
                document.getElementById('btnFileUpdate').disabled = false;
            }
            
            return false;
        }
        
        // Monitor update status (for URL method)
        function monitorUpdateStatus() {
            const interval = setInterval(async () => {
                try {
                    const response = await fetch('/api/ota/status');
                    const data = await response.json();
                    
                    if (data.state === 'DOWNLOADING' || data.state === 'WRITING') {
                        updateProgress(data.progress, 'Stato: ' + data.state);
                    } else if (data.state === 'SUCCESS') {
                        clearInterval(interval);
                    } else if (data.state === 'ERROR') {
                        clearInterval(interval);
                        addLog('✗ Errore: ' + data.error, 'error');
                    }
                } catch (error) {
                    // Connection lost, device probably rebooting
                    clearInterval(interval);
                }
            }, 1000);
        }
        
        // Perform rollback
        async function performRollback() {
            if (!confirm('Sei sicuro di voler ripristinare la versione precedente del firmware?\nIl dispositivo verrà riavviato immediatamente.')) {
                return;
            }
            
            try {
                showAlert('warning', 'Rollback in corso...');
                
                const response = await fetch('/api/ota/rollback', { method: 'POST' });
                const data = await response.json();
                
                if (data.success) {
                    showAlert('success', '✓ Rollback avviato! Il dispositivo si sta riavviando...');
                } else {
                    showAlert('danger', '✗ Errore durante il rollback: ' + data.message);
                }
            } catch (error) {
                showAlert('danger', '✗ Errore: ' + error.message);
            }
        }
        
        // Drag and drop support
        const uploadArea = document.getElementById('fileUploadArea');
        
        uploadArea.addEventListener('dragover', (e) => {
            e.preventDefault();
            uploadArea.classList.add('drag-over');
        });
        
        uploadArea.addEventListener('dragleave', () => {
            uploadArea.classList.remove('drag-over');
        });
        
        uploadArea.addEventListener('drop', (e) => {
            e.preventDefault();
            uploadArea.classList.remove('drag-over');
            
            const files = e.dataTransfer.files;
            if (files.length > 0) {
                document.getElementById('firmwareFile').files = files;
                fileSelected();
            }
        });
        
        // Load info on page load
        window.addEventListener('load', loadOTAInfo);
    </script>
</body>
</html>
)rawliteral";

#endif // OTA_PAGES_H
