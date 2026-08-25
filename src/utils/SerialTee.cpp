/* Variant --usb-net uniquement : ailleurs, Serial sort par un port bien réel et
   le détournement n'a pas lieu d'être. Voir SerialTee.h. */
#if NIDMI_USB_NET

#include "SerialTee.h"

#include "../server/WebDebugConsole.h"

#include <cstring>

namespace {

constexpr size_t kLineCap = 200;

char g_line[kLineCap];
size_t g_len = 0;
portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;

/*
 * Garde-fou anti-récursion : nidmi_web_debug_append_line pousse la ligne sur le
 * WebSocket, et AsyncTCP peut imprimer à son tour depuis la même pile. Sans ce
 * drapeau la récursion est infinie. Effet de bord assumé : une ligne émise par
 * une autre tâche pendant cette fenêtre n'atteint pas la console (elle part
 * quand même sur le port série).
 */
volatile bool g_flushing = false;

void capture(uint8_t c) {
    if (c == '\r') {
        return;
    }

    char ready[kLineCap];
    bool complete = false;

    portENTER_CRITICAL(&g_mux);
    if (c == '\n') {
        if (g_len > 0) {
            memcpy(ready, g_line, g_len);
            ready[g_len] = '\0';
            complete = true;
        }
        g_len = 0;
    } else {
        g_line[g_len++] = static_cast<char>(c);
        if (g_len == kLineCap - 1) {
            /* Ligne plus longue que le ring : on coupe ici plutôt que de
               perdre la suite. */
            memcpy(ready, g_line, g_len);
            ready[g_len] = '\0';
            complete = true;
            g_len = 0;
        }
    }
    portEXIT_CRITICAL(&g_mux);

    if (complete && !g_flushing) {
        g_flushing = true;
        nidmi_web_debug_append_line(ready);
        g_flushing = false;
    }
}

}  // namespace

size_t SerialTee::write(uint8_t c) {
    const size_t written = nidmi_raw_serial().write(c);
    capture(c);
    return written;
}

size_t SerialTee::write(const uint8_t* buffer, size_t size) {
    const size_t written = nidmi_raw_serial().write(buffer, size);
    for (size_t i = 0; i < size; ++i) {
        capture(buffer[i]);
    }
    return written;
}

#endif  // NIDMI_USB_NET
