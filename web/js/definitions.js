/**
 * Gestion des définitions de composants (cache et accès)
 * Module refactorisé depuis api.js
 */

/**
 * Répare un tableau JSON tronqué par corruption mémoire (NUL bytes + garbage).
 * 1. Tronque au premier NUL (0x00)
 * 2. Cherche le dernier objet complet du tableau (profondeur = 1 après })
 * 3. Ferme le tableau et parse
 * @param {string} text - Réponse brute du backend
 * @returns {Array} Tableau parsé (éventuellement partiel), ou [] en dernier recours
 */
function parseDefinitionsJson(text) {
  /* Étape 1 : essai normal */
  try { return JSON.parse(text); } catch(firstErr) {
    /* Étape 2 : tronquer au premier NUL si présent */
    const nulIdx = text.indexOf('\u0000');
    if (nulIdx >= 0) {
      console.warn('[parseDefinitionsJson] NUL byte à position ' + nulIdx +
        ' (text.length=' + text.length + '). Corruption mémoire détectée, tentative de récupération...');
      text = text.substring(0, nulIdx);
    } else {
      console.warn('[parseDefinitionsJson] Erreur JSON sans NUL:', firstErr.message);
    }

    /* Étape 3 : essayer le texte tronqué tel quel */
    try { return JSON.parse(text); } catch(e2) { /* continue */ }

    /* Étape 4 : trouver le dernier objet top-level complet dans le tableau */
    if (text.length > 0 && text[0] === '[') {
      let depth = 0;
      let inStr = false;
      let esc = false;
      let lastCompleteObj = -1;

      for (let i = 0; i < text.length; i++) {
        const ch = text[i];
        if (esc) { esc = false; continue; }
        if (inStr) {
          if (ch === '\\') { esc = true; continue; }
          if (ch === '"') { inStr = false; }
          continue;
        }
        if (ch === '"') { inStr = true; continue; }
        if (ch === '{' || ch === '[') { depth++; continue; }
        if (ch === '}' || ch === ']') {
          depth--;
          /* depth 1 = on vient de fermer un objet top-level du tableau */
          if (depth === 1 && ch === '}') { lastCompleteObj = i; }
        }
      }

      if (lastCompleteObj > 0) {
        const repaired = text.substring(0, lastCompleteObj + 1) + ']';
        try {
          const result = JSON.parse(repaired);
          if (Array.isArray(result)) {
            console.warn('[parseDefinitionsJson] Récupération réussie: ' + result.length +
              ' composants sur ' + repaired.length + ' octets (tronqué de ' + (nulIdx >= 0 ? nulIdx : text.length) + ')');
            return result;
          }
        } catch(e3) {
          console.warn('[parseDefinitionsJson] Réparation échouée:', e3.message);
        }
      }
    }

    console.error('[parseDefinitionsJson] Impossible de parser les définitions. Réponse brute (200 premiers chars):',
      text.substring(0, 200));
    return [];
  }
}

