/**
 * @file component-menus.js
 * @brief Gestion des menus Famille et Composant
 */

/**
 * Vérifie si un composant avec additionalPins est disponible pour une pin
 * @param {Object} def - Définition du composant
 * @param {number} pinType - Type de pin (0=ANALOG, 1=DIGITAL)
 * @param {Object} pin - Objet pin avec gpio
 * @returns {boolean} true si le composant est disponible
 * 
 * NOTE: Restrictions désactivées pour ne pas bloquer le développement
 * Tous les composants sont maintenant toujours disponibles
 */
function checkComponentAvailability(def, pinType, pin) {
  /* RESTRICTIONS DÉSACTIVÉES - TOUS LES COMPOSANTS SONT DISPONIBLES */
  return true;
  
  /* CODE COMMENTÉ - LOGIQUE DE VÉRIFICATION DES ADDITIONAL PINS
  // Composants sans additionalPins sont toujours disponibles
  if (!hasAdditionalPins(def)) return true;
  
  // Pour l'instant, on vérifie uniquement pour les pins analogiques
  // (les composants avec additionalPins sont généralement sur pins analogiques)
  if (pinType !== 0) return true;
  
  if (!pin || pin.gpio === undefined) return false;
  
  if (typeof GpioManager === 'undefined') {
    return false;
  }
  
  try {
    const sigGpio = parseInt(pin.gpio);
    if (isNaN(sigGpio)) return false;
    
    // Utiliser la nouvelle fonction générique basée sur la définition
    if (typeof GpioManager.areAdditionalPinsAvailable === 'function') {
      // Obtenir les GPIOs déjà utilisés (depuis pcfg)
      const usedGpios = GpioManager.getUsedGpios ? GpioManager.getUsedGpios([]) : new Set();
      return GpioManager.areAdditionalPinsAvailable(def, sigGpio, usedGpios);
    }
    
    // Fallback pour compatibilité (ancienne méthode hardcodée)
    if (typeof GpioManager.areAddressPinsAvailable === 'function') {
      const usedGpios = GpioManager.getUsedGpios ? GpioManager.getUsedGpios([]) : new Set();
      return GpioManager.areAddressPinsAvailable(sigGpio, usedGpios);
    }
    
    return false;
  } catch (err) {
    console.warn('[checkComponentAvailability] Erreur vérification disponibilité:', err);
    return false;
  }
  */
}

/**
 * Groupe les composants par famille et remplit le menu Famille
 * @param {number} pinType - Type de pin (0=ANALOG, 1=DIGITAL, 3=PWM)
 * @param {Object} pin - Objet pin avec gpio
 */
