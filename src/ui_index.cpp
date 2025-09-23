#include "ui_index.h"

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Server</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #f3f4f6; color: #111827; }
        .container { max-width: 1200px; margin: 0 auto; padding: 20px; }
        .header { text-align: center; margin-bottom: 30px; }
        .header h1 { color: #1f2937; margin-bottom: 10px; }
        .header p { color: #6b7280; }
        
        .tabs { display: flex; background: white; border-radius: 8px; box-shadow: 0 1px 3px rgba(0,0,0,0.1); margin-bottom: 20px; }
        .tab { flex: 1; padding: 15px; text-align: center; cursor: pointer; border-bottom: 3px solid transparent; transition: all 0.2s; }
        .tab.active { border-bottom-color: #3b82f6; color: #3b82f6; font-weight: 600; }
        .tab:hover:not(.active) { background: #f9fafb; }
        
        .panel { display: none; background: white; border-radius: 8px; padding: 25px; box-shadow: 0 1px 3px rgba(0,0,0,0.1); }
        .panel.active { display: block; }
        
        .form-group { margin-bottom: 20px; }
        .form-group label { display: block; margin-bottom: 8px; font-weight: 500; color: #374151; }
        .form-group input, .form-group select { width: 100%; padding: 12px; border: 1px solid #d1d5db; border-radius: 6px; font-size: 14px; }
        .form-group input:focus, .form-group select:focus { outline: none; border-color: #3b82f6; box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1); }
        
        .toggle { display: flex; align-items: center; gap: 10px; margin-bottom: 20px; }
        .toggle input[type="checkbox"] { width: 20px; height: 20px; }
        .status { padding: 4px 12px; border-radius: 20px; font-size: 12px; font-weight: 500; }
        .status.enabled { background: #dcfce7; color: #166534; }
        .status.disabled { background: #fee2e2; color: #991b1b; }
        
        button { background: #3b82f6; color: white; border: none; padding: 12px 24px; border-radius: 6px; cursor: pointer; font-size: 14px; font-weight: 500; transition: background 0.2s; }
        button:hover { background: #2563eb; }
        button:disabled { background: #9ca3af; cursor: not-allowed; }
        
        .hint { margin-top: 10px; color: #6b7280; font-size: 14px; }
        .hint small { display: block; margin-top: 5px; }
        
        details { margin-top: 15px; }
        details summary { cursor: pointer; font-weight: 500; color: #374151; }
        details[open] summary { margin-bottom: 10px; }
        
        .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; }
        .info-card { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 6px; padding: 15px; }
        .info-card h3 { color: #1f2937; margin-bottom: 10px; font-size: 16px; }
        .info-card p { color: #6b7280; font-size: 14px; margin-bottom: 5px; }
        
        .board { width: 100%; height: 240px; border: 1px solid #e5e7eb; border-radius: 8px; background: #f9fafb; }
        .pin { fill: #e5e7eb; stroke: #9ca3af; cursor: pointer; }
        .pin:hover { fill: #d1d5db; }
        .pin.selected { fill: #3b82f6; stroke: #1d4ed8; }
        /* Légende/badges fonctions */
        .legend { display:flex; gap:14px; align-items:center; margin:10px 0 8px; flex-wrap:wrap; }
        .swatch { width:14px; height:14px; border-radius:3px; display:inline-block; margin-right:6px; }
        .digital { background:#3B82F6; } /* bleu */
        .analog  { background:#EC4899; } /* rose */
        .i2c     { background:#10B981; } /* vert */
        .uart    { background:#6B7280; } /* gris */
        .spi     { background:#8B5CF6; } /* violet */
        .power   { background:#EF4444; } /* rouge */
        .gnd     { background:#000000; } /* noir */
        /* Styles SVG textes */
        .square-text { font-size:9px; fill:#ffffff; dominant-baseline:middle; pointer-events:none; user-select:none; }
        .square-outline { stroke:#9ca3af; }
        .selectedSquare { stroke:#1d4ed8; stroke-width:2; }
        /* Bus (I2C/SPI/UART) - visuel pins désactivés par bus */
        .busDisabled { opacity: 0.45; filter: grayscale(100%); }

        /* Disposition Pins/Config */
        .pins-layout { display:flex; gap:20px; align-items:flex-start; }
        .left-pane  { flex:0 0 30%; min-width:300px; }
        .right-pane { flex:1 1 auto; }
        .config-panel { background:#fff; border:1px solid #e5e7eb; border-radius:8px; padding:16px; }
        .config-panel h4 { margin:6px 0 10px; font-size:15px; color:#1f2937; }
        .row { display:flex; gap:12px; align-items:center; margin:8px 0; flex-wrap:wrap; }
        .row label { color:#374151; font-size:14px; }
        .row select, .row input[type="text"] { padding:8px 10px; border:1px solid #d1d5db; border-radius:6px; }
        .switch { display:flex; align-items:center; gap:8px; }
        .subcard { background:#f9fafb; border:1px solid #e5e7eb; border-radius:8px; padding:12px; margin-top:8px; }
        .subcard .row { margin:6px 0; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>ESP32 Server</h1>
            <p>Configuration Wi-Fi, RTP-MIDI et OSC</p>
        </div>
        
        <div class="tabs">
            <div class="tab active" data-t="status">Statut</div>
            <div class="tab" data-t="connection">Connection</div>
            <div class="tab" data-t="pins">Pins</div>
        </div>
        
        <!-- Panel Statut -->
        <div class="panel active" id="panel-status">
            <div class="grid">
                <div class="info-card">
                    <h3>Access Point</h3>
                    <p><strong>SSID:</strong> <span id="apSsid">-</span></p>
                    <p><strong>IP:</strong> <span id="apIp">-</span></p>
                </div>
                <div class="info-card">
                    <h3>Station Wi-Fi</h3>
                    <p><strong>SSID:</strong> <span id="staSsid">-</span></p>
                    <p><strong>IP:</strong> <span id="staIp">-</span></p>
                    <p><strong>Statut:</strong> <span id="staStatus">-</span></p>
                </div>
            </div>
        </div>
        
        <!-- Panel Connection -->
        <div class="panel" id="panel-connection">
            <!-- RTP-MIDI -->
            <div class="form-group">
                <h3>RTP-MIDI</h3>
                <div class="toggle">
                    <input type="checkbox" id="rtpEnabled">
                    <label for="rtpEnabled">Activer RTP-MIDI</label>
                    <span class="status disabled" id="rtpStatus">Désactivé</span>
                </div>
                <form id="rtp">
                    <div class="form-group">
                        <label for="rtpName">Nom du périphérique</label>
                        <input type="text" id="rtpName" placeholder="ESP32-Studio" required>
                    </div>
                    <div class="form-group">
                        <label for="rtpTarget">Destination</label>
                        <select id="rtpTarget">
                            <option value="ap">AP (192.168.4.1)</option>
                            <option value="sta" id="rtpStaOption">STA</option>
                        </select>
                    </div>
                    <button type="submit">Enregistrer</button>
                    <div class="hint" id="rtpMsg"></div>
                </form>
            </div>
            
            <!-- OSC -->
            <div class="form-group">
                <h3>OSC</h3>
                <form id="osc">
                    <div class="form-group">
                        <label for="oscTarget">Destination</label>
                        <select id="oscTarget">
                            <option value="ap">AP (192.168.4.1)</option>
                            <option value="sta" id="oscStaOption">STA</option>
                        </select>
                    </div>
                    <div class="form-group">
                        <label for="oscPort">Port</label>
                        <input type="number" id="oscPort" value="8000" min="1024" max="65535" required>
                    </div>
                    <button type="submit">Enregistrer</button>
                    <div class="hint" id="oscMsg"></div>
                </form>
            </div>
            
            <!-- Wi-Fi STA -->
            <div class="form-group">
                <h3>Wi-Fi Station</h3>
                <form id="sta">
                    <div class="form-group">
                        <label for="ssid">SSID</label>
                        <input type="text" id="ssid" placeholder="Nom du réseau" required>
                    </div>
                    <div class="form-group">
                        <label for="pass">Mot de passe</label>
                        <input type="password" id="pass" placeholder="Mot de passe">
                    </div>
                    <button type="submit">Connecter</button>
                    <div class="hint" id="staMsg"></div>
                </form>
            </div>
        </div>
        
        <!-- Panel Pins -->
        <div class="panel" id="panel-pins">
            <div class="pins-layout">
                <div class="left-pane">
                    <h3 id="boardName">ESP32-C3</h3>
                    <div class="legend">
                        <span class="swatch digital"></span> Digital
                        <span class="swatch analog"></span> Analog
                        <span class="swatch i2c"></span> I2C
                        <span class="swatch uart"></span> UART
                        <span class="swatch spi"></span> SPI
                        <span class="swatch power"></span> Power
                        <span class="swatch gnd"></span> GND
                    </div>
                    <svg class="board" viewBox="50 -20 260 260">
                        <!-- Corps MCU au centre (colonne 3) aligné sur la hauteur des 7 rangées -->
                        <rect x="114" y="20" width="122" height="188" rx="10" fill="#ffffff" stroke="#9ca3af"/>
                        <text x="174" y="114" text-anchor="middle" font-size="12" fill="#6b7280">MCU</text>
                        <!-- USB-C en haut (repère d'orientation) au-dessus du MCU -->
                        <rect x="144" y="2" width="60" height="60" rx="6" fill="#e5e7eb" stroke="#9ca3af"/>
                        <g id="pinsLeft"></g>
                        <g id="pinsRight"></g>
                    </svg>
                </div>
                <div class="right-pane">
                    <div class="config-panel">
                        <h4>Fonction du pin</h4>
                        <div class="row">
                            <label>Pin sélectionné:</label>
                            <span id="selPin">-</span>
                            <select id="funcSelect">
                                <option>Digital</option>
                                <option>Analog</option>
                                <option>I2C</option>
                                <option>UART</option>
                                <option>SPI</option>
                            </select>
                        </div>
                        
                        <div id="digitalButtonCard" class="subcard" style="display:none;">
                            <div class="row"><label>Mode bouton:</label>
                                <select id="btnMode">
                                    <option value="pulse">Push (impulsion immédiate 1→0)</option>
                                    <option value="press_release">Push (1 à l'appui, 0 au relâchement)</option>
                                    <option value="toggle">On/Off (toggle)</option>
                                </select>
                            </div>
                        </div>
                        
                        <div id="ledCard" class="subcard" style="display:none;">
                            <div class="row"><label>LED mode:</label>
                                <select id="ledMode">
                                    <option value="onoff">LED On/Off</option>
                                    <option value="pwm">LED PWM</option>
                                </select>
                            </div>
                        </div>
                        
                        <h4>RTP‑MIDI</h4>
                        <div class="row switch">
                            <input type="checkbox" id="rtpEnabled2"/>
                            <label for="rtpEnabled2">Activer</label>
                            <label>Type de message:</label>
                            <select id="rtpMsgType">
                                <option>Note</option>
                                <option>Control Change</option>
                                <option>Program Change</option>
                                <option>Pitch Bend</option>
                                <option>Aftertouch (Channel)</option>
                                <option>Note + vélocité</option>
                                <option>Note (balayage)</option>
                                <option>Clock</option>
                                <option>Tap Tempo</option>
                            </select>
                        </div>
                        <div id="rtpParams" class="subcard" style="display:none;">
                            <div class="row" id="rtpNoteRow" style="display:none;">
                                <label>Note:</label>
                                <input type="number" id="rtpNote" min="0" max="127" placeholder="60" style="width:90px;"/>
                            </div>
                            <div class="row" id="rtpCcRow" style="display:none;">
                                <label>CC#:</label>
                                <input type="number" id="rtpCc" min="0" max="127" placeholder="7" style="width:90px;"/>
                            </div>
                            <div class="row" id="rtpCcOnOffRow" style="display:none;">
                                <label>Valeurs:</label>
                                <span>ON</span>
                                <input type="number" id="rtpCcOn" min="0" max="127" placeholder="127" style="width:90px;"/>
                                <span>OFF</span>
                                <input type="number" id="rtpCcOff" min="0" max="127" placeholder="0" style="width:90px;"/>
                            </div>
                            <div class="row" id="rtpPcRow" style="display:none;">
                                <label>Program#:</label>
                                <input type="number" id="rtpPc" min="0" max="127" placeholder="0" style="width:90px;"/>
                            </div>
                            <div class="row" id="rtpVelRow" style="display:none;">
                                <label>Vélocité:</label>
                                <input type="number" id="rtpVel" min="1" max="127" placeholder="100" style="width:90px;"/>
                            </div>
                            <div class="row" id="rtpCcRangeRow" style="display:none;">
                                <label>Plage MIDI:</label>
                                <input type="number" id="rtpCcMin" min="0" max="127" placeholder="0" style="width:90px;"/>
                                <span>→</span>
                                <input type="number" id="rtpCcMax" min="0" max="127" placeholder="127" style="width:90px;"/>
                            </div>
                            <div class="row" id="rtpChanRow" style="display:none;">
                                <label>Canal:</label>
                                <input type="number" id="rtpChan" min="1" max="16" placeholder="1" style="width:90px;"/>
                            </div>
                            <div class="row" id="rtpClockHint" style="display:none; color:#6b7280;">
                                <span>Clock / Tap Tempo: pas de canal.</span>
                            </div>
                            <div class="row" id="rtpNoteSweepRow" style="display:none;">
                                <label>Balayage:</label>
                                <input type="number" id="rtpNoteMin" min="0" max="127" placeholder="48" style="width:90px;"/>
                                <span>→</span>
                                <input type="number" id="rtpNoteMax" min="0" max="127" placeholder="72" style="width:90px;"/>
                                <label style="margin-left:8px;">Vélocité fixe:</label>
                                <input type="number" id="rtpNoteVelFix" min="1" max="127" placeholder="100" style="width:90px;"/>
                            </div>
                        </div>
                        <h4>OSC</h4>
                        <div class="row switch">
                            <input type="checkbox" id="oscEnabled2"/>
                            <label for="oscEnabled2">Activer</label>
                            <label>Adresse:</label>
                            <input type="text" id="oscAddress" placeholder="/mon/adresse"/>
                        </div>
                        <h4>Debug console</h4>
                        <div class="row switch">
                            <input type="checkbox" id="dbgEnabled"/>
                            <label for="dbgEnabled">Activer</label>
                            <label>Entête:</label>
                            <input type="text" id="dbgHeader" placeholder="[DBG]"/>
                        </div>
                        
                        <div class="row" style="margin-top:12px; justify-content:flex-start; gap:12px;">
                            <button id="savePinBtn" type="button">Enregistrer</button>
                            <div class="hint" id="savePinMsg"></div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>
    
    <script>
        const $=s=>document.querySelector(s[0]=='#'?s:'#'+s);
        // Gestion des onglets
        document.querySelectorAll('.tab').forEach(tab => {
            tab.onclick = () => {
                document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
                document.querySelectorAll('.panel').forEach(p => p.classList.remove('active'));
                tab.classList.add('active');
                $(`panel-${tab.dataset.t}`).classList.add('active');
            };
        });
        
        // Chargement du statut
        async function loadStatus() {
            const res = await fetch('/api/status');
            const data = await res.json();
            $('#apSsid').textContent = data.ap_ssid;
            $('#apIp').textContent = data.ap_ip;
            $('#staSsid').textContent = data.sta_ssid;
            $('#staIp').textContent = data.sta_ip;
            $('#staStatus').textContent = data.sta_connected ? 'Connecté' : 'Déconnecté';
            $('#staStatus').style.color = data.sta_connected ? '#059669' : '#dc2626';
        }
        
        // Charger l'état RTP-MIDI
        async function loadRtpStatus() {
            const res = await fetch('/api/rtp/status');
            const data = await res.json();
            $('#rtpEnabled').checked = data.enabled;
            $('#rtpName').value = data.name;
            $('#rtpTarget').value = data.target;
            $('#rtpStatus').textContent = data.enabled ? 'Activé' : 'Désactivé';
            $('#rtpStatus').className = 'status ' + (data.enabled ? 'enabled' : 'disabled');
        }
        
        // Chargement des capacités des pins
        let caps = null;
        let selectedRect = null;
        // Etats bus et index des rectangles par label
        const busStates = { I2C:false, SPI:false, UART:false };
        const pinRects = {};
        // Persistance d’état UI par pin (avant backend)
        const pinConfigs = {}; // { 'D0': { role: 'Bouton', rtpEnabled:true, rtpType:'Note', ... }, ... }
        let currentPinLabel = '';

        function readUIToConfig(){
            const cfg = {};
            cfg.role = document.getElementById('funcSelect')?.value || '';
            // Bouton
            cfg.btnMode = document.getElementById('btnMode')?.value || '';
            // RTP-MIDI
            cfg.rtpEnabled = !!document.getElementById('rtpEnabled2')?.checked;
            cfg.rtpType = document.getElementById('rtpMsgType')?.value || '';
            cfg.rtpNote = document.getElementById('rtpNote')?.value || '';
            cfg.rtpCc = document.getElementById('rtpCc')?.value || '';
            cfg.rtpPc = document.getElementById('rtpPc')?.value || '';
            cfg.rtpChan = document.getElementById('rtpChan')?.value || '';
            cfg.rtpCcOn = document.getElementById('rtpCcOn')?.value || '';
            cfg.rtpCcOff = document.getElementById('rtpCcOff')?.value || '';
            cfg.rtpVel = document.getElementById('rtpVel')?.value || '';
            cfg.rtpCcMin = document.getElementById('rtpCcMin')?.value || '';
            cfg.rtpCcMax = document.getElementById('rtpCcMax')?.value || '';
            cfg.rtpNoteMin = document.getElementById('rtpNoteMin')?.value || '';
            cfg.rtpNoteMax = document.getElementById('rtpNoteMax')?.value || '';
            cfg.rtpNoteVelFix = document.getElementById('rtpNoteVelFix')?.value || '';
            // LED
            cfg.ledMode = document.getElementById('ledMode')?.value || '';
            // OSC & Debug (pour UI)
            cfg.oscEnabled = !!document.getElementById('oscEnabled2')?.checked;
            cfg.oscAddress = document.getElementById('oscAddress')?.value || '';
            cfg.dbgEnabled = !!document.getElementById('dbgEnabled')?.checked;
            cfg.dbgHeader = document.getElementById('dbgHeader')?.value || '';
            return cfg;
        }

        function applyConfigToUI(cfg){
            if(!cfg) return;
            const setVal = (id, v)=>{ const el=document.getElementById(id); if(el && typeof v!== 'undefined' && v !== null){ el.value = v; } };
            const setChk = (id, b)=>{ const el=document.getElementById(id); if(el){ el.checked = !!b; } };
            // Rôle (reconstruit après updateFuncMenuForLabel)
            if(cfg.role){ const sel=document.getElementById('funcSelect'); if(sel && !sel.disabled){ sel.value = cfg.role; updateRoleSubcards(); } }
            // Bouton
            setVal('btnMode', cfg.btnMode);
            // RTP-MIDI
            setChk('rtpEnabled2', cfg.rtpEnabled);
            if(cfg.rtpType){ const t=document.getElementById('rtpMsgType'); if(t && !t.disabled){ t.value = cfg.rtpType; } }
            updateRtpParamsVisibility();
            setVal('rtpNote', cfg.rtpNote);
            setVal('rtpCc', cfg.rtpCc);
            setVal('rtpPc', cfg.rtpPc);
            setVal('rtpChan', cfg.rtpChan);
            setVal('rtpCcOn', cfg.rtpCcOn);
            setVal('rtpCcOff', cfg.rtpCcOff);
            setVal('rtpVel', cfg.rtpVel);
            setVal('rtpCcMin', cfg.rtpCcMin);
            setVal('rtpCcMax', cfg.rtpCcMax);
            setVal('rtpNoteMin', cfg.rtpNoteMin);
            setVal('rtpNoteMax', cfg.rtpNoteMax);
            setVal('rtpNoteVelFix', cfg.rtpNoteVelFix);
            // LED
            setVal('ledMode', cfg.ledMode);
            // OSC & Debug (UI)
            setChk('oscEnabled2', cfg.oscEnabled);
            setVal('oscAddress', cfg.oscAddress);
            setChk('dbgEnabled', cfg.dbgEnabled);
            setVal('dbgHeader', cfg.dbgHeader);
        }
 
        function labelsForBus(bus){
            if(bus==='I2C') return ['SDA','SCL','D4','D5'];
            if(bus==='SPI') return ['MOSI','MISO','SCK','D10','D9','D8'];
            if(bus==='UART') return ['TX','RX','D6','D7'];
            return [];
        }

        function updateBusVisuals(){
            ['I2C','SPI','UART'].forEach(bus=>{
                const labels = labelsForBus(bus);
                labels.forEach(lbl=>{
                    const r = pinRects[lbl];
                    if(!r) return;
                    if(busStates[bus]){
                        r.classList.add('busDisabled');
                        r.dataset.bus = bus;
                    } else {
                        r.classList.remove('busDisabled');
                        if(r.dataset.bus===bus) delete r.dataset.bus;
                    }
                });
            });
        }

        function updateRoleSubcards(){
            const sel = document.getElementById('funcSelect');
            const digitalBtn = document.getElementById('digitalButtonCard');
            const ledCard = document.getElementById('ledCard');
            if(digitalBtn) digitalBtn.style.display='none';
            if(ledCard) ledCard.style.display='none';
            if(!sel || sel.disabled) { updateRtpForRole(''); return; }
            const v = sel.value || '';
            if(v==='Bouton') {
                if(digitalBtn) digitalBtn.style.display='block';
            } else if(v==='LED') {
                if(ledCard) ledCard.style.display='block';
            }
            updateRtpForRole(v);
        }

        function setSelectOptions(selectEl, labels){
            if(!selectEl) return;
            selectEl.innerHTML = labels.map((label,i)=>`<option ${i===0?'selected':''}>${label}</option>`).join('');
        }

        function updateRtpForRole(role){
            const rtpEnable = document.getElementById('rtpEnabled2');
            const rtpType = document.getElementById('rtpMsgType');
            const rtpParams = document.getElementById('rtpParams');
            // Par défaut: désactiver si rôle inconnu
            let enabled = true;
            let types = [];
            if(role==='Potentiomètre'){
                types = ['Control Change','Pitch Bend','Aftertouch (Channel)','Note + vélocité','Note (balayage)'];
            } else if(role==='Bouton'){
                types = ['Note','Control Change','Program Change','Clock','Tap Tempo'];
            } else if(role==='LED'){
                // Réception: suivre Note/CC
                types = ['Note','Control Change'];
            } else if(role==='I2C' || role==='SPI' || role==='UART' || role==='Analog in (raw)' || role==='Digital in/out'){
                enabled = false;
            } else if(!role){
                enabled = false;
            }
            if(rtpEnable){ rtpEnable.checked = enabled; rtpEnable.disabled = !enabled; }
            if(rtpType){
                if(enabled){ setSelectOptions(rtpType, types); }
                rtpType.disabled = !enabled;
            }
            if(rtpParams){ rtpParams.style.display = enabled ? 'block' : 'none'; }
            if(enabled) updateRtpParamsVisibility();
        }

        function updateRtpParamsVisibility(){
            const typeSel = document.getElementById('rtpMsgType');
            const params = document.getElementById('rtpParams');
            const noteRow = document.getElementById('rtpNoteRow');
            const ccRow = document.getElementById('rtpCcRow');
            const ccOnOffRow = document.getElementById('rtpCcOnOffRow');
            const pcRow = document.getElementById('rtpPcRow');
            const velRow = document.getElementById('rtpVelRow');
            const ccRangeRow = document.getElementById('rtpCcRangeRow');
            const chanRow = document.getElementById('rtpChanRow');
            const clockHint = document.getElementById('rtpClockHint');
            const noteSweepRow = document.getElementById('rtpNoteSweepRow');
            const roleSel = document.getElementById('funcSelect');
            if(!typeSel || !params) return;
            const v = typeSel.value;
            // reset
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

        function updateFuncMenuForLabel(label){
            const sel = document.getElementById('funcSelect');
            const selPin = document.getElementById('selPin');
            if(selPin) selPin.textContent = label || '-';
            if(!sel) return;
            const setOptions = (opts, enabled=true, preselect=0)=>{
                sel.innerHTML = opts.map((o,i)=>`<option ${i===preselect?'selected':''}>${o}</option>`).join('');
                sel.disabled = !enabled;
            };
            if(label==='5V' || label==='3V3' || label==='GND'){ setOptions([], false); updateRoleSubcards(); return; }
            if(/^A\d+$/.test(label)){ setOptions(['Potentiomètre','Analog in (raw)'], true, 0); updateRoleSubcards(); return; }
            if(label==='SDA' || label==='SCL'){ setOptions(['I2C'], true, 0); updateRoleSubcards();
                // Activer immédiatement I2C et griser D4/D5 + SDA/SCL
                busStates.I2C = true; updateBusVisuals();
                return; }
            if(label==='MOSI' || label==='MISO' || label==='SCK'){ setOptions(['SPI'], true, 0); updateRoleSubcards();
                // Activer immédiatement SPI et griser D8/D9/D10 + MOSI/MISO/SCK
                busStates.SPI = true; updateBusVisuals();
                return; }
            if(label==='TX' || label==='RX'){ setOptions(['UART'], true, 0); updateRoleSubcards();
                // Activer immédiatement UART et griser D6/D7 + TX/RX
                busStates.UART = true; updateBusVisuals();
                return; }
            if(/^D\d+$/.test(label)){
                // Si D appartient à un bus actif, désactiver la fonction
                const isI2c = (label==='D4' || label==='D5') && busStates.I2C;
                const isSpi = (label==='D8' || label==='D9' || label==='D10') && busStates.SPI;
                const isUart = (label==='D6' || label==='D7') && busStates.UART;
                if(isI2c || isSpi || isUart){ setOptions([], false); updateRoleSubcards(); return; }
                const roles = ['Bouton','LED','Digital in/out'];
                setOptions(roles, true, 0); updateRoleSubcards(); return;
            }
            setOptions([], false); updateRoleSubcards();
        }
        // Couleurs fonctions (SVG)
        const FUNC_COLORS = { DIGITAL:'#3B82F6', ANALOG:'#EC4899', I2C:'#10B981', UART:'#6B7280', SPI:'#8B5CF6', POWER:'#EF4444', GND:'#000000' };
        // Disposition CMU simple (statique) — D0..D10 + alim
        const PIN_LAYOUT = [
            // Alim en haut à droite (1 carré non cliquable chacun)
            {name:'5V',  side:'right', row:0,  type:'POWER', clickable:false},
            {name:'GND', side:'right', row:1,  type:'GND',   clickable:false},
            {name:'3V3', side:'right', row:2,  type:'POWER', clickable:false},
            // Gauche: Type puis Digital
            {name:'D0',  side:'left',  row:0,  type:'ANALOG', typeLabel:'A0'},
            {name:'D1',  side:'left',  row:1,  type:'ANALOG', typeLabel:'A1'},
            {name:'D2',  side:'left',  row:2,  type:'ANALOG', typeLabel:'A2'},
            {name:'D3',  side:'left',  row:3,  type:'ANALOG', typeLabel:'A3'},
            {name:'D4',  side:'left',  row:4,  type:'I2C',    typeLabel:'SDA'},
            {name:'D5',  side:'left',  row:5,  type:'I2C',    typeLabel:'SCL'},
            {name:'D6',  side:'left',  row:6,  type:'UART',   typeLabel:'TX'},
            // Droite: Digital puis Type
            {name:'D10', side:'right', row:3,  type:'SPI',    typeLabel:'MOSI'},
            {name:'D9',  side:'right', row:4,  type:'SPI',    typeLabel:'MISO'},
            {name:'D8',  side:'right', row:5,  type:'SPI',    typeLabel:'SCK'},
            {name:'D7',  side:'right', row:6,  type:'UART',   typeLabel:'RX'}
        ];
        function drawBoardPins() {
            const left = $('#pinsLeft'), right = $('#pinsRight');
            if(!left||!right) return;
            left.innerHTML=''; right.innerHTML='';
            const rowH = 28;
            const makeSquare = (x,y,w,h,fill,stroke,label,clickable=true) => {
                const g = document.createElementNS('http://www.w3.org/2000/svg','g');
                const r = document.createElementNS('http://www.w3.org/2000/svg','rect');
                r.setAttribute('x',x); r.setAttribute('y',y); r.setAttribute('width',w); r.setAttribute('height',h); r.setAttribute('rx','4');
                r.setAttribute('fill', fill); r.setAttribute('stroke', stroke); g.appendChild(r);
                if(clickable){ r.style.cursor='pointer'; }
                const t = document.createElementNS('http://www.w3.org/2000/svg','text');
                t.setAttribute('x', x + w/2); t.setAttribute('y', y + h/2 + 1);
                t.setAttribute('text-anchor','middle'); t.setAttribute('class','square-text');
                t.textContent = label; g.appendChild(t);
                if(clickable){
                    g.style.cursor='pointer';
                    r.dataset.label = label;
                    pinRects[label] = r;
                    r.addEventListener('click', () => {
                        // Si la pin est grisée par un bus, un clic désactive le bus entier
                        if(r.classList.contains('busDisabled') && r.dataset.bus){
                            busStates[r.dataset.bus] = false;
                            updateBusVisuals();
                        }
                        // Sauver l'ancienne pin courante avant de changer
                        if(currentPinLabel){ pinConfigs[currentPinLabel] = readUIToConfig(); }
                        if(selectedRect) selectedRect.classList.remove('selectedSquare');
                        r.classList.add('selectedSquare');
                        selectedRect = r;
                        updateFuncMenuForLabel(r.dataset.label);
                        // Réévaluer RTP selon rôle courant
                        updateRtpForRole(document.getElementById('funcSelect')?.value || '');
                        currentPinLabel = r.dataset.label || '';
                        // Appliquer la config sauvegardée si elle existe
                        if(currentPinLabel && pinConfigs[currentPinLabel]){
                            applyConfigToUI(pinConfigs[currentPinLabel]);
                        }
                    });
                }
                return g;
            };
            // grille 5 colonnes: c1(typeL) c2(DL) c3(MCU) c4(DR) c5(typeR)
            // légère séparation entre c1 et c2; ajusté à la nouvelle largeur du MCU
            const COL = { c1:20, c2:68, c4:238, c5:286 };
            const W=44, H=20;
            const renderRowLeft = (row, typeLabel, typeColor, dLabel) => {
                const y = 30 + row*rowH;
                const g = document.createDocumentFragment();
                g.appendChild(makeSquare(COL.c1, y-10, W, H, typeColor, '#9ca3af', typeLabel));
                g.appendChild(makeSquare(COL.c2, y-10, W, H, FUNC_COLORS.DIGITAL, '#9ca3af', dLabel));
                left.appendChild(g);
            };
            const renderRowRightPower = (row, label, color) => {
                const y = 30 + row*rowH;
                right.appendChild(makeSquare(COL.c4, y-10, W, H, color, '#9ca3af', label, false));
            };
            const renderRowRight = (row, dLabel, typeLabel, typeColor) => {
                const y = 30 + row*rowH;
                const g = document.createDocumentFragment();
                g.appendChild(makeSquare(COL.c4, y-10, W, H, FUNC_COLORS.DIGITAL, '#9ca3af', dLabel));
                g.appendChild(makeSquare(COL.c5, y-10, W, H, typeColor, '#9ca3af', typeLabel));
                right.appendChild(g);
            };
            // Gauche
            renderRowLeft(0,'A0',FUNC_COLORS.ANALOG,'D0');
            renderRowLeft(1,'A1',FUNC_COLORS.ANALOG,'D1');
            renderRowLeft(2,'A2',FUNC_COLORS.ANALOG,'D2');
            renderRowLeft(3,'A3',FUNC_COLORS.ANALOG,'D3');
            renderRowLeft(4,'SDA',FUNC_COLORS.I2C,'D4');
            renderRowLeft(5,'SCL',FUNC_COLORS.I2C,'D5');
            renderRowLeft(6,'TX', FUNC_COLORS.UART,'D6');
            // Droite
            renderRowRightPower(0,'5V',  FUNC_COLORS.POWER);
            renderRowRightPower(1,'GND', FUNC_COLORS.GND);
            renderRowRightPower(2,'3V3', FUNC_COLORS.POWER);
            renderRowRight(3,'D10','MOSI',FUNC_COLORS.SPI);
            renderRowRight(4,'D9','MISO', FUNC_COLORS.SPI);
            renderRowRight(5,'D8','SCK',  FUNC_COLORS.SPI);
            renderRowRight(6,'D7','RX',   FUNC_COLORS.UART);
            if(caps && caps.board){ $('#boardName').textContent = caps.board.toUpperCase(); }
            updateBusVisuals();
        }
        async function loadCaps() {
            const res = await fetch('/api/pins/caps');
            caps = await res.json();
            drawBoardPins();
        }
        
        // Attendre que le DOM soit prêt
        window.addEventListener('load', () => {
            loadStatus();
            loadRtpStatus();
            loadCaps();
            drawBoardPins();
            // Init menu Fonction du pin
            const sel = document.getElementById('funcSelect');
            if(sel){ sel.innerHTML = '<option selected>— Sélectionnez une pin —</option>'; sel.disabled = true; }
            // Brancher le changement de rôle pour afficher/masquer les sous-cartes
            if(sel){
                sel.onchange = () => {
                    // Activation bus si on sélectionne une fonction bus sur un label bus
                    const label = (document.getElementById('selPin')?.textContent || '').trim();
                    const val = sel.value;
                    if((label==='SDA' || label==='SCL') && val==='I2C'){ busStates.I2C = true; updateBusVisuals(); }
                    if((label==='MOSI' || label==='MISO' || label==='SCK') && val==='SPI'){ busStates.SPI = true; updateBusVisuals(); }
                    if((label==='TX' || label==='RX') && val==='UART'){ busStates.UART = true; updateBusVisuals(); }
                    updateRoleSubcards();
                    // Mettre à jour le cache de la pin courante
                    if(currentPinLabel){ pinConfigs[currentPinLabel] = readUIToConfig(); }
                };
            }

            // RTP-MIDI params visibility
            const rtpType = document.getElementById('rtpMsgType');
            if(rtpType){ rtpType.onchange = updateRtpParamsVisibility; updateRtpParamsVisibility(); }

            // Brancher les changements champs pour sauvegarde locale immédiate
            const fieldsToWatch = ['btnMode','rtpEnabled2','rtpMsgType','rtpNote','rtpCc','rtpPc','rtpChan','rtpCcOn','rtpCcOff','rtpVel','rtpCcMin','rtpCcMax','rtpNoteMin','rtpNoteMax','rtpNoteVelFix','ledMode','oscEnabled2','oscAddress','dbgEnabled','dbgHeader'];
            fieldsToWatch.forEach(id=>{
                const el = document.getElementById(id);
                if(el){
                    el.addEventListener('change', ()=>{ if(currentPinLabel){ pinConfigs[currentPinLabel] = readUIToConfig(); } });
                    el.addEventListener('input', ()=>{ if(currentPinLabel){ pinConfigs[currentPinLabel] = readUIToConfig(); } });
                }
            });
            
            // Formulaire Wi-Fi STA
            $('#sta').onsubmit = async (ev) => {
                ev.preventDefault();
                $('#staMsg').textContent = 'Connexion...';
                const res = await fetch('/api/sta', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                    body: `ssid=${encodeURIComponent($('#ssid').value)}&pass=${encodeURIComponent($('#pass').value)}`
                });
                $('#staMsg').textContent = res.ok ? 'Configuration sauvegardée' : 'Erreur ' + res.status;
            };
            
            // Formulaire RTP-MIDI
            $('#rtp').onsubmit = async (ev) => {
                ev.preventDefault();
                const res = await fetch('/api/rtp', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                    body: 'name=' + encodeURIComponent($('#rtpName').value) + '&target=' + $('#rtpTarget').value
                });
                $('#rtpMsg').textContent = res.ok ? 'Configuration RTP-MIDI enregistrée' : 'Erreur ' + res.status;
            };
            
            // Toggle RTP-MIDI
            $('#rtpEnabled').onchange = async () => {
                const enabled = $('#rtpEnabled').checked;
                const res = await fetch('/api/rtp/enable', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                    body: 'enable=' + enabled
                });
                $('#rtpMsg').textContent = res.ok ? 'RTP-MIDI ' + (enabled ? 'activé' : 'désactivé') : 'Erreur ' + res.status;
            };
            
            // Formulaire OSC
            $('#osc').onsubmit = async (ev) => {
                ev.preventDefault();
                const res = await fetch('/api/osc', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                    body: 'target=' + $('#oscTarget').value + '&port=' + $('#oscPort').value
                });
                $('#oscMsg').textContent = res.ok ? 'Configuration OSC enregistrée' : 'Erreur ' + res.status;
            };

            // Enregistrer la config du pin sélectionné
            const saveBtn = document.getElementById('savePinBtn');
            if(saveBtn){
                saveBtn.onclick = async () => {
                    const msg = document.getElementById('savePinMsg');
                    const pinLabel = (document.getElementById('selPin')?.textContent || '').trim();
                    const role = (document.getElementById('funcSelect')?.value || '').trim();
                    if(!pinLabel || pinLabel==='— Sélectionnez une pin —' || !role){ if(msg) msg.textContent='Sélectionnez un pin et un rôle'; return; }
                    // Collecte RTP-MIDI
                    const rtpOn = !!document.getElementById('rtpEnabled2')?.checked;
                    const rtpType = document.getElementById('rtpMsgType')?.value || '';
                    const rtpNote = document.getElementById('rtpNote')?.value || '';
                    const rtpCc = document.getElementById('rtpCc')?.value || '';
                    const rtpPc = document.getElementById('rtpPc')?.value || '';
                    const rtpChan = document.getElementById('rtpChan')?.value || '';
                    const rtpCcOn = document.getElementById('rtpCcOn')?.value || '';
                    const rtpCcOff = document.getElementById('rtpCcOff')?.value || '';
                    const rtpVel = document.getElementById('rtpVel')?.value || '';
                    // LED mode
                    const ledMode = document.getElementById('ledMode')?.value || '';
                    // Build form body (x-www-form-urlencoded)
                    const params = new URLSearchParams();
                    params.set('pinLabel', pinLabel);
                    params.set('role', role);
                    params.set('rtpEnabled', String(rtpOn));
                    if(rtpType) params.set('rtpType', rtpType);
                    if(rtpNote) params.set('rtpNote', rtpNote);
                    if(rtpCc) params.set('rtpCc', rtpCc);
                    if(rtpPc) params.set('rtpPc', rtpPc);
                    if(rtpChan) params.set('rtpChan', rtpChan);
                    if(rtpCcOn) params.set('rtpCcOn', rtpCcOn);
                    if(rtpCcOff) params.set('rtpCcOff', rtpCcOff);
                    if(rtpVel) params.set('rtpVel', rtpVel);
                    if(ledMode) params.set('ledMode', ledMode);
                    if(msg) msg.textContent='Enregistrement…';
                    try{
                        const res = await fetch('/api/pins/set', {
                            method:'POST',
                            headers:{'Content-Type':'application/x-www-form-urlencoded'},
                            body: params.toString()
                        });
                        if(!res.ok){ if(msg) msg.textContent = 'Erreur ' + res.status; return; }
                        if(msg) msg.textContent='Sauvegardé';
                    }catch(e){ if(msg) msg.textContent='Erreur réseau'; }
                };
            }
        });
    </script>
</body>
</html>
)rawliteral";