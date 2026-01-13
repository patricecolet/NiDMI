# Upload du Firmware sur macOS

Guide concis pour uploader le firmware sur le XIAO ESP32-C3/S3 depuis macOS.

## Prérequis (Installation)

### Installer Python 3 et pip3

```bash
# Installer Homebrew si ce n'est pas déjà fait
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Installer Python 3 (qui inclut pip3)
brew install python3
```

### Vérifier l'installation

```bash
python3 --version
pip3 --version
```

## Méthode 1 : Script automatique (Recommandé)

### Compiler et uploader automatiquement

```bash
cd /Users/patricecolet/repo/NiDMI
./scripts/esp32server.sh upload
```

### Compiler et générer le fichier binaire

```bash
./scripts/esp32server.sh build
```

Le fichier binaire sera généré dans `bin/esp32server_basic.ino.merged.bin` et le fichier UF2 dans `bin/esp32server_basic.uf2`.

## Méthode 2 : Arduino IDE

### Via l'interface graphique

1. Ouvrir Arduino IDE
2. Fichier > Ouvrir > Sélectionner le sketch dans `examples/esp32server_basic/`
3. Outils > Type de carte > XIAO_ESP32C3 (ou XIAO_ESP32S3)
4. Outils > Port > Sélectionner le port série
5. Bouton "Téléverser" (compilera puis uploadera automatiquement)

**Note** : Arduino IDE compile toujours avant d'uploader. Il n'y a pas d'option pour uploader sans compiler.

## Méthode 3 : esptool.py en ligne de commande

### Installer esptool

```bash
pip3 install esptool
```

### Trouver le port série

```bash
# Lister les ports série disponibles
ls /dev/cu.usbmodem* /dev/cu.SLAB_USBtoUART* 2>/dev/null
```

### Uploader le firmware

**Pour ESP32-C3 :**
```bash
esptool.py --chip esp32c3 \
  --port /dev/cu.usbmodem* \
  write_flash 0x0 bin/esp32server_basic.ino.merged.bin
```

**Pour ESP32-S3 :**
```bash
esptool.py --chip esp32s3 \
  --port /dev/cu.usbmodem* \
  write_flash 0x0 bin/esp32server_basic.ino.merged.bin
```

### Utiliser esptool.py inclus avec Arduino IDE

Si vous préférez utiliser l'esptool.py inclus avec Arduino IDE (sans installer Python globalement) :

```bash
# Trouver esptool.py
ESPTOOL=$(find ~/Library/Arduino15/packages/esp32/tools/esptool_py -name esptool.py 2>/dev/null | head -1)

# Uploader
python3 "$ESPTOOL" --chip esp32c3 --port /dev/cu.usbmodem* write_flash 0x0 bin/esp32server_basic.ino.merged.bin
```

## Méthode 4 : Format UF2 (non supporté nativement)

Le format UF2 est généré automatiquement par le script (`./scripts/esp32server.sh build`), mais **le XIAO ESP32-C3/S3 ne supporte pas nativement le drag-and-drop UF2** comme le RP2040.

Pour ESP32, utilisez les méthodes ci-dessus (Arduino IDE ou esptool.py).

## Résumé des commandes rapides

```bash
# 1. Compiler + uploader (tout automatique)
./scripts/esp32server.sh upload

# 2. Compiler uniquement (génère le .bin)
./scripts/esp32server.sh build

# 3. Uploader avec esptool (nécessite pip3 install esptool)
esptool.py --chip esp32c3 --port /dev/cu.usbmodem* write_flash 0x0 bin/esp32server_basic.ino.merged.bin
```

## Dépannage

### Port série non trouvé

```bash
# Vérifier que le XIAO est connecté
ls /dev/cu.*

# Si aucun port n'apparaît :
# - Vérifier le câble USB
# - Vérifier que le XIAO est allumé
# - Essayer un autre câble USB (certains ne transmettent que l'alimentation)
```

### Erreur de permissions

```bash
# Sur macOS, généralement pas nécessaire, mais si problème :
sudo chmod 666 /dev/cu.usbmodem*
```

### esptool.py non trouvé

```bash
# Vérifier si Python 3 est installé
python3 --version

# Installer esptool
pip3 install esptool

# Ou utiliser celui inclus avec Arduino IDE
find ~/Library/Arduino15/packages/esp32/tools/esptool_py -name esptool.py
```
