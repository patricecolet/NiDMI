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
 if($('#oscTarget')) $('#oscTarget').value=d.target||'sta';
 if($('#oscPort')) $('#oscPort').value=d.port||8000;
 if($('#oscIp')) $('#oscIp').value=d.ip||'';
if($('#oscBroadcast')) $('#oscBroadcast').checked=!!d.broadcast;


const oscTarget = $('#oscTarget');
 const oscIpRow = $('#oscIpRow');
 if (oscTarget && oscIpRow) {
 if (oscTarget.value === 'ip') {
 oscIpRow.style.display = 'block';
 } else {
 oscIpRow.style.display = 'none';
 }
 }
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
    
    /* Charger l'état USB MIDI (statut uniquement, pas de checkbox) */
    const usbRes = await fetch('/api/usbmidi/status');
    if(usbRes.ok) {
      const usbData = await usbRes.json();
      const statusEl = document.getElementById('usbMidiStatus');
      if(statusEl) {
        let statusText = '';
        if(usbData.supported) {
          if(usbData.connected) {
            statusText = '✅ Connecté';
          } else if(usbData.enabled) {
            statusText = '⚠️ Initialisé (non connecté)';
          } else {
            statusText = '❌ Non initialisé';
          }
        } else {
          statusText = '❌ Non supporté';
        }
        statusEl.textContent = statusText;
      }
    }
  } catch(err) {
    console.log('Erreur chargement interfaces MIDI:', err);
  }
}

function initForms(){
 const oscTarget = $('#oscTarget');
 const oscIpRow = $('#oscIpRow');
 const oscBroadcast = $('#oscBroadcast');
 
 function updateOscForm() {
 const target = oscTarget.value;
 if (target === 'ip') {
 oscIpRow.style.display = 'block';
 oscBroadcast.checked = false;
 } else {
 oscIpRow.style.display = 'none';
 oscBroadcast.checked = true;
 }
 }
 
 if (oscTarget) {
 oscTarget.addEventListener('change', updateOscForm);
 updateOscForm();
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
 
 $('#osc').addEventListener('submit', async (e) => {
 e.preventDefault();
 const formData = new FormData();
 const target = $('#oscTarget').value;
 formData.append('target', target);
 formData.append('port', $('#oscPort').value);
 
 if (target === 'ip' && $('#oscIp').value) {
 formData.append('ip', $('#oscIp').value);
 }
 
 if ($('#oscBroadcast')) {
 formData.append('broadcast', $('#oscBroadcast').checked ? 'true' : 'false');
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
 
 /* Fallback minimal: pas de logique métier dupliquée ici */
 console.warn('[getComponentsForPinType] ComponentDefinitions.getForPinType indisponible, retour tableau vide (fallback)');
 return [];
}

/* Gestion des capacités GPIO et des GPIOs utilisés déplacée dans web/js/api-gpios.js */

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

/* Gestion des pins configurées et de saveAll déplacée dans web/js/api-pins.js */
