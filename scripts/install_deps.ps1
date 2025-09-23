# Requires: arduino-cli installed (winget install ArduinoLLC.ArduinoCLI)
$ErrorActionPreference = "Stop"

if (-not (Get-Command arduino-cli -ErrorAction SilentlyContinue)) {
  Write-Error "arduino-cli non trouvé. Installez-le: winget install ArduinoLLC.ArduinoCLI"
}

arduino-cli config init | Out-Null
arduino-cli core update-index
arduino-cli core install esp32:esp32

arduino-cli lib update-index
arduino-cli lib install "ESP Async WebServer"
arduino-cli lib install "AsyncTCP"

Write-Output "OK: core esp32 et bibliothèques installés."
Write-Output "Ouvrez l’IDE Arduino et téléversez Fichier > Exemples > ESP32Server > esp32server_basic"
