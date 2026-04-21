// --- Monitoring SVG : select unique off | raw | midi (runtime-only, non NVS) ---
const telemetryByLabel = {};
const ledDecayTimersByLabel = {};
let telemetryMode = 'off';

function getTelemetryModeFromUI() {
  const el = document.getElementById('telemetryMode');
  if (!el) return telemetryMode;
  telemetryMode = el.value;
  return telemetryMode;
}

function isTelemetryMonitoringOn() {
  const m = getTelemetryModeFromUI();
  return m === 'raw' || m === 'midi';
}

function updateValueTextForLabel(label) {
  if (typeof valueTextByLabel === 'undefined' || !valueTextByLabel) { console.warn('[DBG VAL] valueTextByLabel undefined'); return; }
  const payload = telemetryByLabel[label];
  if (!payload) { console.warn('[DBG VAL] pas de payload pour', label); return; }

  const tVal = valueTextByLabel[label];
  if (!tVal) { console.warn('[DBG VAL] pas de tVal pour label="' + label + '"'); return; }

  const mode = getTelemetryModeFromUI();
  if (mode === 'off') {
    tVal.style.visibility = 'hidden';
    return;
  }

  if (!payload.show_value) {
    console.log('[DBG VAL] show_value=false pour', label);
    tVal.style.visibility = 'hidden';
    return;
  }

  const v = (mode === 'midi') ? payload.midi : payload.raw;
  tVal.textContent = String(v);
  tVal.style.visibility = 'visible';
  console.log('[DBG VAL] affiché label="' + label + '" mode=' + mode + ' v=' + v);
}

function updateAllValuesForMode() {
  Object.keys(telemetryByLabel).forEach(updateValueTextForLabel);
}

function clearTelemetryVisuals() {
  Object.keys(telemetryByLabel).forEach(k => delete telemetryByLabel[k]);
  if (typeof valueTextByLabel !== 'undefined' && valueTextByLabel) {
    Object.keys(valueTextByLabel).forEach(lbl => {
      const el = valueTextByLabel[lbl];
      if (!el) return;
      el.textContent = '';
      el.style.visibility = 'hidden';
    });
  }
  if (typeof ledByLabel !== 'undefined' && ledByLabel) {
    Object.keys(ledByLabel).forEach(lbl => {
      if (ledDecayTimersByLabel[lbl]) {
        clearTimeout(ledDecayTimersByLabel[lbl]);
        delete ledDecayTimersByLabel[lbl];
      }
      const led = ledByLabel[lbl];
      if (led) led.style.visibility = 'hidden';
    });
  }
}

function applyPinReplacementLogic(pin){
 if(typeof pin !== 'string' || !pin) return;
 if(typeof pcfg === 'undefined' || !pcfg) return;
 
 if(pin.startsWith('A')){
  const dLabel=pin.replace('A','D');
  if(pcfg[dLabel]) delete pcfg[dLabel];
 } else if(['D0','D1','D2','D3'].includes(pin)){
  const aLabel=pin.replace('D','A');
  if(pcfg[aLabel]) delete pcfg[aLabel];
 } else if(['SDA','SCL'].includes(pin)){
  if(pcfg['D4']) delete pcfg['D4'];
  if(pcfg['D5']) delete pcfg['D5'];
  if(pcfg['SDA']) delete pcfg['SDA'];
  if(pcfg['SCL']) delete pcfg['SCL'];
 } else if(['MOSI','MISO','SCK'].includes(pin)){
  if(pcfg['D8']) delete pcfg['D8'];
  if(pcfg['D9']) delete pcfg['D9'];
  if(pcfg['D10']) delete pcfg['D10'];
  if(pcfg['MOSI']) delete pcfg['MOSI'];
  if(pcfg['MISO']) delete pcfg['MISO'];
  if(pcfg['SCK']) delete pcfg['SCK'];
 } else if(pin==='TX'){
  if(pcfg['D6']) delete pcfg['D6'];
 } else if(pin==='RX'){
  if(pcfg['D7']) delete pcfg['D7'];
 } else if(['D4','D5'].includes(pin)){
  if(pcfg['I2C']) delete pcfg['I2C'];
 } else if(['D8','D9','D10'].includes(pin)){
  if(pcfg['SPI']) delete pcfg['SPI'];
 } else if(pin==='D6'){
  if(pcfg['TX']) delete pcfg['TX'];
 } else if(pin==='D7'){
  if(pcfg['RX']) delete pcfg['RX'];
 }
}

