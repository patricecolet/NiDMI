/* Gestion des multiplexeurs */
let muxList = [];

async function loadMuxList() {
    try {
        const r = await fetch('/api/mux/list');
        
        /* Vérifier que la réponse est OK */
        if (!r.ok) {
            console.error('Erreur HTTP:', r.status, r.statusText);
            muxList = [];
            updateMuxListUI();
            updatePinsList();
            return;
        }
        
        /* Vérifier que le body n'est pas vide */
        const text = await r.text();
        if (!text || text.trim().length === 0) {
            console.warn('Réponse vide de /api/mux/list');
            muxList = [];
            updateMuxListUI();
            updatePinsList();
            return;
        }
        
        /* Parser le JSON */
        const d = JSON.parse(text);
        muxList = d.muxes || [];
        updateMuxListUI();
        updatePinsList();
        /* Mettre à jour le SVG pour griser les pins utilisées par les MUX */
        if (caps && caps.pins) drawBoard();
    } catch(e) {
        console.error('Erreur chargement mux:', e);
        muxList = [];
        updateMuxListUI();
        updatePinsList();
        /* Mettre à jour le SVG pour griser les pins utilisées par les MUX */
        if (caps && caps.pins) drawBoard();
    }
}

function updateMuxListUI() {
    const list = $('#muxList');
    if (!list) return;
    list.innerHTML = '';
    if (muxList.length === 0) {
        list.innerHTML = '<p style="color:#6b7280;">Aucun multiplexeur configure.</p>';
        return;
    }
    muxList.forEach(m => {
        const div = document.createElement('div');
        div.className = 'item';
        div.style.borderLeftColor = '#8B5CF6';
        div.innerHTML = `<span class="lbl">MUX${m.id}</span><span class="role">HC4067</span><span class="stat">16 canaux</span><button class="del-btn" onclick="deleteMux(${m.id}, event)">x</button>`;
        div.onclick = (e) => {
            if (e.target.classList.contains('del-btn')) return;
            showMuxForm(m.id);
        };
        list.appendChild(div);
    });
}

function showMuxForm(muxId = null) {
    const overlay = $('#muxModalOverlay');
    if (overlay) overlay.classList.add('active');
    
    /* Initialiser le select MUX ID (sera rempli par populateMuxPinSelects) */
    const idSel = $('#muxId');
    if (idSel && muxId !== null) {
        idSel.value = muxId;
        idSel.disabled = true; /* Désactiver si modification */
    } else if (idSel) {
        idSel.value = ''; /* Réinitialiser pour nouveau MUX */
        idSel.disabled = false;
    }
    
    /* Si modification, charger les valeurs existantes */
    if (muxId !== null) {
        const mux = muxList.find(m => m.id == muxId);
        if (mux) {
            if ($('#muxPinGroup') && mux.s0 !== undefined) {
                const firstD = getDFromGpio(mux.s0);
                if (firstD !== null) $('#muxPinGroup').value = firstD;
            }
            if ($('#muxEn')) $('#muxEn').value = mux.en || 255;
            if ($('#muxCcBase')) $('#muxCcBase').value = mux.ccBase || 1;
            if ($('#muxMidiChan')) $('#muxMidiChan').value = mux.midiChan || 1;
            if ($('#muxOscBase')) $('#muxOscBase').value = mux.oscBase || '/mux' + muxId;
            if ($('#muxMin')) $('#muxMin').value = mux.min !== undefined ? mux.min : 0;
            if ($('#muxMax')) $('#muxMax').value = mux.max !== undefined ? mux.max : 4095;
            if ($('#muxHysteresis')) $('#muxHysteresis').checked = mux.hysteresis !== undefined ? (mux.hysteresis === 1 || mux.hysteresis === true) : true;
            if ($('#muxOscFormat')) {
                const oscFormatValue = mux.oscFormat || 'float';
                $('#muxOscFormat').value = oscFormatValue;
            }
            if ($('#muxFilterIntensity')) $('#muxFilterIntensity').value = mux.filterIntensity !== undefined ? mux.filterIntensity : 5;
        }
    } else {
        /* Réinitialiser le formulaire pour nouveau mux */
        if ($('#muxSig')) $('#muxSig').value = '';
        if ($('#muxPinGroup')) $('#muxPinGroup').value = '';
        if ($('#muxEn')) $('#muxEn').value = 255;
        if ($('#muxCcBase')) $('#muxCcBase').value = 1;
        if ($('#muxMidiChan')) $('#muxMidiChan').value = 1;
        if ($('#muxOscBase')) $('#muxOscBase').value = '/mux' + (idSel ? idSel.value : '0');
        if ($('#muxMin')) $('#muxMin').value = 0;
        if ($('#muxMax')) $('#muxMax').value = 4095;
        if ($('#muxHysteresis')) $('#muxHysteresis').checked = true;
        if ($('#muxOscFormat')) $('#muxOscFormat').value = 'float';
        if ($('#muxFilterIntensity')) $('#muxFilterIntensity').value = 5;
    }

    /* Peupler les selects */
    populateMuxPinSelects();

    /* Charger SIG apres avoir peuple les selects (mode modification uniquement) */
    if (muxId !== null) {
        const mux = muxList.find(m => m.id == muxId);
        if (mux && $('#muxSig')) {
            $('#muxSig').value = mux.sig;
        }
    }
}

