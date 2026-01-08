function applyPinReplacementLogic(pin){
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
 if(label.startsWith('A') || label.startsWith('M')){
 pcfg[label] = {role: 'Potentiomètre'};
 if(label.startsWith('A')){
 const dLabel = label.replace('A', 'D');
 if(pcfg[dLabel]) delete pcfg[dLabel];
 }
 } else if(['D0','D1','D2','D3'].includes(label)){
 pcfg[label] = {role: 'Bouton'};
 const aLabel = label.replace('D', 'A');
 if(pcfg[aLabel]) delete pcfg[aLabel];
 } else if(['SDA','SCL'].includes(label)){
 pcfg['I2C'] = {role: 'I2C'};
 if(pcfg['D4']) delete pcfg['D4'];
 if(pcfg['D5']) delete pcfg['D5'];
 if(pcfg['SDA']) delete pcfg['SDA'];
 if(pcfg['SCL']) delete pcfg['SCL'];
 label = 'I2C';
 } else if(['MOSI','MISO','SCK'].includes(label)){
 pcfg['SPI'] = {role: 'SPI'};
 if(pcfg['D8']) delete pcfg['D8'];
 if(pcfg['D9']) delete pcfg['D9'];
 if(pcfg['D10']) delete pcfg['D10'];
 if(pcfg['MOSI']) delete pcfg['MOSI'];
 if(pcfg['MISO']) delete pcfg['MISO'];
 if(pcfg['SCK']) delete pcfg['SCK'];
 label = 'SPI';
 } else if(label === 'TX' || label === 'RX'){
 pcfg[label] = {role: 'UART'};
 if(label === 'TX' && pcfg['D6']) delete pcfg['D6'];
 if(label === 'RX' && pcfg['D7']) delete pcfg['D7'];
 } else if(['D4','D5'].includes(label)){
 if(pcfg['I2C']) delete pcfg['I2C'];
 pcfg[label] = {role: 'Bouton'};
 } else if(['D8','D9','D10'].includes(label)){
 if(pcfg['SPI']) delete pcfg['SPI'];
 pcfg[label] = {role: 'Bouton'};
 } else if(['D6','D7'].includes(label)){
 pcfg[label] = {role: 'Bouton'};
 if(label === 'D6' && pcfg['TX']) delete pcfg['TX'];
 if(label === 'D7' && pcfg['RX']) delete pcfg['RX'];
 } else if(/^D\d+$/.test(label)){
 pcfg[label] = {role: 'Bouton'};
 }
 updatePinsList();
 updateBusVisuals();
 const finalLabel = (label === 'I2C' || label === 'SPI') ? label : label;
 if(cur === finalLabel){
 updFunc(finalLabel);
 if(pcfg[finalLabel] || pcfg[cur]) applyCfg(pcfg[finalLabel] || pcfg[cur]);
 }
}

function handlePinClick(label){
 if(prect[label] && prect[label].classList.contains('busDisabled')){
 return;
 }
 
 
 if(websocket&&websocket.readyState===WebSocket.OPEN){
 websocket.send(`PIN_CLICKED:${label}`);
 } else{
 
 handlePinClickLocal(label);
 }
}

function updatePinsList(){
 const pl=$('#pinsList');
 if(!pl) return;
 pl.innerHTML='';
 Object.keys(pcfg).forEach(lbl=>{
 const cfg=pcfg[lbl];
 if(!cfg||!cfg.role) return;
 const isMuxPin=lbl.startsWith('M');
 if(isMuxPin) return;
 const it=document.createElement('div');
 it.className=`item ${pType(lbl)}`;
 it.innerHTML=`<span class="lbl">${lbl}</span><span class="role">${cfg.role}</span><span class="stat">${stat(cfg, lbl)}</span><button class="del-btn">×</button>`;
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
 if(pcfg[lbl]) applyCfg(pcfg[lbl]);
 };
 const delBtn=it.querySelector('.del-btn');
 if(delBtn) delBtn.onclick=(e)=>{
 e.stopPropagation();
 delete pcfg[lbl];
 updatePinsList();
 updateBusVisuals();
 };
 pl.appendChild(it);
 });
}

