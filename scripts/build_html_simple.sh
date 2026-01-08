#!/bin/bash
# Script simple qui minifie web/index.html et génère src/ui_index.cpp
# Intègre web/app.js entre les marqueurs <!--JS-->...<!--/JS-->
# Ne modifie PAS le code, juste la minification (commentaires + espaces)

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

# Concaténer les fichiers JS dans l'ordre : core.js, api.js, pins.js, puis app.js
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

# Intégrer le JS concaténé dans le HTML entre les marqueurs <!--JS-->...<!--/JS-->
echo "📄 Intégration du JavaScript dans web/index.html..."
TEMP_HTML=$(mktemp)
python3 << EOF > "$TEMP_HTML"
import sys
import re

# Lire web/index.html
with open('web/index.html', 'r', encoding='utf-8') as f:
    html_content = f.read()

# Lire le JS concaténé depuis le fichier temporaire
with open('${TEMP_JS}', 'r', encoding='utf-8') as f:
    js_content = f.read().strip()

# Remplacer <!--JS--><!--/JS--> par <script>...</script> avec le contenu JS
# Utiliser une fonction de remplacement pour éviter les problèmes d'échappement
pattern = r'<!--JS-->.*?<!--/JS-->'
def replace_func(match):
    # Supprimer les newlines en début/fin du JS
    js_clean = js_content.strip()
    return '<script>' + js_clean + '</script>'
result = re.sub(pattern, replace_func, html_content, flags=re.DOTALL)

# Supprimer les newlines juste après <script> (problème d'encodage ESP32)
result = re.sub(r'<script>\n+', '<script>', result)
result = re.sub(r'\n+</script>', '</script>', result)

sys.stdout.write(result)
EOF
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
rm -f "$TEMP_HTML"

# Supprimer les newlines après <script> (problème d'encodage ESP32)
python3 << 'PYTHON_FIX' > build/index.min.html
import sys
import re
with open('build/index.min.html.tmp', 'r', encoding='utf-8') as f:
    content = f.read()
# Supprimer les newlines juste après <script> et avant </script>
content = re.sub(r'<script>\s+', '<script>', content)
content = re.sub(r'\s+</script>', '</script>', content)
sys.stdout.write(content)
PYTHON_FIX
rm -f build/index.min.html.tmp

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
