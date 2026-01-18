/**
 * @file component-form.js
 * @brief Gestion de l'affichage et de l'initialisation des formulaires de composants
 */

/**
 * Affiche la carte de configuration correspondant au rôle sélectionné
 * Utilise les définitions du backend pour déterminer le cardId
 * @param {string} role - ID du rôle (ex: "potentiometer", "hc4067")
 * @param {Object} currentCfg - Configuration actuelle
 */
function showRoleCards(role, currentCfg = {}) {
  /* Utiliser un seul conteneur générique pour tous les composants */
  const card = $('#componentFormCard');
  if (!card) {
    console.warn('[showRoleCards] Conteneur componentFormCard non trouvé');
    return;
  }

  /* Masquer et vider le conteneur par défaut */
  card.style.display = 'none';
  card.innerHTML = '';

  if (!role) return;

  const migratedRole = migrateRoleValue(role);
  const def = getComponentDef(migratedRole);
  if (!def) {
    console.warn('[showRoleCards] Définition non trouvée pour:', migratedRole);
    return;
  }

  const hasAdditionalPinsFlag = hasAdditionalPins(def);
  console.log('[showRoleCards] Définition trouvée, def.id:', def.id, 'hasAdditionalPins:', hasAdditionalPinsFlag, 'additionalPinCount:', def.additionalPinCount);

  /* Afficher le conteneur */
  card.style.display = 'block';

  /* Générer les champs de formulaire dynamiquement */
  if (def.formFields && Array.isArray(def.formFields) && def.formFields.length > 0) {
    if (typeof FormGenerator !== 'undefined' && FormGenerator.generateFormFields) {
      FormGenerator.generateFormFields(def, 'componentFormCard', currentCfg);
    }
  }

  /* Générer les pins additionnelles si composant avec additionalPins */
  if (hasAdditionalPinsFlag && def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0) {
    console.log('[showRoleCards] Appel generateAdditionalPins, def.id:', def.id, 'additionalPins.length:', def.additionalPins.length);
    if (typeof FormGenerator !== 'undefined' && FormGenerator.generateAdditionalPins) {
      FormGenerator.generateAdditionalPins(def, 'componentFormCard', currentCfg);
      console.log('[showRoleCards] generateAdditionalPins appelé');
    } else {
      console.warn('[showRoleCards] FormGenerator.generateAdditionalPins non disponible');
    }
  } else {
    console.log('[showRoleCards] Pas de additionalPins, hasAdditionalPins:', hasAdditionalPinsFlag);
  }

  /* Générer la section MIDI dynamiquement */
  if (typeof MidiConfig !== 'undefined' && MidiConfig.generateMessageSection) {
    MidiConfig.generateMessageSection(def, currentCfg, 'rtpMidiSection');
  }
}

/**
 * Initialise les additionalPins d'un composant avec leurs valeurs par défaut
 * @param {Object} def - Définition du composant
 * @param {Object} pin - Objet pin avec gpio
 * @param {Set} usedGpios - Set des GPIOs déjà utilisés
 */
