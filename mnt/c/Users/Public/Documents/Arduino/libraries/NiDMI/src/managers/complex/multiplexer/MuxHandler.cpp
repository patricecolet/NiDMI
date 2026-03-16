#include "MuxHandler.h"
#include "../../ComponentManager.h"  /* Doit être avant Globals.h pour la définition complète */
#include "../../../Globals.h"
#include "../../../components/ComponentRegistry.h"
#include "../../../hardware/MuxConstants.h"
#include "../../MuxManager.h"
#include <cstring>

MuxHandler::MuxHandler(MuxManager* muxManager) {
    /* MuxManager est accessible via g_componentManager, pas besoin de le stocker */
    (void)muxManager;  /* Éviter warning unused parameter */
}

uint8_t MuxHandler::findOrCreateMuxId(uint8_t mainPinGpio) {
    /* Trouver un MUX existant avec le même SIG, ou générer un ID disponible */
    uint8_t mux_id = 255;
    
    /* Chercher si un MUX existe déjà avec ce SIG */
    if (mainPinGpio != 255) {
        for (uint8_t i = 0; i < MAX_MUXES; i++) {
            const MuxConfig* existingCfg = g_componentManager.getMuxConfig(i);
            if (existingCfg && existingCfg->enabled && existingCfg->sig_pin == mainPinGpio) {
                mux_id = i;  /* Réutiliser l'ID existant */
                break;
            }
        }
    }
    
    /* Si pas trouvé, générer un ID disponible */
    if (mux_id == 255) {
        for (uint8_t i = 0; i < MAX_MUXES; i++) {
            const MuxConfig* cfg = g_componentManager.getMuxConfig(i);
            if (!cfg || !cfg->enabled) {
                mux_id = i;
                break;
            }
        }
    }
    
    return mux_id;
}

bool MuxHandler::mapAdditionalPins(const ComplexComponentData& data, 
                                   uint8_t& s0, uint8_t& s1, uint8_t& s2, uint8_t& s3, uint8_t& en) {
    s0 = s1 = s2 = s3 = en = 255;
    
    /* Mapper les additionalPins depuis data */
    for (uint8_t i = 0; i < data.additionalPinCount; i++) {
        const char* pinId = data.additionalPins[i].id;
        uint8_t gpio = data.additionalPins[i].gpio;
        
        if (strcmp(pinId, "s0") == 0) s0 = gpio;
        else if (strcmp(pinId, "s1") == 0) s1 = gpio;
        else if (strcmp(pinId, "s2") == 0) s2 = gpio;
        else if (strcmp(pinId, "s3") == 0) s3 = gpio;
        else if (strcmp(pinId, "en") == 0) en = gpio;
    }
    
    /* Vérifier que les pins requises sont présentes */
    bool hasRequiredPins = (s0 != 255 && s1 != 255 && s2 != 255);
    
    /* s3 peut être absent pour HC4051 (3 bits seulement) */
    const char* componentId = data.def ? data.def->id : nullptr;
    if (componentId && strcmp(componentId, "hc4051") == 0) {
        /* Pour HC4051, s3 n'est pas requis */
        return hasRequiredPins;
    } else {
        /* Pour HC4067, s3 est requis */
        return hasRequiredPins && (s3 != 255);
    }
}

bool MuxHandler::mapFormFields(const ComplexComponentData& data,
                               uint16_t& analog_min, uint16_t& analog_max, uint8_t& filter_intensity) {
    analog_min = 0;
    analog_max = 4095;
    filter_intensity = 5;
    
    /* Chercher dans formFields */
    for (uint8_t i = 0; i < data.formFieldCount; i++) {
        const char* fieldId = data.formFields[i].id;
        String value = data.formFields[i].value;
        
        if (strcmp(fieldId, "muxMin") == 0 || strcmp(fieldId, "min") == 0) {
            analog_min = value.toInt();
            if (analog_min > 4095) analog_min = 4095;
        } else if (strcmp(fieldId, "muxMax") == 0 || strcmp(fieldId, "max") == 0) {
            analog_max = value.toInt();
            if (analog_max > 4095) analog_max = 4095;
        } else if (strcmp(fieldId, "muxFilterIntensity") == 0 || strcmp(fieldId, "filterIntensity") == 0) {
            filter_intensity = value.toInt();
            if (filter_intensity < 1) filter_intensity = 1;
            if (filter_intensity > 10) filter_intensity = 10;
        }
    }
    
    return true;
}

