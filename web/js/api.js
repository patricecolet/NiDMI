/* Fonctions d'appels API et initialisation des formulaires */

async function loadStatus(){
 const r=await fetch('/api/status');
 const d=await r.json();
 $('#apSsid').textContent=d.ap_ssid;
 $('#apIp').textContent=d.ap_ip;
 $('#staSsid').textContent=d.sta_ssid;
 $('#staIp').textContent=d.sta_ip;
 const s=$('#staStatus');
 s.textContent=d.sta_connected?'Connecté':'Déconnecté';
 s.style.color=d.sta_connected?'#059669':'#dc2626';
 $('#mdnsAddress').textContent=d.mdns_address||'-';
 let oscInfo='';
 if(d.osc_target==='ip'&&d.osc_ip){
 oscInfo=d.osc_ip+':'+d.osc_port;
 }else if(d.osc_target==='ap'){
 oscInfo='Broadcast AP (192.168.4.255:'+d.osc_port+')';
 }else{
 oscInfo='Broadcast STA ('+(d.sta_ip||'0.0.0.0')+':'+d.osc_port+')';
 }
 $('#oscConfig').textContent=oscInfo;
 const wdc = document.getElementById('webDebugConsoleSection');
 if (wdc) {
  wdc.style.display = d.web_debug_console ? 'block' : 'none';
 }
}

async function loadMdns(){
 const r=await fetch('/api/mdns/status');
 const d=await r.json();
 $('#mdnsName').value=d.name;
}

async function loadOscConfig(){
 try {
 const r=await fetch('/api/osc/status');
 const d=await r.json();
 if($('#oscPort')) $('#oscPort').value=d.port||8000;
 if($('#oscIp')) $('#oscIp').value=d.ip||'';
 if($('#oscBroadcast')) $('#oscBroadcast').checked=!!d.broadcast;
 /* Liens de diffusion : masque "ap+sta+usb" renvoye par /api/osc/status */
 const links=(d.interface||'ap').split('+');
 if($('#oscLinkAp')) $('#oscLinkAp').checked=links.indexOf('ap')>=0;
 if($('#oscLinkSta')) $('#oscLinkSta').checked=links.indexOf('sta')>=0;
 if($('#oscLinkUsb')) $('#oscLinkUsb').checked=links.indexOf('usb')>=0;
 /* La case USB n'existe que si le firmware est le variant --usb-net */
 const usbRow=$('#oscLinkUsbRow');
 if(usbRow) usbRow.style.display=d.usb_link?'block':'none';
 const oscOutCb = $('#oscEnabled2');
 if(oscOutCb && oscOutCb.type === 'checkbox' && d.output_all_enabled !== undefined) {
   oscOutCb.checked = !!d.output_all_enabled;
 }
 const oscOutMsg = $('#oscOutputMsg');
 if(oscOutMsg) { oscOutMsg.textContent = ''; oscOutMsg.style.color = ''; }

 if (typeof updateOscForm === 'function') updateOscForm();
 } catch(err) {
 console.log('Erreur chargement OSC:', err);
 }
}

async function loadStaConfig(){
 try {
 const r=await fetch('/api/sta/status');
 const d=await r.json();
 if($('#ssid')) $('#ssid').value=d.ssid||'';
 
 if($('#pass')) {
 const currentValue = $('#pass').value;
 $('#pass').placeholder=d.has_pass ? '•••••••• (déjà configuré)' : 'Mot de passe';
 
 if(!currentValue || currentValue === '') {
 $('#pass').value='';
 }
 }
 } catch(err) {
  console.log('Erreur chargement STA:', err);
 }
}

async function loadMidiInterfaces(){
  try {
    /* Charger l'état RTP-MIDI */
    const rtpRes = await fetch('/api/rtp/status');
    if(rtpRes.ok) {
      const rtpData = await rtpRes.json();
      if($('#rtpMidiEnabled')) {
        $('#rtpMidiEnabled').checked = !!rtpData.enabled;
      }
    }
    
    /* USB-MIDI : état compile-time + runtime (pas de toggle NVS) */
    const usbRes = await fetch('/api/usbmidi/status');
    if(usbRes.ok) {
      const usbData = await usbRes.json();
      const statusEl = document.getElementById('usbMidiStatus');
      const usbCheckbox = $('#usbMidiEnabled');
      if(usbCheckbox && usbCheckbox.type === 'checkbox') {
        usbCheckbox.disabled = true;
        const compiled = (usbData.compiledEnabled !== undefined) ? !!usbData.compiledEnabled : !!usbData.savedEnabled;
        usbCheckbox.checked = compiled;
      }
      if(statusEl) {
        const compiled = (usbData.compiledEnabled !== undefined) ? !!usbData.compiledEnabled : !!usbData.savedEnabled;
        let statusText = '';
        if(!usbData.supported) {
          statusText = '❌ Non supporté (MCU)';
        } else if(!compiled) {
          statusText = '⛔ Désactivé au build (UsbMidiManager.h)';
        } else if(usbData.connected) {
          statusText = '✅ Connecté';
        } else if(usbData.enabled) {
          statusText = '⚠️ Activé (USB non connecté ou hôte)';
        } else {
          statusText = '❌ Non initialisé';
        }
        statusEl.textContent = statusText;
      }
    }
  } catch(err) {
    console.log('Erreur chargement interfaces MIDI:', err);
  }
}

