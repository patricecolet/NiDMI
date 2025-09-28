#include "ui_index.h"
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
 <meta charset="UTF-8">
 <meta name="viewport" content="width=device-width, initial-scale=1.0">
 <title>ESP32 Server</title>
 <style>
 :root{--cd:#3B82F6;--ca:#EC4899;--ci:#10B981;--cu:#6B7280;--cs:#8B5CF6;--cp:#EF4444;--cg:#000;--bg:#f9fafb;--bd:#e5e7eb;--tx:#374151;--mt:#6b7280}
 *{margin:0;padding:0;box-sizing:border-box}
 body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#f3f4f6;color:#111827}
 .c{max-width:1200px;margin:0 auto;padding:20px}
 .h{text-align:center;margin-bottom:30px}.h h1{color:#1f2937;margin-bottom:10px}.h p{color:#6b7280}
 .t{display:flex;background:#fff;border-radius:8px;box-shadow:0 1px 3px rgba(0,0,0,.1);margin-bottom:20px}
 .tab{flex:1;padding:15px;text-align:center;cursor:pointer;border-bottom:3px solid transparent;transition:all .2s}
 .tab.active{border-bottom-color:var(--cd);color:var(--cd);font-weight:600}.tab:hover:not(.active){background:var(--bg)}
 .p{display:none;background:#fff;border-radius:8px;padding:25px;box-shadow:0 1px 3px rgba(0,0,0,.1)}.p.active{display:block}
 .f{margin-bottom:20px}.f label{display:block;margin-bottom:8px;font-weight:500;color:var(--tx)}
 .f input,.f select{width:100%;padding:12px;border:1px solid #d1d5db;border-radius:6px;font-size:14px}
 .f input:focus,.f select:focus{outline:none;border-color:var(--cd);box-shadow:0 0 0 3px rgba(59,130,246,.1)}
 .btn{background:var(--cd);color:#fff;border:none;padding:12px 24px;border-radius:6px;cursor:pointer;font-size:14px;font-weight:500;transition:background .2s}
 .btn:hover{background:#2563eb}.btn:disabled{background:#9ca3af;cursor:not-allowed}
 .hint{margin-top:10px;color:var(--mt);font-size:14px}
 .g{display:grid;grid-template-columns:1fr 1fr;gap:20px}.card{background:#f8fafc;border:1px solid #e2e8f0;border-radius:6px;padding:15px}
 .card h3{color:#1f2937;margin-bottom:10px;font-size:16px}.card p{color:var(--mt);font-size:14px;margin-bottom:5px}
 .pl{display:flex;gap:20px;align-items:flex-start}.lp{flex:0 0 30%;min-width:300px}.rp{flex:1 1 auto}
 .cp{background:#fff;border:1px solid var(--bd);border-radius:8px;padding:16px}.cp h4{margin:6px 0 10px;font-size:15px;color:#1f2937}
 .r{display:flex;gap:12px;align-items:center;margin:8px 0;flex-wrap:wrap}.r label{color:var(--tx);font-size:14px}
 .b{width:100%;height:260px;border:1px solid var(--bd);border-radius:8px;background:var(--bg)}
 .l{display:flex;gap:14px;align-items:center;margin:10px 0 8px;flex-wrap:wrap}.s{width:14px;height:14px;border-radius:3px;display:inline-block;margin-right:6px}
 .s.digital{background:var(--cd)}.s.analog{background:var(--ca)}.s.i2c{background:var(--ci)}.s.uart{background:var(--cu)}.s.spi{background:var(--cs)}.s.power{background:var(--cp)}.s.gnd{background:var(--cg)}
 .plist{margin-top:20px;padding:15px;background:var(--bg);border-radius:8px}.plist h4{margin:0 0 10px;font-size:15px;color:var(--tx)}
 .list{display:flex;flex-direction:column;gap:4px}
 .item{display:flex;align-items:center;padding:8px 12px;background:#fff;border-radius:6px;border-left:4px solid var(--bd);cursor:pointer;transition:all .2s}
 .item:hover{background:var(--bg)}.item.analog{border-left-color:var(--ca)}.item.digital{border-left-color:var(--cd)}.item.i2c{border-left-color:var(--ci)}.item.spi{border-left-color:var(--cs)}.item.uart{border-left-color:var(--cu)}
 .lbl{font-weight:700;min-width:40px;margin-right:12px}.role{flex:1;color:var(--tx)}.stat{font-size:.9em;color:var(--mt)}
 .del-btn{background:#ef4444;color:#fff;border:none;border-radius:3px;width:20px;height:20px;cursor:pointer;font-size:12px;margin-left:8px}
 .del-btn:hover{background:#dc2626}
 .btn-p{width:100%;margin-top:15px;padding:12px;background:var(--cd);color:#fff;border:none;border-radius:6px;font-weight:700;cursor:pointer}
 .btn-p:hover{background:#2563eb}
 .svg-t{font-size:9px;fill:#ffffff;dominant-baseline:middle;pointer-events:none;user-select:none}
 .selectedSquare{stroke:#1d4ed8;stroke-width:2}
 .subcard{background:#f9fafb;border:1px solid var(--bd);border-radius:8px;padding:12px;margin-top:8px}
 .subcard .r{margin:6px 0}
 .switch{display:flex;align-items:center;gap:8px}
 .busDisabled{opacity:0.45;filter:grayscale(100%);cursor:not-allowed}
 </style>
 <script>
 const $=s=>document.querySelector(s[0]=='#'?s:'#'+s);
 const pcfg={}; let cur=''; let caps=null; const prect={}; const FC={DIGITAL:'#3B82F6',ANALOG:'#EC4899',I2C:'#10B981',UART:'#6B7280',SPI:'#8B5CF6',POWER:'#EF4444',GND:'#000'};
 
 function initTabs(){ document.querySelectorAll('.tab').forEach(t=>{ t.onclick=()=>{ document.querySelectorAll('.tab').forEach(x=>x.classList.remove('active')); document.querySelectorAll('.p').forEach(p=>p.classList.remove('active')); t.classList.add('active'); $(`panel-${t.dataset.t}`).classList.add('active'); }; }); }
 
 async function loadStatus(){ const r=await fetch('/api/status'); const d=await r.json(); $('#apSsid').textContent=d.ap_ssid; $('#apIp').textContent=d.ap_ip; $('#staSsid').textContent=d.sta_ssid; $('#staIp').textContent=d.sta_ip; const s=$('#staStatus'); s.textContent=d.sta_connected?'Connecté':'Déconnecté'; s.style.color=d.sta_connected?'#059669':'#dc2626'; }
 
 async function loadMdns(){ const r=await fetch('/api/mdns/status'); const d=await r.json(); $('#mdnsName').value=d.name; }
 
 async function loadCaps(){ const r=await fetch('/api/pins/caps'); caps=await r.json(); drawBoard(); }
 
 // Gestionnaires de formulaires
 function initForms(){
 // Formulaire mDNS
 $('#mdns').addEventListener('submit', async (e) => {
 e.preventDefault();
 const formData = new FormData();
 formData.append('name', $('#mdnsName').value);
 try {
 const r = await fetch('/api/mdns', { method: 'POST', body: formData });
 const d = await r.json();
 $('#mdnsMsg').textContent = d.status === 'ok' ? 'Nom enregistré' : 'Erreur: ' + d.error;
 $('#mdnsMsg').style.color = d.status === 'ok' ? '#059669' : '#dc2626';
 } catch (err) {
 $('#mdnsMsg').textContent = 'Erreur de connexion';
 $('#mdnsMsg').style.color = '#dc2626';
 }
 });
 
 // Formulaire STA
 $('#sta').addEventListener('submit', async (e) => {
 e.preventDefault();
 const formData = new FormData();
 formData.append('ssid', $('#ssid').value);
 formData.append('pass', $('#pass').value);
 try {
 const r = await fetch('/api/sta', { method: 'POST', body: formData });
 const d = await r.json();
 $('#staMsg').textContent = d.status === 'ok' ? 'Configuration enregistrée, redémarrage...' : 'Erreur: ' + d.error;
 $('#staMsg').style.color = d.status === 'ok' ? '#059669' : '#dc2626';
 if (d.status === 'ok') {
 setTimeout(() => location.reload(), 2000);
 }
 } catch (err) {
 $('#staMsg').textContent = 'Erreur de connexion';
 $('#staMsg').style.color = '#dc2626';
 }
 });
 
 // Formulaire OSC
 $('#osc').addEventListener('submit', async (e) => {
 e.preventDefault();
 const formData = new FormData();
 formData.append('target', $('#oscTarget').value);
 formData.append('port', $('#oscPort').value);
 try {
 const r = await fetch('/api/osc', { method: 'POST', body: formData });
 const d = await r.json();
 $('#oscMsg').textContent = d.status === 'ok' ? 'Configuration OSC enregistrée' : 'Erreur: ' + d.error;
 $('#oscMsg').style.color = d.status === 'ok' ? '#059669' : '#dc2626';
 } catch (err) {
 $('#oscMsg').textContent = 'Erreur de connexion';
 $('#oscMsg').style.color = '#dc2626';
 }
 });
 }
 
 function drawBoard(){ const L=$('#pinsLeft'),R=$('#pinsRight'); if(!L||!R)return; L.innerHTML=''; R.innerHTML=''; const RH=28; const COL={c1:20,c2:68,c4:238,c5:286}; const W=44,H=20;
 const mk=(x,y,w,h,fill,stroke,label,clk=true)=>{ const g=document.createElementNS('http://www.w3.org/2000/svg','g'); const r=document.createElementNS('http://www.w3.org/2000/svg','rect'); r.setAttribute('x',x); r.setAttribute('y',y); r.setAttribute('width',w); r.setAttribute('height',h); r.setAttribute('rx','4'); r.setAttribute('fill',fill); r.setAttribute('stroke',stroke); g.appendChild(r); const t=document.createElementNS('http://www.w3.org/2000/svg','text'); t.setAttribute('x',x+w/2); t.setAttribute('y',y+h/2+1); t.setAttribute('text-anchor','middle'); t.setAttribute('class','svg-t'); t.textContent=label; g.appendChild(t); if(clk){ g.style.cursor='pointer'; r.dataset.label=label; prect[label]=r; r.addEventListener('click',()=>{ if(window._selRect) window._selRect.classList.remove('selectedSquare'); window._selRect=r; r.classList.add('selectedSquare'); cur=label; $('#selPin').textContent=label; handlePinClick(label); updFunc(label); if(pcfg[cur]) applyCfg(pcfg[cur]); }); } return g; };
 const left=(row,tl,tc,dl)=>{ const y=30+row*RH; const f=document.createDocumentFragment(); f.appendChild(mk(COL.c1,y-10,W,H,tc,'#9ca3af',tl,true)); f.appendChild(mk(COL.c2,y-10,W,H,FC.DIGITAL,'#9ca3af',dl,true)); L.appendChild(f); };
 const rightPow=(row,label,color)=>{ const y=30+row*RH; R.appendChild(mk(COL.c4,y-10,W,H,color,'#9ca3af',label,false)); };
 const right=(row,dl,tl,tc)=>{ const y=30+row*RH; const f=document.createDocumentFragment(); f.appendChild(mk(COL.c4,y-10,W,H,FC.DIGITAL,'#9ca3af',dl,true)); f.appendChild(mk(COL.c5,y-10,W,H,tc,'#9ca3af',tl,true)); R.appendChild(f); };
 left(0,'A0',FC.ANALOG,'D0'); left(1,'A1',FC.ANALOG,'D1'); left(2,'A2',FC.ANALOG,'D2'); left(3,'A3',FC.ANALOG,'D3'); left(4,'SDA',FC.I2C,'D4'); left(5,'SCL',FC.I2C,'D5'); left(6,'TX',FC.UART,'D6'); rightPow(0,'5V',FC.POWER); rightPow(1,'GND',FC.GND); rightPow(2,'3V3',FC.POWER); right(3,'D10','MOSI',FC.SPI); right(4,'D9','MISO',FC.SPI); right(5,'D8','SCK',FC.SPI); right(6,'D7','RX',FC.UART);
 }
 
 function pType(lbl){ 
 if(lbl.startsWith('A')) return 'analog'; 
 if(['SDA','SCL','I2C'].includes(lbl)) return 'i2c'; 
 if(['MOSI','MISO','SCK','SPI'].includes(lbl)) return 'spi'; 
 if(['TX','RX'].includes(lbl)) return 'uart'; 
 return 'digital'; 
 }
 
 function stat(cfg, pinLabel){ 
 if(cfg.role==='Potentiomètre') return cfg.rtpEnabled ? `CC#${cfg.rtpCc||7}` : 'Raw';
 if(cfg.role==='Bouton') return cfg.rtpEnabled ? `Note ${cfg.rtpNote||60}` : 'Digital';
 if(cfg.role==='LED') return cfg.ledMode==='pwm' ? 'PWM' : 'On/Off';
 if(cfg.role==='I2C') return 'I2C';
 if(cfg.role==='SPI') return 'SPI';
 if(cfg.role==='UART') return pinLabel?.includes('TX') ? 'TX' : 'RX';
 return cfg.role||''; 
 }
 
 function updatePinsList(){ const pl=$('#pinsList'); if(!pl) return; pl.innerHTML=''; Object.keys(pcfg).forEach(lbl=>{ const cfg=pcfg[lbl]; if(!cfg||!cfg.role) return; const it=document.createElement('div'); it.className=`item ${pType(lbl)}`; it.innerHTML=`<span class="lbl">${lbl}</span><span class="role">${cfg.role}</span><span class="stat">${stat(cfg, lbl)}</span><button class="del-btn">×</button>`; it.onclick=()=>{ const r=prect[lbl]; if(r) r.dispatchEvent(new Event('click')); }; const delBtn=it.querySelector('.del-btn'); delBtn.onclick=(e)=>{ e.stopPropagation(); delete pcfg[lbl]; updatePinsList(); updateBusVisuals(); }; pl.appendChild(it); }); }
 
 function updateBusVisuals(){
 // Réinitialiser tous les états visuels
 Object.keys(prect).forEach(lbl=>{
 const r = prect[lbl];
 if(!r) return;
 r.classList.remove('busDisabled');
 });
 
 // I2C : griser SDA, SCL, D4, D5 quand ligne I2C existe
 if(pcfg['I2C']){
 ['SDA','SCL','D4','D5'].forEach(lbl=>{
 const r = prect[lbl];
 if(!r) return;
 r.classList.add('busDisabled');
 });
 }
 
 // SPI : griser MOSI, MISO, SCK, D8, D9, D10 quand ligne SPI existe
 if(pcfg['SPI']){
 ['MOSI','MISO','SCK','D8','D9','D10'].forEach(lbl=>{
 const r = prect[lbl];
 if(!r) return;
 r.classList.add('busDisabled');
 });
 }
 }
 
 function setOptions(sel,arr,pre=0){ if(!sel) return; sel.innerHTML=arr.map((o,i)=>`<option ${i===pre?'selected':''}>${o}</option>`).join(''); }
 function showRoleCards(role){ const b=$('#cardBtn'), l=$('#cardLed'), p=$('#cardPot'); if(b) b.style.display=(role==='Bouton')?'block':'none'; if(l) l.style.display=(role==='LED')?'block':'none'; if(p) p.style.display=(role==='Potentiomètre')?'block':'none'; }
 function updateRtpForRole(role){
 const rtpEnable = $('#rtpEnabled2');
 const rtpType = $('#rtpMsgType');
 const rtpParams = $('#rtpParams');
 let enabled = true;
 let types = [];
 if(role==='Potentiomètre'){
 types = ['Control Change','Pitch Bend','Aftertouch (Channel)','Note + vélocité','Note (balayage)'];
 } else if(role==='Bouton'){
 types = ['Note','Control Change','Program Change','Clock','Tap Tempo'];
 } else if(role==='LED'){
 types = ['Note','Control Change'];
 } else if(role==='I2C' || role==='SPI' || role==='UART' || role==='Analog in (raw)' || role==='Digital in/out'){
 enabled = false;
 } else if(!role){
 enabled = false;
 }
 if(rtpEnable){ rtpEnable.checked = enabled; rtpEnable.disabled = !enabled; }
 if(rtpType){
 if(enabled){ setOptions(rtpType, types); }
 rtpType.disabled = !enabled;
 }
 if(rtpParams){ rtpParams.style.display = enabled ? 'block' : 'none'; }
 if(enabled) updateRtpParamsVisibility();
 }

 function updateRtpParamsVisibility(){
 const typeSel = $('#rtpMsgType');
 const params = $('#rtpParams');
 const noteRow = $('#rtpNoteRow');
 const ccRow = $('#rtpCcRow');
 const ccOnOffRow = $('#rtpCcOnOffRow');
 const pcRow = $('#rtpPcRow');
 const velRow = $('#rtpVelRow');
 const ccRangeRow = $('#rtpCcRangeRow');
 const chanRow = $('#rtpChanRow');
 const clockHint = $('#rtpClockHint');
 const noteSweepRow = $('#rtpNoteSweepRow');
 const roleSel = $('#funcSelect');
 if(!typeSel || !params) return;
 const v = typeSel.value;
 [noteRow, ccRow, ccOnOffRow, pcRow, velRow, chanRow, clockHint, noteSweepRow, ccRangeRow].forEach(el=>{ if(el) el.style.display='none'; });
 params.style.display = 'block';
 if(v==='Note'){
 if(noteRow) noteRow.style.display='flex';
 if(chanRow) chanRow.style.display='flex';
 const role = roleSel ? roleSel.value : '';
 if(role==='Bouton' && velRow){ velRow.style.display='flex'; }
 } else if(v==='Control Change'){
 if(ccRow) ccRow.style.display='flex';
 if(chanRow) chanRow.style.display='flex';
 const role = roleSel ? roleSel.value : '';
 if(role==='Potentiomètre' && ccRangeRow){ ccRangeRow.style.display='flex'; }
 if(role==='Bouton' && ccOnOffRow){ ccOnOffRow.style.display='flex'; }
 } else if(v==='Program Change'){
 if(pcRow) pcRow.style.display='flex';
 if(chanRow) chanRow.style.display='flex';
 } else if(v==='Pitch Bend'){
 if(chanRow) chanRow.style.display='flex';
 } else if(v==='Aftertouch (Channel)'){
 if(chanRow) chanRow.style.display='flex';
 } else if(v==='Note + vélocité'){
 if(noteRow) noteRow.style.display='flex';
 if(chanRow) chanRow.style.display='flex';
 } else if(v==='Note (balayage)'){
 if(noteSweepRow) noteSweepRow.style.display='flex';
 if(chanRow) chanRow.style.display='flex';
 } else if(v==='Clock' || v==='Tap Tempo'){
 if(clockHint) clockHint.style.display='flex';
 }
 }

 function updFunc(lbl){ const sel=$('#funcSelect'); if(!sel) return; const isI2C=(lbl==='SDA'||lbl==='SCL'); const isSPI=(lbl==='MOSI'||lbl==='MISO'||lbl==='SCK'); const isUART=(lbl==='TX'||lbl==='RX'); if(/^A\d+$/.test(lbl)){ setOptions(sel,['Potentiomètre','Analog in (raw)'],0); } else if(/^D\d+$/.test(lbl) && !isI2C && !isSPI && !isUART){ setOptions(sel,['Bouton','LED','Digital in/out'],0); } else if(isI2C){ setOptions(sel,['I2C'],0); } else if(isSPI){ setOptions(sel,['SPI'],0); } else if(isUART){ setOptions(sel,['UART'],0); } else { setOptions(sel,[],0); } showRoleCards(sel.value||''); updateRtpForRole(sel.value||''); sel.onchange=()=>{ showRoleCards(sel.value||''); updateRtpForRole(sel.value||''); if(cur){ pcfg[cur]=readCfg(); updatePinsList(); updateBusVisuals(); } }; }
 
 // WebSocket pour synchronisation avec C++
 let websocket = null;
 
 // Fonction pour appliquer la logique de remplacement
 function applyPinReplacementLogic(pin) {
 // Pins analogiques
 if (pin.startsWith('A')) {
 // Supprimer le pin digital correspondant seulement s'il existe
 const dLabel = pin.replace('A', 'D');
 if (pcfg[dLabel]) delete pcfg[dLabel];
 }
 // Pins digitales normales
 else if (['D0','D1','D2','D3'].includes(pin)) {
 // Supprimer le pin analogique correspondant seulement s'il existe
 const aLabel = pin.replace('D', 'A');
 if (pcfg[aLabel]) delete pcfg[aLabel];
 }
 // Bus I2C - griser SDA,SCL,D4,D5 et supprimer les configs individuelles
 else if (['SDA','SCL'].includes(pin)) {
 // Supprimer les pins digitales individuelles pour libérer D4,D5
 if (pcfg['D4']) delete pcfg['D4'];
 if (pcfg['D5']) delete pcfg['D5'];
 // Supprimer les configs SDA/SCL individuelles
 if (pcfg['SDA']) delete pcfg['SDA'];
 if (pcfg['SCL']) delete pcfg['SCL'];
 }
 // Bus SPI - griser MOSI,MISO,SCK,D8,D9,D10 et supprimer les configs individuelles
 else if (['MOSI','MISO','SCK'].includes(pin)) {
 // Supprimer les pins digitales individuelles pour libérer D8,D9,D10
 if (pcfg['D8']) delete pcfg['D8'];
 if (pcfg['D9']) delete pcfg['D9'];
 if (pcfg['D10']) delete pcfg['D10'];
 // Supprimer les configs MOSI/MISO/SCK individuelles
 if (pcfg['MOSI']) delete pcfg['MOSI'];
 if (pcfg['MISO']) delete pcfg['MISO'];
 if (pcfg['SCK']) delete pcfg['SCK'];
 }
 // UART TX
 else if (pin === 'TX') {
 if (pcfg['D6']) delete pcfg['D6'];
 }
 // UART RX
 else if (pin === 'RX') {
 if (pcfg['D7']) delete pcfg['D7'];
 }
 // Pins digitales de bus I2C
 else if (['D4','D5'].includes(pin)) {
 if (pcfg['I2C']) delete pcfg['I2C'];
 }
 // Pins digitales de bus SPI
 else if (['D8','D9','D10'].includes(pin)) {
 if (pcfg['SPI']) delete pcfg['SPI'];
 }
 // Pins digitales UART - remplacement simple
 else if (pin === 'D6') {
 if (pcfg['TX']) delete pcfg['TX'];
 }
 else if (pin === 'D7') {
 if (pcfg['RX']) delete pcfg['RX'];
 }
 }
 
 function initWebSocket() {
 const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
 const wsUrl = `${protocol}//${window.location.host}/ws`;
 websocket = new WebSocket(wsUrl);
 
 websocket.onopen = function() {
 console.log('WebSocket connected');
 };
 
 websocket.onmessage = function(event) {
 const message = event.data;
 if (message.startsWith('PIN_CONFIG:')) {
 const parts = message.split(':');
 if (parts.length >= 3) {
 const pin = parts[1];
 const config = JSON.parse(parts.slice(2).join(':'));
 
 // Appliquer la logique de remplacement AVANT d'ajouter la config
 applyPinReplacementLogic(pin);
 
 // Appliquer la configuration reçue avec les bonnes clés
 if (['SDA','SCL'].includes(pin) && config.role === 'I2C') {
 pcfg['I2C'] = config; // Créer clé I2C pour le grisage
 } else if (['MOSI','MISO','SCK'].includes(pin) && config.role === 'SPI') {
 pcfg['SPI'] = config; // Créer clé SPI pour le grisage
 } else {
 pcfg[pin] = config; // Clé normale pour les autres pins
 }
 updatePinsList();
 updateBusVisuals();
 
 // Si c'est la pin actuellement sélectionnée, mettre à jour l'interface
 if (cur === pin) {
 applyCfg(config);
 }
 }
 }
 };
 
 websocket.onclose = function() {
 console.log('WebSocket disconnected');
 // Tentative de reconnexion après 3 secondes
 setTimeout(initWebSocket, 3000);
 };
 }
 
 function handlePinClick(label){
 // Vérifier si la pin est bloquée
 if(prect[label] && prect[label].classList.contains('busDisabled')){
 return;
 }
 
 // Envoyer message WebSocket au C++
 if (websocket && websocket.readyState === WebSocket.OPEN) {
 websocket.send(`PIN_CLICKED:${label}`);
 } else {
 // Fallback: logique locale si WebSocket pas disponible
 handlePinClickLocal(label);
 }
 }
 
 function handlePinClickLocal(label){
 // Pins analogiques
 if(label.startsWith('A')){
 pcfg[label] = {role: 'Potentiomètre', cfg: {}};
 // Supprimer le pin digital correspondant seulement s'il existe
 const dLabel = label.replace('A', 'D');
 if(pcfg[dLabel]) delete pcfg[dLabel];
 }
 // Pins digitales normales
 else if(['D0','D1','D2','D3'].includes(label)){
 pcfg[label] = {role: 'Bouton', cfg: {}};
 // Supprimer le pin analogique correspondant seulement s'il existe
 const aLabel = label.replace('D', 'A');
 if(pcfg[aLabel]) delete pcfg[aLabel];
 }
 // Bus I2C
 else if(['SDA','SCL'].includes(label)){
 pcfg['I2C'] = {role: 'I2C', cfg: {}};
 // Supprimer les pins digitales seulement si elles existent
 if(pcfg['D4']) delete pcfg['D4'];
 if(pcfg['D5']) delete pcfg['D5'];
 }
 // Bus SPI
 else if(['MOSI','MISO','SCK'].includes(label)){
 pcfg['SPI'] = {role: 'SPI', cfg: {}};
 // Supprimer les pins digitales seulement si elles existent
 if(pcfg['D8']) delete pcfg['D8'];
 if(pcfg['D9']) delete pcfg['D9'];
 if(pcfg['D10']) delete pcfg['D10'];
 }
 // UART TX
 else if(label === 'TX'){
 pcfg['TX'] = {role: 'UART', cfg: {}};
 if(pcfg['D6']) delete pcfg['D6'];
 }
 // UART RX
 else if(label === 'RX'){
 pcfg['RX'] = {role: 'UART', cfg: {}};
 if(pcfg['D7']) delete pcfg['D7'];
 }
 // Pins digitales de bus I2C
 else if(['D4','D5'].includes(label)){
 if(pcfg['I2C']) delete pcfg['I2C'];
 pcfg[label] = {role: 'Bouton', cfg: {}};
 }
 // Pins digitales de bus SPI
 else if(['D8','D9','D10'].includes(label)){
 if(pcfg['SPI']) delete pcfg['SPI'];
 pcfg[label] = {role: 'Bouton', cfg: {}};
 }
 // Pins digitales UART (indépendantes)
 else if(['D6','D7'].includes(label)){
 // D6 supprime TX, D7 supprime RX
 if(label === 'D6' && pcfg['TX']) delete pcfg['TX'];
 if(label === 'D7' && pcfg['RX']) delete pcfg['RX'];
 pcfg[label] = {role: 'Bouton', cfg: {}};
 }
 
 updatePinsList();
 updateBusVisuals();
 }
 
 function readCfg(){ const c={}; c.role=$('#funcSelect')?.value||''; c.btnMode=$('#btnMode')?.value||''; c.ledMode=$('#ledMode')?.value||''; c.potFilter=$('#potFilter')?.value||''; c.rtpEnabled=!!$('#rtpEnabled2')?.checked; c.rtpType=$('#rtpMsgType')?.value||''; c.rtpNote=$('#rtpNote')?.value||''; c.rtpCc=$('#rtpCc')?.value||''; c.rtpPc=$('#rtpPc')?.value||''; c.rtpChan=$('#rtpChan')?.value||''; c.rtpCcOn=$('#rtpCcOn')?.value||''; c.rtpCcOff=$('#rtpCcOff')?.value||''; c.rtpVel=$('#rtpVel')?.value||''; c.rtpCcMin=$('#rtpCcMin')?.value||''; c.rtpCcMax=$('#rtpCcMax')?.value||''; c.rtpNoteMin=$('#rtpNoteMin')?.value||''; c.rtpNoteMax=$('#rtpNoteMax')?.value||''; c.rtpNoteVelFix=$('#rtpNoteVelFix')?.value||''; c.oscEnabled=!!$('#oscEnabled2')?.checked; c.oscAddress=$('#oscAddress')?.value||''; c.dbgEnabled=!!$('#dbgEnabled')?.checked; c.dbgHeader=$('#dbgHeader')?.value||''; return c; }
 function applyCfg(c){ if(!c) return; const setV=(id,v)=>{ const el=$(id); if(el&&v!=null) el.value=v; }; const setC=(id,b)=>{ const el=$(id); if(el) el.checked=!!b; }; setV('funcSelect',c.role); showRoleCards(c.role); updateRtpForRole(c.role); setV('btnMode',c.btnMode); setV('ledMode',c.ledMode); setV('potFilter',c.potFilter); setC('rtpEnabled2',c.rtpEnabled); setV('rtpMsgType',c.rtpType); setV('rtpNote',c.rtpNote); setV('rtpCc',c.rtpCc); setV('rtpPc',c.rtpPc); setV('rtpChan',c.rtpChan); setV('rtpCcOn',c.rtpCcOn); setV('rtpCcOff',c.rtpCcOff); setV('rtpVel',c.rtpVel); setV('rtpCcMin',c.rtpCcMin); setV('rtpCcMax',c.rtpCcMax); setV('rtpNoteMin',c.rtpNoteMin); setV('rtpNoteMax',c.rtpNoteMax); setV('rtpNoteVelFix',c.rtpNoteVelFix); setC('oscEnabled2',c.oscEnabled); setV('oscAddress',c.oscAddress); setC('dbgEnabled',c.dbgEnabled); setV('dbgHeader',c.dbgHeader); updateRtpParamsVisibility(); }
 
 async function saveAll(){ const msg=$('#saveAllMsg'); msg.textContent='Enregistrement...'; try{ const ps=Object.keys(pcfg).map(async lbl=>{ const c=pcfg[lbl]; if(!c||!c.role) return; const p=new URLSearchParams(); p.set('pinLabel',lbl); p.set('role',c.role); if(c.rtpEnabled) p.set('rtpEnabled','true'); if(c.rtpType) p.set('rtpType',c.rtpType); if(c.rtpNote) p.set('rtpNote',c.rtpNote); if(c.rtpCc) p.set('rtpCc',c.rtpCc); if(c.rtpChan) p.set('rtpChan',c.rtpChan); if(c.ledMode) p.set('ledMode',c.ledMode); return fetch('/api/pins/set',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:p.toString()}); }); await Promise.all(ps); msg.textContent='Toutes les configurations enregistrées'; msg.style.color='#10b981'; }catch(e){ msg.textContent='Erreur lors de l\'enregistrement'; msg.style.color='#ef4444'; } }
 
 document.addEventListener('DOMContentLoaded', () => {
 initTabs();
 initForms();
 initWebSocket();
 loadStatus();
 loadMdns();
 loadCaps();
 setInterval(loadStatus, 5000);
 const btn=$('#saveAllBtn'); if(btn) btn.onclick=saveAll; 
 
 const fieldsToWatch=['funcSelect','btnMode','ledMode','potFilter','rtpEnabled2','rtpMsgType','rtpNote','rtpCc','rtpPc','rtpChan','rtpCcOn','rtpCcOff','rtpVel','rtpCcMin','rtpCcMax','rtpNoteMin','rtpNoteMax','rtpNoteVelFix','oscEnabled2','oscAddress','dbgEnabled','dbgHeader'];
 fieldsToWatch.forEach(id=>{
 const el=document.getElementById(id);
 if(el){
 el.addEventListener('change',()=>{ 
 if(cur){ 
 pcfg[cur]=readCfg(); 
 updatePinsList(); 
 updateBusVisuals();
 } 
 });
 el.addEventListener('input',()=>{ 
 if(cur){ 
 pcfg[cur]=readCfg(); 
 updatePinsList(); 
 updateBusVisuals();
 } 
 });
 }
 });
 
 
 const rtpType = $('#rtpMsgType');
 if(rtpType){ rtpType.onchange = updateRtpParamsVisibility; }
 });
 </script>
</head>
<body>
 <div class="c">
 <div class="h"><h1>ESP32 Server</h1><p>Configuration Wi‑Fi, RTP‑MIDI et OSC</p></div>
 <div class="t"><div class="tab active" data-t="status">Statut</div><div class="tab" data-t="connection">Connection</div><div class="tab" data-t="pins">Pins</div></div>
 <div class="p active" id="panel-status">
 <div class="g"><div class="card"><h3>Access Point</h3><p><strong>SSID:</strong> <span id="apSsid">-</span></p><p><strong>IP:</strong> <span id="apIp">-</span></p></div><div class="card"><h3>Station Wi‑Fi</h3><p><strong>SSID:</strong> <span id="staSsid">-</span></p><p><strong>IP:</strong> <span id="staIp">-</span></p><p><strong>Statut:</strong> <span id="staStatus">-</span></p></div></div>
 </div>
 <div class="p" id="panel-connection">
 <div class="f"><h3>Serveur</h3><form id="mdns"><div class="f"><label for="mdnsName">Nom du serveur</label><input type="text" id="mdnsName" placeholder="esp32rtpmidi" required><div class="hint"><small>Ce nom sera utilisé pour l'AP Wi‑Fi et http://nom.local</small></div></div><button type="submit" class="btn">Enregistrer</button><div class="hint" id="mdnsMsg"></div></form></div>
 <div class="f"><h3>OSC</h3><form id="osc"><div class="f"><label for="oscTarget">Destination</label><select id="oscTarget"><option value="ap">AP (192.168.4.1)</option><option value="sta" id="oscStaOption">STA</option></select></div><div class="f"><label for="oscPort">Port</label><input type="number" id="oscPort" value="8000" min="1024" max="65535" required></div><button type="submit" class="btn">Enregistrer</button><div class="hint" id="oscMsg"></div></form></div>
 <div class="f"><h3>Wi‑Fi Station</h3><form id="sta"><div class="f"><label for="ssid">SSID</label><input type="text" id="ssid" placeholder="Nom du réseau" required></div><div class="f"><label for="pass">Mot de passe</label><input type="password" id="pass" placeholder="Mot de passe"></div><button type="submit" class="btn">Connecter</button><div class="hint" id="staMsg"></div></form></div>
 </div>
 <div class="p" id="panel-pins">
 <div class="pl">
 <div class="lp">
 <h3 id="boardName">ESP32‑C3</h3>
 <div class="l"><span class="s digital"></span> Digital <span class="s analog"></span> Analog <span class="s i2c"></span> I2C <span class="s uart"></span> UART <span class="s spi"></span> SPI <span class="s power"></span> Power <span class="s gnd"></span> GND</div>
 <svg class="b" viewBox="50 -20 260 260"><rect x="114" y="20" width="122" height="188" rx="10" fill="#ffffff" stroke="#9ca3af"/><text x="174" y="114" text-anchor="middle" font-size="12" fill="#6b7280">MCU</text><rect x="144" y="2" width="60" height="60" rx="6" fill="#e5e7eb" stroke="#9ca3af"/><g id="pinsLeft"></g><g id="pinsRight"></g></svg>
 <div class="plist"><h4>Pins configurées</h4><div id="pinsList" class="list"></div><button id="saveAllBtn" class="btn-p">Enregistrer tout</button><div id="saveAllMsg" class="hint"></div></div>
 </div>
 <div class="rp"><div class="cp">
 <h4>Fonction du pin</h4>
 <div class="r"><label>Pin:</label><span id="selPin">-</span><select id="funcSelect"></select></div>
 <div id="cardBtn" class="subcard" style="display:none;"><div class="r"><label>Mode bouton:</label><select id="btnMode"><option value="pulse">Push</option><option value="press_release">Press/Release</option><option value="toggle">Toggle</option></select></div></div>
 <div id="cardLed" class="subcard" style="display:none;"><div class="r"><label>LED:</label><select id="ledMode"><option value="onoff">On/Off</option><option value="pwm">PWM</option></select></div></div>
 <div id="cardPot" class="subcard" style="display:none;"><div class="r"><label>Filtre:</label><select id="potFilter"><option value="none">Aucun</option><option value="lowpass">Passe-bas</option><option value="median">Médiane</option></select></div></div>
 <h4>RTP‑MIDI</h4>
 <div class="r switch"><input type="checkbox" id="rtpEnabled2"><label for="rtpEnabled2">Activer</label><label>Type:</label><select id="rtpMsgType"><option>Note</option><option>Control Change</option><option>Program Change</option><option>Pitch Bend</option><option>Aftertouch (Channel)</option><option>Note + vélocité</option><option>Note (balayage)</option><option>Clock</option><option>Tap Tempo</option></select></div>
 <div id="rtpParams" class="subcard" style="display:none;">
 <div class="r" id="rtpNoteRow" style="display:none;"><label>Note:</label><input type="number" id="rtpNote" min="0" max="127" placeholder="60" style="width:90px;"></div>
 <div class="r" id="rtpCcRow" style="display:none;"><label>CC#:</label><input type="number" id="rtpCc" min="0" max="127" placeholder="7" style="width:90px;"></div>
 <div class="r" id="rtpCcOnOffRow" style="display:none;"><label>Valeurs:</label><span>ON</span><input type="number" id="rtpCcOn" min="0" max="127" placeholder="127" style="width:90px;"><span>OFF</span><input type="number" id="rtpCcOff" min="0" max="127" placeholder="0" style="width:90px;"></div>
 <div class="r" id="rtpPcRow" style="display:none;"><label>Program#:</label><input type="number" id="rtpPc" min="0" max="127" placeholder="0" style="width:90px;"></div>
 <div class="r" id="rtpVelRow" style="display:none;"><label>Vélocité:</label><input type="number" id="rtpVel" min="1" max="127" placeholder="100" style="width:90px;"></div>
 <div class="r" id="rtpCcRangeRow" style="display:none;"><label>Plage MIDI:</label><input type="number" id="rtpCcMin" min="0" max="127" placeholder="0" style="width:90px;"><span>→</span><input type="number" id="rtpCcMax" min="0" max="127" placeholder="127" style="width:90px;"></div>
 <div class="r" id="rtpChanRow" style="display:none;"><label>Canal:</label><input type="number" id="rtpChan" min="1" max="16" placeholder="1" style="width:90px;"></div>
 <div class="r" id="rtpClockHint" style="display:none; color:#6b7280;"><span>Clock / Tap Tempo: pas de canal.</span></div>
 <div class="r" id="rtpNoteSweepRow" style="display:none;"><label>Balayage:</label><input type="number" id="rtpNoteMin" min="0" max="127" placeholder="48" style="width:90px;"><span>→</span><input type="number" id="rtpNoteMax" min="0" max="127" placeholder="72" style="width:90px;"><label style="margin-left:8px;">Vélocité fixe:</label><input type="number" id="rtpNoteVelFix" min="1" max="127" placeholder="100" style="width:90px;"></div>
 </div>
 <h4>OSC</h4>
 <div class="r switch"><input type="checkbox" id="oscEnabled2"><label for="oscEnabled2">Activer</label><label>Addr:</label><input type="text" id="oscAddress" placeholder="/ctl"></div>
 <h4>Debug</h4>
 <div class="r switch"><input type="checkbox" id="dbgEnabled"><label for="dbgEnabled">Activer</label><label>Hdr:</label><input type="text" id="dbgHeader" placeholder="[DBG]"></div>
 </div></div>
 </div>
 </div>
 </div>
</body>
</html>)rawliteral";
