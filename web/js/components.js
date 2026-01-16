/* Fonctions de gestion des composants et configurations */

/**
 * Affiche la carte de configuration correspondant au rôle sélectionné
 * Utilise les définitions du backend pour déterminer le cardId
 * @param {string} role - ID du rôle (ex: "potentiometer", "hc4067")
 */
function showRoleCards(role, currentCfg = {}){
 // Masquer toutes les cartes en utilisant les cardId du backend
 if(typeof componentDefinitions !== 'undefined' && componentDefinitions) {
  componentDefinitions.forEach(def => {
   if(def.cardId) {
    const card = $('#' + def.cardId);
    if(card) card.style.display = 'none';
   }
  });
 }
 
 if(!role) return;
 
 // Migrer les anciens formats si nécessaire
 const migratedRole = typeof migrateRole === 'function' ? migrateRole(role) : role;
 
 // Trouver la définition du composant
 const def = typeof getComponentDefinition === 'function' ? getComponentDefinition(migratedRole) : null;
 if(def && def.cardId) {
  const card = $('#' + def.cardId);
  if(card) {
   card.style.display = 'block';
   // Générer les champs de formulaire dynamiquement
   if(typeof generateFormFields === 'function' && def.formFields && def.formFields.length > 0) {
    generateFormFields(def, def.cardId, currentCfg);
   }
  }
 }
}

/**
 * Groupe les composants par famille et remplit le menu Famille
 * @param {number} pinType - Type de pin (0=ANALOG, 1=DIGITAL, 3=PWM)
 * @param {Object} pin - Objet pin avec gpio
 */
function populateFamilySelect(pinType, pin) {
 const familySel = $('#familySelect');
 if(!familySel) return;
 
 console.log('[populateFamilySelect] pinType=', pinType, 'componentDefinitions.length=', componentDefinitions?.length);
 
 if(pinType === null || !componentDefinitions || componentDefinitions.length === 0) {
  console.log('[populateFamilySelect] Pas de définitions, menu vide');
  setOptions(familySel, {}, 0);
  return;
 }
 
 // Obtenir les composants compatibles avec le type de pin (implémentés uniquement pour les familles)
 const compatibleComponents = typeof getComponentsForPinType === 'function' 
  ? getComponentsForPinType(pinType, true) 
  : [];
 
 console.log('[populateFamilySelect] compatibleComponents:', compatibleComponents.length);
 
 // Grouper par famille
 const familiesMap = new Map();
 compatibleComponents.forEach(def => {
  // getComponentsForPinType avec implementedOnly=true a déjà filtré, mais on garde pour sécurité
  if(!def.implemented) return;
  
  const familyId = def.family !== undefined ? def.family : 0;
  const familyName = def.familyName || 'Basic';
  
  if(!familiesMap.has(familyId)) {
   familiesMap.set(familyId, {
    id: familyId,
    name: familyName,
    components: []
   });
  }
  
  // Vérifier disponibilité MUX pour les multiplexeurs
  if(def.family === 1 && pinType === 0) { // MULTIPLEXER
   const muxAvailable = pin ? areMuxAddressPinsAvailable(pin.gpio) : false;
   if(muxAvailable) {
    familiesMap.get(familyId).components.push(def);
   }
  } else {
   familiesMap.get(familyId).components.push(def);
  }
 });
 
 // Créer les options pour le menu Famille
 const familyOptions = {};
 Array.from(familiesMap.values()).forEach(fam => {
  if(fam.components.length > 0) {
   familyOptions[fam.id] = fam.name;
   console.log(`[populateFamilySelect] Famille ${fam.id} (${fam.name}): ${fam.components.length} composants`);
  }
 });
 
 console.log('[populateFamilySelect] Options famille:', familyOptions);
 setOptions(familySel, familyOptions, 0);
 
 // Sélectionner automatiquement la première famille si aucune n'est sélectionnée
 if(!familySel.value && Object.keys(familyOptions).length > 0) {
  const firstFamilyId = parseInt(Object.keys(familyOptions)[0]);
  familySel.value = firstFamilyId;
 }
 
 // Déclencher le filtrage du menu Composant
 const selectedFamilyId = familySel.value ? parseInt(familySel.value) : null;
 if(selectedFamilyId !== null) {
  populateComponentSelect(selectedFamilyId, pinType, pin);
 }
}

/**
 * Remplit le menu Composant selon la famille sélectionnée
 * @param {number} familyId - ID de la famille (0=BASIC, 1=MULTIPLEXER, etc.)
 * @param {number} pinType - Type de pin
 * @param {Object} pin - Objet pin avec gpio
 */
function populateComponentSelect(familyId, pinType, pin) {
 const compSel = $('#funcSelect');
 if(!compSel) return;
 
 console.log('[populateComponentSelect] familyId=', familyId, 'pinType=', pinType);
 
 if(pinType === null || !componentDefinitions || componentDefinitions.length === 0) {
  console.log('[populateComponentSelect] Pas de définitions, menu vide');
  setOptions(compSel, {}, 0);
  return;
 }
 
 // Obtenir TOUS les composants compatibles (y compris non implémentés)
 const compatibleComponents = typeof getComponentsForPinType === 'function' 
  ? getComponentsForPinType(pinType, false) 
  : [];
 
 console.log('[populateComponentSelect] compatibleComponents:', compatibleComponents.length);
 
 // Filtrer par famille
 const familyComponents = compatibleComponents.filter(def => {
  const defFamilyId = def.family !== undefined ? def.family : 0;
  return defFamilyId === familyId;
 });
 
 console.log('[populateComponentSelect] familyComponents (familyId=' + familyId + '):', familyComponents.length, familyComponents.map(d => `${d.id} (implemented=${d.implemented})`));
 
 // Créer les options pour le menu Composant
 const options = {};
 familyComponents.forEach(def => {
  // Vérifier disponibilité MUX pour les multiplexeurs
  if(def.family === 1 && pinType === 0) { // MULTIPLEXER
   const muxAvailable = pin ? areMuxAddressPinsAvailable(pin.gpio) : false;
   // Afficher même si non disponible si non implémenté (pour info)
   if(muxAvailable || !def.implemented) {
    options[def.id] = {
     label: def.displayName,
     disabled: !def.implemented || !muxAvailable
    };
   }
  } else {
   options[def.id] = {
    label: def.displayName,
    disabled: !def.implemented
   };
  }
 });
 
 console.log('[populateComponentSelect] Options composant:', Object.keys(options));
 setOptions(compSel, options, 0);
}

