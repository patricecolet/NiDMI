/**
 * @file pin-list.js
 * @brief Gestion de la liste des pins configurées avec support de nom personnalisé
 */

function updatePinsList() {
  const pl = $('#pinsList');
  if (!pl) return;
  pl.innerHTML = '';

  const roleCounters = {}; 
  const savedComplexMainPinGpios = new Set();

  if (typeof pcfg !== 'undefined' && pcfg && caps && caps.pins) {
    Object.keys(pcfg).forEach(lbl => {
      const cfg = pcfg[lbl];
      if (!cfg || !cfg.role) return;
      const cfgRole = typeof migrateRole === 'function' ? migrateRole(cfg.role) : cfg.role;
      if (typeof isBusRole === 'function' && isBusRole(cfgRole)) return;
      const cfgDef = typeof getComponentDefinition === 'function' ? getComponentDefinition(cfgRole) : null;
      const cfgHasAdditionalPins = cfgDef && cfgDef.additionalPins && Array.isArray(cfgDef.additionalPins) && cfgDef.additionalPins.length > 0
        && cfg.additionalPins && typeof cfg.additionalPins === 'object' && Object.keys(cfg.additionalPins).length > 0;
      if (cfgHasAdditionalPins) {
        const complexPin = caps.pins.find(p => p.label === lbl);
        if (complexPin && complexPin.gpio !== undefined) {
          const mainPinGpio = parseInt(complexPin.gpio);
          if (!isNaN(mainPinGpio)) savedComplexMainPinGpios.add(mainPinGpio);
        }
      }
    });
  }

  if (typeof pcfg === 'undefined' || !pcfg) return;

  Object.keys(pcfg).forEach(lbl => {
    const cfg = pcfg[lbl];
    if (!cfg || !cfg.role || lbl.startsWith('M')) return;

    // --- LOGIQUE DE NUMÉROTATION ET NOM ---
    roleCounters[cfg.role] = (roleCounters[cfg.role] || 0) + 1;
    const currentCount = roleCounters[cfg.role];
    
    // On passe cfg.name (le nom personnalisé) à la fonction de génération de label
    const roleName = getRoleDisplayLabel(cfg.role, currentCount, cfg.name);
    // --------------------------------------

    const role = typeof migrateRole === 'function' ? migrateRole(cfg.role) : cfg.role;
    /* Les rôles de bus (I2C, SPI, UART) ne sont pas des composants */
    if (typeof isBusRole === 'function' && isBusRole(role)) return;
    const def = typeof getComponentDefinition === 'function' ? getComponentDefinition(role) : null;
    const hasAdditionalPinsFromDef = def && def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0;
    const hasAdditionalPinsInCfg = cfg.additionalPins && typeof cfg.additionalPins === 'object' && Object.keys(cfg.additionalPins).length > 0;
    const hasAdditionalPins = hasAdditionalPinsFromDef && hasAdditionalPinsInCfg;

    if (hasAdditionalPins) {
      if (caps && caps.pins) {
        const pin = caps.pins.find(p => p.label === lbl);
        if (pin && savedComplexMainPinGpios.has(parseInt(pin.gpio)) && parseInt(pin.gpio) !== parseInt(caps.pins.find(p => p.label === lbl)?.gpio || 0)) return;

        const statText = stat(cfg, lbl);
        const it = document.createElement('div');
        it.className = `item complex ${pType(lbl)}`;
        it.innerHTML = `<span class="lbl">${lbl}</span><span class="role">${roleName}</span><span class="stat">${statText}</span><button class="del-btn">×</button>`;
        
        setupPinEvents(it, lbl, roleName);
        pl.appendChild(it);
      }
      return;
    }

    if (caps && caps.pins) {
      const pin = caps.pins.find(p => p.label === lbl);
      if (pin && savedComplexMainPinGpios.has(parseInt(pin.gpio))) return;
    }

    const it = document.createElement('div');
    it.className = `item ${pType(lbl)}`;
    it.innerHTML = `<span class="lbl">${lbl}</span><span class="role">${roleName}</span><span class="stat">${stat(cfg, lbl)}</span><button class="del-btn">×</button>`;
    
    setupPinEvents(it, lbl, roleName);
    pl.appendChild(it);
  });
}

/**
 * Attache les événements et synchronise l'input "Nom du composant"
 */
function setupPinEvents(it, lbl, defaultGeneratedName) {
  it.onclick = () => {
    if (window._selRect) window._selRect.classList.remove('selectedSquare');
    const r = prect[lbl];
    if (r) {
      window._selRect = r;
      r.classList.add('selectedSquare');
    }
    cur = lbl;
    $('#selPin').textContent = lbl;
    updFunc(lbl);

    // --- MISE À JOUR DE L'INPUT HTML ---
    const nameInput = document.getElementById('ComponentName');
    if (nameInput && pcfg[lbl]) {
      // Affiche le nom personnalisé sauvegardé, ou le nom généré par défaut
      nameInput.value = pcfg[lbl].name || defaultGeneratedName;
    }

    if (pcfg[lbl]) applyCfg(pcfg[lbl]);
  };

  const delBtn = it.querySelector('.del-btn');
  if (delBtn) {
    delBtn.onclick = (e) => {
      e.stopPropagation();
      delete pcfg[lbl];
      updatePinsList();
      if (typeof updateBusVisuals === 'function') updateBusVisuals();
    };
  }

}