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
  pcfg[label] = {role: 'potentiometer'};
  if(label.startsWith('A')){
   const dLabel = label.replace('A', 'D');
   if(pcfg[dLabel]) delete pcfg[dLabel];
  }
 } else if(['D0','D1','D2','D3'].includes(label)){
  pcfg[label] = {role: 'button'};
  const aLabel = label.replace('D', 'A');
  if(pcfg[aLabel]) delete pcfg[aLabel];
 } else if(['SDA','SCL'].includes(label)){
  pcfg['I2C'] = {role: 'i2c'};
  if(pcfg['D4']) delete pcfg['D4'];
  if(pcfg['D5']) delete pcfg['D5'];
  if(pcfg['SDA']) delete pcfg['SDA'];
  if(pcfg['SCL']) delete pcfg['SCL'];
  label = 'I2C';
 } else if(['MOSI','MISO','SCK'].includes(label)){
  pcfg['SPI'] = {role: 'spi'};
  if(pcfg['D8']) delete pcfg['D8'];
  if(pcfg['D9']) delete pcfg['D9'];
  if(pcfg['D10']) delete pcfg['D10'];
  if(pcfg['MOSI']) delete pcfg['MOSI'];
  if(pcfg['MISO']) delete pcfg['MISO'];
  if(pcfg['SCK']) delete pcfg['SCK'];
  label = 'SPI';
 } else if(label === 'TX' || label === 'RX'){
  pcfg[label] = {role: 'uart'};
  if(label === 'TX' && pcfg['D6']) delete pcfg['D6'];
  if(label === 'RX' && pcfg['D7']) delete pcfg['D7'];
 } else if(['D4','D5'].includes(label)){
  if(pcfg['I2C']) delete pcfg['I2C'];
  pcfg[label] = {role: 'button'};
 } else if(['D8','D9','D10'].includes(label)){
  if(pcfg['SPI']) delete pcfg['SPI'];
  pcfg[label] = {role: 'button'};
 } else if(['D6','D7'].includes(label)){
  pcfg[label] = {role: 'button'};
  if(label === 'D6' && pcfg['TX']) delete pcfg['TX'];
  if(label === 'D7' && pcfg['RX']) delete pcfg['RX'];
 } else if(/^D\d+$/.test(label)){
  pcfg[label] = {role: 'button'};
 }
 
 if(typeof updatePinsList === 'function') updatePinsList();
 if(typeof updateBusVisuals === 'function') updateBusVisuals();
}

function handlePinClick(label){
 if(typeof label !== 'string' || !label) return;
 if(typeof prect === 'undefined' || !prect) return;
 if(prect[label] && prect[label].classList && prect[label].classList.contains('busDisabled')){
  return;
 }
 
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
  };
  websocket.onmessage = function(event){
   if(!event || !event.data) return;
   const message = String(event.data);
   if(message.startsWith('PIN_CONFIG:')){
    const parts = message.split(':');
    if(parts.length >= 3){
     const pin = parts[1];
     try {
      const config = JSON.parse(parts.slice(2).join(':'));
      if(typeof pcfg === 'undefined' || !pcfg) return;
      applyPinReplacementLogic(pin);
      if(['SDA','SCL'].includes(pin) && config && config.role === 'I2C'){
       pcfg['I2C'] = config;
      } else if(['MOSI','MISO','SCK'].includes(pin) && config && config.role === 'SPI'){
       pcfg['SPI'] = config;
      } else {
       pcfg[pin] = config;
      }
      if(typeof updatePinsList === 'function') updatePinsList();
      if(typeof updateBusVisuals === 'function') updateBusVisuals();
      if(typeof cur !== 'undefined' && cur === pin && typeof applyCfg === 'function'){
       applyCfg(config);
      }
     } catch(err) {
      console.error('[initWebSocket] Erreur parsing config:', err);
     }
    }
   }
  };
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
