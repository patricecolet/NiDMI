# Guide de Localisation de l'Interface Web

Ce guide explique comment utiliser le système de localisation pour changer la langue de l'interface web NiDMI entre français et anglais.

## 📋 Vue d'ensemble

Le système de localisation utilise des fichiers JSON pour stocker les traductions et remplace automatiquement les placeholders `{{t.clé}}` dans le HTML et JavaScript lors du build.

### Structure

```
web/
├── lang/
│   ├── fr.json    # Traductions françaises
│   └── en.json    # Traductions anglaises
├── index.html     # Template HTML avec placeholders {{t.xxx}}
└── js/            # Fichiers JavaScript
```

## 🚀 Utilisation

### Option 1 : Via `esp32server.sh` (recommandé)

```bash
# Build en français (défaut)
./scripts/esp32server.sh sync

# Build en anglais
./scripts/esp32server.sh sync --lang en

# Upload avec interface anglaise
./scripts/esp32server.sh upload --lang en
```

### Option 2 : Directement avec `build_html_simple.sh`

```bash
# Français (défaut)
./scripts/build_html_simple.sh

# Anglais
LANG_CODE=en ./scripts/build_html_simple.sh
```

## 📦 Prérequis

### Installation de jq

Le système de traduction nécessite `jq` pour parser les fichiers JSON :

**macOS :**
```bash
brew install jq
```

**Linux (Debian/Ubuntu) :**
```bash
sudo apt-get install jq
```

**Linux (Fedora/RHEL) :**
```bash
sudo dnf install jq
```

**Vérification :**
```bash
jq --version
```

Si `jq` n'est pas installé et que vous utilisez une langue autre que français, le script affichera une erreur claire.

## 📝 Format des fichiers de traduction

Les fichiers JSON sont organisés par sections logiques :

```json
{
  "lang": "fr",
  "title": "ESP32 Server",
  "tabs": {
    "status": "Statut",
    "connection": "Connection"
  },
  "forms": {
    "save": "Enregistrer",
    "serverName": "Nom du serveur"
  }
}
```

### Structure recommandée

- **Clés simples** : Pour les valeurs directement utilisables (ex: `title`, `subtitle`)
- **Clés imbriquées** : Pour grouper logiquement (ex: `tabs.status`, `forms.save`)
- **Notation pointée** : Utiliser la notation `section.key` pour accéder aux valeurs imbriquées

## 🔧 Utilisation dans le HTML

### Placeholders

Remplacer les textes statiques par des placeholders `{{t.clé}}` :

**Avant :**
```html
<h1>ESP32 Server</h1>
<p>Configuration Wi‑Fi, RTP‑MIDI et OSC</p>
<button>Enregistrer</button>
```

**Après :**
```html
<h1>{{t.title}}</h1>
<p>{{t.subtitle}}</p>
<button>{{t.forms.save}}</button>
```

### Clés imbriquées

Pour les clés imbriquées, utiliser la notation pointée :

```html
<div class="tab">{{t.tabs.status}}</div>
<div class="tab">{{t.tabs.connection}}</div>
```

### Attribut lang

Mettre à jour l'attribut `lang` de la balise HTML :

```html
<html lang="{{t.lang}}">
```

## 🔧 Utilisation dans le JavaScript

Les placeholders peuvent aussi être utilisés dans le JavaScript :

```javascript
const message = '{{t.forms.save}}';
console.log('{{t.status.connected}}');
```

**Note :** Les traductions JavaScript sont appliquées avant la minification, donc les placeholders sont remplacés avant la compression gzip.

## 📚 Ajouter une nouvelle langue

### 1. Créer le fichier JSON