/* Diffusion : les liens a cocher. Sinon : une IP unique. Les deux ne peuvent
   pas etre actifs en meme temps, d'ou l'echange des deux blocs. */
function updateOscForm() {
 const broadcast = $('#oscBroadcast');
 const ipRow = $('#oscIpRow');
 const linksRow = $('#oscLinksRow');
 if (!broadcast) return;
 if (ipRow) ipRow.style.display = broadcast.checked ? 'none' : 'block';
 if (linksRow) linksRow.style.display = broadcast.checked ? 'block' : 'none';
}

function initForms(){
 const oscBroadcast = $('#oscBroadcast');
 if (oscBroadcast) {
 oscBroadcast.addEventListener('change', updateOscForm);
 updateOscForm();
 }

  /* Sortie OSC globale : enregistrement immédiat (NVS osc_out_all + rechargement runtime) */
  const oscOutCheckbox = $('#oscEnabled2');
  if(oscOutCheckbox && oscOutCheckbox.type === 'checkbox') {
    oscOutCheckbox.addEventListener('change', async () => {
      const enabled = !!oscOutCheckbox.checked;
      const msgEl = $('#oscOutputMsg');
      if(msgEl) {
        msgEl.textContent = enabled ? 'Enregistrement…' : 'Enregistrement…';
        msgEl.style.color = '#6b7280';
      }
      try {
        oscOutCheckbox.disabled = true;
        const fd = new URLSearchParams();
        fd.append('enable', enabled ? 'true' : 'false');
        const resp = await fetch('/api/osc/output-enable', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: fd.toString()
        });
        const d = await resp.json().catch(() => ({}));
        if(!resp.ok || d.status !== 'ok') {
          throw new Error(d.error || ('HTTP ' + resp.status));
        }
        if(msgEl) {
          msgEl.textContent = enabled ? 'Sortie OSC activée (mémorisée)' : 'Sortie OSC désactivée (mémorisée)';
          msgEl.style.color = '#059669';
        }
      } catch(err) {
        console.log('Erreur toggle sortie OSC:', err);
        oscOutCheckbox.checked = !enabled;
        if(msgEl) {
          msgEl.textContent = 'Erreur : impossible d’enregistrer';
          msgEl.style.color = '#dc2626';
        }
      } finally {
        oscOutCheckbox.disabled = false;
      }
    });
  }

  const webDbgCb = $('#webDebugConsoleEnabled');
  const webDbgClear = $('#webDebugConsoleClear');
  if (webDbgCb && webDbgCb.type === 'checkbox') {
    webDbgCb.addEventListener('change', () => {
      if (typeof websocket === 'undefined' || !websocket) return;
      if (websocket.readyState !== WebSocket.OPEN) return;
      websocket.send(webDbgCb.checked ? 'DEBUG_CONSOLE:1' : 'DEBUG_CONSOLE:0');
    });
  }
  if (webDbgClear) {
    webDbgClear.addEventListener('click', () => {
      const pre = document.getElementById('webDebugConsoleOut');
      if (pre) pre.textContent = '';
    });
  }
 
 $('#mdns').addEventListener('submit', async (e) => {
 e.preventDefault();
 const formData = new FormData();
 formData.append('name', $('#mdnsName').value);
 try {
 const r = await fetch('/api/mdns', { method: 'POST', body: formData });
 const d = await r.json();
 $('#mdnsMsg').textContent = d.status === 'ok' ? 'Nom enregistré' : 'Erreur: ' + d.error;
 $('#mdnsMsg').style.color = d.status === 'ok' ? '#059669' : '#dc2626';
 } catch (err) {
 $('#mdnsMsg').textContent = 'Erreur de connexion';
 $('#mdnsMsg').style.color = '#dc2626';
 }
 });
 
 $('#sta').addEventListener('submit', async (e) => {
 e.preventDefault();
 const formData = new FormData();
 formData.append('ssid', $('#ssid').value);
 formData.append('pass', $('#pass').value);
 try {
 const r = await fetch('/api/sta', { method: 'POST', body: formData });
 const d = await r.json();
 $('#staMsg').textContent = d.status === 'ok' ? 'Configuration enregistrée, redémarrage...' : 'Erreur: ' + d.error;
 $('#staMsg').style.color = d.status === 'ok' ? '#059669' : '#dc2626';
 if (d.status === 'ok') {
 setTimeout(() => location.reload(), 2000);
 }
 } catch (err) {
 $('#staMsg').textContent = 'Configuration enregistrée, redémarrage...';
 $('#staMsg').style.color = '#059669';
 setTimeout(() => location.reload(), 5000);
 }
 });
 
 $('#ota').addEventListener('submit', (e) => {
 e.preventDefault();
 const f = $('#otaFile').files[0];
 if (!f) return;
 const prog = $('#otaProgress');
 const msg = $('#otaMsg');
 prog.style.display = 'block'; prog.value = 0;
 msg.style.color = '#059669';
 msg.textContent = 'Préparation...';
 const xhr = new XMLHttpRequest();
 xhr.open('POST', '/api/ota');
 xhr.upload.onprogress = (ev) => {
 if (ev.lengthComputable) {
 const pct = Math.round(ev.loaded / ev.total * 100);
 prog.value = pct;
 msg.textContent = 'Envoi: ' + pct + '%';
 }
 };
 xhr.onload = () => {
 let ok = false, err = xhr.responseText;
 try { const d = JSON.parse(xhr.responseText); ok = d.status === 'ok'; if (d.error) err = d.error; } catch(_e) {}
 if (xhr.status === 200 && ok) {
 prog.value = 100;
 msg.textContent = 'Image reçue, redémarrage... (la page se recharge automatiquement)';
 setTimeout(() => location.reload(), 8000);
 } else {
 msg.style.color = '#dc2626';
 msg.textContent = 'Échec: ' + err;
 prog.style.display = 'none';
 }
 };
 xhr.onerror = () => {
 msg.style.color = '#dc2626';
 msg.textContent = 'Erreur réseau pendant l’envoi';
 prog.style.display = 'none';
 };
 xhr.setRequestHeader('Content-Type', 'application/octet-stream');
 xhr.send(f);
 });

 $('#osc').addEventListener('submit', async (e) => {
 e.preventDefault();
 const formData = new FormData();
 const broadcast = $('#oscBroadcast') && $('#oscBroadcast').checked;
 const links = [];
 if ($('#oscLinkAp') && $('#oscLinkAp').checked) links.push('ap');
 if ($('#oscLinkSta') && $('#oscLinkSta').checked) links.push('sta');
 if ($('#oscLinkUsb') && $('#oscLinkUsb').checked) links.push('usb');
 const mask = links.join('+');

 formData.append('target', broadcast ? (mask || 'ap') : 'ip');
 formData.append('interface', mask || 'ap');
 formData.append('port', $('#oscPort').value);
 formData.append('broadcast', broadcast ? 'true' : 'false');
 if (!broadcast && $('#oscIp').value) {
 formData.append('ip', $('#oscIp').value);
 }
 
 try {
 const r = await fetch('/api/osc', { method: 'POST', body: formData });
 const d = await r.json();
 $('#oscMsg').textContent = d.status === 'ok' ? 'Configuration OSC enregistrée' : 'Erreur: ' + d.error;
 $('#oscMsg').style.color = d.status === 'ok' ? '#059669' : '#dc2626';
 } catch (err) {
 $('#oscMsg').textContent = 'Erreur de connexion';
 $('#oscMsg').style.color = '#dc2626';
 }
 });
}

