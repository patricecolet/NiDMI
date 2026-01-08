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

# Intégrer le JS concaténé dans le HTML entre les marqueurs <!--JS-->...<!--/JS-->
# Utiliser awk pour le remplacement multi-lignes (portable)
echo "📄 Intégration du JavaScript dans web/index.html..."
TEMP_HTML=$(mktemp)
awk -v js_file="$TEMP_JS" '
BEGIN {
    # Lire tout le contenu JS depuis le fichier
    while ((getline line < js_file) > 0) {
        if (js_content != "") js_content = js_content "\n"
        js_content = js_content line
    }
    close(js_file)
    # Supprimer les newlines en début/fin
    gsub(/^[ \t\n\r]+|[ \t\n\r]+$/, "", js_content)
    
    in_js_section = 0
    js_section_replaced = 0
}
/<!--JS-->/ {
    in_js_section = 1
    if (!js_section_replaced) {
        print "<script>" js_content "</script>"
        js_section_replaced = 1
    }
    next
}
/<!--\/JS-->/ {
    if (in_js_section) {
        in_js_section = 0
        next
    }
}
in_js_section == 0 {
    print
}
' web/index.html > "$TEMP_HTML"

rm -f "$TEMP_JS"

if [ ! -s "$TEMP_HTML" ]; then
    echo "❌ Erreur: L'intégration a échoué"
    rm -f "$TEMP_HTML"
    exit 1
fi

echo "✅ JavaScript intégré"

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

# Afficher les tailles
HTML_SIZE=$(wc -c < web/index.html)
JS_SIZE=0
if [ -f "web/js/core.js" ]; then
    JS_SIZE=$((JS_SIZE + $(wc -c < web/js/core.js)))
fi
if [ -f "web/js/api.js" ]; then
    JS_SIZE=$((JS_SIZE + $(wc -c < web/js/api.js)))
fi
if [ -f "web/js/pins.js" ]; then
    JS_SIZE=$((JS_SIZE + $(wc -c < web/js/pins.js)))
fi
if [ -f "web/js/components.js" ]; then
    JS_SIZE=$((JS_SIZE + $(wc -c < web/js/components.js)))
fi
if [ -f "web/js/websocket.js" ]; then
    JS_SIZE=$((JS_SIZE + $(wc -c < web/js/websocket.js)))
fi
if [ -f "web/js/mux.js" ]; then
    JS_SIZE=$((JS_SIZE + $(wc -c < web/js/mux.js)))
fi
if [ -f "web/app.js" ]; then
    JS_SIZE=$((JS_SIZE + $(wc -c < web/app.js)))
fi
MIN_SIZE=$(wc -c < build/index.min.html)
CPP_SIZE=$(wc -c < src/ui_index.cpp)

COMBINED_SIZE=$((HTML_SIZE + JS_SIZE))

if [ "$COMBINED_SIZE" -gt 0 ]; then
    REDUCTION=$(( (COMBINED_SIZE - MIN_SIZE) * 100 / COMBINED_SIZE ))
else
    REDUCTION=0
fi

echo ""
echo "✅ Build terminé !"
echo "📊 Résultats:"
echo "  HTML source:     $HTML_SIZE bytes"
echo "  JS source:       $JS_SIZE bytes"
echo "  Total source:    $COMBINED_SIZE bytes"
echo "  HTML minifié:    $MIN_SIZE bytes"
echo "  ui_index.cpp:    $CPP_SIZE bytes"
echo "  Réduction:       $REDUCTION%"
echo ""
echo "📁 Fichiers générés:"
echo "  build/index.min.html  (vérifier avant de déployer)"
echo "  src/ui_index.cpp      (remplace l'ancien)"