bool MuxHandler::mapMidiParams(const ComplexComponentData& data,
                               uint8_t& ccBase, uint8_t& midiChan, 
                               String& oscBase, MuxOSCFormat& oscFormat) {
    ccBase = 1;
    midiChan = 1;
    oscBase = "";
    oscFormat = MuxOSCFormat::FLOAT;
    
    /* Chercher dans midiParams */
    for (uint8_t i = 0; i < data.midiParamCount; i++) {
        const char* paramId = data.midiParams[i].id;
        String value = data.midiParams[i].value;
        
        if (strcmp(paramId, "midiCc") == 0 || strcmp(paramId, "rtpCc") == 0) { // Nouveau format puis compatibilité
            ccBase = value.toInt();
            if (ccBase > 127) ccBase = 127;
        } else if (strcmp(paramId, "midiChannel") == 0 || strcmp(paramId, "rtpChan") == 0) { // Nouveau format puis compatibilité
            midiChan = value.toInt();
            if (midiChan < 1) midiChan = 1;
            if (midiChan > 16) midiChan = 16;
        }
    }
    
    /* Chercher dans OSC params */
    if (data.oscEnabled && data.oscAddress.length() > 0) {
        oscBase = data.oscAddress;
    }
    
    if (data.oscFormat.length() > 0) {
        if (data.oscFormat == "raw") {
            oscFormat = MuxOSCFormat::RAW;
        } else if (data.oscFormat == "midi") {
            oscFormat = MuxOSCFormat::MIDI;
        } else {
            oscFormat = MuxOSCFormat::FLOAT;
        }
    }
    
    return true;
}

void MuxHandler::saveMuxConfigToNVS(uint8_t mux_id, uint8_t sig, uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3, uint8_t en,
                                    uint16_t analog_min, uint16_t analog_max, uint8_t filter_intensity,
                                    uint8_t ccBase, uint8_t midiChan, const char* oscBase, MuxOSCFormat oscFormat) {
    Preferences prefs;
    prefs.begin("nidmi", false);
    
    /* Sauvegarder les clés NVS spécifiques aux mux (mux_X, mux_thresh_X) pour compatibilité */
    String mux_key = "mux_" + String(mux_id);
    String mux_config = String(sig) + "," + String(s0) + "," + String(s1) + "," +
                       String(s2) + "," + String(s3) + "," + String(en) + "," +
                       String(ccBase) + "," + String(midiChan) + "," +
                       "1," + String((int)oscFormat) + "," +
                       String(filter_intensity) + "," + String(oscBase ? oscBase : "");
    prefs.putString(mux_key.c_str(), mux_config);
    
    /* Sauvegarder les seuils */
    String thresh_key = "mux_thresh_" + String(mux_id);
    uint8_t buffer[5];
    buffer[0] = 0x01;
    buffer[1] = analog_min & 0xFF;
    buffer[2] = (analog_min >> 8) & 0xFF;
    buffer[3] = analog_max & 0xFF;
    buffer[4] = (analog_max >> 8) & 0xFF;
    prefs.putBytes(thresh_key.c_str(), buffer, 5);
    
    prefs.end();
}

bool MuxHandler::addComponent(const ComplexComponentData& data) {
    if (!data.def || !supportsComponent(data.def->id)) {
        return false;
    }
    
    /* Mapper les additionalPins */
    uint8_t s0, s1, s2, s3, en;
    if (!mapAdditionalPins(data, s0, s1, s2, s3, en)) {
        return false;
    }
    
    /* Mapper les formFields */
    uint16_t analog_min, analog_max;
    uint8_t filter_intensity;
    mapFormFields(data, analog_min, analog_max, filter_intensity);
    
    /* Mapper les paramètres MIDI/OSC */
    uint8_t ccBase, midiChan;
    String oscBase;
    MuxOSCFormat oscFormat;
    mapMidiParams(data, ccBase, midiChan, oscBase, oscFormat);
    
    /* Utiliser mux_id par défaut si oscBase vide */
    uint8_t mux_id = findOrCreateMuxId(data.mainPinGpio);
    if (mux_id >= MAX_MUXES) {
        return false;
    }
    
    if (oscBase.length() == 0) {
        oscBase = "/mux" + String(mux_id);
    }
    
    /* Ajouter le MUX via ComponentManager */
    if (g_componentManager.addMux(mux_id, data.mainPinGpio, s0, s1, s2, s3, en, 
                                  analog_min, analog_max, true, oscFormat, filter_intensity, 
                                  ccBase, midiChan, oscBase.c_str())) {
        /* Sauvegarder aussi dans NVS pour compatibilité */
        saveMuxConfigToNVS(mux_id, data.mainPinGpio, s0, s1, s2, s3, en,
                          analog_min, analog_max, filter_intensity,
                          ccBase, midiChan, oscBase.c_str(), oscFormat);
        return true;
    }
    
    return false;
}

