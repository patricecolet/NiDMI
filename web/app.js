/* Initialisation principale quand le DOM est prêt */
document.addEventListener('DOMContentLoaded', ()=>{
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
 /* Initialiser le formulaire de multiplexeur */
 initMuxForm();
 /* Charger les capacités de la carte, puis dessiner le board */
 loadCaps().then(async ()=>{
  /* Dessiner le board SVG avec les pins */
  drawBoard();
  /* Charger les pins déjà configurées */
  await loadConfiguredPins();
  /* Charger la liste des multiplexeurs */
  await loadMuxList();
  /* Mettre à jour les visuels après chargement complet */
  updateBusVisuals();
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
