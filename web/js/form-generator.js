/**
 * Génération de formulaires HTML dynamiques
 * Module refactorisé depuis components.js - Phase 1.2
 * 
 * Responsabilités:
 * - Génération de champs de formulaire depuis FormFieldDef
 * - Génération de pins additionnelles depuis AdditionalPinDef
 * - Gestion des IDs dynamiques (préfixés par componentId)
 * - Affichage conditionnel (dependsOn, showWhen)
 * - Remplacement des templates de traduction
 */

const FormGenerator = {
  /**
   * Remplace les templates de traduction {{t.pins.xxx}} par les valeurs traduites
   * @param {string} text - Texte avec templates
   * @returns {string} Texte avec templates remplacés
   */
  replaceTranslationTemplate(text) {
    if(!text) return '';
    
    // Mapping des traductions (extrait de fr.json)
    const translations = {
      'cc': 'CC#',
      'channel': 'Canal',
      'midiRange': 'Plage MIDI',
      'note': 'Note',
      'program': 'Program#',
      'velocity': 'Vélocité',
      'values': 'Valeurs',
      'sweep': 'Balayage',
      'fixedVelocity': 'Vélocité fixe',
      'autoOff': 'Auto-off',
      'activate': 'Activer',
      'type': 'Type'
    };
    
    // Remplacer {{t.pins.xxx}}: (avec :) et {{t.pins.xxx}} (sans :)
    return text.replace(/\{\{t\.pins\.(\w+)\}\}(:?)/g, (match, key, colon) => {
      const translation = translations[key] || key;
      return translation + (colon || '');
    });
  },

  /**
   * Construit l'ID d'un champ HTML depuis la définition du composant et l'ID de la pin/field
   * @param {Object} def - Définition du composant
   * @param {string} fieldId - ID de la pin additionnelle ou du formField (ex: "s0", "sig", "en")
   * @returns {string} ID du champ HTML (ex: "hc4067S0", "hc4067Sig")
   */
  getFieldId(def, fieldId) {
    if(!def || !fieldId) return '';
    const prefix = def.id ? def.id : 'comp';
    return prefix + fieldId.charAt(0).toUpperCase() + fieldId.slice(1);
  },

  /**
   * Génère dynamiquement les champs de formulaire depuis FormFieldDef
   * @param {Object} def - Définition du composant depuis le backend
   * @param {string} containerId - ID du conteneur HTML (ex: "componentFormCard")
   * @param {Object} currentCfg - Configuration actuelle (optionnel)
   */
  generateFormFields(def, containerId, currentCfg = {}) {
    const container = $('#' + containerId);
    if(!container || !def || !def.formFields || !Array.isArray(def.formFields)) {
      console.warn('[FormGenerator.generateFormFields] Container ou formFields manquant', containerId, def);
      return;
    }
    
    // Vider le conteneur
    container.innerHTML = '';
    
    // Parcourir tous les champs de formulaire
    def.formFields.forEach(field => {
      // Gérer l'affichage conditionnel
      let fieldContainer = container;
      if(field.dependsOn && field.showWhen) {
        const dependsOnEl = $('#' + field.dependsOn);
        if(dependsOnEl) {
          const showWhenValues = JSON.parse(field.showWhen || '[]');
          const shouldShow = showWhenValues.includes(dependsOnEl.value);
          // Créer un élément pour gérer l'affichage conditionnel
          let hiddenWrapper = $('#' + field.id + 'Row');
          if(!hiddenWrapper) {
            hiddenWrapper = document.createElement('div');
            hiddenWrapper.id = field.id + 'Row';
            container.appendChild(hiddenWrapper);
            // Ajouter l'écouteur d'événement
            dependsOnEl.addEventListener('change', () => {
              const newValue = dependsOnEl.value;
              const shouldShowNow = showWhenValues.includes(newValue);
              hiddenWrapper.style.display = shouldShowNow ? 'block' : 'none';
            });
          }
          hiddenWrapper.style.display = shouldShow ? 'block' : 'none';
          fieldContainer = hiddenWrapper;
        }
      }
      
      // Créer le wrapper
      const wrapper = document.createElement('div');
      wrapper.className = field.wrapperClass || 'r';
      if(field.type === 5) { // INFO
        // Pour INFO, pas de wrapper, juste la div.hint
        const infoDiv = document.createElement('div');
        infoDiv.className = field.hintClass || 'hint';
        infoDiv.textContent = field.hint || '';
        fieldContainer.appendChild(infoDiv);
        return;
      }
      
      // Label principal
      if(field.label) {
        const label = document.createElement('label');
        label.textContent = field.label;
        if(field.required) label.textContent += ' *';
        wrapper.appendChild(label);
      }
      
      // Label avant (pour champs complexes)
      if(field.labelBefore) {
        const labelBefore = document.createElement('span');
        labelBefore.textContent = field.labelBefore;
        wrapper.appendChild(labelBefore);
      }
      
      // Créer l'input selon le type
      let input;
      
      switch(field.type) {
        case 0: // TEXT
          input = document.createElement('input');
          input.type = 'text';
          input.id = field.id;
          if(field.placeholder) input.placeholder = field.placeholder;
          if(field.maxLength > 0) input.maxLength = field.maxLength;
          if(field.pattern) input.pattern = field.pattern;
          if(field.width > 0) input.style.width = field.width + 'px';
          if(field.defaultValue && !currentCfg[field.id]) input.value = field.defaultValue;
          else if(currentCfg[field.id]) input.value = currentCfg[field.id];
          break;
          
        case 1: // NUMBER
          input = document.createElement('input');
          input.type = 'number';
          input.id = field.id;
          input.min = field.min;
          input.max = field.max;
          if(field.step) input.step = field.step;
          if(field.placeholder) input.placeholder = field.placeholder;
          if(field.width > 0) input.style.width = field.width + 'px';
          if(field.defaultValue && !currentCfg[field.id]) input.value = field.defaultValue;
          else if(currentCfg[field.id] !== undefined) input.value = currentCfg[field.id];
          break;
          
        case 2: // SELECT
          input = document.createElement('select');
          input.id = field.id;
          if(field.width > 0) input.style.width = field.width + 'px';
          if(field.options) {
            try {
              const options = JSON.parse(field.options);
              options.forEach(opt => {
                const option = document.createElement('option');
                option.value = opt.value;
                option.textContent = opt.label;
                input.appendChild(option);
              });
            } catch(e) {
              console.warn('[FormGenerator.generateFormFields] Erreur parsing options:', e);
            }
          }
          if(field.defaultValue && !currentCfg[field.id]) input.value = field.defaultValue;
          else if(currentCfg[field.id]) input.value = currentCfg[field.id];
          break;
          
        case 3: // CHECKBOX
          input = document.createElement('input');
          input.type = 'checkbox';
          input.id = field.id;
          if(field.label) {
            const labelFor = document.createElement('label');
            labelFor.setAttribute('for', field.id);
            labelFor.textContent = field.label;
            wrapper.insertBefore(labelFor, wrapper.firstChild);
          }
          if(field.defaultValue === 'true' || currentCfg[field.id] === true || currentCfg[field.id] === 'true') {
            input.checked = true;
          }
          break;
          
        case 4: // RANGE
          // Pour RANGE, créer deux inputs number
          const rangeWrapper = document.createElement('span');
          const inputMin = document.createElement('input');
          inputMin.type = 'number';
          inputMin.id = field.id + 'Min';
          inputMin.min = field.min;
          inputMin.max = field.max;
          if(field.step) inputMin.step = field.step;
          if(field.width > 0) inputMin.style.width = field.width + 'px';
          // Pour RANGE, defaultValue est utilisé pour le min, max utilise field.max par défaut
          if(currentCfg[field.id + 'Min'] !== undefined) {
            inputMin.value = currentCfg[field.id + 'Min'];
          } else if(field.defaultValue) {
            inputMin.value = field.defaultValue;
          } else {
            inputMin.value = field.min;
          }
          
          const separator = document.createElement('span');
          separator.textContent = field.separator || '→';
          separator.style.margin = '0 4px';
          
          const inputMax = document.createElement('input');
          inputMax.type = 'number';
          inputMax.id = field.id + 'Max';
          inputMax.min = field.min;
          inputMax.max = field.max;
          if(field.step) inputMax.step = field.step;
          if(field.width > 0) inputMax.style.width = field.width + 'px';
          if(currentCfg[field.id + 'Max'] !== undefined) {
            inputMax.value = currentCfg[field.id + 'Max'];
          } else {
            inputMax.value = field.max;
          }
          
          rangeWrapper.appendChild(inputMin);
          rangeWrapper.appendChild(separator);
          rangeWrapper.appendChild(inputMax);
          input = rangeWrapper;
          break;
          
        default:
          console.warn('[FormGenerator.generateFormFields] Type de champ inconnu:', field.type);
          return;
      }
      
      if(input && field.inputClass) {
        if(typeof input.classList !== 'undefined') {
          input.classList.add(...field.inputClass.split(' '));
        } else {
          input.className = (input.className || '') + ' ' + field.inputClass;
        }
      }
      
      wrapper.appendChild(input);
      
      // Label après (pour champs complexes)
      if(field.labelAfter) {
        const labelAfter = document.createElement('span');
        labelAfter.textContent = field.labelAfter;
        wrapper.appendChild(labelAfter);
      }
      
      // Hint inline
      if(field.hintPosition === 1 && field.hint) { // INLINE
        const hintSpan = document.createElement('span');
        hintSpan.textContent = field.hint;
        if(field.hintClass) {
          hintSpan.setAttribute('style', field.hintClass);
        } else {
          hintSpan.style.marginLeft = '8px';
          hintSpan.style.fontSize = '0.9em';
          hintSpan.style.color = '#666';
        }
        wrapper.appendChild(hintSpan);
      }
      
      fieldContainer.appendChild(wrapper);
      
      // Hint en dessous
      if(field.hintPosition === 2 && field.hint) { // BELOW
        const hintDiv = document.createElement('div');
        hintDiv.className = field.hintClass || 'hint';
        hintDiv.textContent = field.hint;
        fieldContainer.appendChild(hintDiv);
      }
    });
  },

  /**
   * Génère dynamiquement les champs pour les pins additionnelles (composants complexes)
   * @param {Object} def - Définition du composant depuis le backend
   * @param {string} containerId - ID du conteneur (ex: "componentFormCard")
   * @param {Object} currentCfg - Configuration actuelle (optionnel)
   */
  generateAdditionalPins(def, containerId, currentCfg = {}) {
    const container = $('#' + containerId);
    if(!container || !def || !def.additionalPins || !Array.isArray(def.additionalPins) || def.additionalPins.length === 0) {
      return;
    }
    
    // Créer une section pour les pins additionnelles
    const section = document.createElement('div');
    section.className = 'f';
    section.style.marginTop = '20px';
    
    const sectionTitle = document.createElement('h4');
    sectionTitle.style.marginTop = '0';
    sectionTitle.textContent = 'Configuration des pins';
    section.appendChild(sectionTitle);
    
    // Obtenir les GPIOs déjà utilisées (pour exclusion)
    if(typeof GpioManager === 'undefined' || !GpioManager.getUsedGpios) {
      console.warn('[FormGenerator.generateAdditionalPins] GpioManager non disponible');
      return;
    }
    const usedGpios = GpioManager.getUsedGpios([]);
    const currentPinLabel = $('#selPin')?.textContent || '';
    const currentPin = caps?.pins?.find(p => p.label === currentPinLabel);
    const currentPinGpio = currentPin ? parseInt(currentPin.gpio) : null;
    
    def.additionalPins.forEach(additionalPin => {
      if(!additionalPin.id) return;
      
      const wrapper = document.createElement('div');
      wrapper.className = 'f';
      
      const label = document.createElement('label');
      label.textContent = additionalPin.displayName || additionalPin.id;
      if(additionalPin.optional) {
        label.textContent += ' (optionnel)';
      }
      wrapper.appendChild(label);
      
      const select = document.createElement('select');
      // ID du champ : préfixe depuis l'ID du composant + id en capital (ex: s0 -> hc4067S0, en -> hc4067En)
      const fieldId = this.getFieldId(def, additionalPin.id);
      select.id = fieldId;
      select.style.width = '200px';
      
      // Ajouter option "Non connecté" pour les pins optionnelles
      if(additionalPin.optional) {
        const optNone = document.createElement('option');
        optNone.value = '255';
        optNone.textContent = 'Non connecté';
        select.appendChild(optNone);
      }
      
      // Remplir avec les pins disponibles selon pinType
      const pinType = additionalPin.pinType !== undefined ? additionalPin.pinType : 1; // Défaut: PIN_DIGITAL
      if(typeof GpioManager === 'undefined' || !GpioManager.getPinsByType) {
        console.warn('[FormGenerator.generateAdditionalPins] GpioManager.getPinsByType non disponible');
        return;
      }
      const availablePins = GpioManager.getPinsByType(pinType, Array.from(usedGpios));
      
      availablePins.forEach(pin => {
        // Ne pas inclure la pin principale (si elle est digitale)
        if(currentPinGpio && parseInt(pin.gpio) === currentPinGpio && pinType === 1) {
          return; // Skip
        }
        
        const option = document.createElement('option');
        option.value = pin.gpio;
        option.textContent = pin.label + ' (GPIO' + pin.gpio + ')';
        select.appendChild(option);
      });
      
      // Restaurer la valeur actuelle si présente
      const currentValue = currentCfg[fieldId] || currentCfg[additionalPin.id];
      if(currentValue !== undefined && currentValue !== null) {
        select.value = String(currentValue);
      } else if(additionalPin.defaultValue !== undefined && additionalPin.defaultValue !== 255) {
        select.value = String(additionalPin.defaultValue);
      }
      
      wrapper.appendChild(select);
      section.appendChild(wrapper);
    });
    
    container.appendChild(section);
  }
};
