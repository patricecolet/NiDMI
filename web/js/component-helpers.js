/**
 * @file component-helpers.js
 * @brief Helpers pour accéder aux définitions de composants et utilitaires de base
 */

/**
 * Obtient une définition de composant par son ID
 * @param {string} roleId - ID du rôle (ex: "potentiometer", "hc4067")
 * @returns {Object|null} Définition du composant ou null
 */
function getComponentDef(roleId) {
  return typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.getById
    ? ComponentDefinitions.getById(roleId)
    : (typeof getComponentDefinition === 'function' ? getComponentDefinition(roleId) : null);
}

/**
 * Obtient le cache des définitions de composants
 * @returns {Array} Tableau des définitions
 */
function getDefsCache() {
  return typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.cache 
    ? ComponentDefinitions.cache 
    : (typeof componentDefinitions !== 'undefined' ? componentDefinitions : []);
}

/**
 * Migre un rôle vers sa nouvelle valeur si nécessaire
 * @param {string} role - Rôle à migrer
 * @returns {string} Rôle migré
 */
function migrateRoleValue(role) {
  return typeof migrateRole === 'function' ? migrateRole(role) : role;
}

/**
 * Vérifie si un composant a des pins additionnelles
 * @param {Object} def - Définition du composant
 * @returns {boolean} true si le composant a des additionalPins
 */
function hasAdditionalPins(def) {
  if (!def) return false;
  return def.additionalPinCount > 0 || 
         (def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0);
}