function updFunc(lbl){
 const sel=$('#funcSelect');
 const familySel=$('#familySelect');
 if(!sel || !familySel) return;
 
 // Vérifier si les définitions sont chargées, sinon les charger
 if(!componentDefinitions || componentDefinitions.length === 0) {
  // Charger les définitions de manière asynchrone et réessayer
  loadComponentDefinitions().then(() => {
   // Réessayer après chargement
   updFunc(lbl);
  }).catch(err => {
   console.warn('Erreur chargement définitions dans updFunc:', err);
  });
  return;
 }
 
 // Toujours vider le formulaire MUX au début pour éviter les valeurs résiduelles
 if($('#muxSig')) $('#muxSig').value='';
 
 // Utiliser pType() qui utilise les données du backend
 const type = typeof pType === 'function' ? pType(lbl) : 'digital';
 const pin = caps?.pins?.find(p => p.label === lbl);
 
 console.log('[updFunc] Pin:', lbl, 'type:', type, 'pin:', pin);
 
 // Gérer les bus (I2C, SPI, UART) - rôles spéciaux
 if(type === 'i2c' || type === 'spi' || type === 'uart') {
  const def = typeof getComponentDefinition === 'function' ? getComponentDefinition(type) : null;
  const displayName = def ? def.displayName : type.toUpperCase();
  const options = {};
  options[type] = displayName;
  setOptions(sel, options, 0);
  setOptions(familySel, {}, 0);
  showRoleCards(sel.value || '');
  updateRtpForRole(sel.value || '');
  return;
 }
 
 // Convertir le type de pin en pinType numérique pour le filtre
 let pinType = null;
 if(type === 'analog') {
  pinType = 0; // PIN_ANALOG
 } else if(type === 'digital') {
  pinType = 1; // PIN_DIGITAL (toujours, peu importe si PWM ou pas)
 }
 
 console.log('[updFunc] pinType calculé:', pinType);
 
 // Remplir le menu Famille (qui déclenchera le remplissage du menu Composant)
 populateFamilySelect(pinType, pin);
 
 // Si une valeur était déjà sélectionnée, essayer de la restaurer APRÈS que les menus soient remplis
 const currentRole = pcfg[lbl]?.role;
 if(currentRole) {
  // Utiliser setTimeout pour s'assurer que les menus sont remplis
  setTimeout(() => {
   const migratedRole = migrateRole(currentRole);
   const def = typeof getComponentDefinition === 'function' ? getComponentDefinition(migratedRole) : null;
   if(def) {
    // Sélectionner la bonne famille
    const defFamilyId = def.family !== undefined ? def.family : 0;
    if(familySel.value != defFamilyId) {
     familySel.value = defFamilyId;
     populateComponentSelect(defFamilyId, pinType, pin);
     // Attendre que le menu Composant soit rempli avant de sélectionner
     setTimeout(() => {
      if(sel.value != migratedRole) {
       sel.value = migratedRole;
       const currentCfg = pcfg && pcfg[lbl] ? pcfg[lbl] : {};
       showRoleCards(migratedRole, currentCfg);
       updateRtpForRole(migratedRole);
       updateRtpParamsVisibility();
       if(migratedRole === 'hc4067' || migratedRole === 'hc4051') {
        initMuxFormForPin(lbl);
       }
      }
     }, 0);
    } else {
     // Famille déjà sélectionnée, juste sélectionner le composant
     setTimeout(() => {
      if(sel.value != migratedRole) {
       sel.value = migratedRole;
       const currentCfg = pcfg && pcfg[lbl] ? pcfg[lbl] : {};
       showRoleCards(migratedRole, currentCfg);
       updateRtpForRole(migratedRole);
       updateRtpParamsVisibility();
       if(migratedRole === 'hc4067' || migratedRole === 'hc4051') {
        initMuxFormForPin(lbl);
       }
      }
     }, 0);
    }
   }
  }, 0);
 } else {
  // Pas de configuration existante : attendre que les menus soient remplis, puis sélectionner le premier composant par défaut
  setTimeout(() => {
   // Vérifier que les menus sont remplis
   if(familySel.value && sel.options.length > 0) {
    // Le premier composant est déjà sélectionné par setOptions avec pre=0
    const firstComponentId = sel.value;
    if(firstComponentId) {
     showRoleCards(firstComponentId, {});
     updateRtpForRole(firstComponentId);
     updateRtpParamsVisibility();
     if(firstComponentId === 'hc4067' || firstComponentId === 'hc4051') {
      initMuxFormForPin(lbl);
     }
    }
   }
  }, 0);
 }
 
 const updateConfig=()=>{
 const lbl = $('#selPin')?.textContent || '';
 const currentCfg = pcfg && pcfg[lbl] ? pcfg[lbl] : {};
 showRoleCards(sel.value||'', currentCfg);
 updateRtpForRole(sel.value||'');
 updateRtpParamsVisibility();
 // Si multiplexeur est sélectionné, initialiser le formulaire MUX
 const role = sel.value || '';
 if((role === 'hc4067' || role === 'hc4051') && cur){
  initMuxFormForPin(cur);
 } else {
  // Effacer les valeurs du formulaire MUX pour ne pas polluer getUsedGpios
  if($('#muxSig')) $('#muxSig').value='';
 }
 if(cur){
  pcfg[cur]=readCfg();
  updatePinsList();
  updateBusVisuals();
 }
 };
 
 // Retirer l'ancien handler pour éviter les doublons
 const oldHandler = familySel._onchangeHandler;
 if(oldHandler) {
  familySel.removeEventListener('change', oldHandler);
 }
 
 // Gérer le changement de famille
 const newHandler = () => {
  const familyId = parseInt(familySel.value);
  if(!isNaN(familyId)) {
   populateComponentSelect(familyId, pinType, pin);
   // Réinitialiser la sélection du composant
   sel.value = '';
   showRoleCards('');
   updateRtpForRole('');
   updateRtpParamsVisibility();
   if($('#muxSig')) $('#muxSig').value='';
  }
 };
 familySel.addEventListener('change', newHandler);
 familySel._onchangeHandler = newHandler; // Stocker pour pouvoir le retirer plus tard
 
 sel.onchange=updateConfig;
 
 const inputs=['#btnMode','#btnPulseTiming','#ledMode','#filterIntensity','#rtpEnabled2','#rtpMsgType','#rtpNote','#rtpCc','#rtpPc','#rtpChan','#rtpCcOn','#rtpCcOff','#rtpVel','#rtpCcMin','#rtpCcMax','#rtpNoteMin','#rtpNoteMax','#rtpNoteVelFix','#rtpNoteSweepAutoOffDelay','#oscEnabled2','#oscAddress','#oscFormat','#dbgEnabled','#dbgHeader','#muxS0','#muxS1','#muxS2','#muxS3','#muxEnManual'];
 inputs.forEach(id=>{
 const el=$(id);
 if(el) el.addEventListener('change',updateConfig);
 if(el) el.addEventListener('input',updateConfig);
 });
}