function initAdditionalPins(def, pin, usedGpios) {
  if (!hasAdditionalPins(def) || !def.additionalPins || !Array.isArray(def.additionalPins)) {
    return;
  }

  if (typeof FormGenerator === 'undefined' || !FormGenerator.getFieldId) {
    console.warn('[initAdditionalPins] FormGenerator non disponible');
    return;
  }

  if (typeof GpioManager === 'undefined') {
    console.warn('[initAdditionalPins] GpioManager non disponible');
    return;
  }

  const sigGpio = parseInt(pin.gpio);
  if (isNaN(sigGpio)) {
    console.warn('[initAdditionalPins] GPIO invalide pour pin:', pin.label);
    return;
  }

  /* Extraire les IDs des pins d'adresse depuis les définitions */
  const addressPinIds = def.additionalPins
    .filter(ap => ap.id && ap.pinType === 1 && !ap.optional && ap.id !== 'sig' && ap.id !== 'en')
    .map(ap => ap.id);

  console.log('[initAdditionalPins] addressPinIds extraits:', addressPinIds);

  /* RESTRICTIONS DÉSACTIVÉES - Calcul automatique des pins désactivé */
  /* L'utilisateur devra sélectionner manuellement les pins */
  let calculatedAddressPins = {};
  /*
  if (addressPinIds.length > 0 && typeof GpioManager.calculateAddressPins === 'function') {
    calculatedAddressPins = GpioManager.calculateAddressPins(sigGpio, usedGpios, addressPinIds);
    console.log('[initAdditionalPins] calculatedAddressPins:', calculatedAddressPins);
  }
  */

  /* Initialiser toutes les additionalPins dynamiquement */
  def.additionalPins.forEach(additionalPin => {
    if (!additionalPin.id) return;
    const fieldId = FormGenerator.getFieldId(def, additionalPin.id);
    const field = fieldId ? $('#' + fieldId) : null;
    if (!field) {
      console.warn('[initAdditionalPins] Champ non trouvé pour additionalPin:', additionalPin.id, 'fieldId:', fieldId);
      return;
    }

    /* Pin principale (SIG) - utiliser le GPIO de la pin sélectionnée */
    if (additionalPin.id === 'sig') {
      field.value = sigGpio;
      console.log('[initAdditionalPins] Pin SIG initialisée:', sigGpio);
    }
    /* Pins d'adresse - utiliser les valeurs calculées */
    else if (addressPinIds.includes(additionalPin.id)) {
      const calculatedValue = calculatedAddressPins[additionalPin.id];
      if (calculatedValue !== null && calculatedValue !== undefined) {
        const valueStr = String(calculatedValue);
        const hasOption = Array.from(field.options).some(opt => opt.value === valueStr);
        if (hasOption) {
          field.value = valueStr;
          console.log('[initAdditionalPins] Pin d\'adresse', additionalPin.id, 'initialisée:', calculatedValue);
        } else {
          console.warn('[initAdditionalPins] Valeur calculée', calculatedValue, 'n\'existe pas dans les options');
          if (field.options.length > 0) {
            field.value = field.options[0].value;
          }
        }
      }
    }
    /* Pins optionnelles - utiliser la valeur par défaut */
    else if (additionalPin.optional) {
      field.value = additionalPin.defaultValue !== undefined ? additionalPin.defaultValue : '255';
      console.log('[initAdditionalPins] Pin optionnelle', additionalPin.id, 'initialisée:', field.value);
    }
    /* Autres pins - utiliser la valeur par défaut */
    else if (additionalPin.defaultValue !== undefined) {
      field.value = additionalPin.defaultValue;
      console.log('[initAdditionalPins] Pin', additionalPin.id, 'initialisée avec defaultValue:', additionalPin.defaultValue);
    }
  });

  /* Initialiser l'ID du composant (chercher un ID disponible) */
  const idFieldId = FormGenerator.getFieldId(def, 'id');
  let idField = idFieldId ? $('#' + idFieldId) : null;

  /* Si le champ n'existe pas, le créer */
  if (!idField && hasAdditionalPins(def)) {
    const card = $('#componentFormCard');
    if (card) {
      const wrapper = document.createElement('div');
      wrapper.className = 'f';

      const label = document.createElement('label');
      label.textContent = 'ID du composant';
      wrapper.appendChild(label);

      const select = document.createElement('select');
      select.id = idFieldId;
      select.style.width = '200px';

      /* Options: 0, 1 */
      for (let i = 0; i <= 1; i++) {
        const option = document.createElement('option');
        option.value = i;
        option.textContent = i;
        select.appendChild(option);
      }

      wrapper.appendChild(select);
      card.appendChild(wrapper);
      idField = select;
      console.log('[initAdditionalPins] Champ id créé:', idFieldId);
    }
  }

  if (idField) {
    /* Parcourir pcfg pour trouver les IDs utilisés */
    const usedIds = [];
    if (typeof pcfg !== 'undefined') {
      Object.keys(pcfg).forEach(lbl => {
        const cfg = pcfg[lbl];
        if (cfg && cfg.complexId !== undefined) {
          usedIds.push(parseInt(cfg.complexId));
        }
      });
    }
    const availableId = [0, 1].find(id => !usedIds.includes(id));
    if (availableId !== undefined) {
      idField.value = availableId;
      console.log('[initAdditionalPins] complexId initialisé:', availableId);
    } else {
      console.warn('[initAdditionalPins] Aucun ID disponible');
    }
  }

  /* Initialiser l'adresse OSC avec un préfixe basé sur l'ID du composant */
  const oscField = $('#oscAddress');
  if (oscField && idField) {
    const prefix = def.id ? def.id : 'complex';
    oscField.value = '/' + prefix + (idField.value || '0');
  }
}