const ComponentDefinitions = {
  /**
   * Cache interne des définitions de composants
   * @private
   */
  _cache: [],
  _loadingPromise: null,

  /**
   * Charge les définitions de composants depuis l'API backend
   * Détecte automatiquement si la pagination est activée et charge toutes les pages
   * @returns {Promise<Array>} Tableau des définitions de composants
   */
  async load() {
    if (this._loadingPromise) {
      return this._loadingPromise;
    }
    this._loadingPromise = (async () => {
    try {
      /* Faire une première requête normale (sans paramètres de pagination) */
      let r = await fetch('/api/components/definitions');
      if(!r.ok) {
        console.warn('[ComponentDefinitions.load] Erreur chargement définitions:', r.status);
        return [];
      }
      let firstText = await r.text();
      if (!firstText || !firstText.trim()) {
        console.warn('[ComponentDefinitions.load] Réponse vide sur requête initiale, retry...');
        r = await fetch('/api/components/definitions?_retry=1');
        if (!r.ok) {
          console.warn('[ComponentDefinitions.load] Retry échoué:', r.status);
          return [];
        }
        firstText = await r.text();
      }

      /* Vérifier si la pagination est activée (présence du header X-Total-Pages) */
      const totalPagesHeader = r.headers.get('X-Total-Pages');
      const totalCountHeader = r.headers.get('X-Total-Count');
      
      console.log('[ComponentDefinitions.load] Headers pagination:', {
        'X-Total-Pages': totalPagesHeader,
        'X-Total-Count': totalCountHeader
      });
      
      /* Si les headers de pagination sont présents, la pagination est activée */
      if (totalPagesHeader !== null && totalCountHeader !== null) {
        const totalPages = parseInt(totalPagesHeader);
        const totalCount = parseInt(totalCountHeader);
        
        if (totalCount > 0) {
          /* Mode pagination activé : charger toutes les pages (même s'il n'y en a qu'une) */
          console.log('[ComponentDefinitions.load] Pagination détectée, chargement de toutes les pages...', 
                     `(totalPages: ${totalPages}, totalCount: ${totalCount})`);
          this._cache = [];
          
          /* Charger toutes les pages */
          /* Utiliser limit=5 pour correspondre au default du backend (buffer 12KB = ~5 composants par page) */
          for (let page = 0; ; page++) {
            let pageR = await fetch(`/api/components/definitions?page=${page}&limit=3`);
            if (!pageR.ok) {
              console.warn(`[ComponentDefinitions.load] Erreur page ${page}:`, pageR.status);
              break;
            }
            let pageText = await pageR.text();
            if (!pageText || !pageText.trim()) {
              console.warn(`[ComponentDefinitions.load] Page ${page} vide, retry...`);
              pageR = await fetch(`/api/components/definitions?page=${page}&limit=3&_retry=1`);
              if (!pageR.ok) {
                console.warn(`[ComponentDefinitions.load] Retry page ${page} échoué:`, pageR.status);
                break;
              }
              pageText = await pageR.text();
              if (!pageText || !pageText.trim()) {
                console.warn(`[ComponentDefinitions.load] Page ${page} encore vide après retry, arrêt`);
                break;
              }
            }
            const pageData = parseDefinitionsJson(pageText);
            if (!Array.isArray(pageData)) {
              console.warn(`[ComponentDefinitions.load] Page ${page} invalide (pas un tableau)`);
              break;
            }
            
            if (pageData.length === 0) {
              console.log(`[ComponentDefinitions.load] Page ${page} vide, arrêt`);
              break; /* Page vide, arrêter */
            }
            
            this._cache.push(...pageData);
            console.log(`[ComponentDefinitions.load] Page ${page} chargée: ${pageData.length} composants`);
          }
          
          console.log(`[ComponentDefinitions.load] Composants chargés (pagination): ${this._cache.length}/${totalCount}`);
          console.log('[ComponentDefinitions.load] IDs des composants (pagination):', this._cache.map(d => d.id).join(', '));
          return this._cache;
        }
      }

      /* Mode normal : parser le texte de la première requête (avec secours si caractères de contrôle) */
      const data = parseDefinitionsJson(firstText);
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
    } finally {
      this._loadingPromise = null;
    }
    })();
    return this._loadingPromise;
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
      /* Filtrer par implémenté si demandé */
      if(implementedOnly && !def.implemented) return false;

      /* Vérifier la compatibilité du type de pin (primaire ou alternatif) */
      const matchesPrimary = (() => {
        switch(pinType) {
          case 0: /* PIN_ANALOG */
            return def.pinType === 0 || def.pinType === 2;
          case 1: /* PIN_DIGITAL */
            return def.pinType === 1 || def.pinType === 2;
          case 3: /* PIN_PWM */
            return def.pinType === 3;
          case 4: /* PIN_I2C */
            return def.pinType === 4;
          case 5: /* PIN_SPI */
            return def.pinType === 5;
          default:
            return false;
        }
      })();
      if(matchesPrimary) return true;
      if(def.altPinType !== undefined && def.altPinType !== null && def.altPinType >= 0) {
        return def.altPinType === pinType;
      }
      /* Fallback: composants I2C connus pour supporter aussi SPI (ex. LIS3DH) si le backend n'envoie pas altPinType */
      if(pinType === 5 && def.pinType === 4 && def.id === 'lis3dh') return true;
      return false;
    });
    
    console.log(`[ComponentDefinitions.getForPinType] pinType=${pinType}, implementedOnly=${implementedOnly}, trouvé ${filtered.length} composants`);
    return filtered;
  }
};
