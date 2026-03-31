/**
 * Gestion des GPIOs et de leur disponibilité
 * Module refactorisé depuis components.js
 * 
 * Responsabilités:
 * - Conversion entre labels de pins (D0, A0) et GPIOs
 * - Vérification de la disponibilité des GPIOs
 * - Calcul automatique des pins d'adressage pour MUX
 * - Filtrage des pins par type et disponibilité
 */

const GpioManager = {
  /**
   * Obtient le GPIO depuis un label digital (ex: "D0" -> GPIO numéro)
   * @param {number} dNum - Numéro de la pin digitale (ex: 0 pour "D0")
   * @returns {number|null} GPIO ou null si non trouvé
   */
  getGpioFromD(dNum) {
    if(typeof caps === 'undefined' || !caps || !caps.pins || !Array.isArray(caps.pins)) return null;
    const pin = caps.pins.find(p => p && p.label === `D${dNum}`);
    return pin && pin.gpio !== undefined ? parseInt(pin.gpio) : null;
  },

  /**
   * Obtient le numéro digital depuis un GPIO (ex: GPIO 5 -> 0 pour "D0")
   * @param {number} gpio - Numéro du GPIO
   * @returns {number|null} Numéro digital ou null si non trouvé
   */
  getDFromGpio(gpio) {
    if(typeof caps === 'undefined' || !caps || !caps.pins || !Array.isArray(caps.pins)) return null;
    if(typeof gpio !== 'number') gpio = parseInt(gpio);
    if(isNaN(gpio)) return null;
    const pin = caps.pins.find(p => p && parseInt(p.gpio) === gpio && p.label && p.label.startsWith('D'));
    return pin ? parseInt(pin.label.substring(1)) : null;
  },

  /**
   * Obtient l'objet pin digitale depuis un GPIO
   * @param {number} gpio - Numéro du GPIO
   * @returns {Object|null} Pin ou null si non trouvé
   */
  getDigitalPinByGpio(gpio) {
    if(typeof caps === 'undefined' || !caps || !caps.pins || !Array.isArray(caps.pins)) return null;
    if(typeof gpio !== 'number') gpio = parseInt(gpio);
    if(isNaN(gpio)) return null;
    return caps.pins.find(p => p && parseInt(p.gpio) === gpio && p.label && p.label.startsWith('D')) || null;
  },

  /**
   * Vérifie si une pin digitale est disponible
   * @param {number} gpio - Numéro du GPIO
   * @param {Set} usedGpios - Set des GPIOs utilisés
   * @returns {boolean} true si disponible
   */
  isDigitalPinAvailable(gpio, usedGpios) {
    if(!usedGpios || typeof usedGpios.has !== 'function') return false;
    if(typeof gpio !== 'number') gpio = parseInt(gpio);
    if(isNaN(gpio)) return false;
    return !!this.getDigitalPinByGpio(gpio) && !usedGpios.has(gpio);
  },

  /**
   * Note: calculateAddressPins supprimé - le calcul automatique des pins additionnelles doit être géré par le backend
   * Toute la logique spécifique aux composants (comme les calculs de pins d'adresse pour les MUX) doit être dans le backend
   */

  /**
   * Obtient toutes les pins digitales uniques (dédupliquées par GPIO)
   * @returns {Array} Liste des pins digitales triées par numéro
   */
  getAllDigitalPins() {
    if(typeof caps === 'undefined' || !caps || !caps.pins || !Array.isArray(caps.pins)) return [];
    const allDPinsRaw = caps.pins.filter(p => p && p.label && typeof p.label === 'string' && p.label.startsWith('D'));
    console.log('[GpioManager.getAllDigitalPins] allDPinsRaw count:', allDPinsRaw.length, 'exemples:', allDPinsRaw.slice(0, 5).map(p => ({label: p.label, gpio: p.gpio})));
    const uniqueDPinsMap = new Map();
    allDPinsRaw.forEach(p => {
      if(!p || p.gpio === undefined) return;
      const gpioKey = parseInt(p.gpio);
      if(isNaN(gpioKey)) return;
      if(!uniqueDPinsMap.has(gpioKey)) {
        uniqueDPinsMap.set(gpioKey, p);
      } else {
        const existing = uniqueDPinsMap.get(gpioKey);
        console.log('[GpioManager.getAllDigitalPins] GPIO dupliqué ignoré:', gpioKey, 'label:', p.label, 'déjà présent:', existing ? existing.label : 'N/A');
      }
    });
    const result = Array.from(uniqueDPinsMap.values()).sort((a, b) => {
      if(!a || !b || !a.label || !b.label) return 0;
      const numA = parseInt(a.label.substring(1));
      const numB = parseInt(b.label.substring(1));
      if(isNaN(numA) || isNaN(numB)) return 0;
      return numA - numB;
    });
    console.log('[GpioManager.getAllDigitalPins] Résultat unique count:', result.length, 'exemples:', result.slice(0, 5).map(p => ({label: p.label, gpio: p.gpio})));
    return result;
  },

  /**
   * Obtient les pins digitales disponibles (filtrées par usedGpios, avec exception pour currentValues)
   * @param {Set} usedGpios - Set des GPIOs utilisés
   * @param {Set|Array} currentValues - GPIOs à ne pas exclure (optionnel)
   * @returns {Array} Liste des pins digitales disponibles
   */
  getAvailableDigitalPins(usedGpios, currentValues = null) {
    const allDPins = this.getAllDigitalPins();
    const currentSet = currentValues instanceof Set ? currentValues : new Set();
    /* Convertir usedGpios en Set si ce n'est pas déjà un Set (gérer le cas où c'est un Array) */
    let usedGpiosSet;
    if(usedGpios instanceof Set) {
      usedGpiosSet = usedGpios;
    } else if(Array.isArray(usedGpios)) {
      usedGpiosSet = new Set(usedGpios);
    } else {
      usedGpiosSet = new Set();
    }
    console.log('[GpioManager.getAvailableDigitalPins] usedGpios type:', usedGpios instanceof Set ? 'Set' : (Array.isArray(usedGpios) ? 'Array' : typeof usedGpios), 'usedGpios:', Array.from(usedGpiosSet), 'allDPins count:', allDPins.length);
    const filtered = allDPins.filter(p => {
      const isUsed = usedGpiosSet.has(p.gpio);
      const isCurrent = currentSet.has(p.gpio);
      const isAvailable = !isUsed || isCurrent;
      return isAvailable;
    });
    console.log('[GpioManager.getAvailableDigitalPins] Pins disponibles après filtrage:', filtered.length, 'exemples:', filtered.slice(0, 5).map(p => ({label: p.label, gpio: p.gpio})));
    return filtered;
  },

  /**
   * Note: Fonctions spécifiques aux MUX supprimées - le backend doit gérer toute la logique spécifique aux composants
   * - checkAutoAvailability() supprimée
   * - checkEnAvailability() supprimée (calcul hardcodé mainPinGpio + 5)
   * - getAvailabilityInfo() supprimée
   * - areAddressPinsAvailable() supprimée (hardcodé 4 pins)
   * Utiliser areAdditionalPinsAvailable() pour une vérification générique basée sur la définition du composant
   */

  /**
   * Vérifie si les pins additionnelles requises sont disponibles selon la définition du composant
   * @param {Object} def - Définition du composant (doit avoir additionalPins)
   * @param {number} mainPinGpio - GPIO de la pin principale sur laquelle le composant est configuré
   * @param {Set} excludeUsedGpios - Set des GPIOs à exclure (optionnel, utilise getUsedGpios() si non fourni)
   * @returns {boolean} true si toutes les pins requises sont disponibles
   */
  areAdditionalPinsAvailable(def, mainPinGpio, excludeUsedGpios = null) {
    if(typeof caps === 'undefined' || !caps || !caps.pins || !Array.isArray(caps.pins)) return false;
    if(typeof mainPinGpio !== 'number') mainPinGpio = parseInt(mainPinGpio);
    if(isNaN(mainPinGpio)) return false;
    
    /* Si pas de définition ou pas d'additionalPins, considérer comme disponible */
    if(!def || !def.additionalPins || !Array.isArray(def.additionalPins) || def.additionalPins.length === 0) {
      return true;
    }
    
    const mainPin = caps.pins.find(p => p && parseInt(p.gpio) === mainPinGpio);
    if(!mainPin) return false;
    
    /* Obtenir les GPIOs utilisés (depuis pcfg si excludeUsedGpios n'est pas fourni) */
    let usedGpios;
    if(excludeUsedGpios instanceof Set) {
      usedGpios = new Set(excludeUsedGpios);
    } else if(Array.isArray(excludeUsedGpios)) {
      usedGpios = new Set(excludeUsedGpios);
    } else {
    /* Utiliser getUsedGpios() pour obtenir les pins déjà configurées */
      usedGpios = this.getUsedGpios([]);
    }
    
    /* Exclure le GPIO de la pin principale lui-même */
    usedGpios.delete(mainPinGpio);
    
    /* Compter uniquement les pins requises (non optionnelles, digitales) */
    const requiredAddressPins = def.additionalPins.filter(ap => {
      if(!ap || !ap.id) return false;
      /* Compter uniquement les pins digitales requises (non optionnelles) */
      return ap.pinType === 1 && !ap.optional;
    });
    
    const requiredCount = requiredAddressPins.length;
    
    /* Si aucune pin d'adresse requise, considérer comme disponible */
    if(requiredCount === 0) return true;
    
    /* Vérifier qu'il y a assez de pins digitales disponibles */
    const availablePins = this.getAvailableDigitalPins(usedGpios);
    const availableCount = Array.isArray(availablePins) ? availablePins.length : 0;
    
    console.log('[GpioManager.areAdditionalPinsAvailable] def.id:', def.id, 'mainPinGpio:', mainPinGpio, 'requiredCount:', requiredCount, 'availableCount:', availableCount);
    
    return availableCount >= requiredCount;
  },

  /**
   * Obtient tous les GPIOs utilisés (par les pins configurées + composants complexes)
   * @param {Array} additionalSelectIds - IDs de selects additionnels à lire (optionnel)
   * @returns {Set} Set des GPIOs utilisés
   */
  getUsedGpios(additionalSelectIds = []) {
    const usedGpios = new Set();
    
    /* Ajouter les GPIO des pins configurées */
    if(typeof pcfg === 'undefined' || !pcfg) return usedGpios;
    if(typeof caps === 'undefined' || !caps || !caps.pins || !Array.isArray(caps.pins)) return usedGpios;
    
    Object.keys(pcfg).forEach(lbl => {
      const cfg = pcfg[lbl];
      if(!cfg) return;
      const pin = caps.pins.find(p => p && p.label === lbl);
      if(!pin) return;
      
    /* Pour les composants avec additionalPins temporaires, ajouter aussi les pins d'adresse */
      const role = cfg.role ? (typeof migrateRole === 'function' ? migrateRole(cfg.role) : cfg.role) : '';
      const def = typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.getById 
        ? ComponentDefinitions.getById(role)
        : (typeof getComponentDefinition === 'function' ? getComponentDefinition(role) : null);
      const hasAdditionalPins = def && (def.additionalPinCount > 0 || (def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0));
      if(hasAdditionalPins && cfg.additionalPins && typeof cfg.additionalPins === 'object') {
        const mainPinGpio = parseInt(pin.gpio);
        if(!isNaN(mainPinGpio)) {
          usedGpios.add(mainPinGpio);
          /* Extraire les IDs des pins d'adresse depuis les définitions (générique) */
          /* Filtrer uniquement les pins digitales requises (non optionnelles) */
          if(def.additionalPins && Array.isArray(def.additionalPins)) {
            const addressPinIds = def.additionalPins
              .filter(ap => ap && ap.id && ap.pinType === 1 && !ap.optional)
              .map(ap => ap.id);
          /* Note: Calcul automatique des pins additionnelles supprimé - doit être géré par le backend */
          /* Lire les valeurs depuis pcfg si disponibles */
          if(cfg.additionalPins && typeof cfg.additionalPins === 'object') {
            addressPinIds.forEach(pinId => {
              const gpioVal = cfg.additionalPins[pinId];
              if(gpioVal !== undefined && gpioVal !== null && gpioVal !== 255) {
                const gpio = parseInt(gpioVal);
                if(!isNaN(gpio)) usedGpios.add(gpio);
              }
            });
          }
          }
        }
      } else {
        const gpioVal = parseInt(pin.gpio);
        if(!isNaN(gpioVal)) usedGpios.add(gpioVal);
      }
    });
    
    /* Ne pas exclure le composant complexe en cours d'édition sauf si on est vraiment en train de l'éditer */
    const funcSelect = typeof $ === 'function' ? $('#funcSelect') : null;
    const funcSelectValue = funcSelect && funcSelect.value ? funcSelect.value : '';
    /* Vérifier si le composant sélectionné est complexe */
    const migratedRole = typeof migrateRole === 'function' ? migrateRole(funcSelectValue) : funcSelectValue;
    const funcDef = typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.getById
      ? ComponentDefinitions.getById(migratedRole)
      : (typeof getComponentDefinition === 'function' ? getComponentDefinition(migratedRole) : null);
    const isEditingComplex = funcDef && (funcDef.additionalPinCount > 0 || (funcDef.additionalPins && Array.isArray(funcDef.additionalPins) && funcDef.additionalPins.length > 0));
    /* Obtenir l'ID du composant avec additionalPins en cours d'édition dynamiquement */
    let currentComplexId = null;
    if(isEditingComplex && typeof FormGenerator !== 'undefined' && FormGenerator.getFieldId) {
      currentComplexId = FormGenerator.getFieldId(funcDef, 'id');
    } else if(isEditingComplex) {
      const prefix = funcDef.id ? funcDef.id : 'comp';
      currentComplexId = prefix + 'Id';
    }
    
    /* Pour les composants avec additionalPins, exclure les pins additionnelles en cours d'édition */
    if(isEditingComplex && funcDef && funcDef.additionalPins) {
      funcDef.additionalPins.forEach(additionalPin => {
        if(!additionalPin.id) return;
        const fieldId = typeof FormGenerator !== 'undefined' && FormGenerator.getFieldId
          ? FormGenerator.getFieldId(funcDef, additionalPin.id)
          : (funcDef.id ? funcDef.id : 'comp') + additionalPin.id.charAt(0).toUpperCase() + additionalPin.id.slice(1);
        const select = $('#' + fieldId);
        if(select && select.value && select.value !== '255') {
          const gpio = parseInt(select.value);
          if(!isNaN(gpio)) {
            usedGpios.add(gpio);
          }
        }
      });
    }
    
    /* Ajouter les GPIOs depuis les selects additionnels (pour composants complexes) */
    additionalSelectIds.forEach(selectId => {
      const select = $('#' + selectId);
      if(select && select.value && select.value !== '255') {
        const gpio = parseInt(select.value);
        if(!isNaN(gpio)) {
          usedGpios.add(gpio);
        }
      }
    });
    
    return usedGpios;
  },

  /**
   * Obtient les pins disponibles filtrées par type
   * @param {number} pinType - Type de pin (0=PIN_ANALOG, 1=PIN_DIGITAL, 2=PIN_ANALOG_OR_DIGITAL, 3=PIN_PWM)
   * @param {Array} excludeGpios - Liste des GPIOs à exclure (optionnel)
   * @returns {Array} Liste des pins disponibles
   */
  getPinsByType(pinType, excludeGpios = []) {
    if(typeof caps === 'undefined' || !caps || !caps.pins || !Array.isArray(caps.pins)) return [];
    
    const excludeSet = Array.isArray(excludeGpios) ? new Set(excludeGpios.map(g => parseInt(g)).filter(g => !isNaN(g))) : new Set();
    
    let result = caps.pins.filter(pin => {
      if(!pin || pin.gpio === undefined) return false;
      const gpio = parseInt(pin.gpio);
      if(isNaN(gpio)) return false;
      
      /* Exclure les pins déjà utilisées */
      if(excludeSet.has(gpio)) return false;
      
      /* Filtrer selon pinType */
      switch(pinType) {
        case 0: /* PIN_ANALOG */
          return pin.caps && pin.caps.adc === true;
        case 1: /* PIN_DIGITAL */
          return true; /* Toutes les pins peuvent être digitales */
        case 2: /* PIN_ANALOG_OR_DIGITAL */
          return true; /* Toutes les pins */
        case 3: /* PIN_PWM */
          return pin.caps && pin.caps.pwm === true;
        default:
          return false;
      }
    });
    
    /* Pour PIN_ANALOG : privilégier les labels A0, A1, A2... (alias analogiques) au lieu de D0, D1, D2 */
    if(pinType === 0 && result.length > 0) {
      const analogLabels = result.filter(p => p.label && String(p.label).match(/^A\d+/));
      if(analogLabels.length > 0) {
        result = analogLabels.sort((a, b) => {
          const na = parseInt(String(a.label).replace(/^A/, ''), 10) || 0;
          const nb = parseInt(String(b.label).replace(/^A/, ''), 10) || 0;
          return na - nb;
        });
      }
    }
    
    return result;
  }
};
