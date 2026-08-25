#include "OSCLinks.h"

#include "../network/UsbNetBootstrap.h"

#include <WiFi.h>

namespace {

/* Un envoi sur une adresse, avec ses reprises. Pas de log ici : ce code est
   traversé à chaque valeur de capteur depuis le cœur 0, et pas de delay() non
   plus pour la même raison. */
bool send_to(WiFiUDP& udp, OSCMessage& msg, const char* address, uint16_t port, uint8_t attempts) {
    for (uint8_t attempt = 0; attempt < attempts; ++attempt) {
        if (!udp.beginPacket(address, port)) {
            continue;
        }
        msg.send(udp);
        if (udp.endPacket()) {
            return true;
        }
    }
    return false;
}

bool has(uint8_t mask, uint8_t link) {
    return (mask & link) != 0;
}

}  // namespace

namespace osc_links {

uint8_t parseMask(const String& value) {
    String v = value;
    v.trim();
    v.toLowerCase();

    if (v.length() == 0) {
        return AP;
    }
    /* Anciennes valeurs, d'avant les cases à cocher. */
    if (v == "both") {
        return AP | STA;
    }
    if (v == "all") {
        return ALL;
    }

    uint8_t mask = NONE;
    int from = 0;
    while (from <= (int)v.length()) {
        int sep = v.indexOf('+', from);
        if (sep < 0) {
            sep = v.length();
        }
        String token = v.substring(from, sep);
        token.trim();
        if (token == "ap") {
            mask |= AP;
        } else if (token == "sta" || token == "wifi") {
            mask |= STA;
        } else if (token == "usb") {
            mask |= USB;
        }
        from = sep + 1;
    }
    return mask;
}

String maskToString(uint8_t mask) {
    String out;
    if (has(mask, AP)) {
        out += "ap";
    }
    if (has(mask, STA)) {
        if (out.length() > 0) {
            out += '+';
        }
        out += "sta";
    }
    if (has(mask, USB)) {
        if (out.length() > 0) {
            out += '+';
        }
        out += "usb";
    }
    return out;
}

bool usbCompiled() {
    return nidmi_usbnet::enabled();
}

bool broadcast(WiFiUDP& udp, OSCMessage& msg, uint8_t mask, uint16_t port, uint8_t attempts) {
    bool sent = false;

    if (has(mask, AP)) {
        /* L'AP du NiDMI est toujours à 192.168.4.1/24 (ServerCore le fixe). */
        sent |= send_to(udp, msg, "192.168.4.255", port, attempts);
    }

    if (has(mask, STA) && WiFi.status() == WL_CONNECTED) {
        const IPAddress ip = WiFi.localIP();
        const IPAddress subnet = WiFi.subnetMask();
        const IPAddress bcast(ip[0] | (~subnet[0]),
                              ip[1] | (~subnet[1]),
                              ip[2] | (~subnet[2]),
                              ip[3] | (~subnet[3]));
        sent |= send_to(udp, msg, bcast.toString().c_str(), port, attempts);
    }

    if (has(mask, USB) && nidmi_usbnet::enabled() && nidmi_usbnet::linkUp()) {
        const String bcast = nidmi_usbnet::broadcastAddress();
        if (bcast.length() > 0) {
            sent |= send_to(udp, msg, bcast.c_str(), port, attempts);
        }
    }

    return sent;
}

bool unicast(WiFiUDP& udp, OSCMessage& msg, const char* ip, uint16_t port, uint8_t attempts) {
    if (ip == nullptr || ip[0] == '\0') {
        return false;
    }
    /* Rien de spécifique au lien : une cible en 192.168.7.x part par le câble,
       en 192.168.4.x par l'AP, le routage lwIP s'en charge. */
    return send_to(udp, msg, ip, port, attempts);
}

}  // namespace osc_links