/* Variable globale pour stocker les définitions de composants (compatibilité avec ancien code) */
/* DÉPRÉCIÉ: Utiliser ComponentDefinitions.cache à la place */
let componentDefinitions = [];

/**
 * Charge les définitions de composants depuis l'API
 * DÉPRÉCIÉ: Utiliser ComponentDefinitions.load() à la place
 * @returns {Promise<Array>} Tableau des définitions de composants
 */
async function loadComponentDefinitions(){
 /* Déléguer à ComponentDefinitions.load() si disponible, sinon ancienne implémentation */
 if(typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.load) {
  const defs = await ComponentDefinitions.load();
  /* Maintenir la compatibilité avec l'ancienne variable globale */
  componentDefinitions = ComponentDefinitions.cache;
  return defs;
 }

 /* Ancienne implémentation (fallback si ComponentDefinitions n'est pas disponible) */
 try {
  const r=await fetch('/api/components/definitions');
  if(!r.ok) {
   console.warn('Erreur chargement définitions composants:', r.status);
   return [];
  }
  componentDefinitions = await r.json();
  console.log('[loadComponentDefinitions] Composants chargés:', componentDefinitions.length);
  return componentDefinitions;
 } catch(err) {
  console.log('Erreur chargement définitions composants:', err);
  return [];
 }
}

/**
 * Trouve une définition de composant par son ID
 * DÉPRÉCIÉ: Utiliser ComponentDefinitions.getById() à la place
 * @param {string} componentId - ID du composant (ex: "potentiometer")
 * @returns {Object|null} Définition du composant ou null
 */
function getComponentDefinition(componentId) {
 /* Déléguer à ComponentDefinitions.getById() si disponible */
 if(typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.getById) {
  return ComponentDefinitions.getById(componentId);
 }

 /* Ancienne implémentation (fallback) */
 if(!componentDefinitions || componentDefinitions.length === 0) return null;
 return componentDefinitions.find(def => def.id === componentId) || null;
}

/**
 * Filtre les composants selon le type de pin
 * DÉPRÉCIÉ: Utiliser ComponentDefinitions.getForPinType() à la place
 * @param {number} pinType - Type de pin (0=ANALOG, 1=DIGITAL, 2=ANALOG_OR_DIGITAL, 3=PWM)
 * @param {boolean} implementedOnly - Si true, retourne uniquement les composants implémentés
 * @returns {Array} Liste des composants compatibles
 */