function updateRtpForRole(role){
 const rtpEnable = $('#rtpEnabled2');
 const rtpType = $('#rtpMsgType');
 const rtpParams = $('#rtpParams');
 
 // Migrer les anciens formats si nécessaire
 const migratedRole = migrateRole(role || '');
 
 // Obtenir la définition du composant depuis le backend
 const def = typeof getComponentDefinition === 'function' ? getComponentDefinition(migratedRole) : null;
 
 // Déterminer si MIDI est supporté et quels messages
 let enabled = false;
 let types = [];
 
 if(def) {
  enabled = def.supportsMidi && def.midiMessages && def.midiMessages.length > 0;
  if(enabled) {
   // Construire la liste des types depuis les définitions backend
   types = def.midiMessages.map(m => m.displayName);
  }
 }
 
 if(rtpEnable){ rtpEnable.checked = enabled; rtpEnable.disabled = !enabled; }
 if(rtpType){
 if(enabled){
  // Sauvegarder la valeur actuelle avant de recréer les options
  const currentValue = rtpType.value;
  setOptions(rtpType, types);
  // Restaurer la valeur si elle existe dans les nouvelles options
  if(types.includes(currentValue)){
   rtpType.value = currentValue;
  }
 }
 rtpType.disabled = !enabled;
 }
 if(rtpParams){ rtpParams.style.display = enabled ? 'block' : 'none'; }
 if(enabled) updateRtpParamsVisibility();
}

function updateRtpParamsVisibility(){
 const typeSel = $('#rtpMsgType');
 const params = $('#rtpParams');
 const noteRow = $('#rtpNoteRow');
 const ccRow = $('#rtpCcRow');
 const ccOnOffRow = $('#rtpCcOnOffRow');
 const pcRow = $('#rtpPcRow');
 const velRow = $('#rtpVelRow');
 const ccRangeRow = $('#rtpCcRangeRow');
 const chanRow = $('#rtpChanRow');
 const clockHint = $('#rtpClockHint');
 const noteSweepRow = $('#rtpNoteSweepRow');
 const roleSel = $('#funcSelect');
 if(!typeSel || !params) return;
 const v = typeSel.value;
 [noteRow, ccRow, ccOnOffRow, pcRow, velRow, chanRow, clockHint, noteSweepRow, ccRangeRow].forEach(el=>{ if(el) el.style.display='none'; });
 params.style.display = 'block';
 if(v==='Note'){
 if(noteRow) noteRow.style.display='flex';
 if(chanRow) chanRow.style.display='flex';
 const role = roleSel ? roleSel.value : '';
 if(role==='button' && velRow){ velRow.style.display='flex'; }
 } else if(v==='Control Change'){
 if(ccRow) ccRow.style.display='flex';
 if(chanRow) chanRow.style.display='flex';
 const role = roleSel ? roleSel.value : '';
 if(role==='potentiometer' && ccRangeRow){ ccRangeRow.style.display='flex'; }
 if(role==='button' && ccOnOffRow){ ccOnOffRow.style.display='flex'; }
 } else if(v==='Program Change'){
 if(pcRow) pcRow.style.display='flex';
 if(chanRow) chanRow.style.display='flex';
 } else if(v==='Pitch Bend'){
 if(chanRow) chanRow.style.display='flex';
 } else if(v==='Aftertouch (Channel)'){
 if(chanRow) chanRow.style.display='flex';
 } else if(v==='Note + vélocité'){
 if(noteRow) noteRow.style.display='flex';
 if(chanRow) chanRow.style.display='flex';
 } else if(v==='Note (balayage)'){
 if(noteSweepRow) noteSweepRow.style.display='flex';
 if(chanRow) chanRow.style.display='flex';
 } else if(v==='Clock' || v==='Tap Tempo'){
 if(clockHint) clockHint.style.display='flex';
 }
}

