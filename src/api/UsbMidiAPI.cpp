#include "APICommon.h"
#include "../Globals.h"
#include "../server/ServerCore.h"
#include "../network/UsbMidiManager.h"

void setupUsbMidiAPI(AsyncWebServer& server) {
    // USB-MIDI : activation uniquement via NIDMI_USB_MIDI_ENABLED_AT_COMPILE_TIME (UsbMidiManager.h), pas NVS / API POST.
    server.on("/api/usbmidi/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        const bool supported = serverCore.usbMidi().isSupported();
        const bool enabled = serverCore.usbMidi().isInitialized();
        const bool connected = serverCore.usbMidi().isConnected();
        const bool compiled = nidmi_usb_midi_enabled_at_compile_time();

        String json = "{";
        json += "\"supported\":" + String(supported ? "true" : "false") + ",";
        json += "\"enabled\":" + String(enabled ? "true" : "false") + ",";
        json += "\"connected\":" + String(connected ? "true" : "false") + ",";
        json += "\"compiledEnabled\":" + String(compiled ? "true" : "false") + ",";
        json += "\"savedEnabled\":" + String(compiled ? "true" : "false");
        json += "}";
        request->send(200, "application/json", json);
    });
}
