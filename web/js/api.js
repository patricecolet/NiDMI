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

// Variable globale pour stocker les définitions de composants (compatibilité avec ancien code)
// DÉPRÉCIÉ: Utiliser ComponentDefinitions.cache à la place
let componentDefinitions = [];

/**
 * Charge les définitions de composants depuis l'API
 * DÉPRÉCIÉ: Utiliser ComponentDefinitions.load() à la place
 * @returns {Promise<Array>} Tableau des définitions de composants
 */
async function loadComponentDefinitions(){
 // Déléguer à ComponentDefinitions.load() si disponible, sinon ancienne implémentation
 if(typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.load) {
  const defs = await ComponentDefinitions.load();
  // Maintenir la compatibilité avec l'ancienne variable globale
  componentDefinitions = ComponentDefinitions.cache;
  return defs;
 }
 
 // Ancienne implémentation (fallback si ComponentDefinitions n'est pas disponible)
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
 // Déléguer à ComponentDefinitions.getById() si disponible
 if(typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.getById) {
  return ComponentDefinitions.getById(componentId);
 }
 
 // Ancienne implémentation (fallback)
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
 // Déléguer à ComponentDefinitions.getForPinType() si disponible
 if(typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.getForPinType) {
  return ComponentDefinitions.getForPinType(pinType, implementedOnly);
 }
 
 // Ancienne implémentation (fallback si ComponentDefinitions n'est pas disponible)
 if(!componentDefinitions || componentDefinitions.length === 0) {
  console.log('[getComponentsForPinType] Aucune définition disponible');
  return [];
 }
 
 const filtered = componentDefinitions.filter(def => {
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
 
 console.log(`[getComponentsForPinType] pinType=${pinType}, implementedOnly=${implementedOnly}, trouvé ${filtered.length} composants:`, filtered.map(d => `${d.id} (pinType=${d.pinType}, family=${d.family})`));
 return filtered;
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
  const migratedRole = typeof migrateRole === 'function' ? migrateRole(editingConfig.role) : editingConfig.role;
  const def = getComponentDefinition(migratedRole);
  
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
 * Convertit un nom d'affichage vers l'ID backend correspondant
 * Utilise uniquement les définitions du backend (pas de hardcoding)
 * @param {string} role - Nom d'affichage ou ID de rôle
 * @returns {string} ID de rôle normalisé
 */
function migrateRole(role){
 if(!role) return role;
 
 // Si c'est déjà un ID valide (format backend), retourner tel quel
 if(/^[a-z0-9_-]+$/.test(role)) return role;
 
 // Chercher dans les définitions du backend par displayName
 if(typeof componentDefinitions !== 'undefined' && componentDefinitions.length > 0) {
  const def = componentDefinitions.find(d => d.displayName === role);
  if(def) return def.id;
 }
 
 // Si rien n'est trouvé, retourner tel quel
 return role;
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
 
 /* Nettoyer les conflits : supprimer pcfg si un composant complexe utilise cette pin comme SIG */
 if(caps && caps.pins && typeof ComplexComponents !== 'undefined' && ComplexComponents.list) {
   ComplexComponents.list.forEach(complexComp => {
     if(complexComp.sig !== undefined) {
       const sigPin = caps.pins.find(p => p.gpio === parseInt(complexComp.sig));
       if(sigPin && sigPin.label && pcfg[sigPin.label]) {
         console.log(`[loadConfiguredPins] Suppression de pcfg[${sigPin.label}] (conflit avec composant complexe SIG ${complexComp.sig})`);
         delete pcfg[sigPin.label];
       }
     }
   });
 }
 
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
 /* Sauvegarder le composant complexe en cours d'édition s'il y en a un */
 const funcSelectValue = $('#funcSelect')?.value || '';
 /* Migrer le rôle si nécessaire (comme dans ComplexComponents.saveFromPin) */
 const migratedRoleValue = typeof migrateRole === 'function' ? migrateRole(funcSelectValue) : funcSelectValue;
 /* Si composant complexe sélectionné, sauvegarder via ComplexComponents.saveFromPin */
 const funcDef = typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.getById
   ? ComponentDefinitions.getById(migratedRoleValue)
   : null;
 if(funcDef && funcDef.isComplex) {
   if(typeof ComplexComponents === 'undefined' || !ComplexComponents.saveFromPin) {
     console.warn('[saveAll] ComplexComponents.saveFromPin non disponible');
   } else {
     /* Appeler saveFromPin() qui gère toute la validation interne */
     await ComplexComponents.saveFromPin();
   }
 }
 
 const ps=Object.keys(pcfg).map(async lbl=>{
 const c=pcfg[lbl];
 if(!c||!c.role) return;
 /* Ne pas sauvegarder les composants complexes via cette API, ils sont gérés via ComplexComponents */
 const role = migrateRole(c.role);
 const def = typeof getComponentDefinition === 'function' ? getComponentDefinition(role) : null;
 if(def && def.isComplex) return;
 
 /* Vérifier s'il y a un composant complexe qui utilise cette pin comme SIG et le supprimer */
 /* Un composant simple remplace toujours le composant complexe sur cette pin */
 if(caps && caps.pins) {
   const currentPin = caps.pins.find(p => p.label === lbl);
   if(currentPin) {
     const existingComplex = typeof ComplexComponents !== 'undefined' && ComplexComponents.list
       ? ComplexComponents.list.find(m => m.sig === parseInt(currentPin.gpio))
       : null;
     if(existingComplex) {
       console.log(`[saveAll] Suppression du composant complexe ${existingComplex.id} sur pin SIG ${existingComplex.sig} (remplacé par ${lbl})`);
       if(typeof ComplexComponents !== 'undefined' && ComplexComponents.delete) {
         await ComplexComponents.delete(existingComplex.id, null, null);
       }
     }
   }
 }

 const p=new URLSearchParams();
 p.set('pinLabel',lbl);
 p.set('role',c.role);
 if(c.rtpEnabled) p.set('rtpEnabled','true');
 if(c.rtpType) p.set('rtpType',c.rtpType);
 
 // Envoyer dynamiquement tous les paramètres MIDI (tous les champs qui commencent par 'rtp' sauf rtpEnabled et rtpType)
 Object.keys(c).forEach(key => {
  if(key.startsWith('rtp') && key !== 'rtpEnabled' && key !== 'rtpType' && c[key] !== undefined && c[key] !== null && c[key] !== '') {
   p.set(key, c[key]);
  }
 });
 
 // Envoyer dynamiquement tous les formFields depuis la définition
 if(def && def.formFields && Array.isArray(def.formFields)) {
  def.formFields.forEach(field => {
   if(field.id && !field.id.startsWith('_')) {
    const value = c[field.id];
    if(value !== undefined && value !== null && value !== '') {
     if(field.type === 3) { // CHECKBOX
      if(value === true || value === 'true') {
       p.set(field.id, 'true');
      }
     } else if(field.type === 4) { // RANGE
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
 
 // Envoyer les additionalPins si composant complexe (mais ici c'est déjà filtré, donc ne devrait pas arriver)
 // Les composants complexes utilisent /api/mux/add (nom d'endpoint historique, mais générique)
 
 // Champs OSC et Debug (communs à tous)
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
