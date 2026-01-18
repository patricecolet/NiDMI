/* Initialisation principale quand le DOM est prêt */
document.addEventListener('DOMContentLoaded', async ()=>{
 /* Initialiser la navigation par onglets */
 initTabs();
 /* Charger l'état du serveur */
 loadStatus();
 /* Charger la configuration mDNS */
 loadMdns();
 /* Charger la configuration OSC */
 loadOscConfig();
 /* Charger la configuration Wi-Fi Station */
 loadStaConfig();
 /* Initialiser les formulaires */
 initForms();
 /* Charger les définitions de composants AVANT de dessiner le board */
 loadComponentDefinitions().then(async () => {
  /* Charger les capacités de la carte, puis dessiner le board */
  await loadCaps();
  /* Dessiner le board SVG avec les pins */
  drawBoard();
  /* Charger les pins déjà configurées (inclut les composants complexes depuis MuxManager) */
  await loadConfiguredPins();
  /* Charger les GPIOs utilisés depuis le backend */
  await loadUsedGpiosFromBackend();
  /* Mettre à jour les visuels après chargement complet */
  updateBusVisuals();
 }).catch(err => {
  console.warn('Erreur chargement définitions composants:', err);
  // Continuer quand même avec le board (sans définitions)
  loadCaps().then(async ()=>{
   drawBoard();
   await loadConfiguredPins();
   await loadUsedGpiosFromBackend();
   updateBusVisuals();
  });
 });
 /* Initialiser le bouton "Enregistrer tout" */
 if($('#saveAllBtn')) $('#saveAllBtn').onclick=saveAll;
 /* Initialiser WebSocket avec un délai pour éviter les conflits */
 setTimeout(()=>{
  initWebSocket();
 }, 2000);
 /* Rafraîchir l'état du serveur toutes les 10 secondes */
 setInterval(loadStatus, 10000);
});
