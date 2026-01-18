/* Variables globales et utilitaires de base */

/* Sélecteur DOM simplifié - retourne un élément par ID ou sélecteur */
const $=s=>{
 if(typeof document === 'undefined') return null;
 if(!s || typeof s !== 'string') return null;
 return document.querySelector(s[0]=='#'?s:'#'+s);
};

/* Configuration des pins - objet global qui stocke toutes les configs */
const pcfg={};

/* Pin actuellement sélectionnée dans l'interface */
let cur='';

/* Capacités de la carte ESP32 (pins disponibles, ADC, etc.) */
let caps=null;

/* Mapping des rectangles SVG vers les labels de pins */
const prect={};

/* Codes couleur pour les différents types de pins */
const FC={DIGITAL:'#3B82F6',ANALOG:'#EC4899',I2C:'#10B981',UART:'#6B7280',SPI:'#8B5CF6',TOUCH:'#F59E0B',POWER:'#EF4444',GND:'#000'};

/* Instance WebSocket pour synchronisation temps réel */
let websocket=null;


/* Initialise la navigation par onglets */
function initTabs(){
 /* Parcourir tous les onglets et leur ajouter un gestionnaire de clic */
 document.querySelectorAll('.tab').forEach(t=>{
  t.onclick=()=>{
   /* Retirer la classe active de tous les onglets */
   document.querySelectorAll('.tab').forEach(x=>x.classList.remove('active'));
   /* Retirer la classe active de tous les panneaux */
   document.querySelectorAll('.p').forEach(p=>p.classList.remove('active'));
   /* Ajouter la classe active à l'onglet cliqué */
   t.classList.add('active');
   /* Afficher le panneau correspondant */
   $(`panel-${t.dataset.t}`).classList.add('active');
  };
 });
}
