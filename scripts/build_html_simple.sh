#!/bin/bash
# Script simple qui minifie web/index.html et génère src/ui/ui_index.cpp
# Intègre web/app.js entre les marqueurs <!--JS-->...<!--/JS-->
# Ne modifie PAS le code, juste la minification (commentaires + espaces)
# Utilise uniquement sed/awk pour la portabilité (pas de Python)
# Compatible macOS (BSD) et Linux (GNU)

set -e

# Récupérer la langue depuis la variable d'environnement (défaut: fr)
LANG_CODE=${LANG_CODE:-fr}

# Validation de la langue
if [ "$LANG_CODE" != "fr" ] && [ "$LANG_CODE" != "en" ]; then
    echo "⚠️  Langue non supportée: $LANG_CODE, utilisation de 'fr' par défaut"
    LANG_CODE="fr"
fi

echo "🗜️  Minification HTML simple..."
echo "🌐 Langue sélectionnée: $LANG_CODE"

# Charger le fichier de traduction
LANG_FILE="web/lang/${LANG_CODE}.json"
if [ ! -f "$LANG_FILE" ]; then
    echo "❌ Erreur: Fichier de traduction non trouvé: $LANG_FILE"
    exit 1
fi

# Vérifier que jq est installé si on utilise une langue autre que fr
if [ "$LANG_CODE" != "fr" ] && ! command -v jq &> /dev/null; then
    echo "❌ Erreur: jq est requis pour les traductions"
    echo "📝 Installation: brew install jq (macOS) ou sudo apt-get install jq (Linux)"
    exit 1
fi

# Fonction pour récupérer une valeur depuis le JSON avec jq
get_translation() {
    local key="$1"
    if command -v jq &> /dev/null; then
        jq -r ".$key // empty" "$LANG_FILE" 2>/dev/null | sed 's/"/\\"/g'
    else
        echo ""
    fi
}

# Fonction pour remplacer les placeholders {{t.key}} dans un fichier
replace_translations() {
    local file="$1"
    local temp_file=$(mktemp)
    
    cp "$file" "$temp_file"
    
    # Chercher tous les placeholders {{t.xxx}} et les remplacer
    while IFS= read -r line || [ -n "$line" ]; do
        # Chercher tous les placeholders dans la ligne
        while echo "$line" | grep -q '{{t\.'; do
            # Extraire la clé du premier placeholder trouvé
            key=$(echo "$line" | sed -n 's/.*{{t\.\([^}]*\)}}.*/\1/p')
            if [ -n "$key" ]; then
                # Récupérer la traduction avec jq
                translation=$(get_translation "$key")
                
                if [ -n "$translation" ] && [ "$translation" != "null" ]; then
                    # Échapper les caractères spéciaux pour sed
                    translation_escaped=$(printf '%s\n' "$translation" | sed 's/[[\.*^$()+?{|]/\\&/g')
                    line=$(echo "$line" | sed "s|{{t\.$key}}|$translation_escaped|g")
                else
                    # Si pas de traduction trouvée, laisser le placeholder
                    break
                fi
            else
                break
            fi
        done
        echo "$line"
    done < "$temp_file" > "$file"
    
    rm -f "$temp_file"
}

# Créer le dossier build s'il n'existe pas
mkdir -p build

# Vérifier que web/index.html existe
if [ ! -f "web/index.html" ]; then
    echo "❌ Erreur: web/index.html n'existe pas"
    exit 1
fi

# Vérifier qu'au moins un fichier JS existe
if [ ! -f "web/app.js" ] && [ ! -f "web/js/core.js" ]; then
    echo "❌ Erreur: Aucun fichier JavaScript trouvé (web/app.js ou web/js/core.js)"
    exit 1
fi