// updateBtnPulseTimingVisibility() est maintenant obsolète
// L'affichage conditionnel est géré par generateFormFields() via dependsOn/showWhen

function readCfg(){
 const c={};
 c.role=$('#funcSelect')?.value||'';
 c.btnMode=$('#btnMode')?.value||'';
 c.btnPulseTiming=$('#btnPulseTiming')?.value||'';
 c.ledMode=$('#ledMode')?.value||'';
 c.filterIntensity=$('#filterIntensity')?.value||'5';
 c.rtpEnabled=!!$('#rtpEnabled2')?.checked;
 c.rtpType=$('#rtpMsgType')?.value||'';
 c.rtpNote=$('#rtpNote')?.value||'';
 c.rtpCc=$('#rtpCc')?.value||'';
 c.rtpPc=$('#rtpPc')?.value||'';
 c.rtpChan=$('#rtpChan')?.value||'';
 c.rtpCcOn=$('#rtpCcOn')?.value||'';
 c.rtpCcOff=$('#rtpCcOff')?.value||'';
 c.rtpVel=$('#rtpVel')?.value||'';
 c.rtpCcMin=$('#rtpCcMin')?.value||'';
 c.rtpCcMax=$('#rtpCcMax')?.value||'';
 c.rtpNoteMin=$('#rtpNoteMin')?.value||'';
 c.rtpNoteMax=$('#rtpNoteMax')?.value||'';
 c.rtpNoteVelFix=$('#rtpNoteVelFix')?.value||'';
 c.rtpNoteSweepAutoOffDelay=$('#rtpNoteSweepAutoOffDelay')?.value||'';
 c.oscEnabled=!!$('#oscEnabled2')?.checked;
 c.oscAddress=$('#oscAddress')?.value||'';
 c.oscFormat=$('#oscFormat')?.value||'float';
 c.dbgEnabled=!!$('#dbgEnabled')?.checked;
 c.dbgHeader=$('#dbgHeader')?.value||'';
 return c;
}

function applyCfg(c){
 if(!c) return;
 const setV=(id,v)=>{
 const el=$(id);
 if(el&&v!=null) el.value=v;
 };
 const setC=(id,b)=>{
 const el=$(id);
 if(el) el.checked=!!b;
 };
 
 // Migrer le rôle si nécessaire
 const migratedRole = typeof migrateRole === 'function' ? migrateRole(c.role) : c.role;
 
 // Restaurer la famille si le rôle est défini
 if(migratedRole && typeof getComponentDefinition === 'function') {
  const def = getComponentDefinition(migratedRole);
  if(def && def.family !== undefined && $('#familySelect')) {
   $('#familySelect').value = def.family;
  }
 }
 
setV('funcSelect',migratedRole);
showRoleCards(migratedRole, c);
updateRtpForRole(migratedRole);
 setC('rtpEnabled2',c.rtpEnabled);
 setV('rtpMsgType',c.rtpType);
 setV('rtpNote',c.rtpNote);
 setV('rtpCc',c.rtpCc);
 setV('rtpPc',c.rtpPc);
 setV('rtpChan',c.rtpChan);
 setV('rtpCcOn',c.rtpCcOn);
 setV('rtpCcOff',c.rtpCcOff);
 setV('rtpVel',c.rtpVel);
 setV('rtpCcMin',c.rtpCcMin);
 setV('rtpCcMax',c.rtpCcMax);
 setV('rtpNoteMin',c.rtpNoteMin);
 setV('rtpNoteMax',c.rtpNoteMax);
 setV('rtpNoteVelFix',c.rtpNoteVelFix);
 setV('rtpNoteSweepAutoOffDelay',c.rtpNoteSweepAutoOffDelay);
 setC('oscEnabled2',c.oscEnabled);
 setV('oscAddress',c.oscAddress);
 setV('oscFormat',c.oscFormat);
 setC('dbgEnabled',c.dbgEnabled);
 setV('dbgHeader',c.dbgHeader);
 updateRtpParamsVisibility();
}

function getGpioFromD(dNum){
 if(!caps||!caps.pins) return null;
 const pin=caps.pins.find(p=>p.label===`D${dNum}`);
 return pin?pin.gpio:null;
}

function getDFromGpio(gpio){
 if(!caps||!caps.pins) return null;
 const pin=caps.pins.find(p=>p.gpio===gpio&&p.label&&p.label.startsWith('D'));
 return pin?parseInt(pin.label.substring(1)):null;
}

function getDigitalPinByGpio(gpio){
 if(!caps||!caps.pins) return null;
 return caps.pins.find(p=>p.gpio===gpio&&p.label&&p.label.startsWith('D'))||null;
}

function isDigitalPinAvailable(gpio, usedGpios){
 return !!getDigitalPinByGpio(gpio) && !usedGpios.has(gpio);
}

