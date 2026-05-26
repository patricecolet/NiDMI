/**
 * @file component-utils.js
 * @brief Utilitaires pour la gestion des composants
 */

/**
 * Met à jour la visibilité des paramètres MIDI pour un rôle
 * @param {string} role - ID du rôle
 */
function updateRtpForRole(role) {
  /* Migrer les anciens formats si nécessaire */
  const migratedRole = migrateRoleValue(role || '');
  const def = getComponentDef(migratedRole);

  /* Mettre à jour la visibilité des paramètres MIDI si nécessaire */
  if (def && def.supportsMidi && def.midiMessages && def.midiMessages.length > 0) {
    if (typeof MidiConfig !== 'undefined' && MidiConfig.updateVisibility) {
      MidiConfig.updateVisibility();
    }
  }
}

/**
 * Collecte tous les IDs de champs depuis les formFields et les champs RTP
 * @returns {Array<string>} Liste des IDs de champs
 */
function getAllFieldIds() {
  const ids = [];

  /* Collecter depuis les formFields de tous les composants */
  const defsCache = getDefsCache();
  if (defsCache && defsCache.length > 0) {
    defsCache.forEach(def => {
      if (def.formFields && Array.isArray(def.formFields)) {
        def.formFields.forEach(field => {
          if (field.id && !field.id.startsWith('_')) { /* Ignorer les hints standalone */
            ids.push('#' + field.id);
            /* Pour RANGE, ajouter Min et Max */
            if (field.type === 4) { /* RANGE */
              ids.push('#' + field.id + 'Min');
              ids.push('#' + field.id + 'Max');
            }
          }
        });
      }
    });

    /* Collecter les paramètres MIDI depuis toutes les définitions */
    defsCache.forEach(def => {
      if (def.midiMessages && Array.isArray(def.midiMessages)) {
        def.midiMessages.forEach(msg => {
          if (msg.params && Array.isArray(msg.params)) {
            msg.params.forEach(param => {
              if (param.id) {
                const paramId = '#' + param.id;
                if (!ids.includes(paramId)) ids.push(paramId);
                /* Pour RANGE, ajouter Min et Max */
                if (param.type === 4) { /* RANGE */
                  const paramMinId = '#' + param.id + 'Min';
                  const paramMaxId = '#' + param.id + 'Max';
                  if (!ids.includes(paramMinId)) ids.push(paramMinId);
                  if (!ids.includes(paramMaxId)) ids.push(paramMaxId);
                }
              }
            });
          }
        });
      }
    });

    /* Ajouter les champs OSC et Debug */
    ids.push('#oscAddress', '#oscFormat', '#dbgEnabled', '#dbgHeader');

    /* Collecter les IDs des additionalPins depuis toutes les définitions */
    defsCache.forEach(def => {
      if (hasAdditionalPins(def) && def.additionalPins && Array.isArray(def.additionalPins)) {
        def.additionalPins.forEach(additionalPin => {
          if (additionalPin.id) {
            /* ID du champ : préfixe depuis l'ID du composant + id en capital (ex: s0 -> hc4067S0) */
            const prefix = def.id ? def.id : 'comp';
            const fieldId = '#' + prefix + additionalPin.id.charAt(0).toUpperCase() + additionalPin.id.slice(1);
            if (!ids.includes(fieldId)) ids.push(fieldId);
          }
        });
      }
    });
  }

  return ids;
}
