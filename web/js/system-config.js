/**
 * Configuration des paramètres système
 */
 
let isS3 = false;  /* Détecté depuis caps.board */
let touchEnabled = false; /* État courant du support touch */

/* Détecte le type de board à partir de la globale `caps` (déjà chargée par loadCaps()).
 * Évite un double fetch /api/pins/caps qui pouvait échouer (body vide) sous charge. */
function checkBoardType() {
  try {
    if (typeof caps === 'undefined' || !caps || !caps.board) {
      console.warn('[checkBoardType] caps non disponible, skip');
      return;
    }
    isS3 = caps.board.toLowerCase().includes('s3');
    
    /* La section est visible sur les deux cartes : le nombre de composants par cycle
       concerne aussi la C3, plus contrainte en CPU. Seule la ligne touch reste S3. */
    const systemSection = $('#systemSettingsSection');
    if (systemSection) {
      systemSection.style.display = 'block';
    }
    const touchRow = $('#touchRow');
    if (touchRow) {
      touchRow.style.display = isS3 ? 'block' : 'none';
    }

    loadSystemConfig();
  } catch (e) {
    console.error('[checkBoardType] Erreur:', e);
  }
}

async function loadSystemConfig() {
  try {
    const r = await fetch('/api/system/get');
    if (!r.ok) {
      console.warn('[loadSystemConfig] Erreur API:', r.status);
      return;
    }
    const d = await r.json();
    const checkbox = $('#touchEnabled');
    touchEnabled = d.touchEnabled === true;
    if (checkbox) {
      checkbox.checked = touchEnabled;
    }

    const sliceInput = $('#componentsPerCycle');
    if (sliceInput && d.componentsPerCycle !== undefined) {
      sliceInput.value = d.componentsPerCycle;
    }

    // Mettre à jour la visibilité du bouton de calibration touch
    const calContainer = $('#touchCalibrateContainer');
    if (calContainer) {
      calContainer.style.display = (isS3 && touchEnabled) ? 'block' : 'none';
    }
  } catch (e) {
    console.error('[loadSystemConfig] Erreur:', e);
  }
}

async function saveSystemConfig() {
  const checkbox = $('#touchEnabled');
  const sliceInput = $('#componentsPerCycle');

  const touchEnabledValue = checkbox ? checkbox.checked : touchEnabled;

  /* touchEnabled n'est envoye que sur S3 : ailleurs la case n'est pas affichee et
     poster sa valeur ecraserait le reglage avec un faux "decoche". */
  const params = [];
  if (isS3 && checkbox) params.push(`touchEnabled=${touchEnabledValue ? 'true' : 'false'}`);
  if (sliceInput && sliceInput.value !== '') params.push(`componentsPerCycle=${parseInt(sliceInput.value, 10)}`);
  if (params.length === 0) return;

  try {
    const r = await fetch('/api/system/set', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: params.join('&')
    });
    
    const d = await r.json();
    const msgEl = $('#systemMsg');
    if (msgEl) {
      if (d.status === 'ok') {
        touchEnabled = touchEnabledValue;
        const calContainer = $('#touchCalibrateContainer');
        if (calContainer) {
          calContainer.style.display = (isS3 && touchEnabled) ? 'block' : 'none';
        }
        msgEl.textContent = 'Configuration enregistrée, rechargement des composants...';
        msgEl.style.color = '#10b981';
        /* Recharger les pins après un court délai */
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
        /* Recharger la page après un court délai */
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

/* Gérer le formulaire (sera appelé depuis app.js après chargement) */
function initSystemConfig() {
  /* Vérifier le type de board d'abord */
  checkBoardType();

  /* Gérer le formulaire */
  const systemForm = $('#system');
  if (systemForm) {
    systemForm.addEventListener('submit', (e) => {
      e.preventDefault();
      saveSystemConfig();
    });
  }

  /* Bouton global "Calibrer touch" (pins panel, seulement S3 + touch activé) */
  const touchCalBtn = $('#touchCalibrateBtn');
  if (touchCalBtn) {
    touchCalBtn.addEventListener('click', (e) => {
      e.preventDefault();
      const msgEl = $('#touchCalibrateMsg');

      if (typeof websocket === 'undefined' || !websocket || websocket.readyState !== WebSocket.OPEN) {
        if (msgEl) {
          msgEl.textContent = 'WebSocket non connecté';
          msgEl.style.color = '#ef4444';
        }
        return;
      }

      if (msgEl) {
        msgEl.textContent = 'Calibration touch en cours...';
        msgEl.style.color = '#6b7280';
      }

      websocket.send('TOUCH_CALIBRATE_ALL');
    });
  }
  
  /* Gérer le bouton Clear NVS */
  const clearNvsBtn = $('#clearNvsBtn');
  if (clearNvsBtn) {
    clearNvsBtn.addEventListener('click', (e) => {
      e.preventDefault();
      clearNVS();
    });
  }

  /* Reset appareil (comme le bouton reset physique) */
  const resetBtn = $('#resetDeviceBtn');
  if (resetBtn) {
    resetBtn.addEventListener('click', async (e) => {
      e.preventDefault();
      try {
        const msgEl = $('#clearNvsMsg');
        if (msgEl) {
          msgEl.textContent = 'Redémarrage...';
          msgEl.style.color = '#6b7280';
        }
        await fetch('/api/system/reboot', { method: 'POST' });
      } catch (err) {
        console.log('Erreur reboot:', err);
      } finally {
        setTimeout(() => location.reload(), 3000);
      }
    });
  }
}