// Calculer automatiquement les pins d'adressage S0-S3 en prenant les 4 premières pins digitales disponibles
function calculateMuxAddressPins(sigGpio, usedGpiosOverride=null){
 // Obtenir les GPIO déjà utilisés (sauf le SIG actuel)
 const usedGpios = usedGpiosOverride || getUsedGpios([]);
 usedGpios.delete(sigGpio);
 
 // Obtenir toutes les pins digitales disponibles
 const availablePins = getAvailableDigitalPins(usedGpios);
 
 // Prendre les 4 premières
 const result = {
  s0: availablePins[0] ? parseInt(availablePins[0].gpio) : null,
  s1: availablePins[1] ? parseInt(availablePins[1].gpio) : null,
  s2: availablePins[2] ? parseInt(availablePins[2].gpio) : null,
  s3: availablePins[3] ? parseInt(availablePins[3].gpio) : null
 };
 
 return result;
}

// Obtenir toutes les pins digitales uniques (dédupliquées par GPIO)
function getAllDigitalPins(){
 if(!caps||!caps.pins) return [];
 const allDPinsRaw=caps.pins.filter(p=>p.label&&p.label.startsWith('D'));
 const uniqueDPinsMap=new Map();
 allDPinsRaw.forEach(p=>{
  if(!uniqueDPinsMap.has(p.gpio)){
   uniqueDPinsMap.set(p.gpio,p);
  }
 });
 return Array.from(uniqueDPinsMap.values()).sort((a,b)=>{
  const numA=parseInt(a.label.substring(1));
  const numB=parseInt(b.label.substring(1));
  return numA-numB;
 });
}

// Obtenir les pins digitales disponibles (filtrées par usedGpios, avec exception pour currentValues)
function getAvailableDigitalPins(usedGpios, currentValues=null){
 const allDPins=getAllDigitalPins();
 const currentSet=currentValues instanceof Set ? currentValues : new Set();
 return allDPins.filter(p=>{
  return !usedGpios.has(p.gpio) || currentSet.has(p.gpio);
 });
}

// Vérifier la disponibilité du mode auto pour un MUX
function checkMuxAutoAvailability(sigGpio, usedGpios){
 if(!caps||!caps.pins) return false;
 const sigPin=caps.pins.find(p=>p.gpio===sigGpio);
 if(!sigPin) return false;
 const usedGpiosCopy=new Set(usedGpios);
 usedGpiosCopy.delete(sigGpio);
 return areMuxAddressPinsAvailable(sigGpio, usedGpiosCopy);
}

// Vérifier la disponibilité de la pin EN pour un MUX
function checkMuxEnAvailability(sigGpio, usedGpios){
 const enGpio=sigGpio+5;
 const enPin=getDigitalPinByGpio(enGpio);
 if(!enPin) return false;
 const usedGpiosCopy=new Set(usedGpios);
 usedGpiosCopy.delete(sigGpio);
 const addrPins=calculateMuxAddressPins(sigGpio);
 usedGpiosCopy.delete(addrPins.s0);
 usedGpiosCopy.delete(addrPins.s1);
 usedGpiosCopy.delete(addrPins.s2);
 usedGpiosCopy.delete(addrPins.s3);
 return !usedGpiosCopy.has(enGpio);
}

// Obtenir toutes les informations de disponibilité pour un MUX (auto + EN)
function getMuxAvailabilityInfo(sigGpio, usedGpios){
 const autoAvailable=checkMuxAutoAvailability(sigGpio, usedGpios);
 const enGpio=sigGpio+5;
 const enPin=getDigitalPinByGpio(enGpio);
 const enAvailable=enPin&&checkMuxEnAvailability(sigGpio, usedGpios);
 return {autoAvailable, enAvailable, enGpio, enPin};
}


// Vérifier si les pins d'adressage sont disponibles pour un GPIO SIG donné
function areMuxAddressPinsAvailable(sigGpio, excludeUsedGpios=null){
 if(!caps||!caps.pins) return false;
 const sigPin=caps.pins.find(p=>p.gpio===sigGpio);
 if(!sigPin) return false;
 const usedGpios=excludeUsedGpios||getUsedGpios([]);
 // Exclure le GPIO SIG lui-même
 usedGpios.delete(sigGpio);
 // Vérifier qu'il y a au moins 4 pins digitales disponibles
 const availablePins = getAvailableDigitalPins(usedGpios);
 return availablePins.length >= 4;
}

