#pragma once

#include "ComponentDefinition.h"

/**
 * @file FormFieldHelpers.h
 * @brief Helpers pour simplifier la création de FormFieldDef
 * 
 * Ces fonctions permettent de créer des FormFieldDef avec moins de code répétitif.
 */

namespace Components {

/**
 * @brief Crée un champ SELECT (liste déroulante)
 */
inline constexpr FormFieldDef makeSelectField(
    const char* id,
    const char* label,
    const char* options,  // JSON array: [{"value":"v","label":"L"},...]
    const char* defaultValue = nullptr,
    const char* wrapperClass = "r",
    const char* dependsOn = nullptr,
    const char* showWhen = nullptr
) {
    FormFieldDef field{};
    field.id = id;
    field.label = label;
    field.type = FieldType::SELECT;
    field.required = false;
    field.placeholder = nullptr;
    field.maxLength = 0;
    field.pattern = nullptr;
    field.min = 0;
    field.max = 0;
    field.step = 0;
    field.options = options;
    field.separator = nullptr;
    field.defaultValue = defaultValue;
    field.hintPosition = HintPosition::NONE;
    field.hint = nullptr;
    field.hintClass = nullptr;
    field.dependsOn = dependsOn;
    field.showWhen = showWhen;
    field.wrapperClass = wrapperClass;
    field.inputClass = nullptr;
    field.width = 0;
    field.labelBefore = nullptr;
    field.labelAfter = nullptr;
    return field;
}

/**
 * @brief Crée un champ NUMBER
 */
inline constexpr FormFieldDef makeNumberField(
    const char* id,
    const char* label,
    int min,
    int max,
    const char* defaultValue = nullptr,
    int step = 1,
    uint16_t width = 0,
    const char* wrapperClass = "r",
    HintPosition hintPosition = HintPosition::NONE,
    const char* hint = nullptr,
    const char* hintClass = nullptr
) {
    FormFieldDef field{};
    field.id = id;
    field.label = label;
    field.type = FieldType::NUMBER;
    field.required = false;
    field.placeholder = nullptr;
    field.maxLength = 0;
    field.pattern = nullptr;
    field.min = min;
    field.max = max;
    field.step = step;
    field.options = nullptr;
    field.separator = nullptr;
    field.defaultValue = defaultValue;
    field.hintPosition = hintPosition;
    field.hint = hint;
    field.hintClass = hintClass;
    field.dependsOn = nullptr;
    field.showWhen = nullptr;
    field.wrapperClass = wrapperClass;
    field.inputClass = nullptr;
    field.width = width;
    field.labelBefore = nullptr;
    field.labelAfter = nullptr;
    return field;
}

/**
 * @brief Crée un champ INFO (texte informatif)
 */
inline constexpr FormFieldDef makeInfoField(
    const char* id,
    const char* hint,
    HintPosition hintPosition = HintPosition::BELOW,
    const char* hintClass = "hint"
) {
    FormFieldDef field{};
    field.id = id;
    field.label = nullptr;
    field.type = FieldType::INFO;
    field.required = false;
    field.placeholder = nullptr;
    field.maxLength = 0;
    field.pattern = nullptr;
    field.min = 0;
    field.max = 0;
    field.step = 0;
    field.options = nullptr;
    field.separator = nullptr;
    field.defaultValue = nullptr;
    field.hintPosition = hintPosition;
    field.hint = hint;
    field.hintClass = hintClass;
    field.dependsOn = nullptr;
    field.showWhen = nullptr;
    field.wrapperClass = nullptr;
    field.inputClass = nullptr;
    field.width = 0;
    field.labelBefore = nullptr;
    field.labelAfter = nullptr;
    return field;
}

/**
 * @brief Crée un champ TEXT (court, pour valeurs compactes type "0,0" ou "2000,500")
 */
inline constexpr FormFieldDef makeTextField(
    const char* id,
    const char* label,
    const char* defaultValue = nullptr,
    const char* placeholder = nullptr,
    uint16_t maxLength = 32,
    uint16_t width = 0,
    const char* hint = nullptr
) {
    FormFieldDef field{};
    field.id = id;
    field.label = label;
    field.type = FieldType::TEXT;
    field.required = false;
    field.placeholder = placeholder;
    field.maxLength = maxLength;
    field.pattern = nullptr;
    field.min = 0;
    field.max = 0;
    field.step = 0;
    field.options = nullptr;
    field.separator = nullptr;
    field.defaultValue = defaultValue;
    field.hintPosition = hint ? HintPosition::INLINE : HintPosition::NONE;
    field.hint = hint;
    field.hintClass = hint ? "margin-left:8px;font-size:0.9em;color:#666;" : nullptr;
    field.dependsOn = nullptr;
    field.showWhen = nullptr;
    field.wrapperClass = "r";
    field.inputClass = nullptr;
    field.width = width;
    field.labelBefore = nullptr;
    field.labelAfter = nullptr;
    return field;
}

/**
 * @brief Crée un champ NUMBER avec hint inline (style commun pour filtrage, seuils, etc.)
 */
inline constexpr FormFieldDef makeNumberFieldWithHint(
    const char* id,
    const char* label,
    int min,
    int max,
    const char* defaultValue,
    const char* hint,
    uint16_t width = 60,
    const char* wrapperClass = "r"
) {
    return makeNumberField(
        id, label, min, max, defaultValue, 1, width, wrapperClass,
        HintPosition::INLINE,
        hint,
        "margin-left:8px;font-size:0.9em;color:#666;"
    );
}

} // namespace Components
