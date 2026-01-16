/**
 * Gestion des définitions de composants (cache et accès)
 * Module refactorisé depuis api.js - Phase 1.1
 */

const ComponentDefinitions = {
  /**
   * Cache interne des définitions de composants
   * @private
   */
  _cache: [],

  /**
   * Charge les définitions de composants depuis l'API backend
   * @returns {Promise<Array>} Tableau des définitions de composants
   */
  async load() {
    try {
      const r = await fetch('/api/components/definitions');
      if(!r.ok) {
        console.warn('[ComponentDefinitions.load] Erreur chargement définitions:', r.status);
        return [];
      }
      this._cache = await r.json();
      console.log('[ComponentDefinitions.load] Composants chargés:', this._cache.length);
      return this._cache;
    } catch(err) {
      console.error('[ComponentDefinitions.load] Erreur:', err);
      return [];
    }
  },

  /**
   * Retourne le cache des définitions
   * @returns {Array} Tableau des définitions (peut être vide si non chargé)
   */
  get cache() {
    return this._cache;
  },

  /**
   * Trouve une définition de composant par son ID
   * @param {string} componentId - ID du composant (ex: "potentiometer", "hc4067")
   * @returns {Object|null} Définition du composant ou null si non trouvé
   */
  getById(componentId) {
    if(!this._cache || this._cache.length === 0) {
      console.warn('[ComponentDefinitions.getById] Cache vide pour:', componentId);
      return null;
    }
    return this._cache.find(def => def.id === componentId) || null;
  },

  /**
   * Filtre les composants par famille
   * @param {number} familyId - ID de la famille (0=BASIC, 1=MULTIPLEXER, etc.)
   * @param {boolean} implementedOnly - Si true, retourne uniquement les composants implémentés
   * @returns {Array} Liste des composants de la famille
   */
  getByFamily(familyId, implementedOnly = true) {
    if(!this._cache || this._cache.length === 0) {
      console.warn('[ComponentDefinitions.getByFamily] Cache vide pour famille:', familyId);
      return [];
    }
    
    return this._cache.filter(def => {
      const matchesFamily = def.family === familyId;
      const matchesImplemented = !implementedOnly || def.implemented === true;
      return matchesFamily && matchesImplemented;
    });
  },

  /**
   * Filtre les composants selon le type de pin compatible
   * @param {number} pinType - Type de pin (0=ANALOG, 1=DIGITAL, 2=ANALOG_OR_DIGITAL, 3=PWM)
   * @param {boolean} implementedOnly - Si true, retourne uniquement les composants implémentés
   * @returns {Array} Liste des composants compatibles avec le type de pin
   */
  getForPinType(pinType, implementedOnly = true) {
    if(!this._cache || this._cache.length === 0) {
      console.warn('[ComponentDefinitions.getForPinType] Cache vide pour pinType:', pinType);
      return [];
    }
    
    const filtered = this._cache.filter(def => {
      // Filtrer par implémenté si demandé
      if(implementedOnly && !def.implemented) return false;
      
      // Vérifier la compatibilité du type de pin
      switch(pinType) {
        case 0: // PIN_ANALOG
          return def.pinType === 0 || def.pinType === 2; // ANALOG ou ANALOG_OR_DIGITAL
        case 1: // PIN_DIGITAL
          return def.pinType === 1 || def.pinType === 2; // DIGITAL ou ANALOG_OR_DIGITAL
        case 3: // PIN_PWM
          return def.pinType === 3; // PWM uniquement
        default:
          return false;
      }
    });
    
    console.log(`[ComponentDefinitions.getForPinType] pinType=${pinType}, implementedOnly=${implementedOnly}, trouvé ${filtered.length} composants`);
    return filtered;
  }
};