function getUsedGpios(additionalSelectIds=[]){
 const usedGpios=new Set();
 
 // Ajouter les GPIO des pins configurées
 Object.keys(pcfg).forEach(lbl=>{
  const cfg=pcfg[lbl];
  const pin=caps.pins.find(p=>p.label===lbl);
  if(!pin) return;
  
  // Pour les MUX temporaires, ajouter aussi les pins d'adresse
  const role = cfg?.role ? migrateRole(cfg.role) : '';
  if(role === 'hc4067' || role === 'hc4051'){
   const sigGpio=parseInt(pin.gpio);
   usedGpios.add(sigGpio);
   // Calculer et ajouter les pins d'adresse (mode auto)
   // Passer usedGpios pour éviter une boucle infinie
   const addrPins=calculateMuxAddressPins(sigGpio, usedGpios);
   if(addrPins.s0 !== null) usedGpios.add(addrPins.s0);
   if(addrPins.s1 !== null) usedGpios.add(addrPins.s1);
   if(addrPins.s2 !== null) usedGpios.add(addrPins.s2);
   if(addrPins.s3 !== null) usedGpios.add(addrPins.s3);
  } else {
   usedGpios.add(pin.gpio);
  }
 });
 
 // Ne pas exclure le MUX en cours d'édition sauf si on est vraiment en train d'éditer un MUX
 const funcSelectValue = $('#funcSelect')?.value || '';
 const isEditingMux = funcSelectValue === 'hc4067' || funcSelectValue === 'hc4051';
 const currentMuxId = (isEditingMux && $('#muxId')) ? parseInt($('#muxId').value) : null;
 if(typeof muxList !== 'undefined' && Array.isArray(muxList)){
  muxList.forEach(m=>{
   // Exclure seulement si on édite vraiment ce MUX spécifique
   if(isEditingMux && currentMuxId!==null && m.id===currentMuxId) return;
   const sig=parseInt(m.sig), s0=parseInt(m.s0), s1=parseInt(m.s1), s2=parseInt(m.s2), s3=parseInt(m.s3), en=parseInt(m.en);
   if(!isNaN(sig)) usedGpios.add(sig);
   if(!isNaN(s0)) usedGpios.add(s0);
   if(!isNaN(s1)) usedGpios.add(s1);
   if(!isNaN(s2)) usedGpios.add(s2);
   if(!isNaN(s3)) usedGpios.add(s3);
   if(!isNaN(en) && en!==255) usedGpios.add(en);
  });
 }
 additionalSelectIds.forEach(id=>{
 if(id==='muxEnManual'){
  const sel=$('#muxEnManual');
  if(sel&&sel.value&&sel.value!=='255'){
   const gpio=parseInt(sel.value);
   if(!isNaN(gpio)) usedGpios.add(gpio);
  }
 } else{
  const sel=$('#'+id);
  if(!sel||!sel.value||sel.value==='255') return;
  if(id==='muxSig'){
   const sigGpio=parseInt(sel.value);
   if(!isNaN(sigGpio)){
    usedGpios.add(sigGpio);
    // Toujours utiliser les valeurs manuelles (S0-S3)
    ['muxS0','muxS1','muxS2','muxS3'].forEach(selId=>{
     const manualSel=$('#'+selId);
     if(manualSel&&manualSel.value){
      const gpio=parseInt(manualSel.value);
      if(!isNaN(gpio)) usedGpios.add(gpio);
     }
    });
   }
  } else{
   const gpio=parseInt(sel.value);
   if(!isNaN(gpio)) usedGpios.add(gpio);
  }
 }
 });
 return usedGpios;
}

// Nouvelle fonction pour initialiser le formulaire multiplexeur depuis un pin
function initMuxFormForPin(pinLabel){
 if(!caps||!caps.pins) return;
 const pin=caps.pins.find(p=>p.label===pinLabel);
 if(!pin) return;
 const sigGpio=pin.gpio;
 
 const usedGpios=getUsedGpios([]);
 const availInfo=getMuxAvailabilityInfo(sigGpio, usedGpios);
 
 if(typeof populateMuxPinSelects === 'function') populateMuxPinSelects();
 
 // Trouver un multiplexeur existant qui utilise ce pin comme SIG, ou créer un nouveau
 const existingMux=muxList.find(m=>m.sig===sigGpio);
 if(existingMux){
  // Charger la configuration existante
  loadMuxConfigIntoForm(existingMux);
 } else {
  // Nouveau multiplexeur - initialiser avec des valeurs par défaut
  if($('#muxSig')) $('#muxSig').value=sigGpio;
  const addrPins=calculateMuxAddressPins(sigGpio, usedGpios);
  if($('#muxS0') && addrPins.s0 !== null) $('#muxS0').value=addrPins.s0;
  if($('#muxS1') && addrPins.s1 !== null) $('#muxS1').value=addrPins.s1;
  if($('#muxS2') && addrPins.s2 !== null) $('#muxS2').value=addrPins.s2;
  if($('#muxS3') && addrPins.s3 !== null) $('#muxS3').value=addrPins.s3;
  if($('#muxEnManual')) $('#muxEnManual').value='255';
  let muxId=0;
  if($('#muxId')){
   // Trouver le premier ID disponible
   const usedIds=muxList.map(m=>m.id);
   const availableId=[0,1].find(id=>!usedIds.includes(id));
   if(availableId!==undefined){
    $('#muxId').value=availableId;
    muxId=availableId;
   }
  }
  // Initialiser l'adresse OSC avec /mux[ID]
  if($('#oscAddress')) $('#oscAddress').value='/mux'+muxId;
  // Mettre à jour la visualisation des pins
  if(typeof updateBusVisuals === 'function') updateBusVisuals();
 }
}

function loadMuxConfigIntoForm(mux){
 if(typeof populateMuxPinSelects === 'function') populateMuxPinSelects();
 if($('#muxId')) $('#muxId').value=mux.id;
 if($('#muxSig')) $('#muxSig').value=mux.sig;
 const sigGpio=mux.sig;
 
 if($('#muxS0')) $('#muxS0').value=mux.s0;
 if($('#muxS1')) $('#muxS1').value=mux.s1;
 if($('#muxS2')) $('#muxS2').value=mux.s2;
 if($('#muxS3')) $('#muxS3').value=mux.s3;
 if($('#muxEnManual')) $('#muxEnManual').value=mux.en!==undefined?mux.en:255;
 if($('#muxMin')) $('#muxMin').value=mux.min!==undefined?mux.min:0;
 if($('#muxMax')) $('#muxMax').value=mux.max!==undefined?mux.max:4095;
 if($('#muxFilterIntensity')) $('#muxFilterIntensity').value=mux.filterIntensity!==undefined?mux.filterIntensity:5;
 if($('#rtpCc')) $('#rtpCc').value=mux.ccBase||1;
 if($('#rtpChan')) $('#rtpChan').value=mux.midiChan||1;
 if($('#rtpMsgType')) $('#rtpMsgType').value='Control Change';
 if($('#oscAddress')) $('#oscAddress').value=mux.oscBase||'/mux'+mux.id;
 if($('#oscFormat')) $('#oscFormat').value=mux.oscFormat||'float';
 if($('#rtpEnabled2')) $('#rtpEnabled2').checked=true;
 if($('#oscEnabled2')) $('#oscEnabled2').checked=true;
 // Mettre à jour la visualisation des pins
 if(typeof updateBusVisuals === 'function') updateBusVisuals();
}

