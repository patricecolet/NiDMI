/**
 * @file component-config.js
 * @brief Lecture et application des configurations de composants
 */

/**
 * Lit les additionalPins d'un composant depuis le formulaire
 * @param {Object} def - Définition du composant
 * @param {Object} c - Objet de configuration à remplir
 */
function readAdditionalPins(def, c) {
  if (!hasAdditionalPins(def) || !def.additionalPins || !Array.isArray(def.additionalPins)) {
    return;
  }

  if (typeof FormGenerator === 'undefined' || !FormGenerator.getFieldId) {
    return;
  }

  console.log('[readAdditionalPins] Lecture additionalPins, def.id:', def.id, 'additionalPins count:', def.additionalPins.length);
  c.additionalPins = {};

  def.additionalPins.forEach(additionalPin => {
    if (!additionalPin.id) return;
    const fieldId = FormGenerator.getFieldId(def, additionalPin.id);
    if (!fieldId) {
      console.warn('[readAdditionalPins] FieldId vide pour additionalPin:', additionalPin.id);
      return;
    }

    /* Vérifier si le champ existe dans le DOM */
    const field = document.getElementById(fieldId);
    if (!field) {
      console.warn('[readAdditionalPins] Champ non trouvé dans le DOM:', fieldId);
      /* Si le champ n'existe pas, vérifier s'il y a une valeur dans pcfg */
      if (cur && pcfg && pcfg[cur] && pcfg[cur].additionalPins && pcfg[cur].additionalPins[additionalPin.id] !== undefined) {
        c.additionalPins[additionalPin.id] = pcfg[cur].additionalPins[additionalPin.id];
        console.log('[readAdditionalPins] additionalPin lu depuis pcfg:', additionalPin.id, '=', c.additionalPins[additionalPin.id]);
      }
      return;
    }

    if (field.value !== undefined && field.value !== null && field.value !== '') {
      const value = parseInt(field.value);
      if (!isNaN(value)) {
        c.additionalPins[additionalPin.id] = value;
        console.log('[readAdditionalPins] additionalPin lu depuis le formulaire:', additionalPin.id, '=', value);
      } else {
        console.warn('[readAdditionalPins] Valeur invalide pour additionalPin:', additionalPin.id, 'value:', field.value);
      }
    } else if (cur && pcfg && pcfg[cur] && pcfg[cur].additionalPins && pcfg[cur].additionalPins[additionalPin.id] !== undefined) {
      /* Si le champ est vide mais qu'il y a une valeur dans pcfg, utiliser celle-ci */
      c.additionalPins[additionalPin.id] = pcfg[cur].additionalPins[additionalPin.id];
      console.log('[readAdditionalPins] additionalPin lu depuis pcfg (champ vide):', additionalPin.id, '=', c.additionalPins[additionalPin.id]);
    } else if (!additionalPin.optional) {
      /* Pin requise mais valeur absente - utiliser la valeur par défaut si disponible */
      if (additionalPin.defaultValue !== undefined && additionalPin.defaultValue !== 255) {
        c.additionalPins[additionalPin.id] = additionalPin.defaultValue;
        console.log('[readAdditionalPins] additionalPin requise, utilisation defaultValue:', additionalPin.id, '=', additionalPin.defaultValue);
      } else {
        console.error('[readAdditionalPins] ERREUR: Pin requise absente et pas de defaultValue:', additionalPin.id);
      }
    }
  });

  /* Note: complexId supprimé - plus besoin de lire le champ id */
}

/**
 * Applique les additionalPins d'un composant au formulaire
 * @param {Object} def - Définition du composant
 * @param {Object} c - Configuration à appliquer
 * @param {Function} setV - Fonction pour définir une valeur (id, value)
 */
