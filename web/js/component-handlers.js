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
      /* Utiliser le premier composant de la liste (surtout pour bus I2C/SPI où sel.value peut être vide) */
      const firstComponentId = sel.options[0]?.value || sel.value;
      if (firstComponentId) {
        sel.value = firstComponentId;
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

  /* Délégation : tout input/select dans le formulaire déclenche updateConfig (champs créés dynamiquement) */
  const formCard = $('#componentFormCard');
  if (formCard && !formCard._updateConfigDelegationAttached) {
    formCard._updateConfigDelegationAttached = true;
    formCard.addEventListener('change', function(e) {
      if (e.target && (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT')) {
        if (typeof updateConfig === 'function') updateConfig();
      }
    });
    formCard.addEventListener('input', function(e) {
      if (e.target && (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT')) {
        if (typeof updateConfig === 'function') updateConfig();
      }
    });
  }

  /* Générer la liste d'inputs dynamiquement (pour les champs déjà présents au chargement) */
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
  
  console.log('[updateConfig] selectedRole depuis funcSelect:', selectedRole, 'cur:', cur);
  
  /* Si selectedRole est vide mais qu'on a une config dans pcfg, récupérer le rôle depuis pcfg */
  if (!selectedRole && cur && pcfg && pcfg[cur] && pcfg[cur].role) {
    selectedRole = pcfg[cur].role;
    console.log('[updateConfig] selectedRole depuis pcfg:', selectedRole);
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
  if (!roleChanged && cur && typeof readCfg === 'function') {
    /* Mise à jour immédiate de pcfg pour que les réglages restent en mémoire (filtre, type MIDI, etc.) */
    const updatedCfg = readCfg(selectedRole);
    if (updatedCfg && updatedCfg.role) {
      pcfg[cur] = updatedCfg;
      console.log('[updateConfig] Config mise à jour depuis formulaire (rôle inchangé), pcfg[cur]:', pcfg[cur]);
      console.log('[updateConfig] additionalPins sauvegardés:', updatedCfg.additionalPins);
    }
    /* Liste et visuels en différé pour laisser le temps aux champs dynamiques (additionalPins) */
    setTimeout(() => {
      if (typeof updatePinsList === 'function') updatePinsList();
      if (typeof updateBusVisuals === 'function') updateBusVisuals();
    }, 10);

    updateRtpForRole(selectedRole);
    if (typeof MidiConfig !== 'undefined' && MidiConfig.updateVisibility) {
      MidiConfig.updateVisibility();
    }
    return;
  }
  
  /* Régénérer le formulaire seulement si le rôle a changé */
  if (roleChanged) {
    /* Lire la config actuelle depuis le formulaire AVANT de régénérer pour préserver les valeurs */
    let currentCfg = {};
    if (cur && typeof readCfg === 'function') {
      const existingCfg = readCfg();
      if (existingCfg && existingCfg.role) {
        currentCfg = existingCfg;
        console.log('[updateConfig] Config lue depuis formulaire avant régénération:', currentCfg);
      }
    }
    /* Si aucune config n'a été lue, utiliser celle de pcfg */
    if (!currentCfg.role && cur && pcfg && pcfg[cur]) {
      currentCfg = pcfg[cur];
    } else if (!currentCfg.role && pcfg && pcfg[lbl]) {
      currentCfg = pcfg[lbl];
    }

    /* Si le nom actuel est un nom généré pour l'ancien rôle, on le réinitialise pour le nouveau rôle */
    if (currentCfg && currentCfg.name && previousRole && typeof isGeneratedComponentName === 'function') {
      if (isGeneratedComponentName(previousRole, currentCfg.name, cur)) {
        currentCfg.name = '';
        console.log('[updateConfig] Nom du composant réinitialisé car généré pour l ancien rôle:', previousRole);
      }
    }

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
  /*Protection pour eviter les incoherences*/
  if (pinsViewMode === 'global') {
    showPinEditor(lbl);
  }
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

  /* Résoudre le pin : pour les bus, créer un objet virtuel depuis caps.bus */
  let pin = null;
  let pinType = null;

  if (lbl?.startsWith('A')) {
    pin = caps?.pins?.find(p => p?.label === lbl) || null;
    pinType = 0; // ANALOG
  } else if (lbl?.startsWith('D')) {
    pin = caps?.pins?.find(p => p?.label === lbl) || null;
    pinType = 1; // DIGITAL
  } else if (lbl === 'I2C') {
    pin = caps?.bus?.i2c ? { label: 'I2C', gpio: caps.bus.i2c.sda, bus: 'i2c' } : null;
    pinType = 4; // PIN_I2C
  } else if (lbl === 'SPI') {
    pin = caps?.bus?.spi ? { label: 'SPI', gpio: caps.bus.spi.mosi, bus: 'spi' } : null;
    pinType = 5; // PIN_SPI
  } else if (lbl === 'TX' || lbl === 'RX') {
    pin = caps?.pins?.find(p => p?.label === lbl) || null;
    pinType = 1; // UART → digital pour l'instant
  } else {
    return;
  }

  console.log('[updFunc] pinType calculé:', pinType);

  /* Remplir le menu Famille (qui déclenchera le remplissage du menu Composant) */
  populateFamilySelect(pinType, pin);

  /* Si une valeur était déjà sélectionnée, essayer de la restaurer APRÈS que les menus soient remplis */
  let currentRole = pcfg[lbl]?.role;

  /* Les rôles de bus (I2C, SPI, UART) ne sont pas des composants → traiter comme "pas de config" */
  if (currentRole && typeof isBusRole === 'function' && isBusRole(currentRole)) {
    currentRole = null;
  }

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
