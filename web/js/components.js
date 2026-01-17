/* Fonctions de gestion des composants et configurations */

/* Helpers pour éviter les patterns répétés */
function getComponentDef(roleId) {
 return typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.getById
   ? ComponentDefinitions.getById(roleId)
   : (typeof getComponentDefinition === 'function' ? getComponentDefinition(roleId) : null);
}

function getDefsCache() {
 return typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.cache ? ComponentDefinitions.cache : (typeof componentDefinitions !== 'undefined' ? componentDefinitions : []);
}

function migrateRoleValue(role) {
 return typeof migrateRole === 'function' ? migrateRole(role) : role;
}

/**
 * Affiche la carte de configuration correspondant au rôle sélectionné
 * Utilise les définitions du backend pour déterminer le cardId
 * @param {string} role - ID du rôle (ex: "potentiometer", "hc4067")
 */
function showRoleCards(role, currentCfg = {}){
 /* Utiliser un seul conteneur générique pour tous les composants */
 const card = $('#componentFormCard');
 if(!card) {
  console.warn('[showRoleCards] Conteneur componentFormCard non trouvé');
  return;
 }
 
 /* Masquer et vider le conteneur par défaut */
 card.style.display = 'none';
 card.innerHTML = '';
 
 if(!role) return;
 
 const migratedRole = migrateRoleValue(role);
 const def = getComponentDef(migratedRole);
 if(!def) {
  console.warn('[showRoleCards] Définition non trouvée pour:', migratedRole);
  return;
 }
 
 /* Afficher le conteneur */
 card.style.display = 'block';
 
/* Générer les champs de formulaire dynamiquement */
if(def.formFields && Array.isArray(def.formFields) && def.formFields.length > 0) {
 if(typeof FormGenerator !== 'undefined' && FormGenerator.generateFormFields) {
  FormGenerator.generateFormFields(def, 'componentFormCard', currentCfg);
 }
}

/* Générer les pins additionnelles si composant complexe */
if(def.isComplex && def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0) {
 if(typeof FormGenerator !== 'undefined' && FormGenerator.generateAdditionalPins) {
  FormGenerator.generateAdditionalPins(def, 'componentFormCard', currentCfg);
 }
}
 
/* Générer la section MIDI dynamiquement */
if(typeof MidiConfig !== 'undefined' && MidiConfig.generateMessageSection) {
 MidiConfig.generateMessageSection(def, currentCfg, 'rtpMidiSection');
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
 
 const defsCache = getDefsCache();
 console.log('[populateFamilySelect] pinType=', pinType, 'defsCache.length=', defsCache.length);
 
 if(pinType === null || !defsCache || defsCache.length === 0) {
  console.log('[populateFamilySelect] Pas de définitions, menu vide');
  setOptions(familySel, {}, 0);
  return;
 }
 
 /* Obtenir les composants compatibles avec le type de pin (implémentés uniquement pour les familles) */
 const compatibleComponents = (typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.getForPinType)
  ? ComponentDefinitions.getForPinType(pinType, true)
  : (typeof getComponentsForPinType === 'function' ? getComponentsForPinType(pinType, true) : []);
 
 console.log('[populateFamilySelect] compatibleComponents:', compatibleComponents.length);
 
 /* Grouper par famille */
 const familiesMap = new Map();
 compatibleComponents.forEach(def => {
  /* getComponentsForPinType avec implementedOnly=true a déjà filtré, mais on garde pour sécurité */
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
  
  /* Vérifier disponibilité pour les composants complexes */
  if(def.isComplex && pinType === 0) { /* Composant complexe sur pin analogique */
   const complexAvailable = pin && typeof GpioManager !== 'undefined' && GpioManager.areAddressPinsAvailable
     ? GpioManager.areAddressPinsAvailable(pin.gpio)
     : false;
   if(complexAvailable) {
    familiesMap.get(familyId).components.push(def);
   }
  } else {
   familiesMap.get(familyId).components.push(def);
  }
 });
 
 /* Créer les options pour le menu Famille */
 const familyOptions = {};
 Array.from(familiesMap.values()).forEach(fam => {
  if(fam.components.length > 0) {
   familyOptions[fam.id] = fam.name;
   console.log(`[populateFamilySelect] Famille ${fam.id} (${fam.name}): ${fam.components.length} composants`);
  }
 });
 
 console.log('[populateFamilySelect] Options famille:', familyOptions);
 setOptions(familySel, familyOptions, 0);
 
 /* Sélectionner automatiquement la première famille si aucune n'est sélectionnée */
 if(!familySel.value && Object.keys(familyOptions).length > 0) {
  const firstFamilyId = parseInt(Object.keys(familyOptions)[0]);
  familySel.value = firstFamilyId;
 }
 
 /* Déclencher le filtrage du menu Composant */
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
 
 const defsCache = getDefsCache();
 if(pinType === null || !defsCache || defsCache.length === 0) {
  console.log('[populateComponentSelect] Pas de définitions, menu vide');
  setOptions(compSel, {}, 0);
  return;
 }
 
 /* Obtenir TOUS les composants compatibles (y compris non implémentés) */
 const compatibleComponents = typeof getComponentsForPinType === 'function' 
  ? getComponentsForPinType(pinType, false) 
  : [];
 
 console.log('[populateComponentSelect] compatibleComponents:', compatibleComponents.length);
 
 /* Filtrer par famille */
 const familyComponents = compatibleComponents.filter(def => {
  const defFamilyId = def.family !== undefined ? def.family : 0;
  return defFamilyId === familyId;
 });
 
 console.log('[populateComponentSelect] familyComponents (familyId=' + familyId + '):', familyComponents.length, familyComponents.map(d => `${d.id} (implemented=${d.implemented})`));
 
 /* Créer les options pour le menu Composant */
 const options = {};
 familyComponents.forEach(def => {
  /* Vérifier disponibilité pour les composants complexes */
  if(def.isComplex && pinType === 0) { /* Composant complexe sur pin analogique */
   const complexAvailable = pin && typeof GpioManager !== 'undefined' && GpioManager.areAddressPinsAvailable
     ? GpioManager.areAddressPinsAvailable(pin.gpio)
     : false;
   /* Afficher même si non disponible si non implémenté (pour info) */
   if(complexAvailable || !def.implemented) {
    options[def.id] = {
     label: def.displayName,
     disabled: !def.implemented || !complexAvailable
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
 
 /* Vérifier si les définitions sont chargées, sinon les charger */
 const defsCacheCheck = getDefsCache();
 if(!defsCacheCheck || defsCacheCheck.length === 0) {
  /* Charger les définitions de manière asynchrone et réessayer */
  loadComponentDefinitions().then(() => {
   /* Réessayer après chargement */
   updFunc(lbl);
  }).catch(err => {
   console.warn('Erreur chargement définitions dans updFunc:', err);
  });
  return;
 }
 
/* Plus besoin de vider manuellement, les champs sont générés dynamiquement */
 
 /* Utiliser pType() qui utilise les données du backend */
 const type = typeof pType === 'function' ? pType(lbl) : 'digital';
 const pin = caps?.pins?.find(p => p.label === lbl);
 
 console.log('[updFunc] Pin:', lbl, 'type:', type, 'pin:', pin);
 
 /* Gérer les bus (I2C, SPI, UART) - rôles spéciaux */
 if(type === 'i2c' || type === 'spi' || type === 'uart') {
  const def = getComponentDef(type);
  const displayName = def ? def.displayName : type.toUpperCase();
  const options = {};
  options[type] = displayName;
  setOptions(sel, options, 0);
  setOptions(familySel, {}, 0);
  showRoleCards(sel.value || '');
  updateRtpForRole(sel.value || '');
  return;
 }
 
 /* Convertir le type de pin en pinType numérique pour le filtre */
 let pinType = null;
 if(type === 'analog') {
  pinType = 0; /* PIN_ANALOG */
 } else if(type === 'digital') {
  pinType = 1; /* PIN_DIGITAL (toujours, peu importe si PWM ou pas) */
 }
 
 console.log('[updFunc] pinType calculé:', pinType);
 
 /* Remplir le menu Famille (qui déclenchera le remplissage du menu Composant) */
 populateFamilySelect(pinType, pin);
 
 /* Si une valeur était déjà sélectionnée, essayer de la restaurer APRÈS que les menus soient remplis */
 const currentRole = pcfg[lbl]?.role;
 if(currentRole) {
  /* Utiliser setTimeout pour s'assurer que les menus sont remplis */
  setTimeout(() => {
   const migratedRoleVal = migrateRoleValue(currentRole);
   const def = getComponentDef(migratedRoleVal);
   if(def) {
    /* Sélectionner la bonne famille */
    const defFamilyId = def.family !== undefined ? def.family : 0;
    if(familySel.value != defFamilyId) {
     familySel.value = defFamilyId;
     populateComponentSelect(defFamilyId, pinType, pin);
     /* Attendre que le menu Composant soit rempli avant de sélectionner */
     setTimeout(() => {
      if(sel.value != migratedRoleVal) {
       sel.value = migratedRoleVal;
       const currentCfg = pcfg && pcfg[lbl] ? pcfg[lbl] : {};
       showRoleCards(migratedRoleVal, currentCfg);
       updateRtpForRole(migratedRoleVal);
       if(typeof MidiConfig !== 'undefined' && MidiConfig.updateVisibility) {
        MidiConfig.updateVisibility();
       }
       /* Si composant complexe, initialiser le formulaire */
      if(def && def.isComplex && typeof ComplexComponents !== 'undefined' && ComplexComponents.initForm) {
       ComplexComponents.initForm(lbl);
       }
      }
     }, 0);
    } else {
     /* Famille déjà sélectionnée, juste sélectionner le composant */
     setTimeout(() => {
      if(sel.value != migratedRoleVal) {
       sel.value = migratedRoleVal;
       const currentCfg = pcfg && pcfg[lbl] ? pcfg[lbl] : {};
       showRoleCards(migratedRoleVal, currentCfg);
       updateRtpForRole(migratedRoleVal);
       if(typeof MidiConfig !== 'undefined' && MidiConfig.updateVisibility) {
        MidiConfig.updateVisibility();
       }
       /* Si composant complexe, initialiser le formulaire */
      if(def && def.isComplex && typeof ComplexComponents !== 'undefined' && ComplexComponents.initForm) {
       ComplexComponents.initForm(lbl);
       }
      }
     }, 0);
    }
   }
  }, 0);
 } else {
  /* Pas de configuration existante : attendre que les menus soient remplis, puis sélectionner le premier composant par défaut */
  setTimeout(() => {
   /* Vérifier que les menus sont remplis */
   if(familySel.value && sel.options.length > 0) {
    /* Le premier composant est déjà sélectionné par setOptions avec pre=0 */
    const firstComponentId = sel.value;
    if(firstComponentId) {
     showRoleCards(firstComponentId, {});
     updateRtpForRole(firstComponentId);
     if(typeof MidiConfig !== 'undefined' && MidiConfig.updateVisibility) {
      MidiConfig.updateVisibility();
     }
     /* Si composant complexe, initialiser le formulaire */
     const firstDef = getComponentDef(firstComponentId);
     if(firstDef && firstDef.isComplex && typeof ComplexComponents !== 'undefined' && ComplexComponents.initForm) {
      ComplexComponents.initForm(lbl);
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
if(typeof MidiConfig !== 'undefined' && MidiConfig.updateVisibility) {
 MidiConfig.updateVisibility();
}
/* Si composant complexe est sélectionné, initialiser le formulaire */
const role = sel.value || '';
const migratedRole = migrateRoleValue(role);
const def = getComponentDef(migratedRole);
if(def && def.isComplex && cur && typeof ComplexComponents !== 'undefined' && ComplexComponents.initForm) {
 ComplexComponents.initForm(cur);
} else {
  /* Plus besoin d'effacer manuellement, les champs sont générés dynamiquement */
 }
 if(cur){
  pcfg[cur]=readCfg();
  updatePinsList();
  updateBusVisuals();
 }
 };
 
 /* Retirer l'ancien handler pour éviter les doublons */
 const oldHandler = familySel._onchangeHandler;
 if(oldHandler) {
  familySel.removeEventListener('change', oldHandler);
 }
 
 /* Gérer le changement de famille */
 const newHandler = () => {
  const familyId = parseInt(familySel.value);
  if(!isNaN(familyId)) {
   populateComponentSelect(familyId, pinType, pin);
   /* Réinitialiser la sélection du composant */
   sel.value = '';
   showRoleCards('');
   updateRtpForRole('');
   if(typeof MidiConfig !== 'undefined' && MidiConfig.updateVisibility) {
    MidiConfig.updateVisibility();
   }
   /* Plus besoin d'effacer manuellement, les champs sont générés dynamiquement */
  }
 };
 familySel.addEventListener('change', newHandler);
 familySel._onchangeHandler = newHandler; /* Stocker pour pouvoir le retirer plus tard */
 
 sel.onchange=updateConfig;
 
 /* Générer la liste d'inputs dynamiquement */
 const inputs = getAllFieldIds();
 inputs.forEach(id=>{
 const el=$(id);
 if(el) el.addEventListener('change',updateConfig);
 if(el) el.addEventListener('input',updateConfig);
 });
}

function updateRtpForRole(role){
 /* Migrer les anciens formats si nécessaire */
 const migratedRole = migrateRoleValue(role || '');
 const def = getComponentDef(migratedRole);
 
 /* Mettre à jour la visibilité des paramètres MIDI si nécessaire */
 if(def && def.supportsMidi && def.midiMessages && def.midiMessages.length > 0) {
  if(typeof MidiConfig !== 'undefined' && MidiConfig.updateVisibility) {
   MidiConfig.updateVisibility();
  }
 }
}


/* updateBtnPulseTimingVisibility() est maintenant obsolète */
/* L'affichage conditionnel est géré par generateFormFields() via dependsOn/showWhen */

/**
 * Collecte tous les IDs de champs depuis les formFields et les champs RTP
 * @returns {Array<string>} Liste des IDs de champs
 */
function getAllFieldIds() {
 const ids = [];
 
 /* Collecter depuis les formFields de tous les composants */
 const defsCache = getDefsCache();
 if(defsCache && defsCache.length > 0) {
  defsCache.forEach(def => {
   if(def.formFields && Array.isArray(def.formFields)) {
    def.formFields.forEach(field => {
     if(field.id && !field.id.startsWith('_')) { /* Ignorer les hints standalone */
      ids.push('#' + field.id);
      /* Pour RANGE, ajouter Min et Max */
      if(field.type === 4) { /* RANGE */
       ids.push('#' + field.id + 'Min');
       ids.push('#' + field.id + 'Max');
      }
     }
    });
   }
  });
 
 /* Collecter les paramètres MIDI depuis toutes les définitions */
  defsCache.forEach(def => {
   if(def.midiMessages && Array.isArray(def.midiMessages)) {
    def.midiMessages.forEach(msg => {
     if(msg.params && Array.isArray(msg.params)) {
      msg.params.forEach(param => {
       if(param.id) {
        const paramId = '#' + param.id;
        if(!ids.includes(paramId)) ids.push(paramId);
        /* Pour RANGE, ajouter Min et Max */
        if(param.type === 4) { /* RANGE */
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
 
 /* Ajouter les champs OSC et Debug */
 ids.push('#oscEnabled2', '#oscAddress', '#oscFormat', '#dbgEnabled', '#dbgHeader');
 
 /* Collecter les IDs des additionalPins depuis toutes les définitions */
  defsCache.forEach(def => {
   if(def.isComplex && def.additionalPins && Array.isArray(def.additionalPins)) {
    def.additionalPins.forEach(additionalPin => {
     if(additionalPin.id) {
      /* ID du champ : préfixe depuis l'ID du composant + id en capital (ex: s0 -> hc4067S0) */
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
 
 /* Lire les champs depuis les formFields du composant actuel */
 const roleSel = $('#funcSelect');
 let migratedRole = null;
 let def = null;
 if(roleSel && roleSel.value) {
  migratedRole = migrateRoleValue(roleSel.value);
  def = getComponentDef(migratedRole);
  
  if(def && def.formFields && Array.isArray(def.formFields)) {
   def.formFields.forEach(field => {
    if(field.id && !field.id.startsWith('_')) {
     const el = $('#' + field.id);
     if(el) {
      if(field.type === 3) { /* CHECKBOX */
       c[field.id] = el.checked;
      } else if(field.type === 4) { /* RANGE */
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
 
 /* Lire les champs MIDI (utiliser MidiConfig.readConfig) */
 if(migratedRole && def && typeof MidiConfig !== 'undefined' && MidiConfig.readConfig) {
  const midiConfig = MidiConfig.readConfig(def);
  Object.assign(c, midiConfig);
 }
 
 /* Lire les champs OSC et Debug */
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
 
 const migratedRole = migrateRoleValue(c.role);
 const def = migratedRole ? getComponentDef(migratedRole) : null;
 
 /* Restaurer la famille si le rôle est défini */
 if(def && def.family !== undefined && $('#familySelect')) {
  $('#familySelect').value = def.family;
 }
 
setV('funcSelect',migratedRole);
showRoleCards(migratedRole, c);
updateRtpForRole(migratedRole);
 
 /* Appliquer les champs depuis les formFields du composant */
 if(def && def.formFields && Array.isArray(def.formFields)) {
  def.formFields.forEach(field => {
   if(field.id && !field.id.startsWith('_')) {
    if(field.type === 3) { /* CHECKBOX */
     setC(field.id, c[field.id]);
    } else if(field.type === 4) { /* RANGE */
     setV(field.id + 'Min', c[field.id + 'Min']);
     setV(field.id + 'Max', c[field.id + 'Max']);
    } else {
     setV(field.id, c[field.id]);
    }
   }
  });
 }
 
 /* Appliquer les champs MIDI (utiliser MidiConfig.applyConfig) */
 if(def && typeof MidiConfig !== 'undefined' && MidiConfig.applyConfig) {
  MidiConfig.applyConfig(c, def);
 }
 
 /* Appliquer les champs OSC et Debug */
 setC('oscEnabled2',c.oscEnabled);
 setV('oscAddress',c.oscAddress);
 setV('oscFormat',c.oscFormat);
 setC('dbgEnabled',c.dbgEnabled);
 setV('dbgHeader',c.dbgHeader);
}

/* Toutes les fonctions dépréciées ont été supprimées - utiliser directement les modules */