async function saveMuxFromPin(){
 const id=$('#muxId').value;
 const sig=parseInt($('#muxSig').value);
 const ccBase=parseInt($('#rtpCc').value)||1;
 const midiChan=parseInt($('#rtpChan').value)||1;
 const oscBase=$('#oscAddress').value||'/mux'+id;
 if(!sig||isNaN(sig)){
  $('#muxMsg').textContent='Erreur: Veuillez choisir un pin analogique';
  $('#muxMsg').style.color='#ef4444';
  return;
 }
 // Toujours utiliser les valeurs manuelles
 const s0=parseInt($('#muxS0')?.value);
 const s1=parseInt($('#muxS1')?.value);
 const s2=parseInt($('#muxS2')?.value);
 const s3=parseInt($('#muxS3')?.value);
 let en=255;
 const enManual=$('#muxEnManual');
 if(enManual&&enManual.value&&enManual.value!=='255'){
  en=parseInt(enManual.value);
 }
 const min=parseInt($('#muxMin').value)||0;
 const max=parseInt($('#muxMax').value)||4095;
 const oscFormat=$('#oscFormat').value||'float';
 const filterIntensity=parseInt($('#muxFilterIntensity').value)||5;
 const formData=new URLSearchParams();
 formData.append('id',id);
 formData.append('sig',sig);
 formData.append('s0',s0);
 formData.append('s1',s1);
 formData.append('s2',s2);
 formData.append('s3',s3);
 formData.append('en',en);
 formData.append('ccBase',ccBase);
 formData.append('midiChan',midiChan);
 formData.append('oscBase',oscBase);
 formData.append('min',min);
 formData.append('max',max);
 formData.append('oscFormat',oscFormat);
 formData.append('filterIntensity',filterIntensity);
 try{
 const r=await fetch('/api/mux/add',{method:'POST',body:formData});
 const d=await r.json();
 if(d.status==='ok'){
 $('#muxMsg').textContent='Multiplexeur enregistré!';
 $('#muxMsg').style.color='#10b981';
 // Supprimer l'entrée de pcfg pour la pin SIG (le MUX est géré via muxList)
 if(caps && caps.pins){
  const sigPin=caps.pins.find(p=>p.gpio===sig);
  if(sigPin && sigPin.label && pcfg[sigPin.label] && pcfg[sigPin.label].role){
   const role = migrateRole(pcfg[sigPin.label].role);
   if(role === 'hc4067' || role === 'hc4051'){
    delete pcfg[sigPin.label];
   }
  }
 }
 if(typeof loadMuxList === 'function') await loadMuxList();
 if(typeof loadCaps === 'function') await loadCaps();
 if(typeof updatePinsList === 'function') updatePinsList();
 if(typeof updateBusVisuals === 'function') updateBusVisuals();
 } else{
 $('#muxMsg').textContent='Erreur: '+(d.error||'Inconnu');
 $('#muxMsg').style.color='#ef4444';
 }
 } catch(e){
 $('#muxMsg').textContent='Erreur réseau';
 $('#muxMsg').style.color='#ef4444';
 }
}

/**
 * Génère dynamiquement les champs de formulaire pour un composant
 * @param {Object} def - Définition du composant depuis le backend
 * @param {string} containerId - ID du conteneur (ex: "cardLed")
 * @param {Object} currentCfg - Configuration actuelle (optionnel, pour pré-remplir)
 */
