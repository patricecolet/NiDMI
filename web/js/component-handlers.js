/**
 * @file component-handlers.js
 * @brief Gestionnaires d'événements pour les composants
 */

/**
 * Trouve le rôle d'un composant complexe utilisant une pin comme pin principale
 * @param {number} mainPinGpio - GPIO de la pin principale
 * @returns {string|null} Rôle du composant ou null
 */
function findComplexRoleByMainPinGpio(mainPinGpio) {
  if (typeof pcfg === 'undefined' || !pcfg) return null;
  
  const existingComplexLabel = Object.keys(pcfg).find(lbl => {
    const cfg = pcfg[lbl];
    if(!cfg || !cfg.role) return false;
    const cfgRole = migrateRole(cfg.role);
    const cfgDef = typeof getComponentDefinition === 'function' ? getComponentDefinition(cfgRole) : null;
    const cfgHasAdditionalPins = cfgDef && cfgDef.additionalPins && Array.isArray(cfgDef.additionalPins) && cfgDef.additionalPins.length > 0
      && cfg.additionalPins && typeof cfg.additionalPins === 'object' && Object.keys(cfg.additionalPins).length > 0;
    if(!cfgHasAdditionalPins) return false;
    /* Vérifier si cette pin est utilisée comme pin principale du composant complexe */
    const complexPin = caps && caps.pins ? caps.pins.find(p => p.label === lbl) : null;
    return complexPin && parseInt(complexPin.gpio) === mainPinGpio;
  });
  
  if (existingComplexLabel) {
    const existingComplex = pcfg[existingComplexLabel];
    if (existingComplex && existingComplex.role) {
      return existingComplex.role;
    }
  }
  return null;
}

/**
 * Restaure la configuration d'un composant dans les menus
 * @param {string} lbl - Label de la pin
 * @param {string} currentRole - Rôle actuel à restaurer
 * @param {number} pinType - Type de pin
 * @param {Object} pin - Objet pin avec gpio
 */
function restoreComponentConfig(lbl, currentRole, pinType, pin) {
  const sel = $('#funcSelect');
  const familySel = $('#familySelect');
  if (!sel || !familySel) return;

  const migratedRoleVal = migrateRoleValue(currentRole);
  const def = getComponentDef(migratedRoleVal);
  if (!def) return;

  /* Sélectionner la bonne famille */
  const defFamilyId = def.family !== undefined ? def.family : 0;
  
  if (familySel.value != defFamilyId) {
    /* Marquer qu'on est en train de restaurer une configuration */
    familySel._restoringConfig = true;
    console.log('[restoreComponentConfig] Restauration config, changement famille de', familySel.value, 'vers', defFamilyId);
    
    /* Appeler populateComponentSelect AVANT de changer familySel.value */
    populateComponentSelect(defFamilyId, pinType, pin);
    
    /* Maintenant changer la valeur */
    familySel.value = defFamilyId;
    
    /* Réinitialiser le flag après un délai */
    setTimeout(() => {
      familySel._restoringConfig = false;
      console.log('[restoreComponentConfig] Flag _restoringConfig réinitialisé');
    }, 100);
    
    /* Attendre que le menu Composant soit rempli avant de sélectionner */
    setTimeout(() => {
      /* Toujours mettre à jour la sélection et réafficher le formulaire, même si le rôle est déjà sélectionné */
      sel.value = migratedRoleVal;
      const currentCfg = pcfg && pcfg[lbl] ? pcfg[lbl] : {};
      showRoleCards(migratedRoleVal, currentCfg);
      updateRtpForRole(migratedRoleVal);
      if (typeof MidiConfig !== 'undefined' && MidiConfig.updateVisibility) {
        MidiConfig.updateVisibility();
      }
      /* Attendre que les champs MIDI soient créés avant d'appliquer la config */
      setTimeout(() => {
        /* Passer le rôle explicitement pour éviter les problèmes de timing */
        initComponentForm(lbl, migratedRoleVal);
      }, 50);
    }, 0);
  } else {
    /* Famille déjà sélectionnée, juste sélectionner le composant */
    setTimeout(() => {
      /* Toujours mettre à jour la sélection et réafficher le formulaire, même si le rôle est déjà sélectionné */
      sel.value = migratedRoleVal;
      const currentCfg = pcfg && pcfg[lbl] ? pcfg[lbl] : {};
      showRoleCards(migratedRoleVal, currentCfg);
      updateRtpForRole(migratedRoleVal);
      if (typeof MidiConfig !== 'undefined' && MidiConfig.updateVisibility) {
        MidiConfig.updateVisibility();
      }
      /* Attendre que les champs MIDI soient créés avant d'appliquer la config */
      setTimeout(() => {
        /* Passer le rôle explicitement pour éviter les problèmes de timing */
        initComponentForm(lbl, migratedRoleVal);
      }, 50);
    }, 0);
  }
}