async function loadMuxList(){
 try{
 const r=await fetch('/api/mux/list');
 if(!r.ok){
 console.error('Erreur HTTP:',r.status,r.statusText);
 muxList=[];
 updateMuxListUI();
 updatePinsList();
 return;
 }
 const text=await r.text();
 if(!text||text.trim().length===0){
 console.warn('Réponse vide de /api/mux/list');
 muxList=[];
 updateMuxListUI();
 updatePinsList();
 return;
 }
 const d=JSON.parse(text);
 muxList=d.muxes||[];
 updateMuxListUI();
 updatePinsList();
 if(caps&&caps.pins) drawBoard();
 } catch(e){
 console.error('Erreur chargement mux:',e);
 muxList=[];
 updateMuxListUI();
 updatePinsList();
 if(caps&&caps.pins) drawBoard();
 }
}

function updateMuxListUI(){
 const list=$('#muxList');
 if(!list) return;
 list.innerHTML='';
 if(muxList.length===0){
 list.innerHTML='<p style="color:#6b7280;">Aucun multiplexeur configure.</p>';
 return;
 }
 muxList.forEach(m=>{
 const div=document.createElement('div');
 div.className='item';
 div.style.borderLeftColor='#8B5CF6';
 div.innerHTML=`<span class="lbl">MUX${m.id}</span><span class="role">HC4067</span><span class="stat">16 canaux</span><button class="del-btn" onclick="deleteMux(${m.id}, event)">x</button>`;
 div.onclick=(e)=>{
 if(e.target.classList.contains('del-btn')) return;
 showMuxForm(m.id);
 };
 list.appendChild(div);
 });
}

function showMuxForm(muxId=null){
 const overlay=$('#muxModalOverlay');
 if(overlay) overlay.classList.add('active');
 const idSel=$('#muxId');
 if(idSel&&muxId!==null){
 idSel.value=muxId;
 idSel.disabled=true;
 } else if(idSel){
 idSel.value='';
 idSel.disabled=false;
 }
 if(muxId!==null){
 const mux=muxList.find(m=>m.id==muxId);
 if(mux){
 if($('#muxPinGroup')&&mux.s0!==undefined){
 const firstD=getDFromGpio(mux.s0);
 if(firstD!==null) $('#muxPinGroup').value=firstD;
 }
 if($('#muxEn')) $('#muxEn').value=mux.en||255;
 if($('#muxCcBase')) $('#muxCcBase').value=mux.ccBase||1;
 if($('#muxMidiChan')) $('#muxMidiChan').value=mux.midiChan||1;
 if($('#muxOscBase')) $('#muxOscBase').value=mux.oscBase||'/mux'+muxId;
 if($('#muxMin')) $('#muxMin').value=mux.min!==undefined?mux.min:0;
 if($('#muxMax')) $('#muxMax').value=mux.max!==undefined?mux.max:4095;
 if($('#muxHysteresis')) $('#muxHysteresis').checked=mux.hysteresis!==undefined?(mux.hysteresis===1||mux.hysteresis===true):true;
 if($('#muxOscFormat')){
 const oscFormatValue=mux.oscFormat||'float';
 $('#muxOscFormat').value=oscFormatValue;
 }
 if($('#muxFilterIntensity')) $('#muxFilterIntensity').value=mux.filterIntensity!==undefined?mux.filterIntensity:5;
 }
 } else{
 if($('#muxSig')) $('#muxSig').value='';
 if($('#muxPinGroup')) $('#muxPinGroup').value='';
 if($('#muxEn')) $('#muxEn').value=255;
 if($('#muxCcBase')) $('#muxCcBase').value=1;
 if($('#muxMidiChan')) $('#muxMidiChan').value=1;
 if($('#muxOscBase')) $('#muxOscBase').value='/mux'+(idSel?idSel.value:'0');
 if($('#muxMin')) $('#muxMin').value=0;
 if($('#muxMax')) $('#muxMax').value=4095;
 if($('#muxHysteresis')) $('#muxHysteresis').checked=true;
 if($('#muxOscFormat')) $('#muxOscFormat').value='float';
 if($('#muxFilterIntensity')) $('#muxFilterIntensity').value=5;
 }
 populateMuxPinSelects();
 if(muxId!==null){
 const mux=muxList.find(m=>m.id==muxId);
 if(mux&&$('#muxSig')){
 $('#muxSig').value=mux.sig;
 }
 }
}

function hideMuxForm(){
 const overlay=$('#muxModalOverlay');
 if(overlay) overlay.classList.remove('active');
}