# Concaténer les fichiers JS dans l'ordre : core.js, component-helpers.js, ui-utils.js, pin-utils.js, api.js, definitions.js, form-generator.js, midi-config.js, gpio-manager.js, pin-visual.js, pin-list.js, component-menus.js, component-form.js, component-config.js, component-utils.js, component-handlers.js, components.js, websocket.js, pins.js (compatibilité), puis app.js
echo "📄 Concaténation des modules JavaScript..."
TEMP_JS=$(mktemp)
{
    if [ -f "web/js/core.js" ]; then
        cat web/js/core.js
    fi
    if [ -f "web/js/component-helpers.js" ]; then
        echo ""
        cat web/js/component-helpers.js
    fi
    if [ -f "web/js/ui-utils.js" ]; then
        echo ""
        cat web/js/ui-utils.js
    fi
    if [ -f "web/js/pin-utils.js" ]; then
        echo ""
        cat web/js/pin-utils.js
    fi
    if [ -f "web/js/api.js" ]; then
        echo ""
        cat web/js/api.js
    fi
    if [ -f "web/js/definitions.js" ]; then
        echo ""
        cat web/js/definitions.js
    fi
    if [ -f "web/js/form-generator.js" ]; then
        echo ""
        cat web/js/form-generator.js
    fi
    if [ -f "web/js/midi-config.js" ]; then
        echo ""
        cat web/js/midi-config.js
    fi
    if [ -f "web/js/gpio-manager.js" ]; then
        echo ""
        cat web/js/gpio-manager.js
    fi
    if [ -f "web/js/pin-visual.js" ]; then
        echo ""
        cat web/js/pin-visual.js
    fi
    if [ -f "web/js/pin-list.js" ]; then
        echo ""
        cat web/js/pin-list.js
    fi
    if [ -f "web/js/component-menus.js" ]; then
        echo ""
        cat web/js/component-menus.js
    fi
    if [ -f "web/js/component-form.js" ]; then
        echo ""
        cat web/js/component-form.js
    fi
    if [ -f "web/js/component-config.js" ]; then
        echo ""
        cat web/js/component-config.js
    fi
    if [ -f "web/js/component-utils.js" ]; then
        echo ""
        cat web/js/component-utils.js
    fi
    if [ -f "web/js/component-handlers.js" ]; then
        echo ""
        cat web/js/component-handlers.js
    fi
    if [ -f "web/js/components.js" ]; then
        echo ""
        cat web/js/components.js
    fi
    if [ -f "web/js/websocket.js" ]; then
        echo ""
        cat web/js/websocket.js
    fi
    if [ -f "web/js/pins.js" ]; then
        echo ""
        cat web/js/pins.js
    fi
    if [ -f "web/app.js" ]; then
        echo ""
        cat web/app.js
    fi
} > "$TEMP_JS"

if [ ! -s "$TEMP_JS" ]; then
    echo "❌ Erreur: Aucun fichier JavaScript trouvé"
    rm -f "$TEMP_JS"
    exit 1
fi

# Minifier le JavaScript : supprimer les commentaires /* */ et espaces multiples
# Note: sed avec pattern /\*[^*]*\*/ ne gère que les commentaires sur une ligne
# Pour les commentaires multi-lignes, on utilise une approche plus robuste
echo "🗜️  Minification JavaScript (suppression commentaires /* */)..."
TEMP_JS_MINIFIED=$(mktemp)

# Supprimer les commentaires /* */ (y compris multi-lignes)
# Utiliser awk pour gérer les commentaires multi-lignes correctement
awk '
BEGIN {
    in_comment = 0
}
{
    line = $0
    result = ""
    i = 1
    while (i <= length(line)) {
        if (!in_comment) {
            # Chercher début de commentaire /* ou fin de ligne
            if (substr(line, i, 2) == "/*") {
                in_comment = 1
                i += 2
            } else {
                result = result substr(line, i, 1)
                i++
            }
        } else {
            # Chercher fin de commentaire */
            if (substr(line, i, 2) == "*/") {
                in_comment = 0
                i += 2
            } else {
                i++
            }
        }
    }
    if (result != "" || !in_comment) {
        print result
    }
}
' "$TEMP_JS" | sed -E 's/  +/ /g; s/^[[:space:]]+//; s/[[:space:]]+$//' | grep -v '^[[:space:]]*$' > "$TEMP_JS_MINIFIED"

