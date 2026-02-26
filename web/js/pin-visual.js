/**
 * @file pin-visual.js
 * @brief Gestion visuelle des pins (SVG board + grisage)
 */

/**
 * Met à jour le grisage visuel des pins utilisées
 */
function updateBusVisuals() {
  /* 1. Enlever tous les grisages */
  if (typeof prect === 'undefined' || !prect) return;
  Object.keys(prect).forEach(lbl => {
    const r = prect[lbl];
    if (!r || typeof r.classList === 'undefined') return;
    r.classList.remove('busDisabled');
  });

  if (typeof caps === 'undefined' || !caps || !caps.pins || !Array.isArray(caps.pins)) return;

  /* 2. Calculer les GPIOs utilisés uniquement depuis pcfg (état actuel de l'interface) */
  /* Cela permet de refléter immédiatement les suppressions même avant enregistrement */
  /* Les GPIOs sont calculés depuis pcfg qui contient tous les composants configurés */
  let usedGpios = new Set();

  if (typeof pcfg === 'undefined' || !pcfg) return;
  
  /* 3. Parcourir pcfg pour calculer tous les GPIOs utilisés */
  Object.keys(pcfg).forEach(lbl => {
    const cfg = pcfg[lbl];
    if (!cfg) return;

    /* Gérer les entrées de bus (I2C, SPI) qui n'ont pas de pin dans caps.pins */
    const bus = caps.bus || {};
    if (lbl === 'I2C' && bus.i2c) {
      if (bus.i2c.sda !== undefined) usedGpios.add(Number(bus.i2c.sda));
      if (bus.i2c.scl !== undefined) usedGpios.add(Number(bus.i2c.scl));
      return;
    }
    if (lbl === 'SPI' && bus.spi) {
      if (bus.spi.mosi !== undefined) usedGpios.add(Number(bus.spi.mosi));
      if (bus.spi.miso !== undefined) usedGpios.add(Number(bus.spi.miso));
      if (bus.spi.sck !== undefined) usedGpios.add(Number(bus.spi.sck));
      return;
    }

    const pin = caps.pins.find(p => p && p.label === lbl);
    if (!pin || pin.gpio === undefined) return;

    const gpio = parseInt(pin.gpio);
    if (!isNaN(gpio)) usedGpios.add(gpio);

    /* Pour les composants complexes, lire les additionalPins depuis pcfg ET depuis le formulaire */
    if (cfg && cfg.role && typeof getComponentDefinition === 'function') {
      const migratedRole = typeof migrateRole === 'function' ? migrateRole(cfg.role) : cfg.role;
      const def = getComponentDefinition(migratedRole);

      if (def && def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0) {
        /* D'abord, lire depuis pcfg (pour les composants sauvegardés) */
        if (cfg.additionalPins && typeof cfg.additionalPins === 'object') {
          def.additionalPins.forEach(pinDef => {
            const pinId = pinDef.id;
            /* Lire depuis pcfg.additionalPins */
            if (cfg.additionalPins[pinId] !== undefined && cfg.additionalPins[pinId] !== null) {
              const val = parseInt(cfg.additionalPins[pinId]);
              if (!isNaN(val) && val !== 255) {
                usedGpios.add(val);
              }
            }
          });
        }
        
        /* Ensuite, lire depuis le formulaire (pour les composants en cours d'édition) */
        /* Cela permet de surcharger avec les valeurs du formulaire si elles existent */
        def.additionalPins.forEach(pinDef => {
          const prefix = def.id ? def.id : 'comp';
          const formId = prefix + pinDef.id.charAt(0).toUpperCase() + pinDef.id.slice(1);
          const formEl = $('#' + formId);
          if (formEl && formEl.value && formEl.value !== '255') {
            const val = parseInt(formEl.value);
            if (!isNaN(val) && val !== 255) {
              usedGpios.add(val);
            }
          }
        });
      }
    }
  });

  /* 4. Créer un map GPIO -> labels pour le grisage */
  const gpioMap = new Map();
  caps.pins.forEach(p => {
    const gpio = parseInt(p.gpio);
    if (isNaN(gpio)) return;
    if (!gpioMap.has(gpio)) gpioMap.set(gpio, []);
    gpioMap.get(gpio).push(p);
  });

  /* 5. Griser toutes les pins dont le GPIO est utilisé */
  usedGpios.forEach(gpio => {
    const pinsForGpio = gpioMap.get(gpio) || [];
    pinsForGpio.forEach(pin => {
      if (pin && pin.label && prect[pin.label]) {
        prect[pin.label].classList.add('busDisabled');
      }
    });
  });
}

