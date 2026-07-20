#pragma once

#include "../ComplexHandler.h"
#include "../../ComponentManager.h"
#include <Preferences.h>

/**
 * @file Joystick3Handler.h
 * @brief Handler pour le joystick 3 axes
 *
 * Gère la configuration et le stockage du joystick avec ses trois GPIO (X, Y et Z).
 */

class Joystick3Handler : public ComplexHandler {
public:
    Joystick3Handler();
    virtual ~Joystick3Handler() = default;

    const char* getComponentId() const override {
        return "joystick3";
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
     * @brief Récupère le GPIO Z pour un joystick donné
     * @param mainPinGpio GPIO de l'axe X (pin principale)
     * @return GPIO de l'axe Z, ou 255 si non trouvé
     */
    uint8_t getZAxisGpio(uint8_t mainPinGpio) const;

    /**
     * @brief Enregistre le mapping X -> Y/Z sans ajouter le composant (chargement NVS).
     * Le composant a déjà été ajouté par ConfigLoader.
     */
    void registerAxes(uint8_t xGpio, uint8_t yGpio, uint8_t zGpio);

private:
    struct Joystick3Config {
        uint8_t xGpio;        // GPIO axe X (pin principale)
        uint8_t yGpio;        // GPIO axe Y (additionalPin)
        uint8_t zGpio;        // GPIO axe Z (additionalPin)
        const char* pinLabel; // Label de la pin (ex: "A0")
    };

    static constexpr uint8_t MAX_JOYSTICK3S = 16;
    Joystick3Config joysticks[MAX_JOYSTICK3S];
    uint8_t joystick_count;

    int findJoystickIndex(uint8_t mainPinGpio) const;

};
