#pragma once

#include "../ComplexHandler.h"
#include "../../ComponentManager.h"  /* Doit être inclus pour la définition complète de ComponentManager */
#include "../../MuxManager.h"
#include "../../../hardware/MuxConstants.h"
#include <Preferences.h>

/**
 * @file MuxHandler.h
 * @brief Handler générique pour les multiplexeurs (hc4067, hc4051)
 * 
 * Implémente ComplexHandler pour gérer les multiplexeurs de manière générique.
 * Encapsule MuxManager pour isoler la logique spécifique MUX.
 */

class MuxHandler : public ComplexHandler {
private:
    /* MuxManager est accessible via g_componentManager, pas besoin de le stocker */
    
    /**
     * @brief Trouve un MUX existant avec le même SIG, ou génère un ID disponible
     */
    uint8_t findOrCreateMuxId(uint8_t mainPinGpio);
    
    /**
     * @brief Mappe les additionalPins depuis ComplexComponentData vers les variables MUX
     */
    bool mapAdditionalPins(const ComplexComponentData& data, 
                          uint8_t& s0, uint8_t& s1, uint8_t& s2, uint8_t& s3, uint8_t& en);
    
    /**
     * @brief Mappe les formFields vers les paramètres MUX
     */
    bool mapFormFields(const ComplexComponentData& data,
                      uint16_t& analog_min, uint16_t& analog_max, uint8_t& filter_intensity);
    
    /**
     * @brief Mappe les paramètres MIDI/OSC depuis ComplexComponentData
     */
    bool mapMidiParams(const ComplexComponentData& data,
                      uint8_t& ccBase, uint8_t& midiChan, 
                      String& oscBase, MuxOSCFormat& oscFormat);
    
    /**
     * @brief Sauvegarde la configuration MUX dans NVS (compatibilité)
     */
    void saveMuxConfigToNVS(uint8_t mux_id, uint8_t sig, uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3, uint8_t en,
                           uint16_t analog_min, uint16_t analog_max, uint8_t filter_intensity,
                           uint8_t ccBase, uint8_t midiChan, const char* oscBase, MuxOSCFormat oscFormat);

public:
    MuxHandler(MuxManager* muxManager = nullptr);
    virtual ~MuxHandler() = default;
    
    const char* getComponentId() const override {
        /* Supporte hc4067 et hc4051 */
        return "hc4067";  /* Utilisé pour identification, mais supporte aussi hc4051 */
    }
    
    /**
     * @brief Vérifie si ce handler supporte un composant par son ID
     */
    bool supportsComponent(const char* componentId) const {
        return componentId && (strcmp(componentId, "hc4067") == 0 || strcmp(componentId, "hc4051") == 0);
    }
    
    bool addComponent(const ComplexComponentData& data) override;
    bool removeComponent(const char* pinLabel, uint8_t mainPinGpio) override;
    bool getComponentInfo(const char* pinLabel, uint8_t mainPinGpio, String& json) override;
    bool isGpioUsed(uint8_t gpio) const override;
    uint8_t getComponentCount() const override;
};