function hideMuxForm() {
    const overlay = $('#muxModalOverlay');
    if (overlay) overlay.classList.remove('active');
}

function populateMuxPinSelects() {
    if (!caps || !caps.pins) return;
    
    /* Vérifier si le MUX existe vraiment dans muxList avant de considérer qu'on est en mode modification */
    const idSel = $('#muxId');
    const currentMuxIdValue = idSel ? parseInt(idSel.value) : null;
    const currentMuxExists = currentMuxIdValue !== null && muxList.find(m => m.id === currentMuxIdValue);
    const currentMuxId = currentMuxExists ? currentMuxIdValue : null;
    
    /* Calculer usedGpios pour SIG et EN (sans muxPinGroup pour éviter les conflits) */
    const usedGpiosForSigAndEn = getUsedGpios(['muxSig', 'muxEn']);
    
    /* SIG : pins analogiques uniquement, avec sélection automatique selon MUX ID */
    const analogPins = caps.pins.filter(p => p.label && p.label.startsWith('A') && p.caps && p.caps.adc);
    let availableSig = analogPins.filter(p => !usedGpiosForSigAndEn.has(p.gpio));
    
    /* Si on est en mode modification, ajouter la pin SIG actuelle du MUX meme si elle est utilisee */
    if (currentMuxId !== null) {
        const currentMux = muxList.find(m => m.id === currentMuxId);
        if (currentMux && currentMux.sig !== undefined && currentMux.sig !== null) {
            const currentSigPin = analogPins.find(p => p.gpio === currentMux.sig);
            if (currentSigPin && !availableSig.find(p => p.gpio === currentMux.sig)) {
                availableSig.push(currentSigPin);
            }
        }
    }
    
    const sigSel = $('#muxSig');
    if (sigSel) {
        sigSel.innerHTML = availableSig.map(p => `<option value="${p.gpio}">${p.label} (GPIO${p.gpio})</option>`).join('');

        /* Valeur par defaut selon MUX ID */
        const selectedMuxId = idSel ? parseInt(idSel.value) : null;
        if (selectedMuxId === 0 && availableSig.find(p => p.gpio === 2)) {
            sigSel.value = 2; /* MUX0 : A0 */
        } else if (selectedMuxId === 1 && availableSig.find(p => p.gpio === 3)) {
            sigSel.value = 3; /* MUX1 : A1 */
        } else if (availableSig.length > 0) {
            sigSel.value = availableSig[0].gpio;
        }
    }
    
    /* Toutes les pins D, triées par numéro */
    const allDPins = caps.pins
        .filter(p => p.label && p.label.startsWith('D'))
        .sort((a, b) => {
            const numA = parseInt(a.label.substring(1));
            const numB = parseInt(b.label.substring(1));
            return numA - numB;
        });
    
    /* Groupes de 4 D consécutifs disponibles */
    /* Calculer usedGpios pour les groupes (sans muxPinGroup pour permettre de préserver la sélection) */
    const usedGpiosForGroups = getUsedGpios(['muxSig', 'muxEn']);
    const pinGroupSel = $('#muxPinGroup');
    if (pinGroupSel) {
        const groups = [];
        for (let i = 0; i < allDPins.length - 3; i++) {
            const d1 = allDPins[i];
            const d2 = allDPins[i + 1];
            const d3 = allDPins[i + 2];
            const d4 = allDPins[i + 3];
            
            const num1 = parseInt(d1.label.substring(1));
            const num2 = parseInt(d2.label.substring(1));
            const num3 = parseInt(d3.label.substring(1));
            const num4 = parseInt(d4.label.substring(1));
            
            /* Vérifier consécutivité (D3, D4, D5, D6) */
            if (num2 === num1 + 1 && num3 === num2 + 1 && num4 === num3 + 1) {
                const allAvailable = [d1.gpio, d2.gpio, d3.gpio, d4.gpio].every(g => !usedGpiosForGroups.has(g));
                if (allAvailable) {
                    groups.push({
                        firstD: num1,
                        label: `${d1.label}-${d4.label}`,
                        gpios: [d1.gpio, d2.gpio, d3.gpio, d4.gpio]
                    });
                }
            }
        }
        /* Préserver la valeur actuelle si elle est valide */
        const currentValue = pinGroupSel.value;
        const currentValueValid = currentValue && groups.find(g => g.firstD === parseInt(currentValue));
        
        pinGroupSel.innerHTML = '<option value="">Choisir...</option>' + 
            groups.map(g => `<option value="${g.firstD}">${g.label}</option>`).join('');
        
        /* Restaurer la valeur précédente si elle est toujours valide, sinon mettre la valeur par défaut */
        if (currentValueValid) {
            pinGroupSel.value = currentValue;
        } else {
            const selectedMuxId = idSel ? parseInt(idSel.value) : null;
            if (selectedMuxId === 0 && groups.find(g => g.firstD === 3)) {
                pinGroupSel.value = 3; /* MUX0 : D3-D6 */
            } else if (selectedMuxId === 1 && groups.find(g => g.firstD === 7)) {
                pinGroupSel.value = 7; /* MUX1 : D7-D10 (automatique) */
            } else if (groups.length > 0) {
                /* Sinon, prendre le premier groupe disponible */
                pinGroupSel.value = groups[0].firstD;
            }
        }
    }
    
    /* EN : D suivant (D+4) si disponible, Non connecté par défaut */
    const enSel = $('#muxEn');
    if (enSel) {
        enSel.innerHTML = '<option value="255">Non connecte</option>';
        
        /* Vérifier si un autre MUX utilise EN */
        const otherMuxUsesEn = muxList.some(m => {
            if (currentMuxId !== null && m.id == currentMuxId) return false;
            return m.en !== undefined && m.en !== null && m.en !== 255;
        });
        
        if (!otherMuxUsesEn && pinGroupSel && pinGroupSel.value) {
            const firstD = parseInt(pinGroupSel.value);
            const enD = firstD + 4;
            const enPin = allDPins.find(p => {
                const num = parseInt(p.label.substring(1));
                return num === enD && !usedGpiosForSigAndEn.has(p.gpio);
            });
            if (enPin) {
                enSel.innerHTML += `<option value="${enPin.gpio}">${enPin.label} (GPIO${enPin.gpio})</option>`;
            }
        }
        
        /* Par défaut : Non connecté */
        if (!enSel.value || enSel.value === '') {
            enSel.value = 255;
        }
    }
    
    /* Détection automatique : proposer MUX1 si disponible */
    if (idSel) {
        if (currentMuxId === null) { /* Nouveau MUX */
            const mux0 = muxList.find(m => m.id === 0);
            const mux1 = muxList.find(m => m.id === 1);
            
            /* Pour la détection de MUX1, recalculer availableSig sans les sélections actuelles du formulaire */
            const usedGpiosForMux1 = getUsedGpios([]); /* Ne pas inclure les sélections du formulaire */
            const availableSigForMux1 = analogPins.filter(p => !usedGpiosForMux1.has(p.gpio));
            
            const availableMuxIds = [0, 1].filter(id => {
                /* Exclure MUX0 s'il est déjà configuré */
                if (id === 0) return !mux0;
                /* Exclure MUX1 s'il est déjà configuré ou s'il n'y a pas de pin analogique disponible */
                if (id === 1) return !mux1 && availableSigForMux1.length > 0;
                return false;
            });
            
            idSel.innerHTML = availableMuxIds.map(id => `<option value="${id}">MUX${id}</option>`).join('');
            /* Sélectionner le premier disponible par défaut */
            if (availableMuxIds.length > 0) {
                idSel.value = availableMuxIds[0];
            }
        } else {
            /* Si modification, la valeur est déjà définie dans showMuxForm() */
            idSel.value = currentMuxId;
            idSel.disabled = true;
        }
    }
    
    /* Event listener pour mettre à jour EN quand le groupe change */
    if (pinGroupSel && !pinGroupSel.dataset.listener) {
        pinGroupSel.dataset.listener = 'true';
        pinGroupSel.addEventListener('change', populateMuxPinSelects);
    }
    
    /* Event listener pour mettre à jour le groupe quand le MUX ID change */
    if (idSel && !idSel.dataset.listenerMuxId) {
        idSel.dataset.listenerMuxId = 'true';
        idSel.addEventListener('change', () => {
            /* Réinitialiser le groupe pour forcer la sélection automatique */
            if ($('#muxPinGroup')) $('#muxPinGroup').value = '';
            /* Réinitialiser SIG pour forcer la sélection automatique */
            if ($('#muxSig')) $('#muxSig').value = '';
            populateMuxPinSelects();
        });
    }
}

