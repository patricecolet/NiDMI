/**
 * Gestion des pins configurées et sauvegarde globale.
 * Fonctions extraites de api.js et exposées globalement pour compatibilité.
 */

(function(global) {
  /**
   * Charge la configuration des pins depuis /api/pins/list
   * et met à jour pcfg puis l'UI.
   */
  async function loadConfiguredPins(){
    try {
      /* Charger les pins simples depuis /api/pins/list */
      const r = await fetch('/api/pins/list');
      if(!r.ok) {
        console.warn('[loadConfiguredPins] Erreur API:', r.status);
        return;
      }
      const d = await r.json();
      if(d && d.pins && Array.isArray(d.pins)) {
        if(typeof global.pcfg === 'undefined') {
          console.error('[loadConfiguredPins] pcfg non défini');
          return;
        }
        d.pins.forEach(pinData => {
          if(pinData && pinData.pinLabel && pinData.role) {
            pinData.role = typeof global.migrateRole === 'function' ? global.migrateRole(pinData.role) : pinData.role;
            if(pinData.pinLabel === 'SPI' || pinData.pinLabel === 'I2C') {
              console.log('[loadConfiguredPins] Config bus', pinData.pinLabel, ': role=' + pinData.role, 'csGpio=' + pinData.csGpio, 'range=' + pinData.range, 'dataRate=' + pinData.dataRate, 'filterIntensity=' + pinData.filterIntensity);
            }
            global.pcfg[pinData.pinLabel] = pinData;
          }
        });
      }

      /* /api/pins/list inclut déjà les composants complexes depuis MuxManager avec additionalPins */

      if (typeof global.updatePinsList === 'function') {
        global.updatePinsList();
      }
      if (typeof global.updateBusVisuals === 'function') {
        global.updateBusVisuals();
      }
    } catch(err) {
      console.log('Erreur chargement pins:', err);
    }
  }

  /**
   * Sauvegarde toutes les configurations de pins (simples et complexes).
   * Logique identique à l'implémentation originale de api.js.
   */
  async function saveAll(){
    const msg = $('#saveAllMsg');
    if(!msg) {
      console.error('[saveAll] Élément saveAllMsg non trouvé');
      return;
    }
    msg.textContent = 'Enregistrement...';
    try{
      if(typeof global.pcfg === 'undefined' || !global.pcfg) {
        msg.textContent = 'Erreur: configuration non disponible';
        msg.style.color = '#ef4444';
        return;
      }

      /* Relire la configuration de la pin actuellement sélectionnée depuis le formulaire */
      if(typeof global.cur !== 'undefined' && global.cur && typeof global.readCfg === 'function') {
        /* Passer le rôle depuis funcSelect pour éviter qu'il soit vide */
        const funcRole = $('#funcSelect')?.value || '';
        const currentCfg = global.readCfg(funcRole || null);
        if(currentCfg && currentCfg.role && typeof global.isBusRole === 'function' && !global.isBusRole(currentCfg.role)) {
          global.pcfg[global.cur] = currentCfg;
          console.log('[saveAll] pcfg[' + global.cur + '] mis à jour avec role:', currentCfg.role);
        } else if(!currentCfg || !currentCfg.role) {
          console.warn('[saveAll] readCfg role vide pour', global.cur, '- funcSelect:', funcRole, 'currentCfg:', currentCfg);
        }
      }

      /* Sauvegarder tous les composants séquentiellement (évite saturation NVS ESP32) */
      const pinLabels = Object.keys(global.pcfg);
      const validPins = pinLabels.filter(l => global.pcfg[l] && global.pcfg[l].role);
      let savedCount = 0;
      for (const lbl of pinLabels) {
        let c = global.pcfg[lbl];
        if(!c||!c.role) continue;
        savedCount++;
        msg.textContent = 'Enregistrement ' + savedCount + '/' + validPins.length + ' (' + lbl + ')...';
        const savePin = async () => {

          /* Pour la pin actuellement affichée, toujours reprendre le formulaire (évite valeurs périmées) */
          if(typeof global.cur !== 'undefined' && lbl === global.cur && typeof global.readCfg === 'function') {
            const freshRole = $('#funcSelect')?.value || '';
            const fresh = global.readCfg(freshRole || null);
            if(fresh && fresh.role && (typeof global.isBusRole !== 'function' || !global.isBusRole(fresh.role))) c = fresh;
          }

          const role = global.migrateRole ? global.migrateRole(c.role) : c.role;
          const def = typeof global.getComponentDefinition === 'function' ? global.getComponentDefinition(role) : null;

          /* Composants I2C (LIS3DH, MPR121) : envoyer en JSON direct */
          if(role === 'lis3dh' || role === 'mpr121') {
            console.log('[saveAll]', role, 'config:', JSON.stringify(c).substring(0, 200));
            const fullCfg = Object.assign({pinLabel: lbl, role: c.role}, c);
            if(lbl === 'SPI') fullCfg.busInterface = '1';
            else if(lbl === 'I2C') fullCfg.busInterface = '0';
            delete fullCfg.additionalPins;
            const jsonStr = JSON.stringify(fullCfg);
            console.log('[saveAll]', role, 'JSON complet (' + jsonStr.length + ' chars)');
            const resp = await fetch('/api/pins/set',{method:'POST',headers:{'Content-Type':'application/json'},body:jsonStr});
            if (resp.status === 413) {
              const d = await resp.json().catch(() => ({}));
              throw new Error(d.message || 'Config trop grande pour NVS (max 1900 octets). Réduisez les options.');
            }
            console.log('[saveAll]', role, 'réponse:', resp.status);
            return resp;
          }

          console.log('[saveAll] Traitement pin:', lbl, 'c:', c);
          console.log('[saveAll] c.additionalPins:', c.additionalPins);

          /* Détecter composant complexe depuis la définition (plus fiable que vérifier sig) */
          const hasAdditionalPins = def && def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0 
            && c.additionalPins && typeof c.additionalPins === 'object' && Object.keys(c.additionalPins).length > 0;

          console.log('[saveAll] hasAdditionalPins:', hasAdditionalPins, 'role:', role, 'def trouvée:', !!def, 'def.additionalPins count:', def ? (def.additionalPins ? def.additionalPins.length : 0) : 0, 'c.additionalPins keys:', c.additionalPins ? Object.keys(c.additionalPins) : []);

          /* Si composant simple, vérifier et supprimer les complexes sur cette pin (chercher dans pcfg) */
          /* NOTE: Les composants complexes sont détectés par leur définition, pas par la présence de sig */
          if(!hasAdditionalPins && global.caps && global.caps.pins) {
            const currentPin = global.caps.pins.find(p => p.label === lbl);
            if(currentPin) {
              const mainPinGpio = parseInt(currentPin.gpio);
              /* Chercher composant complexe dans pcfg par pinLabel (la pin principale du composant complexe) */
              const existingComplexLabel = Object.keys(global.pcfg).find(plbl => {
                const cfg = global.pcfg[plbl];
                if(!cfg || !cfg.role) return false;
                const cfgRole = global.migrateRole ? global.migrateRole(cfg.role) : cfg.role;
                const cfgDef = typeof global.getComponentDefinition === 'function' ? global.getComponentDefinition(cfgRole) : null;
                const cfgHasAdditionalPins = cfgDef && cfgDef.additionalPins && Array.isArray(cfgDef.additionalPins) && cfgDef.additionalPins.length > 0
                  && cfg.additionalPins && typeof cfg.additionalPins === 'object' && Object.keys(cfg.additionalPins).length > 0;
                if(!cfgHasAdditionalPins) return false;
                /* Vérifier si cette pin est utilisée comme pin principale du composant complexe */
                const complexPin = global.caps.pins.find(p => p.label === plbl);
                return complexPin && parseInt(complexPin.gpio) === mainPinGpio;
              });
              if(existingComplexLabel) {
                const existingComplex = global.pcfg[existingComplexLabel];
                console.log(`[saveAll] Suppression du composant complexe sur pin principale ${mainPinGpio} (remplacé par ${lbl})`);
                /* Supprimer via /api/pins/delete (unifié) */
                try {
                  const formData = new URLSearchParams();
                  formData.append('pin', existingComplexLabel);
                  await fetch('/api/pins/delete', {method: 'POST', body: formData});
                  console.log(`[saveAll] Composant complexe supprimé via /api/pins/delete`);
                } catch(e) {
                  console.error('[saveAll] Erreur suppression composant complexe:', e);
                }
              }
            }
          }

          const p = new URLSearchParams();
          p.set('pinLabel',lbl);
          p.set('role',c.role);
          /* Envoyer rtpMidiEnabled (ou rtpEnabled pour compatibilité) */
          if(c.rtpMidiEnabled) p.set('rtpMidiEnabled','true');
          else if(c.rtpEnabled) p.set('rtpEnabled','true'); /* Compatibilité ancien format */
          /* Envoyer midiMessageType (ou rtpType pour compatibilité) */
          if(c.midiMessageType) p.set('midiMessageType',c.midiMessageType);
          else if(c.rtpType) p.set('rtpType',c.rtpType); /* Compatibilité ancien format */

          /* Envoyer dynamiquement tous les paramètres MIDI (nouveaux noms midi* puis anciens rtp* pour compatibilité) */
          Object.keys(c).forEach(key => {
            /* Nouveaux noms (midi*) */
            /* Pour les paramètres RANGE (midiCcRangeMin/Max), toujours envoyer même si valeur par défaut */
            if(key.startsWith('midi') && c[key] !== undefined && c[key] !== null) {
              /* Accepter les chaînes vides et les valeurs "0" pour midiCcRangeMin/Max */
              if(c[key] !== '' || key.endsWith('Min') || key.endsWith('Max')) {
                p.set(key, c[key] || (key.endsWith('Min') ? '0' : key.endsWith('Max') ? '127' : ''));
              }
            }
            /* Paramètres MIDI par axe (X_/Y_/Z_: midiCc, midiChannel, etc.) - envoyer même "0" */
            else if((key.startsWith('X_') || key.startsWith('Y_') || key.startsWith('Z_')) && c[key] !== undefined && c[key] !== null) {
              if(c[key] !== '' || key.includes('midiCc') || key.includes('midiChannel')) p.set(key, c[key]);
            }
            /* Anciens noms (rtp*) sauf rtpEnabled et rtpType (déjà gérés ci-dessus) */
            else if(key.startsWith('rtp') && key !== 'rtpEnabled' && key !== 'rtpType' && key !== 'rtpMidiEnabled' && c[key] !== undefined && c[key] !== null && c[key] !== '') {
              p.set(key, c[key]); /* Compatibilité ancien format */
            }
          });

          /* Joystick : forcer envoi X_midiCc / Y_midiCc depuis le formulaire si absents de c */
          if(role === 'joystick') {
            const xCc = c.X_midiCc !== undefined && c.X_midiCc !== null ? c.X_midiCc : ($('#X_midiCc') && $('#X_midiCc').value !== undefined ? $('#X_midiCc').value : '7');
            const yCc = c.Y_midiCc !== undefined && c.Y_midiCc !== null ? c.Y_midiCc : ($('#Y_midiCc') && $('#Y_midiCc').value !== undefined ? $('#Y_midiCc').value : '7');
            p.set('X_midiCc', xCc);
            p.set('Y_midiCc', yCc);
          }

          /* Envoyer dynamiquement tous les formFields depuis la définition */
          if(def && def.formFields && Array.isArray(def.formFields)) {
            def.formFields.forEach(field => {
              if(field.id && !field.id.startsWith('_')) {
                const value = c[field.id];
                if(value !== undefined && value !== null && value !== '') {
                  if(field.type === 3) { /* CHECKBOX */
                    if(value === true || value === 'true') {
                      p.set(field.id, 'true');
                    }
                  } else if(field.type === 4) { /* RANGE */
                    if(c[field.id + 'Min'] !== undefined && c[field.id + 'Min'] !== null && c[field.id + 'Min'] !== '') {
                      p.set(field.id + 'Min', c[field.id + 'Min']);
                    }
                    if(c[field.id + 'Max'] !== undefined && c[field.id + 'Max'] !== null && c[field.id + 'Max'] !== '') {
                      p.set(field.id + 'Max', c[field.id + 'Max']);
                    }
                  } else {
                    p.set(field.id, value);
                  }
                }
              }
            });
          }

          /* Envoyer additionalPins si présent (générique basé sur def.additionalPins) */
          if(hasAdditionalPins && c.additionalPins && def && def.additionalPins && Array.isArray(def.additionalPins)) {
            console.log('[saveAll] Envoi additionalPins, c.additionalPins:', c.additionalPins);
            /* Envoyer dynamiquement tous les additionalPins depuis la définition */
            def.additionalPins.forEach(pinDef => {
              if(pinDef && pinDef.id) {
                const value = c.additionalPins[pinDef.id];
                console.log('[saveAll] Vérification pinDef.id:', pinDef.id, 'value:', value, 'optional:', pinDef.optional);
                
                if(value !== undefined && value !== null) {
                  /* Toujours envoyer les pins requises, même si valeur est 255 */
                  /* Pour les pins optionnelles, ne pas envoyer si valeur est 255 */
                  if(value !== 255 || !pinDef.optional) {
                    p.set(pinDef.id, value);
                    console.log('[saveAll] additionalPin envoyé:', pinDef.id, '=', value);
                  } else {
                    console.log('[saveAll] additionalPin ignoré (255 et optionnel):', pinDef.id);
                  }
                } else if(!pinDef.optional) {
                  /* Pin requise absente - utiliser la valeur par défaut ou 255 */
                  const defaultValue = (pinDef.defaultValue !== undefined) ? pinDef.defaultValue : 255;
                  p.set(pinDef.id, defaultValue);
                  console.log('[saveAll] additionalPin requise absente, utilisation defaultValue:', pinDef.id, '=', defaultValue);
                } else {
                  console.warn('[saveAll] ERREUR: Pin requise absente:', pinDef.id, 'value:', value, 'defaultValue:', pinDef.defaultValue);
                }
              }
            });
            /* Note: complexId supprimé - plus besoin d'envoyer un ID explicite */
          } else if(def && def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0) {
            /* Seulement avertir si la définition indique qu'il devrait y avoir additionalPins mais qu'elles manquent */
            console.warn('[saveAll] ERREUR: Composant devrait avoir additionalPins mais elles sont absentes. def:', def.id, 'def.additionalPins:', def.additionalPins, 'c.additionalPins:', c.additionalPins);
          }
          /* Sinon, c'est normal - composant simple sans additionalPins */

          /* Champs OSC et Debug (communs à tous) */
          if(c.oscEnabled) p.set('oscEnabled','true');
          if(c.oscAddress) p.set('oscAddress',c.oscAddress);
          if(c.oscFormat) p.set('oscFormat',c.oscFormat);
          if(c.dbgEnabled) p.set('dbgEnabled','true');
          if(c.dbgHeader) p.set('dbgHeader',c.dbgHeader);
          if(lbl === 'SPI' || lbl === 'I2C') {
            console.log('[saveAll] POST body pour', lbl, ':', p.toString());
          }

          const r = await fetch('/api/pins/set',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:p.toString()});
          if (r.status === 413) { const d = await r.json().catch(() => ({})); throw new Error(d.message || 'Config trop grande pour NVS (max 1900 octets).'); }
          return r;
        };
        await savePin();
        await new Promise(r => setTimeout(r, 80));
      }

      /* Attendre que le backend traite le rechargement (ESP32-C3 mono-cœur) */
      await new Promise(r => setTimeout(r, 300));

      const listRes = await fetch('/api/pins/list');
      if(!listRes.ok){
        throw new Error('Erreur lors de la récupération de la liste des pins: '+listRes.status);
      }
      const text = await listRes.text();
      let listData;
      if(!text || text.trim().length===0){
        console.warn('Réponse vide de /api/pins/list');
        listData = {pins:[]};
      }else{
        try{
          listData = JSON.parse(text);
        }catch(e){
          console.error('Erreur parsing JSON /api/pins/list:',e,'Réponse:',text);
          throw e;
        }
      }
      const serverPins = new Set();
      if(listData.pins && Array.isArray(listData.pins)){
        listData.pins.forEach(p=>{
          if(p.pinLabel) serverPins.add(p.pinLabel);
          if(p.pinLabel === 'SPI' || p.pinLabel === 'I2C') {
            console.log('[saveAll] Vérification post-save', p.pinLabel, ': csGpio=' + p.csGpio, 'range=' + p.range, 'dataRate=' + p.dataRate, 'filterIntensity=' + p.filterIntensity);
          }
        });
      }
      /* Vérifier que les pins bus ont bien été sauvegardées */
      if(typeof global.pcfg !== 'undefined') {
        ['SPI','I2C'].forEach(bus => {
          if(global.pcfg[bus] && global.pcfg[bus].role && !serverPins.has(bus)) {
            console.error('[saveAll] ERREUR: pin', bus, 'configurée localement mais ABSENTE de la réponse backend !');
          }
        });
      }

      const localPins = new Set(Object.keys(global.pcfg));
      const toDelete = Array.from(serverPins).filter(p=>!localPins.has(p));

      for (const pinLabel of toDelete) {
        const p = new URLSearchParams();
        p.set('pin',pinLabel);
        await fetch('/api/pins/delete',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:p.toString()});
        await new Promise(r => setTimeout(r, 80));
      }
      if (toDelete.length > 0) await new Promise(r => setTimeout(r, 200));
      /* Rafraîchir pcfg et la liste des pins depuis le serveur (évite rechargement manuel) */
      await loadConfiguredPins();

      /* Sauvegarder les interfaces MIDI globales */
      try {
        /* Sauvegarder RTP-MIDI */
        const rtpMidiElement = $('#rtpMidiEnabled');
        if (rtpMidiElement && rtpMidiElement.type === 'checkbox' && typeof rtpMidiElement.checked !== 'undefined') {
          const rtpFormData = new URLSearchParams();
          rtpFormData.append('enable', rtpMidiElement.checked ? 'true' : 'false');
          const bodyString = rtpFormData.toString();
          if (bodyString && bodyString.includes('enable=')) {
            try {
              const rtpResponse = await fetch('/api/rtp/enable', {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: bodyString});
              if (!rtpResponse.ok) {
                const errorText = await rtpResponse.text();
                console.warn('[saveAll] Erreur RTP-MIDI:', rtpResponse.status, errorText);
              }
            } catch(e) {
              console.warn('[saveAll] Erreur lors de la sauvegarde RTP-MIDI:', e);
            }
          }
        }
        /* Note: USB MIDI s'active automatiquement au boot si supporté, pas de contrôle via interface */
      } catch(e) {
        console.error('Erreur sauvegarde interfaces MIDI:', e);
      }

      /* Rafraîchir le cache des GPIOs utilisés depuis le backend */
      if (typeof global.loadUsedGpiosFromBackend === 'function') {
        await global.loadUsedGpiosFromBackend();
      }
      if (typeof global.updateBusVisuals === 'function') {
        global.updateBusVisuals();
      }

      msg.textContent = 'Toutes les configurations enregistrées';
      msg.style.color = '#10b981';
    }catch(e){
      msg.textContent = e && e.message ? e.message : 'Erreur lors de l\'enregistrement';
      msg.style.color = '#ef4444';
      console.error('Erreur saveAll:',e);
    }
  }

  // Exposer globalement pour compatibilité et via un namespace.
  global.loadConfiguredPins = loadConfiguredPins;
  global.saveAll = saveAll;

  global.ApiPins = {
    loadConfiguredPins,
    saveAll
  };
})(typeof window !== 'undefined' ? window : this);

