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
   * Calcule automatiquement les pins d'adressage S0-S3 en prenant les 4 premières pins digitales disponibles
   * @param {number} sigGpio - GPIO de la pin signal (SIG)
   * @param {Set} usedGpiosOverride - Set des GPIOs utilisés (optionnel, sinon utilise getUsedGpios)
   * @returns {Object} {s0, s1, s2, s3} GPIOs ou null
   */
  calculateAddressPins(sigGpio, usedGpiosOverride = null, addressPinIds = ['s0', 's1', 's2', 's3']) {
    // Obtenir les GPIO déjà utilisés (sauf le SIG actuel)
    // Note: éviter la récursion infinie en ne passant pas par getUsedGpios si usedGpiosOverride est fourni
    // Convertir en Set si ce n'est pas déjà un Set (gérer le cas où usedGpiosOverride est un Array)
    let usedGpios;
    if(usedGpiosOverride instanceof Set) {
      usedGpios = new Set(usedGpiosOverride);
    } else if(Array.isArray(usedGpiosOverride)) {
      usedGpios = new Set(usedGpiosOverride);
    } else if(usedGpiosOverride) {
      usedGpios = new Set([usedGpiosOverride]);
    } else {
      usedGpios = new Set();
    }
    usedGpios.delete(sigGpio);
    
    // Obtenir toutes les pins digitales disponibles
    // Note: getAvailableDigitalPins exclut déjà les GPIOs dans usedGpios, mais on doit aussi
    // filtrer explicitement la pin SIG pour être cohérent avec generateAdditionalPins()
    // qui exclut la pin principale si elle est digitale (voir form-generator.js ligne 432)
    let availablePins = this.getAvailableDigitalPins(usedGpios);
    // Exclure explicitement la pin SIG (comme dans generateAdditionalPins ligne 432)
    availablePins = availablePins.filter(p => parseInt(p.gpio) !== sigGpio);
    console.log('[GpioManager.calculateAddressPins] Après exclusion SIG pin:', sigGpio, 'availablePins count:', availablePins.length);
    console.log('[GpioManager.calculateAddressPins] sigGpio:', sigGpio, 'availablePins count:', availablePins.length, 'addressPinIds:', addressPinIds);
    console.log('[GpioManager.calculateAddressPins] availablePins:', availablePins.map(p => ({label: p.label, gpio: p.gpio})));
    
    // Construire le résultat dynamiquement depuis addressPinIds
    const result = {};
    addressPinIds.forEach((pinId, index) => {
      if(availablePins[index]) {
        result[pinId] = parseInt(availablePins[index].gpio);
        console.log('[GpioManager.calculateAddressPins]', pinId, '=', result[pinId], 'depuis', availablePins[index].label);
      } else {
        result[pinId] = null;
        console.warn('[GpioManager.calculateAddressPins] Pas assez de pins disponibles pour', pinId);
      }
    });
    
    return result;
  },

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
    // Convertir usedGpios en Set si ce n'est pas déjà un Set (gérer le cas où c'est un Array)
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
   * Vérifie la disponibilité du mode auto pour un MUX
   * @param {number} sigGpio - GPIO de la pin signal (SIG)
   * @param {Set} usedGpios - Set des GPIOs utilisés
   * @returns {boolean} true si le mode auto est disponible
   */
  checkAutoAvailability(sigGpio, usedGpios) {
    if(typeof caps === 'undefined' || !caps || !caps.pins || !Array.isArray(caps.pins)) return false;
    if(typeof sigGpio !== 'number') sigGpio = parseInt(sigGpio);
    if(isNaN(sigGpio)) return false;
    const sigPin = caps.pins.find(p => p && parseInt(p.gpio) === sigGpio);
    if(!sigPin) return false;
    const usedGpiosCopy = usedGpios instanceof Set ? new Set(usedGpios) : new Set();
    usedGpiosCopy.delete(sigGpio);
    return this.areAddressPinsAvailable(sigGpio, usedGpiosCopy);
  },

  /**
   * Vérifie la disponibilité de la pin EN pour un MUX
   * @param {number} sigGpio - GPIO de la pin signal (SIG)
   * @param {Set} usedGpios - Set des GPIOs utilisés
   * @returns {boolean} true si la pin EN est disponible
   */
  checkEnAvailability(sigGpio, usedGpios) {
    const enGpio = sigGpio + 5;
    const enPin = this.getDigitalPinByGpio(enGpio);
    if(!enPin) return false;
    const usedGpiosCopy = new Set(usedGpios);
    usedGpiosCopy.delete(sigGpio);
    const addrPins = this.calculateAddressPins(sigGpio);
    usedGpiosCopy.delete(addrPins.s0);
    usedGpiosCopy.delete(addrPins.s1);
    usedGpiosCopy.delete(addrPins.s2);
    usedGpiosCopy.delete(addrPins.s3);
    return !usedGpiosCopy.has(enGpio);
  },

  /**
   * Obtient toutes les informations de disponibilité pour un MUX (auto + EN)
   * @param {number} sigGpio - GPIO de la pin signal (SIG)
   * @param {Set} usedGpios - Set des GPIOs utilisés
   * @returns {Object} {autoAvailable, enAvailable, enGpio, enPin}
   */
  getAvailabilityInfo(sigGpio, usedGpios) {
    const autoAvailable = this.checkAutoAvailability(sigGpio, usedGpios);
    const enGpio = sigGpio + 5;
    const enPin = this.getDigitalPinByGpio(enGpio);
    const enAvailable = enPin && this.checkEnAvailability(sigGpio, usedGpios);
    return {autoAvailable, enAvailable, enGpio, enPin};
  },

  /**
   * Vérifie si les pins d'adressage sont disponibles pour un GPIO SIG donné
   * @param {number} sigGpio - GPIO de la pin signal (SIG)
   * @param {Set} excludeUsedGpios - Set des GPIOs à exclure (optionnel)
   * @returns {boolean} true si au moins 4 pins digitales sont disponibles
   * @deprecated Utiliser areAdditionalPinsAvailable() à la place pour une vérification basée sur la définition
   */
  areAddressPinsAvailable(sigGpio, excludeUsedGpios = null) {
    if(typeof caps === 'undefined' || !caps || !caps.pins || !Array.isArray(caps.pins)) return false;
    if(typeof sigGpio !== 'number') sigGpio = parseInt(sigGpio);
    if(isNaN(sigGpio)) return false;
    const sigPin = caps.pins.find(p => p && parseInt(p.gpio) === sigGpio);
    if(!sigPin) return false;
    // Note: éviter la récursion infinie en utilisant new Set() si excludeUsedGpios n'est pas fourni
    const usedGpios = excludeUsedGpios instanceof Set ? new Set(excludeUsedGpios) : (excludeUsedGpios ? new Set(excludeUsedGpios) : new Set());
    // Exclure le GPIO SIG lui-même
    usedGpios.delete(sigGpio);
    // Vérifier qu'il y a au moins 4 pins digitales disponibles (fallback pour compatibilité)
    const availablePins = this.getAvailableDigitalPins(usedGpios);
    return Array.isArray(availablePins) && availablePins.length >= 4;
  },

  /**
   * Vérifie si les pins additionnelles requises sont disponibles selon la définition du composant
   * @param {Object} def - Définition du composant (doit avoir additionalPins)
   * @param {number} sigGpio - GPIO de la pin signal (SIG)
   * @param {Set} excludeUsedGpios - Set des GPIOs à exclure (optionnel, utilise getUsedGpios() si non fourni)
   * @returns {boolean} true si toutes les pins requises sont disponibles
   */
  areAdditionalPinsAvailable(def, sigGpio, excludeUsedGpios = null) {
    if(typeof caps === 'undefined' || !caps || !caps.pins || !Array.isArray(caps.pins)) return false;
    if(typeof sigGpio !== 'number') sigGpio = parseInt(sigGpio);
    if(isNaN(sigGpio)) return false;
    
    // Si pas de définition ou pas d'additionalPins, considérer comme disponible
    if(!def || !def.additionalPins || !Array.isArray(def.additionalPins) || def.additionalPins.length === 0) {
      return true;
    }
    
    const sigPin = caps.pins.find(p => p && parseInt(p.gpio) === sigGpio);
    if(!sigPin) return false;
    
    // Obtenir les GPIOs utilisés (depuis pcfg si excludeUsedGpios n'est pas fourni)
    let usedGpios;
    if(excludeUsedGpios instanceof Set) {
      usedGpios = new Set(excludeUsedGpios);
    } else if(Array.isArray(excludeUsedGpios)) {
      usedGpios = new Set(excludeUsedGpios);
    } else {
      // Utiliser getUsedGpios() pour obtenir les pins déjà configurées
      usedGpios = this.getUsedGpios([]);
    }
    
    // Exclure le GPIO SIG lui-même
    usedGpios.delete(sigGpio);
    
    // Compter uniquement les pins requises (non optionnelles, digitales, pas sig/en)
    const requiredAddressPins = def.additionalPins.filter(ap => {
      if(!ap || !ap.id) return false;
      // Ignorer sig et en (gérées séparément)
      if(ap.id === 'sig' || ap.id === 'en') return false;
      // Compter uniquement les pins digitales requises
      return ap.pinType === 1 && !ap.optional;
    });
    
    const requiredCount = requiredAddressPins.length;
    
    // Si aucune pin d'adresse requise, considérer comme disponible
    if(requiredCount === 0) return true;
    
    // Vérifier qu'il y a assez de pins digitales disponibles
    const availablePins = this.getAvailableDigitalPins(usedGpios);
    const availableCount = Array.isArray(availablePins) ? availablePins.length : 0;
    
    console.log('[GpioManager.areAdditionalPinsAvailable] def.id:', def.id, 'sigGpio:', sigGpio, 'requiredCount:', requiredCount, 'availableCount:', availableCount);
    
    return availableCount >= requiredCount;
  },

  /**
   * Obtient tous les GPIOs utilisés (par les pins configurées + composants complexes)
   * @param {Array} additionalSelectIds - IDs de selects additionnels à lire (optionnel)
   * @returns {Set} Set des GPIOs utilisés
   */
  getUsedGpios(additionalSelectIds = []) {
    const usedGpios = new Set();
    
    // Ajouter les GPIO des pins configurées
    if(typeof pcfg === 'undefined' || !pcfg) return usedGpios;
    if(typeof caps === 'undefined' || !caps || !caps.pins || !Array.isArray(caps.pins)) return usedGpios;
    
    Object.keys(pcfg).forEach(lbl => {
      const cfg = pcfg[lbl];
      if(!cfg) return;
      const pin = caps.pins.find(p => p && p.label === lbl);
      if(!pin) return;
      
      // Pour les composants avec additionalPins temporaires, ajouter aussi les pins d'adresse
      const role = cfg.role ? (typeof migrateRole === 'function' ? migrateRole(cfg.role) : cfg.role) : '';
      const def = typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.getById 
        ? ComponentDefinitions.getById(role)
        : (typeof getComponentDefinition === 'function' ? getComponentDefinition(role) : null);
      const hasAdditionalPins = def && (def.additionalPinCount > 0 || (def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0));
      if(hasAdditionalPins && cfg.additionalPins && typeof cfg.additionalPins === 'object') {
        const sigGpio = parseInt(pin.gpio);
        if(!isNaN(sigGpio)) {
          usedGpios.add(sigGpio);
          /* Extraire les IDs des pins d'adresse depuis les définitions (générique) */
          if(def.additionalPins && Array.isArray(def.additionalPins)) {
            const addressPinIds = def.additionalPins
              .filter(ap => ap && ap.id && ap.pinType === 1 && !ap.optional && ap.id !== 'sig' && ap.id !== 'en')
              .map(ap => ap.id);
            /* Calculer et ajouter les pins d'adresse (mode auto) */
            /* Passer usedGpios pour éviter une boucle infinie */
            const addrPins = this.calculateAddressPins(sigGpio, usedGpios, addressPinIds);
            /* Ajouter toutes les pins d'adresse calculées (générique) */
            if(addrPins && typeof addrPins === 'object') {
              addressPinIds.forEach(pinId => {
                if(addrPins[pinId] !== null && addrPins[pinId] !== undefined) {
                  const gpioVal = parseInt(addrPins[pinId]);
                  if(!isNaN(gpioVal)) usedGpios.add(gpioVal);
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
    
    // Ne pas exclure le composant complexe en cours d'édition sauf si on est vraiment en train de l'éditer
    const funcSelect = typeof $ === 'function' ? $('#funcSelect') : null;
    const funcSelectValue = funcSelect && funcSelect.value ? funcSelect.value : '';
    // Vérifier si le composant sélectionné est complexe
    const migratedRole = typeof migrateRole === 'function' ? migrateRole(funcSelectValue) : funcSelectValue;
    const funcDef = typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.getById
      ? ComponentDefinitions.getById(migratedRole)
      : (typeof getComponentDefinition === 'function' ? getComponentDefinition(migratedRole) : null);
    const isEditingComplex = funcDef && (funcDef.additionalPinCount > 0 || (funcDef.additionalPins && Array.isArray(funcDef.additionalPins) && funcDef.additionalPins.length > 0));
    // Obtenir l'ID du composant avec additionalPins en cours d'édition dynamiquement
    let currentComplexId = null;
    if(isEditingComplex && typeof FormGenerator !== 'undefined' && FormGenerator.getFieldId) {
      currentComplexId = FormGenerator.getFieldId(funcDef, 'id');
    } else if(isEditingComplex) {
      const prefix = funcDef.id ? funcDef.id : 'comp';
      currentComplexId = prefix + 'Id';
    }
    
    // Pour les composants avec additionalPins, exclure les pins additionnelles en cours d'édition
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
    
    // Ajouter les GPIOs depuis les selects additionnels (pour composants complexes)
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
    
    return caps.pins.filter(pin => {
      if(!pin || pin.gpio === undefined) return false;
      const gpio = parseInt(pin.gpio);
      if(isNaN(gpio)) return false;
      
      // Exclure les pins déjà utilisées
      if(excludeSet.has(gpio)) return false;
      
      // Filtrer selon pinType
      switch(pinType) {
        case 0: // PIN_ANALOG
          return pin.caps && pin.caps.adc === true;
        case 1: // PIN_DIGITAL
          return true; // Toutes les pins peuvent être digitales
        case 2: // PIN_ANALOG_OR_DIGITAL
          return true; // Toutes les pins
        case 3: // PIN_PWM
          return pin.caps && pin.caps.pwm === true;
        default:
          return false;
      }
    });
  }
};
