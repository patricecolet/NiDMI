# TODO – Plan d’implémentation (Pins, Démo par défaut, MIDI/OSC)

## 1) Config par défaut (démo au boot)
- A0/A1/A2/A3: Potentiomètres → MIDI CC 1/5/7/71 (canal 1) + OSC /ctl/mod, /ctl/vol, /ctl/porta, /ctl/cutoff
- D4/D5/D6: Boutons poussoirs → Notes 60/65/67 (Note On 127 / Note Off 0) + OSC /note 60/65/67
- D7/D8/D9: LED (digital) → s’allume Note On 60/65/67, s’éteint Note Off + idem OSC
- D10: LED PWM → luminosité = MIDI CC7 (volume)

## 2) Persistance (NVS)
- Schéma de données: par pin (gpio), rôle (pot, bouton, led, led_pwm, i2c, spi, uart, power, gnd), params (chan, cc/note, osc_path, etc.)
- Chargement au boot (si NVS vide → config par défaut). Sauvegarde via API.

## 3) API HTTP
- GET /api/pins/config → renvoie la configuration complète
- POST /api/pins/set → gpio, role, params (json ou x-www-form-urlencoded)
- POST /api/pins/save → persiste en NVS

## 4) UI (onglet Pins)
- Menu « Fonction du pin » avec rôles réels: Potentiomètre, Bouton, LED, LED PWM, I2C (SDA/SCL), SPI (MOSI/MISO/SCK), UART (TX/RX), Power, GND
- Clic sur carrés:
  - Dn → fixe type Digital (rôle selon menu)
  - Ax → fixe type Analog (rôle Potentiomètre)
  - SDA active SCL; MOSI active MISO+SCK; TX/RX non couplés obligatoirement
- Sélection visuelle stable (highlight) et synchronisation de #selPin + menu
- Quand rôle = Potentiomètre (A0–A3): afficher paramètres:
  - MIDI: Min/Max (0–127)
  - OSC: Min/Max (ex: 0.0–1.0)
  - Filtre: moving average (une seule valeur « filtre » = fenêtre ou alpha)
  (Pas d’inversion, pas de deadzone/seuil séparés)

## 5) Runtime MIDI/OSC
- Entrées analogiques (A0..A3): lecture avec moving average (paramètre « filtre ») → map linéaire sur Min/Max MIDI et Min/Max OSC; émettre sur changement (implémenter un delta minimal implicite issu du filtre)
- Boutons (D4..D6): debouncing → MIDI Note On/Off + OSC /note
- Sorties LED (D7..D9): état sur Note On/Off (MIDI & OSC)
- Sortie PWM (D10): CC7 → duty cycle
- Canal MIDI par défaut = 1; adresse OSC paramétrable

## 6) Démo auto au boot
- Active les pipelines ci-dessus
- Expose l’état dans /api/status

## 7) Documentation
- README: « Démo par défaut » et « Mapper un pin » (exemples rapides)
- Avertissements timing (ADC, debouncing, PWM)

