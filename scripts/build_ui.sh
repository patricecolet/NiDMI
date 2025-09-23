#!/bin/bash
# Script de build pour externaliser l'UI HTML

set -e

echo "🔧 Build UI - Externalisation HTML"

# Créer les dossiers
mkdir -p web build

# Extraire le HTML de ui_index.cpp (entre R"rawliteral( et )rawliteral")
echo "📄 Extraction du HTML..."
sed -n '/^const char INDEX_HTML\[\] PROGMEM = R"rawliteral(/,/^)rawliteral";$/p' src/ui_index.cpp | \
sed '1d;$d' > web/index.html

echo "✅ HTML extrait vers web/index.html"

# Créer le script de minification
cat > scripts/minify_ui.sh << 'EOF'
#!/bin/bash
# Minification de l'UI HTML

set -e

echo "🗜️  Minification de l'UI..."

# Minifier le HTML (supprimer commentaires, espaces, etc.)
cat web/index.html | \
sed -E 's/<!--[^>]*-->//g' | \
sed -E 's@//.*$@@' | \
tr -d '\n' | \
tr -s ' ' > build/index.min.html

echo "✅ HTML minifié vers build/index.min.html"

# Générer le C++ minifié
echo "🔨 Génération du C++ minifié..."
{
  echo '#include "ui_index.h"'
  echo 'const char INDEX_HTML[] PROGMEM = R"rawliteral('
  cat build/index.min.html
  echo ')rawliteral";'
} > src/ui_index_min.cpp

echo "✅ C++ minifié généré vers src/ui_index_min.cpp"
echo "📊 Taille originale: $(wc -c < src/ui_index.cpp) bytes"
echo "📊 Taille minifiée: $(wc -c < src/ui_index_min.cpp) bytes"
EOF

chmod +x scripts/minify_ui.sh

echo "✅ Script de minification créé: scripts/minify_ui.sh"
echo ""
echo "🚀 Usage:"
echo "  ./scripts/minify_ui.sh  # Minifier et générer ui_index_min.cpp"
echo "  # Puis remplacer src/ui_index.cpp par src/ui_index_min.cpp"
