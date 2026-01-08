/* Variables globales et utilitaires de base */

const $=s=>document.querySelector(s[0]=='#'?s:'#'+s);
const pcfg={};
let cur='';
let caps=null;
const prect={};
const FC={DIGITAL:'#3B82F6',ANALOG:'#EC4899',I2C:'#10B981',UART:'#6B7280',SPI:'#8B5CF6',TOUCH:'#F59E0B',POWER:'#EF4444',GND:'#000'};
let muxList=[];
let websocket=null;

function initTabs(){
 document.querySelectorAll('.tab').forEach(t=>{
 t.onclick=()=>{
 document.querySelectorAll('.tab').forEach(x=>x.classList.remove('active'));
 document.querySelectorAll('.p').forEach(p=>p.classList.remove('active'));
 t.classList.add('active');
 $(`panel-${t.dataset.t}`).classList.add('active');
 };
 });
}
