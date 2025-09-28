# Synchronisation Pins via WebSocket

## Principe
- Pin détectée → Vérifier NVS → Si rien : valeurs par défaut
- Interface web synchronisée en temps réel
- Messages WebSocket pour les changements

## Structure des messages (Templates simples)

### Message `PIN_CLICKED`
```
PIN_CLICKED:A0
```
**Usage :** Interface web → C++ (pin cliquée)

### Message `PIN_CONFIG`
```
PIN_CONFIG:A0:{"role":"Potentiomètre","rtpEnabled":true,"rtpType":"Control Change","rtpCc":7,"rtpChan":1}
```
**Usage :** C++ → Interface web (config NVS OU valeurs par défaut complètes)


## Valeurs par défaut par pin (uniques)

### Pins analogiques (A0-A3)
```json
// A0
{
  "role": "Potentiomètre",
  "rtpEnabled": true,
  "rtpType": "Control Change",
  "rtpCc": 1,
  "rtpChan": 1,
  "potFilter": "lowpass",
  "oscEnabled": true,
  "oscAddress": "/ctl",
  "dbgEnabled": false,
  "dbgHeader": ""
}

// A1
{
  "role": "Potentiomètre",
  "rtpEnabled": true,
  "rtpType": "Control Change",
  "rtpCc": 2,
  "rtpChan": 1,
  "potFilter": "lowpass",
  "oscEnabled": true,
  "oscAddress": "/ctl",
  "dbgEnabled": false,
  "dbgHeader": ""
}

// A2
{
  "role": "Potentiomètre",
  "rtpEnabled": true,
  "rtpType": "Control Change",
  "rtpCc": 3,
  "rtpChan": 1,
  "potFilter": "lowpass",
  "oscEnabled": true,
  "oscAddress": "/ctl",
  "dbgEnabled": false,
  "dbgHeader": ""
}

// A3
{
  "role": "Potentiomètre",
  "rtpEnabled": true,
  "rtpType": "Control Change",
  "rtpCc": 4,
  "rtpChan": 1,
  "potFilter": "lowpass",
  "oscEnabled": true,
  "oscAddress": "/ctl",
  "dbgEnabled": false,
  "dbgHeader": ""
}
```

### Pins digitales (D0-D3)
```json
// D0
{
  "role": "Bouton",
  "rtpEnabled": true,
  "rtpType": "Note",
  "rtpNote": 60,
  "rtpChan": 1,
  "btnMode": "pulse",
  "oscEnabled": true,
  "oscAddress": "/note",
  "dbgEnabled": false,
  "dbgHeader": ""
}

// D1
{
  "role": "Bouton",
  "rtpEnabled": true,
  "rtpType": "Note",
  "rtpNote": 61,
  "rtpChan": 1,
  "btnMode": "pulse",
  "oscEnabled": true,
  "oscAddress": "/note",
  "dbgEnabled": false,
  "dbgHeader": ""
}

// D2
{
  "role": "Bouton",
  "rtpEnabled": true,
  "rtpType": "Note",
  "rtpNote": 62,
  "rtpChan": 1,
  "btnMode": "pulse",
  "oscEnabled": true,
  "oscAddress": "/note",
  "dbgEnabled": false,
  "dbgHeader": ""
}

// D3
{
  "role": "Bouton",
  "rtpEnabled": true,
  "rtpType": "Note",
  "rtpNote": 63,
  "rtpChan": 1,
  "btnMode": "pulse",
  "oscEnabled": true,
  "oscAddress": "/note",
  "dbgEnabled": false,
  "dbgHeader": ""
}
```

### LEDs spéciales (D7-D10)
```json
// D7 - LED on/off
{
  "role": "LED",
  "rtpEnabled": true,
  "rtpType": "Note",
  "rtpNote": 36,
  "rtpChan": 1,
  "ledMode": "onoff",
  "oscEnabled": true,
  "oscAddress": "/note",
  "dbgEnabled": false,
  "dbgHeader": ""
}

// D8 - LED on/off
{
  "role": "LED",
  "rtpEnabled": true,
  "rtpType": "Note",
  "rtpNote": 37,
  "rtpChan": 1,
  "ledMode": "onoff",
  "oscEnabled": true,
  "oscAddress": "/note",
  "dbgEnabled": false,
  "dbgHeader": ""
}

// D9 - LED on/off
{
  "role": "LED",
  "rtpEnabled": true,
  "rtpType": "Note",
  "rtpNote": 38,
  "rtpChan": 1,
  "ledMode": "onoff",
  "oscEnabled": true,
  "oscAddress": "/note",
  "dbgEnabled": false,
  "dbgHeader": ""
}

// D10 - LED PWM
{
  "role": "LED",
  "rtpEnabled": true,
  "rtpType": "Control Change",
  "rtpCc": 10,
  "rtpChan": 1,
  "ledMode": "pwm",
  "oscEnabled": true,
  "oscAddress": "/ctl",
  "dbgEnabled": false,
  "dbgHeader": ""
}
```

### Bus (SDA, SCL, MOSI, MISO, SCK, TX, RX)
```json
{
  "role": "I2C/SPI/UART",
  "rtpEnabled": false,
  "oscEnabled": true,
  "oscAddress": "/ctl",
  "dbgEnabled": false,
  "dbgHeader": ""
}
```

## Flux de synchronisation

1. **Interface web** → Envoie `PIN_CLICKED:A0`
2. **C++** → Vérifie NVS avec `preferences.getString("pin_A0", "")`
3. **Si config NVS existe** → Envoie `PIN_CONFIG:A0:{config NVS complète}`
4. **Si pas de config NVS** → Envoie `PIN_CONFIG:A0:{valeurs par défaut complètes}`
5. **Interface web** → Met à jour liste et formulaires
6. **Utilisateur modifie** → Bouton "Enregistrer tout" → API `/api/pins/set` → Sauvegarde NVS
7. **Interface web** → Affiche "Toutes les configurations enregistrées" via `#saveAllMsg`

**Note :** Pour "supprimer" une pin, on fait simplement `PIN_CLICKED:A0` → `PIN_CONFIG:A0:{valeurs par défaut}` (remet à zéro)

## Intégration avec WebAPI.cpp existant

Le système WebSocket existe déjà avec `sendRtpStatus()`. 
On peut étendre avec des fonctions similaires :

```cpp
// Gestionnaire WebSocket pour les pins
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_DATA) {
        String message = String((char*)data);
        
        if (message.startsWith("PIN_CLICKED:")) {
            String pin = message.substring(12);
            
            // Vérifier NVS (compatible avec système existant)
            Preferences preferences;
            preferences.begin("esp32server", true);
            String key = "pin_" + pin;
            String config = preferences.getString(key.c_str(), "");
            preferences.end();
            
            if (config.length() > 0) {
                // Config trouvée → Envoyer config NVS
                String msg = "PIN_CONFIG:" + pin + ":" + config;
                client->text(msg);
            } else {
                // Pas de config → Envoyer valeurs par défaut complètes
                String defaultConfig = getDefaultConfig(pin);
                String msg = "PIN_CONFIG:" + pin + ":" + defaultConfig;
                client->text(msg);
            }
            
            // Note: La confirmation de sauvegarde se fait via l'API HTTP
            // et l'affichage #saveAllMsg existant, pas besoin de WebSocket
        }
    }
}
```
