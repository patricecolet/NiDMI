/* Fonctions de gestion des composants et configurations */

function showRoleCards(role){
 const b=$('#cardBtn'), l=$('#cardLed'), p=$('#cardPot'), m=$('#cardMux');
 if(b) b.style.display=(role==='Bouton')?'block':'none';
 if(l) l.style.display=(role==='LED')?'block':'none';
 if(p) p.style.display=(role==='Potentiomètre')?'block':'none';
 if(m) m.style.display=(role==='Multiplexeur')?'block':'none';
}

function updFunc(lbl){
 const sel=$('#funcSelect');
 if(!sel) return;
 const isI2C=(lbl==='SDA'||lbl==='SCL');
 const isSPI=(lbl==='MOSI'||lbl==='MISO'||lbl==='SCK');
 const isUART=(lbl==='TX'||lbl==='RX');
 const isMuxPin=lbl.startsWith('M');
 if(/^A\d+$/.test(lbl)||isMuxPin){
 setOptions(sel,['Potentiomètre','Analog in (raw)','Multiplexeur'],0);
 } else if(/^D\d+$/.test(lbl) && !isI2C && !isSPI && !isUART){
 setOptions(sel,['Bouton','LED','Digital in/out'],0);
 } else if(isI2C){
 setOptions(sel,['I2C'],0);
 } else if(isSPI){
 setOptions(sel,['SPI'],0);
 } else if(isUART){
 setOptions(sel,['UART'],0);
 } else {
 setOptions(sel,[],0);
 }
 showRoleCards(sel.value||'');
 updateRtpForRole(sel.value||'');
 
 // Si Multiplexeur est sélectionné, initialiser le formulaire
 if(sel.value==='Multiplexeur'){
  initMuxFormForPin(lbl);
 }
 
 const updateConfig=()=>{
 showRoleCards(sel.value||'');
 updateRtpForRole(sel.value||'');
 updateBtnPulseTimingVisibility();
 if(cur){
 pcfg[cur]=readCfg();
 updatePinsList();
 updateBusVisuals();
 }
 };
 
 sel.onchange=updateConfig;
 
 const inputs=['#btnMode','#btnPulseTiming','#ledMode','#potFilter','#filterIntensity','#rtpEnabled2','#rtpMsgType','#rtpNote','#rtpCc','#rtpPc','#rtpChan','#rtpCcOn','#rtpCcOff','#rtpVel','#rtpCcMin','#rtpCcMax','#rtpNoteMin','#rtpNoteMax','#rtpNoteVelFix','#rtpNoteSweepAutoOffDelay','#oscEnabled2','#oscAddress','#oscFormat','#dbgEnabled','#dbgHeader'];
 inputs.forEach(id=>{
 const el=$(id);
 if(el) el.addEventListener('change',updateConfig);
 if(el) el.addEventListener('input',updateConfig);
 });
}

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
 } else if(role==='Multiplexeur'){
 enabled = false; // Le multiplexeur gère son propre MIDI/OSC
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

function updateBtnPulseTimingVisibility(){
 const btnMode = $('#btnMode');
 const pulseTimingRow = $('#btnPulseTimingRow');
 if(!btnMode || !pulseTimingRow) return;
 pulseTimingRow.style.display = (btnMode.value === 'pulse') ? 'flex' : 'none';
}

function readCfg(){
 const c={};
 c.role=$('#funcSelect')?.value||'';
 c.btnMode=$('#btnMode')?.value||'';
 c.btnPulseTiming=$('#btnPulseTiming')?.value||'';
 c.ledMode=$('#ledMode')?.value||'';
 c.potFilter=$('#potFilter')?.value||'';
 c.filterIntensity=$('#filterIntensity')?.value||'5';
 c.rtpEnabled=!!$('#rtpEnabled2')?.checked;
 c.rtpType=$('#rtpMsgType')?.value||'';
 c.rtpNote=$('#rtpNote')?.value||'';
 c.rtpCc=$('#rtpCc')?.value||'';
 c.rtpPc=$('#rtpPc')?.value||'';
 c.rtpChan=$('#rtpChan')?.value||'';
 c.rtpCcOn=$('#rtpCcOn')?.value||'';
 c.rtpCcOff=$('#rtpCcOff')?.value||'';
 c.rtpVel=$('#rtpVel')?.value||'';
 c.rtpCcMin=$('#rtpCcMin')?.value||'';
 c.rtpCcMax=$('#rtpCcMax')?.value||'';
 c.rtpNoteMin=$('#rtpNoteMin')?.value||'';
 c.rtpNoteMax=$('#rtpNoteMax')?.value||'';
 c.rtpNoteVelFix=$('#rtpNoteVelFix')?.value||'';
 c.rtpNoteSweepAutoOffDelay=$('#rtpNoteSweepAutoOffDelay')?.value||'';
 c.oscEnabled=!!$('#oscEnabled2')?.checked;
 c.oscAddress=$('#oscAddress')?.value||'';
 c.oscFormat=$('#oscFormat')?.value||'float';
 c.dbgEnabled=!!$('#dbgEnabled')?.checked;
 c.dbgHeader=$('#dbgHeader')?.value||'';
 return c;
}

function applyCfg(c){
 if(!c) return;
 const setV=(id,v)=>{
 const el=$(id);
 if(el&&v!=null) el.value=v;
 };
 const setC=(id,b)=>{
 const el=$(id);
 if(el) el.checked=!!b;
 };
 setV('funcSelect',c.role);
 showRoleCards(c.role);
 updateRtpForRole(c.role);
 setV('btnMode',c.btnMode);
 setV('btnPulseTiming',c.btnPulseTiming);
 updateBtnPulseTimingVisibility();
 setV('ledMode',c.ledMode);
 setV('potFilter',c.potFilter);
 setV('filterIntensity',c.filterIntensity||'5');
 setC('rtpEnabled2',c.rtpEnabled);
 setV('rtpMsgType',c.rtpType);
 setV('rtpNote',c.rtpNote);
 setV('rtpCc',c.rtpCc);
 setV('rtpPc',c.rtpPc);
 setV('rtpChan',c.rtpChan);
 setV('rtpCcOn',c.rtpCcOn);
 setV('rtpCcOff',c.rtpCcOff);
 setV('rtpVel',c.rtpVel);
 setV('rtpCcMin',c.rtpCcMin);
 setV('rtpCcMax',c.rtpCcMax);
 setV('rtpNoteMin',c.rtpNoteMin);
 setV('rtpNoteMax',c.rtpNoteMax);
 setV('rtpNoteVelFix',c.rtpNoteVelFix);
 setV('rtpNoteSweepAutoOffDelay',c.rtpNoteSweepAutoOffDelay);
 setC('oscEnabled2',c.oscEnabled);
 setV('oscAddress',c.oscAddress);
 setV('oscFormat',c.oscFormat);
 setC('dbgEnabled',c.dbgEnabled);
 setV('dbgHeader',c.dbgHeader);
 updateRtpParamsVisibility();
}

function getGpioFromD(dNum){
 if(!caps||!caps.pins) return null;
 const pin=caps.pins.find(p=>p.label===`D${dNum}`);
 return pin?pin.gpio:null;
}

function getDFromGpio(gpio){
 if(!caps||!caps.pins) return null;
 const pin=caps.pins.find(p=>p.gpio===gpio&&p.label&&p.label.startsWith('D'));
 return pin?parseInt(pin.label.substring(1)):null;
}

function getUsedGpios(additionalSelectIds=[]){
 const usedGpios=new Set();
 Object.keys(pcfg).forEach(lbl=>{
  const pin=caps.pins.find(p=>p.label===lbl);
  if(pin) usedGpios.add(pin.gpio);
 });
 const currentMuxId=$('#muxId')?parseInt($('#muxId').value):null;
 if(typeof muxList !== 'undefined' && Array.isArray(muxList)){
  muxList.forEach(m=>{
   if(currentMuxId!==null&&m.id==currentMuxId) return;
   if(m.sig!==undefined&&m.sig!==null) usedGpios.add(m.sig);
   if(m.s0!==undefined&&m.s0!==null) usedGpios.add(m.s0);
   if(m.s1!==undefined&&m.s1!==null) usedGpios.add(m.s1);
   if(m.s2!==undefined&&m.s2!==null) usedGpios.add(m.s2);
   if(m.s3!==undefined&&m.s3!==null) usedGpios.add(m.s3);
   if(m.en!==undefined&&m.en!==null&&m.en!==255) usedGpios.add(m.en);
  });
 }
 additionalSelectIds.forEach(id=>{
 const sel=$('#'+id);
 if(!sel||!sel.value||sel.value==='255') return;
 if(id==='muxPinGroup'){
 const firstD=parseInt(sel.value);
 const s0=getGpioFromD(firstD);
 const s1=getGpioFromD(firstD+1);
 const s2=getGpioFromD(firstD+2);
 const s3=getGpioFromD(firstD+3);
 if(s0) usedGpios.add(s0);
 if(s1) usedGpios.add(s1);
 if(s2) usedGpios.add(s2);
 if(s3) usedGpios.add(s3);
 } else{
 usedGpios.add(parseInt(sel.value));
 }
 });
 return usedGpios;
}

// Nouvelle fonction pour initialiser le formulaire multiplexeur depuis un pin
function initMuxFormForPin(pinLabel){
 if(!caps||!caps.pins) return;
 const pin=caps.pins.find(p=>p.label===pinLabel);
 if(!pin) return;
 
 // Trouver un multiplexeur existant qui utilise ce pin comme SIG, ou créer un nouveau
 const existingMux=muxList.find(m=>m.sig===pin.gpio);
 if(existingMux){
  // Charger la configuration existante
  loadMuxConfigIntoForm(existingMux);
 } else {
  // Nouveau multiplexeur - initialiser avec des valeurs par défaut
  if(typeof populateMuxPinSelects === 'function') populateMuxPinSelects();
  if($('#muxSig')) $('#muxSig').value=pin.gpio;
  if($('#muxId')){
   // Trouver le premier ID disponible
   const usedIds=muxList.map(m=>m.id);
   const availableId=[0,1].find(id=>!usedIds.includes(id));
   if(availableId!==undefined) $('#muxId').value=availableId;
  }
 }
}

function loadMuxConfigIntoForm(mux){
 if($('#muxId')) $('#muxId').value=mux.id;
 if($('#muxSig')) $('#muxSig').value=mux.sig;
 if($('#muxPinGroup')&&mux.s0!==undefined){
  const firstD=getDFromGpio(mux.s0);
  if(firstD!==null) $('#muxPinGroup').value=firstD;
 }
 if($('#muxEn')) $('#muxEn').value=mux.en||255;
 if($('#muxCcBase')) $('#muxCcBase').value=mux.ccBase||1;
 if($('#muxMidiChan')) $('#muxMidiChan').value=mux.midiChan||1;
 if($('#muxOscBase')) $('#muxOscBase').value=mux.oscBase||'/mux'+mux.id;
 if($('#muxMin')) $('#muxMin').value=mux.min!==undefined?mux.min:0;
 if($('#muxMax')) $('#muxMax').value=mux.max!==undefined?mux.max:4095;
 if($('#muxOscFormat')){
  const oscFormatValue=mux.oscFormat||'float';
  $('#muxOscFormat').value=oscFormatValue;
 }
 if($('#muxFilterIntensity')) $('#muxFilterIntensity').value=mux.filterIntensity!==undefined?mux.filterIntensity:5;
 if(typeof populateMuxPinSelects === 'function') populateMuxPinSelects();
}

async function saveMuxFromPin(){
 const id=$('#muxId').value;
 const sig=$('#muxSig').value;
 const pinGroup=parseInt($('#muxPinGroup').value);
 const en=$('#muxEn').value;
 const ccBase=parseInt($('#muxCcBase').value)||1;
 const midiChan=parseInt($('#muxMidiChan').value)||1;
 const oscBase=$('#muxOscBase').value||'/mux'+id;
 if(!pinGroup){
 $('#muxMsg').textContent='Erreur: Veuillez choisir un groupe de pins';
 $('#muxMsg').style.color='#ef4444';
 return;
 }
 const s0=getGpioFromD(pinGroup);
 const s1=getGpioFromD(pinGroup+1);
 const s2=getGpioFromD(pinGroup+2);
 const s3=getGpioFromD(pinGroup+3);
 if(!s0||!s1||!s2||!s3){
 $('#muxMsg').textContent='Erreur: Groupe de pins invalide';
 $('#muxMsg').style.color='#ef4444';
 return;
 }
 const min=parseInt($('#muxMin').value)||0;
 const max=parseInt($('#muxMax').value)||4095;
 const oscFormat=$('#muxOscFormat').value||'float';
 const filterIntensity=parseInt($('#muxFilterIntensity').value)||5;
 const formData=new URLSearchParams();
 formData.append('id',id);
 formData.append('sig',sig);
 formData.append('s0',s0);
 formData.append('s1',s1);
 formData.append('s2',s2);
 formData.append('s3',s3);
 formData.append('en',en);
 formData.append('ccBase',ccBase);
 formData.append('midiChan',midiChan);
 formData.append('oscBase',oscBase);
 formData.append('min',min);
 formData.append('max',max);
 formData.append('oscFormat',oscFormat);
 formData.append('filterIntensity',filterIntensity);
 try{
 const r=await fetch('/api/mux/add',{method:'POST',body:formData});
 const d=await r.json();
 if(d.status==='ok'){
 $('#muxMsg').textContent='Multiplexeur enregistré!';
 $('#muxMsg').style.color='#10b981';
 if(typeof loadMuxList === 'function') await loadMuxList();
 if(typeof loadCaps === 'function') await loadCaps();
 if(typeof updatePinsList === 'function') updatePinsList();
 if(typeof updateBusVisuals === 'function') updateBusVisuals();
 } else{
 $('#muxMsg').textContent='Erreur: '+(d.error||'Inconnu');
 $('#muxMsg').style.color='#ef4444';
 }
 } catch(e){
 $('#muxMsg').textContent='Erreur réseau';
 $('#muxMsg').style.color='#ef4444';
 }
}