function getComponentsForPinType(pinType, implementedOnly = true) {
 /* Déléguer à ComponentDefinitions.getForPinType() si disponible */
 if(typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.getForPinType) {
  return ComponentDefinitions.getForPinType(pinType, implementedOnly);
 }

 /* Ancienne implémentation (fallback si ComponentDefinitions n'est pas disponible) */
 if(!componentDefinitions || componentDefinitions.length === 0) {
  console.log('[getComponentsForPinType] Aucune définition disponible');
  return [];
 }
 
 const filtered = componentDefinitions.filter(def => {
  if(implementedOnly && !def.implemented) return false;
  const matchesPrimary = (() => {
   switch(pinType) {
    case 0: return def.pinType === 0 || def.pinType === 2;
    case 1: return def.pinType === 1 || def.pinType === 2;
    case 3: return def.pinType === 3;
    case 4: return def.pinType === 4;
    case 5: return def.pinType === 5;
    default: return false;
   }
  })();
  if(matchesPrimary) return true;
  if(def.altPinType !== undefined && def.altPinType !== null && def.altPinType >= 0) {
   return def.altPinType === pinType;
  }
  if(pinType === 5 && def.pinType === 4 && def.id === 'lis3dh') return true;
  return false;
 });
 
 console.log(`[getComponentsForPinType] pinType=${pinType}, implementedOnly=${implementedOnly}, trouvé ${filtered.length} composants:`, filtered.map(d => `${d.id} (pinType=${d.pinType}, family=${d.family})`));
 return filtered;
}

async function loadCaps(){
 for (let attempt = 1; attempt <= 3; attempt++) {
  try {
   const r = await fetch('/api/pins/caps');
   if (!r.ok) {
    console.warn(`[loadCaps] HTTP ${r.status} (tentative ${attempt}/3)`);
   } else {
    const text = await r.text();
    if (text && text.trim()) {
     caps = JSON.parse(text);
     return caps;
    }
    console.warn(`[loadCaps] Réponse vide (tentative ${attempt}/3)`);
   }
  } catch (e) {
   console.warn(`[loadCaps] Erreur (tentative ${attempt}/3):`, e);
  }
  await new Promise(resolve => setTimeout(resolve, 200 * attempt));
 }
 /* Fallback pour éviter de casser l'initialisation UI */
 if (typeof caps === 'undefined' || !caps) {
  caps = { board: '', pins: [], bus: {} };
 }
 return caps;
}

/* Variable pour stocker les GPIOs utilisés (cache) */
let cachedUsedGpios = new Set();

/**
 * Charge les GPIOs utilisés depuis l'API backend
 * C'est la source de vérité unique pour le grisage
 * @returns {Promise<Set<number>>} Set des GPIOs utilisés
 */
async function loadUsedGpiosFromBackend() {
 try {
  const r = await fetch('/api/components/used-gpios');
  if(!r.ok) {
   console.warn('Erreur chargement GPIOs utilisés:', r.status);
   return new Set();
  }
  const data = await r.json();
  cachedUsedGpios = new Set(data.gpios || []);
  console.log('[loadUsedGpiosFromBackend] GPIOs utilisés:', Array.from(cachedUsedGpios));
  return cachedUsedGpios;
 } catch(err) {
  console.warn('Erreur chargement GPIOs utilisés:', err);
  return new Set();
 }
}

/**
 * Retourne les GPIOs utilisés (depuis le cache)
 * @returns {Set<number>} Set des GPIOs utilisés
 */
function getCachedUsedGpios() {
 return cachedUsedGpios;
}

/**
 * Ajoute les GPIOs d'un composant en cours d'édition au cache
 * Utilise les additionalPins du backend pour les composants complexes
 * @param {Object} editingConfig - Configuration du composant en cours d'édition
 * @returns {Set<number>} Set mis à jour des GPIOs utilisés
 */
function getUsedGpiosWithEditing(editingConfig) {
 const gpios = new Set(cachedUsedGpios);
 
 if(!editingConfig) return gpios;
 
 /* Ajouter le GPIO principal */
 if(editingConfig.gpio !== undefined && editingConfig.gpio !== null) {
  gpios.add(parseInt(editingConfig.gpio));
 }
 
 /* Pour les composants complexes, utiliser les additionalPins du backend */
 if(editingConfig.role && typeof getComponentDefinition === 'function') {
  const migratedRole = typeof migrateRole === 'function' ? migrateRole(editingConfig.role) : editingConfig.role;
  const def = getComponentDefinition(migratedRole);

  if(def && def.additionalPins && def.additionalPinCount > 0) {
   /* Parcourir les pins additionnelles définies par le backend */
   def.additionalPins.forEach(pinDef => {
    const pinId = pinDef.id; /* ex: "s0", "s1", "en" */
    const gpio = editingConfig[pinId];
    if(gpio !== undefined && gpio !== null && gpio !== 255) {
     gpios.add(parseInt(gpio));
    }
   });
  }
 }
 
 return gpios;
}