function applyAdditionalPins(def, c, setV) {
  const hasAdditionalPinsFlag = hasAdditionalPins(def);
  const hasAdditionalPinsInCfg = c.additionalPins && typeof c.additionalPins === 'object' && Object.keys(c.additionalPins).length > 0;

  if (!hasAdditionalPinsFlag || !hasAdditionalPinsInCfg || !def.additionalPins || typeof FormGenerator === 'undefined' || !FormGenerator.getFieldId) {
    return;
  }

  def.additionalPins.forEach(additionalPin => {
    if (!additionalPin.id) return;
    const fieldId = FormGenerator.getFieldId(def, additionalPin.id);
    if (!fieldId) return;
    const value = c.additionalPins[additionalPin.id];
    if (value !== undefined && value !== null) {
      setV(fieldId, value);
    }
  });

  /* Note: complexId supprimé - plus besoin d'appliquer le champ id */
}

/**
 * Lit la configuration complète depuis le formulaire
 * @param {string|null} roleOverride - Rôle à utiliser au lieu de celui du select
 * @returns {Object} Configuration lue
 */
function readCfg(roleOverride = null) {
  const c = {};
  
  /* Determine which MIDI mode is active */
  const rtpRadio = $('#midiModeRtp');
  const scriptRadio = $('#midiModeScript');
  let midiMode = 'rtp';
  if (scriptRadio && scriptRadio.checked) {
    midiMode = 'script';
  }
  c.midiMode = midiMode;
  
  /* Only read mapping script if script mode is active */
  if (midiMode === 'script') {
    c.mappingScript = $('#mappingPin')?.value || '';
  } else {
    c.mappingScript = ''; /* Clear mapping script if RTP mode is active */
  }
  
  /* Lire le nom personnalisé depuis l'input avant tout écrasement */
  const nameInput = document.getElementById('ComponentName');
  if (nameInput && nameInput.value) {
    c.name = nameInput.value;
  } else if (typeof cur !== 'undefined' && cur && typeof pcfg !== 'undefined' && pcfg[cur] && pcfg[cur].name) {
    c.name = pcfg[cur].name;
  }
  /* Utiliser le roleOverride si fourni, sinon lire depuis le select, sinon depuis pcfg */
  c.role = roleOverride || $('#funcSelect')?.value || '';
  /* Fallback : si funcSelect est vide mais qu'on a un composant dans pcfg, l'utiliser */
  if (!c.role && typeof cur !== 'undefined' && cur && typeof pcfg !== 'undefined' && pcfg[cur] && pcfg[cur].role) {
    const existingRole = pcfg[cur].role;
    if (typeof isBusRole !== 'function' || !isBusRole(existingRole)) {
      c.role = existingRole;
    }
  }

  /* Lire les champs depuis les formFields du composant actuel */
  let migratedRole = null;
  let def = null;
  
  if (c.role) {
    migratedRole = migrateRoleValue(c.role);
    def = getComponentDef(migratedRole);

    if (def && def.formFields && Array.isArray(def.formFields)) {
      def.formFields.forEach(field => {
        if (field.id && !field.id.startsWith('_')) {
          const el = $('#' + field.id);
          if (el) {
            if (field.type === 3) { /* CHECKBOX */
              c[field.id] = el.checked;
            } else if (field.type === 4) { /* RANGE */
              const elMin = $('#' + field.id + 'Min');
              const elMax = $('#' + field.id + 'Max');
              if (elMin) c[field.id + 'Min'] = elMin.value || '';
              if (elMax) c[field.id + 'Max'] = elMax.value || '';
            } else {
              c[field.id] = el.value || '';
            }
          }
        }
      });
    }
    /* Log des valeurs critiques LIS3DH */
    if (migratedRole === 'lis3dh') {
      console.log('[readCfg] LIS3DH DOM: range=' + c.range + ' dataRate=' + c.dataRate + ' filter=' + c.filterIntensity + ' cs=' + c.csGpio +
        ' | #range existe=' + !!$('#range') + ' #dataRate existe=' + !!$('#dataRate') + ' #filterIntensity existe=' + !!$('#filterIntensity'));
    }
  }

  /* Lire les champs MIDI */
  if (midiMode === 'rtp' && migratedRole && def && typeof MidiConfig !== 'undefined' && MidiConfig.readConfig) {
    const midiConfig = MidiConfig.readConfig(def);
    Object.assign(c, midiConfig);
    // Défensif: garder le mode décidé par les radios UI.
    c.midiMode = midiMode;
  }
  /* If script mode, don't read RTP config */

  /* Auto-déterminer busInterface depuis le contexte (pin I2C ou SPI) */
  if (typeof cur !== 'undefined' && cur === 'SPI') {
    c.busInterface = '1';
  } else if (typeof cur !== 'undefined' && cur === 'I2C') {
    c.busInterface = '0';
  }

  /* Lire les champs OSC et Debug */
  c.oscEnabled = !!$('#oscEnabled2')?.checked;
  c.oscAddress = $('#oscAddress')?.value || '';
  c.oscFormat = $('#oscFormat')?.value || 'float';
  c.dbgEnabled = !!$('#dbgEnabled')?.checked;
  c.dbgHeader = $('#dbgHeader')?.value || '';

  /* Lire les additionalPins si composant avec additionalPins */
  if (def) {
    readAdditionalPins(def, c);
  }

  console.log('[readCfg] additionalPins final:', c.additionalPins);
  return c;
}