bool MuxHandler::removeComponent(const char* pinLabel, uint8_t mainPinGpio) {
    /* Chercher le MUX par sig_pin */
    for (uint8_t i = 0; i < MAX_MUXES; i++) {
        const MuxConfig* cfg = g_componentManager.getMuxConfig(i);
        if (cfg && cfg->enabled && cfg->sig_pin == mainPinGpio) {
            if (g_componentManager.removeMux(i)) {
                /* Supprimer aussi les clés NVS (compatibilité) */
                Preferences prefs;
                prefs.begin("nidmi", false);
                String mux_key = "mux_" + String(i);
                prefs.remove(mux_key.c_str());
                String thresh_key = "mux_thresh_" + String(i);
                prefs.remove(thresh_key.c_str());
                String pinLabelKey = "pinLabel_complex_" + String(i);
                prefs.remove(pinLabelKey.c_str());
                String roleKey = "role_complex_" + String(i);
                prefs.remove(roleKey.c_str());
                prefs.end();
                return true;
            }
        }
    }
    
    return false;
}

bool MuxHandler::getComponentInfo(const char* pinLabel, uint8_t mainPinGpio, String& json) {
    /* Chercher le MUX par sig_pin */
    for (uint8_t i = 0; i < MAX_MUXES; i++) {
        const MuxConfig* cfg = g_componentManager.getMuxConfig(i);
        if (cfg && cfg->enabled && cfg->sig_pin == mainPinGpio) {
            /* Construire le JSON depuis MuxConfig (fallback si pas trouvé dans NVS) */
            const ComponentDefinition* def = ComponentRegistry::findById("hc4067");  /* Utiliser hc4067 par défaut */
            
            json += "\"additionalPins\":{";
            if (def && def->additionalPinCount > 0 && def->additionalPins) {
                bool firstPin = true;
                for (uint8_t j = 0; j < def->additionalPinCount && j < def->additionalPinsCapacity; j++) {
                    String pinId = String(def->additionalPins[j].id);
                    if (pinId == "sig") continue;  /* Ignorer sig car c'est la pin principale */
                    
                    if (!firstPin) json += ",";
                    uint8_t pinValue = 255;
                    if (pinId == "s0") pinValue = cfg->s0;
                    else if (pinId == "s1") pinValue = cfg->s1;
                    else if (pinId == "s2") pinValue = cfg->s2;
                    else if (pinId == "s3") pinValue = cfg->s3;
                    else if (pinId == "en") pinValue = cfg->en_pin;
                    json += "\"" + pinId + "\":" + String(pinValue);
                    firstPin = false;
                }
            } else {
                /* Fallback pour compatibilité */
                json += "\"s0\":" + String(cfg->s0) + ",";
                json += "\"s1\":" + String(cfg->s1) + ",";
                json += "\"s2\":" + String(cfg->s2) + ",";
                json += "\"s3\":" + String(cfg->s3) + ",";
                json += "\"en\":" + String(cfg->en_pin);
            }
            json += "},";
            
            /* FormFields */
            uint16_t min_val = cfg->analog_min[0];
            uint16_t max_val = cfg->analog_max[0];
            json += "\"min\":" + String(min_val) + ",";
            json += "\"max\":" + String(max_val) + ",";
            json += "\"filterIntensity\":" + String(cfg->filter_intensity) + ",";
            
            /* MIDI/OSC config */
            json += "\"rtpCc\":" + String(cfg->cc_base) + ",";
            json += "\"rtpChan\":" + String(cfg->midi_channel) + ",";
            json += "\"rtpEnabled\":true,";
            String oscBase = (cfg->osc_base[0] != '\0') ? String(cfg->osc_base) : "/mux" + String(i);
            json += "\"oscAddress\":\"" + oscBase + "\",";
            String oscFormatStr = "float";
            if (cfg->osc_format == MuxOSCFormat::RAW) {
                oscFormatStr = "raw";
            } else if (cfg->osc_format == MuxOSCFormat::MIDI) {
                oscFormatStr = "midi";
            }
            json += "\"oscFormat\":\"" + oscFormatStr + "\"";
            
            return true;
        }
    }
    
    return false;
}

bool MuxHandler::isGpioUsed(uint8_t gpio) const {
    /* Vérifier si le GPIO est utilisé par un MUX */
    for (uint8_t i = 0; i < MAX_MUXES; i++) {
        const MuxConfig* cfg = g_componentManager.getMuxConfig(i);
        if (cfg && cfg->enabled) {
            if (cfg->sig_pin == gpio || cfg->s0 == gpio || cfg->s1 == gpio || 
                cfg->s2 == gpio || cfg->s3 == gpio || cfg->en_pin == gpio) {
                return true;
            }
        }
    }
    return false;
}

uint8_t MuxHandler::getComponentCount() const {
    return g_componentManager.getMuxCount();
}
