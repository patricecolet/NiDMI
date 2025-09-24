#!/bin/bash
# Minification de test pour comparer index.html vs index.test.html
# Sorties: build/index.min.html et build/index.test.min.html

set -euo pipefail

echo "🗜️  Minification (test) de l'UI..."

mkdir -p build

# Fonction de minification sûre (mêmes règles que minify_safe.sh)
minify() {
  local in_file="$1"; local out_file="$2"
  sed -E 's/<!--[^>]*-->//g; s|/\*[^*]*\*/||g; s/  +/ /g' "$in_file" > "$out_file"
}

# Minifier les deux variantes
minify web/index.html build/index.min.html
minify web/index.test.html build/index.test.min.html

echo "✅ Fichiers minifiés:"
echo "  - build/index.min.html"
echo "  - build/index.test.min.html"

# Calcul tailles
O_HTML=$(wc -c < web/index.html)
M_HTML=$(wc -c < build/index.min.html)
O_TEST=$(wc -c < web/index.test.html)
M_TEST=$(wc -c < build/index.test.min.html)

RED_HTML=$(( (O_HTML - M_HTML) * 100 / (O_HTML==0?1:O_HTML) ))
RED_TEST=$(( (O_TEST - M_TEST) * 100 / (O_TEST==0?1:O_TEST) ))

DELTA=$(( M_HTML - M_TEST ))

echo "\n📊 Tailles (octets):"
printf "  index.html         : %8d (source) → %8d (min)  [-%d%%]\n" "$O_HTML" "$M_HTML" "$RED_HTML"
printf "  index.test.html    : %8d (source) → %8d (min)  [-%d%%]\n" "$O_TEST" "$M_TEST" "$RED_TEST"

if [ $DELTA -gt 0 ]; then
  echo "\n✅ Gain test vs original (min): $DELTA octets de moins"
elif [ $DELTA -lt 0 ]; then
  echo "\n⚠️  Test plus gros que l'original (min): $((-DELTA)) octets en plus"
else
  echo "\nℹ️  Tailles minifiées identiques"
fi

echo "\n📁 Sorties:"
echo "  build/index.min.html"
echo "  build/index.test.min.html"

exit 0


