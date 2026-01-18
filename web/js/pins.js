/* Fonctions de gestion des pins et affichage du board */

/**
 * Détermine le type de pin pour le CSS
 * Utilise les données du backend (caps.bus) pour les bus
 * @param {string} lbl - Label de la pin
 * @returns {string} Type CSS (analog, digital, uart, i2c, spi)
 */
/**
 * Détermine le type de pin pour le CSS (affichage visuel uniquement)
 * @param {string} lbl - Label de la pin
 * @returns {string} Type CSS (analog, digital, uart, i2c, spi)
 */
function pType(lbl){
 if(!lbl) return 'digital';
 if(lbl.startsWith('A')) return 'analog';
 if(lbl.startsWith('D')) {
  if(!caps || !caps.pins || !caps.bus) return 'digital';
  const pin = caps.pins.find(p => p && p.label === lbl);
  if(!pin) return 'digital';
  const gpio = parseInt(pin.gpio);
  if(isNaN(gpio)) return 'digital';
  if(caps.bus.uart && (gpio === caps.bus.uart.tx || gpio === caps.bus.uart.rx)) return 'uart';
  if(caps.bus.i2c && (gpio === caps.bus.i2c.sda || gpio === caps.bus.i2c.scl)) return 'i2c';
  if(caps.bus.spi && (gpio === caps.bus.spi.mosi || gpio === caps.bus.spi.miso || gpio === caps.bus.spi.sck)) return 'spi';
  return 'digital';
 }
 if(lbl === 'SDA' || lbl === 'SCL') return 'i2c';
 if(lbl === 'TX' || lbl === 'RX') return 'uart';
 if(lbl === 'MOSI' || lbl === 'MISO' || lbl === 'SCK') return 'spi';
 return 'digital';
}

function getRoleDisplayLabel(role){
 if(!role) return '';
 
 // Migrer les anciens formats si nécessaire
 const migratedRole = typeof migrateRole === 'function' ? migrateRole(role) : role;
 
 // Utiliser les définitions du backend
 if(typeof getComponentDefinition === 'function') {
  const def = getComponentDefinition(migratedRole);
  if(def) return def.displayName;
 }
 
 // Fallback: retourner le rôle tel quel
 return role;
}

/**
 * Remplace les variables dans un template avec les valeurs de la config
 * @param {string} template - Template avec variables (ex: "CC#{cc}", "Note {note}")
 * @param {Object} cfg - Configuration du composant
 * @returns {string} Texte avec variables remplacées
 */
