/**
 * Initialisation des formulaires (mDNS, STA, OSC).
 * Fonction extraite de api.js et exposée globalement pour compatibilité.
 */

function initForms(){
 const oscTarget = $('#oscTarget');
 const oscIpRow = $('#oscIpRow');
 const oscBroadcast = $('#oscBroadcast');
 
 function updateOscForm() {
 const target = oscTarget.value;
 if (target === 'ip') {
 oscIpRow.style.display = 'block';
 oscBroadcast.checked = false;
 } else {
 oscIpRow.style.display = 'none';
 oscBroadcast.checked = true;
 }
 }
 
 if (oscTarget) {
 oscTarget.addEventListener('change', updateOscForm);
 updateOscForm();
 }
 
 $('#mdns').addEventListener('submit', async (e) => {
 e.preventDefault();
 const formData = new FormData();
 formData.append('name', $('#mdnsName').value);
 try {
 const r = await fetch('/api/mdns', { method: 'POST', body: formData });
 const d = await r.json();
 $('#mdnsMsg').textContent = d.status === 'ok' ? 'Nom enregistré' : 'Erreur: ' + d.error;
 $('#mdnsMsg').style.color = d.status === 'ok' ? '#059669' : '#dc2626';
 } catch (err) {
 $('#mdnsMsg').textContent = 'Erreur de connexion';
 $('#mdnsMsg').style.color = '#dc2626';
 }
 });
 
 $('#sta').addEventListener('submit', async (e) => {
 e.preventDefault();
 const formData = new FormData();
 formData.append('ssid', $('#ssid').value);
 formData.append('pass', $('#pass').value);
 try {
 const r = await fetch('/api/sta', { method: 'POST', body: formData });
 const d = await r.json();
 $('#staMsg').textContent = d.status === 'ok' ? 'Configuration enregistrée, redémarrage...' : 'Erreur: ' + d.error;
 $('#staMsg').style.color = d.status === 'ok' ? '#059669' : '#dc2626';
 if (d.status === 'ok') {
 setTimeout(() => location.reload(), 2000);
 }
 } catch (err) {
 $('#staMsg').textContent = 'Configuration enregistrée, redémarrage...';
 $('#staMsg').style.color = '#059669';
 setTimeout(() => location.reload(), 5000);
 }
 });
 
 $('#osc').addEventListener('submit', async (e) => {
 e.preventDefault();
 const formData = new FormData();
 const target = $('#oscTarget').value;
 formData.append('target', target);
 formData.append('port', $('#oscPort').value);
 
 if (target === 'ip' && $('#oscIp').value) {
 formData.append('ip', $('#oscIp').value);
 }
 
 if ($('#oscBroadcast')) {
 formData.append('broadcast', $('#oscBroadcast').checked ? 'true' : 'false');
 }
 
 try {
 const r = await fetch('/api/osc', { method: 'POST', body: formData });
 const d = await r.json();
 $('#oscMsg').textContent = d.status === 'ok' ? 'Configuration OSC enregistrée' : 'Erreur: ' + d.error;
 $('#oscMsg').style.color = d.status === 'ok' ? '#059669' : '#dc2626';
 } catch (err) {
 $('#oscMsg').textContent = 'Erreur de connexion';
 $('#oscMsg').style.color = '#dc2626';
 }
 });
}

