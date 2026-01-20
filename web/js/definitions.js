/**
 * Gestion des définitions de composants (cache et accès)
 * Module refactorisé depuis api.js
 */

const ComponentDefinitions = {
  /**
   * Cache interne des définitions de composants
   * @private
   */
  _cache: [],

  /**
   * Charge les définitions de composants depuis l'API backend
   * Détecte automatiquement si la pagination est activée et charge toutes les pages
   * @returns {Promise<Array>} Tableau des définitions de composants
   */
  async load() {
    try {
      // Faire une première requête normale (sans paramètres de pagination)
      const r = await fetch('/api/components/definitions');
      if(!r.ok) {
        console.warn('[ComponentDefinitions.load] Erreur chargement définitions:', r.status);
        return [];
      }
      
      // Vérifier si la pagination est activée (présence du header X-Total-Pages)
      const totalPagesHeader = r.headers.get('X-Total-Pages');
      const totalCountHeader = r.headers.get('X-Total-Count');
      
      console.log('[ComponentDefinitions.load] Headers pagination:', {
        'X-Total-Pages': totalPagesHeader,
        'X-Total-Count': totalCountHeader
      });
      
      // Si les headers de pagination sont présents, la pagination est activée
      if (totalPagesHeader !== null && totalCountHeader !== null) {
        const totalPages = parseInt(totalPagesHeader);
        const totalCount = parseInt(totalCountHeader);
        
        if (totalCount > 0) {
          // Mode pagination activé : charger toutes les pages (même s'il n'y en a qu'une)
          console.log('[ComponentDefinitions.load] Pagination détectée, chargement de toutes les pages...', 
                     `(totalPages: ${totalPages}, totalCount: ${totalCount})`);
          this._cache = [];
          
          // Charger toutes les pages
          // Utiliser limit=5 pour correspondre au default du backend (buffer 12KB = ~5 composants par page)
          for (let page = 0; page < totalPages; page++) {
            const pageR = await fetch(`/api/components/definitions?page=${page}&limit=5`);
            if (!pageR.ok) {
              console.warn(`[ComponentDefinitions.load] Erreur page ${page}:`, pageR.status);
              break;
            }
            
            const pageData = await pageR.json();
            if (!Array.isArray(pageData)) {
              console.warn(`[ComponentDefinitions.load] Page ${page} invalide (pas un tableau)`);
              break;
            }
            
            if (pageData.length === 0) {
              console.log(`[ComponentDefinitions.load] Page ${page} vide, arrêt`);
              break; // Page vide, arrêter
            }
            
            this._cache.push(...pageData);
            console.log(`[ComponentDefinitions.load] Page ${page} chargée: ${pageData.length} composants`);
          }
          
          console.log(`[ComponentDefinitions.load] Composants chargés (pagination): ${this._cache.length}/${totalCount}`);
          console.log('[ComponentDefinitions.load] IDs des composants (pagination):', this._cache.map(d => d.id).join(', '));
          return this._cache;
        }
      }
      
      // Mode normal : utiliser les données de la première requête
      const data = await r.json();
      if (!Array.isArray(data)) {
        console.error('[ComponentDefinitions.load] Réponse invalide (pas un tableau):', data);
        return [];
      }
      this._cache = data;
      console.log('[ComponentDefinitions.load] Composants chargés:', this._cache.length);
      console.log('[ComponentDefinitions.load] IDs des composants:', this._cache.map(d => d.id).join(', '));
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
      console.warn('[ComponentDefinitions.getById] Cache vide pour:', componentId, '(cache:', this._cache, ')');
      return null;
    }
    const found = this._cache.find(def => def.id === componentId);
    if (!found) {
      console.warn('[ComponentDefinitions.getById] Composant non trouvé:', componentId, '(composants disponibles:', this._cache.map(d => d.id).join(', '), ')');
    }
    return found || null;
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