function handlePinClickLocal(label){
 if(typeof label !== 'string' || !label) return;
 if(typeof pcfg === 'undefined' || !pcfg) return;
 
 if(label.startsWith('A') || label.startsWith('M')){
  if(!pcfg[label] || !pcfg[label].role) pcfg[label] = {role: 'potentiometer'};
  if(label.startsWith('A')){
   const dLabel = label.replace('A', 'D');
   if(pcfg[dLabel]) delete pcfg[dLabel];
  }
 } else if(['D0','D1','D2','D3'].includes(label)){
  if(!pcfg[label] || !pcfg[label].role) pcfg[label] = {role: 'button'};
  const aLabel = label.replace('D', 'A');
  if(pcfg[aLabel]) delete pcfg[aLabel];
 } else if(['SDA','SCL'].includes(label)){
  if(!pcfg['I2C'] || !pcfg['I2C'].role) pcfg['I2C'] = {role: 'i2c'};
  if(pcfg['D4']) delete pcfg['D4'];
  if(pcfg['D5']) delete pcfg['D5'];
  if(pcfg['SDA']) delete pcfg['SDA'];
  if(pcfg['SCL']) delete pcfg['SCL'];
  label = 'I2C';
 } else if(['MOSI','MISO','SCK'].includes(label)){
  if(!pcfg['SPI'] || !pcfg['SPI'].role) pcfg['SPI'] = {role: 'spi'};
  if(pcfg['D8']) delete pcfg['D8'];
  if(pcfg['D9']) delete pcfg['D9'];
  if(pcfg['D10']) delete pcfg['D10'];
  if(pcfg['MOSI']) delete pcfg['MOSI'];
  if(pcfg['MISO']) delete pcfg['MISO'];
  if(pcfg['SCK']) delete pcfg['SCK'];
  label = 'SPI';
 } else if(label === 'TX' || label === 'RX'){
  if(!pcfg[label] || !pcfg[label].role) pcfg[label] = {role: 'uart'};
  if(label === 'TX' && pcfg['D6']) delete pcfg['D6'];
  if(label === 'RX' && pcfg['D7']) delete pcfg['D7'];
 } else if(['D4','D5'].includes(label)){
  if(pcfg['I2C']) delete pcfg['I2C'];
  if(!pcfg[label] || !pcfg[label].role) pcfg[label] = {role: 'button'};
 } else if(['D8','D9','D10'].includes(label)){
  if(pcfg['SPI']) delete pcfg['SPI'];
  if(!pcfg[label] || !pcfg[label].role) pcfg[label] = {role: 'button'};
 } else if(['D6','D7'].includes(label)){
  if(!pcfg[label] || !pcfg[label].role) pcfg[label] = {role: 'button'};
  if(label === 'D6' && pcfg['TX']) delete pcfg['TX'];
  if(label === 'D7' && pcfg['RX']) delete pcfg['RX'];
 } else if(/^D\d+$/.test(label)){
  if(!pcfg[label] || !pcfg[label].role) pcfg[label] = {role: 'button'};
 }
 
 if(typeof updatePinsList === 'function') updatePinsList();
 if(typeof updateBusVisuals === 'function') updateBusVisuals();
}

function handlePinClick(label){
 if(typeof label !== 'string' || !label) return;
 if(typeof prect === 'undefined' || !prect) return;
 /* RESTRICTIONS DÉSACTIVÉES - Permettre le clic même sur les pins grisées */
 /*
 if(prect[label] && prect[label].classList && prect[label].classList.contains('busDisabled')){
  return;
 }
 */
 
 if(typeof websocket !== 'undefined' && websocket && websocket.readyState === WebSocket.OPEN){
  websocket.send(`PIN_CLICKED:${label}`);
 } else {
  handlePinClickLocal(label);
 }
}

