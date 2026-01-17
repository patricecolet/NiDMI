/**
 * Gestion des GPIOs et de leur disponibilité
 * Module refactorisé depuis components.js - Phase 1.4
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
    if(!caps || !caps.pins) return null;
    const pin = caps.pins.find(p => p.label === `D${dNum}`);
    return pin ? pin.gpio : null;
  },

  /**
   * Obtient le numéro digital depuis un GPIO (ex: GPIO 5 -> 0 pour "D0")
   * @param {number} gpio - Numéro du GPIO
   * @returns {number|null} Numéro digital ou null si non trouvé
   */
  getDFromGpio(gpio) {
    if(!caps || !caps.pins) return null;
    const pin = caps.pins.find(p => p.gpio === gpio && p.label && p.label.startsWith('D'));
    return pin ? parseInt(pin.label.substring(1)) : null;
  },

  /**
   * Obtient l'objet pin digitale depuis un GPIO
   * @param {number} gpio - Numéro du GPIO
   * @returns {Object|null} Pin ou null si non trouvé
   */
  getDigitalPinByGpio(gpio) {
    if(!caps || !caps.pins) return null;
    return caps.pins.find(p => p.gpio === gpio && p.label && p.label.startsWith('D')) || null;
  },

  /**
   * Vérifie si une pin digitale est disponible
   * @param {number} gpio - Numéro du GPIO
   * @param {Set} usedGpios - Set des GPIOs utilisés
   * @returns {boolean} true si disponible
   */
  isDigitalPinAvailable(gpio, usedGpios) {
    return !!this.getDigitalPinByGpio(gpio) && !usedGpios.has(gpio);
  },

  /**
   * Calcule automatiquement les pins d'adressage S0-S3 en prenant les 4 premières pins digitales disponibles
   * @param {number} sigGpio - GPIO de la pin signal (SIG)
   * @param {Set} usedGpiosOverride - Set des GPIOs utilisés (optionnel, sinon utilise getUsedGpios)
   * @returns {Object} {s0, s1, s2, s3} GPIOs ou null
   */
  calculateAddressPins(sigGpio, usedGpiosOverride = null) {
    // Obtenir les GPIO déjà utilisés (sauf le SIG actuel)
    // Note: éviter la récursion infinie en ne passant pas par getUsedGpios si usedGpiosOverride est fourni
    const usedGpios = usedGpiosOverride || new Set();
    usedGpios.delete(sigGpio);
    
    // Obtenir toutes les pins digitales disponibles
    const availablePins = this.getAvailableDigitalPins(usedGpios);
    
    // Prendre les 4 premières
    const result = {
      s0: availablePins[0] ? parseInt(availablePins[0].gpio) : null,
      s1: availablePins[1] ? parseInt(availablePins[1].gpio) : null,
      s2: availablePins[2] ? parseInt(availablePins[2].gpio) : null,
      s3: availablePins[3] ? parseInt(availablePins[3].gpio) : null
    };
    
    return result;
  },

  /**
   * Obtient toutes les pins digitales uniques (dédupliquées par GPIO)
   * @returns {Array} Liste des pins digitales triées par numéro
   */
  getAllDigitalPins() {
    if(!caps || !caps.pins) return [];
    const allDPinsRaw = caps.pins.filter(p => p.label && p.label.startsWith('D'));
    const uniqueDPinsMap = new Map();
    allDPinsRaw.forEach(p => {
      if(!uniqueDPinsMap.has(p.gpio)) {
        uniqueDPinsMap.set(p.gpio, p);
      }
    });
    return Array.from(uniqueDPinsMap.values()).sort((a, b) => {
      const numA = parseInt(a.label.substring(1));
      const numB = parseInt(b.label.substring(1));
      return numA - numB;
    });
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
    return allDPins.filter(p => {
      return !usedGpios.has(p.gpio) || currentSet.has(p.gpio);
    });
  },

  /**
   * Vérifie la disponibilité du mode auto pour un MUX
   * @param {number} sigGpio - GPIO de la pin signal (SIG)
   * @param {Set} usedGpios - Set des GPIOs utilisés
   * @returns {boolean} true si le mode auto est disponible
   */
  checkAutoAvailability(sigGpio, usedGpios) {
    if(!caps || !caps.pins) return false;
    const sigPin = caps.pins.find(p => p.gpio === sigGpio);
    if(!sigPin) return false;
    const usedGpiosCopy = new Set(usedGpios);
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
   */
  areAddressPinsAvailable(sigGpio, excludeUsedGpios = null) {
    if(!caps || !caps.pins) return false;
    const sigPin = caps.pins.find(p => p.gpio === sigGpio);
    if(!sigPin) return false;
    // Note: éviter la récursion infinie en utilisant new Set() si excludeUsedGpios n'est pas fourni
    const usedGpios = excludeUsedGpios || new Set();
    // Exclure le GPIO SIG lui-même
    usedGpios.delete(sigGpio);
    // Vérifier qu'il y a au moins 4 pins digitales disponibles
    const availablePins = this.getAvailableDigitalPins(usedGpios);
    return availablePins.length >= 4;
  },

  /**
   * Obtient tous les GPIOs utilisés (par les pins configurées + composants complexes)
   * @param {Array} additionalSelectIds - IDs de selects additionnels à lire (optionnel)
   * @returns {Set} Set des GPIOs utilisés
   */
  getUsedGpios(additionalSelectIds = []) {
    const usedGpios = new Set();
    
    // Ajouter les GPIO des pins configurées
    Object.keys(pcfg).forEach(lbl => {
      const cfg = pcfg[lbl];
      const pin = caps.pins.find(p => p.label === lbl);
      if(!pin) return;
      
      // Pour les composants complexes (MUX) temporaires, ajouter aussi les pins d'adresse
      const role = cfg?.role ? (typeof migrateRole === 'function' ? migrateRole(cfg.role) : cfg.role) : '';
      const def = typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.getById 
        ? ComponentDefinitions.getById(role)
        : (typeof getComponentDefinition === 'function' ? getComponentDefinition(role) : null);
      if(def && def.isComplex) {
        const sigGpio = parseInt(pin.gpio);
        usedGpios.add(sigGpio);
        // Calculer et ajouter les pins d'adresse (mode auto)
        // Passer usedGpios pour éviter une boucle infinie
        const addrPins = this.calculateAddressPins(sigGpio, usedGpios);
        if(addrPins.s0 !== null) usedGpios.add(addrPins.s0);
        if(addrPins.s1 !== null) usedGpios.add(addrPins.s1);
        if(addrPins.s2 !== null) usedGpios.add(addrPins.s2);
        if(addrPins.s3 !== null) usedGpios.add(addrPins.s3);
      } else {
        usedGpios.add(pin.gpio);
      }
    });
    
    // Ne pas exclure le composant complexe en cours d'édition sauf si on est vraiment en train de l'éditer
    const funcSelectValue = $('#funcSelect')?.value || '';
    // Vérifier si le composant sélectionné est complexe
    const migratedRole = typeof migrateRole === 'function' ? migrateRole(funcSelectValue) : funcSelectValue;
    const funcDef = typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.getById
      ? ComponentDefinitions.getById(migratedRole)
      : null;
    const isEditingComplex = funcDef && funcDef.isComplex;
    // Obtenir l'ID du composant complexe en cours d'édition dynamiquement
    let currentComplexId = null;
    if(isEditingComplex && typeof FormGenerator !== 'undefined' && FormGenerator.getFieldId) {
      currentComplexId = FormGenerator.getFieldId(funcDef, 'id');
    } else if(isEditingComplex) {
      const prefix = funcDef.id ? funcDef.id : 'comp';
      currentComplexId = prefix + 'Id';
    }
    
    // Pour les composants complexes, exclure les pins additionnelles en cours d'édition
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
    if(!caps || !caps.pins) return [];
    
    const excludeSet = new Set(excludeGpios);
    
    return caps.pins.filter(pin => {
      // Exclure les pins déjà utilisées
      if(excludeSet.has(parseInt(pin.gpio))) return false;
      
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
