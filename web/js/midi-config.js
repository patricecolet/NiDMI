/**
 * Configuration des messages MIDI (générique)
 * Module refactorisé depuis components.js
 * 
 * Responsabilités:
 * - Génération de la section de configuration MIDI
 * - Génération des paramètres MIDI depuis MidiParamDef
 * - Gestion de la visibilité conditionnelle des paramètres
 * - Lecture/Application de la configuration MIDI
 * 
 * Note: RTP-MIDI, USB-MIDI, Debug MIDI sont des interfaces activables via checkboxes HTML (hardcodées)
 */

const MidiConfig = {
  FIELDS: {
    messageType: 'midiMessageType',  /* Renommé depuis rtpMsgType (compatibilité: rtpMsgType) */
    params: 'midiParams',            /* Renommé depuis rtpParams (compatibilité: rtpParams) */
    section: 'midiMessageSection',   /* Renommé depuis rtpMidiSection (compatibilité: rtpMidiSection) */
    enabled: 'rtpMidiEnabled',       /* Checkbox RTP-MIDI (hardcodée dans HTML) */
    enabled2: 'rtpEnabled2'          /* Alias pour compatibilité (peut être supprimé plus tard) */
  },

  /**
   * Génère la section de configuration des messages MIDI
   * Génère dynamiquement le select du type de message et tous les paramètres
   * depuis def.midiMessages[].params[] (pas limité à cc, note, channel - tous les paramètres)
   * @param {Object} def - Définition du composant
   * @param {Object} currentCfg - Configuration actuelle
   * @param {string} containerId - ID du conteneur (optionnel, défaut: "rtpMidiSection")
   */
  generateMessageSection(def, currentCfg = {}, containerId = 'rtpMidiSection') {
    console.log('[MidiConfig.generateMessageSection] APPEL pour composant:', def?.id);
    const container = $('#' + containerId);
    if(!container) {
      console.warn('[MidiConfig.generateMessageSection] Conteneur non trouvé:', containerId);
      return;
    }
    
    /* Vider le conteneur */
    container.innerHTML = '';

    /* Si le composant ne supporte pas MIDI, ne rien afficher */
    if(!def || !def.supportsMidi || !def.midiMessages || def.midiMessages.length === 0) {
      return;
    }
    
    /* Si c'est un joystick, générer deux sections séparées (une par axe) */
    if(def.id === 'joystick') {
      this.generateAxisSection(def, currentCfg, container, 'x');
      this.generateAxisSection(def, currentCfg, container, 'y');
      return;
    }
    
    /* Log de diagnostic pour voir combien de messages MIDI sont présents */
    console.log('[MidiConfig.generateMessageSection] Composant:', def.id, 'Messages MIDI:', def.midiMessages.length, def.midiMessages.map(m => m.displayName));
    
    /* Utiliser une copie du tableau pour éviter les mutations */
    const midiMessages = Array.isArray(def.midiMessages) ? [...def.midiMessages] : [];
    
    /* Créer le wrapper principal (Type de message MIDI) */
    const wrapper = document.createElement('div');
    wrapper.className = 'r';
    
    const typeLabel = document.createElement('label');
    const typeText = '{{t.pins.type}}:';
    typeLabel.textContent = (typeof FormGenerator !== 'undefined' && FormGenerator.replaceTranslationTemplate)
      ? FormGenerator.replaceTranslationTemplate(typeText)
      : typeText.replace(/\{\{t\.pins\.(\w+)\}\}/g, (match, key) => {
          const translations = { 'type': 'Type' };
          return translations[key] || key;
        });
    
    /* Select midiMessageType (compatibilité: rtpMsgType) */
    const rtpMsgTypeSelect = document.createElement('select');
    rtpMsgTypeSelect.id = 'rtpMsgType'; /* Garder rtpMsgType pour compatibilité */
    rtpMsgTypeSelect.setAttribute('data-midi-message-type', 'true'); /* Marquer pour migration future */

    /* Ajouter les options depuis midiMessages (utiliser la copie) */
    let optionsAdded = 0;
    midiMessages.forEach(msg => {
      const option = document.createElement('option');
      option.value = msg.displayName;
      option.textContent = msg.displayName;
      rtpMsgTypeSelect.appendChild(option);
      optionsAdded++;
    });
    console.log('[MidiConfig.generateMessageSection] Options ajoutées au select:', optionsAdded, 'sur', midiMessages.length);
    
    /* Sélectionner la valeur actuelle si disponible (nouveau format puis ancien pour compatibilité) */
    if(currentCfg.midiMessageType) {
      rtpMsgTypeSelect.value = currentCfg.midiMessageType;
    } else if(currentCfg.rtpType) {
      rtpMsgTypeSelect.value = currentCfg.rtpType; /* Compatibilité ancien format */
    }

    wrapper.appendChild(typeLabel);
    wrapper.appendChild(rtpMsgTypeSelect);
    container.appendChild(wrapper);

    /* Créer le conteneur pour les paramètres (compatibilité: rtpParams) */
    /* Le formulaire de configuration MIDI est toujours visible, indépendamment de l'activation de RTP-MIDI */
    /* car les paramètres peuvent être utilisés par USB-MIDI, Debug MIDI, ou OSC */
    const paramsContainer = document.createElement('div');
    paramsContainer.id = 'rtpParams'; /* Garder rtpParams pour compatibilité */
    paramsContainer.setAttribute('data-midi-params', 'true'); /* Marquer pour migration future */
    paramsContainer.className = 'subcard';
    paramsContainer.style.display = 'block'; /* Toujours visible */
    container.appendChild(paramsContainer);

    /* Générer les champs de paramètres selon le type de message MIDI */
    MidiConfig.generateParams(def, paramsContainer, currentCfg);

    /* Gérer le changement de type de message */
    rtpMsgTypeSelect.addEventListener('change', () => {
      MidiConfig.updateVisibility();
      /* Sauvegarder la modification du type de message */
      if (typeof updateConfig === 'function') {
        updateConfig();
      }
    });
  },

  /**
   * Génère une section MIDI pour un axe de joystick (X ou Y)
   * @param {Object} def - Définition du composant
   * @param {Object} currentCfg - Configuration actuelle
   * @param {HTMLElement} container - Conteneur parent
   * @param {string} axis - Axe ('x' ou 'y')
   */
  generateAxisSection(def, currentCfg = {}, container, axis) {
    /* Filtrer les messages MIDI pour cet axe */
    const axisMessages = def.midiMessages.filter(msg => {
      /* Si le message a un champ axis, vérifier qu'il correspond */
      if(msg.axis) {
        return msg.axis === axis;
      }
      /* Sinon, répartir les messages : première moitié pour X, deuxième pour Y */
      const index = def.midiMessages.indexOf(msg);
      const half = Math.floor(def.midiMessages.length / 2);
      return (axis === 'x' && index < half) || (axis === 'y' && index >= half);
    });
    
    if(axisMessages.length === 0) {
      return;
    }
    
    /* Créer un conteneur pour cet axe */
    const axisContainer = document.createElement('div');
    axisContainer.className = 'joystick-axis-section';
    axisContainer.id = `midiSection${axis.toUpperCase()}`;
    axisContainer.style.marginBottom = '20px';
    axisContainer.style.padding = '10px';
    axisContainer.style.border = '1px solid #ccc';
    axisContainer.style.borderRadius = '4px';
    
    /* Titre de la section */
    const title = document.createElement('h3');
    title.textContent = `Configuration MIDI - Axe ${axis.toUpperCase()}`;
    title.style.marginTop = '0';
    title.style.marginBottom = '10px';
    axisContainer.appendChild(title);
    
    /* Créer le wrapper principal (Type de message MIDI) */
    const wrapper = document.createElement('div');
    wrapper.className = 'r';
    
    const typeLabel = document.createElement('label');
    const typeText = '{{t.pins.type}}:';
    typeLabel.textContent = (typeof FormGenerator !== 'undefined' && FormGenerator.replaceTranslationTemplate)
      ? FormGenerator.replaceTranslationTemplate(typeText)
      : typeText.replace(/\{\{t\.pins\.(\w+)\}\}/g, (match, key) => {
          const translations = { 'type': 'Type' };
          return translations[key] || key;
        });
    
    /* Select midiMessageType pour cet axe */
    const rtpMsgTypeSelect = document.createElement('select');
    rtpMsgTypeSelect.id = `rtpMsgType${axis.toUpperCase()}`;
    rtpMsgTypeSelect.setAttribute('data-midi-message-type', 'true');
    rtpMsgTypeSelect.setAttribute('data-axis', axis);
    
    /* Ajouter les options depuis les messages de cet axe */
    axisMessages.forEach(msg => {
      const option = document.createElement('option');
      option.value = msg.displayName;
      option.textContent = msg.displayName;
      rtpMsgTypeSelect.appendChild(option);
    });
    
    /* Sélectionner la valeur actuelle si disponible */
    const cfgKey = `midiMessageType${axis.toUpperCase()}`;
    if(currentCfg[cfgKey]) {
      rtpMsgTypeSelect.value = currentCfg[cfgKey];
    } else if(currentCfg.midiMessageType && axis === 'x') {
      /* Par défaut, utiliser midiMessageType pour X si disponible */
      rtpMsgTypeSelect.value = currentCfg.midiMessageType;
    }
    
    wrapper.appendChild(typeLabel);
    wrapper.appendChild(rtpMsgTypeSelect);
    axisContainer.appendChild(wrapper);
    
    /* Créer le conteneur pour les paramètres */
    const paramsContainer = document.createElement('div');
    paramsContainer.id = `rtpParams${axis.toUpperCase()}`;
    paramsContainer.setAttribute('data-midi-params', 'true');
    paramsContainer.className = 'subcard';
    paramsContainer.style.display = 'block';
    axisContainer.appendChild(paramsContainer);
    
    /* Générer les champs de paramètres pour cet axe */
    /* Créer une définition temporaire avec seulement les messages de cet axe */
    const axisDef = {
      ...def,
      midiMessages: axisMessages
    };
    this.generateParams(axisDef, paramsContainer, currentCfg, axis);
    
    /* Gérer le changement de type de message */
    rtpMsgTypeSelect.addEventListener('change', () => {
      MidiConfig.updateVisibility();
      if (typeof updateConfig === 'function') {
        updateConfig();
      }
    });
    
    container.appendChild(axisContainer);
  },

  /**
   * Génère les champs de paramètres MIDI selon le type de message
   * @param {Object} def - Définition du composant
   * @param {HTMLElement} container - Conteneur pour les paramètres
   * @param {Object} currentCfg - Configuration actuelle
   * @param {string} axis - Axe optionnel ('x' ou 'y') pour préfixer les IDs
   */
  generateParams(def, container, currentCfg = {}, axis = null) {
    /* Vider le conteneur */
    container.innerHTML = '';

    if(!def || !def.midiMessages || def.midiMessages.length === 0) {
      return;
    }

    /* Utiliser une copie du tableau pour éviter les mutations */
    const midiMessages = Array.isArray(def.midiMessages) ? [...def.midiMessages] : [];

    /* Collecter tous les paramètres uniques de tous les messages MIDI */
    const allParams = new Map();
    
    midiMessages.forEach(msg => {
      if(msg.params && Array.isArray(msg.params)) {
        msg.params.forEach(param => {
          if(!allParams.has(param.id)) {
            /* Stocker le paramètre avec le displayName du message pour la visibilité */
            allParams.set(param.id, {
              ...param,
              _showFor: [msg.displayName]
            });
          } else {
            /* Si le paramètre existe déjà, ajouter ce message à _showFor */
            const existing = allParams.get(param.id);
            if(!existing._showFor.includes(msg.displayName)) {
              existing._showFor.push(msg.displayName);
            }
          }
        });
      }
    });

    /* Générer les champs pour chaque paramètre unique */
    allParams.forEach((param, paramId) => {
      const row = document.createElement('div');
      row.className = 'r';
      /* Préfixer l'ID avec l'axe si fourni */
      const fieldIdPrefix = axis ? axis.toUpperCase() + '_' : '';
      row.id = fieldIdPrefix + param.id + 'Row';
      row.style.display = 'none';

      /* Convertir le type numérique en string */
      const fieldType = param.type === 0 ? 'text' : 
                        param.type === 1 ? 'number' : 
                        param.type === 2 ? 'select' : 
                        param.type === 3 ? 'checkbox' : 
                        param.type === 4 ? 'range' : 
                        param.type === 5 ? 'info' : 'text';
      
      if(fieldType === 'info') {
        const hintDiv = document.createElement('div');
        hintDiv.className = param.hintClass || 'hint';
        /* Résoudre le template pour le hint */
        const hintText = param.hint || '';
        hintDiv.textContent = (typeof FormGenerator !== 'undefined' && FormGenerator.replaceTranslationTemplate)
          ? FormGenerator.replaceTranslationTemplate(hintText)
          : hintText.replace(/\{\{t\.pins\.(\w+)\}\}(:?)/g, (match, key, colon) => {
              /* Utiliser la traduction depuis l'objet global si disponible */
              if (typeof translations !== 'undefined' && translations.pins && translations.pins[key]) {
                return translations.pins[key] + (colon || '');
              }
              /* Fallback vers un dictionnaire local */
              const fallbackTranslations = {
                'clockHint': 'Clock / Tap Tempo: pas de canal.'
              };
              return (fallbackTranslations[key] || key) + (colon || '');
            });
        row.appendChild(hintDiv);
      } else if(fieldType === 'range') {
        const label = document.createElement('label');
        const labelText = param.label || '';
        label.textContent = (typeof FormGenerator !== 'undefined' && FormGenerator.replaceTranslationTemplate)
          ? FormGenerator.replaceTranslationTemplate(labelText)
          : labelText.replace(/\{\{t\.pins\.(\w+)\}\}(:?)/g, (match, key, colon) => {
              const translations = {
                'cc': 'CC#', 'channel': 'Canal', 'midiRange': 'Plage MIDI', 'note': 'Note',
                'program': 'Program#', 'velocity': 'Vélocité', 'values': 'Valeurs',
                'sweep': 'Balayage', 'fixedVelocity': 'Vélocité fixe', 'autoOff': 'Auto-off'
              };
              return (translations[key] || key) + (colon || '');
            });
        row.appendChild(label);
        
        const inputMin = document.createElement('input');
        inputMin.type = 'number';
        inputMin.id = fieldIdPrefix + param.id + 'Min';
        inputMin.min = param.min || 0;
        inputMin.max = param.max || 127;
        inputMin.placeholder = param.defaultMin || param.min || 0;
        if(param.width) inputMin.style.width = param.width + 'px';
        /* Toujours initialiser avec une valeur (config actuelle ou valeur par défaut) */
        if(currentCfg[param.id + 'Min'] !== undefined && currentCfg[param.id + 'Min'] !== null && currentCfg[param.id + 'Min'] !== '') {
          inputMin.value = currentCfg[param.id + 'Min'];
        } else {
          inputMin.value = param.defaultMin || param.min || '0';
        }
        
        const separator = document.createElement('span');
        separator.textContent = param.separator || '→';
        separator.style.margin = '0 4px';
        
        const inputMax = document.createElement('input');
        inputMax.type = 'number';
        inputMax.id = fieldIdPrefix + param.id + 'Max';
        inputMax.min = param.min || 0;
        inputMax.max = param.max || 127;
        inputMax.placeholder = param.defaultMax || param.max || 127;
        if(param.width) inputMax.style.width = param.width + 'px';
        /* Toujours initialiser avec une valeur (config actuelle ou valeur par défaut) */
        if(currentCfg[param.id + 'Max'] !== undefined && currentCfg[param.id + 'Max'] !== null && currentCfg[param.id + 'Max'] !== '') {
          inputMax.value = currentCfg[param.id + 'Max'];
        } else {
          inputMax.value = param.defaultMax || param.max || '127';
        }
        
        row.appendChild(inputMin);
        row.appendChild(separator);
        row.appendChild(inputMax);
        
        /* Attacher les event listeners pour sauvegarder les modifications */
        inputMin.addEventListener('change', updateConfig);
        inputMin.addEventListener('input', updateConfig);
        inputMax.addEventListener('change', updateConfig);
        inputMax.addEventListener('input', updateConfig);
      } else {
        const label = document.createElement('label');
        const labelText = param.label || '';
        label.textContent = (typeof FormGenerator !== 'undefined' && FormGenerator.replaceTranslationTemplate)
          ? FormGenerator.replaceTranslationTemplate(labelText)
          : labelText.replace(/\{\{t\.pins\.(\w+)\}\}(:?)/g, (match, key, colon) => {
              const translations = {
                'cc': 'CC#', 'channel': 'Canal', 'midiRange': 'Plage MIDI', 'note': 'Note',
                'program': 'Program#', 'velocity': 'Vélocité', 'values': 'Valeurs',
                'sweep': 'Balayage', 'fixedVelocity': 'Vélocité fixe', 'autoOff': 'Auto-off'
              };
              return (translations[key] || key) + (colon || '');
            });
        row.appendChild(label);
        
        const input = document.createElement('input');
        input.type = fieldType;
        input.id = fieldIdPrefix + param.id;
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
        
        /* Attacher les event listeners pour sauvegarder les modifications */
        input.addEventListener('change', updateConfig);
        input.addEventListener('input', updateConfig);
      }

      /* Stocker les informations pour updateVisibility */
      row._showFor = param._showFor || [];
      /* Parser dependsOnRole si c'est une string JSON, sinon utiliser directement */
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

    /* Mettre à jour la visibilité initiale */
    MidiConfig.updateVisibility();
  },

  /**
   * Met à jour la visibilité des paramètres MIDI selon le type de message et le rôle
   */
  updateVisibility() {
    const roleSel = $('#funcSelect');
    const role = roleSel ? roleSel.value : '';

    /* Pour le joystick, mettre à jour les deux sections séparément */
    const typeSelX = $('#rtpMsgTypeX');
    const paramsX = $('#rtpParamsX');
    if(typeSelX && paramsX) {
      const vX = typeSelX.value;
      const allRowsX = paramsX.querySelectorAll('[id$="Row"]');
      allRowsX.forEach(row => {
        if(row._showFor) {
          const shouldShow = row._showFor.includes(vX);
          if(shouldShow && row._dependsOnRole) {
            row.style.display = row._dependsOnRole.includes(role) ? 'flex' : 'none';
          } else {
            row.style.display = shouldShow ? 'flex' : 'none';
          }
        } else {
          row.style.display = 'none';
        }
      });
    }

    const typeSelY = $('#rtpMsgTypeY');
    const paramsY = $('#rtpParamsY');
    if(typeSelY && paramsY) {
      const vY = typeSelY.value;
      const allRowsY = paramsY.querySelectorAll('[id$="Row"]');
      allRowsY.forEach(row => {
        if(row._showFor) {
          const shouldShow = row._showFor.includes(vY);
          if(shouldShow && row._dependsOnRole) {
            row.style.display = row._dependsOnRole.includes(role) ? 'flex' : 'none';
          } else {
            row.style.display = shouldShow ? 'flex' : 'none';
          }
        } else {
          row.style.display = 'none';
        }
      });
    }

    /* Pour les autres composants, comportement normal */
    const typeSel = $('#rtpMsgType'); /* Compatibilité: garder rtpMsgType */
    const params = $('#rtpParams'); /* Compatibilité: garder rtpParams */
    if(!typeSel || !params) return;

    const v = typeSel.value;

    /* Masquer tous les champs */
    const allRows = params.querySelectorAll('[id$="Row"]');
    allRows.forEach(row => {
      if(row._showFor) {
        /* Vérifier si ce champ doit être affiché pour ce type de message */
        const shouldShow = row._showFor.includes(v);
        if(shouldShow && row._dependsOnRole) {
          /* Vérifier aussi le rôle si nécessaire */
          row.style.display = row._dependsOnRole.includes(role) ? 'flex' : 'none';
        } else {
          row.style.display = shouldShow ? 'flex' : 'none';
        }
      } else {
        row.style.display = 'none';
      }
    });
  },

  /**
   * Lit la configuration MIDI depuis le formulaire
   * Lit dynamiquement tous les paramètres depuis les définitions du composant
   * (ne se limite pas à cc, note, channel - supporte tous les paramètres définis)
   * @param {Object} def - Définition du composant
   * @returns {Object} Configuration MIDI
   */
  readConfig(def) {
    const config = {
      /* Interfaces MIDI (checkboxes HTML, codées en dur OK) */
      rtpMidiEnabled: !!$('#rtpMidiEnabled')?.checked,
      rtpEnabled: !!$('#rtpMidiEnabled')?.checked || !!$('#rtpEnabled2')?.checked, /* Alias pour compatibilité */
      /* USB-MIDI et Debug MIDI sont hardcodés dans HTML (si présents) */
      usbMidiEnabled: !!$('#usbMidiEnabled')?.checked,
      debugMidiEnabled: !!$('#debugMidiEnabled')?.checked
    };

    /* Pour le joystick, lire les configs MIDI séparées pour X et Y */
    if(def && def.id === 'joystick') {
      config.midiMessageTypeX = $('#rtpMsgTypeX')?.value || '';
      config.midiMessageTypeY = $('#rtpMsgTypeY')?.value || '';
      config.rtpTypeX = $('#rtpMsgTypeX')?.value || '';
      config.rtpTypeY = $('#rtpMsgTypeY')?.value || '';
    } else {
      /* Pour les autres composants, comportement normal */
      config.midiMessageType = $('#rtpMsgType')?.value || '';
      config.rtpType = $('#rtpMsgType')?.value || '';
    }

    /* Pour le joystick, lire les paramètres avec préfixes X_ et Y_ */
    if(def && def.id === 'joystick') {
      /* Lire les paramètres pour l'axe X */
      const paramsX = $('#rtpParamsX');
      if(paramsX && def.midiMessages) {
        def.midiMessages.filter(m => !m.axis || m.axis === 'x').forEach(msg => {
          if(msg.params && Array.isArray(msg.params)) {
            msg.params.forEach(param => {
              if(param.id) {
                const fieldId = 'X_' + param.id;
                if(param.type === 4) { /* RANGE */
                  const elMin = $('#' + fieldId + 'Min');
                  const elMax = $('#' + fieldId + 'Max');
                  if(elMin) config[fieldId + 'Min'] = elMin.value || param.defaultMin || param.min || '0';
                  if(elMax) config[fieldId + 'Max'] = elMax.value || param.defaultMax || param.max || '127';
                } else {
                  const el = $('#' + fieldId);
                  if(el) config[fieldId] = el.value || '';
                }
              }
            });
          }
        });
      }
      
      /* Lire les paramètres pour l'axe Y */
      const paramsY = $('#rtpParamsY');
      if(paramsY && def.midiMessages) {
        def.midiMessages.filter(m => !m.axis || m.axis === 'y').forEach(msg => {
          if(msg.params && Array.isArray(msg.params)) {
            msg.params.forEach(param => {
              if(param.id) {
                const fieldId = 'Y_' + param.id;
                if(param.type === 4) { /* RANGE */
                  const elMin = $('#' + fieldId + 'Min');
                  const elMax = $('#' + fieldId + 'Max');
                  if(elMin) config[fieldId + 'Min'] = elMin.value || param.defaultMin || param.min || '0';
                  if(elMax) config[fieldId + 'Max'] = elMax.value || param.defaultMax || param.max || '127';
                } else {
                  const el = $('#' + fieldId);
                  if(el) config[fieldId] = el.value || '';
                }
              }
            });
          }
        });
      }
    } else {
      /* Pour les autres composants, comportement normal */
      /* Lire dynamiquement tous les paramètres MIDI depuis les définitions */
      /* Parcourt tous les messages MIDI et leurs paramètres (cc, note, channel, velocity, range, etc.) */
      if(def && def.midiMessages && Array.isArray(def.midiMessages)) {
        def.midiMessages.forEach(msg => {
          if(msg.params && Array.isArray(msg.params)) {
            msg.params.forEach(param => {
              if(param.id) {
                /* Pour les paramètres RANGE, traiter différemment car il n'y a pas d'élément avec param.id */
                if(param.type === 4) { /* RANGE */
                  const elMin = $('#' + param.id + 'Min');
                  const elMax = $('#' + param.id + 'Max');
                  /* Toujours collecter les valeurs (même si par défaut ou cachées) */
                  if(elMin) {
                    const minValue = elMin.value || (param.defaultMin ? String(param.defaultMin) : (param.min ? String(param.min) : '0'));
                    config[param.id + 'Min'] = minValue;
                  } else if(param.defaultMin) {
                    /* Si le champ n'existe pas, utiliser la valeur par défaut */
                    config[param.id + 'Min'] = String(param.defaultMin);
                  } else {
                    /* Fallback final */
                    config[param.id + 'Min'] = String(param.min || '0');
                  }
                  if(elMax) {
                    const maxValue = elMax.value || (param.defaultMax ? String(param.defaultMax) : (param.max ? String(param.max) : '127'));
                    config[param.id + 'Max'] = maxValue;
                  } else if(param.defaultMax) {
                    /* Si le champ n'existe pas, utiliser la valeur par défaut */
                    config[param.id + 'Max'] = String(param.defaultMax);
                  } else {
                    /* Fallback final */
                    config[param.id + 'Max'] = String(param.max || '127');
                  }
                } else {
                  /* Pour les autres types, chercher l'élément normal */
                  const el = $('#' + param.id);
                  if(el) {
                    config[param.id] = el.value || '';
                  }
                }
              }
            });
          }
        });
      }
    }
    
    return config;
  },

  /**
   * Applique la configuration MIDI au formulaire
   * Applique dynamiquement tous les paramètres depuis les définitions du composant
   * (ne se limite pas à cc, note, channel - supporte tous les paramètres définis)
   * @param {Object} cfg - Configuration MIDI à appliquer
   * @param {Object} def - Définition du composant
   */
  applyConfig(cfg, def) {
    if(!cfg) return;
    
    const setV = (id, v) => {
      const el = $(typeof id === 'string' && id[0] === '#' ? id : '#' + id);
      if(el && v != null && v !== '') el.value = v;
    };
    const setC = (id, b) => {
      const el = $(typeof id === 'string' && id[0] === '#' ? id : '#' + id);
      if(el) el.checked = !!b;
    };
    
    /* Pour le joystick, appliquer les configs MIDI séparées pour X et Y */
    if(def && def.id === 'joystick') {
      if(cfg.midiMessageTypeX) {
        setV('rtpMsgTypeX', cfg.midiMessageTypeX);
      } else if(cfg.rtpTypeX) {
        setV('rtpMsgTypeX', cfg.rtpTypeX);
      }
      if(cfg.midiMessageTypeY) {
        setV('rtpMsgTypeY', cfg.midiMessageTypeY);
      } else if(cfg.rtpTypeY) {
        setV('rtpMsgTypeY', cfg.rtpTypeY);
      }
    } else {
      /* Pour les autres composants, comportement normal */
      /* Appliquer le type de message (nouveau format puis ancien pour compatibilité) */
      if(cfg.midiMessageType) {
        setV('rtpMsgType', cfg.midiMessageType);
      } else if(cfg.rtpType) {
        setV('rtpMsgType', cfg.rtpType); /* Compatibilité ancien format */
      }
    }

    /* Appliquer les interfaces MIDI */
    if(cfg.rtpMidiEnabled !== undefined) setC('rtpMidiEnabled', cfg.rtpMidiEnabled);
    if(cfg.rtpEnabled !== undefined) setC('rtpMidiEnabled', cfg.rtpEnabled); /* Alias pour compatibilité (rtpEnabled -> rtpMidiEnabled) */
    if(cfg.usbMidiEnabled !== undefined) setC('usbMidiEnabled', cfg.usbMidiEnabled);
    if(cfg.debugMidiEnabled !== undefined) setC('debugMidiEnabled', cfg.debugMidiEnabled);

    /* Appliquer dynamiquement tous les paramètres MIDI depuis les définitions */
    /* Parcourt tous les messages MIDI et leurs paramètres (cc, note, channel, velocity, range, etc.) */
    if(def && def.midiMessages && Array.isArray(def.midiMessages)) {
      if(def.id === 'joystick') {
        /* Pour le joystick, appliquer les paramètres avec préfixes X_ et Y_ */
        def.midiMessages.filter(m => !m.axis || m.axis === 'x').forEach(msg => {
          if(msg.params && Array.isArray(msg.params)) {
            msg.params.forEach(param => {
              if(param.id) {
                const fieldId = 'X_' + param.id;
                if(param.type === 4) { /* RANGE */
                  if(cfg[fieldId + 'Min'] !== undefined) setV(fieldId + 'Min', cfg[fieldId + 'Min']);
                  if(cfg[fieldId + 'Max'] !== undefined) setV(fieldId + 'Max', cfg[fieldId + 'Max']);
                } else {
                  if(cfg[fieldId] !== undefined) setV(fieldId, cfg[fieldId]);
                }
              }
            });
          }
        });
        def.midiMessages.filter(m => !m.axis || m.axis === 'y').forEach(msg => {
          if(msg.params && Array.isArray(msg.params)) {
            msg.params.forEach(param => {
              if(param.id) {
                const fieldId = 'Y_' + param.id;
                if(param.type === 4) { /* RANGE */
                  if(cfg[fieldId + 'Min'] !== undefined) setV(fieldId + 'Min', cfg[fieldId + 'Min']);
                  if(cfg[fieldId + 'Max'] !== undefined) setV(fieldId + 'Max', cfg[fieldId + 'Max']);
                } else {
                  if(cfg[fieldId] !== undefined) setV(fieldId, cfg[fieldId]);
                }
              }
            });
          }
        });
      } else {
        /* Pour les autres composants, comportement normal */
        def.midiMessages.forEach(msg => {
          if(msg.params && Array.isArray(msg.params)) {
            msg.params.forEach(param => {
              if(param.id && cfg[param.id] !== undefined) {
                if(param.type === 4) { /* RANGE */
                  if(cfg[param.id + 'Min'] !== undefined) setV(param.id + 'Min', cfg[param.id + 'Min']);
                  if(cfg[param.id + 'Max'] !== undefined) setV(param.id + 'Max', cfg[param.id + 'Max']);
                } else {
                  setV(param.id, cfg[param.id]);
                }
              }
            });
          }
        });
      }
    }

    /* Mettre à jour la visibilité des paramètres */
    MidiConfig.updateVisibility();
  }
};
