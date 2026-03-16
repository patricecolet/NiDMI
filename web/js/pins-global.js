/*global view state*/
let pinsViewMode ='pin';
let gcfg = defaultGlobalConfig();

/* default config*/
function defaultGlobalConfig() {
    return {
        midiChannel: 1,
        clockSource: "internal",
        steps: 16,
        tempo: 120
    };

}

/* View mode management */
function applyPinsViewMode(){
    const pinCard = document.getElementById('pinEditorCard');
    const globalCard = document.getElementById('globalEditorCard');
    const globalBtnRect = document.getElementById('globalModeBtnRect');

    if (!pinCard || !globalCard || !globalBtnRect) return;

    if(pinsViewMode === "global"){
        globalCard.classList.remove('hidden');
        pinCard.classList.add('hidden');
        globalBtnRect.classList.add('is-global-active');
    }
    else{
        pinCard.classList.remove('hidden');
        globalCard.classList.add('hidden');
        globalBtnRect.classList.remove('is-global-active');
    }
}

/* Pin editor view*/
function showPinEditor(pinLabel){
    pinsViewMode = 'pin';
    if(pinLabel) cur = pinLabel;
    applyPinsViewMode();
}

/* Global editor view*/
function showGlobalEditor(){
    pinsViewMode = 'global';
    applyPinsViewMode();
    renderGlobalForm();
}

/* Render global config form*/
function renderGlobalForm(){
    const ch = document.getElementById('globalMidiChannel');
    const clock = document.getElementById('globalClockSource');
    const steps = document.getElementById('seqSteps');
    const tempo = document.getElementById('seqTempo');

    if(ch) ch.value = gcfg.midiChannel;
    if(clock) clock.value = gcfg.clockSource;
    if(steps) steps.value = gcfg.steps;
    if(tempo) tempo.value = gcfg.tempo;
}

/* Form read */
function readGlobalForm(){
    const ch = document.getElementById('globalMidiChannel');
    const clock = document.getElementById('globalClockSource');
    const steps = document.getElementById('seqSteps');
    const tempo = document.getElementById('seqTempo');

    gcfg.midiChannel = ch ? parseInt(ch.value) : 1;
    gcfg.clockSource = clock ? clock.value : "internal";
    gcfg.steps = steps ? parseInt(steps.value) : 16;
    gcfg.tempo = tempo ? parseInt(tempo.value) : 120;
}

/* Load config */
async function loadGlobalConfig(){
    try{
        const res = await fetch('/global-config');
        if(!res.ok) return;
        const data = await res.json();
        gcfg = Object.assign(defaultGlobalConfig(), data);
        renderGlobalForm();
    }
    catch(err){console.warn("global config load failed", err);}
}

/*save config*/
async function saveGlobalConfig(){
    readGlobalForm();
    const msg = document.getElementById('globalStatusMsg');
    try{
        const res = await fetch('/global-config', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify(gcfg)
        });
        if(!res.ok) throw new Error("Failed to save config");
        if(msg){
            msg.textContent = "Config saved"; 
            setTimeout(() => { msg.textContent = ""; }, 2000);
        }
    }
    catch(err){
        if(msg) msg.textContent = "Save failed";
        console.error(err);
    }   
}
/*Init*/
function initGlobalView(){
    const saveBtn = document.getElementById('globalSaveBtn');
    const reloadBtn = document.getElementById('globalReloadBtn');

    if(saveBtn) saveBtn.addEventListener('click', saveGlobalConfig);
    if(reloadBtn) reloadBtn.addEventListener('click', loadGlobalConfig);
}
