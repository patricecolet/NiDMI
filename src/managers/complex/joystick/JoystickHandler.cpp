#include "JoystickHandler.h"
#include "../../ComponentManager.h"  /* Doit être avant Globals.h pour la définition complète */
#include "../../../Globals.h"
#include "../../../components/ComponentTypes.h"
#include "../../../utils/PinMapper.h"
#include <Preferences.h>
#include <cstring>

JoystickHandler::JoystickHandler() : joystick_count(0) {
    // Initialiser le tableau
    for (uint8_t i = 0; i < MAX_JOYSTICKS; i++) {
        joysticks[i].xGpio = 255;
        joysticks[i].yGpio = 255;
        joysticks[i].pinLabel = nullptr;
    }
}

bool JoystickHandler::addComponent(const ComplexComponentData& data) {
    if (!data.def || !data.def->id || strcmp(data.def->id, "joystick") != 0) {
        return false;
    }
    
    // Trouver le GPIO Y depuis additionalPins
    uint8_t yGpio = 255;
    for (uint8_t i = 0; i < data.additionalPinCount; i++) {
        if (data.additionalPins[i].id && strcmp(data.additionalPins[i].id, "joyYPin") == 0) {
            yGpio = data.additionalPins[i].gpio;
            break;
        }
    }
    
    if (yGpio == 255) {
        return false; // GPIO Y requis
    }
    
    // Vérifier si le joystick existe déjà
    int index = findJoystickIndex(data.mainPinGpio);
    if (index >= 0) {
        // Mettre à jour
        joysticks[index].yGpio = yGpio;
        return true;
    }
    
    // Ajouter nouveau joystick
    if (joystick_count >= MAX_JOYSTICKS) {
        return false;
    }
    
    joysticks[joystick_count].xGpio = data.mainPinGpio;
    joysticks[joystick_count].yGpio = yGpio;
    joysticks[joystick_count].pinLabel = data.pinLabel;
    joystick_count++;
    
    // Ajouter le composant X dans ComponentManager
    // Le composant Y sera géré séparément ou via le processor
    g_componentManager.addComponent(
        data.mainPinGpio,
        ComponentType::JOYSTICK,
        0, // midi_param sera configuré plus tard
        1, // channel par défaut
        MidiMessageType::CONTROL_CHANGE
    );
    
    return true;
}

bool JoystickHandler::removeComponent(const char* pinLabel, uint8_t mainPinGpio) {
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
    joysticks[joystick_count].pinLabel = nullptr;
    
    return true;
}

bool JoystickHandler::getComponentInfo(const char* pinLabel, uint8_t mainPinGpio, String& json) {
    int index = findJoystickIndex(mainPinGpio);
    if (index < 0) {
        return false;
    }
    
    json += ",\"joyYPin\":" + String(joysticks[index].yGpio);
    return true;
}

bool JoystickHandler::isGpioUsed(uint8_t gpio) const {
    for (uint8_t i = 0; i < joystick_count; i++) {
        if (joysticks[i].xGpio == gpio || joysticks[i].yGpio == gpio) {
            return true;
        }
    }
    return false;
}

uint8_t JoystickHandler::getComponentCount() const {
    return joystick_count;
}

uint8_t JoystickHandler::getYAxisGpio(uint8_t mainPinGpio) const {
    int index = findJoystickIndex(mainPinGpio);
    if (index < 0) {
        return 255;
    }
    return joysticks[index].yGpio;
}

void JoystickHandler::registerYAxis(uint8_t xGpio, uint8_t yGpio) {
    if (yGpio == 255) return;
    int index = findJoystickIndex(xGpio);
    if (index >= 0) {
        joysticks[index].yGpio = yGpio;
        return;
    }
    if (joystick_count >= MAX_JOYSTICKS) return;
    joysticks[joystick_count].xGpio = xGpio;
    joysticks[joystick_count].yGpio = yGpio;
    joysticks[joystick_count].pinLabel = nullptr;
    joystick_count++;
}

int JoystickHandler::findJoystickIndex(uint8_t mainPinGpio) const {
    for (uint8_t i = 0; i < joystick_count; i++) {
        if (joysticks[i].xGpio == mainPinGpio) {
            return i;
        }
    }
    return -1;
}