/**
 * Build default mapping script for a role and component name.
 * @param {string} role - Normalized role id
 * @param {string} componentName - User component name
 * @returns {string} default script or empty string
 */
function buildDefaultMappingScriptForRole(role, componentName) {
  const name = componentName || role || '';
  const defaultScripts = {
    'button': 'r("' + name + '"):*(127):note.out(60,1)',
    'potentiometer': 'r("' + name + '"):*(127):ctl.out(7,1)',
    'velostat': 'r("' + name + '"):*(127):ctl.out(7,1)',
    'touch': 'r("' + name + '"):*(127):ctl.out(7,1)',
    'joystick': 'r("' + name + '"):*(127):ctl.out(7,1)',
    'lis3dh': 'r("' + name + '"):*(127):ctl.out(7,1)'
  };
  return defaultScripts[role] || '';
}

/**
 * Check whether a script still matches the role default structure.
 * If true, we can safely update the name in r("...") automatically.
 * @param {string} role - Normalized role id
 * @param {string} script - Current script
 * @returns {boolean}
 */
function isRoleDefaultMappingScript(role, script) {
  if (!script || typeof script !== 'string') return false;
  const patterns = {
    'button': /^r\(".*"\)\s*:\s*\*\(127\)\s*:\s*note\.out\(60,1\)$/,
    'potentiometer': /^r\(".*"\)\s*:\s*\*\(127\)\s*:\s*ctl\.out\(7,1\)$/,
    'velostat': /^r\(".*"\)\s*:\s*\*\(127\)\s*:\s*ctl\.out\(7,1\)$/,
    'touch': /^r\(".*"\)\s*:\s*\*\(127\)\s*:\s*ctl\.out\(7,1\)$/,
    'joystick': /^r\(".*"\)\s*:\s*\*\(127\)\s*:\s*ctl\.out\(7,1\)$/,
    'lis3dh': /^r\(".*"\)\s*:\s*\*\(127\)\s*:\s*ctl\.out\(7,1\)$/
  };
  const re = patterns[role];
  return !!(re && re.test(script.trim()));
}

/**
 * Keep default mapping script in sync with component name.
 * Only updates when script is empty or still in default format.
 * @param {string} role - Normalized role id
 * @param {string} componentName - User component name
 */
function syncDefaultMappingScriptWithName(role, componentName) {
  const mappingEl = $('#mappingPin');
  if (!mappingEl) return;
  const current = (mappingEl.value || '').trim();
  if (!current || isRoleDefaultMappingScript(role, current)) {
    mappingEl.value = buildDefaultMappingScriptForRole(role, componentName || role);
  }
}

/**
 * Applique une configuration dans le formulaire (simples et complexes)
 * @param {Object} c - Configuration depuis pcfg (peut contenir additionalPins)
 */