function initWebSocket(){
 if(typeof window === 'undefined' || !window.location) return;
 if(typeof WebSocket === 'undefined') {
  console.warn('WebSocket non supporté');
  return;
 }
 const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
 const wsUrl = `${protocol}//${window.location.host}/ws`;
 try {
  websocket = new WebSocket(wsUrl);
  websocket.onopen = function(){
   console.log('WebSocket connected');
   // OFF par défaut à chaque connexion, côté serveur aussi
   websocket.send('PIN_MONITORING:0');
   const wdc = document.getElementById('webDebugConsoleEnabled');
   if (wdc && wdc.checked) {
    websocket.send('DEBUG_CONSOLE:1');
   } else {
    websocket.send('DEBUG_CONSOLE:0');
   }
  };
  websocket.onmessage = function(event){
  if(!event || !event.data) return;
  const message = String(event.data);

  if (message.startsWith('DEBUG_LOG:')) {
   const pre = document.getElementById('webDebugConsoleOut');
   if (pre) {
    const line = message.substring(10);
    pre.textContent += line + '\n';
    const lines = pre.textContent.split('\n');
    if (lines.length > 500) {
     pre.textContent = lines.slice(-500).join('\n');
    }
    pre.scrollTop = pre.scrollHeight;
   }
   return;
  }

  // Accusé de réception de calibration touch (globale)
  if (message === 'TOUCH_CALIBRATE_DONE') {
   const msgEl = $('#touchCalibrateMsg');
   if (msgEl) {
    msgEl.textContent = 'Calibration touch terminée.';
    msgEl.style.color = '#10b981';
   }
   return;
  }

  if (message.startsWith('PIN_MONITORING_STATE:')) {
    const modeEl = document.getElementById('telemetryMode');
    if (message.endsWith(':0')) {
      if (modeEl) modeEl.value = 'off';
      telemetryMode = 'off';
      clearTelemetryVisuals();
    }
    return;
  }

  // Monitoring SVG : télémétrie temps réel
  if (message.startsWith('PIN_TELEMETRY:')) {
    const _monOn = isTelemetryMonitoringOn();
    console.log('[DBG TELEM] reçu: ' + message.substring(0, 120) + ' | on:' + _monOn + ' | mode:' + telemetryMode);
    if (!_monOn) return;
    const prefix = 'PIN_TELEMETRY:';
    const rest = message.substring(prefix.length);
    const sep = rest.indexOf(':');
    if (sep === -1) { console.warn('[DBG TELEM] séparateur absent'); return; }

    const pinLabel = rest.substring(0, sep);
    const jsonStr = rest.substring(sep + 1);
    const _hasVal = typeof valueTextByLabel !== 'undefined' && !!valueTextByLabel[pinLabel];
    const _hasLed = typeof ledByLabel !== 'undefined' && !!ledByLabel[pinLabel];
    const _knownLabels = typeof valueTextByLabel !== 'undefined' ? Object.keys(valueTextByLabel) : [];
    console.log('[DBG TELEM] label="' + pinLabel + '" hasValEl:' + _hasVal + ' hasLedEl:' + _hasLed + ' knownLabels:' + JSON.stringify(_knownLabels));

    try {
      const payload = JSON.parse(jsonStr);
      telemetryByLabel[pinLabel] = payload;

      // Valeur numérique (si show_value=true)
      updateValueTextForLabel(pinLabel);

      // LED d’activité (binaire + decay côté front)
      if (typeof ledByLabel !== 'undefined' && ledByLabel) {
        const led = ledByLabel[pinLabel];
        if (led && payload && payload.active) {
          led.style.visibility = 'visible';
          if (ledDecayTimersByLabel[pinLabel]) {
            clearTimeout(ledDecayTimersByLabel[pinLabel]);
          }
          ledDecayTimersByLabel[pinLabel] = setTimeout(() => {
            if (led) led.style.visibility = 'hidden';
          }, 300);
        }
      }
    } catch (e) {
      console.error('[PIN_TELEMETRY] JSON parse error:', e);
    }
    return;
  }

  if(message.startsWith('PIN_CONFIG:')){
    /* Format: PIN_CONFIG:pin:json — le JSON peut contenir des ":", ne pas split sur tout */
    const firstColon = message.indexOf(':');
    const secondColon = message.indexOf(':', firstColon + 1);
    if(secondColon !== -1){
     const pin = message.substring(firstColon + 1, secondColon);
     const jsonStr = message.substring(secondColon + 1);
     console.log('[initWebSocket] PIN_CONFIG reçu, pin:', pin, 'jsonStr (premiers 100 chars):', jsonStr.substring(0, 100));
     try {
      const config = JSON.parse(jsonStr);
      if(typeof pcfg === 'undefined' || !pcfg) return;
      applyPinReplacementLogic(pin);
      var targetKey = pin;
      if(['SDA','SCL'].includes(pin)) targetKey = 'I2C';
      else if(['MOSI','MISO','SCK'].includes(pin)) targetKey = 'SPI';
      /* Ne pas écraser une config avec un composant réel par une config bus par défaut */
      var existingCfg = pcfg[targetKey];
      var existingHasComponent = existingCfg && existingCfg.role && typeof isBusRole === 'function' && !isBusRole(existingCfg.role);
      var newIsBusDefault = config && config.role && typeof isBusRole === 'function' && isBusRole(config.role);
      if(existingHasComponent && newIsBusDefault) {
       console.log('[initWebSocket] Config existante préservée pour', targetKey, '(role:', existingCfg.role, ')');
      } else if (existingCfg && existingCfg.role) {
       /* Fusionner: garder les réglages locaux (filtre, type MIDI, etc.) non sauvegardés */
       pcfg[targetKey] = Object.assign({}, config, existingCfg);
      } else {
       pcfg[targetKey] = config;
      }
      if(typeof updatePinsList === 'function') updatePinsList();
      if(typeof updateBusVisuals === 'function') updateBusVisuals();
      if(typeof cur !== 'undefined' && cur === pin && typeof applyCfg === 'function'){
       applyCfg(pcfg[targetKey]);
      }
     } catch(err) {
      console.error('[initWebSocket] Erreur parsing config:', err, 'pin:', pin, 'jsonStr:', jsonStr.substring(0, 200));
     }
    } else {
     console.warn('[initWebSocket] Format PIN_CONFIG invalide, message:', message.substring(0, 200));
    }
   }
  };

  // Off / RAW / MIDI : une seule commande PIN_MONITORING (0 = off, 1 = actif)
  const modeEl = document.getElementById('telemetryMode');
  if (modeEl && !modeEl._listenerAttached) {
    modeEl._listenerAttached = true;
    modeEl.addEventListener('change', () => {
      telemetryMode = modeEl.value;
      const on = telemetryMode === 'raw' || telemetryMode === 'midi';
      console.log('[DBG SELECT] mode changé:', telemetryMode, '| on:', on, '| ws.readyState:', websocket ? websocket.readyState : 'null');
      if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(`PIN_MONITORING:${on ? '1' : '0'}`);
        console.log('[DBG SELECT] PIN_MONITORING:' + (on ? '1' : '0') + ' envoyé');
      } else {
        console.warn('[DBG SELECT] WebSocket non disponible, état:', websocket ? websocket.readyState : 'null');
      }
      if (!on) clearTelemetryVisuals();
      else updateAllValuesForMode();
    });
  }
  websocket.onerror = function(error){
   console.error('[initWebSocket] Erreur WebSocket:', error);
  };
  websocket.onclose = function(){
   console.log('WebSocket disconnected');
   setTimeout(initWebSocket, 3000);
  };
 } catch(err) {
  console.error('[initWebSocket] Erreur création WebSocket:', err);
 }
}