/**
 * Convertit un nom d'affichage vers l'ID backend correspondant
 * Utilise uniquement les définitions du backend (pas de hardcoding)
 * @param {string} role - Nom d'affichage ou ID de rôle
 * @returns {string} ID de rôle normalisé
 */
function migrateRole(role){
 if(!role) return role;
 
 /* Si c'est déjà un ID valide (format backend), retourner tel quel */
 if(/^[a-z0-9_-]+$/.test(role)) return role;

 /* Chercher dans les définitions du backend par displayName */
 if(typeof componentDefinitions !== 'undefined' && componentDefinitions.length > 0) {
  const def = componentDefinitions.find(d => d.displayName === role);
  if(def) return def.id;
 }

 /* Si rien n'est trouvé, retourner tel quel */
 return role;
}

async function loadConfiguredPins(){
 try {
 /* Charger les pins simples depuis /api/pins/list */
 const r=await fetch('/api/pins/list');
 if(!r.ok) {
  console.warn('[loadConfiguredPins] Erreur API:', r.status);
  return;
 }
 const d=await r.json();
 if(d && d.pins && Array.isArray(d.pins)) {
  if(typeof pcfg === 'undefined') {
   console.error('[loadConfiguredPins] pcfg non défini');
   return;
  }
  d.pins.forEach(pinData => {
   if(pinData && pinData.pinLabel && pinData.role) {
    pinData.role = typeof migrateRole === 'function' ? migrateRole(pinData.role) : pinData.role;
    if(pinData.pinLabel === 'SPI' || pinData.pinLabel === 'I2C') {
     console.log('[loadConfiguredPins] Config bus', pinData.pinLabel, ': role=' + pinData.role, 'csGpio=' + pinData.csGpio, 'range=' + pinData.range, 'dataRate=' + pinData.dataRate, 'filterIntensity=' + pinData.filterIntensity);
    }
    pcfg[pinData.pinLabel] = pinData;
   }
  });
 }

 /* /api/pins/list inclut déjà les composants complexes depuis MuxManager avec additionalPins */

 updatePinsList();
 updateBusVisuals();
 } catch(err) {
 console.log('Erreur chargement pins:', err);
 }
}

