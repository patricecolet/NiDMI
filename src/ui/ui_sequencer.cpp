#include "ui_sequencer.h"

const char SEQUENCER_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>NIDMIDI SEQ</title>
<style>
  :root {
    --bg:       #f9fafb;
    --panel:    #ffffff;
    --border:   #e5e7eb;
    --green:    #10b981;
    --green-dim:#d1fae5;
    --green-dk: #6ee7b7;
    --amber:    #f59e0b;
    --red:      #ef4444;
    --text:     #1f2937;
    --muted:    #6b7280;
    --radius:   6px;
    --mono:     -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue',sans-serif;
  }

  *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

  body {
    background: var(--bg);
    color: var(--text);
    font-family: var(--mono);
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 20px 12px 40px;
    font-size: 13px;
  }

  body::before {
    content: '';
    position: fixed; inset: 0;
    background: repeating-linear-gradient(
      to bottom,
      transparent 0px, transparent 3px,
      rgba(0,0,0,.02) 3px, rgba(0,0,0,.02) 4px
    );
    pointer-events: none;
    z-index: 999;
  }

  /* ── Header ── */
  header {
    width: 100%; max-width: 480px;
    display: flex; align-items: center; justify-content: space-between;
    border-bottom: 1px solid var(--border);
    padding-bottom: 10px; margin-bottom: 20px;
  }
  .logo {
    font-size: 15px; font-weight: bold;
    letter-spacing: .35em;
    color: var(--green);
    text-shadow: 0 0 8px rgba(16,185,129,.2);
  }
  .status-led {
    width: 8px; height: 8px; border-radius: 50%;
    background: #d1d5db;
    box-shadow: 0 0 4px #d1d5db;
    transition: background .15s, box-shadow .15s;
  }
  .status-led.active {
    background: var(--green);
    box-shadow: 0 0 8px var(--green);
  }

  /* ── Cards ── */
  .card {
    width: 100%; max-width: 480px;
    background: var(--panel);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: 14px 16px;
    margin-bottom: 10px;
  }
  .card-label {
    font-size: 9px; letter-spacing: .2em;
    color: var(--muted); text-transform: uppercase;
    margin-bottom: 8px;
  }

  /* ── File name ── */
  #file-name {
    font-size: 20px; font-weight: bold;
    color: var(--green);
    text-shadow: 0 0 6px rgba(16,185,129,.1);
    letter-spacing: .06em;
    word-break: break-all;
    min-height: 28px;
  }
  #file-name.empty { color: var(--muted); font-size: 13px; font-weight: normal; }

  /* ── Progress bar ── */
  .prog-wrap { width: 100%; height: 3px; background: var(--border); border-radius: 2px; overflow: hidden; margin-top: 10px; }
  #prog-bar  { height: 100%; background: var(--green); box-shadow: 0 0 6px var(--green); width: 0%; transition: width .3s; }

  /* ── Inputs ── */
  .row { display: flex; gap: 8px; align-items: stretch; }

  select, input[type=number], input[type=file] {
    background: var(--bg);
    border: 1px solid var(--green-dim);
    border-radius: var(--radius);
    color: var(--text);
    font-family: var(--mono);
    font-size: 13px;
    padding: 7px 10px;
    outline: none;
    -webkit-appearance: none;
    appearance: none;
    transition: border-color .15s, box-shadow .15s;
  }
  select {
    flex: 1; cursor: pointer;
    background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='7'%3E%3Cpath d='M0 0l6 7 6-7z' fill='%2310b981'/%3E%3C/svg%3E");
    background-repeat: no-repeat; background-position: right 10px center;
    padding-right: 28px;
  }
  select:focus, input[type=number]:focus { border-color: var(--green); box-shadow: 0 0 0 2px rgba(57,255,106,.14); }
  select option { background: #ffffff; }

  input[type=number] {
    width: 86px; text-align: center;
    font-size: 24px; font-weight: bold;
    color: var(--amber); border-color: #fde68a;
    letter-spacing: .05em;
  }
  input[type=number]:focus { border-color: var(--amber); box-shadow: 0 0 0 2px rgba(245,158,11,.1); }
  input[type=number]::-webkit-inner-spin-button,
  input[type=number]::-webkit-outer-spin-button { -webkit-appearance: none; }

  /* ── Buttons ── */
  button {
    font-family: var(--mono);
    font-size: 12px; letter-spacing: .12em;
    border: 1px solid; border-radius: var(--radius);
    cursor: pointer; transition: all .1s;
    padding: 7px 14px; text-transform: uppercase;
    background: transparent;
  }
  button:active { transform: scale(.95); }

  .btn-green { color: var(--green); border-color: var(--green-dim); }
  .btn-green:hover { background: var(--green-dim); box-shadow: 0 0 8px rgba(16,185,129,.15); }

  .btn-amber { color: var(--amber); border-color: #fef3c7; }
  .btn-amber:hover { background: #fef3c7; box-shadow: 0 0 8px rgba(245,158,11,.15); }

  .btn-step {
    font-size: 20px; width: 44px; padding: 7px 0;
    color: var(--text); border-color: var(--border); text-align: center;
  }
  .btn-step:hover { border-color: var(--amber); color: var(--amber); background: #fffbeb; }

  .btn-play {
    font-size: 13px; font-weight: bold; letter-spacing: .2em;
    color: var(--green); border-color: var(--green);
    padding: 13px 0; flex: 1;
    position: relative; overflow: hidden;
  }
  .btn-play:hover { background: var(--green-dim); box-shadow: 0 0 12px rgba(16,185,129,.2); }
  .btn-play.flash {
    background: var(--green-dim);
    box-shadow: 0 0 16px rgba(16,185,129,.3), inset 0 0 12px rgba(16,185,129,.1);
  }
  .btn-play::after {
    content: ''; position: absolute; inset: 0;
    background: linear-gradient(90deg, transparent, rgba(16,185,129,.08), transparent);
    transform: translateX(-100%);
  }
  .btn-play.flash::after { transition: transform .35s; transform: translateX(100%); }

  /* ── Measure row ── */
  .measure-row { display: flex; align-items: center; gap: 10px; justify-content: center; }
  .measure-info { font-size: 10px; color: var(--muted); text-align: center; margin-top: 6px; letter-spacing: .1em; }

  /* ── Upload ── */
  .upload-area {
    border: 1px dashed var(--green-dim);
    border-radius: var(--radius);
    padding: 14px; text-align: center;
    cursor: pointer; transition: all .15s;
    position: relative;
  }
  .upload-area:hover, .upload-area.drag { border-color: var(--green); background: #f0fdf4; }
  .upload-area input[type=file] {
    position: absolute; inset: 0; opacity: 0; cursor: pointer;
    width: 100%; height: 100%; border: none; padding: 0;
  }
  .upload-label { color: var(--muted); font-size: 11px; letter-spacing: .1em; pointer-events: none; }
  .upload-label span { color: var(--green); }
  #upload-status { font-size: 11px; margin-top: 6px; color: var(--muted); min-height: 16px; text-align: center; }

  /* ── Note log ── */
  #note-log {
    background: var(--bg); border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: 8px 10px; min-height: 54px;
    font-size: 11px; color: var(--muted);
    line-height: 1.75; overflow-y: auto; max-height: 110px;
  }
  #note-log .entry .note { color: var(--green); font-weight: bold; }
  #note-log .entry .ts   { color: var(--muted); font-size: 10px; }
  #note-log .err  { color: var(--red); }
  #note-log .warn { color: var(--amber); }

  footer {
    margin-top: 18px;
    font-size: 9px; color: var(--muted);
    letter-spacing: .14em; text-align: center;
  }
</style>
</head>
<body>

<header>
  <div class="logo">NIDMIDI SEQ</div>
  <div class="status-led" id="led" title="ESP32 connection"></div>
</header>

<!-- Active file -->
<div class="card">
  <div class="card-label">Active file</div>
  <div id="file-name" class="empty">&#8212; no file loaded &#8212;</div>
  <div class="prog-wrap"><div id="prog-bar"></div></div>
</div>

<!-- File selector -->
<div class="card">
  <div class="card-label">Select file</div>
  <div class="row">
    <select id="file-select"><option value="">Loading&#8230;</option></select>
    <button class="btn-green" onclick="loadSelected()">&#9654; LOAD</button>
  </div>
</div>

<!-- Upload -->
<div class="card">
  <div class="card-label">Upload new .nidmid file</div>
  <div class="upload-area" id="drop-zone">
    <input type="file" id="file-input" accept=".nidmid" onchange="uploadFile(this.files[0])">
    <div class="upload-label">Drop here or <span>browse</span> &#8212; .nidmid only</div>
  </div>
  <div id="upload-status"></div>
</div>

<!-- Measure navigation -->
<div class="card">
  <div class="card-label">Measure</div>
  <div class="measure-row">
    <button class="btn-step" onclick="stepMeasure(-1)">&#8722;</button>
    <input type="number" id="measure-input" value="1" min="1"
           onchange="setMeasure(this.value)"
           onkeydown="if(event.key==='Enter')setMeasure(this.value)">
    <button class="btn-step" onclick="stepMeasure(+1)">+</button>
  </div>
  <div class="measure-info" id="measure-info">&#8212; / &#8212; measures</div>
</div>

<!-- Step play -->
<div class="card">
  <div class="card-label">Step play</div>
  <div class="row" style="margin-bottom:10px;">
    <button class="btn-play" id="btn-play" onclick="stepPlay()">&#9654; STEP</button>
    <button class="btn-amber" onclick="resetPlay()" title="Go back to start">&#8635; RESET</button>
  </div>
  <div class="card-label" style="margin-bottom:4px;">Note log</div>
  <div id="note-log">Waiting&#8230;</div>
</div>

<footer>ESP32 &middot; LittleFS &middot; .nidmid &nbsp;|&nbsp; v0.1</footer>

<script>
/* ═══════════════════════════════════════════════
   API endpoints — adjust to match your firmware
═══════════════════════════════════════════════ */
const API = {
  files:   '/api/files',    // GET  → ["a.nidmid","b.nidmid",…]
  status:  '/api/status',   // GET  → {file,measure,totalMeasures,stepIndex,totalSteps}
  select:  '/api/select',   // POST {file:"x.nidmid"} → {ok,file,totalMeasures}
  measure: '/api/measure',  // POST {measure:N}        → {ok,measure}
  step:    '/api/step',     // POST {}                 → {ok,note,velocity,channel,measure,stepIndex,totalSteps,done}
  reset:   '/api/reset',    // POST {}                 → {ok}
  upload:  '/api/upload',   // POST multipart/form-data, field "file"
};

let state = { file:'', measure:1, totalMeasures:0, stepIndex:0, totalSteps:0 };

const $   = id => document.getElementById(id);
const led = $('led');
const setLed = on => led.classList.toggle('active', on);

/*
async function apiFetch(url, opts={}, retries = 2) {
  try {
    const r = await fetch(url, {
      ...opts,
      headers: { 'Accept':'application/json', 'Content-Type':'application/json', ...(opts.headers||{}) }
    });
    if (!r.ok) throw new Error('HTTP ' + r.status);
    return await r.json();
  } catch(e) {
    console.error(e.name === 'TypeError' ? 'Connection reset or blocked' : e.message);
    // If connection was reset and we have retries left, try again with backoff
    if (retries > 0 && (e.message.includes('Failed to fetch') || e.message.includes('ERR_'))) {
      const delay = (3 - retries) * 300; // 300ms, 600ms backoff
      await new Promise(resolve => setTimeout(resolve, delay));
      return await apiFetch(url, opts, retries - 1);
    }
    setLed(false);
    throw e;
  }
}
*/

async function apiFetch(url, opts={}, retries = 2) {
  try {
    const r = await fetch(url, {
      ...opts,
      headers: { 'Accept':'application/json', 'Content-Type':'application/json', ...(opts.headers||{}) }
    });
    if (!r.ok) throw new Error('HTTP ' + r.status);
    return await r.json();
  } catch(e) {
    // Log the actual error 'e' to see the specific browser reason (e.g., ERR_CONNECTION_RESET)
    console.error(e.name === 'TypeError' ? `Connection reset or blocked: ${e.message}` : e.message);
    
    // Check for common network failure indicators in the error message
    const isNetworkError = e.name === 'TypeError' || e.message.includes('Failed to fetch') || e.message.includes('ERR_');
    
    if (retries > 0 && isNetworkError) {
      const delay = (3 - retries) * 300;
      await new Promise(resolve => setTimeout(resolve, delay));
      return await apiFetch(url, opts, retries - 1);
    }
    setLed(false);
    throw e;
  }
}

function noteToName(n) {
  return ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'][n % 12]
       + (Math.floor(n / 12) - 1);
}

function log(html, cls='entry') {
  const el  = $('note-log');
  const now = new Date();
  const ts  = String(now.getMinutes()).padStart(2,'0') + ':'
            + String(now.getSeconds()).padStart(2,'0');
  el.innerHTML = `<div class="${cls}"><span class="ts">${ts}</span> ${html}</div>` + el.innerHTML;
  while (el.children.length > 24) el.lastElementChild.remove();
}
function logErr (msg) { log('&#10005; ' + msg, 'err');  }
function logWarn(msg) { log(msg, 'warn'); }

/* ── Update UI ── */
function updateUI(s) {
  state = { ...state, ...s };

  const nameEl = $('file-name');
  if (state.file) {
    nameEl.textContent = state.file.replace(/\.nidmid$/i, '');
    nameEl.classList.remove('empty');
  } else {
    nameEl.innerHTML = '&#8212; no file loaded &#8212;';
    nameEl.classList.add('empty');
  }

  $('measure-input').value = state.measure || 1;
  $('measure-info').textContent = state.totalMeasures
    ? `${state.measure} / ${state.totalMeasures} measures`
    : '— / — measures';

  $('prog-bar').style.width = state.totalSteps
    ? (state.stepIndex / state.totalSteps * 100) + '%' : '0%';

  setLed(true);
}

/* ── Init ── */
async function init() {
  try {
    const [status, files] = await Promise.all([apiFetch(API.status), apiFetch(API.files)]);
    updateUI(status);
    buildSelect(files, status.file);
    log('Connected to ESP32');
  } catch(e) {
    buildSelect(['demo_song.nidmid','bass_line.nidmid','arp_001.nidmid'], 'demo_song.nidmid');
    updateUI({ file:'demo_song.nidmid', measure:1, totalMeasures:16, stepIndex:0, totalSteps:64 });
    logWarn('Demo mode &#8212; ESP32 not reachable');
  }
}

function buildSelect(files, current) {
  const sel = $('file-select');
  sel.innerHTML = files.length
    ? files.map(f => `<option value="${f}"${f===current?' selected':''}>${f.replace(/\.nidmid$/i,'')}</option>`).join('')
    : '<option value="">No files found</option>';
}

/* ── Load selected ── */
async function loadSelected() {
  const file = $('file-select').value;
  if (!file) return;
  try {
    logWarn('Loading file (may take a few seconds)...');
    const res = await apiFetch(API.select, {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify({ file })
    });
    updateUI({ file:res.file, measure:1, totalMeasures:res.totalMeasures, stepIndex:0 });
    log(`Loaded: <span class="note">${res.file.replace(/\.nidmid$/i,'')}</span>`);
  } catch(e) { logErr('Load failed: ' + e.message); }
}

/* ── Upload ── */
async function uploadFile(file) {
  if (!file) return;
  const st = $('upload-status');
  st.style.color  = 'var(--text)';
  st.textContent  = 'Sending ' + file.name + '\u2026';
  const fd = new FormData();
  fd.append('file', file);
  try {
    const r = await fetch(API.upload, { method:'POST', body:fd });
    if (!r.ok) throw new Error('HTTP ' + r.status);
    st.style.color  = 'var(--green)';
    st.textContent  = '\u2713 ' + file.name + ' saved';
    const files = await apiFetch(API.files);
    buildSelect(files, state.file);
    log(`Uploaded: <span class="note">${file.name.replace(/\.nidmid$/i,'')}</span>`);
  } catch(e) {
    st.style.color  = 'var(--red)';
    st.textContent  = '\u2715 Error: ' + e.message;
    logErr('Upload failed: ' + e.message);
  }
  $('file-input').value = '';
}

/* ── Measure ── */
async function setMeasure(n) {
  n = Math.max(1, parseInt(n) || 1);
  if (state.totalMeasures) n = Math.min(n, state.totalMeasures);
  try {
    const res = await apiFetch(API.measure, {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify({ measure: n })
    });
    updateUI({ measure:res.measure, stepIndex:0 });
  } catch(e) {
    updateUI({ measure:n, stepIndex:0 });
    logErr('Measure not confirmed by ESP32');
  }
}
function stepMeasure(d) { setMeasure((state.measure || 1) + d); }

/* ── Step play ── */
async function stepPlay() {
  const btn = $('btn-play');
  btn.classList.add('flash');
  setTimeout(() => btn.classList.remove('flash'), 320);
  try {
    const res = await apiFetch(API.step, { method:'POST' });
    if (!res.ok && res.error) { logErr(res.error); return; }
    const name = noteToName(res.note);
    log(`M${res.measure} &bull; step&nbsp;${res.stepIndex} &bull; <span class="note">${name}</span> &bull; vel:${res.velocity} &bull; ch:${res.channel}`);
    updateUI({ measure:res.measure, stepIndex:res.stepIndex, totalSteps:res.totalSteps });
    if (res.done) logWarn('&#8635; End of sequence');
  } catch(e) {
    /* Demo fallback */
    const n = 48 + Math.floor(Math.random() * 24);
    log(`M${state.measure} &bull; demo &bull; <span class="note">${noteToName(n)}</span> &bull; vel:80 &bull; ch:1`);
    const next = (state.stepIndex || 0) + 1;
    updateUI({ stepIndex: next < 64 ? next : 0 });
  }
}

/* ── Reset ── */
async function resetPlay() {
  try { await apiFetch(API.reset, { method:'POST' }); } catch(e) {}
  updateUI({ stepIndex:0 });
  log('&#8635; Sequencer reset');
}

/* ── WebSocket listener for physical button mapping ── */
let sequencerWebSocket = null;
let wsReconnectAttempts = 0;
const MAX_WS_RECONNECT_ATTEMPTS = 5;

function initWebSocket() {
  /* Avoid multiple reconnection attempts piling up */
  if (wsReconnectAttempts >= MAX_WS_RECONNECT_ATTEMPTS) {
    console.warn('[SequencerWebSocket] Max reconnection attempts reached, giving up');
    return;
  }
  
  /* Don't create a new WebSocket if one is already open/connecting */
  if (sequencerWebSocket && (sequencerWebSocket.readyState === WebSocket.OPEN || sequencerWebSocket.readyState === WebSocket.CONNECTING)) {
    return;
  }
  
  const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
  const wsUrl = `${protocol}//${window.location.host}/ws`;
  
  try {
    sequencerWebSocket = new WebSocket(wsUrl);
    wsReconnectAttempts = 0;
    
    sequencerWebSocket.onopen = () => {
      console.log('[SequencerWebSocket] Connected');
    };
    
    sequencerWebSocket.onmessage = (event) => {
      if (!event || !event.data) return;
      
      try {
        const message = JSON.parse(event.data);
        
        /* Handle sequencer play step event from physical button mapping */
        if (message.type === 'seq_event' && message.source === 'play_step' && message.value > 0) {
          log('<span class="note">Button pressed</span> &#8594; play step');
          stepPlay();
        }
      } catch (parseErr) {
        /* Not a JSON message, ignore silently */
      }
    };
    
    sequencerWebSocket.onerror = (error) => {
      console.error('[SequencerWebSocket] Error:', error);
    };
    
    sequencerWebSocket.onclose = () => {
      console.log('[SequencerWebSocket] Disconnected');
      wsReconnectAttempts++;
      /* Exponential backoff: 5s, 10s, 15s, 20s, 25s */
      const delay = Math.min(5000 * wsReconnectAttempts, 25000);
      console.log(`[SequencerWebSocket] Attempt ${wsReconnectAttempts}/${MAX_WS_RECONNECT_ATTEMPTS}, reconnecting in ${delay}ms...`);
      setTimeout(initWebSocket, delay);
    };
  } catch (err) {
    console.error('[SequencerWebSocket] Failed to create WebSocket:', err);
    wsReconnectAttempts++;
    const delay = Math.min(5000 * wsReconnectAttempts, 25000);
    setTimeout(initWebSocket, delay);
  }
}

/* ── Drag & drop ── */
const dz = $('drop-zone');
dz.addEventListener('dragover',  e => { e.preventDefault(); dz.classList.add('drag'); });
dz.addEventListener('dragleave', ()  => dz.classList.remove('drag'));
dz.addEventListener('drop', e => {
  e.preventDefault(); dz.classList.remove('drag');
  const f = e.dataTransfer.files[0];
  if (f && f.name.toLowerCase().endsWith('.nidmid')) uploadFile(f);
  else logErr('Invalid format \u2014 .nidmid required');
});

init();
initWebSocket();
</script>
</body>
</html>
)rawliteral";
