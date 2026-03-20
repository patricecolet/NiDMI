/**
 * @file pin-utils.js
 * @brief Utilitaires de formatage et affichage pour les pins
 */

/**
 * Détermine le type de pin pour le CSS (affichage visuel uniquement)
 * @param {string} lbl - Label de la pin
 * @returns {string} Type CSS (analog, digital, uart, i2c, spi)
 */
function pType(lbl) {
  if (!lbl) return 'digital';
  if (lbl.startsWith('A')) return 'analog';
  if (lbl.startsWith('D')) {
    if (!caps || !caps.pins || !caps.bus) return 'digital';
    const pin = caps.pins.find(p => p && p.label === lbl);
    if (!pin) return 'digital';
    const gpio = parseInt(pin.gpio);
    if (isNaN(gpio)) return 'digital';
    if (caps.bus.uart && (gpio === caps.bus.uart.tx || gpio === caps.bus.uart.rx)) return 'uart';
    if (caps.bus.i2c && (gpio === caps.bus.i2c.sda || gpio === caps.bus.i2c.scl)) return 'i2c';
    if (caps.bus.spi && (gpio === caps.bus.spi.mosi || gpio === caps.bus.spi.miso || gpio === caps.bus.spi.sck)) return 'spi';
    return 'digital';
  }
  if (lbl === 'SDA' || lbl === 'SCL') return 'i2c';
  if (lbl === 'TX' || lbl === 'RX') return 'uart';
  if (lbl === 'MOSI' || lbl === 'MISO' || lbl === 'SCK') return 'spi';
  return 'digital';
}

/**
 * Obtient le label d'affichage d'un rôle
 * @param {string} role - Rôle du composant
 * @returns {string} Label d'affichage
 */
function getRoleDisplayLabel(role, count, customName) {
  // PRIORITÉ 1 : Si un nom personnalisé existe, on l'utilise direct
  if (customName && customName.trim() !== '') {
    return customName;
  }
  
  // PRIORITÉ 2 : Sinon, on génère le nom par défaut
  if (!role) return '';
  const migratedRole = typeof migrateRole === 'function' ? migrateRole(role) : role;
  let displayName = migratedRole;
  
  if (typeof getComponentDefinition === 'function') {
    const def = getComponentDefinition(migratedRole);
    if (def && def.displayName) displayName = def.displayName;
  }
  
  // Ajoute le numéro (ex: Bouton 1) seulement s'il n'y a pas de nom custom
  return count > 0 ? `${displayName} ${count}` : displayName;
}

/**
 * Remplace les variables dans un template avec les valeurs de la config
 * @param {string} template - Template avec variables (ex: "CC#{cc}", "Note {note}")
 * @param {Object} cfg - Configuration du composant
 * @param {Object} def - Définition du composant (optionnel)
 * @returns {string} Texte avec variables remplacées
 */