function populateFamilySelect(pinType, pin) {
  const familySel = $('#familySelect');
  if (!familySel) return;

  const defsCache = getDefsCache();
  console.log('[populateFamilySelect] pinType=', pinType, 'defsCache.length=', defsCache.length);

  if (pinType === null || !defsCache || defsCache.length === 0) {
    console.log('[populateFamilySelect] Pas de définitions, menu vide');
    setOptions(familySel, {}, 0);
    return;
  }

  /* Obtenir les composants compatibles avec le type de pin (implémentés uniquement pour les familles) */
  const compatibleComponents = (typeof ComponentDefinitions !== 'undefined' && ComponentDefinitions.getForPinType)
    ? ComponentDefinitions.getForPinType(pinType, true)
    : (typeof getComponentsForPinType === 'function' ? getComponentsForPinType(pinType, true) : []);

  console.log('[populateFamilySelect] compatibleComponents:', compatibleComponents.length);

  /* Grouper par famille */
  const familiesMap = new Map();
  compatibleComponents.forEach(def => {
    if (!def.implemented) return;

    const familyId = def.family !== undefined ? def.family : 0;
    const familyName = def.familyName || 'Basic';

    if (!familiesMap.has(familyId)) {
      familiesMap.set(familyId, {
        id: familyId,
        name: familyName,
        components: []
      });
    }

    /* RESTRICTIONS DÉSACTIVÉES - TOUS LES COMPOSANTS SONT AJOUTÉS */
    familiesMap.get(familyId).components.push(def);
    
    /* CODE COMMENTÉ - VÉRIFICATION DE DISPONIBILITÉ
    // Vérifier disponibilité pour les composants avec additionalPins
    if (checkComponentAvailability(def, pinType, pin)) {
      familiesMap.get(familyId).components.push(def);
    }
    */
  });

  /* Créer les options pour le menu Famille */
  const familyOptions = {};
  Array.from(familiesMap.values()).forEach(fam => {
    if (fam.components.length > 0) {
      familyOptions[fam.id] = fam.name;
      console.log(`[populateFamilySelect] Famille ${fam.id} (${fam.name}): ${fam.components.length} composants`);
    }
  });

  console.log('[populateFamilySelect] Options famille:', familyOptions);

  /* Déclencher le filtrage du menu Composant */
  const selectedFamilyId = familySel.value ? parseInt(familySel.value) : null;

  /* Sélectionner automatiquement la première famille si aucune n'est sélectionnée */
  if (!familySel.value && Object.keys(familyOptions).length > 0) {
    familySel._restoringConfig = true;
  }

  /* Maintenant remplir le select */
  setOptions(familySel, familyOptions, 0);

  /* Si on a mis le flag, le réinitialiser après un délai */
  if (familySel._restoringConfig) {
    const firstFamilyId = parseInt(Object.keys(familyOptions)[0]);
    setTimeout(() => {
      familySel._restoringConfig = false;
    }, 10);
    /* Déclencher manuellement populateComponentSelect pour la première famille */
    if (selectedFamilyId === null) {
      populateComponentSelect(firstFamilyId, pinType, pin);
      return;
    }
  }

  /* Déclencher le filtrage du menu Composant si une famille était déjà sélectionnée */
  if (selectedFamilyId !== null) {
    populateComponentSelect(selectedFamilyId, pinType, pin);
  }
}

/**
 * Remplit le menu Composant selon la famille sélectionnée
 * @param {number} familyId - ID de la famille (0=BASIC, 1=MULTIPLEXER, etc.)
 * @param {number} pinType - Type de pin
 * @param {Object} pin - Objet pin avec gpio
 */
function populateComponentSelect(familyId, pinType, pin) {
  const compSel = $('#funcSelect');
  if (!compSel) return;

  console.log('[populateComponentSelect] familyId=', familyId, 'pinType=', pinType);

  const defsCache = getDefsCache();
  if (pinType === null || !defsCache || defsCache.length === 0) {
    console.log('[populateComponentSelect] Pas de définitions, menu vide');
    setOptions(compSel, {}, 0);
    return;
  }

  /* Obtenir TOUS les composants compatibles (y compris non implémentés) */
  const compatibleComponents = typeof getComponentsForPinType === 'function'
    ? getComponentsForPinType(pinType, false)
    : [];

  console.log('[populateComponentSelect] compatibleComponents:', compatibleComponents.length);

  /* Filtrer par famille */
  const familyComponents = compatibleComponents.filter(def => {
    const defFamilyId = def.family !== undefined ? def.family : 0;
    return defFamilyId === familyId;
  });

  console.log('[populateComponentSelect] familyComponents (familyId=' + familyId + '):', familyComponents.length, familyComponents.map(d => `${d.id} (implemented=${d.implemented})`));

  /* Trier les composants pour toujours mettre "button" en premier s'il est disponible */
  const sortedFamilyComponents = [...familyComponents].sort((a, b) => {
    if (a.id === 'button' && b.id !== 'button') return -1;
    if (b.id === 'button' && a.id !== 'button') return 1;
    return 0;
  });

  /* Créer les options pour le menu Composant */
  const options = {};
  sortedFamilyComponents.forEach(def => {
    /* RESTRICTIONS DÉSACTIVÉES - TOUS LES COMPOSANTS SONT DISPONIBLES */
    options[def.id] = {
      label: def.displayName,
      disabled: !def.implemented /* Désactiver uniquement si non implémenté */
    };
    
    /* CODE COMMENTÉ - VÉRIFICATION DE DISPONIBILITÉ
    const available = checkComponentAvailability(def, pinType, pin);
    
    // Afficher même si non disponible si non implémenté (pour info)
    if (available || !def.implemented) {
      options[def.id] = {
        label: def.displayName,
        disabled: !def.implemented || !available
      };
    }
    */
  });

  console.log('[populateComponentSelect] Options composant:', Object.keys(options));
  setOptions(compSel, options, 0);
}
