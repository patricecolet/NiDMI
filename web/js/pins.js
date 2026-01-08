/* Fonctions de gestion des pins et affichage du board */

function pType(lbl){
 if(['TX','RX'].includes(lbl)) return 'uart';
 if(lbl.startsWith('A')) return 'analog';
 if(['SDA','SCL','I2C'].includes(lbl)) return 'i2c';
 if(['MOSI','MISO','SCK','SPI'].includes(lbl)) return 'spi';
 return 'digital';
}

function stat(cfg, pinLabel){
 if(cfg.role==='Potentiomètre') {
 if(!cfg.rtpEnabled) return 'Raw';
 if(cfg.rtpType === 'Control Change') return `CC#${cfg.rtpCc||7}`;
 if(cfg.rtpType === 'Program Change') return `PC#${cfg.rtpPc||0}`;
 if(cfg.rtpType === 'Pitch Bend') return 'Pitch Bend';
 if(cfg.rtpType === 'Aftertouch (Channel)') return 'Aftertouch';
 return cfg.rtpType || `CC#${cfg.rtpCc||7}`;
 }
 if(cfg.role==='Bouton') {
 if(!cfg.rtpEnabled) return 'Digital';
 if(cfg.rtpType === 'Note') return `Note ${cfg.rtpNote||60}`;
 if(cfg.rtpType === 'Control Change') return `CC#${cfg.rtpCc||7}`;
 if(cfg.rtpType === 'Program Change') return `PC#${cfg.rtpPc||0}`;
 if(cfg.rtpType === 'Note + vélocité') return `Note ${cfg.rtpNote||60} +vel`;
 if(cfg.rtpType === 'Note (balayage)') return `Note ${cfg.rtpNote||60} scan`;
 if(cfg.rtpType === 'Clock') return 'Clock';
 if(cfg.rtpType === 'Tap Tempo') return 'Tap Tempo';
 return cfg.rtpType || `Note ${cfg.rtpNote||60}`;
 }
 if(cfg.role==='LED') return cfg.ledMode==='pwm' ? 'PWM' : 'On/Off';
 if(cfg.role==='I2C') return 'I2C';
 if(cfg.role==='SPI') return 'SPI';
 if(cfg.role==='UART') return pinLabel?.includes('TX') ? 'TX' : 'RX';
 return cfg.role||'';
}

function setOptions(sel,arr,pre=0){
 if(!sel) return;
 sel.innerHTML=arr.map((o,i)=>`<option ${i===pre?'selected':''}>${o}</option>`).join('');
}

function updateBusVisuals(){
 Object.keys(prect).forEach(lbl=>{
 const r = prect[lbl];
 if(!r) return;
 r.classList.remove('busDisabled');
 });
 
 if(pcfg['I2C']){
 ['SDA','SCL','D4','D5'].forEach(lbl=>{
 const r = prect[lbl];
 if(!r) return;
 r.classList.add('busDisabled');
 });
 }
 
 if(pcfg['SPI']){
 ['MOSI','MISO','SCK','D8','D9','D10'].forEach(lbl=>{
 const r = prect[lbl];
 if(!r) return;
 r.classList.add('busDisabled');
 });
 }
}

function drawBoard(){
 const L=$('#pinsLeft'),R=$('#pinsRight');
 if(!L||!R||!caps||!caps.pins)return;
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
 if(window._selRect) window._selRect.classList.remove('selectedSquare');
 window._selRect=r;
 r.classList.add('selectedSquare');
 cur=label;
 $('#selPin').textContent=label;
 handlePinClick(label);
 updFunc(label);
 if(pcfg[cur]) applyCfg(pcfg[cur]);
 });
 }
 return g;
 };
 
 const getPinColor=(label)=>{
 if(['TX','RX'].includes(label))return FC.UART;
 if(label.startsWith('A'))return FC.ANALOG;
 if(['SDA','SCL'].includes(label))return FC.I2C;
 if(['MOSI','MISO','SCK'].includes(label))return FC.SPI;
 return FC.DIGITAL;
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
 const ps=gpioMap.get(gpio)||[];
 return ps.find(p=>['SDA','SCL','TX','RX','MOSI','MISO','SCK'].includes(p.label))?.label||'';
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
 
 const dPins=pins.filter(p=>p.label.startsWith('D')).sort((a,b)=>{
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
 
 const analogPins=pins.filter(p=>p.label.startsWith('A')&&p.caps.adc).sort((a,b)=>a.label.localeCompare(b.label));
 const i2cPins=pins.filter(p=>['SDA','SCL'].includes(p.label)).sort((a,b)=>a.label==='SDA'?-1:1);
 const uartPins=pins.filter(p=>['TX','RX'].includes(p.label)).sort((a,b)=>a.label==='TX'?-1:1);
 const digitalPins=pins.filter(p=>p.label.startsWith('D')&&!['SDA','SCL','MOSI','MISO','SCK','TX','RX'].some(bus=>pins.find(bp=>bp.label===bus&&bp.gpio===p.gpio))).sort((a,b)=>{
 const na=parseInt(a.label.substring(1));
 const nb=parseInt(b.label.substring(1));
 return na-nb;
 });
 
 const leftPins=[];
 const displayedGpios=new Set();
 let leftRow=0,rightRow=0;
 
 analogPins.forEach(p=>{
 leftPins.push({gpio:p.gpio,label:p.label,color:getPinColor(p.label),dLabel:getAlias(p.gpio,'D')});
 displayedGpios.add(p.gpio);
 });
 
 digitalPins.forEach(p=>{
 if(!displayedGpios.has(p.gpio)){
 leftPins.push({gpio:p.gpio,label:'',color:FC.DIGITAL,dLabel:p.label});
 displayedGpios.add(p.gpio);
 }
 });
 
 i2cPins.forEach(p=>{
 leftPins.push({gpio:p.gpio,label:p.label,color:getPinColor(p.label),dLabel:getAlias(p.gpio,'D')});
 displayedGpios.add(p.gpio);
 });
 
 const uartTx=uartPins.find(p=>p.label==='TX');
 if(uartTx){
 leftPins.push({gpio:uartTx.gpio,label:uartTx.label,color:getPinColor(uartTx.label),dLabel:getAlias(uartTx.gpio,'D')});
 displayedGpios.add(uartTx.gpio);
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
 
 const spiPins=pins.filter(p=>['MOSI','MISO','SCK'].includes(p.label)).sort((a,b)=>{
 const o={MOSI:0,MISO:1,SCK:2};
 return (o[a.label]||99)-(o[b.label]||99);
 });
 spiPins.forEach(p=>{
 R.appendChild(right(rightRow++,getAlias(p.gpio,'D'),p.label,getPinColor(p.label)));
 });
 
 const uartRx=pins.find(p=>p.label==='RX');
 if(uartRx){
 R.appendChild(right(rightRow++,getAlias(uartRx.gpio,'D'),uartRx.label,getPinColor(uartRx.label)));
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