function applyCfg(c) {
  if (!c) return;
  /* Les rôles de bus (I2C, SPI, UART) n'ont pas de formulaire de composant */
  if (c.role && typeof isBusRole === 'function' && isBusRole(c.role)) return;

  const mappingIsEmpty = !c || !c.mappingScript || c.mappingScript.trim() === '';
  if (c && typeof c.mappingScript === 'string' && c.mappingScript.trim() !== '') {
    $('#mappingPin').value = c.mappingScript;
  } else if (mappingIsEmpty && c && c.role) {
    const role = migrateRoleValue(c.role);
    $('#mappingPin').value = buildDefaultMappingScriptForRole(role, c.name || role);
  } else {
    $('#mappingPin').value = '';
  }
  
  // Apply MIDI mode radio buttons and containers
  // Migration fallback: old configs may not have midiMode but do have mappingScript.
  const inferredScriptMode = !c.midiMode && !!(c.mappingScript && c.mappingScript.trim() !== '');
  if (c.midiMode === 'script' || inferredScriptMode) {
    applyMidiMode('script');
  } else {
    applyMidiMode('rtp');
  }
  
  const setV = (id, v) => {
    /* Gérer les IDs avec ou sans # */
    const idStr = typeof id === 'string' ? (id[0] === '#' ? id : '#' + id) : id;
    const el = $(idStr) || document.getElementById(id);
    if (el && v != null && v !== undefined) {
      el.value = String(v);
    }
  };

  const setC = (id, b) => {
    const el = $(id);
    if (el) el.checked = !!b;
  };

  const migratedRole = migrateRoleValue(c.role);
  const def = migratedRole ? getComponentDef(migratedRole) : null;
  const hasAdditionalPinsFlag = def && hasAdditionalPins(def) && c.additionalPins && typeof c.additionalPins === 'object' && Object.keys(c.additionalPins).length > 0;

  /* Restaurer la famille si le rôle est défini */
  if (def && def.family !== undefined && $('#familySelect')) {
    $('#familySelect').value = def.family;
  }

  setV('funcSelect', migratedRole);

  /* Vérifier si on doit régénérer le formulaire */
  const card = $('#componentFormCard');
  const currentRoleInSelect = $('#funcSelect')?.value;
  /* Régénérer si le rôle a changé, si le conteneur est vide, ou s'il est masqué */
  const needToRegenerate = (currentRoleInSelect !== migratedRole || !card || card.innerHTML.trim() === '' || card.style.display === 'none');
  
  if (needToRegenerate) {
    showRoleCards(migratedRole, c);
    /* Attendre que les champs soient créés avant d'appliquer les valeurs */
    setTimeout(() => {
      applyConfigValues(c, def, setV, setC);
    }, 50);
  } else {
    /* S'assurer que le conteneur est visible */
    if (card && card.style.display === 'none') {
      card.style.display = 'block';
    }
    /* Appliquer directement les valeurs si le formulaire existe déjà */
    applyConfigValues(c, def, setV, setC);
  }
}

/**
 * Applique les valeurs de configuration au formulaire (helper pour applyCfg)
 */
