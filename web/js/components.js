/* Fonctions de gestion des composants et configurations */

function showRoleCards(role){
 const b=$('#cardBtn'), l=$('#cardLed'), p=$('#cardPot');
 if(b) b.style.display=(role==='Bouton')?'block':'none';
 if(l) l.style.display=(role==='LED')?'block':'none';
 if(p) p.style.display=(role==='Potentiomètre')?'block':'none';
}

function updFunc(lbl){
 const sel=$('#funcSelect');
 if(!sel) return;
 const isI2C=(lbl==='SDA'||lbl==='SCL');
 const isSPI=(lbl==='MOSI'||lbl==='MISO'||lbl==='SCK');
 const isUART=(lbl==='TX'||lbl==='RX');
 const isMuxPin=lbl.startsWith('M');
 if(/^A\d+$/.test(lbl)||isMuxPin){
 setOptions(sel,['Potentiomètre','Analog in (raw)'],0);
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
 muxList.forEach(m=>{
 if(currentMuxId!==null&&m.id==currentMuxId) return;
 if(m.sig!==undefined&&m.sig!==null) usedGpios.add(m.sig);
 if(m.s0!==undefined&&m.s0!==null) usedGpios.add(m.s0);
 if(m.s1!==undefined&&m.s1!==null) usedGpios.add(m.s1);
 if(m.s2!==undefined&&m.s2!==null) usedGpios.add(m.s2);
 if(m.s3!==undefined&&m.s3!==null) usedGpios.add(m.s3);
 if(m.en!==undefined&&m.en!==null&&m.en!==255) usedGpios.add(m.en);
 });
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
