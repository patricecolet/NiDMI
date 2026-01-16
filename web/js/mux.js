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
 if(typeof updateBusVisuals === 'function') updateBusVisuals();
 } catch(e){
 console.error('Erreur chargement mux:',e);
 muxList=[];
 updateMuxListUI();
 updatePinsList();
 if(caps&&caps.pins) drawBoard();
 if(typeof updateBusVisuals === 'function') updateBusVisuals();
 }
}

function updateMuxListUI(){
 // Cette fonction n'est plus utilisée car les multiplexeurs sont maintenant dans la liste des pins
 // Gardée pour compatibilité mais ne fait rien
 // La liste est mise à jour via updatePinsList() dans pins.js
}

async function showMuxForm(muxId=null){
 // Vérifier si on est dans le panneau pins (componentFormCard visible)
 const componentFormCard=$('#componentFormCard');
 if(componentFormCard&&componentFormCard.style.display!=='none'){
  // On est déjà dans le panneau pins, juste charger la config
  if(muxId!==null){
   const mux=muxList.find(m=>m.id==muxId);
   if(mux&&typeof loadMuxConfigIntoForm === 'function'){
    loadMuxConfigIntoForm(mux);
   }
  }
  return;
 }
 
 // Sinon, utiliser la modal
 const overlay=$('#muxModalOverlay');
 const modalCard=$('#muxModalFormCard');
 if(!overlay || !modalCard) return;
 
 overlay.classList.add('active');
 modalCard.innerHTML = ''; // Vider le conteneur
 
 // Trouver la définition MUX (premier composant complexe trouvé)
 let muxDef = null;
 if(typeof componentDefinitions !== 'undefined' && componentDefinitions) {
  muxDef = componentDefinitions.find(def => def.isComplex);
 }
 
 if(!muxDef) {
  console.warn('[showMuxForm] Définition MUX non trouvée');
  return;
 }
 
 // Préparer la configuration actuelle
 const currentCfg = {};
 if(muxId!==null){
  const mux=muxList.find(m=>m.id==muxId);
  if(mux){
   // Mapper les champs MUX vers les IDs attendus
   currentCfg.muxId = mux.id;
   currentCfg.muxSig = mux.sig;
   currentCfg.muxS0 = mux.s0;
   currentCfg.muxS1 = mux.s1;
   currentCfg.muxS2 = mux.s2;
   currentCfg.muxS3 = mux.s3;
   currentCfg.muxEn = mux.en!==undefined?mux.en:255;
   currentCfg.muxMin = mux.min!==undefined?mux.min:0;
   currentCfg.muxMax = mux.max!==undefined?mux.max:4095;
   currentCfg.muxFilterIntensity = mux.filterIntensity!==undefined?mux.filterIntensity:5;
   currentCfg.rtpCc = mux.ccBase||1;
   currentCfg.rtpChan = mux.midiChan||1;
   currentCfg.rtpMsgType = 'Control Change';
   currentCfg.oscAddress = mux.oscBase||'/mux'+muxId;
   currentCfg.oscFormat = mux.oscFormat||'float';
   currentCfg.rtpEnabled2 = true;
   currentCfg.oscEnabled2 = true;
  }
 } else {
  // Valeurs par défaut pour nouveau MUX
  const idSel = muxId!==null ? muxId : 0;
  currentCfg.muxId = idSel;
  currentCfg.muxSig = '';
  currentCfg.muxEn = 255;
  currentCfg.muxCcBase = 1;
  currentCfg.muxMidiChan = 1;
  currentCfg.muxOscBase = '/mux'+idSel;
  currentCfg.muxMin = 0;
  currentCfg.muxMax = 4095;
  currentCfg.muxOscFormat = 'float';
  currentCfg.muxFilterIntensity = 5;
 }
 
 // Générer les champs de formulaire
 if(typeof generateFormFields === 'function' && muxDef.formFields && muxDef.formFields.length > 0) {
  generateFormFields(muxDef, 'muxModalFormCard', currentCfg);
 }
 
 // Générer les pins additionnelles
 if(typeof generateAdditionalPins === 'function' && muxDef.isComplex && muxDef.additionalPins && muxDef.additionalPins.length > 0) {
  generateAdditionalPins(muxDef, 'muxModalFormCard', currentCfg);
 }
 
 // Générer la section RTP-MIDI dans le modal
 const rtpSection = document.createElement('div');
 rtpSection.id = 'muxModalRtpSection';
 modalCard.appendChild(rtpSection);
 if(typeof generateRtpMidiSection === 'function') {
  generateRtpMidiSection(muxDef, currentCfg, 'muxModalRtpSection');
 }
 
 // Populate pins après un court délai pour que les selects soient créés
 setTimeout(() => {
  if(typeof populateMuxPinSelects === 'function') {
   populateMuxPinSelects();
  }
 }, 100);
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
 // Simplifié : montrer toutes les pins disponibles sans filtrage
 const analogPins=caps.pins.filter(p=>p.label&&p.label.startsWith('A')&&p.caps&&p.caps.adc);
 const sigSel=$('#muxSig');
 if(sigSel){
  sigSel.innerHTML=analogPins.map(p=>`<option value="${p.gpio}">${p.label} (GPIO${p.gpio})</option>`).join('');
  // Préserver la valeur actuelle si elle est valide
  const currentSigValue = sigSel.value;
  const currentSigValid = currentSigValue && analogPins.find(p=>p.gpio===parseInt(currentSigValue));
  if(currentSigValid){
   sigSel.value = currentSigValue;
  } else {
   const a0Pin = analogPins.find(p=>p.label==='A0');
   if(a0Pin){
    sigSel.value = a0Pin.gpio;
   } else if(analogPins.length>0){
    sigSel.value=analogPins[0].gpio;
   }
  }
 }
 const allDPins=typeof getAllDigitalPins === 'function' ? getAllDigitalPins() : [];
 const enSel=$('#muxEn');
 if(enSel){
  enSel.innerHTML='<option value="255">Non connecte</option>';
  allDPins.forEach(pin=>{
   enSel.innerHTML+=`<option value="${pin.gpio}">${pin.label} (GPIO${pin.gpio})</option>`;
  });
  if(!enSel.value||enSel.value===''){
   enSel.value=255;
  }
 }
 if(idSel){
 if(currentMuxId===null){
 const mux0=muxList.find(m=>m.id===0);
 const mux1=muxList.find(m=>m.id===1);
 const availableMuxIds=[0,1].filter(id=>{
 if(id===0) return!mux0;
 if(id===1) return!mux1;
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
 if(idSel&&!idSel.dataset.listenerMuxId){
 idSel.dataset.listenerMuxId='true';
 idSel.addEventListener('change',()=>{
 if($('#muxSig')) $('#muxSig').value='';
 populateMuxPinSelects();
 });
 }

 // Simplifié : montrer toutes les pins digitales sans filtrage
 const manualOptions=allDPins.map(p=>`<option value="${p.gpio}">${p.label} (GPIO${p.gpio})</option>`).join('');
 ['muxS0','muxS1','muxS2','muxS3'].forEach(id=>{
  const sel=$('#'+id);
  if(sel){
   const current=sel.value;
   sel.innerHTML=manualOptions;
   if(current&&allDPins.find(p=>p.gpio===parseInt(current))){
    sel.value=current;
   }
  }
 });
 const enManual=$('#muxEnManual');
 if(enManual){
  const current=enManual.value;
  enManual.innerHTML=`<option value="255">Non connecte</option>${manualOptions}`;
  if(current&&current!=='255'&&allDPins.find(p=>p.gpio===parseInt(current))){
   enManual.value=current;
  } else if(!enManual.value){
   enManual.value='255';
  }
 }
}

async function saveMux(e){
 e.preventDefault();
 const id=$('#muxId').value;
 const sig=parseInt($('#muxSig').value);
 const autoEnabled=typeof isMuxAutoPinsEnabled === 'function' ? isMuxAutoPinsEnabled() : true;
 let en=255;
 const ccBase=parseInt($('#muxCcBase').value)||1;
 const midiChan=parseInt($('#muxMidiChan').value)||1;
 const oscBase=$('#muxOscBase').value||'/mux'+id;
 if(!sig||isNaN(sig)){
 $('#muxMsg').textContent='Erreur: Veuillez choisir un pin analogique';
 $('#muxMsg').style.color='#ef4444';
 return;
 }
 let s0,s1,s2,s3;
 if(autoEnabled){
  const addrPins=calculateMuxAddressPins(sig);
  s0=addrPins.s0;
  s1=addrPins.s1;
  s2=addrPins.s2;
  s3=addrPins.s3;
  const enSel=$('#muxEn');
  en=enSel?enSel.value:255;
 } else {
  s0=parseInt($('#muxS0')?.value);
  s1=parseInt($('#muxS1')?.value);
  s2=parseInt($('#muxS2')?.value);
  s3=parseInt($('#muxS3')?.value);
  const enManual=$('#muxEnManual');
  if(enManual&&enManual.value&&enManual.value!=='255'){
   en=parseInt(enManual.value);
  }
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

function initMuxForm(){
 const form=$('#muxForm');
 if(form) form.onsubmit=saveMux;
}
