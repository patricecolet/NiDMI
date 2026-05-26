/*global view state*/
let pinsViewMode ='pin';
let gcfg = defaultGlobalConfig();

/* default config*/
function defaultGlobalConfig() {
    return {
        mapping: {
            enabled : true,
            source: "midi_cc",
            target: "global_param",
            mode: "scale",
            inMin: 0,
            inMax: 127,
            outMin: 0,
            outMax: 100
        },
        sequencer: {
            enabled: false,
            clockSource: "internal",
            tempo: 120,
            steps: 16,
            swing: 0,
            direction: "forward",
            gateMs: 80
        },
        midiChannel: 1,
        midiNote: 60,
        velocite: 100
        
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
    const note = document.getElementById('globalMidiNote');
    const vel = document.getElementById('globalMidiVelocity');
    const steps = document.getElementById('seqSteps');

    if(ch) ch.value = gcfg.midiChannel;
    if(steps) steps.value = gcfg.sequencer.steps;
    if(note) note.value = gcfg.midiNote;
    if(vel) vel.value = gcfg.velocite;
}

/* Form read */
function readGlobalForm(){
    const ch = document.getElementById('globalMidiChannel');
    const note = document.getElementById('globalMidiNote');
    const vel = document.getElementById('globalMidiVelocity');
    const steps = document.getElementById('seqSteps');

    gcfg.midiChannel = ch ? parseInt(ch.value) : 1;
    gcfg.midiNote = note ? parseInt(note.value) : 60;
    gcfg.sequencer.steps = steps ? parseInt(steps.value) : 16;
    gcfg.velocite = vel ? parseInt(vel.value) : 100;
}

/* Load config */
async function loadGlobalConfig(){
    try{
        const res = await fetch('/api/pins/global');
        if(!res.ok) return;
        const data = await res.json();
        gcfg = {...defaultGlobalConfig(), ...data, sequencer: {...defaultGlobalConfig().sequencer, ...(data.sequencer || {})}};
        renderGlobalForm();
    }
    catch(err){console.warn("global config load failed", err);}
}

/*save config*/
async function saveGlobalConfig(){
    readGlobalForm();
    const msg = document.getElementById('globalStatusMsg');
    try{
        const res = await fetch('/api/pins/global', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify(gcfg)
        });
        if(!res.ok) throw new Error("Failed to save config");
        if(msg){
            msg.textContent = "Config saved"; 
            setTimeout(() => { msg.textContent = ""; }, 2000);
        }

        console.log("Sending global config:", gcfg);
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
