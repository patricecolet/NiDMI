#include "APICommon.h"
#include "../Globals.h"
#include "../server/ServerCore.h"
#include "../server/ServerCallbacks.h"
#include "../midi/MidiRouter.h"
#include <Preferences.h>
#include <Esp.h>

void setupUsbMidiAPI(AsyncWebServer& server) {
    // API - Activation/Désactivation USB MIDI
    server.on("/api/usbmidi/enable", HTTP_POST, [](AsyncWebServerRequest *request){
        if(request->hasParam("enable", true)){
            String enabled = request->getParam("enable", true)->value();
            bool isEnabled = (enabled == "true");
            
            // Sauvegarder l'état en NVS
            Preferences preferences;
            preferences.begin("nidmi", false);
            preferences.putBool("usbmidi_enabled", isEnabled);
            preferences.end();
            
            // Activer/désactiver USB MIDI dans le routeur
            g_midiRouter.enableUsbMidi(isEnabled);
            
            if(!isEnabled) {
                serverCore.usbMidi().stop();
            }
            // Ne pas appeler begin() ici après une activation : si le boot a fait beginCdc()
            // seul (USB-MIDI désactivé en NVS), USB.begin() a déjà été appelé sans interface
            // MIDI ; ajouter USBMIDI après coup ne met pas à jour le descripteur composite.
            // Un redémarrage charge la NVS et MidiRouter::begin() appelle begin() une fois
            // avec CDC + MIDI depuis le premier USB.begin().
            
            const bool willReboot = serverCore.usbMidi().isSupported();
            String body = "{\"status\":\"ok\",\"reboot\":";
            body += willReboot ? "true" : "false";
            body += "}";
            request->send(200, "application/json", body);
            
            if(willReboot) {
                Serial.println(isEnabled
                    ? "[USB-MIDI] Activation: reboot différé (composite CDC+MIDI au prochain boot)"
                    : "[USB-MIDI] Désactivation: reboot différé demandé");
                nidmi_requestReboot();
            }
        } else {
            request->send(400, "application/json", "{\"error\":\"enable parameter required\"}");
        }
    });
    
    // API - Statut USB MIDI
    server.on("/api/usbmidi/status", HTTP_GET, [](AsyncWebServerRequest *request){
        bool supported = serverCore.usbMidi().isSupported();
        bool enabled = serverCore.usbMidi().isInitialized();
        bool connected = serverCore.usbMidi().isConnected();
        
        Preferences preferences;
        preferences.begin("nidmi", true);
        bool savedEnabled = preferences.getBool("usbmidi_enabled", true);
        preferences.end();
        
        String json = "{";
        json += "\"supported\":" + String(supported ? "true" : "false") + ",";
        json += "\"enabled\":" + String(enabled ? "true" : "false") + ",";
        json += "\"connected\":" + String(connected ? "true" : "false") + ",";
        json += "\"savedEnabled\":" + String(savedEnabled ? "true" : "false");
        json += "}";
        request->send(200, "application/json", json);
    });
}
