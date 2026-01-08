/* WebSocket pour synchronisation avec C++ */
let websocket = null;

/* Fonction pour appliquer la logique de remplacement */
function applyPinReplacementLogic(pin) {
    /* Pins analogiques */
    if (pin.startsWith('A')) {
        /* Supprimer le pin digital correspondant seulement s'il existe */
        const dLabel = pin.replace('A', 'D');
        if (pcfg[dLabel]) delete pcfg[dLabel];
    }
    /* Pins digitales normales */
    else if (['D0','D1','D2','D3'].includes(pin)) {
        /* Supprimer le pin analogique correspondant seulement s'il existe */
        const aLabel = pin.replace('D', 'A');
        if (pcfg[aLabel]) delete pcfg[aLabel];
    }
    /* Bus I2C - griser SDA,SCL,D4,D5 et supprimer les configs individuelles */
    else if (['SDA','SCL'].includes(pin)) {
        /* Supprimer les pins digitales individuelles pour libérer D4,D5 */
        if (pcfg['D4']) delete pcfg['D4'];
        if (pcfg['D5']) delete pcfg['D5'];
        /* Supprimer les configs SDA/SCL individuelles */
        if (pcfg['SDA']) delete pcfg['SDA'];
        if (pcfg['SCL']) delete pcfg['SCL'];
    }
    /* Bus SPI - griser MOSI,MISO,SCK,D8,D9,D10 et supprimer les configs individuelles */
    else if (['MOSI','MISO','SCK'].includes(pin)) {
        /* Supprimer les pins digitales individuelles pour libérer D8,D9,D10 */
        if (pcfg['D8']) delete pcfg['D8'];
        if (pcfg['D9']) delete pcfg['D9'];
        if (pcfg['D10']) delete pcfg['D10'];
        /* Supprimer les configs MOSI/MISO/SCK individuelles */
        if (pcfg['MOSI']) delete pcfg['MOSI'];
        if (pcfg['MISO']) delete pcfg['MISO'];
        if (pcfg['SCK']) delete pcfg['SCK'];
    }
    /* UART TX */
    else if (pin === 'TX') {
        if (pcfg['D6']) delete pcfg['D6'];
    }
    /* UART RX */
    else if (pin === 'RX') {
        if (pcfg['D7']) delete pcfg['D7'];
    }
    /* Pins digitales de bus I2C */
    else if (['D4','D5'].includes(pin)) {
        if (pcfg['I2C']) delete pcfg['I2C'];
    }
    /* Pins digitales de bus SPI */
    else if (['D8','D9','D10'].includes(pin)) {
        if (pcfg['SPI']) delete pcfg['SPI'];
    }
    /* Pins digitales UART - remplacement simple */
    else if (pin === 'D6') {
        if (pcfg['TX']) delete pcfg['TX'];
    }
    else if (pin === 'D7') {
        if (pcfg['RX']) delete pcfg['RX'];
    }
}

function initWebSocket() {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.host}/ws`;
    websocket = new WebSocket(wsUrl);
    
    websocket.onopen = function() {
        console.log('WebSocket connected');
    };
    
    websocket.onmessage = function(event) {
        const message = event.data;
        if (message.startsWith('PIN_CONFIG:')) {
            const parts = message.split(':');
            if (parts.length >= 3) {
                const pin = parts[1];
                const config = JSON.parse(parts.slice(2).join(':'));
                
                /* Appliquer la logique de remplacement AVANT d'ajouter la config */
                applyPinReplacementLogic(pin);
                
                /* Appliquer la configuration reçue avec les bonnes clés */
                if (['SDA','SCL'].includes(pin) && config.role === 'I2C') {
                    pcfg['I2C'] = config;  /* Créer clé I2C pour le grisage */
                } else if (['MOSI','MISO','SCK'].includes(pin) && config.role === 'SPI') {
                    pcfg['SPI'] = config;  /* Créer clé SPI pour le grisage */
                } else {
                    pcfg[pin] = config;    /* Clé normale pour les autres pins */
                }
                updatePinsList();
                updateBusVisuals();
                
                /* Si c'est la pin actuellement sélectionnée, mettre à jour l'interface */
                if (cur === pin) {
                    applyCfg(config);
                }
            }
        }
    };
    
    websocket.onclose = function() {
        console.log('WebSocket disconnected');
        /* Tentative de reconnexion après 3 secondes */
        setTimeout(initWebSocket, 3000);
    };
}

function handlePinClick(label){
    /* Vérifier si la pin est bloquée */
    if(prect[label] && prect[label].classList.contains('busDisabled')){
        return;
    }
    
    /* Envoyer message WebSocket au C++ */
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(`PIN_CLICKED:${label}`);
    } else {
        /* Fallback: logique locale si WebSocket pas disponible */
        handlePinClickLocal(label);
    }
}

function handlePinClickLocal(label){
    /* Pins analogiques (y compris MUX) */
    if(label.startsWith('A') || label.startsWith('M')){
        pcfg[label] = {role: 'Potentiomètre', cfg: {}};
        /* Supprimer le pin digital correspondant seulement s'il existe (pour A0-A3) */
        if(label.startsWith('A')){
            const dLabel = label.replace('A', 'D');
            if(pcfg[dLabel]) delete pcfg[dLabel];
        }
    }
    /* Pins digitales normales */
    else if(['D0','D1','D2','D3'].includes(label)){
        pcfg[label] = {role: 'Bouton', cfg: {}};
        /* Supprimer le pin analogique correspondant seulement s'il existe */
        const aLabel = label.replace('D', 'A');
        if(pcfg[aLabel]) delete pcfg[aLabel];
    }
    /* Bus I2C */
    else if(['SDA','SCL'].includes(label)){
        pcfg['I2C'] = {role: 'I2C', cfg: {}};
        /* Supprimer les pins digitales seulement si elles existent */
        if(pcfg['D4']) delete pcfg['D4'];
        if(pcfg['D5']) delete pcfg['D5'];
    }
    /* Bus SPI */
    else if(['MOSI','MISO','SCK'].includes(label)){
        pcfg['SPI'] = {role: 'SPI', cfg: {}};
        /* Supprimer les pins digitales seulement si elles existent */
        if(pcfg['D8']) delete pcfg['D8'];
        if(pcfg['D9']) delete pcfg['D9'];
        if(pcfg['D10']) delete pcfg['D10'];
    }
    /* UART TX */
    else if(label === 'TX'){
        pcfg['TX'] = {role: 'UART', cfg: {}};
        if(pcfg['D6']) delete pcfg['D6'];
    }
    /* UART RX */
    else if(label === 'RX'){
        pcfg['RX'] = {role: 'UART', cfg: {}};
        if(pcfg['D7']) delete pcfg['D7'];
    }
    /* Pins digitales de bus I2C */
    else if(['D4','D5'].includes(label)){
        if(pcfg['I2C']) delete pcfg['I2C'];
        pcfg[label] = {role: 'Bouton', cfg: {}};
    }
    /* Pins digitales de bus SPI */
    else if(['D8','D9','D10'].includes(label)){
        if(pcfg['SPI']) delete pcfg['SPI'];
        pcfg[label] = {role: 'Bouton', cfg: {}};
    }
    /* Pins digitales UART (indépendantes) */
    else if(['D6','D7'].includes(label)){
        /* D6 supprime TX, D7 supprime RX */
        if(label === 'D6' && pcfg['TX']) delete pcfg['TX'];
        if(label === 'D7' && pcfg['RX']) delete pcfg['RX'];
        pcfg[label] = {role: 'Bouton', cfg: {}};
    }
    
    updatePinsList();
    updateBusVisuals();
}