async function saveAll(){
 const msg=$('#saveAllMsg');
 if(!msg) {
  console.error('[saveAll] Élément saveAllMsg non trouvé');
  return;
 }
 /* --- 1. VALIDATION : ANTI-DOUBLONS --- */
  const duplicateName = findDuplicateComponentName();
  if (duplicateName) {
    msg.textContent = `Erreur: Le nom "${duplicateName}" est utilisé plusieurs fois.`;
    msg.style.color = '#ef4444';
    // On arrête tout ici pour éviter une config invalide sur l'ESP32
    return; 
  }
 msg.textContent='Enregistrement...';
 try{
 if(typeof pcfg === 'undefined' || !pcfg) {
  msg.textContent='Erreur: configuration non disponible';
  msg.style.color='#ef4444';
  return;
 }
 
 /* Relire la configuration de la pin actuellement sélectionnée depuis le formulaire */
 if(typeof cur !== 'undefined' && cur && typeof readCfg === 'function') {
   /* Passer le rôle depuis funcSelect pour éviter qu'il soit vide */
   const funcRole = $('#funcSelect')?.value || '';
   const currentCfg = readCfg(funcRole || null);
   if(currentCfg && currentCfg.role && typeof isBusRole === 'function' && !isBusRole(currentCfg.role)) {
     pcfg[cur] = currentCfg;
     console.log('[saveAll] pcfg[' + cur + '] mis à jour avec role:', currentCfg.role);
   } else if(!currentCfg || !currentCfg.role) {
     console.warn('[saveAll] readCfg role vide pour', cur, '- funcSelect:', funcRole, 'currentCfg:', currentCfg);
   }
 }
 
/* Sauvegarder tous les composants séquentiellement (évite saturation NVS ESP32) */
 const pinLabels = Object.keys(pcfg);
 const validPins = pinLabels.filter(l => pcfg[l] && pcfg[l].role);
 let savedCount = 0;
 for (const lbl of pinLabels) {
 let c=pcfg[lbl];
 if(!c||!c.role) continue;
 savedCount++;
 msg.textContent='Enregistrement ' + savedCount + '/' + validPins.length + ' (' + lbl + ')...';
 const savePin = async () => {
 /* Pour la pin actuellement affichée, toujours reprendre le formulaire (évite valeurs périmées) */
 if(typeof cur !== 'undefined' && lbl === cur && typeof readCfg === 'function') {
  const freshRole = $('#funcSelect')?.value || '';
  const fresh = readCfg(freshRole || null);
  if(fresh && fresh.role && (typeof isBusRole !== 'function' || !isBusRole(fresh.role))) c = fresh;
 }

 const role = migrateRole(c.role);
 const def = typeof getComponentDefinition === 'function' ? getComponentDefinition(role) : null;

 /* Composants I2C (LIS3DH, MPR121) : envoyer en JSON direct */
 if(role === 'lis3dh' || role === 'mpr121') {
  console.log('[saveAll]', role, 'config:', JSON.stringify(c).substring(0, 200));
  const fullCfg = Object.assign({pinLabel: lbl, role: c.role}, c);
  if(lbl === 'SPI') fullCfg.busInterface = '1';
  else if(lbl === 'I2C') fullCfg.busInterface = '0';
  delete fullCfg.additionalPins;
  const jsonStr = JSON.stringify(fullCfg);
  console.log('[saveAll]', role, 'JSON complet (' + jsonStr.length + ' chars)');
  const resp = await fetch('/api/pins/set',{method:'POST',headers:{'Content-Type':'application/json'},body:jsonStr});
  if (resp.status === 413) {
    const d = await resp.json().catch(() => ({}));
    throw new Error(d.message || 'Config trop grande pour NVS (max 1900 octets). Réduisez les options.');
  }
  console.log('[saveAll]', role, 'réponse:', resp.status);
  return resp;
 }

 console.log('[saveAll] Traitement pin:', lbl, 'c:', c);
 console.log('[saveAll] c.additionalPins:', c.additionalPins);
 
 /* Détecter composant complexe depuis la définition (plus fiable que vérifier sig) */
 const hasAdditionalPins = def && def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0 
   && c.additionalPins && typeof c.additionalPins === 'object' && Object.keys(c.additionalPins).length > 0;
 
 console.log('[saveAll] hasAdditionalPins:', hasAdditionalPins, 'role:', role, 'def trouvée:', !!def, 'def.additionalPins count:', def ? (def.additionalPins ? def.additionalPins.length : 0) : 0, 'c.additionalPins keys:', c.additionalPins ? Object.keys(c.additionalPins) : []);
 
 /* Si composant simple, vérifier et supprimer les complexes sur cette pin (chercher dans pcfg) */
 /* NOTE: Les composants complexes sont détectés par leur définition, pas par la présence de sig */
 if(!hasAdditionalPins && caps && caps.pins) {
   const currentPin = caps.pins.find(p => p.label === lbl);
   if(currentPin) {
     const mainPinGpio = parseInt(currentPin.gpio);
     /* Chercher composant complexe dans pcfg par pinLabel (la pin principale du composant complexe) */
     const existingComplexLabel = Object.keys(pcfg).find(plbl => {
       const cfg = pcfg[plbl];
       if(!cfg || !cfg.role) return false;
       const cfgRole = migrateRole(cfg.role);
       const cfgDef = typeof getComponentDefinition === 'function' ? getComponentDefinition(cfgRole) : null;
       const cfgHasAdditionalPins = cfgDef && cfgDef.additionalPins && Array.isArray(cfgDef.additionalPins) && cfgDef.additionalPins.length > 0
         && cfg.additionalPins && typeof cfg.additionalPins === 'object' && Object.keys(cfg.additionalPins).length > 0;
       if(!cfgHasAdditionalPins) return false;
       /* Vérifier si cette pin est utilisée comme pin principale du composant complexe */
       const complexPin = caps.pins.find(p => p.label === plbl);
       return complexPin && parseInt(complexPin.gpio) === mainPinGpio;
     });
     if(existingComplexLabel) {
       const existingComplex = pcfg[existingComplexLabel];
       console.log(`[saveAll] Suppression du composant complexe sur pin principale ${mainPinGpio} (remplacé par ${lbl})`);
       /* Supprimer via /api/pins/delete (unifié) */
       try {
         const formData = new URLSearchParams();
         formData.append('pin', existingComplexLabel);
         await fetch('/api/pins/delete', {method: 'POST', body: formData});
         console.log(`[saveAll] Composant complexe supprimé via /api/pins/delete`);
       } catch(e) {
         console.error('[saveAll] Erreur suppression composant complexe:', e);
       }
     }
   }
 }

 const p=new URLSearchParams();
 p.set('pinLabel',lbl);
 p.set('role',c.role);
 if(c.name) p.set('name', c.name);
 /* Envoyer rtpMidiEnabled (ou rtpEnabled pour compatibilité) */
 if(c.rtpMidiEnabled) p.set('rtpMidiEnabled','true');
 else if(c.rtpEnabled) p.set('rtpEnabled','true'); /* Compatibilité ancien format */
 /* Envoyer midiMessageType (ou rtpType pour compatibilité) */
 if(c.midiMessageType) p.set('midiMessageType',c.midiMessageType);
 else if(c.rtpType) p.set('rtpType',c.rtpType); /* Compatibilité ancien format */

 /* Envoyer dynamiquement tous les paramètres MIDI (nouveaux noms midi* puis anciens rtp* pour compatibilité) */
 Object.keys(c).forEach(key => {
  /* Nouveaux noms (midi*) */
  /* Pour les paramètres RANGE (midiCcRangeMin/Max), toujours envoyer même si valeur par défaut */
  if(key.startsWith('midi') && c[key] !== undefined && c[key] !== null) {
   /* Accepter les chaînes vides et les valeurs "0" pour midiCcRangeMin/Max */
   if(c[key] !== '' || key.endsWith('Min') || key.endsWith('Max')) {
     p.set(key, c[key] || (key.endsWith('Min') ? '0' : key.endsWith('Max') ? '127' : ''));
   }
  }
  /* Paramètres MIDI par axe (X_/Y_/Z_: midiCc, midiChannel, etc.) - envoyer même "0" */
  else if((key.startsWith('X_') || key.startsWith('Y_') || key.startsWith('Z_')) && c[key] !== undefined && c[key] !== null) {
   if(c[key] !== '' || key.includes('midiCc') || key.includes('midiChannel')) p.set(key, c[key]);
  }
  /* Anciens noms (rtp*) sauf rtpEnabled et rtpType (déjà gérés ci-dessus) */
  else if(key.startsWith('rtp') && key !== 'rtpEnabled' && key !== 'rtpType' && key !== 'rtpMidiEnabled' && c[key] !== undefined && c[key] !== null && c[key] !== '') {
   p.set(key, c[key]); /* Compatibilité ancien format */
  }
 });

 /* Joystick : forcer envoi X_midiCc / Y_midiCc depuis le formulaire si absents de c */
 if(role === 'joystick') {
  const xCc = c.X_midiCc !== undefined && c.X_midiCc !== null ? c.X_midiCc : ($('#X_midiCc') && $('#X_midiCc').value !== undefined ? $('#X_midiCc').value : '7');
  const yCc = c.Y_midiCc !== undefined && c.Y_midiCc !== null ? c.Y_midiCc : ($('#Y_midiCc') && $('#Y_midiCc').value !== undefined ? $('#Y_midiCc').value : '7');
  p.set('X_midiCc', xCc);
  p.set('Y_midiCc', yCc);
 }

 /* Envoyer dynamiquement tous les formFields depuis la définition */
 if(def && def.formFields && Array.isArray(def.formFields)) {
  def.formFields.forEach(field => {
   if(field.id && !field.id.startsWith('_')) {
    const value = c[field.id];
    if(value !== undefined && value !== null && value !== '') {
     if(field.type === 3) { /* CHECKBOX */
      if(value === true || value === 'true') {
       p.set(field.id, 'true');
      }
     } else if(field.type === 4) { /* RANGE */
      if(c[field.id + 'Min'] !== undefined && c[field.id + 'Min'] !== null && c[field.id + 'Min'] !== '') {
       p.set(field.id + 'Min', c[field.id + 'Min']);
      }
      if(c[field.id + 'Max'] !== undefined && c[field.id + 'Max'] !== null && c[field.id + 'Max'] !== '') {
       p.set(field.id + 'Max', c[field.id + 'Max']);
      }
     } else {
      p.set(field.id, value);
     }
    }
   }
  });
 }
 
 /* Envoyer additionalPins si présent (générique basé sur def.additionalPins) */
 if(hasAdditionalPins && c.additionalPins && def && def.additionalPins && Array.isArray(def.additionalPins)) {
   console.log('[saveAll] Envoi additionalPins, c.additionalPins:', c.additionalPins);
   /* Envoyer dynamiquement tous les additionalPins depuis la définition */
   def.additionalPins.forEach(pinDef => {
     if(pinDef && pinDef.id) {
       const value = c.additionalPins[pinDef.id];
       console.log('[saveAll] Vérification pinDef.id:', pinDef.id, 'value:', value, 'optional:', pinDef.optional);
       
       if(value !== undefined && value !== null) {
         /* Toujours envoyer les pins requises, même si valeur est 255 */
         /* Pour les pins optionnelles, ne pas envoyer si valeur est 255 */
         if(value !== 255 || !pinDef.optional) {
           p.set(pinDef.id, value);
           console.log('[saveAll] additionalPin envoyé:', pinDef.id, '=', value);
         } else {
           console.log('[saveAll] additionalPin ignoré (255 et optionnel):', pinDef.id);
         }
       } else if(!pinDef.optional) {
         /* Pin requise absente - utiliser la valeur par défaut ou 255 */
         const defaultValue = (pinDef.defaultValue !== undefined) ? pinDef.defaultValue : 255;
         p.set(pinDef.id, defaultValue);
         console.log('[saveAll] additionalPin requise absente, utilisation defaultValue:', pinDef.id, '=', defaultValue);
       } else {
         console.warn('[saveAll] ERREUR: Pin requise absente:', pinDef.id, 'value:', value, 'defaultValue:', pinDef.defaultValue);
       }
     }
  });
  /* Note: complexId supprimé - plus besoin d'envoyer un ID explicite */
} else if(def && def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0) {
  /* Seulement avertir si la définition indique qu'il devrait y avoir additionalPins mais qu'elles manquent */
  console.warn('[saveAll] ERREUR: Composant devrait avoir additionalPins mais elles sont absentes. def:', def.id, 'def.additionalPins:', def.additionalPins, 'c.additionalPins:', c.additionalPins);
}
/* Sinon, c'est normal - composant simple sans additionalPins */

 /* Champ Mapping Script */
 if(c.mappingScript) p.set('mappingScript', c.mappingScript);
 /* Mode MIDI (RTP vs Mapping) */
 if(c.midiMode) p.set('midiMode', c.midiMode);

 /* Champs OSC et Debug (communs à tous) — toujours envoyer les booléens */
 const oscOn = (c.oscEnabled === true || c.oscEnabled === 'true');
 p.set('oscEnabled', oscOn ? 'true' : 'false');
 if(c.oscAddress) p.set('oscAddress',c.oscAddress);
 if(c.oscFormat) p.set('oscFormat',c.oscFormat);
 const dbgOn = (c.dbgEnabled === true || c.dbgEnabled === 'true');
 p.set('dbgEnabled', dbgOn ? 'true' : 'false');
 if(c.dbgHeader) p.set('dbgHeader',c.dbgHeader);
 if(lbl === 'SPI' || lbl === 'I2C') {
  console.log('[saveAll] POST body pour', lbl, ':', p.toString());
 }
 const r = await fetch('/api/pins/set',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:p.toString()});
 if (r.status === 413) { const d = await r.json().catch(() => ({})); throw new Error(d.message || 'Config trop grande pour NVS (max 1900 octets).'); }
 return r;
 };
 await savePin();
 await new Promise(r => setTimeout(r, 80));
 }

 /* Attendre que le backend traite le rechargement */
 await new Promise(r => setTimeout(r, 300));
 
 const listRes=await fetch('/api/pins/list');
 if(!listRes.ok){
  throw new Error('Erreur lors de la récupération de la liste des pins: '+listRes.status);
 }
 const text=await listRes.text();
 if(!text || text.trim().length===0){
  console.warn('Réponse vide de /api/pins/list');
  var listData={pins:[]};
 }else{
  try{
   var listData=JSON.parse(text);
  }catch(e){
   console.error('Erreur parsing JSON /api/pins/list:',e,'Réponse:',text);
   throw e;
  }
 }
 const serverPins=new Set();
 if(listData.pins && Array.isArray(listData.pins)){
 listData.pins.forEach(p=>{
 if(p.pinLabel) serverPins.add(p.pinLabel);
 if(p.pinLabel === 'SPI' || p.pinLabel === 'I2C') {
  console.log('[saveAll] Vérification post-save', p.pinLabel, ': csGpio=' + p.csGpio, 'range=' + p.range, 'dataRate=' + p.dataRate, 'filterIntensity=' + p.filterIntensity);
 }
 });
 }
 /* Vérifier que les pins bus ont bien été sauvegardées */
 if(typeof pcfg !== 'undefined') {
  ['SPI','I2C'].forEach(bus => {
   if(pcfg[bus] && pcfg[bus].role && !serverPins.has(bus)) {
    console.error('[saveAll] ERREUR: pin', bus, 'configurée localement mais ABSENTE de la réponse backend !');
   }
  });
 }
 
 const localPins=new Set(Object.keys(pcfg));
 const toDelete=Array.from(serverPins).filter(p=>!localPins.has(p));
 for (const pinLabel of toDelete) {
  const p=new URLSearchParams();
  p.set('pin',pinLabel);
  await fetch('/api/pins/delete',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:p.toString()});
  await new Promise(r => setTimeout(r, 80));
 }
 if (toDelete.length > 0) await new Promise(r => setTimeout(r, 200));
 /* Rafraîchir pcfg et la liste des pins depuis le serveur (évite rechargement manuel) */
 await loadConfiguredPins();

