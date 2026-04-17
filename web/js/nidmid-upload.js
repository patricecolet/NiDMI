/**
 * Affiche un message de statut d'upload
 */
function showUploadStatus(message, statusType = 'info') {
    const statusDiv = document.getElementById('seqUploadStatus');
    if (!statusDiv) return;
    
    // Supprimer les classes précédentes
    statusDiv.className = 'seq-status';
    
    if (statusType === 'success') {
        statusDiv.classList.add('seq-status-success');
    } else if (statusType === 'error') {
        statusDiv.classList.add('seq-status-error');
    } else {
        statusDiv.classList.add('seq-status-info');
    }
    
    statusDiv.textContent = message;
    statusDiv.style.display = 'block';
}

/**
 * Formate les bytes en taille lisible
 */
function formatBytes(bytes) {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return Math.round((bytes / Math.pow(k, i)) * 100) / 100 + ' ' + sizes[i];
}

/**
 * Valide et upload le fichier séquenceur
 */
async function uploadFile() {
    const fileInput = document.getElementById('nidmidFile');
    const file = fileInput.files[0];
    
    if (!file) {
        showUploadStatus('Sélectionnez un fichier', 'info');
        return;
    }
    
    // Validation: extension
    const validExtensions = ['.nidmid', '.bin'];
    const fileName = file.name.toLowerCase();
    const isValidExt = validExtensions.some(ext => fileName.endsWith(ext));
    
    if (!isValidExt) {
        showUploadStatus('❌ Format invalide: utilisez .nidmid ou .bin', 'error');
        return;
    }
    
    // Validation: taille (max 512 KB)
    const MAX_SIZE = 512 * 1024;  // 512 KB
    if (file.size > MAX_SIZE) {
        showUploadStatus(`❌ Fichier trop volumineux: ${formatBytes(file.size)} (max: ${formatBytes(MAX_SIZE)})`, 'error');
        return;
    }
    
    // Afficher le statut d'upload en cours
    showUploadStatus(`⏳ Upload en cours... (${formatBytes(file.size)})`, 'info');
    console.log(`[Upload] Starting upload: ${file.name} (${formatBytes(file.size)})`);
    
    try {
        const arrayBuffer = await file.arrayBuffer();
        
        console.log(`[Upload] File read into memory, ${formatBytes(arrayBuffer.byteLength)}`);
        
        // Fetch avec timeout long pour les gros fichiers
        const controller = new AbortController();
        const timeoutId = setTimeout(() => {
            controller.abort();
            console.log(`[Upload] Timeout triggered after 3 minutes`);
        }, 180000); // 3 minutes de timeout
        
        console.log(`[Upload] Sending POST to /api/sequencer/upload`);
        
        const response = await fetch('/api/sequencer/upload', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/octet-stream'
            },
            body: arrayBuffer,
            signal: controller.signal
        });
        
        clearTimeout(timeoutId);
        
        console.log(`[Upload] Response received: status=${response.status}`);
        
        let responseData = {};
        try {
            const contentType = response.headers.get('content-type');
            console.log(`[Upload] Content-Type: ${contentType}`);
            if (contentType && contentType.includes('application/json')) {
                responseData = await response.json();
            } else {
                responseData = { status: response.ok ? 'success' : 'error', message: `HTTP ${response.status}` };
            }
        } catch (e) {
            console.error(`[Upload] Failed to parse response:`, e);
            responseData = { status: 'error', message: 'Invalid response format' };
        }
        
        console.log(`[Upload] Response data:`, responseData);
        
        if (!response.ok) {
            const errorMsg = responseData?.message || `HTTP ${response.status}: ${response.statusText}`;
            showUploadStatus(`❌ ${errorMsg}`, 'error');
            console.error('[Upload] Failed:', errorMsg, responseData);
            return;
        }
        
        showUploadStatus(`✅ Fichier uploadé avec succès (${formatBytes(file.size)})`, 'success');
        console.log('[Upload] ✅ Success:', responseData);
        
        // Recharger l'aperçu
        setTimeout(() => {
            console.log('[Upload] Loading preview...');
            loadView();
        }, 500);
        
    } catch (error) {
        console.error('[Upload] Exception:', error);
        if (error.name === 'AbortError') {
            showUploadStatus(`❌ Timeout: le serveur n'a pas répondu (> 3min)`, 'error');
            console.error('[Upload] Timeout after 3 minutes');
        } else {
            showUploadStatus(`❌ Erreur: ${error.message}`, 'error');
            console.error('[Upload] Network error:', error);
        }
    }
}

/**
 * Télécharge le fichier séquenceur actuel
 */
async function downloadSequencerFile() {
    try {
        const response = await fetch('/api/sequencer/download');
        
        if (!response.ok) {
            alert('Aucun fichier à télécharger');
            return;
        }
        
        const blob = await response.blob();
        const url = URL.createObjectURL(blob);
        
        const link = document.createElement('a');
        const timestamp = new Date().toISOString().replace(/[:.]/g, '-').slice(0, -5);
        link.href = url;
        link.download = `sequencer_${timestamp}.nidmid`;
        
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
        URL.revokeObjectURL(url);
        
        console.log('[Download] ✅ Fichier téléchargé');
        
    } catch (error) {
        console.error('[Download error]', error);
        alert('Erreur lors du téléchargement');
    }
}

/**
 * Initialise le chargeur NIDMID
 */
function initNidmidLoader() {
    const fileInput = document.getElementById('nidmidFile');
    if (fileInput) {
        fileInput.addEventListener('change', uploadFile);
    }
    
    // Charger l'aperçu initial
    loadView();
}