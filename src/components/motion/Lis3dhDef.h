#pragma once

#include "../ComponentDefinition.h"
#include "../ComponentBuilder.h"
#include "../FormFieldHelpers.h"
#include "../MidiMessageFactory.h"
#include "../../midi/MidiMessageType.h"

/**
 * @file Lis3dhDef.h
 * @brief Définition du composant LIS3DH (accéléromètre 3 axes)
 * 
 * Accéléromètre LIS3DH avec 3 axes (X, Y, Z).
 * Chaque axe peut avoir un type MIDI différent et des seuils configurables.
 * 
 * Famille: MOTION
 * Interface: I2C
 */
namespace Components {

/**
 * @brief Configuration spécifique au LIS3DH
 */
struct ImuConfig {
    uint8_t filter_intensity;  // Intensité du filtrage (1-10)
    uint8_t bus_interface;     // 0 = I2C, 1 = SPI (déduit de la pin)
    uint8_t i2c_address;       // Adresse I2C (0x18 ou 0x19)
    uint8_t cs_gpio;           // GPIO Chip Select pour SPI (obligatoire, CS doit basculer)
    uint8_t range;              // Plage de mesure (0=±2g, 1=±4g, 2=±8g, 3=±16g)
    uint8_t data_rate;          // Taux de données (0=1Hz, 1=10Hz, 2=25Hz, 3=50Hz, 4=100Hz, 5=200Hz, 6=400Hz, 7=1.6kHz, 8=5kHz)
    
    // Seuils axe X (en LSB, ±2000 pour ±2g)
    int16_t xMin;          // Seuil minimum axe X
    int16_t xZeroMin;      // Début zone morte axe X
    int16_t xZeroMax;      // Fin zone morte axe X
    int16_t xMax;          // Seuil maximum axe X
    
    // Seuils axe Y
    int16_t yMin;
    int16_t yZeroMin;
    int16_t yZeroMax;
    int16_t yMax;
    
    // Seuils axe Z
    int16_t zMin;
    int16_t zZeroMin;
    int16_t zZeroMax;
    int16_t zMax;
    
    // Inversion par axe
    bool invertX;
    bool invertY;
    bool invertZ;
    
    // Configuration MIDI par axe
    MidiMessageType xMsgType;  // Type de message MIDI pour l'axe X
    MidiMessageType yMsgType;  // Type de message MIDI pour l'axe Y
    MidiMessageType zMsgType;  // Type de message MIDI pour l'axe Z
    uint8_t xMidiParam;        // Paramètre MIDI axe X (CC#, note#, etc.)
    uint8_t yMidiParam;        // Paramètre MIDI axe Y (CC#, note#, etc.)
    uint8_t zMidiParam;        // Paramètre MIDI axe Z (CC#, note#, etc.)
    uint8_t xMidiChannel;      // Canal MIDI axe X (1-16)
    uint8_t yMidiChannel;      // Canal MIDI axe Y (1-16)
    uint8_t zMidiChannel;      // Canal MIDI axe Z (1-16)
    
    ImuConfig()
        : filter_intensity(5)
        , bus_interface(0)    // 0 = I2C par défaut
        , i2c_address(0x18)  // Adresse par défaut (SA0=LOW)
        , cs_gpio(2)         // GPIO2 (D0) par défaut pour CS
        , range(0)            // ±2g par défaut
        , data_rate(4)        // 100Hz par défaut
        // Seuils par défaut pour ±2g (±2000 LSB)
        , xMin(-2000), xZeroMin(-100), xZeroMax(100), xMax(2000)
        , yMin(-2000), yZeroMin(-100), yZeroMax(100), yMax(2000)
        , zMin(-2000), zZeroMin(-100), zZeroMax(100), zMax(2000)
        , invertX(false), invertY(false), invertZ(false)
        , xMsgType(MidiMessageType::CONTROL_CHANGE)
        , yMsgType(MidiMessageType::CONTROL_CHANGE)
        , zMsgType(MidiMessageType::CONTROL_CHANGE)
        , xMidiParam(1), yMidiParam(2), zMidiParam(3)
        , xMidiChannel(1), yMidiChannel(1), zMidiChannel(1) {}
};

/**
 * @brief Définition complète du LIS3DH
 */
struct Lis3dh {
    // Identifiants
    static constexpr const char* ID = "lis3dh";
    static constexpr const char* DISPLAY_NAME = "LIS3DH (Accéléromètre 3 axes)";
    static constexpr const char* FAMILY_NAME = "Motion";
    
