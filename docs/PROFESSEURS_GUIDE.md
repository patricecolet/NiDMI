# Guide Professeurs - NiDMI MIDI

## Table des matières
1. [Vue d'ensemble](#vue-densemble)
2. [Configuration des navigateurs](#configuration-des-navigateurs)
3. [Capacités techniques](#capacités-techniques)
4. [Projets pédagogiques](#projets-pédagogiques)
5. [Budget et matériel](#budget-et-matériel)
6. [Progression d'apprentissage](#progression-dapprentissage)
7. [Exemples concrets](#exemples-concrets)

---

## Vue d'ensemble

### Qu'est-ce que l'NiDMI MIDI ?
L'NiDMI MIDI est une plateforme éducative permettant de créer des contrôleurs MIDI professionnels avec des capteurs variés. Les étudiants apprennent l'électronique, la programmation et la musique de manière interactive.

### Avantages pédagogiques
- ✅ **Apprentissage progressif** (débutant → expert)
- ✅ **Projets concrets** et motivants
- ✅ **Interface web intuitive** (pas de programmation complexe)
- ✅ **Budget maîtrisé** (23€ pour 16 potentiomètres)
- ✅ **Collaboration** entre étudiants

---

## Configuration des navigateurs

### Interface web et compatibilité navigateur

**✅ Compatibilité** : L'interface web fonctionne avec tous les navigateurs (Firefox, Chrome, Brave, Safari).

#### 🌐 Navigateurs recommandés
- **Firefox** : Fonctionne immédiatement, recommandé pour les ateliers
- **Chrome/Brave** : Fonctionnent aussi pour la configuration
- **Safari** : Compatible mais peut avoir des limitations

### 🎵 Web MIDI (Fonctionnalité future)

**⚠️ Note** : Web MIDI n'est pas encore implémenté dans l'interface web actuelle.

**Planifié pour plus tard** :
- Une page de test Web MIDI sera créée sur GitHub (HTTPS)
- Cette page permettra de tester Web MIDI avec l'ESP32
- Firefox sera recommandé pour cette fonctionnalité
- La page GitHub sera accessible via HTTPS, permettant l'utilisation de Web MIDI même avec Chrome/Brave

**Actuellement disponible** :
- Interface web complète pour configuration
- RTP-MIDI (fonctionne avec macOS/Logic)
- OSC (Open Sound Control)
- Configuration des pins en temps réel

### Accès à l'interface web

1. **Connexion** : L'ESP32 crée un point d'accès WiFi `NiDMI-XXXX`
2. **Interface** : Ouvrir `http://192.168.4.1` ou `http://myesp32.local` dans un navigateur
3. **Configuration** : Interface web complète pour configurer les pins, MIDI, OSC, etc.

**Note** : L'interface web fonctionne avec tous les navigateurs. Firefox est recommandé pour une meilleure compatibilité.

---

## Capacités techniques

### Mémoire disponible
- **Total ESP32** : 1.3MB Flash
- **Utilisé** : 1.1MB (84%)
- **Disponible** : 200ko pour les capteurs

### Types de capteurs supportés

#### Composants de base
- **Potentiomètres** : Contrôle analogique (CC MIDI)
- **Boutons** : Contrôle digital (Note MIDI)
- **LEDs** : Feedback visuel (contrôlé par MIDI)

#### Capteurs environnementaux (20ko)
- **DHT22** : Température/Humidité haute précision (2ko)
- **BME280** : Pression/Température/Humidité (3ko)
- **BH1750** : Luminosité (1ko)
- **MQ135** : Qualité de l'air (1ko)
- **SHT30** : Température/Humidité précise (2ko)
- **BMP280** : Pression/Température (2ko)
- **DS18B20** : Température OneWire (2ko)
- **LM35** : Température analogique (1ko)
- **Si7021** : Température/Humidité I2C (2ko)
- **AM2320** : Température/Humidité (1ko)
- **CCS811** : CO2/Composés organiques (3ko)

#### Capteurs de mouvement (25ko)
- **MPU6050** : Accéléromètre/Gyroscope (4ko)
- **HMC5883L** : Magnétomètre (2ko)
- **ADXL345** : Accéléromètre 3 axes (2ko)
- **LSM303** : Accéléromètre + Magnétomètre (3ko)
- **BNO055** : IMU 9-DOF (4ko)
- **LSM9DS1** : IMU 9-DOF (3ko)
- **LSM6DS3** : Accéléromètre + Gyroscope (3ko)
- **BMI160** : IMU 6-DOF (2ko)
- **ICM20948** : IMU 9-DOF (2ko)

#### Capteurs audio/vibration (15ko)
- **MAX4466** : Microphone (1ko)
- **MAX9814** : Microphone avec AGC (2ko)
- **DFPlayer** : Lecteur audio (3ko)
- **VS1053** : Codec audio (5ko)
- **WM8960** : Codec audio avancé (4ko)

#### Capteurs de distance/proximité (10ko)
- **VL53L0X** : Distance laser (3ko)
- **VL6180X** : Proximité (2ko)
- **HC-SR04** : Ultrason (1ko)
- **RCWL0516** : Radar Doppler (1ko)
- **GP2Y0A21YK** : Distance infrarouge (1ko)
- **Sharp_IR** : Distance infrarouge (2ko)

#### Capteurs de gaz/qualité air (15ko)
- **MQ2** : Fumée/Gaz (1ko)
- **MQ7** : Monoxyde de carbone (1ko)
- **MQ135** : Qualité de l'air (1ko)
- **TGS2600** : Gaz général (1ko)
- **TGS2610** : Méthane (1ko)
- **TGS2620** : Alcool (1ko)
- **TGS4161** : CO2 (1ko)
- **TGS2602** : Composés organiques (1ko)
- **SGP30** : CO2/Composés organiques (3ko)
- **SCD30** : CO2/Température/Humidité (4ko)

#### Capteurs spécialisés (20ko)
- **PIR** : Détecteur de mouvement (1ko)
- **Piezo** : Détecteur de chocs (1ko)
- **Hall Effect** : Détecteur magnétique (1ko)
- **Touch** : Capteur tactile (1ko)
- **Flex** : Capteur de flexion (1ko)
- **Strain** : Capteur de contrainte (1ko)
- **Load Cell** : Capteur de force (2ko)
- **Ultrasonic** : Distance ultrason (1ko)
- **Infrared** : Détection infrarouge (1ko)
- **Color** : Capteur de couleur (2ko)
- **UV** : Capteur UV (1ko)
- **Sound** : Capteur de son (1ko)
- **Vibration** : Capteur de vibration (1ko)
- **Tilt** : Capteur d'inclinaison (1ko)
- **Proximity** : Capteur de proximité (1ko)

#### Afficheurs et écrans (25ko)
- **SSD1306** : OLED 128x64 (3ko)
- **SSD1327** : OLED 128x128 (4ko)
- **ST7735** : TFT 128x160 (5ko)
- **ILI9341** : TFT 240x320 (6ko)
- **ST7789** : TFT 240x240 (4ko)
- **MAX7219** : Matrice LED 8x8 (2ko)
- **TM1637** : Affichage 7 segments (1ko)
- **LCD1602** : LCD 16x2 (2ko)
- **LCD2004** : LCD 20x4 (3ko)

#### LEDs et éclairage (15ko)
- **WS2812B** : LED RGB adressable (2ko)
- **APA102** : LED RGB SPI (2ko)
- **SK6812** : LED RGB (2ko)
- **WS2811** : LED RGB (1ko)
- **NeoPixel** : LED RGB (1ko)
- **RGB LED** : LED RGB simple (1ko)
- **LED Strip** : Bande LED (2ko)
- **LED Matrix** : Matrice LED (3ko)
- **LED Ring** : Anneau LED (2ko)

#### Encodeurs et contrôleurs (10ko)
- **Rotary Encoder** : Encodeur rotatif (1ko)
- **Quadrature Encoder** : Encodeur quadratique (1ko)
- **Incremental Encoder** : Encodeur incrémental (1ko)
- **Absolute Encoder** : Encodeur absolu (2ko)
- **Magnetic Encoder** : Encodeur magnétique (2ko)
- **Optical Encoder** : Encodeur optique (1ko)
- **Hall Encoder** : Encodeur à effet Hall (1ko)
- **Potentiometer Encoder** : Encodeur potentiométrique (1ko)

#### Moteurs et actionneurs (20ko)
- **Servo Motor** : Moteur servo (2ko)
- **Stepper Motor** : Moteur pas à pas (3ko)
- **DC Motor** : Moteur continu (2ko)
- **Brushless Motor** : Moteur brushless (3ko)
- **Solenoid** : Solénoïde (1ko)
- **Linear Actuator** : Actionneur linéaire (2ko)
- **Pneumatic Actuator** : Actionneur pneumatique (2ko)
- **Hydraulic Actuator** : Actionneur hydraulique (2ko)
- **Vibrator Motor** : Moteur vibrant (1ko)
- **Fan Motor** : Moteur ventilateur (1ko)

### Capacité maximale
- **500+ composants** configurables
- **30+ bibliothèques** spécialisées
- **Interface web** complète
- **Configuration en temps réel**

---

## Projets pédagogiques

### Niveau 1 : Premier Potentiomètre
**Objectif** : Comprendre le MIDI de base
- **Matériel** : 1 potentiomètre, 1 ESP32
- **Durée** : 2 heures
- **Résultat** : Contrôleur CC simple

### Niveau 2 : Multiplexeur
**Objectif** : Optimiser l'utilisation des pins
- **Matériel** : 1 multiplexeur, 16 potentiomètres
- **Durée** : 4 heures
- **Résultat** : Contrôleur 16 CCs

### Niveau 3 : Projet Final
**Objectif** : Créer un contrôleur professionnel
- **Matériel** : Capteurs variés selon le projet
- **Durée** : 20 heures
- **Résultat** : Contrôleur MIDI complet

---

## Budget et matériel

### Kit de base (23€)
- **ESP32** : 5€
- **Multiplexeur CD4067** : 2€
- **16 potentiomètres** : 16€
- **Total** : 23€

### Kit avancé (50€)
- **ESP32** : 5€
- **4 multiplexeurs** : 8€
- **64 potentiomètres** : 32€
- **Capteurs environnementaux** : 5€
- **Total** : 50€

### Kit professionnel (100€)
- **ESP32** : 5€
- **Capteurs de mouvement** : 20€
- **Capteurs audio** : 15€
- **Capteurs spécialisés** : 30€
- **Matériel de construction** : 30€
- **Total** : 100€

---

## Progression d'apprentissage

### Semestre 1 : Bases
**Semaine 1-2** : Introduction MIDI
- Théorie du MIDI
- Premier potentiomètre
- Interface web

**Semaine 3-4** : Multiplexeurs
- Optimisation des pins
- 16 potentiomètres
- Configuration avancée

**Semaine 5-6** : Capteurs de base
- Boutons et LEDs
- Capteurs IR
- Ultrason

### Semestre 2 : Avancé
**Semaine 7-8** : Capteurs environnementaux
- Température/Humidité
- Pression atmosphérique
- Luminosité

**Semaine 9-10** : Capteurs de mouvement
- Accéléromètre
- Gyroscope
- Magnétomètre

**Semaine 11-12** : Projet final
- Choix du projet
- Implémentation
- Présentation

---

## Exemples concrets

### Projet 1 : Mini-Synthesizer
**Objectif** : Contrôler un synthétiseur
- **4 potentiomètres** : Fréquence, Filtre, LFO, Volume
- **4 boutons** : Waveform (Saw, Square, Triangle, Noise)
- **Interface** : Configuration web

### Projet 2 : Drum Machine
**Objectif** : Créer une machine à rythmes
- **16 boutons** : Pads de batterie
- **4 potentiomètres** : Tempo, Volume, Reverb, Delay
- **Interface** : Séquenceur simple

### Projet 3 : Controller DJ
**Objectif** : Contrôler un logiciel DJ
- **2 crossfaders** : A/B, C/D
- **8 potentiomètres** : EQ, Effects
- **8 boutons** : Play, Stop, Cue, Loop

### Projet 4 : Station Météo MIDI
**Objectif** : Convertir les données météo en MIDI
- **DHT22** : Température → CC 3
- **BME280** : Pression → CC 5
- **BH1750** : Luminosité → CC 6
- **MQ135** : Qualité de l'air → CC 7

### Projet 5 : Contrôleur de Mouvement
**Objectif** : Contrôler avec les gestes
- **MPU6050** : Accélération → CCs 7-9
- **HMC5883L** : Boussole → CC 10
- **PIR** : Mouvement → Note 60

---

## Avantages pour l'enseignement

### Pour les professeurs
- **Pédagogie modulaire** : Progression claire
- **Budget maîtrisé** : Coût par étudiant < 50€
- **Projets collaboratifs** : Travail en équipe
- **Évaluation facile** : Démonstration concrète

### Pour les étudiants
- **Apprentissage progressif** : Du simple au complexe
- **Projets concrets** : Résultats visibles
- **Interface intuitive** : Pas de programmation complexe
- **Créativité** : Liberté dans les projets

### Pour l'institution
- **Réputation** : Projets innovants
- **Recrutement** : Attractivité des cours
- **Partenariats** : Collaboration avec l'industrie
- **Recherche** : Projets de fin d'études

---

## Conclusion

L'NiDMI MIDI offre une plateforme éducative complète pour l'apprentissage de l'électronique, de la programmation et de la musique. Avec 200ko de mémoire disponible, les possibilités sont énormes : 500+ composants, 30+ bibliothèques spécialisées, et des projets collaboratifs complexes.

**C'est l'avenir de l'éducation musicale et électronique !** 🎵🔬✨