Créer `web/lang/nouvelle_langue.json` (ex: `web/lang/es.json` pour l'espagnol) :

```json
{
  "lang": "es",
  "title": "Servidor ESP32",
  "tabs": {
    "status": "Estado",
    "connection": "Conexión"
  }
}
```

### 2. Modifier les scripts

**`scripts/esp32server.sh` :**

Modifier la validation de la langue :

```bash
# Validation de la langue
if [ "$LANG_CODE" != "fr" ] && [ "$LANG_CODE" != "en" ] && [ "$LANG_CODE" != "es" ]; then
    echo "⚠️  Langue non supportée: $LANG_CODE"
    LANG_CODE="fr"
fi
```

**`scripts/build_html_simple.sh` :**

Même modification dans la validation :

```bash
if [ "$LANG_CODE" != "fr" ] && [ "$LANG_CODE" != "en" ] && [ "$LANG_CODE" != "es" ]; then
    echo "⚠️  Langue non supportée: $LANG_CODE, utilisation de 'fr' par défaut"
    LANG_CODE="fr"
fi
```

### 3. Utiliser la nouvelle langue

```bash
./scripts/esp32server.sh sync --lang es
```

## 🔍 Dépannage

### Erreur "jq non trouvé"

```
❌ Erreur: jq est requis pour les traductions
📝 Installation: brew install jq (macOS) ou sudo apt-get install jq (Linux)
```

**Solution :** Installer jq (voir section Prérequis ci-dessus).

### Erreur "Fichier de traduction non trouvé"

```
❌ Erreur: Fichier de traduction non trouvé: web/lang/xx.json
```

**Solution :** Vérifier que le fichier JSON existe dans `web/lang/` et que la langue est supportée.

### Placeholders non remplacés

Si les placeholders `{{t.xxx}}` ne sont pas remplacés :

1. Vérifier que la clé existe dans le fichier JSON
2. Vérifier que la notation est correcte (ex: `{{t.tabs.status}}` pour une clé imbriquée)
3. Vérifier que jq est installé et fonctionne : `jq --version`
4. Vérifier les logs du script pour voir les erreurs de parsing

### Langue non prise en compte

Si la langue sélectionnée n'est pas appliquée :

1. Vérifier que `--lang` est bien passé à `esp32server.sh`
2. Vérifier que la variable `LANG_CODE` est exportée (visible dans les logs)
3. Pour un build manuel, utiliser `LANG_CODE=en ./scripts/build_html_simple.sh`

## 📊 Exemples complets

### Exemple 1 : Build complet en anglais

```bash
# 1. Synchroniser avec interface anglaise
./scripts/esp32server.sh sync --lang en

# 2. Compiler
./scripts/esp32server.sh compile --lang en

# 3. Uploader
./scripts/esp32server.sh upload --lang en
```

### Exemple 2 : Build français (défaut)

```bash
# Pas besoin de spécifier --lang, français par défaut
./scripts/esp32server.sh sync
./scripts/esp32server.sh upload
```

### Exemple 3 : Vérifier les traductions

```bash
# Build avec langue anglaise
LANG_CODE=en ./scripts/build_html_simple.sh

# Vérifier le HTML généré
grep -o '{{t\.[^}]*}}' build/index.min.html

# Si aucun placeholder n'est trouvé, les traductions ont été appliquées
```

## 🎯 Bonnes pratiques

1. **Toujours tester** après avoir ajouté/modifié des traductions
2. **Maintenir la cohérence** : utiliser les mêmes clés dans HTML et JS
3. **Organiser par sections** : grouper les clés logiquement (tabs, forms, status, etc.)
4. **Documenter les nouvelles clés** : ajouter un commentaire dans le JSON si nécessaire
5. **Vérifier les caractères spéciaux** : certains caractères peuvent nécessiter un échappement

## 📖 Références

- **jq** : https://stedolan.github.io/jq/
- **Format JSON** : https://www.json.org/
- **Scripts** : `scripts/esp32server.sh`, `scripts/build_html_simple.sh`
- **Fichiers de traduction** : `web/lang/fr.json`, `web/lang/en.json`

---

*Guide créé pour faciliter la localisation de l'interface web NiDMI*
*Dernière mise à jour : Janvier 2025*