    // Configuration
    static constexpr ComponentFamily FAMILY = ComponentFamily::MOTION;
    static constexpr ComponentType TYPE = ComponentType::IMU;
    static constexpr PinType PIN_TYPE = PinType::PIN_I2C;
    static constexpr bool IMPLEMENTED = true;
    static constexpr bool SUPPORTS_MIDI = true;
    static constexpr bool SUPPORTS_OSC = true;
    
    // Valeurs par défaut
    static constexpr uint8_t DEFAULT_CHANNEL = 1;
    static constexpr uint8_t DEFAULT_FILTER_INTENSITY = 5;
    static constexpr int16_t DEFAULT_MIN = -2000;
    static constexpr int16_t DEFAULT_ZERO_MIN = -100;
    static constexpr int16_t DEFAULT_ZERO_MAX = 100;
    static constexpr int16_t DEFAULT_MAX = 2000;
    
    /**
     * @brief Validation : pour I2C, n'importe quel GPIO digital est valide
     */
    static bool validate(uint8_t gpio) {
        return gpio < 48;  // GPIO valide
    }
    
    /**
     * @brief Crée la définition complète pour le registre
     */
    static ComponentDefinition createDefinition() {
        return ComponentBuilder()
            .setBasicInfo(ID, DISPLAY_NAME, "cardLis3dh")
            .setFamily(FAMILY, FAMILY_NAME)
            .setType(TYPE, PIN_TYPE)
            .setAltPinType(PinType::PIN_SPI)
            .setCapabilities(SUPPORTS_MIDI, SUPPORTS_OSC)
            .setImplemented(IMPLEMENTED)
            
            // Adresse I2C (masqué en SPI par le frontend)
            .addFormField(makeSelectField(
                "i2cAddress",
                "Adresse I2C",
                "[{\"value\":\"24\",\"label\":\"0x18 (SA0=LOW)\"},{\"value\":\"25\",\"label\":\"0x19 (SA0=HIGH)\"}]",
                "24",
                "r"
            ))
            // GPIO CS pour SPI (masqué en I2C par le frontend)
            // Mapping C3: D0=GPIO2, D1=GPIO3, D2=GPIO4, D3=GPIO5, D4=GPIO6, D5=GPIO7, D6=GPIO21, D7=GPIO20
            .addFormField(makeSelectField(
                "csGpio",
                "Pin CS",
                "[{\"value\":\"2\",\"label\":\"D0\"},{\"value\":\"3\",\"label\":\"D1\"},{\"value\":\"4\",\"label\":\"D2\"},{\"value\":\"5\",\"label\":\"D3\"},{\"value\":\"6\",\"label\":\"D4\"},{\"value\":\"7\",\"label\":\"D5\"},{\"value\":\"21\",\"label\":\"D6\"},{\"value\":\"20\",\"label\":\"D7\"}]",
                "2",
                "r"
            ))
            // Configuration plage
            .addFormField(makeSelectField(
                "range",
                "Plage de mesure",
                "[{\"value\":\"0\",\"label\":\"±2g\"},{\"value\":\"1\",\"label\":\"±4g\"},{\"value\":\"2\",\"label\":\"±8g\"},{\"value\":\"3\",\"label\":\"±16g\"}]",
                "0",
                "r"
            ))
            
            // Configuration taux de données
            .addFormField(makeSelectField(
                "dataRate",
                "Taux de données",
                "[{\"value\":\"0\",\"label\":\"1 Hz\"},{\"value\":\"1\",\"label\":\"10 Hz\"},{\"value\":\"2\",\"label\":\"25 Hz\"},{\"value\":\"3\",\"label\":\"50 Hz\"},{\"value\":\"4\",\"label\":\"100 Hz\"},{\"value\":\"5\",\"label\":\"200 Hz\"},{\"value\":\"6\",\"label\":\"400 Hz\"},{\"value\":\"7\",\"label\":\"1.6 kHz\"},{\"value\":\"8\",\"label\":\"5 kHz\"}]",
                "4",
                "r"
            ))
            
            // Seuils axe X
            .addFormField(makeNumberField("xMin", "X Min", -32768, 32767, "-2000", 1, 100, "f"))
            .addFormField(makeNumberField("xZeroMin", "X Zero Min", -32768, 32767, "-100", 1, 100, "f"))
            .addFormField(makeNumberField("xZeroMax", "X Zero Max", -32768, 32767, "100", 1, 100, "f"))
            .addFormField(makeNumberField("xMax", "X Max", -32768, 32767, "2000", 1, 100, "f"))
            
            // Seuils axe Y
            .addFormField(makeNumberField("yMin", "Y Min", -32768, 32767, "-2000", 1, 100, "f"))
            .addFormField(makeNumberField("yZeroMin", "Y Zero Min", -32768, 32767, "-100", 1, 100, "f"))
            .addFormField(makeNumberField("yZeroMax", "Y Zero Max", -32768, 32767, "100", 1, 100, "f"))
            .addFormField(makeNumberField("yMax", "Y Max", -32768, 32767, "2000", 1, 100, "f"))
            
            // Seuils axe Z
            .addFormField(makeNumberField("zMin", "Z Min", -32768, 32767, "-2000", 1, 100, "f"))
            .addFormField(makeNumberField("zZeroMin", "Z Zero Min", -32768, 32767, "-100", 1, 100, "f"))
            .addFormField(makeNumberField("zZeroMax", "Z Zero Max", -32768, 32767, "100", 1, 100, "f"))
            .addFormField(makeNumberField("zMax", "Z Max", -32768, 32767, "2000", 1, 100, "f"))
            
            // Inversion d'axes
            .addFormField(makeSelectField(
                "invertX",
                "Inverser axe X",
                "[{\"value\":\"0\",\"label\":\"Normal\"},{\"value\":\"1\",\"label\":\"Inversé\"}]",
                "0",
                "r"
            ))
            .addFormField(makeSelectField(
                "invertY",
                "Inverser axe Y",
                "[{\"value\":\"0\",\"label\":\"Normal\"},{\"value\":\"1\",\"label\":\"Inversé\"}]",
                "0",
                "r"
            ))
            .addFormField(makeSelectField(
                "invertZ",
                "Inverser axe Z",
                "[{\"value\":\"0\",\"label\":\"Normal\"},{\"value\":\"1\",\"label\":\"Inversé\"}]",
                "0",
                "r"
            ))
            
            // Filtrage
            .addFormField(makeNumberFieldWithHint(
                "filterIntensity",
                "Intensité filtrage (1-10)",
                1, 10, "5", "1=rapide, 10=stable", 60, "r"
            ))
            
            // Messages MIDI pour l'axe X
            .addMidiMessage(createCcMessageForAxis("x", true))
            .addMidiMessage(createPitchBendMessageForAxis("x"))
            .addMidiMessage(createAftertouchMessageForAxis("x"))
            .addMidiMessage(createNoteSweepMessageForAxis("x"))
            
            // Messages MIDI pour l'axe Y
            .addMidiMessage(createCcMessageForAxis("y", true))
            .addMidiMessage(createPitchBendMessageForAxis("y"))
            .addMidiMessage(createAftertouchMessageForAxis("y"))
            .addMidiMessage(createNoteSweepMessageForAxis("y"))
            
            // Messages MIDI pour l'axe Z
            .addMidiMessage(createCcMessageForAxis("z", true))
            .addMidiMessage(createPitchBendMessageForAxis("z"))
            .addMidiMessage(createAftertouchMessageForAxis("z"))
            .addMidiMessage(createNoteSweepMessageForAxis("z"))
            
            .build();
    }
};

} // namespace Components
