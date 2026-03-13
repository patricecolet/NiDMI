/**
 * Gestion des capacités GPIO et des GPIOs utilisés.
 * Fonctions extraites de api.js et exposées globalement pour compatibilité.
 */

(function(global) {
  
  let cachedUsedGpios = new Set();

  /**
   * Charge les capacités des pins depuis le backend.
   * Renseigne la variable globale caps.
   */
  async function loadCaps(){
    const r = await fetch('/api/pins/caps');
    caps = await r.json();
  }

  /**
   * Charge les GPIOs utilisés depuis le backend et met à jour le cache.
   * @returns {Promise<Set<number>>}
   */
  async function loadUsedGpiosFromBackend() {
    try {
      const r = await fetch('/api/components/used-gpios');
      if(!r.ok) {
        console.warn('Erreur chargement GPIOs utilisés:', r.status);
        return new Set();
      }
      const data = await r.json();
      cachedUsedGpios = new Set(data.gpios || []);
      console.log('[loadUsedGpiosFromBackend] GPIOs utilisés:', Array.from(cachedUsedGpios));
      return cachedUsedGpios;
    } catch(err) {
      console.warn('Erreur chargement GPIOs utilisés:', err);
      return new Set();
    }
  }

  /**
   * Retourne le cache des GPIOs utilisés.
   * @returns {Set<number>}
   */
  function getCachedUsedGpios() {
    return cachedUsedGpios;
  }

  /**
   * Calcule les GPIOs utilisés en tenant compte de la config en cours d'édition.
   * @param {Object} editingConfig
   * @returns {Set<number>}
   */
  function getUsedGpiosWithEditing(editingConfig) {
    const gpios = new Set(cachedUsedGpios);

    if(!editingConfig) return gpios;

    /* Ajouter le GPIO principal */
    if(editingConfig.gpio !== undefined && editingConfig.gpio !== null) {
      gpios.add(parseInt(editingConfig.gpio));
    }

    /* Pour les composants complexes, utiliser les additionalPins du backend */
    if(editingConfig.role && typeof getComponentDefinition === 'function') {
      const migratedRole = typeof migrateRole === 'function' ? migrateRole(editingConfig.role) : editingConfig.role;
      const def = getComponentDefinition(migratedRole);

      if(def && def.additionalPins && def.additionalPinCount > 0) {
        /* Parcourir les pins additionnelles définies par le backend */
        def.additionalPins.forEach(pinDef => {
          const pinId = pinDef.id; /* ex: "s0", "s1", "en" */
          const gpio = editingConfig[pinId];
          if(gpio !== undefined && gpio !== null && gpio !== 255) {
            gpios.add(parseInt(gpio));
          }
        });
      }
    }

    return gpios;
  }

  // Exposer les fonctions globalement (compatibilité) et via un namespace.
  global.loadCaps = loadCaps;
  global.loadUsedGpiosFromBackend = loadUsedGpiosFromBackend;
  global.getCachedUsedGpios = getCachedUsedGpios;
  global.getUsedGpiosWithEditing = getUsedGpiosWithEditing;

  global.ApiGpios = {
    loadCaps,
    loadUsedGpiosFromBackend,
    getCachedUsedGpios,
    getUsedGpiosWithEditing
  };
})(typeof window !== 'undefined' ? window : this);

