#!/bin/bash
# Script qui copie web/index.html vers src/ui_index.cpp SANS RIEN TOUCHER

set -e

echo "📄 Copie de web/index.html vers src/ui_index.cpp..."

# Créer le C++ avec le HTML intact
{
  echo '#include "ui_index.h"'
  echo 'const char INDEX_HTML[] PROGMEM = R"rawliteral('
  cat web/index.html
  echo ')rawliteral";'
} > src/ui_index.cpp

echo "✅ HTML copié vers src/ui_index.cpp (sans modification)"
