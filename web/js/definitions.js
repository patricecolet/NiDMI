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

/**
 * Normalise un objet de définition compacte (clés courtes envoyées par le firmware)
 * vers le format complet attendu par le reste du frontend.
 * Table : dn→displayName  cid→cardId  fn→familyName  pt→pinType  apt→altPinType
 *         impl→implemented  midi→supportsMidi  osc→supportsOsc  apc→additionalPinCount
 *         stt→statusTextTemplate  svm→statusValueMappings  ap→additionalPins
 *         mm→midiMessages  st→statusTemplate  p→params  ph→placeholder  dv→defaultValue
 *         dmin→defaultMin  dmax→defaultMax  sep→separator  hc→hintClass  dep→dependsOnRole
 *         ff→formFields  ml→maxLength  hp→hintPosition  don→dependsOn  sw→showWhen
 *         wc→wrapperClass  ic→inputClass  w→width  req→required  lb→labelBefore  la→labelAfter
 *         o→options (déjà un tableau, pas de JSON.parse nécessaire)
 */
function normalizeDef(d) {
  if (!d) return d;
  /* paramètres MIDI */
  const normParam = p => !p ? p : {
    id: p.id, type: p.type, label: p.label,
    min: p.min, max: p.max,
    placeholder: p.ph, defaultValue: p.dv,
    defaultMin: p.dmin, defaultMax: p.dmax, separator: p.sep,
    hint: p.hint, hintClass: p.hc,
    dependsOnRole: p.dep, width: p.w
  };
  /* messages MIDI */
  const normMsg = m => !m ? m : {
    id: m.id, displayName: m.dn, axis: m.axis,
    statusTemplate: m.st,
    params: m.p ? m.p.map(normParam) : []
  };
  /* form fields */
  const normField = f => !f ? f : {
    id: f.id, type: f.type, label: f.label,
    required: f.req, placeholder: f.ph,
    maxLength: f.ml, pattern: f.pattern,
    min: f.min, max: f.max, step: f.step,
    options: f.o,           /* o est déjà un tableau inline */
    separator: f.sep, defaultValue: f.dv,
    hintPosition: f.hp, hint: f.hint, hintClass: f.hc,
    dependsOn: f.don, showWhen: f.sw,
    wrapperClass: f.wc, inputClass: f.ic, width: f.w,
    labelBefore: f.lb, labelAfter: f.la
  };
  /* additionalPins */
  const normPin = p => !p ? p : {
    id: p.id, displayName: p.dn, pinType: p.pt, optional: p.optional
  };
  return {
    id: d.id,
    displayName: d.dn,
    cardId: d.cid,
    family: d.family,
    familyName: d.fn,
    pinType: d.pt,
    altPinType: d.apt,
    implemented: d.impl,
    supportsMidi: d.midi,
    supportsOsc: d.osc,
    additionalPinCount: d.apc,
    statusTextTemplate: d.stt,
    statusValueMappings: d.svm,
    additionalPins: d.ap ? d.ap.map(normPin) : [],
    midiMessages: d.mm ? d.mm.map(normMsg) : [],
    formFields: d.ff ? d.ff.map(normField) : []
  };
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
      /* limit=1 : le serveur force 1 composant/page (beginResponse_P, zéro copie heap).
       * 41 pages × 70ms ≈ 3s — acceptable et fiable même avec 29 KB heap libre. */
      const firstLimit = 1;
      let r = null;
      let firstText = '';
      for (let attempt = 1; attempt <= 3; attempt++) {
        try {
          r = await fetch(`/api/components/definitions?page=0&limit=${firstLimit}`);
          if (r.ok) {
            firstText = await r.text();
            if (firstText && firstText.trim()) break;
            console.warn(`[ComponentDefinitions.load] Page 0 vide (tentative ${attempt}/3)`);
          } else {
            console.warn(`[ComponentDefinitions.load] Erreur chargement définitions (tentative ${attempt}/3):`, r.status);
          }
        } catch (e) {
          console.warn(`[ComponentDefinitions.load] Erreur réseau page 0 (tentative ${attempt}/3):`, e);
        }
        await new Promise(resolve => setTimeout(resolve, 200 * attempt));
      }
      if (!r || !r.ok || !firstText || !firstText.trim()) {
        return [];
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
        const totalCount = parseInt(totalCountHeader);
        const perPage = parseInt(r.headers.get('X-Per-Page')) || 5;
        const totalPages = Math.ceil(totalCount / perPage);
        
        if (totalCount > 0) {
          console.log('[ComponentDefinitions.load] Pagination détectée',
                     `(totalCount: ${totalCount}, perPage: ${perPage}, pages: ${totalPages})`);
          
          /* Réutiliser le body de la première requête comme page 0
           * (évite un fetch supplémentaire et la pression mémoire sur l'ESP32) */
          const page0 = parseDefinitionsJson(firstText);
          const pageMap = new Map();
          pageMap.set(0, Array.isArray(page0) ? page0 : []);
          console.log(`[ComponentDefinitions.load] Page 0 (initiale): ${pageMap.get(0).length} composants`);

          const fetchPageWithRetry = async (page, maxAttempts = 5) => {
            for (let attempt = 1; attempt <= maxAttempts; attempt++) {
              try {
                const pageR = await fetch(`/api/components/definitions?page=${page}&limit=${perPage}`);
                if (pageR.ok) {
                  const pageText = await pageR.text();
                  if (pageText && pageText.trim()) {
                    const parsed = parseDefinitionsJson(pageText);
                    if (Array.isArray(parsed) && parsed.length > 0) return parsed;
                  } else {
                    console.warn(`[ComponentDefinitions.load] Page ${page} vide (tentative ${attempt}/${maxAttempts})`);
                  }
                } else {
                  console.warn(`[ComponentDefinitions.load] Page ${page} HTTP ${pageR.status} (tentative ${attempt}/${maxAttempts})`);
                }
              } catch (e) {
                console.warn(`[ComponentDefinitions.load] Page ${page} erreur réseau (tentative ${attempt}/${maxAttempts}):`, e);
              }
              await new Promise(resolve => setTimeout(resolve, 220 * attempt));
            }
            return null;
          };

          const failedPages = [];
          for (let page = 1; page < totalPages; page++) {
            await new Promise(resolve => setTimeout(resolve, 70));
            const pageData = await fetchPageWithRetry(page, 4);
            if (Array.isArray(pageData) && pageData.length > 0) {
              pageMap.set(page, pageData);
              console.log(`[ComponentDefinitions.load] Page ${page}: ${pageData.length} composants`);
            } else {
              failedPages.push(page);
            }
          }

          /* Deuxième passe pour les pages ratées (ne pas abandonner tout le chargement) */
          for (const page of failedPages) {
            await new Promise(resolve => setTimeout(resolve, 350));
            const pageData = await fetchPageWithRetry(page, 6);
            if (Array.isArray(pageData) && pageData.length > 0) {
              pageMap.set(page, pageData);
              console.log(`[ComponentDefinitions.load] Page ${page} récupérée en 2e passe: ${pageData.length} composants`);
            } else {
              console.warn(`[ComponentDefinitions.load] Page ${page} irrécupérable après 2 passes`);
            }
          }

          this._cache = [];
          for (let page = 0; page < totalPages; page++) {
            const pageData = pageMap.get(page);
            if (Array.isArray(pageData) && pageData.length > 0) {
              this._cache.push(...pageData.map(normalizeDef));
            }
          }

          console.log(`[ComponentDefinitions.load] Total chargé: ${this._cache.length}/${totalCount}`);
          if (this._cache.length < totalCount) {
            console.warn(`[ComponentDefinitions.load] Chargement partiel: ${this._cache.length}/${totalCount}`);
          }
          console.log('[ComponentDefinitions.load] IDs:', this._cache.map(d => d.id).join(', '));
          return this._cache;
        }
      }

      /* Mode normal : parser le texte de la première requête (avec secours si caractères de contrôle) */
      const data = parseDefinitionsJson(firstText);
      if (!Array.isArray(data)) {
        console.error('[ComponentDefinitions.load] Réponse invalide (pas un tableau):', data);
        return [];
      }
      this._cache = data.map(normalizeDef);
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
