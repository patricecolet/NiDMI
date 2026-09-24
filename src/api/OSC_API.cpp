#include "APICommon.h"
#include "../server/ServerCallbacks.h"
#include "../osc/OSCLinks.h"
#include "../osc/OSCConfigLoader.h"
#include <Preferences.h>

void setupOSC_API(AsyncWebServer& server) {
    /* API - Activer / désactiver la sortie OSC globale (toutes les pins + MUX), NVS + rechargement runtime */
    server.on("/api/osc/output-enable", HTTP_POST, [](AsyncWebServerRequest *request){
        if (!request->hasParam("enable", true)) {
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"enable required\"}");
            return;
        }
        bool enabled = request->getParam("enable", true)->value() == "true";
        Preferences preferences;
        preferences.begin("nidmi", false);
        preferences.putBool("osc_out_all", enabled);
        preferences.end();
        nidmi_requestReloadPins();
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    /* API - Statut OSC */
    server.on("/api/osc/status", HTTP_GET, [](AsyncWebServerRequest *request){
        /* Relue par le même chargeur que l'émetteur : la page montre ce que fait
           la carte, valeurs de repli comprises. */
        const OSCConfigLoader::OSCConfig config = OSCConfigLoader::loadFromNVS();
        Preferences preferences;
        preferences.begin("nidmi", true);
        bool outputAll = preferences.getBool("osc_out_all", true);
        preferences.end();
        const String& ip = config.ip;
        String json = "{";
        json += "\"target\":\"" + config.target + "\",";
        json += "\"port\":" + String(config.port) + ",";
        json += "\"broadcast\":" + String(config.broadcast ? "true" : "false") + ",";
        json += "\"interface\":\"" + osc_links::maskToString(config.links) + "\",";
        /* L'IP unicast n'était pas renvoyée : le champ du formulaire repartait
           vide à chaque chargement de page. */
        json += "\"ip\":\"" + ip + "\",";
        /* Sans le variant --usb-net, l'UI n'a pas à proposer la case USB. */
        json += "\"usb_link\":" + String(osc_links::usbCompiled() ? "true" : "false") + ",";
        json += "\"output_all_enabled\":" + String(outputAll ? "true" : "false");
        json += "}";
        request->send(200, "application/json", json);
    });

    /* APRÈS les sous-routes : ESPAsyncWebServer apparie par préfixe, et « /api/osc » déclaré
       en premier capturait « /api/osc/output-enable » (réponse « target and port required »). */
    /* API - Configuration OSC */
    server.on("/api/osc", HTTP_POST, [](AsyncWebServerRequest *request){
        if(request->hasParam("target", true) && request->hasParam("port", true)){
            String target = request->getParam("target", true)->value();
            int port = request->getParam("port", true)->value().toInt();
            bool broadcast = request->hasParam("broadcast", true) && 
                           request->getParam("broadcast", true)->value() == "true";
            /* Liens de diffusion cochés dans l'UI, normalisés ici : "ap+sta+usb". */
            String interface = request->hasParam("interface", true) ?
                             request->getParam("interface", true)->value() : "ap";
            interface = osc_links::maskToString(osc_links::parseMask(interface));

            /* Sauvegarder en NVS */
            Preferences preferences;
            preferences.begin("nidmi", false);
            preferences.putString("osc_target", target);
            preferences.putInt("osc_port", port);
            preferences.putBool("osc_broadcast", broadcast);
            preferences.putString("osc_interface", interface);
            /* L'IP unicast n'était jamais écrite : le firmware repartait donc
               toujours sur la valeur par défaut, quoi qu'on ait saisi. */
            if (request->hasParam("ip", true)) {
                preferences.putString("osc_ip", request->getParam("ip", true)->value());
            }
            preferences.end();

            nidmi_requestReloadOsc();
            
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"target and port required\"}");
        }
    });
}
