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
 /* Charger l'état des interfaces MIDI globales */
 loadMidiInterfaces();
 /* Initialiser les formulaires */
 initForms();
 /* Charger les définitions de composants AVANT de dessiner le board */
 loadComponentDefinitions().then(async () => {
  /* Charger les capacités de la carte, puis dessiner le board */
  await loadCaps();
  /* Vérifier le type de board et charger la config système (si S3) */
  if (typeof initSystemConfig === 'function') {
    await initSystemConfig();
  }
  /* Dessiner le board SVG avec les pins */
  drawBoard();
  /* Charger les pins déjà configurées (inclut les composants complexes depuis MuxManager) */
  await loadConfiguredPins();
  /* Charger les GPIOs utilisés depuis le backend */
  await loadUsedGpiosFromBackend();
  /* Mettre à jour les visuels après chargement complet */
  updateBusVisuals();
  /* Initialiser le chargeur de fichiers NiDMI */
  initNidmidLoader();

  /* Vue globale*/
  if (typeof initGlobalView === 'function') initGlobalView();
  if (typeof loadGlobalConfig === 'function') await loadGlobalConfig();
  if (typeof applyPinsViewMode === 'function') applyPinsViewMode();

 }).catch(err => {
  console.warn('Erreur chargement définitions composants:', err);
  // Continuer quand même avec le board (sans définitions)
  loadCaps().then(async ()=>{
   /* Vérifier le type de board et charger la config système (si S3) */
   if (typeof initSystemConfig === 'function') {
     await initSystemConfig();
   }
   drawBoard();
   await loadConfiguredPins();
   await loadUsedGpiosFromBackend();
   updateBusVisuals();

   /* Vue globale : fallback*/
    if (typeof initGlobalView === 'function') initGlobalView();
    if (typeof loadGlobalConfig === 'function') await loadGlobalConfig();
    if (typeof applyPinsViewMode === 'function') applyPinsViewMode();
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