# Vérifier que la minification n'a pas supprimé tout le contenu
if [ ! -s "$TEMP_JS_MINIFIED" ]; then
    echo "⚠️  Attention: La minification a supprimé tout le contenu, utilisation du JS original"
    cp "$TEMP_JS" "$TEMP_JS_MINIFIED"
fi

# Remplacer le fichier temporaire par la version minifiée
mv "$TEMP_JS_MINIFIED" "$TEMP_JS"

# Appliquer les traductions au JavaScript (même en français pour remplacer les placeholders éventuels)
echo "🌐 Application des traductions JavaScript ($LANG_CODE)..."
replace_translations "$TEMP_JS"

# Remplacer <!--JS-->...<!--/JS--> par <script src="/bundle"></script>
echo "📄 Génération HTML minimal avec /bundle..."
TEMP_HTML=$(mktemp)
awk '
/<!--JS-->/ {
    print "<script src=\"/bundle\"></script>"
    in_js_section = 1
    next
}
/<!--\/JS-->/ {
    in_js_section = 0
    next
}
in_js_section == 0 {
    print
}
' web/index.html > "$TEMP_HTML"

if [ ! -s "$TEMP_HTML" ]; then
    echo "❌ Erreur: La génération HTML a échoué"
    rm -f "$TEMP_HTML" "$TEMP_JS"
    exit 1
fi

# Appliquer les traductions au HTML (même en français pour remplacer {{t.lang}})
echo "🌐 Application des traductions HTML ($LANG_CODE)..."
replace_translations "$TEMP_HTML"

echo "✅ HTML minimal généré"

# Calculer la taille JS avant compression (après minification)
JS_SIZE=$(wc -c < "$TEMP_JS")

# Compresser le JS avec gzip
echo "🗜️  Compression JavaScript en gzip..."
if ! gzip -c "$TEMP_JS" > build/bundle.js.gz; then
    echo "❌ Erreur: La compression gzip a échoué"
    rm -f "$TEMP_HTML" "$TEMP_JS"
    exit 1
fi

BUNDLE_GZ_SIZE=$(wc -c < build/bundle.js.gz)
echo "✅ JavaScript compressé ($BUNDLE_GZ_SIZE bytes, source: $JS_SIZE bytes)"

# Minifier : supprimer commentaires HTML, commentaires JS /* */, et espaces multiples
# Ne PAS toucher aux // (peuvent être dans des URLs)
echo "📄 Minification..."
sed -E 's/<!--[^>]*-->//g; s|/\*[^*]*\*/||g; s/  +/ /g' "$TEMP_HTML" > build/index.min.html.tmp

# Supprimer les newlines après <script> et avant </script> (problème d'encodage ESP32)
# Utiliser sed avec des patterns simples (compatible macOS et Linux)
sed -E 's|<script>[[:space:]]+|<script>|g; s|[[:space:]]+</script>|</script>|g' build/index.min.html.tmp > build/index.min.html
rm -f "$TEMP_HTML" build/index.min.html.tmp

if [ ! -s build/index.min.html ]; then
    echo "❌ Erreur: Le fichier minifié est vide"
    exit 1
fi

echo "✅ HTML minifié vers build/index.min.html"

# Créer le dossier src/ui s'il n'existe pas
mkdir -p src/ui

# Générer le C++ avec le HTML minifié
echo "🔨 Génération de src/ui/ui_index.cpp..."
{
  echo '#include "ui_index.h"'
  echo 'const char INDEX_HTML[] PROGMEM = R"rawliteral('
  cat build/index.min.html
  echo ')rawliteral";'
} > src/ui/ui_index.cpp