/* Sauvegarder les interfaces MIDI globales */
try {
  /* Sauvegarder RTP-MIDI */
  const rtpMidiElement = $('#rtpMidiEnabled');
  if (rtpMidiElement && rtpMidiElement.type === 'checkbox' && typeof rtpMidiElement.checked !== 'undefined') {
    const rtpFormData = new URLSearchParams();
    rtpFormData.append('enable', rtpMidiElement.checked ? 'true' : 'false');
    const bodyString = rtpFormData.toString();
    if (bodyString && bodyString.includes('enable=')) {
      try {
        const rtpResponse = await fetch('/api/rtp/enable', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: bodyString});
        if (!rtpResponse.ok) {
          const errorText = await rtpResponse.text();
          console.warn('[saveAll] Erreur RTP-MIDI:', rtpResponse.status, errorText);
        }
      } catch(e) {
        console.warn('[saveAll] Erreur lors de la sauvegarde RTP-MIDI:', e);
      }
    }
  }
  /* USB-MIDI : compile-time uniquement, pas via saveAll */
} catch(e) {
  console.error('Erreur sauvegarde interfaces MIDI:', e);
}

 /* Rafraîchir le cache des GPIOs utilisés depuis le backend */
 await loadUsedGpiosFromBackend();
 updateBusVisuals();
 
 msg.textContent='Toutes les configurations enregistrées';
 msg.style.color='#10b981';
 }catch(e){
 msg.textContent = e && e.message ? e.message : 'Erreur lors de l\'enregistrement';
 msg.style.color='#ef4444';
 console.error('Erreur saveAll:',e);
 }
}
