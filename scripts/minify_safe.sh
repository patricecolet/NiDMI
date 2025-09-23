#!/bin/bash
# Script de minification ultra-sûr (commentaires HTML + JavaScript)

set -e

echo "🗜️  Minification ultra-sûre de l'UI..."

# Créer le dossier build s'il n'existe pas
mkdir -p build

# Minifier le HTML de façon ultra-sûre (commentaires HTML + JavaScript + espaces)
echo "📄 Lecture de web/index.html..."
# Supprimer commentaires HTML, JavaScript et espaces multiples
sed -E 's/<!--[^>]*-->//g; s|/\*[^*]*\*/||g; s/  +/ /g' web/index.html > build/index.min.html

echo "✅ HTML minifié vers build/index.min.html"

# Créer le C++ avec le HTML minifié
echo "🔨 Génération du C++ minifié..."
{
  echo '#include "ui_index.h"'
  echo 'const char INDEX_HTML[] PROGMEM = R"rawliteral('
  cat build/index.min.html
  echo ')rawliteral";'
} > src/ui_index.cpp

echo "✅ C++ minifié a remplacé src/ui_index.cpp"

# Afficher les tailles
HTML_SIZE=$(wc -c < web/index.html)
MIN_SIZE=$(wc -c < src/ui_index.cpp)
REDUCTION=$(( (HTML_SIZE - MIN_SIZE) * 100 / HTML_SIZE ))

echo ""
echo "📊 Résultats:"
echo "  Taille HTML:      $HTML_SIZE bytes"
echo "  Taille C++:         $MIN_SIZE bytes"
echo "  Réduction:          $REDUCTION%"
