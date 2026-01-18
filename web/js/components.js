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
 
 const hasAdditionalPins = def.additionalPinCount > 0 || (def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0);
 console.log('[showRoleCards] Définition trouvée, def.id:', def.id, 'hasAdditionalPins:', hasAdditionalPins, 'additionalPinCount:', def.additionalPinCount, 'additionalPins:', def.additionalPins, 'type:', typeof def.additionalPins, 'isArray:', Array.isArray(def.additionalPins));

/* Afficher le conteneur */
card.style.display = 'block';

/* Générer les champs de formulaire dynamiquement */
if(def.formFields && Array.isArray(def.formFields) && def.formFields.length > 0) {
 if(typeof FormGenerator !== 'undefined' && FormGenerator.generateFormFields) {
  FormGenerator.generateFormFields(def, 'componentFormCard', currentCfg);
 }
}

/* Générer les pins additionnelles si composant avec additionalPins */
if(hasAdditionalPins && def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0) {
 console.log('[showRoleCards] Appel generateAdditionalPins, def.id:', def.id, 'additionalPins.length:', def.additionalPins.length);
 if(typeof FormGenerator !== 'undefined' && FormGenerator.generateAdditionalPins) {
  FormGenerator.generateAdditionalPins(def, 'componentFormCard', currentCfg);
  console.log('[showRoleCards] generateAdditionalPins appelé');
 } else {
  console.warn('[showRoleCards] FormGenerator.generateAdditionalPins non disponible');
 }
} else {
 console.log('[showRoleCards] Pas de additionalPins, hasAdditionalPins:', hasAdditionalPins, 'additionalPins:', def.additionalPins);
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
  
  /* Vérifier disponibilité pour les composants avec additionalPins */
  const hasAdditionalPins = def.additionalPinCount > 0 || (def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0);
  if(hasAdditionalPins && pinType === 0) { /* Composant avec additionalPins sur pin analogique */
   let complexAvailable = false;
   if(pin && pin.gpio !== undefined && typeof GpioManager !== 'undefined' && typeof GpioManager.areAddressPinsAvailable === 'function') {
    try {
     const gpio = parseInt(pin.gpio);
     if(!isNaN(gpio)) {
      complexAvailable = GpioManager.areAddressPinsAvailable(gpio);
     }
    } catch(err) {
     console.warn('[populateFamilySelect] Erreur vérification disponibilité composant avec additionalPins:', err);
     complexAvailable = false;
    }
   }
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
 
 /* Déclencher le filtrage du menu Composant */
 const selectedFamilyId = familySel.value ? parseInt(familySel.value) : null;
 
 /* Sélectionner automatiquement la première famille si aucune n'est sélectionnée */
 /* Mettre le flag AVANT setOptions pour éviter que le handler change vide le select Component */
 if(!familySel.value && Object.keys(familyOptions).length > 0) {
  familySel._restoringConfig = true;
 }
 
 /* Maintenant remplir le select (peut déclencher le handler change, mais le flag protège) */
 setOptions(familySel, familyOptions, 0);
 
 /* Si on a mis le flag, le réinitialiser après un délai */
 if(familySel._restoringConfig) {
  const firstFamilyId = parseInt(Object.keys(familyOptions)[0]);
  setTimeout(() => {
   familySel._restoringConfig = false;
  }, 10);
  /* Déclencher manuellement populateComponentSelect pour la première famille */
  if(selectedFamilyId === null) {
   populateComponentSelect(firstFamilyId, pinType, pin);
   return;
  }
 }
 
 /* Déclencher le filtrage du menu Composant si une famille était déjà sélectionnée */
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
 
 /* Trier les composants pour toujours mettre "button" en premier s'il est disponible */
 const sortedFamilyComponents = [...familyComponents].sort((a, b) => {
  // Mettre "button" en premier s'il est disponible
  if(a.id === 'button' && b.id !== 'button') return -1;
  if(b.id === 'button' && a.id !== 'button') return 1;
  // Sinon garder l'ordre original
  return 0;
 });
 
 /* Créer les options pour le menu Composant */
 const options = {};
 sortedFamilyComponents.forEach(def => {
  /* Vérifier disponibilité pour les composants avec additionalPins */
  const hasAdditionalPins = def.additionalPinCount > 0 || (def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0);
  if(hasAdditionalPins && pinType === 0) { /* Composant avec additionalPins sur pin analogique */
   let complexAvailable = false;
   if(pin && pin.gpio !== undefined && typeof GpioManager !== 'undefined' && typeof GpioManager.areAddressPinsAvailable === 'function') {
    try {
     const gpio = parseInt(pin.gpio);
     if(!isNaN(gpio)) {
      complexAvailable = GpioManager.areAddressPinsAvailable(gpio);
     }
    } catch(err) {
     console.warn('[populateComponentSelect] Erreur vérification disponibilité composant avec additionalPins:', err);
     complexAvailable = false;
    }
   }
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
 
const pin = caps?.pins?.find(p => p?.label === lbl) || null;
 
 let pinType = null;
 if(lbl?.startsWith('A')) {
  pinType = 0;
 } else if(lbl?.startsWith('D')) {
  pinType = 1;
 } else {
  return; // Bus purs non implémentés
 }
 
 console.log('[updFunc] pinType calculé:', pinType);
 
 /* Remplir le menu Famille (qui déclenchera le remplissage du menu Composant) */
 populateFamilySelect(pinType, pin);
 
 /* Si une valeur était déjà sélectionnée, essayer de la restaurer APRÈS que les menus soient remplis */
 let currentRole = pcfg[lbl]?.role;
 
 // Vérifier si cette pin est utilisée par un composant complexe sauvegardé
 /* Chercher dans pcfg */
 if(!currentRole && pin && pin.gpio !== undefined && typeof pcfg !== 'undefined' && pcfg) {
  const sigGpio = parseInt(pin.gpio);
  if(isNaN(sigGpio)) return;
  const existingComplexLabel = Object.keys(pcfg).find(lbl => {
   const cfg = pcfg[lbl];
   return cfg && cfg.additionalPins && typeof cfg.additionalPins === 'object' && cfg.additionalPins.sig === sigGpio;
  });
  if(existingComplexLabel) {
   const existingComplex = pcfg[existingComplexLabel];
   if(existingComplex && existingComplex.role) {
    // Utiliser le rôle depuis pcfg
    currentRole = existingComplex.role;
    console.log('[updFunc] Pin utilisée par composant complexe sauvegardé:', existingComplexLabel, 'role:', currentRole);
   }
  }
 }
 
 if(currentRole) {
  /* Utiliser setTimeout pour s'assurer que les menus sont remplis */
  setTimeout(() => {
   const migratedRoleVal = migrateRoleValue(currentRole);
   const def = getComponentDef(migratedRoleVal);
   if(def) {
    /* Sélectionner la bonne famille */
    const defFamilyId = def.family !== undefined ? def.family : 0;
    if(familySel.value != defFamilyId) {
     /* Marquer qu'on est en train de restaurer une configuration AVANT de changer la valeur */
     familySel._restoringConfig = true;
     console.log('[updFunc] Restauration config, changement famille de', familySel.value, 'vers', defFamilyId);
     /* Appeler populateComponentSelect AVANT de changer familySel.value pour éviter le déclenchement du handler */
     populateComponentSelect(defFamilyId, pinType, pin);
     /* Maintenant changer la valeur (le handler sera appelé mais ignorera le vidage grâce au flag) */
     familySel.value = defFamilyId;
     /* Réinitialiser le flag après que tout soit fait */
     setTimeout(() => {
      familySel._restoringConfig = false;
      console.log('[updFunc] Flag _restoringConfig réinitialisé');
     }, 100);
       /* Attendre que le menu Composant soit rempli avant de sélectionner */
       setTimeout(() => {
        if(sel.value != migratedRoleVal) {
         sel.value = migratedRoleVal;
         /* Charger depuis pcfg */
         let currentCfg = pcfg && pcfg[lbl] ? pcfg[lbl] : {};
         showRoleCards(migratedRoleVal, currentCfg);
         updateRtpForRole(migratedRoleVal);
         if(typeof MidiConfig !== 'undefined' && MidiConfig.updateVisibility) {
          MidiConfig.updateVisibility();
         }
         /* Initialiser le formulaire (gère simples et complexes) */
         initComponentForm(lbl);
        }
       }, 0);
    } else {
     /* Famille déjà sélectionnée, juste sélectionner le composant */
     setTimeout(() => {
      if(sel.value != migratedRoleVal) {
       sel.value = migratedRoleVal;
       /* Charger depuis pcfg */
       let currentCfg = pcfg && pcfg[lbl] ? pcfg[lbl] : {};
       showRoleCards(migratedRoleVal, currentCfg);
       updateRtpForRole(migratedRoleVal);
       if(typeof MidiConfig !== 'undefined' && MidiConfig.updateVisibility) {
        MidiConfig.updateVisibility();
       }
       /* Initialiser le formulaire (gère simples et complexes) */
       initComponentForm(lbl);
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
     /* Initialiser le formulaire (gère simples et complexes) */
     initComponentForm(lbl);
    }
   }
  }, 0);
 }
 
const updateConfig=()=>{
 const lbl = $('#selPin')?.textContent || '';
 const currentCfg = pcfg && pcfg[lbl] ? pcfg[lbl] : {};
 const selectedRole = sel.value || '';
 console.log('[updateConfig] Début, lbl:', lbl, 'cur:', cur, 'selectedRole:', selectedRole);
 showRoleCards(selectedRole, currentCfg);
 /* Vérifier si les champs additionalPins sont créés */
 setTimeout(() => {
  const migratedRole = migrateRoleValue(selectedRole);
  const def = getComponentDef(migratedRole);
  const hasAdditionalPins = def && (def.additionalPinCount > 0 || (def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0));
  if(hasAdditionalPins && def.additionalPins) {
   console.log('[updateConfig] Vérification champs additionalPins créés...');
   def.additionalPins.forEach(ap => {
    if(ap.id) {
     const fieldId = FormGenerator.getFieldId(def, ap.id);
     const field = $('#' + fieldId);
     console.log('[updateConfig] Champ', ap.id, 'fieldId:', fieldId, 'trouvé:', !!field, 'value:', field ? field.value : 'N/A');
    }
   });
  }
 }, 50);
 updateRtpForRole(selectedRole);
if(typeof MidiConfig !== 'undefined' && MidiConfig.updateVisibility) {
 MidiConfig.updateVisibility();
}
/* Initialiser le formulaire (gère simples et complexes) - attendre que les champs soient créés */
if(cur) {
 console.log('[updateConfig] Initialisation formulaire pour cur:', cur);
 /* Attendre un peu pour que generateAdditionalPins() crée les champs */
 setTimeout(() => {
  if(cur) {
   initComponentForm(cur);
   /* Attendre que initComponentForm() ait rempli les champs avant de lire la config */
   setTimeout(() => {
    if(cur){
     console.log('[updateConfig] Lecture config pour cur:', cur, 'selectedRole:', selectedRole);
     /* Passer le role explicitement pour éviter les problèmes de timing */
     pcfg[cur]=readCfg(selectedRole);
     console.log('[updateConfig] Config lue, pcfg[cur]:', pcfg[cur]);
     updatePinsList();
     updateBusVisuals();
    } else {
     console.warn('[updateConfig] cur n\'est plus défini après timeout');
    }
   }, 100);
  } else {
   console.warn('[updateConfig] cur n\'est plus défini avant initComponentForm');
  }
 }, 50);
 } else {
  console.warn('[updateConfig] cur n\'est pas défini, lbl:', lbl);
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
  if(isNaN(familyId)) return;
  
  /* Ne pas vider le select Component si on est en train de restaurer une configuration */
  if(familySel._restoringConfig) {
   console.log('[newHandler] Restauration config, pas de vidage du select Component');
   populateComponentSelect(familyId, pinType, pin);
   return;
  }
  
  console.log('[newHandler] Changement famille par utilisateur, vidage select Component');
  populateComponentSelect(familyId, pinType, pin);
  /* Réinitialiser la sélection du composant uniquement si le changement vient de l'utilisateur */
  sel.value = '';
  showRoleCards('');
  updateRtpForRole('');
  if(typeof MidiConfig !== 'undefined' && MidiConfig.updateVisibility) {
   MidiConfig.updateVisibility();
  }
  /* Plus besoin d'effacer manuellement, les champs sont générés dynamiquement */
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
   const hasAdditionalPins = def.additionalPinCount > 0 || (def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0);
   if(hasAdditionalPins && def.additionalPins && Array.isArray(def.additionalPins)) {
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

function readCfg(roleOverride = null){
 const c={};
 /* Utiliser le roleOverride si fourni, sinon lire depuis le select */
 c.role = roleOverride || $('#funcSelect')?.value || '';
 
 console.log('[readCfg] Début, role:', c.role, 'roleOverride:', roleOverride);
 
 /* Lire les champs depuis les formFields du composant actuel */
 let migratedRole = null;
 let def = null;
 if(c.role) {
  migratedRole = migrateRoleValue(c.role);
  def = getComponentDef(migratedRole);
  const hasAdditionalPins = def && (def.additionalPinCount > 0 || (def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0));
  console.log('[readCfg] migratedRole:', migratedRole, 'def trouvée:', !!def, 'hasAdditionalPins:', hasAdditionalPins);
  
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
 
/* Lire les additionalPins si composant avec additionalPins */
const hasAdditionalPinsForRead = def && (def.additionalPinCount > 0 || (def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0));
if(hasAdditionalPinsForRead && def.additionalPins && Array.isArray(def.additionalPins) && typeof FormGenerator !== 'undefined' && FormGenerator.getFieldId) {
  console.log('[readCfg] Lecture additionalPins, def.id:', def.id, 'additionalPins count:', def.additionalPins.length);
  c.additionalPins = {};
  def.additionalPins.forEach(additionalPin => {
   if(!additionalPin.id) return;
   const fieldId = FormGenerator.getFieldId(def, additionalPin.id);
   if(!fieldId) {
    console.warn('[readCfg] FieldId vide pour additionalPin:', additionalPin.id);
    return;
   }
   console.log('[readCfg] Recherche champ, additionalPin.id:', additionalPin.id, 'fieldId calculé:', fieldId);
   const field = $('#' + fieldId);
   if(!field) {
    console.warn('[readCfg] Champ non trouvé:', fieldId, 'Vérification DOM...');
    /* Vérifier si le champ existe avec un autre ID ou dans un autre conteneur */
    const allSelects = document.querySelectorAll('select');
    const selectsInfo = Array.from(allSelects).map(s => ({
     id: s.id || '(sans ID)',
     value: s.value,
     label: s.previousElementSibling?.textContent || s.parentElement?.querySelector('label')?.textContent || '(sans label)',
     parent: s.parentElement?.className || s.parentElement?.id || '(sans parent)'
    }));
    console.log('[readCfg] Selects trouvés dans le DOM:', selectsInfo);
    /* Chercher aussi dans componentFormCard */
    const card = $('#componentFormCard');
    if(card) {
     const cardSelects = card.querySelectorAll('select');
     console.log('[readCfg] Selects dans componentFormCard:', Array.from(cardSelects).map(s => ({id: s.id || '(sans ID)', value: s.value})));
    }
    return;
   }
   if(field.value !== undefined && field.value !== null && field.value !== '') {
    const value = parseInt(field.value);
    if(!isNaN(value)) {
     c.additionalPins[additionalPin.id] = value;
     console.log('[readCfg] additionalPin lu:', additionalPin.id, '=', value, 'depuis fieldId:', fieldId);
    } else {
     console.warn('[readCfg] Valeur non numérique pour', additionalPin.id, ':', field.value);
    }
   } else {
    console.warn('[readCfg] Valeur vide pour', additionalPin.id, 'fieldId:', fieldId);
   }
  });
  /* Lire complexId si présent */
  const idFieldId = FormGenerator.getFieldId(def, 'id');
  if(idFieldId) {
   const idField = $('#' + idFieldId);
   if(idField && idField.value !== undefined && idField.value !== null && idField.value !== '') {
    const complexId = parseInt(idField.value);
    if(!isNaN(complexId)) {
     c.complexId = complexId;
     console.log('[readCfg] complexId lu:', complexId, 'depuis fieldId:', idFieldId);
    }
   } else {
    console.warn('[readCfg] complexId non trouvé ou vide, fieldId:', idFieldId);
   }
  } else {
   console.warn('[readCfg] idFieldId vide pour complexId');
  }
  console.log('[readCfg] additionalPins final:', c.additionalPins, 'complexId:', c.complexId);
 }
 
 return c;
}

/**
 * Version unifiée qui gère aussi les composants complexes via pcfg
 * Applique une configuration dans le formulaire (simples et complexes)
 * @param {Object} c - Configuration depuis pcfg (peut contenir additionalPins)
 */
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
 const hasAdditionalPins = c.additionalPins && typeof c.additionalPins === 'object' && c.additionalPins.sig !== undefined;
 
 /* Restaurer la famille si le rôle est défini */
 if(def && def.family !== undefined && $('#familySelect')) {
  $('#familySelect').value = def.family;
 }
 
setV('funcSelect',migratedRole);
/* Ne pas appeler showRoleCards si le conteneur contient déjà les champs pour ce rôle */
const card = $('#componentFormCard');
const currentRoleInSelect = $('#funcSelect')?.value;
if(currentRoleInSelect !== migratedRole || !card || card.innerHTML.trim() === '') {
 console.log('[applyCfg] Appel showRoleCards, currentRoleInSelect:', currentRoleInSelect, 'migratedRole:', migratedRole, 'card vide:', !card || card.innerHTML.trim() === '');
 showRoleCards(migratedRole, c);
} else {
 console.log('[applyCfg] showRoleCards déjà appelé pour ce rôle, skip pour éviter de vider le conteneur');
}
updateRtpForRole(migratedRole);
 
/* Appliquer les additionalPins si composant avec additionalPins */
const hasAdditionalPinsForApply = def && (def.additionalPinCount > 0 || (def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0));
if(hasAdditionalPins && hasAdditionalPinsForApply && def.additionalPins && typeof FormGenerator !== 'undefined' && FormGenerator.getFieldId) {
  def.additionalPins.forEach(additionalPin => {
   if(!additionalPin.id) return;
   const fieldId = FormGenerator.getFieldId(def, additionalPin.id);
   if(!fieldId) return;
   const value = c.additionalPins[additionalPin.id];
   if(value !== undefined && value !== null) {
    setV(fieldId, value);
   }
  });
  /* Appliquer complexId si présent */
  if(c.complexId !== undefined && def) {
   const idFieldId = FormGenerator.getFieldId(def, 'id');
   if(idFieldId) setV(idFieldId, c.complexId);
  }
 }
 
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

/**
 * Initialise le formulaire d'un composant (simples et complexes)
 * Gère les valeurs par défaut si pas de config dans pcfg
 * @param {string} pinLabel - Label de la pin (ex: "A0")
 */
function initComponentForm(pinLabel) {
 console.log('[initComponentForm] Début, pinLabel:', pinLabel);
 if(typeof caps === 'undefined' || !caps || !caps.pins || !Array.isArray(caps.pins)) {
  console.warn('[initComponentForm] caps ou caps.pins manquant');
  return;
 }
 const pin = caps.pins.find(p => p && p.label === pinLabel);
 if(!pin || pin.gpio === undefined) {
  console.warn('[initComponentForm] Pin non trouvée:', pinLabel);
  return;
 }
 
 const funcSelectValue = $('#funcSelect')?.value || '';
 const migratedRoleValue = migrateRoleValue(funcSelectValue);
 const def = getComponentDef(migratedRoleValue);
 
 if(!def) {
  console.warn('[initComponentForm] Définition non trouvée pour:', migratedRoleValue);
  return;
 }
 const hasAdditionalPinsForInit = def && (def.additionalPinCount > 0 || (def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0));
 console.log('[initComponentForm] Définition trouvée, def.id:', def.id, 'hasAdditionalPins:', hasAdditionalPinsForInit);
 
 /* Vérifier si configuration existe dans pcfg */
 const pcfgEntry = typeof pcfg !== 'undefined' && pcfg[pinLabel] ? pcfg[pinLabel] : null;
 const currentSelectedRole = $('#funcSelect')?.value || '';
 if(pcfgEntry) {
  /* Configuration existe, vérifier si le rôle correspond */
  const pcfgRole = migrateRoleValue(pcfgEntry.role || '');
  console.log('[initComponentForm] Configuration trouvée dans pcfg, pcfgRole:', pcfgRole, 'currentSelectedRole:', currentSelectedRole);
  /* Si le rôle correspond, appliquer la config */
  if(pcfgRole && currentSelectedRole && pcfgRole === currentSelectedRole) {
   applyCfg(pcfgEntry);
   return;
  }
  /* Si le rôle ne correspond pas, continuer pour initialiser avec les valeurs par défaut (nouveau composant sur cette pin) */
  console.log('[initComponentForm] Rôle dans pcfg ne correspond pas au rôle sélectionné, initialisation avec valeurs par défaut');
 }
 
/* Pas de config : initialiser valeurs par défaut pour composants avec additionalPins uniquement */
if(hasAdditionalPinsForInit && def.additionalPins && Array.isArray(def.additionalPins) && typeof FormGenerator !== 'undefined' && FormGenerator.getFieldId && typeof GpioManager !== 'undefined') {
  const sigGpio = parseInt(pin.gpio);
  if(isNaN(sigGpio)) {
   console.warn('[initComponentForm] GPIO invalide pour pin:', pinLabel);
   return;
  }
  const usedGpios = (GpioManager.getUsedGpios && typeof GpioManager.getUsedGpios === 'function') ? GpioManager.getUsedGpios([]) : new Set();
  
  /* Extraire les IDs des pins d'adresse depuis les définitions (au lieu de hardcoder s0, s1, s2, s3) */
  const addressPinIds = def.additionalPins
   .filter(ap => ap.id && ap.pinType === 1 && !ap.optional && ap.id !== 'sig' && ap.id !== 'en')
   .map(ap => ap.id);
  
  console.log('[initComponentForm] addressPinIds extraits depuis définitions:', addressPinIds);
  
 /* Calculer toutes les pins d'adresse en une fois (générique) */
 let calculatedAddressPins = {};
 if(addressPinIds.length > 0 && typeof GpioManager.calculateAddressPins === 'function') {
  calculatedAddressPins = GpioManager.calculateAddressPins(sigGpio, usedGpios, addressPinIds);
  console.log('[initComponentForm] calculatedAddressPins:', calculatedAddressPins, 'type:', typeof calculatedAddressPins, 'keys:', Object.keys(calculatedAddressPins));
 } else {
  console.warn('[initComponentForm] Impossible de calculer les pins d\'adresse, addressPinIds:', addressPinIds, 'calculateAddressPins available:', typeof GpioManager.calculateAddressPins);
 }
  
  /* Initialiser toutes les additionalPins dynamiquement */
  def.additionalPins.forEach(additionalPin => {
   if(!additionalPin.id) return;
   const fieldId = FormGenerator.getFieldId(def, additionalPin.id);
   const field = fieldId ? $('#' + fieldId) : null;
   if(!field) {
    console.warn('[initComponentForm] Champ non trouvé pour additionalPin:', additionalPin.id, 'fieldId:', fieldId);
    return;
   }
   
   /* Pin principale (SIG) - utiliser le GPIO de la pin sélectionnée */
   if(additionalPin.id === 'sig') {
    field.value = sigGpio;
    console.log('[initComponentForm] Pin SIG initialisée:', sigGpio);
   }
   /* Pins d'adresse - utiliser les valeurs calculées (générique) */
   else if(addressPinIds.includes(additionalPin.id)) {
    const calculatedValue = calculatedAddressPins[additionalPin.id];
    if(calculatedValue !== null && calculatedValue !== undefined) {
     /* Vérifier si la valeur existe dans les options du select (pour éviter les erreurs silencieuses) */
     const valueStr = String(calculatedValue);
     const hasOption = Array.from(field.options).some(opt => opt.value === valueStr);
     if(hasOption) {
      field.value = valueStr;
      console.log('[initComponentForm] Pin d\'adresse', additionalPin.id, 'initialisée:', calculatedValue, 'field.value après:', field.value);
     } else {
      console.warn('[initComponentForm] Valeur calculée', calculatedValue, 'pour', additionalPin.id, 'n\'existe pas dans les options du select. Options disponibles:', Array.from(field.options).map(o => o.value).join(', '));
      /* Utiliser la première option disponible comme fallback */
      if(field.options.length > 0) {
       field.value = field.options[0].value;
       console.warn('[initComponentForm] Utilisation de la première option disponible comme fallback:', field.value);
      }
     }
    } else {
     console.warn('[initComponentForm] Valeur calculée manquante pour', additionalPin.id, 'calculatedAddressPins:', calculatedAddressPins);
    }
   }
   /* Pins optionnelles - utiliser la valeur par défaut */
   else if(additionalPin.optional) {
    field.value = additionalPin.defaultValue !== undefined ? additionalPin.defaultValue : '255';
    console.log('[initComponentForm] Pin optionnelle', additionalPin.id, 'initialisée:', field.value);
   }
   /* Autres pins - utiliser la valeur par défaut */
   else if(additionalPin.defaultValue !== undefined) {
    field.value = additionalPin.defaultValue;
    console.log('[initComponentForm] Pin', additionalPin.id, 'initialisée avec defaultValue:', additionalPin.defaultValue);
   }
  });
  
  /* Initialiser l'ID du composant (chercher un ID disponible) */
  const idFieldId = FormGenerator.getFieldId(def, 'id');
  let idField = idFieldId ? $('#' + idFieldId) : null;
  
  /* Si le champ n'existe pas, le créer pour les composants avec additionalPins */
  if(!idField && hasAdditionalPinsForInit) {
   const card = $('#componentFormCard');
   if(card) {
    const wrapper = document.createElement('div');
    wrapper.className = 'f';
    
    const label = document.createElement('label');
    label.textContent = 'ID du composant';
    wrapper.appendChild(label);
    
    const select = document.createElement('select');
    select.id = idFieldId;
    select.style.width = '200px';
    
    /* Options: 0, 1 */
    for(let i = 0; i <= 1; i++) {
     const option = document.createElement('option');
     option.value = i;
     option.textContent = i;
     select.appendChild(option);
    }
    
    wrapper.appendChild(select);
    card.appendChild(wrapper);
    idField = select;
    console.log('[initComponentForm] Champ id créé:', idFieldId);
   }
  }
  
  if(idField) {
   /* Parcourir pcfg pour trouver les IDs utilisés */
   const usedIds = [];
   if(typeof pcfg !== 'undefined') {
    Object.keys(pcfg).forEach(lbl => {
     const cfg = pcfg[lbl];
     if(cfg && cfg.complexId !== undefined) {
      usedIds.push(parseInt(cfg.complexId));
     }
    });
   }
   const availableId = [0, 1].find(id => !usedIds.includes(id));
   if(availableId !== undefined) {
    idField.value = availableId;
    console.log('[initComponentForm] complexId initialisé:', availableId);
   } else {
    console.warn('[initComponentForm] Aucun ID disponible (0 et 1 sont utilisés)');
   }
  } else if(hasAdditionalPinsForInit) {
   console.warn('[initComponentForm] Impossible de créer ou trouver le champ id, idFieldId:', idFieldId);
  }
  
  /* Initialiser l'adresse OSC avec un préfixe basé sur l'ID du composant */
  const oscField = $('#oscAddress');
  if(oscField && idField) {
   const prefix = def.id ? def.id : 'complex';
   oscField.value = '/' + prefix + (idField.value || '0');
  }
  
  if(typeof updateBusVisuals === 'function') updateBusVisuals();
 }
}

/* Toutes les fonctions dépréciées ont été supprimées - utiliser directement les modules */
