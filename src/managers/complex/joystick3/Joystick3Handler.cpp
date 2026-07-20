#include "Joystick3Handler.h"
#include "../../ComponentManager.h"  /* Doit être avant Globals.h pour la définition complète */
#include "../../../Globals.h"
#include "../../../components/ComponentTypes.h"
#include "../../../utils/PinMapper.h"
#include <Preferences.h>
#include <cstring>

Joystick3Handler::Joystick3Handler() : joystick_count(0) {
    // Initialiser le tableau
    for (uint8_t i = 0; i < MAX_JOYSTICK3S; i++) {
        joysticks[i].xGpio = 255;
        joysticks[i].yGpio = 255;
        joysticks[i].zGpio = 255;
        joysticks[i].pinLabel = nullptr;
    }
}

bool Joystick3Handler::addComponent(const ComplexComponentData& data) {
    if (!data.def || !data.def->id || strcmp(data.def->id, "joystick3") != 0) {
        return false;
    }

    // Trouver les GPIO Y et Z depuis additionalPins
    uint8_t yGpio = 255;
    uint8_t zGpio = 255;
    for (uint8_t i = 0; i < data.additionalPinCount; i++) {
        if (data.additionalPins[i].id && strcmp(data.additionalPins[i].id, "joyYPin") == 0) {
            yGpio = data.additionalPins[i].gpio;
        } else if (data.additionalPins[i].id && strcmp(data.additionalPins[i].id, "joyZPin") == 0) {
            zGpio = data.additionalPins[i].gpio;
        }
    }

    if (yGpio == 255 || zGpio == 255) {
        return false; // GPIO Y et Z requis
    }

    // Vérifier si le joystick existe déjà
    int index = findJoystickIndex(data.mainPinGpio);
    if (index >= 0) {
        // Mettre à jour
        joysticks[index].yGpio = yGpio;
        joysticks[index].zGpio = zGpio;
        return true;
    }

    // Ajouter nouveau joystick
    if (joystick_count >= MAX_JOYSTICK3S) {
        return false;
    }

    joysticks[joystick_count].xGpio = data.mainPinGpio;
    joysticks[joystick_count].yGpio = yGpio;
    joysticks[joystick_count].zGpio = zGpio;
    joysticks[joystick_count].pinLabel = data.pinLabel;
    joystick_count++;

    // Ajouter le composant X dans ComponentManager
    // Les composants Y et Z sont gérés séparément via le processor
    g_componentManager.addComponent(
        data.mainPinGpio,
        ComponentType::JOYSTICK3,
        0, // midi_param sera configuré plus tard
        1, // channel par défaut
        MidiMessageType::CONTROL_CHANGE
    );

    return true;
}

bool Joystick3Handler::removeComponent(const char* pinLabel, uint8_t mainPinGpio) {
    int index = findJoystickIndex(mainPinGpio);
    if (index < 0) {
        return false;
    }

    // Retirer de ComponentManager
    g_componentManager.removeComponent(mainPinGpio);

    // Décaler les éléments suivants
    for (int i = index; i < joystick_count - 1; i++) {
        joysticks[i] = joysticks[i + 1];
    }
    joystick_count--;
    joysticks[joystick_count].xGpio = 255;
    joysticks[joystick_count].yGpio = 255;
    joysticks[joystick_count].zGpio = 255;
    joysticks[joystick_count].pinLabel = nullptr;

    return true;
}

bool Joystick3Handler::getComponentInfo(const char* pinLabel, uint8_t mainPinGpio, String& json) {
    int index = findJoystickIndex(mainPinGpio);
    if (index < 0) {
        return false;
    }

    json += ",\"joyYPin\":" + String(joysticks[index].yGpio);
    json += ",\"joyZPin\":" + String(joysticks[index].zGpio);
    return true;
}

bool Joystick3Handler::isGpioUsed(uint8_t gpio) const {
    for (uint8_t i = 0; i < joystick_count; i++) {
        if (joysticks[i].xGpio == gpio || joysticks[i].yGpio == gpio || joysticks[i].zGpio == gpio) {
            return true;
        }
    }
    return false;
}

uint8_t Joystick3Handler::getComponentCount() const {
    return joystick_count;
}

uint8_t Joystick3Handler::getYAxisGpio(uint8_t mainPinGpio) const {
    int index = findJoystickIndex(mainPinGpio);
    if (index < 0) {
        return 255;
    }
    return joysticks[index].yGpio;
}

uint8_t Joystick3Handler::getZAxisGpio(uint8_t mainPinGpio) const {
    int index = findJoystickIndex(mainPinGpio);
    if (index < 0) {
        return 255;
    }
    return joysticks[index].zGpio;
}

void Joystick3Handler::registerAxes(uint8_t xGpio, uint8_t yGpio, uint8_t zGpio) {
    if (yGpio == 255 || zGpio == 255) return;
    int index = findJoystickIndex(xGpio);
    if (index >= 0) {
        joysticks[index].yGpio = yGpio;
        joysticks[index].zGpio = zGpio;
    } else {
        if (joystick_count >= MAX_JOYSTICK3S) return;
        joysticks[joystick_count].xGpio = xGpio;
        joysticks[joystick_count].yGpio = yGpio;
        joysticks[joystick_count].zGpio = zGpio;
        joysticks[joystick_count].pinLabel = nullptr;
        joystick_count++;
    }
}

int Joystick3Handler::findJoystickIndex(uint8_t mainPinGpio) const {
    for (uint8_t i = 0; i < joystick_count; i++) {
        if (joysticks[i].xGpio == mainPinGpio) {
            return i;
        }
    }
    return -1;
}
