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

document.addEventListener('DOMContentLoaded', ()=>{initTabs(); loadStatus(); loadMdns(); loadOscConfig(); loadStaConfig(); initForms(); initMuxForm(); initWebSocket(); loadCaps().then(()=>{drawBoard(); loadConfiguredPins(); loadMuxList();}); if($('#saveAllBtn')) $('#saveAllBtn').onclick=saveAll; setInterval(loadStatus, 5000);});
