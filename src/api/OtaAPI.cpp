#include "APICommon.h"
#include "../server/ServerCallbacks.h"   // nidmi_requestReboot
#include "../server/WebDebugConsole.h"   // NIDMI_WEB_LOG
#include <Update.h>
#include <esp_ota_ops.h>

/*
 * OTA embarqué : POST /api/ota reçoit l'image firmware (le .bin "app", PAS le
 * .merged.bin) en CORPS BRUT (application/octet-stream), l'écrit dans le slot
 * OTA libre via la lib Update, puis reboote en différé pour booter dessus.
 *
 * Intégrité : on envoie le binaire brut (et non du multipart) pour que le
 * Content-Length = taille exacte du firmware. On passe cette taille à
 * Update.begin(total) ; si l'upload est tronqué (coupure réseau), la dernière
 * frame (index+len == total) n'arrive jamais -> Update.end() n'est pas validé
 * -> la partition n'est PAS marquée bootable -> la carte reste sur le firmware
 * actuel (pas de brick). C'est le correctif après un upload tronqué qui avait
 * booté une image partielle.
 *
 * Prérequis : table de partitions avec 2 slots app (build --ota /
 * nidmi_s3_ota_dual_littlefs). Sans slot OTA, Update.begin() échoue.
 * Progression suivie côté navigateur (XHR upload).
 *
 * Sécurité : endpoint non authentifié (usage atelier sur réseau isolé / AP).
 * À protéger (token/mot de passe) si exposé sur un réseau ouvert.
 */
static bool s_otaOk = false;
/* Rejet précoce : renseigné dès le premier fragment quand l'image est refusée.
   Sert à ignorer les fragments suivants ET à empêcher onRequest de répondre une
   seconde fois (la réponse est déjà partie depuis onBody). */
static bool s_otaRejected = false;

/* Le .merged.bin (bootloader + partitions + app) commence lui aussi par 0xE9 :
   le seul discriminateur fiable est le mot magique du descripteur applicatif,
   place juste apres l'en-tete d'image et le premier en-tete de segment. */
static const size_t APP_DESC_OFFSET = 32;
static const uint32_t APP_DESC_MAGIC = 0xABCD5432;

/* Refuse l'image et coupe la connexion, au lieu d'avaler le reste du corps.
   Sans cette coupure, un .merged.bin de 8 Mo traverse le lien en entier pour
   etre jete fragment par fragment — six fois le volume d'un upload legitime,
   sans contre-pression, ce qui suffit a faire tomber le lien reseau USB. */
static void otaReject(AsyncWebServerRequest* request, int code, const String& message) {
    s_otaRejected = true;
    if (Update.isRunning()) Update.abort();
    Serial.printf("[OTA] Refuse: %s\n", message.c_str());
    NIDMI_WEB_LOG("[OTA] Refuse: %s", message.c_str());
    String body = "{\"status\":\"error\",\"error\":\"" + message + "\"}";
    /* Ne PAS fermer la connexion nous-memes : request->send() met la reponse en file
       d'attente et l'ecrit de maniere asynchrone ; un close() immediat la jette et le
       client ne recoit qu'un "empty reply", sans le message qui explique le refus.
       L'en-tete Connection: close fait fermer le serveur une fois la reponse vidée,
       ce qui interrompt aussi l'envoi du client — le but recherche. */
    AsyncWebServerResponse* response = request->beginResponse(code, "application/json", body);
    response->addHeader("Connection", "close");
    request->send(response);
}

void setupOtaAPI(AsyncWebServer& server) {
    server.on("/api/ota", HTTP_POST,
        // onRequest : appelé une fois tout le corps reçu (uploads complets seulement)
        [](AsyncWebServerRequest *request) {
            if (s_otaRejected) {
                /* onBody a deja repondu et ferme : ne pas envoyer une 2e reponse. */
                s_otaRejected = false;
                s_otaOk = false;
                return;
            }
            if (s_otaOk && Update.isFinished() && !Update.hasError()) {
                request->send(200, "application/json", "{\"status\":\"ok\",\"reboot\":true}");
                NIDMI_WEB_LOG("[OTA] Image validée, redémarrage...");
                nidmi_requestReboot();  // reboot différé (laisse partir la réponse)
            } else {
                String err = Update.hasError() ? String(Update.errorString())
                                               : String("upload incomplet ou invalide");
                if (Update.isRunning()) Update.abort();
                request->send(500, "application/json",
                              "{\"status\":\"error\",\"error\":\"" + err + "\"}");
                NIDMI_WEB_LOG("[OTA] Echec: %s", err.c_str());
            }
            s_otaOk = false;
        },
        NULL,  // pas de handler multipart : on lit le corps brut ci-dessous
        // onBody : fragments du corps brut ; total = Content-Length = taille firmware
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                s_otaOk = false;
                s_otaRejected = false;
                Serial.printf("[OTA] Debut upload: %u octets attendus\n", (unsigned)total);
                NIDMI_WEB_LOG("[OTA] Debut upload: %u octets", (unsigned)total);

                if (total == 0) {
                    otaReject(request, 400, "corps vide");
                    return;
                }

                /* Verifier la taille AVANT d'ecrire : message explicite plutot que
                   l'echec opaque d'Update.begin(). */
                const esp_partition_t* slot = esp_ota_get_next_update_partition(NULL);
                if (!slot) {
                    otaReject(request, 500, "aucun slot OTA (table de partitions sans --ota)");
                    return;
                }
                if (total > slot->size) {
                    char msg[160];
                    snprintf(msg, sizeof(msg),
                             "image de %u Ko pour un slot de %u Ko - c'est probablement un .merged.bin, "
                             "il faut le .bin applicatif",
                             (unsigned)(total / 1024), (unsigned)(slot->size / 1024));
                    otaReject(request, 413, msg);
                    return;
                }

                /* Verifier que c'est bien une image applicative et pas une image flash
                   complete : les deux commencent par 0xE9, seul le descripteur distingue. */
                if (len < APP_DESC_OFFSET + sizeof(uint32_t)) {
                    otaReject(request, 400, "premier fragment trop court pour valider l'image");
                    return;
                }
                if (data[0] != 0xE9) {
                    otaReject(request, 400, "ce n'est pas une image ESP32 (magic 0xE9 absent)");
                    return;
                }
                uint32_t descMagic;
                memcpy(&descMagic, data + APP_DESC_OFFSET, sizeof(descMagic));
                if (descMagic != APP_DESC_MAGIC) {
                    otaReject(request, 400,
                              "image sans descripteur applicatif - bootloader ou .merged.bin, "
                              "il faut le .bin applicatif");
                    return;
                }

                if (!Update.begin(total, U_FLASH)) {
                    Update.printError(Serial);
                    otaReject(request, 500, String("Update.begin: ") + Update.errorString());
                    return;
                }
            }

            if (s_otaRejected) return;  /* image deja refusee : ne pas avaler la suite */

            if (Update.isRunning() && len) {
                if (Update.write(data, len) != len) {
                    Update.printError(Serial);
                    return;
                }
            }
            // Dernière frame uniquement si on a bien reçu tout le corps annoncé
            if (total > 0 && index + len == total) {
                if (Update.end(true)) {
                    s_otaOk = true;
                    Serial.printf("[OTA] Termine: %u octets\n", (unsigned)total);
                } else {
                    Update.printError(Serial);
                }
            }
        }
    );
}
