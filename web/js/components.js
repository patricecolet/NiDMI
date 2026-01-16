/* Fonctions de gestion des composants et configurations */

/**
 * Affiche la carte de configuration correspondant au rôle sélectionné
 * Utilise les définitions du backend pour déterminer le cardId
 * @param {string} role - ID du rôle (ex: "potentiometer", "hc4067")
 */
function showRoleCards(role, currentCfg = {}){
 // Utiliser un seul conteneur générique pour tous les composants
 const card = $('#componentFormCard');
 if(!card) {
  console.warn('[showRoleCards] Conteneur componentFormCard non trouvé');
  return;
 }
 
 // Masquer et vider le conteneur par défaut
 card.style.display = 'none';
 card.innerHTML = '';
 
 if(!role) return;
 
 // Migrer les anciens formats si nécessaire
 const migratedRole = typeof migrateRole === 'function' ? migrateRole(role) : role;
 
 // Trouver la définition du composant
 const def = typeof getComponentDefinition === 'function' ? getComponentDefinition(migratedRole) : null;
 if(!def) {
  console.warn('[showRoleCards] Définition non trouvée pour:', migratedRole);
  return;
 }
 
 // Afficher le conteneur
 card.style.display = 'block';
 
 // Générer les champs de formulaire dynamiquement
 if(def.formFields && Array.isArray(def.formFields) && def.formFields.length > 0) {
  if(typeof generateFormFields === 'function') {
   generateFormFields(def, 'componentFormCard', currentCfg);
  }
 }
 
 // Générer les pins additionnelles si composant complexe
 if(def.isComplex && def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0) {
  if(typeof generateAdditionalPins === 'function') {
   generateAdditionalPins(def, 'componentFormCard', currentCfg);
  }
 }
 
 // Générer la section RTP-MIDI dynamiquement
 if(typeof generateRtpMidiSection === 'function') {
  generateRtpMidiSection(def, currentCfg, 'rtpMidiSection');
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
  
  // Vérifier disponibilité MUX pour les composants complexes (multiplexeurs)
  if(def.isComplex && pinType === 0) { // Composant complexe sur pin analogique
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
  // Vérifier disponibilité MUX pour les composants complexes (multiplexeurs)
  if(def.isComplex && pinType === 0) { // Composant complexe sur pin analogique
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
       // Si composant complexe (MUX), initialiser le formulaire
       if(def && def.isComplex && typeof initMuxFormForPin === 'function') {
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
       // Si composant complexe (MUX), initialiser le formulaire
       if(def && def.isComplex && typeof initMuxFormForPin === 'function') {
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
     // Si composant complexe (MUX), initialiser le formulaire
     const firstDef = typeof getComponentDefinition === 'function' ? getComponentDefinition(firstComponentId) : null;
     if(firstDef && firstDef.isComplex && typeof initMuxFormForPin === 'function') {
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
 // Si composant complexe (MUX) est sélectionné, initialiser le formulaire
 const role = sel.value || '';
 const migratedRole = typeof migrateRole === 'function' ? migrateRole(role) : role;
 const def = typeof getComponentDefinition === 'function' ? getComponentDefinition(migratedRole) : null;
 if(def && def.isComplex && cur && typeof initMuxFormForPin === 'function'){
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
 
 // Générer la liste d'inputs dynamiquement
 const inputs = getAllFieldIds();
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
 const roleSel = $('#funcSelect');
 if(!typeSel || !params) return;
 
 const v = typeSel.value;
 const role = roleSel ? roleSel.value : '';
 
 // Masquer tous les champs
 const allRows = params.querySelectorAll('[id$="Row"]');
 allRows.forEach(row => {
  if(row._showFor) {
   // Vérifier si ce champ doit être affiché pour ce type de message
   const shouldShow = row._showFor.includes(v);
   if(shouldShow && row._dependsOnRole) {
    // Vérifier aussi le rôle si nécessaire
    row.style.display = row._dependsOnRole.includes(role) ? 'flex' : 'none';
   } else {
    row.style.display = shouldShow ? 'flex' : 'none';
   }
  } else {
   row.style.display = 'none';
  }
 });
 
 // La logique conditionnelle est maintenant gérée par dependsOnRole dans generateRtpParams
}

// updateBtnPulseTimingVisibility() est maintenant obsolète
// L'affichage conditionnel est géré par generateFormFields() via dependsOn/showWhen

/**
 * Collecte tous les IDs de champs depuis les formFields et les champs RTP
 * @returns {Array<string>} Liste des IDs de champs
 */
function getAllFieldIds() {
 const ids = [];
 
 // Collecter depuis les formFields de tous les composants
 if(typeof componentDefinitions !== 'undefined' && componentDefinitions) {
  componentDefinitions.forEach(def => {
   if(def.formFields && Array.isArray(def.formFields)) {
    def.formFields.forEach(field => {
     if(field.id && !field.id.startsWith('_')) { // Ignorer les hints standalone
      ids.push('#' + field.id);
      // Pour RANGE, ajouter Min et Max
      if(field.type === 4) { // RANGE
       ids.push('#' + field.id + 'Min');
       ids.push('#' + field.id + 'Max');
      }
     }
    });
   }
  });
 }
 
 // Ajouter les champs RTP-MIDI standards (toujours présents)
 ids.push('#rtpEnabled2', '#rtpMsgType');
 
 // Collecter les paramètres MIDI depuis toutes les définitions
 if(typeof componentDefinitions !== 'undefined' && componentDefinitions) {
  componentDefinitions.forEach(def => {
   if(def.midiMessages && Array.isArray(def.midiMessages)) {
    def.midiMessages.forEach(msg => {
     if(msg.params && Array.isArray(msg.params)) {
      msg.params.forEach(param => {
       if(param.id) {
        const paramId = '#' + param.id;
        if(!ids.includes(paramId)) ids.push(paramId);
        // Pour RANGE, ajouter Min et Max
        if(param.type === 4) { // RANGE
         const paramMinId = '#' + param.id + 'Min';
         const paramMaxId = '#' + param.id + 'Max';
         if(!ids.includes(paramMinId)) ids.push(paramMinId);
         if(!ids.includes(paramMaxId)) ids.push(paramMaxId);
        }
       }
      });
     }
    });
   }
  });
 }
 
 // Ajouter les champs OSC et Debug
 ids.push('#oscEnabled2', '#oscAddress', '#oscFormat', '#dbgEnabled', '#dbgHeader');
 
 // Collecter les IDs des additionalPins depuis toutes les définitions
 if(typeof componentDefinitions !== 'undefined' && componentDefinitions) {
  componentDefinitions.forEach(def => {
   if(def.isComplex && def.additionalPins && Array.isArray(def.additionalPins)) {
    def.additionalPins.forEach(additionalPin => {
     if(additionalPin.id) {
      // ID du champ : préfixe depuis l'ID du composant + id en capital (ex: s0 -> hc4067S0)
      const prefix = def.id ? def.id : 'comp';
      const fieldId = '#' + prefix + additionalPin.id.charAt(0).toUpperCase() + additionalPin.id.slice(1);
      if(!ids.includes(fieldId)) ids.push(fieldId);
     }
    });
   }
  });
 }
 
 return ids;
}

function readCfg(){
 const c={};
 c.role=$('#funcSelect')?.value||'';
 
 // Lire les champs depuis les formFields du composant actuel
 const roleSel = $('#funcSelect');
 if(roleSel && roleSel.value) {
  const migratedRole = typeof migrateRole === 'function' ? migrateRole(roleSel.value) : roleSel.value;
  const def = typeof getComponentDefinition === 'function' ? getComponentDefinition(migratedRole) : null;
  
  if(def && def.formFields && Array.isArray(def.formFields)) {
   def.formFields.forEach(field => {
    if(field.id && !field.id.startsWith('_')) {
     const el = $('#' + field.id);
     if(el) {
      if(field.type === 3) { // CHECKBOX
       c[field.id] = el.checked;
      } else if(field.type === 4) { // RANGE
       const elMin = $('#' + field.id + 'Min');
       const elMax = $('#' + field.id + 'Max');
       if(elMin) c[field.id + 'Min'] = elMin.value || '';
       if(elMax) c[field.id + 'Max'] = elMax.value || '';
      } else {
       c[field.id] = el.value || '';
      }
     }
    }
   });
  }
 }
 
 // Lire les champs RTP-MIDI standards
 c.rtpEnabled=!!$('#rtpEnabled2')?.checked;
 c.rtpType=$('#rtpMsgType')?.value||'';
 
 // Lire dynamiquement tous les paramètres MIDI depuis les définitions
 if(migratedRole && def && def.midiMessages && Array.isArray(def.midiMessages)) {
  def.midiMessages.forEach(msg => {
   if(msg.params && Array.isArray(msg.params)) {
    msg.params.forEach(param => {
     if(param.id) {
      const el = $('#' + param.id);
      if(el) {
       if(param.type === 4) { // RANGE
        const elMin = $('#' + param.id + 'Min');
        const elMax = $('#' + param.id + 'Max');
        if(elMin) c[param.id + 'Min'] = elMin.value || '';
        if(elMax) c[param.id + 'Max'] = elMax.value || '';
       } else {
        c[param.id] = el.value || '';
       }
      }
     }
    });
   }
  });
 }
 
 // Lire les champs OSC et Debug
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

 // Appliquer les champs depuis les formFields du composant
 if(migratedRole && typeof getComponentDefinition === 'function') {
  const def = getComponentDefinition(migratedRole);
  if(def && def.formFields && Array.isArray(def.formFields)) {
   def.formFields.forEach(field => {
    if(field.id && !field.id.startsWith('_')) {
     if(field.type === 3) { // CHECKBOX
      setC(field.id, c[field.id]);
     } else if(field.type === 4) { // RANGE
      setV(field.id + 'Min', c[field.id + 'Min']);
      setV(field.id + 'Max', c[field.id + 'Max']);
     } else {
      setV(field.id, c[field.id]);
     }
    }
   });
  }
 }
 
 // Appliquer les champs RTP-MIDI standards
 setC('rtpEnabled2',c.rtpEnabled);
 setV('rtpMsgType',c.rtpType);
 
 // Appliquer dynamiquement tous les paramètres MIDI depuis les définitions
 if(migratedRole && typeof getComponentDefinition === 'function') {
  const def = getComponentDefinition(migratedRole);
  if(def && def.midiMessages && Array.isArray(def.midiMessages)) {
   def.midiMessages.forEach(msg => {
    if(msg.params && Array.isArray(msg.params)) {
     msg.params.forEach(param => {
      if(param.id) {
       if(param.type === 4) { // RANGE
        setV(param.id + 'Min', c[param.id + 'Min']);
        setV(param.id + 'Max', c[param.id + 'Max']);
       } else {
        setV(param.id, c[param.id]);
       }
      }
     });
    }
   });
  }
 }
 
 // Appliquer les champs OSC et Debug
 setC('oscEnabled2',c.oscEnabled);
 setV('oscAddress',c.oscAddress);
 setV('oscFormat',c.oscFormat);
 setC('dbgEnabled',c.dbgEnabled);
 setV('dbgHeader',c.dbgHeader);
 
 // Mettre à jour la visibilité des paramètres RTP
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
  
  // Pour les composants complexes (MUX) temporaires, ajouter aussi les pins d'adresse
  const role = cfg?.role ? migrateRole(cfg.role) : '';
  const def = typeof getComponentDefinition === 'function' ? getComponentDefinition(role) : null;
  if(def && def.isComplex){
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
 // Vérifier si le composant sélectionné est complexe (MUX)
 const funcDef = typeof getComponentDefinition === 'function' ? getComponentDefinition(funcSelectValue) : null;
 const isEditingMux = funcDef && funcDef.isComplex;
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
 // Trouver la définition MUX
 const funcSelectValue = $('#funcSelect')?.value || '';
 const migratedRole = typeof migrateRole === 'function' ? migrateRole(funcSelectValue) : funcSelectValue;
 const def = typeof getComponentDefinition === 'function' ? getComponentDefinition(migratedRole) : null;
 
 if(!def || !def.isComplex) {
  console.warn('[saveMuxFromPin] Définition MUX non trouvée ou composant non complexe');
  return;
 }
 
 // Lire le MUX ID (peut être dans un champ formField ou généré automatiquement)
 let muxId = null;
 const muxIdField = $('#muxId');
 if(muxIdField) {
  muxId = muxIdField.value;
 } else {
  // Générer un ID disponible
  const existingIds = (muxList || []).map(m => parseInt(m.id)).filter(id => !isNaN(id));
  muxId = existingIds.length > 0 ? Math.max(...existingIds) + 1 : 0;
 }
 
 // Lire dynamiquement les additionalPins
 const additionalPinValues = {};
 if(def.additionalPins && Array.isArray(def.additionalPins)) {
  def.additionalPins.forEach(additionalPin => {
   if(additionalPin.id) {
    // ID du champ : préfixe depuis l'ID du composant + id en capital (ex: s0 -> hc4067S0)
    const prefix = def.id ? def.id : 'comp';
    const fieldId = prefix + additionalPin.id.charAt(0).toUpperCase() + additionalPin.id.slice(1);
    const field = $('#' + fieldId);
    if(field && field.value) {
     additionalPinValues[additionalPin.id] = parseInt(field.value);
    } else if(additionalPin.defaultValue !== undefined && additionalPin.defaultValue !== null) {
     additionalPinValues[additionalPin.id] = parseInt(additionalPin.defaultValue);
    } else if(additionalPin.optional) {
     additionalPinValues[additionalPin.id] = 255; // Non connecté par défaut
    }
   }
  });
 }
 
 const sig = additionalPinValues.sig || parseInt($('#muxSig')?.value);
 if(!sig || isNaN(sig)){
  $('#muxMsg').textContent='Erreur: Veuillez choisir un pin analogique';
  $('#muxMsg').style.color='#ef4444';
  return;
 }
 
 const s0 = additionalPinValues.s0 || parseInt($('#muxS0')?.value);
 const s1 = additionalPinValues.s1 || parseInt($('#muxS1')?.value);
 const s2 = additionalPinValues.s2 || parseInt($('#muxS2')?.value);
 const s3 = additionalPinValues.s3 || parseInt($('#muxS3')?.value);
 const en = additionalPinValues.en !== undefined ? additionalPinValues.en : (parseInt($('#muxEnManual')?.value) || 255);
 
 // Lire dynamiquement les formFields
 const formFieldValues = {};
 if(def.formFields && Array.isArray(def.formFields)) {
  def.formFields.forEach(field => {
   if(field.id && !field.id.startsWith('_')) {
    const el = $('#' + field.id);
    if(el) {
     if(field.type === 3) { // CHECKBOX
      formFieldValues[field.id] = el.checked ? 'true' : 'false';
     } else if(field.type === 4) { // RANGE
      const elMin = $('#' + field.id + 'Min');
      const elMax = $('#' + field.id + 'Max');
      if(elMin) formFieldValues[field.id + 'Min'] = elMin.value || field.defaultValue || '0';
      if(elMax) formFieldValues[field.id + 'Max'] = elMax.value || field.defaultValue || '4095';
     } else {
      formFieldValues[field.id] = el.value || field.defaultValue || '';
     }
    }
   }
  });
 }
 
 const min = parseInt(formFieldValues.muxMin || $('#muxMin')?.value || '0');
 const max = parseInt(formFieldValues.muxMax || $('#muxMax')?.value || '4095');
 const filterIntensity = parseInt(formFieldValues.muxFilterIntensity || $('#muxFilterIntensity')?.value || '5');
 
 // Lire les paramètres RTP-MIDI dynamiquement
 const rtpCc = parseInt($('#rtpCc')?.value || $('#rtpNote')?.value || $('#rtpPc')?.value || '1');
 const rtpChan = parseInt($('#rtpChan')?.value || '1');
 const oscAddress = $('#oscAddress')?.value || '/mux' + muxId;
 const oscFormat = $('#oscFormat')?.value || 'float';
 
 const formData=new URLSearchParams();
 formData.append('id', muxId);
 formData.append('sig', sig);
 formData.append('s0', s0);
 formData.append('s1', s1);
 formData.append('s2', s2);
 formData.append('s3', s3);
 formData.append('en', en);
 formData.append('ccBase', rtpCc);
 formData.append('midiChan', rtpChan);
 formData.append('oscBase', oscAddress);
 formData.append('min', min);
 formData.append('max', max);
 formData.append('oscFormat', oscFormat);
 formData.append('filterIntensity', filterIntensity);
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
   const def = typeof getComponentDefinition === 'function' ? getComponentDefinition(role) : null;
   if(def && def.isComplex){
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

/**
 * Obtient les pins disponibles filtrées par type
 * @param {number} pinType - Type de pin (0=PIN_ANALOG, 1=PIN_DIGITAL, 2=PIN_ANALOG_OR_DIGITAL, 3=PIN_PWM)
 * @param {Array} excludeGpios - Liste des GPIOs à exclure (optionnel)
 * @returns {Array} Liste des pins disponibles
 */
function getPinsByType(pinType, excludeGpios = []) {
 if(!caps || !caps.pins) return [];
 
 const excludeSet = new Set(excludeGpios);
 
 return caps.pins.filter(pin => {
  // Exclure les pins déjà utilisées
  if(excludeSet.has(parseInt(pin.gpio))) return false;
  
  // Filtrer selon pinType
  switch(pinType) {
   case 0: // PIN_ANALOG
    return pin.caps && pin.caps.adc === true;
   case 1: // PIN_DIGITAL
    return true; // Toutes les pins peuvent être digitales
   case 2: // PIN_ANALOG_OR_DIGITAL
    return true; // Toutes les pins
   case 3: // PIN_PWM
    return pin.caps && pin.caps.pwm === true;
   default:
    return false;
  }
 });
}

/**
 * Génère dynamiquement les champs pour les pins additionnelles (composants complexes)
 * @param {Object} def - Définition du composant depuis le backend
 * @param {string} containerId - ID du conteneur (ex: "componentFormCard")
 * @param {Object} currentCfg - Configuration actuelle (optionnel)
 */
function generateAdditionalPins(def, containerId, currentCfg = {}) {
 const container = $('#' + containerId);
 if(!container || !def || !def.additionalPins || !Array.isArray(def.additionalPins) || def.additionalPins.length === 0) {
  return;
 }
 
 // Créer une section pour les pins additionnelles
 const section = document.createElement('div');
 section.className = 'f';
 section.style.marginTop = '20px';
 
 const sectionTitle = document.createElement('h4');
 sectionTitle.style.marginTop = '0';
 sectionTitle.textContent = 'Configuration des pins';
 section.appendChild(sectionTitle);
 
 // Obtenir les GPIOs déjà utilisées (pour exclusion)
 const usedGpios = typeof getUsedGpios === 'function' ? getUsedGpios() : [];
 const currentPinLabel = $('#selPin')?.textContent || '';
 const currentPin = caps?.pins?.find(p => p.label === currentPinLabel);
 const currentPinGpio = currentPin ? parseInt(currentPin.gpio) : null;
 
 def.additionalPins.forEach(additionalPin => {
  if(!additionalPin.id) return;
  
  const wrapper = document.createElement('div');
  wrapper.className = 'f';
  
  const label = document.createElement('label');
  label.textContent = additionalPin.displayName || additionalPin.id;
  if(additionalPin.optional) {
   label.textContent += ' (optionnel)';
  }
  wrapper.appendChild(label);
  
  const select = document.createElement('select');
  // ID du champ : préfixe depuis l'ID du composant + id en capital (ex: s0 -> hc4067S0, en -> hc4067En)
  const prefix = def.id ? def.id : 'comp';
  const fieldId = prefix + additionalPin.id.charAt(0).toUpperCase() + additionalPin.id.slice(1);
  select.id = fieldId;
  select.style.width = '200px';
  
  // Ajouter option "Non connecté" pour les pins optionnelles
  if(additionalPin.optional) {
   const optNone = document.createElement('option');
   optNone.value = '255';
   optNone.textContent = 'Non connecté';
   select.appendChild(optNone);
  }
  
  // Remplir avec les pins disponibles selon pinType
  const pinType = additionalPin.pinType !== undefined ? additionalPin.pinType : 1; // Défaut: PIN_DIGITAL
  const availablePins = getPinsByType(pinType, usedGpios);
  
  availablePins.forEach(pin => {
   // Ne pas inclure la pin principale (si elle est digitale)
   if(currentPinGpio && parseInt(pin.gpio) === currentPinGpio && pinType === 1) {
    return; // Skip
   }
   
   const option = document.createElement('option');
   option.value = pin.gpio;
   option.textContent = pin.label + ' (GPIO' + pin.gpio + ')';
   select.appendChild(option);
  });
  
  // Pré-remplir avec currentCfg ou defaultValue
  const cfgValue = currentCfg[fieldId] || currentCfg[additionalPin.id];
  if(cfgValue !== undefined && cfgValue !== null && cfgValue !== '') {
   select.value = cfgValue.toString();
  } else if(additionalPin.defaultValue !== undefined && additionalPin.defaultValue !== null) {
   select.value = additionalPin.defaultValue.toString();
  } else if(additionalPin.optional) {
   select.value = '255'; // Non connecté par défaut pour optionnel
  }
  
  wrapper.appendChild(select);
  section.appendChild(wrapper);
 });
 
 container.appendChild(section);
}

/**
 * Génère dynamiquement la section RTP-MIDI depuis les définitions du backend
 * @param {Object} def - Définition du composant
 * @param {Object} currentCfg - Configuration actuelle
 * @param {string} containerId - ID du conteneur (optionnel, défaut: "rtpMidiSection")
 */
function generateRtpMidiSection(def, currentCfg = {}, containerId = 'rtpMidiSection') {
 const container = $('#' + containerId);
 if(!container) {
  console.warn('[generateRtpMidiSection] Conteneur non trouvé:', containerId);
  return;
 }
 
 // Vider le conteneur
 container.innerHTML = '';
 
 // Si le composant ne supporte pas MIDI, ne rien afficher
 if(!def || !def.supportsMidi || !def.midiMessages || def.midiMessages.length === 0) {
  return;
 }
 
 // Créer le wrapper principal
 const wrapper = document.createElement('div');
 wrapper.className = 'r switch';
 
 // Checkbox rtpEnabled
 const rtpEnabledCheckbox = document.createElement('input');
 rtpEnabledCheckbox.type = 'checkbox';
 rtpEnabledCheckbox.id = 'rtpEnabled2';
 if(currentCfg.rtpEnabled) rtpEnabledCheckbox.checked = true;
 
 const rtpEnabledLabel = document.createElement('label');
 rtpEnabledLabel.setAttribute('for', 'rtpEnabled2');
 rtpEnabledLabel.textContent = '{{t.pins.activate}}'; // TODO: utiliser traduction
 
 const typeLabel = document.createElement('label');
 typeLabel.textContent = '{{t.pins.type}}:'; // TODO: utiliser traduction
 
 // Select rtpMsgType
 const rtpMsgTypeSelect = document.createElement('select');
 rtpMsgTypeSelect.id = 'rtpMsgType';
 
 // Ajouter les options depuis midiMessages
 def.midiMessages.forEach(msg => {
  const option = document.createElement('option');
  option.value = msg.displayName;
  option.textContent = msg.displayName;
  rtpMsgTypeSelect.appendChild(option);
 });
 
 // Sélectionner la valeur actuelle si disponible
 if(currentCfg.rtpType) {
  rtpMsgTypeSelect.value = currentCfg.rtpType;
 }
 
 wrapper.appendChild(rtpEnabledCheckbox);
 wrapper.appendChild(rtpEnabledLabel);
 wrapper.appendChild(typeLabel);
 wrapper.appendChild(rtpMsgTypeSelect);
 container.appendChild(wrapper);
 
 // Créer le conteneur pour les paramètres
 const paramsContainer = document.createElement('div');
 paramsContainer.id = 'rtpParams';
 paramsContainer.className = 'subcard';
 paramsContainer.style.display = rtpEnabledCheckbox.checked ? 'block' : 'none';
 container.appendChild(paramsContainer);
 
 // Gérer l'affichage/masquage des paramètres selon rtpEnabled
 rtpEnabledCheckbox.addEventListener('change', () => {
  paramsContainer.style.display = rtpEnabledCheckbox.checked ? 'block' : 'none';
  if(rtpEnabledCheckbox.checked) {
   updateRtpParamsVisibility();
  }
 });
 
 // Générer les champs de paramètres selon le type de message MIDI
 generateRtpParams(def, paramsContainer, currentCfg);
 
 // Gérer le changement de type de message
 rtpMsgTypeSelect.addEventListener('change', () => {
  updateRtpParamsVisibility();
 });
}

/**
 * Génère les champs de paramètres RTP-MIDI selon le type de message
 * @param {Object} def - Définition du composant
 * @param {HTMLElement} container - Conteneur pour les paramètres
 * @param {Object} currentCfg - Configuration actuelle
 */
function generateRtpParams(def, container, currentCfg = {}) {
 // Vider le conteneur
 container.innerHTML = '';
 
 if(!def || !def.midiMessages || def.midiMessages.length === 0) {
  return;
 }
 
 // Collecter tous les paramètres uniques de tous les messages MIDI
 const allParams = new Map();
 
 def.midiMessages.forEach(msg => {
  if(msg.params && Array.isArray(msg.params)) {
   msg.params.forEach(param => {
    if(!allParams.has(param.id)) {
     // Stocker le paramètre avec le displayName du message pour la visibilité
     allParams.set(param.id, {
      ...param,
      _showFor: [msg.displayName]
     });
    } else {
     // Si le paramètre existe déjà, ajouter ce message à _showFor
     const existing = allParams.get(param.id);
     if(!existing._showFor.includes(msg.displayName)) {
      existing._showFor.push(msg.displayName);
     }
    }
   });
  }
 });
 
 // Générer les champs pour chaque paramètre unique
 allParams.forEach((param, paramId) => {
  const row = document.createElement('div');
  row.className = 'r';
  row.id = param.id + 'Row';
  row.style.display = 'none';
  
  // Convertir le type numérique en string
  const fieldType = param.type === 0 ? 'text' : 
                    param.type === 1 ? 'number' : 
                    param.type === 2 ? 'select' : 
                    param.type === 3 ? 'checkbox' : 
                    param.type === 4 ? 'range' : 
                    param.type === 5 ? 'info' : 'text';
  
  if(fieldType === 'info') {
   const hintDiv = document.createElement('div');
   hintDiv.className = param.hintClass || 'hint';
   hintDiv.textContent = param.hint || '';
   row.appendChild(hintDiv);
  } else if(fieldType === 'range') {
   const label = document.createElement('label');
   label.textContent = param.label || '';
   row.appendChild(label);
   
   const inputMin = document.createElement('input');
   inputMin.type = 'number';
   inputMin.id = param.id + 'Min';
   inputMin.min = param.min || 0;
   inputMin.max = param.max || 127;
   inputMin.placeholder = param.defaultMin || param.min || 0;
   if(param.width) inputMin.style.width = param.width + 'px';
   if(currentCfg[param.id + 'Min'] !== undefined) {
    inputMin.value = currentCfg[param.id + 'Min'];
   } else if(param.defaultMin) {
    inputMin.value = param.defaultMin;
   }
   
   const separator = document.createElement('span');
   separator.textContent = param.separator || '→';
   separator.style.margin = '0 4px';
   
   const inputMax = document.createElement('input');
   inputMax.type = 'number';
   inputMax.id = param.id + 'Max';
   inputMax.min = param.min || 0;
   inputMax.max = param.max || 127;
   inputMax.placeholder = param.defaultMax || param.max || 127;
   if(param.width) inputMax.style.width = param.width + 'px';
   if(currentCfg[param.id + 'Max'] !== undefined) {
    inputMax.value = currentCfg[param.id + 'Max'];
   } else if(param.defaultMax) {
    inputMax.value = param.defaultMax;
   }
   
   row.appendChild(inputMin);
   row.appendChild(separator);
   row.appendChild(inputMax);
  } else {
   const label = document.createElement('label');
   label.textContent = param.label || '';
   row.appendChild(label);
   
   const input = document.createElement('input');
   input.type = fieldType;
   input.id = param.id;
   if(fieldType === 'number') {
    input.min = param.min || 0;
    input.max = param.max || 127;
   }
   input.placeholder = param.placeholder || '';
   if(param.width) input.style.width = param.width + 'px';
   if(currentCfg[param.id] !== undefined) {
    input.value = currentCfg[param.id];
   } else if(param.defaultValue) {
    input.value = param.defaultValue;
   } else if(param.placeholder && fieldType === 'number') {
    input.value = param.placeholder;
   }
   
   row.appendChild(input);
  }
  
  // Stocker les informations pour updateRtpParamsVisibility
  row._showFor = param._showFor || [];
  // Parser dependsOnRole si c'est une string JSON, sinon utiliser directement
  if(param.dependsOnRole) {
   try {
    row._dependsOnRole = typeof param.dependsOnRole === 'string' ? JSON.parse(param.dependsOnRole) : param.dependsOnRole;
   } catch(e) {
    row._dependsOnRole = null;
   }
  } else {
   row._dependsOnRole = null;
  }
  
  container.appendChild(row);
 });
 
 // Mettre à jour la visibilité initiale
 updateRtpParamsVisibility();
}
