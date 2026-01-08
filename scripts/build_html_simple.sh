#!/bin/bash
# Script simple qui minifie web/index.html et génère src/ui_index.cpp
# Intègre web/app.js entre les marqueurs <!--JS-->...<!--/JS-->
# Ne modifie PAS le code, juste la minification (commentaires + espaces)
# Utilise uniquement sed/awk pour la portabilité (pas de Python)
# Compatible macOS (BSD) et Linux (GNU)

set -e

echo "🗜️  Minification HTML simple..."

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

# Concaténer les fichiers JS dans l'ordre : core.js, api.js, pins.js, components.js, websocket.js, mux.js, puis app.js
echo "📄 Concaténation des modules JavaScript..."
TEMP_JS=$(mktemp)
{
    if [ -f "web/js/core.js" ]; then
        cat web/js/core.js
    fi
    if [ -f "web/js/api.js" ]; then
        echo ""
        cat web/js/api.js
    fi
    if [ -f "web/js/pins.js" ]; then
        echo ""
        cat web/js/pins.js
    fi
    if [ -f "web/js/components.js" ]; then
        echo ""
        cat web/js/components.js
    fi
    if [ -f "web/js/websocket.js" ]; then
        echo ""
        cat web/js/websocket.js
    fi
    if [ -f "web/js/mux.js" ]; then
        echo ""
        cat web/js/mux.js
    fi
    if [ -f "web/app.js" ]; then
        echo ""
        cat web/app.js
    fi
} > "$TEMP_JS"

# Le JS est prêt, pas besoin de nettoyage supplémentaire
# (les lignes vides en début/fin seront gérées par awk lors de l'intégration)

if [ ! -s "$TEMP_JS" ]; then
    echo "❌ Erreur: Aucun fichier JavaScript trouvé"
    rm -f "$TEMP_JS"
    exit 1
fi

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

echo "✅ HTML minimal généré"

# Calculer la taille JS avant compression
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

# Générer le C++ avec le HTML minifié
echo "🔨 Génération de src/ui_index.cpp..."
{
  echo '#include "ui_index.h"'
  echo 'const char INDEX_HTML[] PROGMEM = R"rawliteral('
  cat build/index.min.html
  echo ')rawliteral";'
} > src/ui_index.cpp

# Générer le bundle C++ avec xxd (format wvr)
echo "🔨 Génération de src/ui_bundle.h..."
xxd -i build/bundle.js.gz > src/ui_bundle.h.tmp

# Extraire le nom du tableau et la longueur
BUNDLE_NAME=$(grep -o '^unsigned char [^[]*\[\]' src/ui_bundle.h.tmp | sed 's/unsigned char //; s/\[\]//')
BUNDLE_LEN=$(grep -o 'unsigned int.*_len = [0-9]*' src/ui_bundle.h.tmp | grep -o '[0-9]*$')

if [ -z "$BUNDLE_NAME" ] || [ -z "$BUNDLE_LEN" ]; then
    echo "❌ Erreur: Impossible d'extraire le nom ou la longueur du bundle"
    rm -f src/ui_bundle.h.tmp "$TEMP_JS"
    exit 1
fi

# Générer le fichier final avec PROGMEM et BUNDLE_LEN (format wvr)
{
    echo '#ifndef UI_BUNDLE_H'
    echo '#define UI_BUNDLE_H'
    echo ''
    echo "#define BUNDLE_LEN $BUNDLE_LEN"
    sed -E "s/unsigned char ${BUNDLE_NAME}\[\] =/const uint8_t BUNDLE[] PROGMEM =/" src/ui_bundle.h.tmp | grep -v "^unsigned int"
    echo ''
    echo '#endif'
} > src/ui_bundle.h

rm -f src/ui_bundle.h.tmp "$TEMP_JS"

if [ ! -s src/ui_bundle.h ]; then
    echo "❌ Erreur: La génération ui_bundle.h a échoué"
    rm -f "$TEMP_HTML"
    exit 1
fi

echo "✅ Bundle généré dans src/ui_bundle.h (BUNDLE_LEN=$BUNDLE_LEN)"

# Afficher les tailles
HTML_SIZE=$(wc -c < web/index.html)
MIN_SIZE=$(wc -c < build/index.min.html)
BUNDLE_SIZE=$(wc -c < build/bundle.js.gz)
CPP_HTML_SIZE=$(wc -c < src/ui_index.cpp)
CPP_BUNDLE_SIZE=$(wc -c < src/ui_bundle.h)

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
echo "  build/index.min.html  (HTML minimal)"
echo "  build/bundle.js.gz    (JS compressé)"
echo "  src/ui_index.cpp      (HTML minimal en C++)"
echo "  src/ui_bundle.h       (Bundle JS gzipé en C++)"