function populateMuxPinSelects(){
 if(!caps||!caps.pins) return;
 const idSel=$('#muxId');
 const currentMuxIdValue=idSel?parseInt(idSel.value):null;
 const currentMuxExists=currentMuxIdValue!==null&&muxList.find(m=>m.id===currentMuxIdValue);
 const currentMuxId=currentMuxExists?currentMuxIdValue:null;
 const usedGpiosForSigAndEn=getUsedGpios(['muxSig','muxEn']);
 const analogPins=caps.pins.filter(p=>p.label&&p.label.startsWith('A')&&p.caps&&p.caps.adc);
 let availableSig=analogPins.filter(p=>!usedGpiosForSigAndEn.has(p.gpio));
 if(currentMuxId!==null){
 const currentMux=muxList.find(m=>m.id===currentMuxId);
 if(currentMux&&currentMux.sig!==undefined&&currentMux.sig!==null){
 const currentSigPin=analogPins.find(p=>p.gpio===currentMux.sig);
 if(currentSigPin&&!availableSig.find(p=>p.gpio===currentMux.sig)){
 availableSig.push(currentSigPin);
 }
 }
 }
 const sigSel=$('#muxSig');
 if(sigSel){
 sigSel.innerHTML=availableSig.map(p=>`<option value="${p.gpio}">${p.label} (GPIO${p.gpio})</option>`).join('');
 const selectedMuxId=idSel?parseInt(idSel.value):null;
 if(selectedMuxId===0&&availableSig.find(p=>p.gpio===2)){
 sigSel.value=2;
 } else if(selectedMuxId===1&&availableSig.find(p=>p.gpio===3)){
 sigSel.value=3;
 } else if(availableSig.length>0){
 sigSel.value=availableSig[0].gpio;
 }
 }
 const allDPins=caps.pins.filter(p=>p.label&&p.label.startsWith('D')).sort((a,b)=>{
 const numA=parseInt(a.label.substring(1));
 const numB=parseInt(b.label.substring(1));
 return numA-numB;
 });
 const usedGpiosForGroups=getUsedGpios(['muxSig','muxEn']);
 const pinGroupSel=$('#muxPinGroup');
 if(pinGroupSel){
 const groups=[];
 for(let i=0;i<allDPins.length-3;i++){
 const d1=allDPins[i];
 const d2=allDPins[i+1];
 const d3=allDPins[i+2];
 const d4=allDPins[i+3];
 const num1=parseInt(d1.label.substring(1));
 const num2=parseInt(d2.label.substring(1));
 const num3=parseInt(d3.label.substring(1));
 const num4=parseInt(d4.label.substring(1));
 if(num2===num1+1&&num3===num2+1&&num4===num3+1){
 const allAvailable=[d1.gpio,d2.gpio,d3.gpio,d4.gpio].every(g=>!usedGpiosForGroups.has(g));
 if(allAvailable){
 groups.push({firstD:num1,label:`${d1.label}-${d4.label}`,gpios:[d1.gpio,d2.gpio,d3.gpio,d4.gpio]});
 }
 }
 }
 const currentValue=pinGroupSel.value;
 const currentValueValid=currentValue&&groups.find(g=>g.firstD===parseInt(currentValue));
 pinGroupSel.innerHTML='<option value="">Choisir...</option>'+groups.map(g=>`<option value="${g.firstD}">${g.label}</option>`).join('');
 if(currentValueValid){
 pinGroupSel.value=currentValue;
 } else{
 const selectedMuxId=idSel?parseInt(idSel.value):null;
 if(selectedMuxId===0&&groups.find(g=>g.firstD===3)){
 pinGroupSel.value=3;
 } else if(selectedMuxId===1&&groups.find(g=>g.firstD===7)){
 pinGroupSel.value=7;
 } else if(groups.length>0){
 pinGroupSel.value=groups[0].firstD;
 }
 }
 }
 const enSel=$('#muxEn');
 if(enSel){
 enSel.innerHTML='<option value="255">Non connecte</option>';
 const otherMuxUsesEn=muxList.some(m=>{
 if(currentMuxId!==null&&m.id==currentMuxId) return false;
 return m.en!==undefined&&m.en!==null&&m.en!==255;
 });
 if(!otherMuxUsesEn&&pinGroupSel&&pinGroupSel.value){
 const firstD=parseInt(pinGroupSel.value);
 const enD=firstD+4;
 const enPin=allDPins.find(p=>{
 const num=parseInt(p.label.substring(1));
 return num===enD&&!usedGpiosForSigAndEn.has(p.gpio);
 });
 if(enPin){
 enSel.innerHTML+=`<option value="${enPin.gpio}">${enPin.label} (GPIO${enPin.gpio})</option>`;
 }
 }
 if(!enSel.value||enSel.value===''){
 enSel.value=255;
 }
 }
 if(idSel){
 if(currentMuxId===null){
 const mux0=muxList.find(m=>m.id===0);
 const mux1=muxList.find(m=>m.id===1);
 const usedGpiosForMux1=getUsedGpios([]);
 const availableSigForMux1=analogPins.filter(p=>!usedGpiosForMux1.has(p.gpio));
 const availableMuxIds=[0,1].filter(id=>{
 if(id===0) return!mux0;
 if(id===1) return!mux1&&availableSigForMux1.length>0;
 return false;
 });
 idSel.innerHTML=availableMuxIds.map(id=>`<option value="${id}">MUX${id}</option>`).join('');
 if(availableMuxIds.length>0){
 idSel.value=availableMuxIds[0];
 }
 } else{
 idSel.value=currentMuxId;
 idSel.disabled=true;
 }
 }
 if(pinGroupSel&&!pinGroupSel.dataset.listener){
 pinGroupSel.dataset.listener='true';
 pinGroupSel.addEventListener('change',populateMuxPinSelects);
 }
 if(idSel&&!idSel.dataset.listenerMuxId){
 idSel.dataset.listenerMuxId='true';
 idSel.addEventListener('change',()=>{
 if($('#muxPinGroup')) $('#muxPinGroup').value='';
 if($('#muxSig')) $('#muxSig').value='';
 populateMuxPinSelects();
 });
 }
}