/**
 * Dessine le board SVG avec toutes les pins
 */
function drawBoard() {
  const L = $('#pinsLeft'), R = $('#pinsRight');
  if (!L || !R || typeof caps === 'undefined' || !caps || !caps.pins || !Array.isArray(caps.pins)) return;
  L.innerHTML = '';
  R.innerHTML = '';
  const RH = 28;
  const H = 20;
  const isS3 = caps.board && caps.board.toLowerCase().includes('s3');
  const W = isS3 ? 32 : 44;
  const COL = isS3 ? { c1: 20, c2: 54, c3: 88, c4: 238, c5: 272, c6: 306 } : { c1: 20, c2: 68, c4: 238, c5: 286 };

  const mk = (x, y, w, h, fill, stroke, label, clk = true) => {
    const g = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    const r = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
    r.setAttribute('x', x);
    r.setAttribute('y', y);
    r.setAttribute('width', w);
    r.setAttribute('height', h);
    r.setAttribute('rx', '4');
    r.setAttribute('fill', fill);
    r.setAttribute('stroke', stroke);
    g.appendChild(r);
    if (label) {
      const t = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      t.setAttribute('x', x + w / 2);
      t.setAttribute('y', y + h / 2 + 1);
      t.setAttribute('text-anchor', 'middle');
      t.setAttribute('class', 'svg-t');
      t.textContent = label;
      g.appendChild(t);
    }
    if (clk && label) {
      g.style.cursor = 'pointer';
      r.dataset.label = label;
      prect[label] = r;
      r.addEventListener('click', () => {
        /* RESTRICTIONS DÉSACTIVÉES - Permettre le clic même sur les pins grisées */
        /* Le grisage est conservé pour l'information visuelle uniquement */
        /*
        Ne pas permettre le clic si la pin est grisée (déjà configurée)
        if (r.classList.contains('busDisabled')) {
          return;
        }
        */
        if (window._selRect) window._selRect.classList.remove('selectedSquare');
        window._selRect = r;
        r.classList.add('selectedSquare');

        /* Remapper les pins de bus vers leur clé pcfg */
        let effectiveLabel = label;
        if (['SDA','SCL'].includes(label)) effectiveLabel = 'I2C';
        else if (['MOSI','MISO','SCK'].includes(label)) effectiveLabel = 'SPI';

        cur = effectiveLabel;
        $('#selPin').textContent = effectiveLabel;
        handlePinClick(label);
        updFunc(effectiveLabel);
        if (pcfg[cur]) {
          applyCfg(pcfg[cur]);
        }
      });
    }
    return g;
  };

  const getPinColor = (label) => {
    /* Utiliser pType qui utilise les données du backend */
    const type = pType(label);
    switch (type) {
      case 'uart': return FC.UART;
      case 'analog': return FC.ANALOG;
      case 'i2c': return FC.I2C;
      case 'spi': return FC.SPI;
      default: return FC.DIGITAL;
    }
  };

  const pins = caps.pins;
  const gpioMap = new Map();
  pins.forEach(p => {
    if (!gpioMap.has(p.gpio)) gpioMap.set(p.gpio, []);
    gpioMap.get(p.gpio).push(p);
  });

  const getAlias = (gpio, prefix) => {
    const ps = gpioMap.get(gpio) || [];
    return ps.find(p => p.label.startsWith(prefix))?.label || '';
  };

  const getBus = (gpio) => {
    /* Utiliser caps.bus pour identifier les pins de bus */
    const bus = caps.bus || {};
    if (bus.i2c && gpio === bus.i2c.sda) return 'SDA';
    if (bus.i2c && gpio === bus.i2c.scl) return 'SCL';
    if (bus.uart && gpio === bus.uart.tx) return 'TX';
    if (bus.uart && gpio === bus.uart.rx) return 'RX';
    if (bus.spi && gpio === bus.spi.mosi) return 'MOSI';
    if (bus.spi && gpio === bus.spi.miso) return 'MISO';
    if (bus.spi && gpio === bus.spi.sck) return 'SCK';
    return '';
  };

  if (isS3) {
    const left3 = (row, busLbl, adcLbl, dLbl) => {
      const y = 30 + row * RH;
      const f = document.createDocumentFragment();
      if (busLbl) {
        f.appendChild(mk(COL.c1, y - 10, W, H, getPinColor(busLbl), '#9ca3af', busLbl));
      } else {
        f.appendChild(mk(COL.c1, y - 10, W, H, '#9ca3af', '#9ca3af', '', false));
      }
      if (adcLbl) {
        f.appendChild(mk(COL.c2, y - 10, W, H, FC.ANALOG, '#9ca3af', adcLbl));
      } else {
        f.appendChild(mk(COL.c2, y - 10, W, H, '#9ca3af', '#9ca3af', '', false));
      }
      f.appendChild(mk(COL.c3, y - 10, W, H, FC.DIGITAL, '#9ca3af', dLbl));
      L.appendChild(f);
    };

    const right3 = (row, dLbl, adcLbl, busLbl) => {
      const y = 30 + row * RH;
      const f = document.createDocumentFragment();
      f.appendChild(mk(COL.c4, y - 10, W, H, FC.DIGITAL, '#9ca3af', dLbl));
      if (adcLbl) {
        f.appendChild(mk(COL.c5, y - 10, W, H, FC.ANALOG, '#9ca3af', adcLbl));
      } else {
        f.appendChild(mk(COL.c5, y - 10, W, H, '#9ca3af', '#9ca3af', '', false));
      }
      if (busLbl) {
        f.appendChild(mk(COL.c6, y - 10, W, H, getPinColor(busLbl), '#9ca3af', busLbl));
      } else {
        f.appendChild(mk(COL.c6, y - 10, W, H, '#9ca3af', '#9ca3af', '', false));
      }
      R.appendChild(f);
    };

    /* Filtrer les pins D en évitant les doublons par GPIO (un GPIO = une seule pin D) */
    /* Prendre la première pin avec label "D" pour chaque GPIO */
    const dPinsMap = new Map();
    pins.filter(p => p.label.startsWith('D')).forEach(p => {
      if (!dPinsMap.has(p.gpio)) dPinsMap.set(p.gpio, p);
    });
    const dPins = Array.from(dPinsMap.values()).sort((a, b) => {
      const na = parseInt(a.label.substring(1));
      const nb = parseInt(b.label.substring(1));
      return na - nb;
    });
    const displayed = new Set();
    let leftRow = 0, rightRow = 0;

    dPins.filter(p => {
      const n = parseInt(p.label.substring(1));
      return n <= 6;
    }).forEach(p => {
      if (displayed.has(p.gpio)) return;
      displayed.add(p.gpio);
      const busLbl = getBus(p.gpio);
      const adcLbl = getAlias(p.gpio, 'A');
      left3(leftRow++, busLbl, adcLbl, p.label);
    });

    R.appendChild(mk(COL.c4, 30 + rightRow * RH - 10, W, H, FC.POWER, '#9ca3af', '5V', false));
    rightRow++;
    R.appendChild(mk(COL.c4, 30 + rightRow * RH - 10, W, H, FC.GND, '#9ca3af', 'GND', false));
    rightRow++;
    R.appendChild(mk(COL.c4, 30 + rightRow * RH - 10, W, H, FC.POWER, '#9ca3af', '3V3', false));
    rightRow++;

    dPins.filter(p => {
      const n = parseInt(p.label.substring(1));
      return n >= 7;
    }).sort((a, b) => {
      const na = parseInt(a.label.substring(1));
      const nb = parseInt(b.label.substring(1));
      return nb - na;
    }).forEach(p => {
      if (displayed.has(p.gpio)) return;
      displayed.add(p.gpio);
      const adcLbl = getAlias(p.gpio, 'A');
      const busLbl = getBus(p.gpio);
      right3(rightRow++, p.label, adcLbl, busLbl);
    });
  } else {
    const left = (row, tl, tc, dl) => {
      const y = 30 + row * RH;
      const f = document.createDocumentFragment();
      if (tl) {
        f.appendChild(mk(COL.c1, y - 10, W, H, tc, '#9ca3af', tl));
      } else {
        f.appendChild(mk(COL.c1, y - 10, W, H, '#9ca3af', '#9ca3af', '', false));
      }
      f.appendChild(mk(COL.c2, y - 10, W, H, FC.DIGITAL, '#9ca3af', dl));
      L.appendChild(f);
    };

    /* Utiliser caps.bus pour identifier les pins de bus (données du backend) */
    const bus = caps.bus || {};
    const busGpios = new Set();
    /* Convertir en nombre pour éviter les problèmes de type (string vs number) */
    if (bus.i2c) {
      busGpios.add(Number(bus.i2c.sda));
      busGpios.add(Number(bus.i2c.scl));
    }
    if (bus.spi) {
      busGpios.add(Number(bus.spi.mosi));
      busGpios.add(Number(bus.spi.miso));
      busGpios.add(Number(bus.spi.sck));
    }
    if (bus.uart) {
      busGpios.add(Number(bus.uart.tx));
      busGpios.add(Number(bus.uart.rx));
    }

    const analogPins = pins.filter(p => p.label.startsWith('A') && p.caps?.adc).sort((a, b) => a.label.localeCompare(b.label));
    const i2cPins = pins.filter(p => bus.i2c && (p.gpio === bus.i2c.sda || p.gpio === bus.i2c.scl)).sort((a, b) => a.gpio === bus.i2c?.sda ? -1 : 1);
    const uartPins = pins.filter(p => bus.uart && (p.gpio === bus.uart.tx || p.gpio === bus.uart.rx)).sort((a, b) => a.gpio === bus.uart?.tx ? -1 : 1);
    /* Filtrer les pins D en évitant les doublons par label ET par GPIO */
    /* Sur C3, getAllMappings() peut retourner des doublons (même label "D4" plusieurs fois) */
    /* Utiliser une Map avec label comme clé unique pour garantir un seul exemplaire par label */
    const digitalPinsMap = new Map();
    const seenGpioForD = new Set();
    /* Convertir p.gpio en nombre pour la comparaison */
    pins.filter(p => p.label.startsWith('D') && !busGpios.has(Number(p.gpio))).forEach(p => {
      /* Utiliser le label comme clé unique pour éviter les doublons de label */
      /* Si plusieurs pins ont le même label "D4", on garde seulement la première */
      /* ET éviter aussi les doublons par GPIO (un GPIO = une seule pin D affichée) */
      if (!digitalPinsMap.has(p.label) && !seenGpioForD.has(p.gpio)) {
        digitalPinsMap.set(p.label, p);
        seenGpioForD.add(p.gpio);
      }
    });
    const digitalPins = Array.from(digitalPinsMap.values()).sort((a, b) => {
      const na = parseInt(a.label.substring(1));
      const nb = parseInt(b.label.substring(1));
      return na - nb;
    });

    const leftPins = [];
    const displayedGpios = new Set();
    let leftRow = 0, rightRow = 0;

    analogPins.forEach(p => {
      leftPins.push({ gpio: p.gpio, label: p.label, color: getPinColor(p.label), dLabel: getAlias(p.gpio, 'D') });
      displayedGpios.add(Number(p.gpio));
    });

    digitalPins.forEach(p => {
      if (!displayedGpios.has(Number(p.gpio))) {
        leftPins.push({ gpio: p.gpio, label: '', color: FC.DIGITAL, dLabel: p.label });
        displayedGpios.add(Number(p.gpio));
      }
    });

    i2cPins.forEach(p => {
      /* Éviter les doublons : vérifier que le GPIO n'a pas déjà été affiché */
      if (!displayedGpios.has(Number(p.gpio))) {
        const busLabel = getBus(p.gpio); /* Utiliser getBus() pour obtenir "SDA" ou "SCL" au lieu de p.label */
        leftPins.push({ gpio: p.gpio, label: busLabel || p.label, color: getPinColor(busLabel || p.label), dLabel: getAlias(p.gpio, 'D') });
        displayedGpios.add(Number(p.gpio));
      }
    });

    /* UART TX depuis caps.bus */
    const uartTx = bus.uart ? uartPins.find(p => p.gpio === bus.uart.tx) : null;
    if (uartTx && !displayedGpios.has(Number(uartTx.gpio))) {
      const busLabel = getBus(uartTx.gpio); /* Utiliser getBus() pour obtenir "TX" au lieu de uartTx.label */
      leftPins.push({ gpio: uartTx.gpio, label: busLabel || uartTx.label, color: getPinColor(busLabel || uartTx.label), dLabel: getAlias(uartTx.gpio, 'D') });
      displayedGpios.add(Number(uartTx.gpio));
    }

    leftPins.sort((a, b) => a.gpio - b.gpio);
    leftPins.forEach(p => {
      left(leftRow++, p.label, p.color, p.dLabel);
    });

    R.appendChild(mk(COL.c4, 30 + rightRow * RH - 10, W, H, FC.POWER, '#9ca3af', '5V', false));
    rightRow++;
    R.appendChild(mk(COL.c4, 30 + rightRow * RH - 10, W, H, FC.GND, '#9ca3af', 'GND', false));
    rightRow++;
    R.appendChild(mk(COL.c4, 30 + rightRow * RH - 10, W, H, FC.POWER, '#9ca3af', '3V3', false));
    rightRow++;

    const right = (row, dl, tl, tc) => {
      const y = 30 + row * RH;
      const f = document.createDocumentFragment();
      f.appendChild(mk(COL.c4, y - 10, W, H, FC.DIGITAL, '#9ca3af', dl));
      f.appendChild(mk(COL.c5, y - 10, W, H, tc, '#9ca3af', tl));
      return f;
    };

    /* Pins SPI depuis caps.bus */
    const spiPins = pins.filter(p => bus.spi && (p.gpio === bus.spi.mosi || p.gpio === bus.spi.miso || p.gpio === bus.spi.sck)).sort((a, b) => {
      const order = [bus.spi?.mosi, bus.spi?.miso, bus.spi?.sck];
      return order.indexOf(a.gpio) - order.indexOf(b.gpio);
    });
    spiPins.forEach(p => {
      /* Éviter les doublons : vérifier que le GPIO n'a pas déjà été affiché */
      if (!displayedGpios.has(Number(p.gpio))) {
        const busLabel = getBus(p.gpio); /* Utiliser getBus() pour obtenir "MOSI", "MISO", ou "SCK" au lieu de p.label */
        R.appendChild(right(rightRow++, getAlias(p.gpio, 'D'), busLabel || p.label, getPinColor(busLabel || p.label)));
        displayedGpios.add(Number(p.gpio));
      }
    });

    /* UART RX depuis caps.bus */
    const uartRx = bus.uart ? pins.find(p => p.gpio === bus.uart.rx) : null;
    if (uartRx && !displayedGpios.has(Number(uartRx.gpio))) {
      const busLabel = getBus(uartRx.gpio); /* Utiliser getBus() pour obtenir "RX" au lieu de uartRx.label */
      R.appendChild(right(rightRow++, getAlias(uartRx.gpio, 'D'), busLabel || uartRx.label, getPinColor(busLabel || uartRx.label)));
      displayedGpios.add(Number(uartRx.gpio));
    }
  }

  const boardNameEl = $('#boardName');
  if (boardNameEl && caps.board) {
    const boardUpper = caps.board.toUpperCase().replace('-', '-');
    boardNameEl.textContent = boardUpper;
  }

  updateBusVisuals();
}