function applyConfigValues(c, def, setV, setC) {
  const nameInput = document.getElementById('ComponentName');
  if (nameInput) {
    // 1. Calcul du rang (ex: c'est le 2ème bouton)
    let count = 0;
    for (const l of Object.keys(pcfg)) {
      if (pcfg[l] && pcfg[l].role === c.role) {
        count++;
        if (l === cur) break; 
      }
    }

    // 2. Remplissage auto : Nom sauvegardé OU Nom par défaut (ex: Bouton 1)
    nameInput.value = c.name || getRoleDisplayLabel(c.role, count);

    // 3. Capture immédiate de la saisie pour éviter de perdre le nom au Save
    nameInput.oninput = (e) => { 
      if(pcfg[cur]) pcfg[cur].name = e.target.value;
      syncDefaultMappingScriptWithName(migratedRole, e.target.value);
      if(typeof updatePinsList === 'function') updatePinsList();
    };

    // Keep default script synced when loading a pin if user already typed a custom name.
    syncDefaultMappingScriptWithName(migratedRole, nameInput.value);
  }
  const migratedRole = migrateRoleValue(c.role);
  
  updateRtpForRole(migratedRole);

  /* Appliquer les additionalPins si composant avec additionalPins */
  if (def) {
    applyAdditionalPins(def, c, setV);
  }

  /* Appliquer les champs depuis les formFields du composant */
  if (def && def.formFields && Array.isArray(def.formFields)) {
    def.formFields.forEach(field => {
      if (field.id && !field.id.startsWith('_')) {
        if (field.type === 3) { /* CHECKBOX */
          setC(field.id, c[field.id]);
        } else if (field.type === 4) { /* RANGE */
          setV(field.id + 'Min', c[field.id + 'Min']);
          setV(field.id + 'Max', c[field.id + 'Max']);
        } else {
          setV(field.id, c[field.id]);
          if (field.id === 'csGpio') {
            console.log('[applyConfigValues] csGpio appliqué:', c[field.id], 'dans config:', c);
          }
        }
      }
    });
  }

  /* Appliquer les champs MIDI */
  if (def && c.midiMode !== 'script' && typeof MidiConfig !== 'undefined' && MidiConfig.applyConfig) {
    MidiConfig.applyConfig(c, def);
  }
  /* If script mode, don't apply RTP config */

  /* Appliquer les champs OSC et Debug */
  setV('oscAddress', c.oscAddress);
  setV('oscFormat', c.oscFormat);

  // IMPORTANT: on retire tout "grisage" potentiel du champ d'adresse OSC.
  // Même si oscEnabled2 est off, on laisse l'édition de l'adresse possible côté UI.
  const oscAddressEl = $('#oscAddress');
  if (oscAddressEl) {
    oscAddressEl.disabled = false;
    oscAddressEl.style.opacity = '';
    oscAddressEl.style.filter = '';
    oscAddressEl.style.pointerEvents = '';
  }
  const oscFormatEl = $('#oscFormat');
  if (oscFormatEl) {
    oscFormatEl.disabled = false;
    oscFormatEl.style.opacity = '';
    oscFormatEl.style.pointerEvents = '';
  }

  setC('dbgEnabled', c.dbgEnabled);
  setV('dbgHeader', c.dbgHeader);
  
  /* Appliquer le mode MIDI (RTP vs Mapping Script) */
  const midiMode = c.midiMode || 'rtp';
  applyMidiMode(midiMode);
  
  /* Appliquer la valeur du mapping script */
  setV('mappingPin', c.mappingScript);
}

/**
 * Initialize MIDI mode toggle on page load
 */
function initMidiModeToggle() {
  const rtpRadio = $('#midiModeRtp');
  const scriptRadio = $('#midiModeScript');
  
  if (!rtpRadio || !scriptRadio) return;
  
  rtpRadio.addEventListener('change', () => {
    applyMidiMode('rtp');
  });
  
  scriptRadio.addEventListener('change', () => {
    applyMidiMode('script');
  });
}

/**
 * Apply MIDI mode visibility and clear unused fields
 * @param {string} mode - 'rtp' or 'script'
 */
function applyMidiMode(mode) {
  const rtpContainer = $('#rtpMidiConfigContainer');
  const scriptContainer = $('#mappingScriptContainer');
  const rtpRadio = $('#midiModeRtp');
  const scriptRadio = $('#midiModeScript');
  
  if (!rtpContainer || !scriptContainer) return;
  
  if (mode === 'script') {
    rtpContainer.style.display = 'none';
    scriptContainer.style.display = 'block';
    if (scriptRadio) scriptRadio.checked = true;
    if (rtpRadio) rtpRadio.checked = false;
  } else {
    rtpContainer.style.display = 'block';
    scriptContainer.style.display = 'none';
    if (rtpRadio) rtpRadio.checked = true;
    if (scriptRadio) scriptRadio.checked = false;
  }
}