function replaceTemplate(template, cfg, def) {
  if (!template) return '';
  
  /* Utiliser le mapping du backend si disponible */
  let valueMappings = {};
  if (def && def.statusValueMappings) {
    try {
      /* Si c'est déjà un objet, l'utiliser directement */
      if (typeof def.statusValueMappings === 'object') {
        valueMappings = def.statusValueMappings;
      } else if (typeof def.statusValueMappings === 'string') {
        /* Sinon, essayer de parser la string JSON */
        valueMappings = JSON.parse(def.statusValueMappings);
      }
    } catch (e) {
      console.warn('Erreur parsing statusValueMappings:', e, 'value:', def.statusValueMappings);
      valueMappings = {};
    }
  }
  
  /* Remplacer les variables {variable} avec les valeurs de cfg */
  return template.replace(/\{(\w+)\}/g, (match, key) => {
    // Essayer d'abord la clé directe
    let value = cfg[key];
    
    // Si pas trouvé, essayer avec le préfixe 'midi' (ex: cc -> midiCc, note -> midiNote)
    if (value === undefined || value === null) {
      const midiKey = 'midi' + key.charAt(0).toUpperCase() + key.slice(1);
      value = cfg[midiKey];
    }
    
    // Si toujours pas trouvé, essayer quelques variantes communes
    if (value === undefined || value === null) {
      const keyMap = {
        'cc': 'midiCc',
        'note': 'midiNote',
        'pc': 'midiPc',
        'channel': 'midiChannel',
        'chan': 'midiChannel',
        'velocity': 'midiVelocity',
        'vel': 'midiVelocity'
      };
      if (keyMap[key]) {
        value = cfg[keyMap[key]];
      }
    }
    
    // Si toujours pas trouvé, essayer l'ancien format rtp* pour compatibilité
    if (value === undefined || value === null) {
      const rtpKey = 'rtp' + key.charAt(0).toUpperCase() + key.slice(1);
      value = cfg[rtpKey];
    }
    
    // Si toujours pas trouvé, essayer les variantes rtp* communes
    if (value === undefined || value === null) {
      const rtpKeyMap = {
        'cc': 'rtpCc',
        'note': 'rtpNote',
        'pc': 'rtpPc',
        'channel': 'rtpChan',
        'chan': 'rtpChan',
        'velocity': 'rtpVel',
        'vel': 'rtpVel'
      };
      if (rtpKeyMap[key]) {
        value = cfg[rtpKeyMap[key]];
      }
    }
    
    /* Si la valeur est undefined ou null, utiliser une valeur par défaut selon la clé */
    if (value === undefined || value === null) {
      if (key === 'cc') return '7';
      if (key === 'note') return '60';
      if (key === 'pc') return '0';
      return '';
    }
    
    /* Vérifier si on a un mapping pour cette clé (depuis le backend) */
    if (valueMappings[key] && valueMappings[key][value]) {
      return valueMappings[key][value];
    }
    
    return String(value);
  });
}

/**
 * Génère le texte de statut d'un composant en utilisant les templates du backend
 * @param {Object} def - Définition du composant depuis le backend
 * @param {Object} cfg - Configuration du composant
 * @param {string} pinLabel - Label de la pin (pour les bus)
 * @returns {string} Texte de statut formaté
 */
function getComponentStatusText(def, cfg, pinLabel) {
  if (!def || !cfg) return '';
  
  /* Si midiMessageType (ou rtpType pour compatibilité) est défini, utiliser le template du message MIDI */
  const msgType = cfg.midiMessageType || cfg.rtpType; // Nouveau format puis ancien pour compatibilité
  if (msgType && def.midiMessages && Array.isArray(def.midiMessages)) {
    const msg = def.midiMessages.find(m => m.displayName === msgType);
    if (msg && msg.statusTemplate) {
      return replaceTemplate(msg.statusTemplate, cfg, def);
    }
  }
  
  /* Cas spéciaux pour les bus (UART, I2C, SPI) */
  /* Ces composants n'ont pas de définition standard, on les gère ici */
  if (cfg.role === 'uart' && typeof caps !== 'undefined' && caps) {
    const pin = caps.pins?.find(p => p.label === pinLabel);
    if (pin && caps.bus?.uart) {
      return pin.gpio === caps.bus.uart.tx ? 'TX' : 'RX';
    }
  }
  
  /* Si pas de rtpType mais qu'on a un statusTextTemplate, l'utiliser */
  /* (ex: pour LED avec ledMode, le backend pourrait fournir un template) */
  if (def.statusTextTemplate) {
    return replaceTemplate(def.statusTextTemplate, cfg, def);
  }
  
  /* Fallback: utiliser le displayName */
  return def.displayName || '';
}

/**
 * Génère un résumé court de la configuration d'un composant
 * Utilise les définitions du backend pour déterminer le support MIDI
 * @param {Object} cfg - Configuration du composant
 * @param {string} pinLabel - Label de la pin
 * @returns {string} Résumé court (ex: "CC#7", "Note 60")
 */
function stat(cfg, pinLabel) {
  if (!cfg || !cfg.role) return '';
  
  /* Migrer les anciens formats si nécessaire */
  const migratedRole = typeof migrateRole === 'function' ? migrateRole(cfg.role) : cfg.role;
  
  /* Obtenir la définition du composant depuis le backend */
  const def = typeof getComponentDefinition === 'function' ? getComponentDefinition(migratedRole) : null;
  
  /* Utiliser la fonction générique pour générer le texte de statut */
  if (def && typeof getComponentStatusText === 'function') {
    return getComponentStatusText(def, cfg, pinLabel);
  }
  
  /* Fallback si les définitions ne sont pas disponibles */
  return cfg.role || '';
}
