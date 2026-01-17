/**
 * Gestion des composants complexes (générique pour tous les types)
 * Module générique pour la gestion de tous les types de composants complexes
 * 
 * Responsabilités:
 * - Chargement et sauvegarde des composants complexes
 * - Initialisation des formulaires depuis les pins
 * - Gestion de la liste des composants complexes
 * - Générique pour tous les types de composants complexes
 */

const ComplexComponents = {
  /* Liste interne des composants complexes */
  _list: [],

  /**
   * Getter pour la liste des composants complexes
   * @returns {Array} Liste des composants complexes
   */
  get list() {
    return this._list;
  },

  /**
   * Obtient l'endpoint API pour un composant complexe donné
   * Utilise les endpoints définis dans la définition du composant si disponibles,
   * sinon utilise les endpoints par défaut (/api/mux/*) pour compatibilité
   * @param {string} operation - 'list', 'add', ou 'delete'
   * @param {Object} componentDef - Définition du composant (optionnel)
   * @returns {string} URL de l'endpoint API
   */
  _getApiEndpoint(operation, componentDef = null) {
    /* TODO: Quand le backend expose les endpoints dans ComponentDefinition,
     * utiliser componentDef.apiEndpoints[operation] au lieu du hardcoding */
    /* Pour l'instant, utiliser /api/mux/* comme fallback */
    /* Format futur attendu : componentDef.apiEndpoints = { list: '/api/mux/list', add: '/api/mux/add', delete: '/api/mux/delete' } */
    if(componentDef && componentDef.apiEndpoints && componentDef.apiEndpoints[operation]) {
      return componentDef.apiEndpoints[operation];
    }
    /* Fallback: endpoints par défaut (historique) */
    return `/api/mux/${operation}`;
  },

  /**
   * Charge la liste des composants complexes depuis le backend
   * @param {Object} componentDef - Définition du composant (optionnel, pour utiliser les endpoints personnalisés)
   * @returns {Promise<void>}
   */
  async loadList(componentDef = null) {
    try {
      const endpoint = this._getApiEndpoint('list', componentDef);
      const r = await fetch(endpoint);
      if(!r.ok) {
        console.error('Erreur HTTP:', r.status, r.statusText);
        this._list = [];
        this._updateUI();
        return;
      }
      const text = await r.text();
      if(!text || text.trim().length === 0) {
        console.warn(`Réponse vide de ${endpoint}`);
        this._list = [];
        this._updateUI();
        return;
      }
      const d = JSON.parse(text);
      /* TODO: Quand le backend expose une propriété dynamique dans ComponentDefinition,
       * utiliser componentDef.apiListProperty au lieu de 'muxes' */
      /* Format futur attendu : componentDef.apiListProperty = 'muxes' ou 'components' */
      /* Pour l'instant, utiliser 'muxes' comme fallback (historique) */
      const listProperty = componentDef && componentDef.apiListProperty ? componentDef.apiListProperty : 'muxes';
      this._list = d[listProperty] || [];
      this._updateUI();
    } catch(e) {
      console.error('Erreur chargement composants complexes:', e);
      this._list = [];
      this._updateUI();
    }
  },

  /**
   * Met à jour l'UI après chargement de la liste
   * @private
   */
  _updateUI() {
    if(typeof updatePinsList === 'function') updatePinsList();
    if(caps && caps.pins && typeof drawBoard === 'function') drawBoard();
    if(typeof updateBusVisuals === 'function') updateBusVisuals();
  },

  /**
   * Initialise le formulaire d'un composant complexe depuis un pin
   * @param {string} pinLabel - Label de la pin (ex: "A0")
   */
  initForm(pinLabel) {
    if(!caps || !caps.pins) return;
    const pin = caps.pins.find(p => p.label === pinLabel);
    if(!pin) return;
    
    const funcSelectValue = $('#funcSelect')?.value || '';
    const migratedRoleValue = typeof migrateRole === 'function' ? migrateRole(funcSelectValue) : funcSelectValue;
    const def = typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.getById
      ? ComponentDefinitions.getById(migratedRoleValue)
      : null;
    
    if(!def || !def.isComplex) {
      console.warn('[ComplexComponents.initForm] Composant non complexe ou définition non trouvée');
      return;
    }
    
    const sigGpio = parseInt(pin.gpio);
    if(typeof GpioManager === 'undefined' || !GpioManager.getUsedGpios) {
      console.warn('[ComplexComponents.initForm] GpioManager non disponible');
      return;
    }
    const usedGpios = GpioManager.getUsedGpios([]);
    
    const existingComplex = this._list.find(m => m.sig === sigGpio);
    if(existingComplex) {
      this.loadConfig(existingComplex);
    } else {
      if(typeof FormGenerator === 'undefined' || !FormGenerator.getFieldId) {
        console.warn('[ComplexComponents.initForm] FormGenerator non disponible');
        return;
      }
      
      /* Initialiser toutes les additionalPins dynamiquement */
      if(def.additionalPins && Array.isArray(def.additionalPins)) {
        def.additionalPins.forEach(additionalPin => {
          if(!additionalPin.id) return;
          const fieldId = FormGenerator.getFieldId(def, additionalPin.id);
          const field = fieldId ? $('#' + fieldId) : null;
          if(!field) return;
          
          /* Pin principale (SIG) - utiliser le GPIO de la pin sélectionnée */
          if(additionalPin.id === 'sig') {
            field.value = sigGpio;
          }
          /* Pins d'adresse - calculer automatiquement si disponible (spécifique aux multiplexeurs) */
          else if(additionalPin.id === 's0' || additionalPin.id === 's1' || additionalPin.id === 's2' || additionalPin.id === 's3') {
            if(typeof GpioManager.calculateAddressPins === 'function') {
              const addrPins = GpioManager.calculateAddressPins(sigGpio, usedGpios);
              if(addrPins && addrPins[additionalPin.id] !== null && addrPins[additionalPin.id] !== undefined) {
                field.value = addrPins[additionalPin.id];
              }
            }
          }
          /* Pins optionnelles - utiliser la valeur par défaut */
          else if(additionalPin.optional) {
            field.value = additionalPin.defaultValue !== undefined ? additionalPin.defaultValue : '255';
          }
          /* Autres pins - utiliser la valeur par défaut */
          else if(additionalPin.defaultValue !== undefined) {
            field.value = additionalPin.defaultValue;
          }
        });
      }
      
      /* Initialiser l'ID du composant */
      const idFieldId = FormGenerator.getFieldId(def, 'id');
      const idField = idFieldId ? $('#' + idFieldId) : null;
      if(idField) {
        const usedIds = this._list.map(m => parseInt(m.id)).filter(id => !isNaN(id));
        const availableId = [0, 1].find(id => !usedIds.includes(id));
        if(availableId !== undefined) {
          idField.value = availableId;
        }
      }
      
      /* Initialiser l'adresse OSC avec un préfixe basé sur l'ID du composant */
      const oscField = $('#oscAddress');
      if(oscField && idField) {
        const prefix = def.id ? def.id : 'complex';
        oscField.value = '/' + prefix + (idField.value || '0');
      }
      
      if(typeof updateBusVisuals === 'function') updateBusVisuals();
    }
  },

  /**
   * Charge une configuration de composant complexe dans le formulaire
   * @param {Object} component - Objet composant complexe depuis le backend
   */
  loadConfig(component) {
    const complexDefs = typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.cache
      ? ComponentDefinitions.cache.filter(d => d.isComplex && d.implemented)
      : [];
    const def = complexDefs.length > 0 ? complexDefs[0] : null;
    
    if(!def) {
      console.warn('[ComplexComponents.loadConfig] Définition de composant complexe non trouvée');
      return;
    }
    
    const fieldMap = {};
    
    /* ID du composant */
    const idFieldId = typeof FormGenerator !== 'undefined' && FormGenerator.getFieldId
      ? FormGenerator.getFieldId(def, 'id')
      : '';
    if(idFieldId) fieldMap[idFieldId] = component.id;
    
    /* Mapper toutes les additionalPins dynamiquement */
    if(def.additionalPins && Array.isArray(def.additionalPins)) {
      def.additionalPins.forEach(additionalPin => {
        if(!additionalPin.id) return;
        const fieldId = FormGenerator.getFieldId(def, additionalPin.id);
        if(!fieldId) return;
        /* Mapper depuis l'objet component (propriétés correspondent aux IDs des additionalPins) */
        if(component[additionalPin.id] !== undefined) {
          fieldMap[fieldId] = component[additionalPin.id];
        } else if(additionalPin.optional && component[additionalPin.id] === undefined) {
          fieldMap[fieldId] = 255; /* Non connecté par défaut pour pins optionnelles */
        }
      });
    }
    
    /* Mapper tous les formFields dynamiquement */
    /* Note: Utilise FormGenerator.getFieldId() car les IDs sont préfixés (ex: hc4067Min) */
    if(def.formFields && Array.isArray(def.formFields)) {
      def.formFields.forEach(field => {
        if(!field.id || field.id.startsWith('_')) return;
        const fieldId = FormGenerator.getFieldId(def, field.id);
        if(!fieldId) return;
        if(component[field.id] !== undefined) {
          fieldMap[fieldId] = component[field.id];
        }
      });
    }
    
    /* Appliquer les formFields via fieldMap */
    Object.keys(fieldMap).forEach(fieldId => {
      const field = $('#' + fieldId);
      if(field) {
        const value = fieldMap[fieldId];
        if(field.type === 'checkbox') {
          field.checked = value === true || value === 'true';
        } else {
          field.value = value;
        }
      }
    });
    
    /* Créer un objet config compatible pour MidiConfig.applyConfig() */
    /* Mapping backend → frontend: ccBase → rtpCc, midiChan → rtpChan, oscBase → oscAddress */
    const midiConfig = {};
    if(component.ccBase !== undefined) midiConfig.rtpCc = component.ccBase;
    if(component.midiChan !== undefined) midiConfig.rtpChan = component.midiChan;
    if(component.oscBase !== undefined) midiConfig.oscAddress = component.oscBase;
    if(component.oscFormat !== undefined) midiConfig.oscFormat = component.oscFormat;
    
    /* Appliquer les paramètres MIDI/OSC via MidiConfig (réutilise la logique de components.js) */
    if(typeof MidiConfig !== 'undefined' && MidiConfig.applyConfig) {
      MidiConfig.applyConfig(midiConfig, def);
    }
    
    /* Appliquer les paramètres OSC/Debug manuellement (comme dans components.js applyCfg) */
    if(component.oscBase !== undefined) {
      const oscField = $('#oscAddress');
      if(oscField) oscField.value = component.oscBase;
    }
    if(component.oscFormat !== undefined) {
      const oscFormatField = $('#oscFormat');
      if(oscFormatField) oscFormatField.value = component.oscFormat;
    }
    
    if(typeof updateBusVisuals === 'function') updateBusVisuals();
  },

  /**
   * Sauvegarde un composant complexe depuis le formulaire de pin
   * @returns {Promise<void>}
   */
  async saveFromPin() {
    const funcSelectValue = $('#funcSelect')?.value || '';
    const migratedRoleValue = typeof migrateRole === 'function' ? migrateRole(funcSelectValue) : funcSelectValue;
    const def = typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.getById
      ? ComponentDefinitions.getById(migratedRoleValue)
      : null;
    
    if(!def || !def.isComplex) {
      console.warn('[ComplexComponents.saveFromPin] Définition non trouvée ou composant non complexe');
      return;
    }
    
    /* Lire l'ID du composant complexe */
    let complexId = null;
    const idFieldId = typeof FormGenerator !== 'undefined' && FormGenerator.getFieldId
      ? FormGenerator.getFieldId(def, 'id')
      : '';
    const idField = idFieldId ? $('#' + idFieldId) : null;
    if(idField) {
      complexId = idField.value;
    } else {
      const existingIds = this._list.map(m => parseInt(m.id)).filter(id => !isNaN(id));
      complexId = existingIds.length > 0 ? Math.max(...existingIds) + 1 : 0;
    }
    
    /* Lire toutes les additionalPins dynamiquement */
    const additionalPinValues = {};
    let sigPinValue = null;
    if(def.additionalPins && Array.isArray(def.additionalPins)) {
      def.additionalPins.forEach(additionalPin => {
        if(!additionalPin.id) return;
        const fieldId = FormGenerator.getFieldId(def, additionalPin.id);
        const field = fieldId ? $('#' + fieldId) : null;
        if(field && field.value) {
          const value = parseInt(field.value);
          if(!isNaN(value)) {
            additionalPinValues[additionalPin.id] = value;
            if(additionalPin.id === 'sig') sigPinValue = value;
          }
        } else if(additionalPin.defaultValue !== undefined && additionalPin.defaultValue !== null) {
          additionalPinValues[additionalPin.id] = parseInt(additionalPin.defaultValue);
        } else if(additionalPin.optional) {
          additionalPinValues[additionalPin.id] = 255; /* Non connecté par défaut */
        }
      });
    }
    
    /* Fallback : lire la pin SIG depuis la pin sélectionnée si le champ n'a pas de valeur */
    if(!sigPinValue || isNaN(sigPinValue)) {
      const currentPinLabel = $('#selPin')?.textContent || '';
      const currentPin = caps?.pins?.find(p => p.label === currentPinLabel);
      if(currentPin && currentPin.gpio) {
        sigPinValue = parseInt(currentPin.gpio);
        additionalPinValues.sig = sigPinValue;
        console.log(`[ComplexComponents.saveFromPin] SIG lu depuis pin sélectionnée: ${currentPinLabel} (GPIO${sigPinValue})`);
      }
    }
    
    /* Valider que le pin SIG est défini */
    if(!sigPinValue || isNaN(sigPinValue)) {
      console.warn('[ComplexComponents.saveFromPin] Pin principale (SIG) non définie');
      console.warn('[ComplexComponents.saveFromPin] Debug: selPin=', $('#selPin')?.textContent, 'caps.pins=', caps?.pins?.length);
      return;
    }
    
    /* Lire tous les formFields dynamiquement */
    const formFieldValues = {};
    if(def.formFields && Array.isArray(def.formFields)) {
      def.formFields.forEach(field => {
        if(!field.id || field.id.startsWith('_')) return;
        const fieldId = FormGenerator.getFieldId(def, field.id);
        const el = fieldId ? $('#' + fieldId) : null;
        if(!el) return;
        
        if(field.type === 3) { /* CHECKBOX */
          formFieldValues[field.id] = el.checked ? 'true' : 'false';
        } else if(field.type === 4) { /* RANGE */
          const elMin = $('#' + fieldId + 'Min');
          const elMax = $('#' + fieldId + 'Max');
          if(elMin) formFieldValues[field.id + 'Min'] = elMin.value || field.defaultValue || '0';
          if(elMax) formFieldValues[field.id + 'Max'] = elMax.value || field.defaultValue || '4095';
        } else {
          formFieldValues[field.id] = el.value || field.defaultValue || '';
        }
      });
    }
    
    /* Construire FormData dynamiquement depuis les définitions */
    const formData = new URLSearchParams();
    formData.append('id', complexId);
    
    /* Ajouter toutes les additionalPins */
    Object.keys(additionalPinValues).forEach(pinId => {
      formData.append(pinId, additionalPinValues[pinId]);
    });
    
    /* Ajouter tous les formFields */
    Object.keys(formFieldValues).forEach(fieldId => {
      formData.append(fieldId, formFieldValues[fieldId]);
    });
    
    /* Ajouter les paramètres MIDI/OSC via MidiConfig.readConfig() (réutilise la logique de components.js) */
    let midiConfig = {};
    if(typeof MidiConfig !== 'undefined' && MidiConfig.readConfig) {
      midiConfig = MidiConfig.readConfig(def);
    }
    
    /* Lire OSC/Debug manuellement (comme dans components.js readCfg) */
    const oscAddress = $('#oscAddress')?.value || '';
    const oscFormat = $('#oscFormat')?.value || 'float';
    
    /* Mapping frontend → backend: rtpCc → ccBase, rtpChan → midiChan, oscAddress → oscBase */
    /* Note: MidiConfig.readConfig() retourne rtpCc, rtpChan, etc. */
    if(midiConfig.rtpCc !== undefined) formData.append('ccBase', midiConfig.rtpCc);
    if(midiConfig.rtpChan !== undefined) formData.append('midiChan', midiConfig.rtpChan);
    if(oscAddress) formData.append('oscBase', oscAddress);
    if(oscFormat) formData.append('oscFormat', oscFormat);
    
    const endpoint = this._getApiEndpoint('add', def);
    try {
      const r = await fetch(endpoint, {method: 'POST', body: formData});
      const d = await r.json();
      if(d.status === 'ok') {
        console.log('[ComplexComponents.saveFromPin] Composant complexe enregistré avec succès');
        
        /* Supprimer l'entrée de pcfg pour la pin principale (peu importe le type de composant précédent) */
        /* Un composant complexe remplace toujours le composant simple sur cette pin */
        if(caps && caps.pins) {
          const sigPin = caps.pins.find(p => p.gpio === sigPinValue);
          if(sigPin && sigPin.label && pcfg[sigPin.label]) {
            delete pcfg[sigPin.label];
            console.log(`[ComplexComponents.saveFromPin] Suppression de pcfg[${sigPin.label}] pour le composant complexe`);
          }
        }
        await this.loadList();
        if(typeof loadCaps === 'function') await loadCaps();
        if(typeof updatePinsList === 'function') updatePinsList();
        if(typeof updateBusVisuals === 'function') updateBusVisuals();
      } else {
        console.error('[ComplexComponents.saveFromPin] Erreur:', d.error || 'Inconnu');
      }
    } catch(e) {
      console.error('[ComplexComponents.saveFromPin] Erreur réseau:', e);
    }
  },

  /**
   * Supprime un composant complexe
   * @param {number} id - ID du composant complexe
   * @param {Event} event - Événement (optionnel, pour stopPropagation)
   * @param {Object} componentDef - Définition du composant (optionnel, pour utiliser les endpoints personnalisés)
   * @returns {Promise<void>}
   */
  async delete(id, event, componentDef = null) {
    if(event) event.stopPropagation();
    if(!componentDef) {
      componentDef = typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.cache
        ? ComponentDefinitions.cache.find(d => d.isComplex && d.implemented)
        : null;
    }
    const componentName = componentDef ? componentDef.displayName : 'composant complexe';
    if(!confirm(`Supprimer le ${componentName} ${id} ?`)) return;
    const formData = new URLSearchParams();
    formData.append('id', id);
    const endpoint = this._getApiEndpoint('delete', componentDef);
    try {
      await fetch(endpoint, {method: 'POST', body: formData});
      await this.loadList();
      if(typeof loadCaps === 'function') await loadCaps();
      if(typeof updatePinsList === 'function') updatePinsList();
      if(typeof updateBusVisuals === 'function') updateBusVisuals();
    } catch(e) {
      console.error('[ComplexComponents.delete] Erreur suppression:', e);
    }
  }
};

/* Compatibilité: créer une variable globale muxList qui pointe vers ComplexComponents.list */
/* Cela permet de garder la compatibilité avec le code existant qui utilise muxList */
if(typeof window !== 'undefined') {
  Object.defineProperty(window, 'muxList', {
    get: () => ComplexComponents.list,
    set: (value) => { ComplexComponents._list = value; },
    configurable: true
  });
}
