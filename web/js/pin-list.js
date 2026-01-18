/**
 * @file pin-list.js
 * @brief Gestion de la liste des pins configurées
 */

/**
 * Met à jour la liste des pins configurées dans l'UI
 */
function updatePinsList() {
  const pl = $('#pinsList');
  if (!pl) return;
  pl.innerHTML = '';

  /* Collecter les GPIOs des composants complexes sauvegardés depuis pcfg */
  const savedComplexSigGpios = new Set();
  if (typeof pcfg !== 'undefined' && pcfg) {
    Object.keys(pcfg).forEach(lbl => {
      const cfg = pcfg[lbl];
      if (cfg && cfg.additionalPins && typeof cfg.additionalPins === 'object' && cfg.additionalPins.sig !== undefined) {
        const sigGpio = parseInt(cfg.additionalPins.sig);
        if (!isNaN(sigGpio)) savedComplexSigGpios.add(sigGpio);
      }
    });
  }

  /* Afficher les pins configurées (unifié depuis pcfg) */
  if (typeof pcfg === 'undefined' || !pcfg) return;
  Object.keys(pcfg).forEach(lbl => {
    const cfg = pcfg[lbl];
    if (!cfg || !cfg.role) return;
    /* Ignorer les pins avec préfixe M (anciennes pins avec préfixe historique) */
    if (lbl.startsWith('M')) return;

    /* Détecter composant avec additionalPins depuis pcfg.additionalPins */
    const hasAdditionalPins = cfg.additionalPins && typeof cfg.additionalPins === 'object' && cfg.additionalPins.sig !== undefined;
    const role = typeof migrateRole === 'function' ? migrateRole(cfg.role) : cfg.role;
    const def = typeof getComponentDefinition === 'function' ? getComponentDefinition(role) : null;
    const hasAdditionalPinsFromDef = def && (def.additionalPinCount > 0 || (def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0));
    const isComplex = hasAdditionalPins || hasAdditionalPinsFromDef;

    /* Pour les composants avec additionalPins : afficher depuis pcfg (unifié) */
    if (isComplex && hasAdditionalPins) {
      if (caps && caps.pins) {
        const pin = caps.pins.find(p => p.label === lbl);
        /* Si ce GPIO est déjà dans un autre composant complexe sauvegardé, ne pas afficher */
        if (pin && savedComplexSigGpios.has(parseInt(pin.gpio)) && parseInt(cfg.additionalPins.sig) !== parseInt(pin.gpio)) return;

        /* Afficher composant complexe depuis pcfg */
        const roleName = getRoleDisplayLabel(cfg.role);
        const statText = stat(cfg, lbl);
        const it = document.createElement('div');
        it.className = 'item complex';
        it.innerHTML = `<span class="lbl">${lbl}</span><span class="role">${roleName}</span><span class="stat">${statText}</span><button class="del-btn">×</button>`;
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
          /* SIMPLIFICATION : Appliquer la config si elle existe */
          if (pcfg[lbl]) {
            applyCfg(pcfg[lbl]);
          }
        };
        const delBtn = it.querySelector('.del-btn');
        if (delBtn) delBtn.onclick = (e) => {
          e.stopPropagation();
          console.log('[deletePin] Marquage pour suppression composant complexe sur pin:', lbl);
          /* Supprimer uniquement de pcfg (sauvegarde NVS lors du clic sur "Enregistrer tout") */
          delete pcfg[lbl];
          updatePinsList();
          updateBusVisuals();
          console.log('[deletePin] Composant complexe marqué pour suppression, enregistrer pour valider');
        };
        pl.appendChild(it);
      }
      return;
    }

    /* Vérifier si cette pin est utilisée par un composant complexe comme SIG (depuis pcfg) */
    /* Si oui, ne pas afficher le composant simple (le complexe a priorité) */
    if (caps && caps.pins) {
      const pin = caps.pins.find(p => p.label === lbl);
      if (pin && savedComplexSigGpios.has(parseInt(pin.gpio))) {
        /* Cette pin est utilisée par un composant complexe, ne pas afficher le simple */
        return;
      }
    }

    /* Afficher les pins simples (non complexes) */
    const it = document.createElement('div');
    it.className = `item ${pType(lbl)}`;
    it.innerHTML = `<span class="lbl">${lbl}</span><span class="role">${getRoleDisplayLabel(cfg.role)}</span><span class="stat">${stat(cfg, lbl)}</span><button class="del-btn">×</button>`;
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
      /* SIMPLIFICATION : Appliquer la config si elle existe */
      if (pcfg[lbl]) {
        applyCfg(pcfg[lbl]);
      }
    };
    const delBtn = it.querySelector('.del-btn');
    if (delBtn) delBtn.onclick = (e) => {
      e.stopPropagation();
      console.log('[deletePin] Marquage pour suppression composant simple sur pin:', lbl);
      /* Supprimer uniquement de pcfg (sauvegarde NVS lors du clic sur "Enregistrer tout") */
      delete pcfg[lbl];
      updatePinsList();
      updateBusVisuals();
      console.log('[deletePin] Composant simple marqué pour suppression, enregistrer pour valider');
    };
    pl.appendChild(it);
  });
}
