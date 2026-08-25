#pragma once

/*
 * Liens réseau sur lesquels l'OSC est diffusé.
 *
 * C'est un masque, pas un choix exclusif : l'AP WiFi, le WiFi infrastructure et
 * le câble USB sont trois interfaces physiques distinctes, et rien n'empêche
 * d'émettre sur les trois. Un broadcast dirigé est envoyé par lien coché ET
 * disponible ; chacun est tenté pour lui-même, un succès sur l'un n'annule pas
 * les autres.
 *
 * Ce module est partagé par OSCManager (calibrage, messages ponctuels) et
 * OSCQueue (capteurs et multiplexeurs), qui portaient chacun leur copie de la
 * même logique — avec les mêmes bugs.
 */

#include <Arduino.h>
#include <OSCMessage.h>
#include <WiFiUdp.h>

namespace osc_links {

enum : uint8_t {
    NONE = 0,
    AP = 1 << 0,   /* point d'accès du NiDMI, 192.168.4.255 */
    STA = 1 << 1,  /* réseau WiFi rejoint, broadcast calculé depuis le masque */
    USB = 1 << 2,  /* câble USB (variant --usb-net), 192.168.7.255 */
    ALL = AP | STA | USB,
};

/* Forme stockée en NVS : "ap+sta+usb". Les anciennes valeurs "ap", "sta" et
   "both" restent comprises, une config existante n'est donc pas perdue. */
uint8_t parseMask(const String& value);
String maskToString(uint8_t mask);

/* Faux hors du variant --usb-net : la case USB n'a alors rien à proposer. */
bool usbCompiled();

/* Vrai si au moins un lien a accepté le message. */
bool broadcast(WiFiUDP& udp, OSCMessage& msg, uint8_t mask, uint16_t port, uint8_t attempts = 3);
bool unicast(WiFiUDP& udp, OSCMessage& msg, const char* ip, uint16_t port, uint8_t attempts = 3);

}  // namespace osc_links
