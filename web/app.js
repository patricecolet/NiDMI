/* Met à jour la liste des pins configurées dans l'interface */
function updatePinsList(){
 const pl=$('#pinsList');
 if(!pl) return;
 /* Vider la liste actuelle */
 pl.innerHTML='';
 /* Parcourir toutes les pins configurées */
 Object.keys(pcfg).forEach(lbl=>{
  const cfg=pcfg[lbl];
  /* Ignorer les pins sans rôle configuré */
  if(!cfg||!cfg.role) return;
  /* Ignorer les pins de multiplexeur (elles sont gérées séparément) */
  const isMuxPin=lbl.startsWith('M');
  if(isMuxPin) return;
  /* Créer un élément DOM pour cette pin */
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
 
 // Ajouter les multiplexeurs configurés à la liste
 if(typeof muxList !== 'undefined' && Array.isArray(muxList)){
 muxList.forEach(mux=>{
 const it=document.createElement('div');
 it.className='item mux';
 it.innerHTML=`<span class="lbl">MUX${mux.id}</span><span class="role">HC4067</span><span class="stat">16 canaux</span><button class="del-btn">×</button>`;
 it.onclick=()=>{
 // Trouver le pin SIG correspondant et le sélectionner
 if(caps&&caps.pins&&mux.sig!==undefined){
  const sigPin=caps.pins.find(p=>p.gpio===mux.sig);
  if(sigPin&&sigPin.label){
   // Sélectionner le pin dans l'interface
   if(window._selRect) window._selRect.classList.remove('selectedSquare');
   const r=prect[sigPin.label];
   if(r){
    window._selRect=r;
    r.classList.add('selectedSquare');
   }
   cur=sigPin.label;
   $('#selPin').textContent=sigPin.label;
   // Mettre à jour le menu déroulant pour afficher "Multiplexeur"
   if($('#funcSelect')){
    $('#funcSelect').value='Multiplexeur';
    if(typeof updFunc === 'function') updFunc(sigPin.label);
   }
   // Charger la configuration du multiplexeur
   if(typeof loadMuxConfigIntoForm === 'function') loadMuxConfigIntoForm(mux);
  }
 }
 };
 const delBtn=it.querySelector('.del-btn');
 if(delBtn) delBtn.onclick=(e)=>{
 e.stopPropagation();
 if(typeof deleteMux === 'function') deleteMux(mux.id, e);
 };
 pl.appendChild(it);
 });
 }
}

/* Initialisation principale quand le DOM est prêt */
document.addEventListener('DOMContentLoaded', ()=>{
 /* Initialiser la navigation par onglets */
 initTabs();
 /* Charger l'état du serveur */
 loadStatus();
 /* Charger la configuration mDNS */
 loadMdns();
 /* Charger la configuration OSC */
 loadOscConfig();
 /* Charger la configuration Wi-Fi Station */
 loadStaConfig();
 /* Initialiser les formulaires */
 initForms();
 /* Initialiser le formulaire de multiplexeur */
 initMuxForm();
 /* Charger les capacités de la carte, puis dessiner le board */
 loadCaps().then(()=>{
  /* Dessiner le board SVG avec les pins */
  drawBoard();
  /* Charger les pins déjà configurées */
  loadConfiguredPins();
  /* Charger la liste des multiplexeurs */
  loadMuxList();
 });
 /* Initialiser le bouton "Enregistrer tout" */
 if($('#saveAllBtn')) $('#saveAllBtn').onclick=saveAll;
 /* Initialiser WebSocket avec un délai pour éviter les conflits */
 setTimeout(()=>{
  initWebSocket();
 }, 2000);
 /* Rafraîchir l'état du serveur toutes les 10 secondes */
 setInterval(loadStatus, 10000);
});
