/**
 * Configuration des messages MIDI (générique)
 * Module refactorisé depuis components.js - Phase 1.3
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
    messageType: 'midiMessageType',  // Renommé depuis rtpMsgType (compatibilité: rtpMsgType)
    params: 'midiParams',            // Renommé depuis rtpParams (compatibilité: rtpParams)
    section: 'midiMessageSection',   // Renommé depuis rtpMidiSection (compatibilité: rtpMidiSection)
    enabled: 'rtpMidiEnabled',       // Checkbox RTP-MIDI (hardcodée dans HTML)
    enabled2: 'rtpEnabled2'          // Alias pour compatibilité (peut être supprimé plus tard)
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
    const container = $('#' + containerId);
    if(!container) {
      console.warn('[MidiConfig.generateMessageSection] Conteneur non trouvé:', containerId);
      return;
    }
    
    // Vider le conteneur
    container.innerHTML = '';
    
    // Si le composant ne supporte pas MIDI, ne rien afficher
    if(!def || !def.supportsMidi || !def.midiMessages || def.midiMessages.length === 0) {
      return;
    }
    
    // Créer le wrapper principal (Type de message MIDI)
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
    
    // Select midiMessageType (compatibilité: rtpMsgType)
    const rtpMsgTypeSelect = document.createElement('select');
    rtpMsgTypeSelect.id = 'rtpMsgType'; // Garder rtpMsgType pour compatibilité
    rtpMsgTypeSelect.setAttribute('data-midi-message-type', 'true'); // Marquer pour migration future
    
    // Ajouter les options depuis midiMessages
    def.midiMessages.forEach(msg => {
      const option = document.createElement('option');
      option.value = msg.displayName;
      option.textContent = msg.displayName;
      rtpMsgTypeSelect.appendChild(option);
    });
    
    // Sélectionner la valeur actuelle si disponible (rtpType ou midiMessageType)
    if(currentCfg.rtpType || currentCfg.midiMessageType) {
      rtpMsgTypeSelect.value = currentCfg.rtpType || currentCfg.midiMessageType;
    }
    
    wrapper.appendChild(typeLabel);
    wrapper.appendChild(rtpMsgTypeSelect);
    container.appendChild(wrapper);
    
    // Créer le conteneur pour les paramètres (compatibilité: rtpParams)
    // Le formulaire de configuration MIDI est toujours visible, indépendamment de l'activation de RTP-MIDI
    // car les paramètres peuvent être utilisés par USB-MIDI, Debug MIDI, ou OSC
    const paramsContainer = document.createElement('div');
    paramsContainer.id = 'rtpParams'; // Garder rtpParams pour compatibilité
    paramsContainer.setAttribute('data-midi-params', 'true'); // Marquer pour migration future
    paramsContainer.className = 'subcard';
    paramsContainer.style.display = 'block'; // Toujours visible
    container.appendChild(paramsContainer);
    
    // Générer les champs de paramètres selon le type de message MIDI
    MidiConfig.generateParams(def, paramsContainer, currentCfg);
    
    // Gérer le changement de type de message
    rtpMsgTypeSelect.addEventListener('change', () => {
      MidiConfig.updateVisibility();
    });
  },

  /**
   * Génère les champs de paramètres MIDI selon le type de message
   * @param {Object} def - Définition du composant
   * @param {HTMLElement} container - Conteneur pour les paramètres
   * @param {Object} currentCfg - Configuration actuelle
   */
  generateParams(def, container, currentCfg = {}) {
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
      
      // Stocker les informations pour updateVisibility
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
    MidiConfig.updateVisibility();
  },

  /**
   * Met à jour la visibilité des paramètres MIDI selon le type de message et le rôle
   */
  updateVisibility() {
    const typeSel = $('#rtpMsgType'); // Compatibilité: garder rtpMsgType
    const params = $('#rtpParams'); // Compatibilité: garder rtpParams
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
      // Compatibilité: rtpType et midiMessageType
      rtpType: $('#rtpMsgType')?.value || '',
      midiMessageType: $('#rtpMsgType')?.value || '',
      // Interfaces MIDI (checkboxes HTML, codées en dur OK)
      rtpMidiEnabled: !!$('#rtpMidiEnabled')?.checked,
      rtpEnabled: !!$('#rtpMidiEnabled')?.checked || !!$('#rtpEnabled2')?.checked, // Alias pour compatibilité
      // USB-MIDI et Debug MIDI sont hardcodés dans HTML (si présents)
      usbMidiEnabled: !!$('#usbMidiEnabled')?.checked,
      debugMidiEnabled: !!$('#debugMidiEnabled')?.checked
    };
    
    // Lire dynamiquement tous les paramètres MIDI depuis les définitions
    // Parcourt tous les messages MIDI et leurs paramètres (cc, note, channel, velocity, range, etc.)
    if(def && def.midiMessages && Array.isArray(def.midiMessages)) {
      def.midiMessages.forEach(msg => {
        if(msg.params && Array.isArray(msg.params)) {
          msg.params.forEach(param => {
            if(param.id) {
              const el = $('#' + param.id);
              if(el) {
                if(param.type === 4) { // RANGE (velocity range, sweep range, etc.)
                  const elMin = $('#' + param.id + 'Min');
                  const elMax = $('#' + param.id + 'Max');
                  if(elMin) config[param.id + 'Min'] = elMin.value || '';
                  if(elMax) config[param.id + 'Max'] = elMax.value || '';
                } else {
                  config[param.id] = el.value || '';
                }
              }
            }
          });
        }
      });
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
    
    // Appliquer le type de message (compatibilité: rtpType et midiMessageType)
    if(cfg.rtpType || cfg.midiMessageType) {
      setV('rtpMsgType', cfg.rtpType || cfg.midiMessageType);
    }
    
    // Appliquer les interfaces MIDI
    if(cfg.rtpMidiEnabled !== undefined) setC('rtpMidiEnabled', cfg.rtpMidiEnabled);
    if(cfg.rtpEnabled !== undefined) setC('rtpMidiEnabled', cfg.rtpEnabled); // Alias pour compatibilité (rtpEnabled -> rtpMidiEnabled)
    if(cfg.usbMidiEnabled !== undefined) setC('usbMidiEnabled', cfg.usbMidiEnabled);
    if(cfg.debugMidiEnabled !== undefined) setC('debugMidiEnabled', cfg.debugMidiEnabled);
    
    // Appliquer dynamiquement tous les paramètres MIDI depuis les définitions
    // Parcourt tous les messages MIDI et leurs paramètres (cc, note, channel, velocity, range, etc.)
    if(def && def.midiMessages && Array.isArray(def.midiMessages)) {
      def.midiMessages.forEach(msg => {
        if(msg.params && Array.isArray(msg.params)) {
          msg.params.forEach(param => {
            if(param.id && cfg[param.id] !== undefined) {
              if(param.type === 4) { // RANGE
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
    
    // Mettre à jour la visibilité des paramètres
    MidiConfig.updateVisibility();
  }
};