async function saveMux(e){
 e.preventDefault();
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
 const hysteresis=$('#muxHysteresis').checked;
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
 formData.append('hysteresis',hysteresis?'true':'false');
 formData.append('oscFormat',oscFormat);
 formData.append('filterIntensity',filterIntensity);
 try{
 const r=await fetch('/api/mux/add',{method:'POST',body:formData});
 const d=await r.json();
 if(d.status==='ok'){
 $('#muxMsg').textContent='Multiplexeur enregistre!';
 $('#muxMsg').style.color='#10b981';
 hideMuxForm();
 loadMuxList();
 loadConfiguredPins();
 loadCaps();
 } else{
 $('#muxMsg').textContent='Erreur: '+(d.error||'Inconnu');
 $('#muxMsg').style.color='#ef4444';
 }
 } catch(e){
 $('#muxMsg').textContent='Erreur reseau';
 $('#muxMsg').style.color='#ef4444';
 }
}

async function deleteMux(id,event){
 if(event) event.stopPropagation();
 if(!confirm('Supprimer le multiplexeur MUX'+id+'?')) return;
 const formData=new URLSearchParams();
 formData.append('id',id);
 try{
 await fetch('/api/mux/delete',{method:'POST',body:formData});
 await loadMuxList();
 loadCaps();
 const overlay=$('#muxModalOverlay');
 if(overlay&&overlay.classList.contains('active')){
 populateMuxPinSelects();
 }
 } catch(e){ console.log('Erreur suppression mux:',e); }
}

function initWebSocket(){
 const protocol=window.location.protocol==='https:'?'wss:':'ws:';
 const wsUrl=`${protocol}//${window.location.host}/ws`;
 websocket=new WebSocket(wsUrl);
 websocket.onopen=function(){
 console.log('WebSocket connected');
 };
 websocket.onmessage=function(event){
 const message=event.data;
 if(message.startsWith('PIN_CONFIG:')){
 const parts=message.split(':');
 if(parts.length>=3){
 const pin=parts[1];
 const config=JSON.parse(parts.slice(2).join(':'));
 applyPinReplacementLogic(pin);
 if(['SDA','SCL'].includes(pin)&&config.role==='I2C'){
 pcfg['I2C']=config;
 } else if(['MOSI','MISO','SCK'].includes(pin)&&config.role==='SPI'){
 pcfg['SPI']=config;
 } else{
 pcfg[pin]=config;
 }
 updatePinsList();
 updateBusVisuals();
 if(cur===pin){
 applyCfg(config);
 }
 }
 }
 };
 websocket.onclose=function(){
 console.log('WebSocket disconnected');
 setTimeout(initWebSocket,3000);
 };
}

function initMuxForm(){
 const form=$('#muxForm');
 if(form) form.onsubmit=saveMux;
}

document.addEventListener('DOMContentLoaded', ()=>{initTabs(); loadStatus(); loadMdns(); loadOscConfig(); loadStaConfig(); initForms(); initMuxForm(); initWebSocket(); loadCaps().then(()=>{drawBoard(); loadConfiguredPins(); loadMuxList();}); if($('#saveAllBtn')) $('#saveAllBtn').onclick=saveAll; setInterval(loadStatus, 5000);});