/**
 * Initialise les menus et sélectionne le premier composant par défaut
 * @param {string} lbl - Label de la pin
 * @param {number} pinType - Type de pin
 * @param {Object} pin - Objet pin avec gpio
 */
function initDefaultComponent(lbl, pinType, pin) {
  const sel = $('#funcSelect');
  const familySel = $('#familySelect');
  
  setTimeout(() => {
    if (familySel.value && sel.options.length > 0) {
      const firstComponentId = sel.value;
      if (firstComponentId) {
        showRoleCards(firstComponentId, {});
        updateRtpForRole(firstComponentId);
        if (typeof MidiConfig !== 'undefined' && MidiConfig.updateVisibility) {
          MidiConfig.updateVisibility();
        }
        /* Passer le rôle explicitement pour éviter les problèmes de timing */
        initComponentForm(lbl, firstComponentId);
      }
    }
  }, 0);
}

/**
 * Configure les handlers d'événements pour les menus
 * @param {string} lbl - Label de la pin
 * @param {number} pinType - Type de pin
 * @param {Object} pin - Objet pin avec gpio
 */
function setupMenuHandlers(lbl, pinType, pin) {
  const sel = $('#funcSelect');
  const familySel = $('#familySelect');
  if (!sel || !familySel) return;

  /* Retirer l'ancien handler pour éviter les doublons */
  const oldHandler = familySel._onchangeHandler;
  if (oldHandler) {
    familySel.removeEventListener('change', oldHandler);
  }

  /* Gérer le changement de famille */
  const newHandler = () => {
    const familyId = parseInt(familySel.value);
    if (isNaN(familyId)) return;

    /* Ne pas vider le select Component si on est en train de restaurer une configuration */
    if (familySel._restoringConfig) {
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
    if (typeof MidiConfig !== 'undefined' && MidiConfig.updateVisibility) {
      MidiConfig.updateVisibility();
    }
  };
  
  familySel.addEventListener('change', newHandler);
  familySel._onchangeHandler = newHandler; /* Stocker pour pouvoir le retirer plus tard */

  sel.onchange = updateConfig;

  /* Générer la liste d'inputs dynamiquement */
  const inputs = getAllFieldIds();
  inputs.forEach(id => {
    const el = $(id);
    if (el) {
      el.addEventListener('change', updateConfig);
      el.addEventListener('input', updateConfig);
    }
  });
}

/**
 * Met à jour la configuration lors d'un changement de champ
 */
function updateConfig() {
  const lbl = $('#selPin')?.textContent || '';
  let selectedRole = $('#funcSelect')?.value || '';
  
  /* Si selectedRole est vide mais qu'on a une config dans pcfg, récupérer le rôle depuis pcfg */
  if (!selectedRole && cur && pcfg && pcfg[cur] && pcfg[cur].role) {
    selectedRole = pcfg[cur].role;
  }
  
  /* Si on n'a toujours pas de selectedRole, sortir tôt */
  if (!selectedRole) {
    return;
  }
  
  /* Vérifier si le rôle a changé pour décider si on doit régénérer le formulaire */
  const previousRole = (cur && pcfg && pcfg[cur]) ? migrateRoleValue(pcfg[cur].role) : null;
  const currentRole = selectedRole ? migrateRoleValue(selectedRole) : null;
  const roleChanged = previousRole !== currentRole;
  
  /* Si le rôle n'a pas changé, lire les valeurs actuelles du formulaire */
  /* Attendre un peu pour s'assurer que les champs additionalPins sont créés */
  if (!roleChanged && cur && typeof readCfg === 'function') {
    /* Utiliser setTimeout pour s'assurer que les champs sont dans le DOM */
    setTimeout(() => {
      const updatedCfg = readCfg(selectedRole);
      if (updatedCfg && updatedCfg.role) {
        /* Mettre à jour pcfg avec les nouvelles valeurs du formulaire */
        pcfg[cur] = updatedCfg;
        console.log('[updateConfig] Config mise à jour depuis formulaire (rôle inchangé), pcfg[cur]:', pcfg[cur]);
        console.log('[updateConfig] additionalPins sauvegardés:', updatedCfg.additionalPins);
        /* Note: complexId supprimé */
      }
      /* Mettre à jour la liste et les visuels après avoir sauvegardé les modifications */
      updatePinsList();
      updateBusVisuals();
    }, 10);  /* Délai court pour s'assurer que les champs sont dans le DOM */
    
    /* Mettre à jour la visibilité des paramètres MIDI */
    updateRtpForRole(selectedRole);
    if (typeof MidiConfig !== 'undefined' && MidiConfig.updateVisibility) {
      MidiConfig.updateVisibility();
    }
    return; /* Sortir tôt si on ne régénère pas le formulaire */
  }
  
  /* Régénérer le formulaire seulement si le rôle a changé */
  if (roleChanged) {
    /* Utiliser la config mise à jour (ou celle de pcfg) pour régénérer le formulaire */
    const currentCfg = (cur && pcfg && pcfg[cur]) ? pcfg[cur] : (pcfg && pcfg[lbl] ? pcfg[lbl] : {});
    showRoleCards(selectedRole, currentCfg);
    /* Après avoir régénéré le formulaire, lire les valeurs si elles existent déjà */
    setTimeout(() => {
      if (cur && typeof readCfg === 'function') {
        const updatedCfg = readCfg(selectedRole);
        if (updatedCfg && updatedCfg.role) {
          /* Mettre à jour pcfg avec les nouvelles valeurs du formulaire */
          pcfg[cur] = updatedCfg;
          console.log('[updateConfig] Config mise à jour depuis formulaire (après régénération), pcfg[cur]:', pcfg[cur]);
          console.log('[updateConfig] additionalPins sauvegardés:', updatedCfg.additionalPins);
        }
      }
    }, 100);
    
    updateRtpForRole(selectedRole);
    if (typeof MidiConfig !== 'undefined' && MidiConfig.updateVisibility) {
      MidiConfig.updateVisibility();
    }
    
    /* Initialiser le formulaire (gère simples et complexes) - attendre que les champs soient créés */
    if (cur) {
      console.log('[updateConfig] Initialisation formulaire pour cur:', cur);
      /* Attendre un peu pour que generateAdditionalPins() crée les champs */
      setTimeout(() => {
        if (cur) {
          /* Passer le rôle explicitement pour éviter les problèmes de timing */
          initComponentForm(cur, selectedRole);
          /* Attendre que initComponentForm() ait rempli les champs avant de mettre à jour la liste */
          setTimeout(() => {
            if (cur) {
              console.log('[updateConfig] Mise à jour de la liste et des visuels');
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
  }
}

/**
 * Fonction principale appelée lors du clic sur une pin
 * Initialise les menus et restaure ou initialise la configuration
 * @param {string} lbl - Label de la pin (ex: "A0", "D7")
 */
function updFunc(lbl) {
  const sel = $('#funcSelect');
  const familySel = $('#familySelect');
  if (!sel || !familySel) return;

  /* Vérifier si les définitions sont chargées, sinon les charger */
  const defsCacheCheck = getDefsCache();
  if (!defsCacheCheck || defsCacheCheck.length === 0) {
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
  if (lbl?.startsWith('A')) {
    pinType = 0;
  } else if (lbl?.startsWith('D')) {
    pinType = 1;
  } else {
    return; /* Bus purs non implémentés */
  }

  console.log('[updFunc] pinType calculé:', pinType);

  /* Remplir le menu Famille (qui déclenchera le remplissage du menu Composant) */
  populateFamilySelect(pinType, pin);

  /* Si une valeur était déjà sélectionnée, essayer de la restaurer APRÈS que les menus soient remplis */
  let currentRole = pcfg[lbl]?.role;

  /* Vérifier si cette pin est utilisée par un composant complexe sauvegardé */
  if (!currentRole && pin && pin.gpio !== undefined && typeof pcfg !== 'undefined' && pcfg) {
    const mainPinGpio = parseInt(pin.gpio);
    if (!isNaN(mainPinGpio)) {
      currentRole = findComplexRoleByMainPinGpio(mainPinGpio);
      if (currentRole) {
        console.log('[updFunc] Pin utilisée par composant complexe sauvegardé, role:', currentRole);
      }
    }
  }

  if (currentRole) {
    /* Utiliser setTimeout pour s'assurer que les menus sont remplis */
    setTimeout(() => {
      restoreComponentConfig(lbl, currentRole, pinType, pin);
    }, 0);
  } else {
    /* Pas de configuration existante : attendre que les menus soient remplis, puis sélectionner le premier composant par défaut */
    initDefaultComponent(lbl, pinType, pin);
  }

  /* Configurer les handlers d'événements */
  setupMenuHandlers(lbl, pinType, pin);
}