function replaceTemplate(template, cfg, def) {
 if(!template) return '';
 
 // Utiliser le mapping du backend si disponible
 let valueMappings = {};
 if(def && def.statusValueMappings) {
  try {
   // Si c'est déjà un objet, l'utiliser directement
   if(typeof def.statusValueMappings === 'object') {
    valueMappings = def.statusValueMappings;
   } else if(typeof def.statusValueMappings === 'string') {
    // Sinon, essayer de parser la string JSON
    valueMappings = JSON.parse(def.statusValueMappings);
   }
  } catch(e) {
   console.warn('Erreur parsing statusValueMappings:', e, 'value:', def.statusValueMappings);
   valueMappings = {};
  }
 }
 
 // Remplacer les variables {variable} avec les valeurs de cfg
 return template.replace(/\{(\w+)\}/g, (match, key) => {
  const value = cfg[key];
  
  // Si la valeur est undefined ou null, utiliser une valeur par défaut selon la clé
  if(value === undefined || value === null) {
   if(key === 'cc') return '7';
   if(key === 'note') return '60';
   if(key === 'pc') return '0';
   return '';
  }
  
  // Vérifier si on a un mapping pour cette clé (depuis le backend)
  if(valueMappings[key] && valueMappings[key][value]) {
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
 if(!def || !cfg) return '';
 
 // Si rtpType est défini, utiliser le template du message MIDI
 if(cfg.rtpType && def.midiMessages && Array.isArray(def.midiMessages)) {
  const msg = def.midiMessages.find(m => m.displayName === cfg.rtpType);
  if(msg && msg.statusTemplate) {
   return replaceTemplate(msg.statusTemplate, cfg, def);
  }
 }
 
 // Cas spéciaux pour les bus (UART, I2C, SPI)
 // Ces composants n'ont pas de définition standard, on les gère ici
 if(cfg.role === 'uart' && typeof caps !== 'undefined' && caps) {
  const pin = caps.pins?.find(p => p.label === pinLabel);
  if(pin && caps.bus?.uart) {
   return pin.gpio === caps.bus.uart.tx ? 'TX' : 'RX';
  }
 }
 
 // Si pas de rtpType mais qu'on a un statusTextTemplate, l'utiliser
 // (ex: pour LED avec ledMode, le backend pourrait fournir un template)
 if(def.statusTextTemplate) {
  return replaceTemplate(def.statusTextTemplate, cfg, def);
 }
 
 // Fallback: utiliser le displayName
 return def.displayName || '';
}

/**
 * Génère un résumé court de la configuration d'un composant
 * Utilise les définitions du backend pour déterminer le support MIDI
 * @param {Object} cfg - Configuration du composant
 * @param {string} pinLabel - Label de la pin
 * @returns {string} Résumé court (ex: "CC#7", "Note 60")
 */
function stat(cfg, pinLabel){
 if(!cfg || !cfg.role) return '';
 
 // Migrer les anciens formats si nécessaire
 const migratedRole = typeof migrateRole === 'function' ? migrateRole(cfg.role) : cfg.role;
 
 // Obtenir la définition du composant depuis le backend
 const def = typeof getComponentDefinition === 'function' ? getComponentDefinition(migratedRole) : null;
 
 // Utiliser la fonction générique pour générer le texte de statut
 if(def && typeof getComponentStatusText === 'function') {
  return getComponentStatusText(def, cfg, pinLabel);
 }
 
 // Fallback si les définitions ne sont pas disponibles
 return cfg.role || '';
}

function setOptions(sel,options,pre=0){
 if(!sel) return;
 let html='';
 let firstValue=null;
 let selectedValue=null;
 if(Array.isArray(options)){
  html=options.map((o,i)=>{
   if(i===0) firstValue=o;
   if(i===pre) selectedValue=o;
   return `<option ${i===pre?'selected':''}>${o}</option>`;
  }).join('');
 }else{
  let isFirst=true;
  Object.keys(options).forEach(groupKey=>{
   const group=options[groupKey];
   if(typeof group==='string'){
    if(firstValue===null) firstValue=groupKey;
    const shouldSelect=(pre===0&&isFirst)||(typeof pre==='string'&&groupKey===pre);
    if(shouldSelect) selectedValue=groupKey;
    html+=`<option value="${groupKey}" ${shouldSelect?'selected':''}>${group}</option>`;
    if(isFirst) isFirst=false;
   }else if(typeof group==='object'&&group.label){
    // Objet avec label et disabled
    // Ne pas sélectionner les éléments désactivés comme première valeur
    if(firstValue===null && !group.disabled) firstValue=groupKey;
    const shouldSelect=(pre===0&&isFirst&&!group.disabled)||(typeof pre==='string'&&groupKey===pre);
    if(shouldSelect) selectedValue=groupKey;
    const disabled=group.disabled?'disabled':'';
    html+=`<option value="${groupKey}" ${shouldSelect?'selected':''} ${disabled}>${group.label}</option>`;
    if(isFirst&&!group.disabled) isFirst=false;
   }else if(group.items&&Array.isArray(group.items)){
    html+=`<optgroup label="${group.label}">`;
    group.items.forEach((item)=>{
     const selected=(typeof pre==='string'&&item.value===pre)?'selected':'';
     if(selected) selectedValue=item.value;
     const disabled=item.disabled?'disabled':'';
     html+=`<option value="${item.value}" ${selected} ${disabled}>${item.label}</option>`;
    });
    html+=`</optgroup>`;
   }else{
    if(firstValue===null) firstValue=groupKey;
    const shouldSelect=(pre===0&&isFirst)||(typeof pre==='string'&&groupKey===pre);
    if(shouldSelect) selectedValue=groupKey;
    html+=`<option value="${groupKey}" ${shouldSelect?'selected':''}>${group}</option>`;
    if(isFirst) isFirst=false;
   }
  });
 }
 sel.innerHTML=html;
 // Définir explicitement sel.value après avoir mis le HTML
 if(selectedValue!==null){
  sel.value=selectedValue;
 }else if(firstValue!==null){
  sel.value=firstValue;
 }
}

 function updateBusVisuals(){
 // 1. Enlever tous les grisages
 if(typeof prect === 'undefined' || !prect) return;
 Object.keys(prect).forEach(lbl=>{
  const r = prect[lbl];
  if(!r || typeof r.classList === 'undefined') return;
  r.classList.remove('busDisabled');
 });
 
 if(typeof caps === 'undefined' || !caps || !caps.pins || !Array.isArray(caps.pins)) return;
 
 // 2. Obtenir les GPIOs utilisés depuis le cache backend
 let usedGpios = new Set();
 
 if(typeof getCachedUsedGpios === 'function') {
  usedGpios = new Set(getCachedUsedGpios());
 }
 
 // 3. Ajouter les GPIOs de l'édition en cours (pas encore sauvegardés)
 if(typeof pcfg === 'undefined' || !pcfg) return;
 Object.keys(pcfg).forEach(lbl => {
  const cfg = pcfg[lbl];
  if(!cfg) return;
  const pin = caps.pins.find(p => p && p.label === lbl);
  if(!pin || pin.gpio === undefined) return;
  
  const gpio = parseInt(pin.gpio);
  if(!isNaN(gpio)) usedGpios.add(gpio);
  
  // Pour les composants complexes, utiliser les additionalPins du backend
  if(cfg && cfg.role && typeof getComponentDefinition === 'function') {
   const migratedRole = typeof migrateRole === 'function' ? migrateRole(cfg.role) : cfg.role;
   const def = getComponentDefinition(migratedRole);
   
   if(def && def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0) {
    def.additionalPins.forEach(pinDef => {
     // Construire l'ID du champ dynamiquement depuis l'ID du composant
     // Utiliser le préfixe basé sur l'ID du composant plutôt que "mux" hardcodé
     const prefix = def.id ? def.id : 'comp';
     const formId = prefix + pinDef.id.charAt(0).toUpperCase() + pinDef.id.slice(1);
     const formEl = $('#' + formId);
     if(formEl) {
      const val = parseInt(formEl.value);
      if(!isNaN(val) && val !== 255) usedGpios.add(val);
     }
    });
   }
  }
 });
 
 // 4. Créer un map GPIO -> labels pour le grisage
 const gpioMap = new Map();
 caps.pins.forEach(p => {
  const gpio = parseInt(p.gpio);
  if(isNaN(gpio)) return;
  if(!gpioMap.has(gpio)) gpioMap.set(gpio, []);
  gpioMap.get(gpio).push(p);
 });
 
 // 5. Griser toutes les pins dont le GPIO est utilisé
 usedGpios.forEach(gpio => {
  const pinsForGpio = gpioMap.get(gpio) || [];
  pinsForGpio.forEach(pin => {
   if(pin && pin.label && prect[pin.label]) {
    prect[pin.label].classList.add('busDisabled');
   }
  });
 });
}

function drawBoard(){
 const L=$('#pinsLeft'),R=$('#pinsRight');
 if(!L||!R||typeof caps === 'undefined'||!caps||!caps.pins||!Array.isArray(caps.pins))return;
 L.innerHTML='';
 R.innerHTML='';
 const RH=28;
 const H=20;
 const isS3=caps.board&&caps.board.toLowerCase().includes('s3');
 const W=isS3?32:44;
 const COL=isS3?{c1:20,c2:54,c3:88,c4:238,c5:272,c6:306}:{c1:20,c2:68,c4:238,c5:286};
 
 const mk=(x,y,w,h,fill,stroke,label,clk=true)=>{
 const g=document.createElementNS('http://www.w3.org/2000/svg','g');
 const r=document.createElementNS('http://www.w3.org/2000/svg','rect');
 r.setAttribute('x',x);
 r.setAttribute('y',y);
 r.setAttribute('width',w);
 r.setAttribute('height',h);
 r.setAttribute('rx','4');
 r.setAttribute('fill',fill);
 r.setAttribute('stroke',stroke);
 g.appendChild(r);
 if(label){
 const t=document.createElementNS('http://www.w3.org/2000/svg','text');
 t.setAttribute('x',x+w/2);
 t.setAttribute('y',y+h/2+1);
 t.setAttribute('text-anchor','middle');
 t.setAttribute('class','svg-t');
 t.textContent=label;
 g.appendChild(t);
 }
 if(clk&&label){
 g.style.cursor='pointer';
 r.dataset.label=label;
 prect[label]=r;
 r.addEventListener('click',()=>{
 // Ne pas permettre le clic si la pin est grisée (déjà configurée)
 if(r.classList.contains('busDisabled')){
  return;
 }
 if(window._selRect) window._selRect.classList.remove('selectedSquare');
 window._selRect=r;
 r.classList.add('selectedSquare');
 cur=label;
 $('#selPin').textContent=label;
 handlePinClick(label);
 updFunc(label);
 /* SIMPLIFICATION : Appliquer la config si elle existe, peu importe le type */
 /* (updFunc() gère déjà les bus et affiche un message) */
 if(pcfg[cur]) {
  applyCfg(pcfg[cur]);
 }
});
 }
 return g;
 };
 
 const getPinColor=(label)=>{
 // Utiliser pType qui utilise les données du backend
 const type = pType(label);
 switch(type) {
  case 'uart': return FC.UART;
  case 'analog': return FC.ANALOG;
  case 'i2c': return FC.I2C;
  case 'spi': return FC.SPI;
  default: return FC.DIGITAL;
 }
 };
 
 const pins=caps.pins;
 const gpioMap=new Map();
 pins.forEach(p=>{
 if(!gpioMap.has(p.gpio))gpioMap.set(p.gpio,[]);
 gpioMap.get(p.gpio).push(p);
 });
 
 const getAlias=(gpio,prefix)=>{
 const ps=gpioMap.get(gpio)||[];
 return ps.find(p=>p.label.startsWith(prefix))?.label||'';
 };
 
 const getBus=(gpio)=>{
 // Utiliser caps.bus pour identifier les pins de bus
 const bus = caps.bus || {};
 if(bus.i2c && gpio === bus.i2c.sda) return 'SDA';
 if(bus.i2c && gpio === bus.i2c.scl) return 'SCL';
 if(bus.uart && gpio === bus.uart.tx) return 'TX';
 if(bus.uart && gpio === bus.uart.rx) return 'RX';
 if(bus.spi && gpio === bus.spi.mosi) return 'MOSI';
 if(bus.spi && gpio === bus.spi.miso) return 'MISO';
 if(bus.spi && gpio === bus.spi.sck) return 'SCK';
 return '';
 };
 
 if(isS3){
 const left3=(row,busLbl,adcLbl,dLbl)=>{
 const y=30+row*RH;
 const f=document.createDocumentFragment();
 if(busLbl){
 f.appendChild(mk(COL.c1,y-10,W,H,getPinColor(busLbl),'#9ca3af',busLbl));
 }else{
 f.appendChild(mk(COL.c1,y-10,W,H,'#9ca3af','#9ca3af','',false));
 }
 if(adcLbl){
 f.appendChild(mk(COL.c2,y-10,W,H,FC.ANALOG,'#9ca3af',adcLbl));
 }else{
 f.appendChild(mk(COL.c2,y-10,W,H,'#9ca3af','#9ca3af','',false));
 }
 f.appendChild(mk(COL.c3,y-10,W,H,FC.DIGITAL,'#9ca3af',dLbl));
 L.appendChild(f);
 };
 
 const right3=(row,dLbl,adcLbl,busLbl)=>{
 const y=30+row*RH;
 const f=document.createDocumentFragment();
 f.appendChild(mk(COL.c4,y-10,W,H,FC.DIGITAL,'#9ca3af',dLbl));
 if(adcLbl){
 f.appendChild(mk(COL.c5,y-10,W,H,FC.ANALOG,'#9ca3af',adcLbl));
 }else{
 f.appendChild(mk(COL.c5,y-10,W,H,'#9ca3af','#9ca3af','',false));
 }
 if(busLbl){
 f.appendChild(mk(COL.c6,y-10,W,H,getPinColor(busLbl),'#9ca3af',busLbl));
 }else{
 f.appendChild(mk(COL.c6,y-10,W,H,'#9ca3af','#9ca3af','',false));
 }
 R.appendChild(f);
 };
 
 // Filtrer les pins D en évitant les doublons par GPIO (un GPIO = une seule pin D)
 // Prendre la première pin avec label "D" pour chaque GPIO
 const dPinsMap=new Map();
 pins.filter(p=>p.label.startsWith('D')).forEach(p=>{
  if(!dPinsMap.has(p.gpio))dPinsMap.set(p.gpio,p);
 });
 const dPins=Array.from(dPinsMap.values()).sort((a,b)=>{
 const na=parseInt(a.label.substring(1));
 const nb=parseInt(b.label.substring(1));
 return na-nb;
 });
 const displayed=new Set();
 let leftRow=0,rightRow=0;
 
 dPins.filter(p=>{
 const n=parseInt(p.label.substring(1));
 return n<=6;
 }).forEach(p=>{
 if(displayed.has(p.gpio))return;
 displayed.add(p.gpio);
 const busLbl=getBus(p.gpio);
 const adcLbl=getAlias(p.gpio,'A');
 left3(leftRow++,busLbl,adcLbl,p.label);
 });
 
 R.appendChild(mk(COL.c4,30+rightRow*RH-10,W,H,FC.POWER,'#9ca3af','5V',false));
 rightRow++;
 R.appendChild(mk(COL.c4,30+rightRow*RH-10,W,H,FC.GND,'#9ca3af','GND',false));
 rightRow++;
 R.appendChild(mk(COL.c4,30+rightRow*RH-10,W,H,FC.POWER,'#9ca3af','3V3',false));
 rightRow++;
 
 dPins.filter(p=>{
 const n=parseInt(p.label.substring(1));
 return n>=7;
 }).sort((a,b)=>{
 const na=parseInt(a.label.substring(1));
 const nb=parseInt(b.label.substring(1));
 return nb-na;
 }).forEach(p=>{
 if(displayed.has(p.gpio))return;
 displayed.add(p.gpio);
 const adcLbl=getAlias(p.gpio,'A');
 const busLbl=getBus(p.gpio);
 right3(rightRow++,p.label,adcLbl,busLbl);
 });
 }else{
 const left=(row,tl,tc,dl)=>{
 const y=30+row*RH;
 const f=document.createDocumentFragment();
 if(tl){
 f.appendChild(mk(COL.c1,y-10,W,H,tc,'#9ca3af',tl));
 }else{
 f.appendChild(mk(COL.c1,y-10,W,H,'#9ca3af','#9ca3af','',false));
 }
 f.appendChild(mk(COL.c2,y-10,W,H,FC.DIGITAL,'#9ca3af',dl));
 L.appendChild(f);
 };
 
 // Utiliser caps.bus pour identifier les pins de bus (données du backend)
 const bus = caps.bus || {};
 const busGpios = new Set();
 // Convertir en nombre pour éviter les problèmes de type (string vs number)
 if(bus.i2c) { 
  busGpios.add(Number(bus.i2c.sda)); 
  busGpios.add(Number(bus.i2c.scl)); 
 }
 if(bus.spi) { 
  busGpios.add(Number(bus.spi.mosi)); 
  busGpios.add(Number(bus.spi.miso)); 
  busGpios.add(Number(bus.spi.sck)); 
 }
 if(bus.uart) { 
  busGpios.add(Number(bus.uart.tx)); 
  busGpios.add(Number(bus.uart.rx)); 
 }
 
 const analogPins=pins.filter(p=>p.label.startsWith('A')&&p.caps?.adc).sort((a,b)=>a.label.localeCompare(b.label));
 const i2cPins=pins.filter(p=>bus.i2c && (p.gpio===bus.i2c.sda || p.gpio===bus.i2c.scl)).sort((a,b)=>a.gpio===bus.i2c?.sda?-1:1);
 const uartPins=pins.filter(p=>bus.uart && (p.gpio===bus.uart.tx || p.gpio===bus.uart.rx)).sort((a,b)=>a.gpio===bus.uart?.tx?-1:1);
 // Filtrer les pins D en évitant les doublons par label ET par GPIO
 // Sur C3, getAllMappings() peut retourner des doublons (même label "D4" plusieurs fois)
 // Utiliser une Map avec label comme clé unique pour garantir un seul exemplaire par label
 const digitalPinsMap=new Map();
 const seenGpioForD=new Set();
 // Convertir p.gpio en nombre pour la comparaison
 pins.filter(p=>p.label.startsWith('D')&&!busGpios.has(Number(p.gpio))).forEach(p=>{
  // Utiliser le label comme clé unique pour éviter les doublons de label
  // Si plusieurs pins ont le même label "D4", on garde seulement la première
  // ET éviter aussi les doublons par GPIO (un GPIO = une seule pin D affichée)
  if(!digitalPinsMap.has(p.label)&&!seenGpioForD.has(p.gpio)){
   digitalPinsMap.set(p.label,p);
   seenGpioForD.add(p.gpio);
  }
 });
 const digitalPins=Array.from(digitalPinsMap.values()).sort((a,b)=>{
  const na=parseInt(a.label.substring(1));
  const nb=parseInt(b.label.substring(1));
  return na-nb;
 });
 
 const leftPins=[];
 const displayedGpios=new Set();
 let leftRow=0,rightRow=0;
 
 analogPins.forEach(p=>{
 leftPins.push({gpio:p.gpio,label:p.label,color:getPinColor(p.label),dLabel:getAlias(p.gpio,'D')});
 displayedGpios.add(Number(p.gpio));
 });

 digitalPins.forEach(p=>{
 if(!displayedGpios.has(Number(p.gpio))){
 leftPins.push({gpio:p.gpio,label:'',color:FC.DIGITAL,dLabel:p.label});
 displayedGpios.add(Number(p.gpio));
 }
 });
 
 i2cPins.forEach(p=>{
 // Éviter les doublons : vérifier que le GPIO n'a pas déjà été affiché
 if(!displayedGpios.has(Number(p.gpio))){
  leftPins.push({gpio:p.gpio,label:p.label,color:getPinColor(p.label),dLabel:getAlias(p.gpio,'D')});
  displayedGpios.add(Number(p.gpio));
 }
 });
 
 // UART TX depuis caps.bus
 const uartTx=bus.uart ? uartPins.find(p=>p.gpio===bus.uart.tx) : null;
 if(uartTx&&!displayedGpios.has(Number(uartTx.gpio))){
 leftPins.push({gpio:uartTx.gpio,label:uartTx.label,color:getPinColor(uartTx.label),dLabel:getAlias(uartTx.gpio,'D')});
 displayedGpios.add(Number(uartTx.gpio));
 }
 
 leftPins.sort((a,b)=>a.gpio-b.gpio);
 leftPins.forEach(p=>{
 left(leftRow++,p.label,p.color,p.dLabel);
 });
 
 R.appendChild(mk(COL.c4,30+rightRow*RH-10,W,H,FC.POWER,'#9ca3af','5V',false));
 rightRow++;
 R.appendChild(mk(COL.c4,30+rightRow*RH-10,W,H,FC.GND,'#9ca3af','GND',false));
 rightRow++;
 R.appendChild(mk(COL.c4,30+rightRow*RH-10,W,H,FC.POWER,'#9ca3af','3V3',false));
 rightRow++;
 
 const right=(row,dl,tl,tc)=>{
 const y=30+row*RH;
 const f=document.createDocumentFragment();
 f.appendChild(mk(COL.c4,y-10,W,H,FC.DIGITAL,'#9ca3af',dl));
 f.appendChild(mk(COL.c5,y-10,W,H,tc,'#9ca3af',tl));
 return f;
 };
 
 // Pins SPI depuis caps.bus
 const spiPins=pins.filter(p=>bus.spi && (p.gpio===bus.spi.mosi || p.gpio===bus.spi.miso || p.gpio===bus.spi.sck)).sort((a,b)=>{
 const order = [bus.spi?.mosi, bus.spi?.miso, bus.spi?.sck];
 return order.indexOf(a.gpio) - order.indexOf(b.gpio);
 });
 spiPins.forEach(p=>{
 // Éviter les doublons : vérifier que le GPIO n'a pas déjà été affiché
 if(!displayedGpios.has(Number(p.gpio))){
  R.appendChild(right(rightRow++,getAlias(p.gpio,'D'),p.label,getPinColor(p.label)));
  displayedGpios.add(Number(p.gpio));
 }
 });
 
 // UART RX depuis caps.bus
 const uartRx=bus.uart ? pins.find(p=>p.gpio===bus.uart.rx) : null;
 if(uartRx&&!displayedGpios.has(Number(uartRx.gpio))){
 R.appendChild(right(rightRow++,getAlias(uartRx.gpio,'D'),uartRx.label,getPinColor(uartRx.label)));
 displayedGpios.add(Number(uartRx.gpio));
 }
 }
 
 const boardNameEl=$('#boardName');
 if(boardNameEl&&caps.board){
 const boardUpper=caps.board.toUpperCase().replace('-','-');
 boardNameEl.textContent=boardUpper;
 }
 
 updateBusVisuals();
}

function updatePinsList(){
 const pl=$('#pinsList');
 if(!pl) return;
 pl.innerHTML='';
 
 // Collecter les GPIOs des composants complexes sauvegardés depuis pcfg
 const savedComplexSigGpios = new Set();
 if(typeof pcfg !== 'undefined' && pcfg) {
   Object.keys(pcfg).forEach(lbl => {
     const cfg = pcfg[lbl];
     if(cfg && cfg.additionalPins && typeof cfg.additionalPins === 'object' && cfg.additionalPins.sig !== undefined) {
       const sigGpio = parseInt(cfg.additionalPins.sig);
       if(!isNaN(sigGpio)) savedComplexSigGpios.add(sigGpio);
     }
   });
 }

 // Afficher les pins configurées (unifié depuis pcfg)
 if(typeof pcfg === 'undefined' || !pcfg) return;
 Object.keys(pcfg).forEach(lbl=>{
  const cfg=pcfg[lbl];
  if(!cfg||!cfg.role) return;
  // Ignorer les pins avec préfixe M (anciennes pins avec préfixe historique)
  if(lbl.startsWith('M')) return;
  
  // Détecter composant avec additionalPins depuis pcfg.additionalPins
  const hasAdditionalPins = cfg.additionalPins && typeof cfg.additionalPins === 'object' && cfg.additionalPins.sig !== undefined;
  const role = typeof migrateRole === 'function' ? migrateRole(cfg.role) : cfg.role;
  const def = typeof getComponentDefinition === 'function' ? getComponentDefinition(role) : null;
  const hasAdditionalPinsFromDef = def && (def.additionalPinCount > 0 || (def.additionalPins && Array.isArray(def.additionalPins) && def.additionalPins.length > 0));
  const isComplex = hasAdditionalPins || hasAdditionalPinsFromDef;
  
  // Pour les composants avec additionalPins : afficher depuis pcfg (unifié)
  if(isComplex && hasAdditionalPins){
   if(caps && caps.pins){
    const pin=caps.pins.find(p=>p.label===lbl);
    // Si ce GPIO est déjà dans un autre composant complexe sauvegardé, ne pas afficher
    if(pin && savedComplexSigGpios.has(parseInt(pin.gpio)) && parseInt(cfg.additionalPins.sig) !== parseInt(pin.gpio)) return;
    
    // Afficher composant complexe depuis pcfg
    const roleName = getRoleDisplayLabel(cfg.role);
    const statText = stat(cfg, lbl);
    const it=document.createElement('div');
    it.className='item complex';
    it.innerHTML=`<span class="lbl">${lbl}</span><span class="role">${roleName}</span><span class="stat">${statText}</span><button class="del-btn">×</button>`;
    it.onclick=()=>{
     if(window._selRect) window._selRect.classList.remove('selectedSquare');
     const r=prect[lbl];
     if(r){
      window._selRect=r;
      r.classList.add('selectedSquare');
     }
     cur=lbl;
     $('#selPin').textContent=lbl;
     updFunc(lbl);
     /* SIMPLIFICATION : Appliquer la config si elle existe */
     if(pcfg[lbl]) {
      applyCfg(pcfg[lbl]);
     }
    };
    const delBtn=it.querySelector('.del-btn');
    if(delBtn) delBtn.onclick=async (e)=>{
     e.stopPropagation();
     console.log('[deletePin] Suppression composant complexe sur pin:', lbl);
     try {
      const formData = new URLSearchParams();
      formData.append('pin', lbl);
      const response = await fetch('/api/pins/delete', {
       method: 'POST',
       headers: {'Content-Type': 'application/x-www-form-urlencoded'},
       body: formData.toString()
      });
      console.log('[deletePin] Réponse API:', response.status, response.statusText);
      if(!response.ok) {
       const errorText = await response.text();
       console.error('[deletePin] Erreur suppression API:', response.status, errorText);
       delete pcfg[lbl];
       updatePinsList();
       updateBusVisuals();
       return;
      }
      let result;
      try {
       result = await response.json();
       console.log('[deletePin] Résultat JSON:', result);
      } catch(jsonErr) {
       const text = await response.text();
       console.warn('[deletePin] Réponse non-JSON, texte:', text);
       result = {status: 'ok'};
      }
      delete pcfg[lbl];
      if(typeof loadConfiguredPins === 'function') {
       await loadConfiguredPins();
      } else {
       updatePinsList();
       updateBusVisuals();
      }
      console.log('[deletePin] Composant complexe supprimé');
     } catch(err) {
      console.error('[deletePin] Erreur suppression composant complexe:', err);
      delete pcfg[lbl];
      updatePinsList();
      updateBusVisuals();
     }
    };
    pl.appendChild(it);
   }
   return;
  }
  
  /* Vérifier si cette pin est utilisée par un composant complexe comme SIG (depuis pcfg) */
  /* Si oui, ne pas afficher le composant simple (le complexe a priorité) */
  if(caps && caps.pins) {
    const pin = caps.pins.find(p => p.label === lbl);
    if(pin && savedComplexSigGpios.has(parseInt(pin.gpio))) {
      /* Cette pin est utilisée par un composant complexe, ne pas afficher le simple */
      return;
    }
  }
  
  // Afficher les pins simples (non complexes)
  const it=document.createElement('div');
  it.className=`item ${pType(lbl)}`;
  it.innerHTML=`<span class="lbl">${lbl}</span><span class="role">${getRoleDisplayLabel(cfg.role)}</span><span class="stat">${stat(cfg, lbl)}</span><button class="del-btn">×</button>`;
  it.onclick=()=>{
   if(window._selRect) window._selRect.classList.remove('selectedSquare');
   const r=prect[lbl];
   if(r){
    window._selRect=r;
    r.classList.add('selectedSquare');
   }
   cur=lbl;
   $('#selPin').textContent=lbl;
   updFunc(lbl);
   /* SIMPLIFICATION : Appliquer la config si elle existe */
   if(pcfg[lbl]) {
    applyCfg(pcfg[lbl]);
   }
  };
  const delBtn=it.querySelector('.del-btn');
  if(delBtn) delBtn.onclick=async (e)=>{
   e.stopPropagation();
   console.log('[deletePin] Suppression composant simple sur pin:', lbl);
   try {
    const formData = new URLSearchParams();
    formData.append('pin', lbl);
    const response = await fetch('/api/pins/delete', {
     method: 'POST',
     headers: {'Content-Type': 'application/x-www-form-urlencoded'},
     body: formData.toString()
    });
    console.log('[deletePin] Réponse API:', response.status, response.statusText);
    if(!response.ok) {
     const errorText = await response.text();
     console.error('[deletePin] Erreur suppression API:', response.status, errorText);
     delete pcfg[lbl];
     updatePinsList();
     updateBusVisuals();
     return;
    }
    let result;
    try {
     result = await response.json();
     console.log('[deletePin] Résultat JSON:', result);
    } catch(jsonErr) {
     const text = await response.text();
     console.warn('[deletePin] Réponse non-JSON, texte:', text);
     result = {status: 'ok'};
    }
    delete pcfg[lbl];
    if(typeof loadConfiguredPins === 'function') {
     await loadConfiguredPins();
    } else {
     updatePinsList();
     updateBusVisuals();
    }
    console.log('[deletePin] Composant simple supprimé');
   } catch(err) {
    console.error('[deletePin] Erreur suppression composant simple:', err);
    delete pcfg[lbl];
    updatePinsList();
    updateBusVisuals();
   }
  };
  pl.appendChild(it);
 });
}
