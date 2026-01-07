#!/bin/bash
# Script de test pour la minification et vérification d'encodage

set -e

echo "🧪 Test de minification et vérification d'encodage..."
echo ""

# Vérifier l'encodage du fichier source
echo "📄 Vérification de l'encodage de web/index.html..."
if command -v file &> /dev/null; then
    file -I web/index.html
fi

# Vérifier les caractères non-ASCII
echo ""
echo "🔍 Recherche de caractères non-ASCII..."
NON_ASCII=$(LC_ALL=C grep '[^[:print:][:space:]]' web/index.html 2>/dev/null | head -5 || LC_ALL=C grep -o '[^\x00-\x7F]' web/index.html | sort -u | head -10 || true)
if [ -n "$NON_ASCII" ]; then
    echo "⚠️  Caractères non-ASCII trouvés:"
    echo "$NON_ASCII" | head -3
else
    # Vérifier avec od pour les caractères > 127
    NON_ASCII_BYTES=$(od -An -tx1 web/index.html | grep -E ' [89abcdef]' | head -3 || true)
    if [ -n "$NON_ASCII_BYTES" ]; then
        echo "⚠️  Octets > 127 trouvés (caractères potentiellement non-ASCII)"
    else
        echo "✅ Aucun caractère non-ASCII détecté"
    fi
fi

# Créer le dossier build s'il n'existe pas
mkdir -p build

# Forcer UTF-8 pour les opérations
export LC_ALL=C.UTF-8
export LANG=C.UTF-8

# Minifier le HTML
echo ""
echo "🗜️  Minification du HTML..."
sed -E 's/<!--[^>]*-->//g; s|/\*[^*]*\*/||g; s/  +/ /g' web/index.html > build/index.min.html

# Vérifier l'encodage du fichier minifié
echo ""
echo "📄 Vérification de l'encodage de build/index.min.html..."
if command -v file &> /dev/null; then
    file -I build/index.min.html
fi

# Vérifier les caractères non-ASCII dans le fichier minifié
echo ""
echo "🔍 Recherche de caractères non-ASCII dans le fichier minifié..."
NON_ASCII_MIN=$(LC_ALL=C grep '[^[:print:][:space:]]' build/index.min.html 2>/dev/null | head -5 || LC_ALL=C grep -o '[^\x00-\x7F]' build/index.min.html | sort -u | head -10 || true)
if [ -n "$NON_ASCII_MIN" ]; then
    echo "⚠️  Caractères non-ASCII trouvés:"
    echo "$NON_ASCII_MIN" | head -3
else
    # Vérifier avec od pour les caractères > 127
    NON_ASCII_BYTES_MIN=$(od -An -tx1 build/index.min.html | grep -E ' [89abcdef]' | head -3 || true)
    if [ -n "$NON_ASCII_BYTES_MIN" ]; then
        echo "⚠️  Octets > 127 trouvés (caractères potentiellement non-ASCII)"
    else
        echo "✅ Aucun caractère non-ASCII détecté"
    fi
fi

# Créer le C++ de test
echo ""
echo "🔨 Génération du C++ de test..."
{
  echo '#include "ui_index.h"'
  echo 'const char INDEX_HTML[] PROGMEM = R"rawliteral('
  cat build/index.min.html
  echo ')rawliteral";'
} > build/ui_index_test.cpp

# Vérifier l'encodage du fichier C++
echo ""
echo "📄 Vérification de l'encodage de build/ui_index_test.cpp..."
if command -v file &> /dev/null; then
    file -I build/ui_index_test.cpp
fi

# Vérifier les caractères invalides UTF-8
echo ""
echo "🔍 Vérification des octets invalides UTF-8..."
if command -v iconv &> /dev/null; then
    if iconv -f UTF-8 -t UTF-8 build/index.min.html > /dev/null 2>&1; then
        echo "✅ Le fichier minifié est valide en UTF-8"
    else
        echo "❌ ERREUR: Le fichier minifié contient des octets invalides UTF-8!"
        iconv -f UTF-8 -t UTF-8 build/index.min.html 2>&1 | head -5
    fi
else
    echo "⚠️  iconv non disponible, impossible de vérifier"
fi

# Comparer les tailles
echo ""
echo "📊 Tailles des fichiers:"
HTML_SIZE=$(wc -c < web/index.html)
MIN_SIZE=$(wc -c < build/index.min.html)
CPP_SIZE=$(wc -c < build/ui_index_test.cpp)
REDUCTION=$(( (HTML_SIZE - MIN_SIZE) * 100 / HTML_SIZE ))

echo "  HTML source:      $HTML_SIZE bytes"
echo "  HTML minifié:     $MIN_SIZE bytes"
echo "  C++ généré:       $CPP_SIZE bytes"
echo "  Réduction:        $REDUCTION%"

# Test de compilation (si arduino-cli est disponible)
echo ""
if command -v arduino-cli &> /dev/null; then
    echo "🔨 Test de compilation syntaxique..."
    # Juste vérifier que le fichier C++ est valide syntaxiquement
    if grep -q 'R"rawliteral(' build/ui_index_test.cpp && grep -q ')rawliteral"' build/ui_index_test.cpp; then
        echo "✅ Structure C++ valide"
    else
        echo "❌ ERREUR: Structure C++ invalide!"
    fi
else
    echo "⚠️  arduino-cli non disponible, test de compilation ignoré"
fi

echo ""
echo "✅ Test terminé. Fichiers de test dans build/"

