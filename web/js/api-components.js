/**
 * API de haut niveau pour les définitions de composants.
 * S'appuie sur ComponentDefinitions (voir definitions.js).
 */

(function(global) {
  const hasComponentDefinitions = () =>
    typeof global.ComponentDefinitions !== 'undefined' &&
    global.ComponentDefinitions !== null;

  const ApiComponents = {
    /**
     * Charge les définitions de composants depuis le backend.
     * Retourne toujours un tableau (éventuellement vide).
     */
    async load() {
      if (hasComponentDefinitions() && typeof global.ComponentDefinitions.load === 'function') {
        const defs = await global.ComponentDefinitions.load();
        return Array.isArray(defs) ? defs : [];
      }

      // Fallback très simple si ComponentDefinitions n'est pas disponible
      try {
        const r = await fetch('/api/components/definitions');
        if (!r.ok) {
          console.warn('[ApiComponents.load] Erreur chargement définitions (fallback):', r.status);
          return [];
        }
        const data = await r.json();
        if (!Array.isArray(data)) {
          console.warn('[ApiComponents.load] Réponse invalide (fallback, pas un tableau):', data);
          return [];
        }
        return data;
      } catch (err) {
        console.error('[ApiComponents.load] Erreur (fallback):', err);
        return [];
      }
    },

    /**
     * Retourne la définition d'un composant par son ID.
     * @param {string} componentId
     * @returns {Object|null}
     */
    getById(componentId) {
      if (!componentId) return null;

      if (hasComponentDefinitions() && typeof global.ComponentDefinitions.getById === 'function') {
        return global.ComponentDefinitions.getById(componentId);
      }

      // Fallback : utiliser componentDefinitions si présent (ancien cache global)
      if (typeof global.componentDefinitions !== 'undefined' &&
          Array.isArray(global.componentDefinitions) &&
          global.componentDefinitions.length > 0) {
        const found = global.componentDefinitions.find(def => def.id === componentId);
        if (!found) {
          console.warn('[ApiComponents.getById] Composant non trouvé (fallback):', componentId);
        }
        return found || null;
      }

      console.warn('[ApiComponents.getById] Aucune définition disponible (ni ComponentDefinitions ni componentDefinitions)');
      return null;
    },

    /**
     * Retourne les définitions compatibles avec un type de pin donné.
     * @param {number} pinType
     * @param {boolean} implementedOnly
     * @returns {Array}
     */
    getForPinType(pinType, implementedOnly = true) {
      if (hasComponentDefinitions() && typeof global.ComponentDefinitions.getForPinType === 'function') {
        return global.ComponentDefinitions.getForPinType(pinType, implementedOnly);
      }

      // Fallback: déléguer à la fonction globale historique si elle existe,
      // sans réimplémenter la logique de compatibilité ici.
      if (typeof global.getComponentsForPinType === 'function') {
        return global.getComponentsForPinType(pinType, implementedOnly);
      }

      console.warn('[ApiComponents.getForPinType] Aucune implémentation disponible (ni ComponentDefinitions ni getComponentsForPinType), retour tableau vide (fallback)');
      return [];
    },

    /**
     * Convertit un displayName en ID backend normalisé.
     * Si le rôle est déjà un ID valide, il est retourné tel quel.
     * @param {string} role
     * @returns {string}
     */
    migrateRole(role) {
      if (!role) return role;

      // Si c'est déjà un ID bien formé (backend), le retourner tel quel
      if (/^[a-z0-9_-]+$/.test(role)) return role;

      const source = hasComponentDefinitions()
        ? (global.ComponentDefinitions.cache || [])
        : (Array.isArray(global.componentDefinitions) ? global.componentDefinitions : []);

      if (source && source.length > 0) {
        const def = source.find(d => d.displayName === role);
        if (def) return def.id;
      }

      return role;
    }
  };

  global.ApiComponents = ApiComponents;
})(typeof window !== 'undefined' ? window : this);

