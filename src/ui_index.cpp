#include "ui_index.h"
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Server - Test</title>
<style>
:root{--cd:#3B82F6;--ca:#EC4899;--ci:#10B981;--cu:#6B7280;--cs:#8B5CF6;--cp:#EF4444;--cg:#000;--bg:#f9fafb;--bd:#e5e7eb;--tx:#374151;--mt:#6b7280}
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#f3f4f6;color:#111827}
.c{max-width:1200px;margin:0 auto;padding:20px}
.h{text-align:center;margin-bottom:30px}.h h1{color:#1f2937;margin-bottom:10px}.h p{color:#6b7280}
.t{display:flex;background:#fff;border-radius:8px;box-shadow:0 1px 3px rgba(0,0,0,.1);margin-bottom:20px}
.tab{flex:1;padding:15px;text-align:center;cursor:pointer;border-bottom:3px solid transparent;transition:all .2s}
.tab.active{border-bottom-color:var(--cd);color:var(--cd);font-weight:600}.tab:hover:not(.active){background:var(--bg)}
.p{display:none;background:#fff;border-radius:8px;padding:25px;box-shadow:0 1px 3px rgba(0,0,0,.1)}.p.active{display:block}
.f{margin-bottom:20px}.f label{display:block;margin-bottom:8px;font-weight:500;color:var(--tx)}
.f input,.f select{width:100%;padding:12px;border:1px solid #d1d5db;border-radius:6px;font-size:14px}
.f input:focus,.f select:focus{outline:none;border-color:var(--cd);box-shadow:0 0 0 3px rgba(59,130,246,.1)}
.btn{background:var(--cd);color:#fff;border:none;padding:12px 24px;border-radius:6px;cursor:pointer;font-size:14px;font-weight:500;transition:background .2s}
.btn:hover{background:#2563eb}.btn:disabled{background:#9ca3af;cursor:not-allowed}
.hint{margin-top:10px;color:var(--mt);font-size:14px}
.g{display:grid;grid-template-columns:1fr 1fr;gap:20px}.card{background:#f8fafc;border:1px solid #e2e8f0;border-radius:6px;padding:15px}
.card h3{color:#1f2937;margin-bottom:10px;font-size:16px}.card p{color:var(--mt);font-size:14px;margin-bottom:5px}
.pl{display:flex;gap:20px;align-items:flex-start}.lp{flex:0 0 30%;min-width:300px}.rp{flex:1 1 auto}
.cp{background:#fff;border:1px solid var(--bd);border-radius:8px;padding:16px}.cp h4{margin:6px 0 10px;font-size:15px;color:#1f2937}
.r{display:flex;gap:12px;align-items:center;margin:8px 0;flex-wrap:wrap}.r label{color:var(--tx);font-size:14px}
.b{width:100%;height:260px;border:1px solid var(--bd);border-radius:8px;background:var(--bg)}
.l{display:flex;gap:14px;align-items:center;margin:10px 0 8px;flex-wrap:wrap}.s{width:14px;height:14px;border-radius:3px;display:inline-block;margin-right:6px}
.s.digital{background:var(--cd)}.s.analog{background:var(--ca)}.s.i2c{background:var(--ci)}.s.uart{background:var(--cu)}.s.spi{background:var(--cs)}.s.touch{background:#F59E0B}.s.power{background:var(--cp)}.s.gnd{background:var(--cg)}
.plist{margin-top:20px;padding:15px;background:var(--bg);border-radius:8px}.plist h4{margin:0 0 10px;font-size:15px;color:var(--tx)}
.list{display:flex;flex-direction:column;gap:4px}
.item{display:flex;align-items:center;padding:8px 12px;background:#fff;border-radius:6px;border-left:4px solid var(--bd);cursor:pointer;transition:all .2s}
.item:hover{background:var(--bg)}.item.analog{border-left-color:var(--ca)}.item.digital{border-left-color:var(--cd)}.item.i2c{border-left-color:var(--ci)}.item.spi{border-left-color:var(--cs)}.item.uart{border-left-color:var(--cu)}.item.touch{border-left-color:#F59E0B}.item.mux{border-left-color:#8B5CF6}
.lbl{font-weight:700;min-width:40px;margin-right:12px}.role{flex:1;color:var(--tx)}.stat{font-size:.9em;color:var(--mt)}
.del-btn{background:#ef4444;color:#fff;border:none;border-radius:3px;width:20px;height:20px;cursor:pointer;font-size:12px;margin-left:8px}
.del-btn:hover{background:#dc2626}
.btn-p{width:100%;margin-top:15px;padding:12px;background:var(--cd);color:#fff;border:none;border-radius:6px;font-weight:700;cursor:pointer}
.btn-p:hover{background:#2563eb}
.svg-t{font-size:9px;fill:#ffffff;dominant-baseline:middle;pointer-events:none;user-select:none}
.selectedSquare{stroke:#1d4ed8;stroke-width:2}
.subcard{background:#f9fafb;border:1px solid var(--bd);border-radius:8px;padding:12px;margin-top:8px}
.subcard .r{margin:6px 0}
.switch{display:flex;align-items:center;gap:8px}
.busDisabled{opacity:0.45;filter:grayscale(100%);cursor:not-allowed}
.modal-overlay{position:fixed;top:0;left:0;right:0;bottom:0;background:rgba(0,0,0,.5);display:none;align-items:center;justify-content:center;z-index:1000}
.modal-overlay.active{display:flex}
.modal{background:#fff;border-radius:8px;padding:25px;max-width:500px;width:90%;max-height:90vh;overflow-y:auto;box-shadow:0 10px 25px rgba(0,0,0,.3)}
.modal h3{margin-bottom:20px;color:#1f2937;font-size:18px}
</style>
<script src="/bundle"></script>
</head>
<body>
 <div class="h"><h1>ESP32 Server</h1><p>Wi‑Fi, RTP‑MIDI and OSC Configuration</p></div>
 <div class="t"><div class="tab active" data-t="status">Status</div><div class="tab" data-t="connection">Connection</div><div class="tab" data-t="pins">Pins</div></div>
 <div class="p active" id="panel-status">
 <div class="g"><div class="card"><h3>Access Point</h3><p><strong>SSID:</strong> <span id="apSsid">-</span></p><p><strong>IP:</strong> <span id="apIp">-</span></p></div><div class="card"><h3>Wi‑Fi Station</h3><p><strong>SSID:</strong> <span id="staSsid">-</span></p><p><strong>IP:</strong> <span id="staIp">-</span></p><p><strong>Status:</strong> <span id="staStatus">-</span></p></div><div class="card"><h3>mDNS</h3><p><strong>Address:</strong> <span id="mdnsAddress">-</span></p></div><div class="card"><h3>OSC</h3><p><strong>Configuration:</strong> <span id="oscConfig">-</span></p></div></div>
 </div>
 <div class="p" id="panel-connection">
 <div class="f"><h3>Server</h3><form id="mdns"><div class="f"><label for="mdnsName">Server name</label><input type="text" id="mdnsName" placeholder="esp32rtpmidi" required><div class="hint"><small>This name will be used for the Wi‑Fi AP and http://name.local</small></div></div><button type="submit" class="btn">Save</button><div class="hint" id="mdnsMsg"></div></form></div>
 <div class="f"><h3>OSC</h3><form id="osc"><div class="f"><label for="oscTarget">Destination</label><select id="oscTarget"><option value="ip">Specific IP</option><option value="ap">Broadcast AP (192.168.4.255)</option><option value="sta" id="oscStaOption">Broadcast STA</option></select></div><div class="f" id="oscIpRow"><label for="oscIp">IP Address</label><input type="text" id="oscIp" placeholder="192.168.1.100" pattern="^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$"></div><div class="f"><label for="oscPort">Port</label><input type="number" id="oscPort" value="8000" min="1024" max="65535" required></div><div class="f"><label><input type="checkbox" id="oscBroadcast"> Broadcast mode</label><div class="hint">Broadcast to all devices on the network</div></div><button type="submit" class="btn">Save</button><div class="hint" id="oscMsg"></div></form></div>
 <div class="f"><h3>Wi‑Fi Station</h3><form id="sta"><div class="f"><label for="ssid">SSID</label><input type="text" id="ssid" placeholder="Network name" required></div><div class="f"><label for="pass">Password</label><input type="password" id="pass" placeholder="Password"></div><button type="submit" class="btn">Connect</button><div class="hint" id="staMsg"></div></form></div>
 </div>
 <div class="p" id="panel-pins">
 <div class="pl">
 <div class="lp">
 <h3 id="boardName">ESP32‑C3</h3>
 <div class="l"><span class="s digital"></span> Digital <span class="s analog"></span> Analog <span class="s i2c"></span> I2C <span class="s uart"></span> UART <span class="s spi"></span> SPI <span class="s power"></span> Power <span class="s gnd"></span> GND</div>
 <svg class="b" viewBox="50 -20 260 260"><rect x="114" y="20" width="122" height="188" rx="10" fill="#ffffff" stroke="#9ca3af"/><text x="174" y="114" text-anchor="middle" font-size="12" fill="#6b7280">MCU</text><rect x="144" y="2" width="60" height="60" rx="6" fill="#e5e7eb" stroke="#9ca3af"/><g id="pinsLeft"></g><g id="pinsRight"></g></svg>
 <div class="plist"><h4>Configured pins</h4><div id="pinsList" class="list"></div><button id="saveAllBtn" class="btn-p">Save All</button><div id="saveAllMsg" class="hint"></div></div>
 </div>
 <div class="rp"><div class="cp">
 <h4>Pin function</h4>
 <div class="r"><label>Pin:</label><span id="selPin">-</span><select id="funcSelect"></select></div>
 <div id="cardBtn" class="subcard" style="display:none;"><div class="r"><label>Button mode:</label><select id="btnMode"><option value="pulse">Push</option><option value="press_release">Press/Release</option><option value="toggle">Toggle</option></select></div><div class="r" id="btnPulseTimingRow" style="display:none;"><label>Push timing:</label><select id="btnPulseTiming"><option value="press">On press</option><option value="release">On release</option></select></div></div>
 <div id="cardLed" class="subcard" style="display:none;"><div class="r"><label>LED:</label><select id="ledMode"><option value="onoff">On/Off</option><option value="pwm">PWM</option></select></div></div>
 <div id="cardPot" class="subcard" style="display:none;"><div class="r"><label>Filter:</label><select id="potFilter"><option value="none">None</option><option value="lowpass">Low-pass</option><option value="median">Median</option></select></div><div class="r"><label>Filter intensity (1-10):</label><input type="number" id="filterIntensity" min="1" max="10" value="5" style="width:60px;"><span style="margin-left:8px;font-size:0.9em;color:#666;">1=fast, 10=stable</span></div></div>
 <div id="cardMux" class="subcard" style="display:none;">
 <h4 style="margin-top:0;">HC4067 Configuration</h4>
 <div class="f"><label>Multiplexer::</label><select id="muxId"><option value="0">MUX0</option><option value="1">MUX1</option></select></div>
 <div class="f"><label>SIG pin (analog):</label><select id="muxSig"></select></div>
 <div class="f"><label>Selection pins (S0-S3):</label><select id="muxPinGroup"></select></div>
 <div class="f"><label>EN pin (optional):</label><select id="muxEn"><option value="255">Not connected</option></select></div>
 <h4 style="margin-top:20px;">MIDI/OSC Configuration</h4>
 <div class="f"><label>Base CC:</label><input type="number" id="muxCcBase" min="0" max="127" value="1" style="width:100px;"></div>
 <div class="f"><label>MIDI Channel:</label><input type="number" id="muxMidiChan" min="1" max="16" value="1" style="width:100px;"></div>
 <div class="f"><label>OSC base address:</label><input type="text" id="muxOscBase" placeholder="/mux0" style="width:200px;"></div>
 <h4 style="margin-top:20px;">Analog Configuration</h4>
 <div class="f"><label>Min threshold (0-4095):</label><input type="number" id="muxMin" min="0" max="4095" value="0" style="width:100px;"></div>
 <div class="f"><label>Max threshold (0-4095):</label><input type="number" id="muxMax" min="0" max="4095" value="4095" style="width:100px;"></div>
 <div class="hint">OSC: /mux/ID/cal/min [CH] /mux/ID/cal/max [CH] /mux/ID/cal/reset [CH] (CH=0-15 comme valeur)</div>
 <div class="f"><label>OSC Format:</label><select id="muxOscFormat"><option value="float">Float (0-1)</option><option value="raw">Raw (0-4095)</option><option value="midi">MIDI (3 int)</option></select></div>
 <div class="f"><label>Filter intensity (1-10):</label><input type="number" id="muxFilterIntensity" min="1" max="10" value="5" style="width:60px;"><span style="margin-left:8px;font-size:0.9em;color:#666;">1=fast, 10=stable</span></div>
 <button type="button" class="btn" onclick="saveMuxFromPin()">Save</button>
 <div class="hint" id="muxMsg"></div>
 </div>
 <h4>RTP‑MIDI</h4>
 <div class="r switch"><input type="checkbox" id="rtpEnabled2"><label for="rtpEnabled2">Activate</label><label>Type:</label><select id="rtpMsgType"><option>Note</option><option>Control Change</option><option>Program Change</option><option>Pitch Bend</option><option>Aftertouch (Channel)</option><option>Note + Velocity</option><option>Note (Sweep)</option><option>Clock</option><option>Tap Tempo</option></select></div>
 <div id="rtpParams" class="subcard" style="display:none;">
 <div class="r" id="rtpNoteRow" style="display:none;"><label>Note:</label><input type="number" id="rtpNote" min="0" max="127" placeholder="60" style="width:90px;"></div>
 <div class="r" id="rtpCcRow" style="display:none;"><label>CC#:</label><input type="number" id="rtpCc" min="0" max="127" placeholder="7" style="width:90px;"></div>
 <div class="r" id="rtpCcOnOffRow" style="display:none;"><label>Values:</label><span>ON</span><input type="number" id="rtpCcOn" min="0" max="127" placeholder="127" style="width:90px;"><span>OFF</span><input type="number" id="rtpCcOff" min="0" max="127" placeholder="0" style="width:90px;"></div>
 <div class="r" id="rtpPcRow" style="display:none;"><label>Program#:</label><input type="number" id="rtpPc" min="0" max="127" placeholder="0" style="width:90px;"></div>
 <div class="r" id="rtpVelRow" style="display:none;"><label>Velocity:</label><input type="number" id="rtpVel" min="1" max="127" placeholder="100" style="width:90px;"></div>
 <div class="r" id="rtpCcRangeRow" style="display:none;"><label>MIDI Range:</label><input type="number" id="rtpCcMin" min="0" max="127" placeholder="0" style="width:90px;"><span>→</span><input type="number" id="rtpCcMax" min="0" max="127" placeholder="127" style="width:90px;"></div>
 <div class="r" id="rtpChanRow" style="display:none;"><label>Channel:</label><input type="number" id="rtpChan" min="1" max="16" placeholder="1" style="width:90px;"></div>
 <div class="r" id="rtpClockHint" style="display:none; color:#6b7280;"><span>Clock / Tap Tempo: no channel.</span></div>
 <div class="r" id="rtpNoteSweepRow" style="display:none;"><label>Sweep:</label><input type="number" id="rtpNoteMin" min="0" max="127" placeholder="48" style="width:90px;"><span>→</span><input type="number" id="rtpNoteMax" min="0" max="127" placeholder="72" style="width:90px;"><label style="margin-left:8px;">Fixed velocity:</label><input type="number" id="rtpNoteVelFix" min="1" max="127" placeholder="100" style="width:90px;"><label style="margin-left:8px;">Auto-off (ms):</label><input type="number" id="rtpNoteSweepAutoOffDelay" min="0" max="65535" placeholder="0" style="width:90px;"></div>
 </div>
 <h4>OSC</h4>
 <div class="r switch"><input type="checkbox" id="oscEnabled2"><label for="oscEnabled2">Enable</label><label>Addr:</label><input type="text" id="oscAddress" placeholder="/ctl"></div>
 <div class="r"><label>Format:</label><select id="oscFormat"><option value="float">Float (0-1)</option><option value="midi">MIDI (3 int)</option></select></div>
 <h4>Debug</h4>
 <div class="r switch"><input type="checkbox" id="dbgEnabled"><label for="dbgEnabled">Enable</label><label>Header:</label><input type="text" id="dbgHeader" placeholder="[DBG]"></div>
 </div></div>
 </div>
 </div>
 </div>
 </div>
 <div class="modal-overlay" id="muxModalOverlay" onclick="if(event.target.id==='muxModalOverlay')hideMuxForm()">
 <div class="modal" onclick="event.stopPropagation()">
 <h3>HC4067 Configuration</h3>
 <form id="muxForm">
 <div class="f"><label>Multiplexer::</label><select id="muxId"><option value="0">MUX0</option><option value="1">MUX1</option></select></div>
 <div class="f"><label>SIG pin (analog):</label><select id="muxSig"></select></div>
 <div class="f"><label>Selection pins (S0-S3):</label><select id="muxPinGroup"></select></div>
 <div class="f"><label>EN pin (optional):</label><select id="muxEn"><option value="255">Not connected</option></select></div>
 <h4 style="margin-top:20px;">MIDI/OSC Configuration</h4>
 <div class="f"><label>Base CC:</label><input type="number" id="muxCcBase" min="0" max="127" value="1" style="width:100px;"></div>
 <div class="f"><label>MIDI Channel:</label><input type="number" id="muxMidiChan" min="1" max="16" value="1" style="width:100px;"></div>
 <div class="f"><label>OSC base address:</label><input type="text" id="muxOscBase" placeholder="/mux0" style="width:200px;"></div>
 <h4 style="margin-top:20px;">Analog Configuration</h4>
 <div class="f"><label>Min threshold (0-4095):</label><input type="number" id="muxMin" min="0" max="4095" value="0" style="width:100px;"></div>
 <div class="f"><label>Max threshold (0-4095):</label><input type="number" id="muxMax" min="0" max="4095" value="4095" style="width:100px;"></div>
 <div class="hint">OSC: /mux/ID/cal/min [CH] /mux/ID/cal/max [CH] /mux/ID/cal/reset [CH] (CH=0-15 comme valeur)</div>
 <div class="f"><label>OSC Format:</label><select id="muxOscFormat"><option value="float">Float (0-1)</option><option value="raw">Raw (0-4095)</option><option value="midi">MIDI (3 int)</option></select></div>
 <div class="f"><label>Filter intensity (1-10):</label><input type="number" id="muxFilterIntensity" min="1" max="10" value="5" style="width:60px;"><span style="margin-left:8px;font-size:0.9em;color:#666;">1=fast, 10=stable</span></div>
 <button type="submit" class="btn">Save</button>
 <button type="button" class="btn" style="background:#6b7280;" onclick="hideMuxForm()">Cancel</button>
 <div class="hint" id="muxMsg"></div>
 </form>
 </div>
 </div>
</body>
</html>
)rawliteral";