function generateFormFields(def, containerId, currentCfg = {}) {
 const container = $('#' + containerId);
 if(!container || !def || !def.formFields || !Array.isArray(def.formFields)) {
  console.warn('[generateFormFields] Container ou formFields manquant', containerId, def);
  return;
 }
 
 // Vider le conteneur
 container.innerHTML = '';
 
 // Parcourir tous les champs de formulaire
 def.formFields.forEach(field => {
  // Gérer l'affichage conditionnel
  let fieldContainer = container;
  if(field.dependsOn && field.showWhen) {
   const dependsOnEl = $('#' + field.dependsOn);
   if(dependsOnEl) {
    const showWhenValues = JSON.parse(field.showWhen || '[]');
    const shouldShow = showWhenValues.includes(dependsOnEl.value);
    // Créer un élément pour gérer l'affichage conditionnel
    let hiddenWrapper = $('#' + field.id + 'Row');
    if(!hiddenWrapper) {
     hiddenWrapper = document.createElement('div');
     hiddenWrapper.id = field.id + 'Row';
     container.appendChild(hiddenWrapper);
     // Ajouter l'écouteur d'événement
     dependsOnEl.addEventListener('change', () => {
      const newValue = dependsOnEl.value;
      const shouldShowNow = showWhenValues.includes(newValue);
      hiddenWrapper.style.display = shouldShowNow ? 'block' : 'none';
     });
    }
    hiddenWrapper.style.display = shouldShow ? 'block' : 'none';
    fieldContainer = hiddenWrapper;
   }
  }
  
  // Créer le wrapper
  const wrapper = document.createElement('div');
  wrapper.className = field.wrapperClass || 'r';
  if(field.type === 5) { // INFO
   // Pour INFO, pas de wrapper, juste la div.hint
   const infoDiv = document.createElement('div');
   infoDiv.className = field.hintClass || 'hint';
   infoDiv.textContent = field.hint || '';
   fieldContainer.appendChild(infoDiv);
   return;
  }
  
  // Label principal
  if(field.label) {
   const label = document.createElement('label');
   label.textContent = field.label;
   if(field.required) label.textContent += ' *';
   wrapper.appendChild(label);
  }
  
  // Label avant (pour champs complexes)
  if(field.labelBefore) {
   const labelBefore = document.createElement('span');
   labelBefore.textContent = field.labelBefore;
   wrapper.appendChild(labelBefore);
  }
  
  // Créer l'input selon le type
  let input;
  
  switch(field.type) {
   case 0: // TEXT
    input = document.createElement('input');
    input.type = 'text';
    input.id = field.id;
    if(field.placeholder) input.placeholder = field.placeholder;
    if(field.maxLength > 0) input.maxLength = field.maxLength;
    if(field.pattern) input.pattern = field.pattern;
    if(field.width > 0) input.style.width = field.width + 'px';
    if(field.defaultValue && !currentCfg[field.id]) input.value = field.defaultValue;
    else if(currentCfg[field.id]) input.value = currentCfg[field.id];
    break;
    
   case 1: // NUMBER
    input = document.createElement('input');
    input.type = 'number';
    input.id = field.id;
    input.min = field.min;
    input.max = field.max;
    if(field.step) input.step = field.step;
    if(field.placeholder) input.placeholder = field.placeholder;
    if(field.width > 0) input.style.width = field.width + 'px';
    if(field.defaultValue && !currentCfg[field.id]) input.value = field.defaultValue;
    else if(currentCfg[field.id] !== undefined) input.value = currentCfg[field.id];
    break;
    
   case 2: // SELECT
    input = document.createElement('select');
    input.id = field.id;
    if(field.width > 0) input.style.width = field.width + 'px';
    if(field.options) {
     try {
      const options = JSON.parse(field.options);
      options.forEach(opt => {
       const option = document.createElement('option');
       option.value = opt.value;
       option.textContent = opt.label;
       input.appendChild(option);
      });
     } catch(e) {
      console.warn('[generateFormFields] Erreur parsing options:', e);
     }
    }
    if(field.defaultValue && !currentCfg[field.id]) input.value = field.defaultValue;
    else if(currentCfg[field.id]) input.value = currentCfg[field.id];
    break;
    
   case 3: // CHECKBOX
    input = document.createElement('input');
    input.type = 'checkbox';
    input.id = field.id;
    if(field.label) {
     const labelFor = document.createElement('label');
     labelFor.setAttribute('for', field.id);
     labelFor.textContent = field.label;
     wrapper.insertBefore(labelFor, wrapper.firstChild);
    }
    if(field.defaultValue === 'true' || currentCfg[field.id] === true || currentCfg[field.id] === 'true') {
     input.checked = true;
    }
    break;
    
   case 4: // RANGE
    // Pour RANGE, créer deux inputs number
    const rangeWrapper = document.createElement('span');
    const inputMin = document.createElement('input');
    inputMin.type = 'number';
    inputMin.id = field.id + 'Min';
    inputMin.min = field.min;
    inputMin.max = field.max;
    if(field.step) inputMin.step = field.step;
    if(field.width > 0) inputMin.style.width = field.width + 'px';
    // Pour RANGE, defaultValue est utilisé pour le min, max utilise field.max par défaut
    if(currentCfg[field.id + 'Min'] !== undefined) {
     inputMin.value = currentCfg[field.id + 'Min'];
    } else if(field.defaultValue) {
     inputMin.value = field.defaultValue;
    } else {
     inputMin.value = field.min;
    }
    
    const separator = document.createElement('span');
    separator.textContent = field.separator || '→';
    separator.style.margin = '0 4px';
    
    const inputMax = document.createElement('input');
    inputMax.type = 'number';
    inputMax.id = field.id + 'Max';
    inputMax.min = field.min;
    inputMax.max = field.max;
    if(field.step) inputMax.step = field.step;
    if(field.width > 0) inputMax.style.width = field.width + 'px';
    if(currentCfg[field.id + 'Max'] !== undefined) {
     inputMax.value = currentCfg[field.id + 'Max'];
    } else {
     inputMax.value = field.max;
    }
    
    rangeWrapper.appendChild(inputMin);
    rangeWrapper.appendChild(separator);
    rangeWrapper.appendChild(inputMax);
    input = rangeWrapper;
    break;
    
   default:
    console.warn('[generateFormFields] Type de champ inconnu:', field.type);
    return;
  }
  
  if(input && field.inputClass) {
   if(typeof input.classList !== 'undefined') {
    input.classList.add(...field.inputClass.split(' '));
   } else {
    input.className = (input.className || '') + ' ' + field.inputClass;
   }
  }
  
  wrapper.appendChild(input);
  
  // Label après (pour champs complexes)
  if(field.labelAfter) {
   const labelAfter = document.createElement('span');
   labelAfter.textContent = field.labelAfter;
   wrapper.appendChild(labelAfter);
  }
  
  // Hint inline
  if(field.hintPosition === 1 && field.hint) { // INLINE
   const hintSpan = document.createElement('span');
   hintSpan.textContent = field.hint;
   if(field.hintClass) {
    hintSpan.setAttribute('style', field.hintClass);
   } else {
    hintSpan.style.marginLeft = '8px';
    hintSpan.style.fontSize = '0.9em';
    hintSpan.style.color = '#666';
   }
   wrapper.appendChild(hintSpan);
  }
  
  fieldContainer.appendChild(wrapper);
  
  // Hint en dessous
  if(field.hintPosition === 2 && field.hint) { // BELOW
   const hintDiv = document.createElement('div');
   hintDiv.className = field.hintClass || 'hint';
   hintDiv.textContent = field.hint;
   fieldContainer.appendChild(hintDiv);
  }
 });
}