async function saveMux(e) {
    e.preventDefault();
    const id = $('#muxId').value;
    const sig = $('#muxSig').value;
    const pinGroup = parseInt($('#muxPinGroup').value);
    const en = $('#muxEn').value;
    const ccBase = parseInt($('#muxCcBase').value) || 1;
    const midiChan = parseInt($('#muxMidiChan').value) || 1;
    const oscBase = $('#muxOscBase').value || '/mux' + id;
    
    /* Extraire les GPIO du groupe (S0, S1, S2, S3) */
    if (!pinGroup) {
        $('#muxMsg').textContent = 'Erreur: Veuillez choisir un groupe de pins';
        $('#muxMsg').style.color = '#ef4444';
        return;
    }
    
    const s0 = getGpioFromD(pinGroup);
    const s1 = getGpioFromD(pinGroup + 1);
    const s2 = getGpioFromD(pinGroup + 2);
    const s3 = getGpioFromD(pinGroup + 3);
    
    if (!s0 || !s1 || !s2 || !s3) {
        $('#muxMsg').textContent = 'Erreur: Groupe de pins invalide';
        $('#muxMsg').style.color = '#ef4444';
        return;
    }
    
    const min = parseInt($('#muxMin').value) || 0;
    const max = parseInt($('#muxMax').value) || 4095;
    const hysteresis = $('#muxHysteresis').checked;
    const oscFormat = $('#muxOscFormat').value || 'float';
    const filterIntensity = parseInt($('#muxFilterIntensity').value) || 5;
    
    const formData = new URLSearchParams();
    formData.append('id', id);
    formData.append('sig', sig);
    formData.append('s0', s0);
    formData.append('s1', s1);
    formData.append('s2', s2);
    formData.append('s3', s3);
    formData.append('en', en);
    formData.append('ccBase', ccBase);
    formData.append('midiChan', midiChan);
    formData.append('oscBase', oscBase);
    formData.append('min', min);
    formData.append('max', max);
    formData.append('hysteresis', hysteresis ? 'true' : 'false');
    formData.append('oscFormat', oscFormat);
    formData.append('filterIntensity', filterIntensity);
    
    try {
        const r = await fetch('/api/mux/add', { method: 'POST', body: formData });
        const d = await r.json();
        if (d.status === 'ok') {
            $('#muxMsg').textContent = 'Multiplexeur enregistre!';
            $('#muxMsg').style.color = '#10b981';
            hideMuxForm();
            loadMuxList();
            loadConfiguredPins(); /* Recharger les pins configurées */
            loadCaps(); /* Recharger les pins */
        } else {
            $('#muxMsg').textContent = 'Erreur: ' + (d.error || 'Inconnu');
            $('#muxMsg').style.color = '#ef4444';
        }
    } catch(e) {
        $('#muxMsg').textContent = 'Erreur reseau';
        $('#muxMsg').style.color = '#ef4444';
    }
}

async function deleteMux(id, event) {
    if (event) event.stopPropagation();
    if (!confirm('Supprimer le multiplexeur MUX' + id + '?')) return;
    const formData = new URLSearchParams();
    formData.append('id', id);
    try {
        await fetch('/api/mux/delete', { method: 'POST', body: formData });
        await loadMuxList(); /* Attendre que la liste soit mise à jour */
        loadCaps();
        /* Si le formulaire MUX est ouvert, mettre à jour la liste déroulante */
        const overlay = $('#muxModalOverlay');
        if (overlay && overlay.classList.contains('active')) {
            populateMuxPinSelects();
        }
    } catch(e) { console.log('Erreur suppression mux:', e); }
}

function initMuxForm() {
    const form = $('#muxForm');
    if (form) form.onsubmit = saveMux;
}
