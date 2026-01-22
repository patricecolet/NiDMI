/**
 * Configuration des paramètres système
 */

let isS3 = false;  // Détecté depuis caps.board

// Vérifier si c'est un ESP32-S3
async function checkBoardType() {
  try {
    const r = await fetch('/api/pins/caps');
    if (!r.ok) return;
    const caps = await r.json();
    isS3 = caps.board && caps.board.toLowerCase().includes('s3');
    
    // Afficher/masquer la section système selon le type de board
    const systemSection = $('#systemSettingsSection');
    if (systemSection) {
      systemSection.style.display = isS3 ? 'block' : 'none';
    }
    
    // Charger la config seulement si c'est un S3
    if (isS3) {
      loadSystemConfig();
    }
  } catch (e) {
    console.error('[checkBoardType] Erreur:', e);
  }
}

async function loadSystemConfig() {
  if (!isS3) return;  // Ne rien faire si ce n'est pas un S3
  
  try {
    const r = await fetch('/api/system/get');
    if (!r.ok) {
      console.warn('[loadSystemConfig] Erreur API:', r.status);
      return;
    }
    const d = await r.json();
    const checkbox = $('#touchEnabled');
    if (checkbox) {
      checkbox.checked = d.touchEnabled === true;
    }
  } catch (e) {
    console.error('[loadSystemConfig] Erreur:', e);
  }
}

async function saveSystemConfig() {
  if (!isS3) return;  // Ne rien faire si ce n'est pas un S3
  
  const checkbox = $('#touchEnabled');
  if (!checkbox) return;
  
  const touchEnabled = checkbox.checked;
  
  try {
    const r = await fetch('/api/system/set', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: `touchEnabled=${touchEnabled ? 'true' : 'false'}`
    });
    
    const d = await r.json();
    const msgEl = $('#systemMsg');
    if (msgEl) {
      if (d.status === 'ok') {
        msgEl.textContent = 'Configuration enregistrée, rechargement des composants...';
        msgEl.style.color = '#10b981';
        // Recharger les pins après un court délai
        setTimeout(() => {
          if (typeof loadConfiguredPins === 'function') {
            loadConfiguredPins();
          }
          msgEl.textContent = '';
        }, 1000);
      } else {
        msgEl.textContent = 'Erreur: ' + (d.message || 'Unknown error');
        msgEl.style.color = '#ef4444';
      }
    }
  } catch (e) {
    console.error('[saveSystemConfig] Erreur:', e);
    const msgEl = $('#systemMsg');
    if (msgEl) {
      msgEl.textContent = 'Erreur de connexion';
      msgEl.style.color = '#ef4444';
    }
  }
}

async function clearNVS() {
  if (!confirm('⚠️ Êtes-vous sûr ? Cela supprimera toutes les configurations (pins, OSC, WiFi, etc.) et réinitialisera complètement le système.')) {
    return;
  }
  
  try {
    const r = await fetch('/api/cache/clear', {
      method: 'POST'
    });
    
    const d = await r.json();
    const msgEl = $('#clearNvsMsg');
    if (msgEl) {
      if (d.status === 'ok') {
        msgEl.textContent = 'NVS effacée avec succès. Rechargement...';
        msgEl.style.color = '#10b981';
        // Recharger la page après un court délai
        setTimeout(() => {
          window.location.reload();
        }, 1500);
      } else {
        msgEl.textContent = 'Erreur: ' + (d.message || 'Unknown error');
        msgEl.style.color = '#ef4444';
      }
    }
  } catch (e) {
    console.error('[clearNVS] Erreur:', e);
    const msgEl = $('#clearNvsMsg');
    if (msgEl) {
      msgEl.textContent = 'Erreur de connexion';
      msgEl.style.color = '#ef4444';
    }
  }
}

// Gérer le formulaire (sera appelé depuis app.js après chargement)
function initSystemConfig() {
  // Vérifier le type de board d'abord
  checkBoardType();
  
  // Gérer le formulaire
  const systemForm = $('#system');
  if (systemForm) {
    systemForm.addEventListener('submit', (e) => {
      e.preventDefault();
      saveSystemConfig();
    });
  }
  
  // Gérer le bouton Clear NVS
  const clearNvsBtn = $('#clearNvsBtn');
  if (clearNvsBtn) {
    clearNvsBtn.addEventListener('click', (e) => {
      e.preventDefault();
      clearNVS();
    });
  }
}