/**
 * Initialise le formulaire d'un composant (simples et complexes)
 * Gère les valeurs par défaut si pas de config dans pcfg
 * @param {string} pinLabel - Label de la pin (ex: "A0")
 */
function initComponentForm(pinLabel, roleOverride = null) {
  console.log('[initComponentForm] Début, pinLabel:', pinLabel, 'roleOverride:', roleOverride);
  
  if (typeof caps === 'undefined' || !caps || !caps.pins || !Array.isArray(caps.pins)) {
    console.warn('[initComponentForm] caps ou caps.pins manquant');
    return;
  }

  const pin = caps.pins.find(p => p && p.label === pinLabel);
  if (!pin || pin.gpio === undefined) {
    console.warn('[initComponentForm] Pin non trouvée:', pinLabel);
    return;
  }

  /* Utiliser roleOverride si fourni, sinon lire depuis funcSelect */
  let funcSelectValue = roleOverride;
  if (!funcSelectValue) {
    funcSelectValue = $('#funcSelect')?.value || '';
  }
  
  /* Si toujours vide, essayer de trouver dans pcfg */
  if (!funcSelectValue && typeof pcfg !== 'undefined' && pcfg[pinLabel]) {
    funcSelectValue = pcfg[pinLabel].role || '';
  }
  
  const migratedRoleValue = migrateRoleValue(funcSelectValue);
  const def = getComponentDef(migratedRoleValue);

  if (!def) {
    console.warn('[initComponentForm] Définition non trouvée pour:', migratedRoleValue, '(funcSelectValue:', funcSelectValue, ')');
    return;
  }

  const hasAdditionalPinsFlag = hasAdditionalPins(def);
  console.log('[initComponentForm] Définition trouvée, def.id:', def.id, 'hasAdditionalPins:', hasAdditionalPinsFlag);

  /* Vérifier si configuration existe dans pcfg */
  const pcfgEntry = typeof pcfg !== 'undefined' && pcfg[pinLabel] ? pcfg[pinLabel] : null;
  const currentSelectedRole = $('#funcSelect')?.value || '';
  
  if (pcfgEntry) {
    /* Configuration existe, vérifier si le rôle correspond */
    const pcfgRole = migrateRoleValue(pcfgEntry.role || '');
    console.log('[initComponentForm] Configuration trouvée dans pcfg, pcfgRole:', pcfgRole, 'currentSelectedRole:', currentSelectedRole);
    /* Si le rôle correspond, appliquer la config */
    if (pcfgRole && currentSelectedRole && pcfgRole === currentSelectedRole) {
      applyCfg(pcfgEntry);
      return;
    }
    console.log('[initComponentForm] Rôle dans pcfg ne correspond pas, initialisation avec valeurs par défaut');
  }

  /* Pas de config : initialiser valeurs par défaut pour composants avec additionalPins uniquement */
  if (hasAdditionalPinsFlag) {
    const usedGpios = (typeof GpioManager !== 'undefined' && GpioManager.getUsedGpios && typeof GpioManager.getUsedGpios === 'function')
      ? GpioManager.getUsedGpios([])
      : new Set();
    
    initAdditionalPins(def, pin, usedGpios);
    
    if (typeof updateBusVisuals === 'function') {
      updateBusVisuals();
    }
  }
}
