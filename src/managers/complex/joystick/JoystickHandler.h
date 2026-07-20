#pragma once

#include "../ComplexHandler.h"
#include "../../ComponentManager.h"
#include <Preferences.h>

/**
 * @file JoystickHandler.h
 * @brief Handler pour le joystick 2 axes
 * 
 * Gère la configuration et le stockage du joystick avec ses deux GPIO (X et Y).
 */

class JoystickHandler : public ComplexHandler {
public:
    JoystickHandler();
    virtual ~JoystickHandler() = default;
    
    const char* getComponentId() const override {
        return "joystick";
    }
    
    bool addComponent(const ComplexComponentData& data) override;
    bool removeComponent(const char* pinLabel, uint8_t mainPinGpio) override;
    bool getComponentInfo(const char* pinLabel, uint8_t mainPinGpio, String& json) override;
    bool isGpioUsed(uint8_t gpio) const override;
    uint8_t getComponentCount() const override;
    
    /**
     * @brief Récupère le GPIO Y pour un joystick donné
     * @param mainPinGpio GPIO de l'axe X (pin principale)
     * @return GPIO de l'axe Y, ou 255 si non trouvé
     */
    uint8_t getYAxisGpio(uint8_t mainPinGpio) const;
    
    /**
     * @brief Enregistre le mapping X -> Y sans ajouter le composant (chargement NVS).
     * Le composant a déjà été ajouté par ConfigLoader.
     */
    void registerYAxis(uint8_t xGpio, uint8_t yGpio);
    
private:
    struct JoystickConfig {
        uint8_t xGpio;      // GPIO axe X (pin principale)
        uint8_t yGpio;      // GPIO axe Y (additionalPin)
        const char* pinLabel; // Label de la pin (ex: "A0")
    };
    
    static constexpr uint8_t MAX_JOYSTICKS = 16;
    JoystickConfig joysticks[MAX_JOYSTICKS];
    uint8_t joystick_count;

    int findJoystickIndex(uint8_t mainPinGpio) const;

};
