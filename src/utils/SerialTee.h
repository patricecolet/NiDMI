#pragma once

/*
 * Tee sur Serial : tout ce que le firmware imprime part à la fois sur le port
 * série réel et dans la console web (WebDebugConsole).
 *
 * Raison d'être : le variant --usb-net occupe le câble USB avec NCM + MIDI, il
 * n'y a plus de CDC, et Serial retombe sur l'UART0 matériel (GPIO43/44) que
 * rien n'écoute par défaut. Sans ce tee, seuls les logs passant par
 * DebugManager ou NIDMI_WEB_LOG arrivent dans l'UI ; les ~390 Serial.printf /
 * println directs disséminés dans le firmware sont perdus.
 *
 * Ce header est force-inclus (-include) par scripts/nidmi.sh dans toutes les
 * unités de compilation C++ quand --usb-net est actif. Il neutralise la macro
 * Serial du core et la remplace par un objet global dérivé de Print, qui
 * recopie tout dans le ring de la console web.
 */

#include <Arduino.h>

#include <utility>

/*
 * Capture la référence vers l'objet série réel AVANT de neutraliser la macro.
 * HardwareSerial.h fait « #define Serial Serial0 / HWCDCSerial / USBSerial »
 * selon build.usb_mode et build.cdc_on_boot : ici, Serial désigne encore ce
 * choix-là. Retour déduit pour garder le type concret (begin, available…).
 */
inline auto& nidmi_raw_serial() {
    return Serial;
}

class SerialTee : public Print {
public:
    size_t write(uint8_t c) override;
    size_t write(const uint8_t* buffer, size_t size) override;

    int availableForWrite() override { return nidmi_raw_serial().availableForWrite(); }
    void flush() override { nidmi_raw_serial().flush(); }

    /* Proxys : le firmware et les bibliothèques continuent d'appeler
       Serial.begin(), Serial.available()… sans rien savoir du tee. */
    template <typename... Args>
    void begin(Args&&... args) { nidmi_raw_serial().begin(std::forward<Args>(args)...); }
    void end() { nidmi_raw_serial().end(); }
    int available() { return nidmi_raw_serial().available(); }
    int read() { return nidmi_raw_serial().read(); }
    int peek() { return nidmi_raw_serial().peek(); }
    void setDebugOutput(bool enable) { nidmi_raw_serial().setDebugOutput(enable); }
    explicit operator bool() const { return static_cast<bool>(nidmi_raw_serial()); }
};

/*
 * Statique de fonction : construit à la première utilisation, jamais après.
 * Indispensable — du code imprime pendant l'initialisation statique
 * (ComponentRegistry), et un objet global ne suffirait pas : le script de lien
 * ESP32 ne trie pas .init_array par priorité (init_priority(101) atterrit vers
 * la 61e entrée sur 88), donc un appel plus précoce trouverait un vptr nul.
 *
 * Conséquence à connaître : decltype(Serial) devient une référence et non un
 * type d'objet. Aucune unité compilée ici ne s'en sert, mais des bibliothèques
 * qui le font existent (OSC/SLIPEncodedSerial.h, Control_Surface) : si l'une
 * d'elles entre dans le build, c'est le point à vérifier.
 */
inline SerialTee& nidmi_serial_tee() {
    static SerialTee tee;
    return tee;
}

#undef Serial
#define Serial nidmi_serial_tee()

/* Écriture sur le port réel sans repasser par la console web. Réservé aux
   endroits qui poussent déjà la ligne dans le ring eux-mêmes, sinon elle y
   apparaîtrait deux fois. */
#define NIDMI_RAW_SERIAL nidmi_raw_serial()