# Générer le bundle C++ avec xxd (format wvr)
echo "🔨 Génération de src/ui/ui_bundle.h..."
xxd -i build/bundle.js.gz > src/ui/ui_bundle.h.tmp

# Extraire le nom du tableau et la longueur
BUNDLE_NAME=$(grep -o '^unsigned char [^[]*\[\]' src/ui/ui_bundle.h.tmp | sed 's/unsigned char //; s/\[\]//')
BUNDLE_LEN=$(grep -o 'unsigned int.*_len = [0-9]*' src/ui/ui_bundle.h.tmp | grep -o '[0-9]*$')

if [ -z "$BUNDLE_NAME" ] || [ -z "$BUNDLE_LEN" ]; then
    echo "❌ Erreur: Impossible d'extraire le nom ou la longueur du bundle"
    rm -f src/ui/ui_bundle.h.tmp "$TEMP_JS"
    exit 1
fi

# Générer le fichier final avec PROGMEM et BUNDLE_LEN (format wvr)
{
    echo '#pragma once'
    echo ''
    echo "#define BUNDLE_LEN $BUNDLE_LEN"
    sed -E "s/unsigned char ${BUNDLE_NAME}\[\] =/const uint8_t BUNDLE[] PROGMEM =/" src/ui/ui_bundle.h.tmp | grep -v "^unsigned int"
} > src/ui/ui_bundle.h

rm -f src/ui/ui_bundle.h.tmp "$TEMP_JS"

if [ ! -s src/ui/ui_bundle.h ]; then
    echo "❌ Erreur: La génération ui_bundle.h a échoué"
    rm -f "$TEMP_HTML"
    exit 1
fi

echo "✅ Bundle généré dans src/ui/ui_bundle.h (BUNDLE_LEN=$BUNDLE_LEN)"

# Afficher les tailles
HTML_SIZE=$(wc -c < web/index.html)
MIN_SIZE=$(wc -c < build/index.min.html)
BUNDLE_SIZE=$(wc -c < build/bundle.js.gz)
CPP_HTML_SIZE=$(wc -c < src/ui/ui_index.cpp)
CPP_BUNDLE_SIZE=$(wc -c < src/ui/ui_bundle.h)

COMBINED_SIZE=$((HTML_SIZE + JS_SIZE))

if [ "$COMBINED_SIZE" -gt 0 ]; then
    TOTAL_MIN=$((MIN_SIZE + BUNDLE_SIZE))
    REDUCTION=$(( (COMBINED_SIZE - TOTAL_MIN) * 100 / COMBINED_SIZE ))
else
    REDUCTION=0
fi

GZIP_RATIO=0
if [ "$JS_SIZE" -gt 0 ]; then
    GZIP_RATIO=$(( (JS_SIZE - BUNDLE_SIZE) * 100 / JS_SIZE ))
fi

rm -f "$TEMP_HTML"

echo ""
echo "✅ Build terminé !"
echo "📊 Résultats:"
echo "  HTML source:        $HTML_SIZE bytes"
echo "  JS source:          $JS_SIZE bytes"
echo "  Total source:       $COMBINED_SIZE bytes"
echo "  HTML minifié:       $MIN_SIZE bytes"
echo "  Bundle JS (gzip):   $BUNDLE_SIZE bytes (réduction: $GZIP_RATIO%)"
echo "  Total minifié:      $((MIN_SIZE + BUNDLE_SIZE)) bytes"
echo "  Réduction totale:   $REDUCTION%"
echo ""
echo "📁 Fichiers générés:"
echo "  build/index.min.html     (HTML minimal)"
echo "  build/bundle.js.gz       (JS compressé)"
echo "  src/ui/ui_index.cpp      (HTML minimal en C++)"
echo "  src/ui/ui_bundle.h       (Bundle JS gzipé en C++)"
