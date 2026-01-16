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

// Variable globale pour stocker les définitions de composants
let componentDefinitions = [];

/**
 * Charge les définitions de composants depuis l'API
 * @returns {Promise<Array>} Tableau des définitions de composants
 */
async function loadComponentDefinitions(){
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
 * @param {string} componentId - ID du composant (ex: "potentiometer")
 * @returns {Object|null} Définition du composant ou null
 */
function getComponentDefinition(componentId) {
 if(!componentDefinitions || componentDefinitions.length === 0) return null;
 return componentDefinitions.find(def => def.id === componentId) || null;
}

/**
 * Filtre les composants selon le type de pin
 * @param {number} pinType - Type de pin (0=ANALOG, 1=DIGITAL, 2=ANALOG_OR_DIGITAL, 3=PWM)
 * @param {boolean} implementedOnly - Si true, retourne uniquement les composants implémentés
 * @returns {Array} Liste des composants compatibles
 */
function getComponentsForPinType(pinType, implementedOnly = true) {
 if(!componentDefinitions || componentDefinitions.length === 0) return [];
 
 return componentDefinitions.filter(def => {
  // Filtrer par implémenté si demandé
  if(implementedOnly && !def.implemented) return false;
  
  // Vérifier la compatibilité du type de pin
  switch(pinType) {
   case 0: // PIN_ANALOG
    return def.pinType === 0 || def.pinType === 2; // ANALOG ou ANALOG_OR_DIGITAL
   case 1: // PIN_DIGITAL
    return def.pinType === 1 || def.pinType === 2; // DIGITAL ou ANALOG_OR_DIGITAL
   case 3: // PIN_PWM
    return def.pinType === 3; // PWM uniquement
   default:
    return false;
  }
 });
}

async function loadCaps(){
 const r=await fetch('/api/pins/caps');
 caps=await r.json();
}

// Variable pour stocker les GPIOs utilisés (cache)
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
 
 // Ajouter le GPIO principal
 if(editingConfig.gpio !== undefined && editingConfig.gpio !== null) {
  gpios.add(parseInt(editingConfig.gpio));
 }
 
 // Pour les composants complexes, utiliser les additionalPins du backend
 if(editingConfig.role && typeof getComponentDefinition === 'function') {
  const baseRole = editingConfig.role.includes(':') ? editingConfig.role.split(':')[0] : editingConfig.role;
  const def = getComponentDefinition(baseRole);
  
  if(def && def.additionalPins && def.additionalPinCount > 0) {
   // Parcourir les pins additionnelles définies par le backend
   def.additionalPins.forEach(pinDef => {
    const pinId = pinDef.id; // ex: "s0", "s1", "en"
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
 * Migre les anciens noms de rôle vers les IDs backend
 * Utilise les définitions du backend pour la migration
 * @param {string} role - Ancien nom ou ID de rôle
 * @returns {string} ID de rôle normalisé
 */
function migrateRole(role){
 if(!role) return role;
 
 // Si c'est déjà un ID valide (minuscule, pas d'accent), retourner tel quel
 if(/^[a-z0-9:_-]+$/.test(role)) return role;
 
 // Chercher dans les définitions du backend par displayName
 if(typeof componentDefinitions !== 'undefined' && componentDefinitions.length > 0) {
  const def = componentDefinitions.find(d => d.displayName === role);
  if(def) return def.id;
 }
 
 // Fallback legacy pour compatibilité
 const legacy = {
  'Potentiomètre':'potentiometer',
  'Bouton':'button',
  'LED':'led',
  'Multiplexeur':'mux:HC4067',
  'I2C':'i2c',
  'SPI':'spi',
  'UART':'uart'
 };
 return legacy[role] || role;
}

async function loadConfiguredPins(){
 try {
 const r=await fetch('/api/pins/list');
 const d=await r.json();
 if(d.pins && Array.isArray(d.pins)) {
 d.pins.forEach(pinData => {
 if(pinData.pinLabel && pinData.role) {
  pinData.role=migrateRole(pinData.role);
  pcfg[pinData.pinLabel] = pinData;
 }
 });
 updatePinsList();
 updateBusVisuals();
 }
 } catch(err) {
 console.log('Erreur chargement pins:', err);
 }
}

async function saveAll(){
 const msg=$('#saveAllMsg');
 msg.textContent='Enregistrement...';
 try{
 // Sauvegarder le MUX en cours d'édition s'il y en a un
 if(typeof saveMuxFromPin === 'function' && $('#funcSelect') && $('#funcSelect').value && $('#funcSelect').value.startsWith('mux:') && $('#muxSig') && $('#muxSig').value){
  await saveMuxFromPin();
 }
 
 const ps=Object.keys(pcfg).map(async lbl=>{
 const c=pcfg[lbl];
 if(!c||!c.role) return;
 // Ne pas sauvegarder les pins MUX via l'API pins (elles sont gérées via l'API mux)
 if(c.role && c.role.startsWith('mux:')) return;
 const p=new URLSearchParams();
 p.set('pinLabel',lbl);
 p.set('role',c.role);
 if(c.rtpEnabled) p.set('rtpEnabled','true');
 if(c.rtpType) p.set('rtpType',c.rtpType);
 if(c.rtpNote) p.set('rtpNote',c.rtpNote);
 if(c.rtpCc) p.set('rtpCc',c.rtpCc);
 if(c.rtpPc) p.set('rtpPc',c.rtpPc);
 if(c.rtpChan) p.set('rtpChan',c.rtpChan);
 if(c.rtpCcOn) p.set('rtpCcOn',c.rtpCcOn);
 if(c.rtpCcOff) p.set('rtpCcOff',c.rtpCcOff);
 if(c.rtpVel) p.set('rtpVel',c.rtpVel);
 if(c.rtpCcMin) p.set('rtpCcMin',c.rtpCcMin);
 if(c.rtpCcMax) p.set('rtpCcMax',c.rtpCcMax);
 if(c.rtpNoteMin) p.set('rtpNoteMin',c.rtpNoteMin);
 if(c.rtpNoteMax) p.set('rtpNoteMax',c.rtpNoteMax);
 if(c.rtpNoteVelFix) p.set('rtpNoteVelFix',c.rtpNoteVelFix);
 if(c.rtpNoteSweepAutoOffDelay) p.set('rtpNoteSweepAutoOffDelay',c.rtpNoteSweepAutoOffDelay);
 if(c.ledMode) p.set('ledMode',c.ledMode);
 if(c.btnMode) p.set('btnMode',c.btnMode);
 if(c.btnPulseTiming) p.set('btnPulseTiming',c.btnPulseTiming);
 if(c.filterIntensity) p.set('filterIntensity',c.filterIntensity);
 if(c.oscEnabled) p.set('oscEnabled','true');
 if(c.oscAddress) p.set('oscAddress',c.oscAddress);
 if(c.oscFormat) p.set('oscFormat',c.oscFormat);
 if(c.dbgEnabled) p.set('dbgEnabled','true');
 if(c.dbgHeader) p.set('dbgHeader',c.dbgHeader);
 return fetch('/api/pins/set',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:p.toString()});
 });
 await Promise.all(ps);
 
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
 });
 }
 
 const localPins=new Set(Object.keys(pcfg));
 const toDelete=Array.from(serverPins).filter(p=>!localPins.has(p));
 const deletePromises=toDelete.map(async pinLabel=>{
 const p=new URLSearchParams();
 p.set('pin',pinLabel);
 return fetch('/api/pins/delete',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:p.toString()});
 });
 await Promise.all(deletePromises);
 
 // Rafraîchir le cache des GPIOs utilisés depuis le backend
 await loadUsedGpiosFromBackend();
 updateBusVisuals();
 
 msg.textContent='Toutes les configurations enregistrées';
 msg.style.color='#10b981';
 }catch(e){
 msg.textContent='Erreur lors de l\'enregistrement';
 msg.style.color='#ef4444';
 console.error('Erreur saveAll:',e);
 }
}
