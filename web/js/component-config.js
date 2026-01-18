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

    const field = $('#' + fieldId);
    if (!field) {
      console.warn('[readAdditionalPins] Champ non trouvé:', fieldId);
      return;
    }

    if (field.value !== undefined && field.value !== null && field.value !== '') {
      const value = parseInt(field.value);
      if (!isNaN(value)) {
        c.additionalPins[additionalPin.id] = value;
        console.log('[readAdditionalPins] additionalPin lu:', additionalPin.id, '=', value);
      }
    }
  });

  /* Lire complexId si présent */
  const idFieldId = FormGenerator.getFieldId(def, 'id');
  if (idFieldId) {
    const idField = $('#' + idFieldId);
    if (idField && idField.value !== undefined && idField.value !== null && idField.value !== '') {
      const complexId = parseInt(idField.value);
      if (!isNaN(complexId)) {
        c.complexId = complexId;
        console.log('[readAdditionalPins] complexId lu:', complexId);
      }
    }
  }
}

/**
 * Applique les additionalPins d'un composant au formulaire
 * @param {Object} def - Définition du composant
 * @param {Object} c - Configuration à appliquer
 * @param {Function} setV - Fonction pour définir une valeur (id, value)
 */
function applyAdditionalPins(def, c, setV) {
  const hasAdditionalPinsFlag = hasAdditionalPins(def);
  const hasAdditionalPinsInCfg = c.additionalPins && typeof c.additionalPins === 'object' && c.additionalPins.sig !== undefined;

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

  /* Appliquer complexId si présent */
  if (c.complexId !== undefined && def) {
    const idFieldId = FormGenerator.getFieldId(def, 'id');
    if (idFieldId) setV(idFieldId, c.complexId);
  }
}

/**
 * Lit la configuration complète depuis le formulaire
 * @param {string|null} roleOverride - Rôle à utiliser au lieu de celui du select
 * @returns {Object} Configuration lue
 */
function readCfg(roleOverride = null) {
  const c = {};
  /* Utiliser le roleOverride si fourni, sinon lire depuis le select */
  c.role = roleOverride || $('#funcSelect')?.value || '';

  console.log('[readCfg] Début, role:', c.role, 'roleOverride:', roleOverride);

  /* Lire les champs depuis les formFields du composant actuel */
  let migratedRole = null;
  let def = null;
  
  if (c.role) {
    migratedRole = migrateRoleValue(c.role);
    def = getComponentDef(migratedRole);
    const hasAdditionalPinsFlag = hasAdditionalPins(def);
    console.log('[readCfg] migratedRole:', migratedRole, 'def trouvée:', !!def, 'hasAdditionalPins:', hasAdditionalPinsFlag);

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
  }

  /* Lire les champs MIDI */
  if (migratedRole && def && typeof MidiConfig !== 'undefined' && MidiConfig.readConfig) {
    const midiConfig = MidiConfig.readConfig(def);
    Object.assign(c, midiConfig);
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

  console.log('[readCfg] additionalPins final:', c.additionalPins, 'complexId:', c.complexId);
  return c;
}

/**
 * Applique une configuration dans le formulaire (simples et complexes)
 * @param {Object} c - Configuration depuis pcfg (peut contenir additionalPins)
 */
function applyCfg(c) {
  if (!c) return;

  const setV = (id, v) => {
    const el = $(id);
    if (el && v != null) el.value = v;
  };

  const setC = (id, b) => {
    const el = $(id);
    if (el) el.checked = !!b;
  };

  const migratedRole = migrateRoleValue(c.role);
  const def = migratedRole ? getComponentDef(migratedRole) : null;
  const hasAdditionalPinsFlag = c.additionalPins && typeof c.additionalPins === 'object' && c.additionalPins.sig !== undefined;

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
    console.log('[applyCfg] Appel showRoleCards, currentRoleInSelect:', currentRoleInSelect, 'migratedRole:', migratedRole);
    showRoleCards(migratedRole, c);
    /* Attendre que les champs soient créés avant d'appliquer les valeurs */
    setTimeout(() => {
      applyConfigValues(c, def, setV, setC);
    }, 50);
  } else {
    console.log('[applyCfg] showRoleCards déjà appelé pour ce rôle, appliquer directement');
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
        }
      }
    });
  }

  /* Appliquer les champs MIDI */
  if (def && typeof MidiConfig !== 'undefined' && MidiConfig.applyConfig) {
    MidiConfig.applyConfig(c, def);
  }

  /* Appliquer les champs OSC et Debug */
  setC('oscEnabled2', c.oscEnabled);
  setV('oscAddress', c.oscAddress);
  setV('oscFormat', c.oscFormat);
  setC('dbgEnabled', c.dbgEnabled);
  setV('dbgHeader', c.dbgHeader);
}
