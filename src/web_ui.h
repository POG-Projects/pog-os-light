#pragma once
#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="fr"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>PogLight</title>
<style>
:root{--bg:#0b0b0e;--panel:#15151b;--line:#272730;--txt:#f0f0f3;--muted:#8a8a96;--accent:#19c37d;--accent2:#0fa968;--mono:ui-monospace,Menlo,Consolas,monospace}
*{box-sizing:border-box}body{margin:0;font-family:var(--mono);background:var(--bg);color:var(--txt);font-size:14px;padding-bottom:30px}
.wrap{max-width:760px;margin:0 auto;padding:18px}
header{display:flex;align-items:center;gap:10px;margin-bottom:16px}
.logo{width:30px;height:30px;border-radius:7px;background:linear-gradient(140deg,#19c37d,#0a7d4f);box-shadow:0 0 16px rgba(25,195,125,.4)}
h1{font-size:18px;margin:0;letter-spacing:.2em;text-transform:uppercase}
.sub{color:var(--muted);font-size:11px;letter-spacing:.12em;text-transform:uppercase}
.chip{margin-left:auto;background:var(--panel);border:1px solid var(--line);border-radius:8px;padding:5px 10px;font-size:11px;color:var(--muted)}
.card{background:var(--panel);border:1px solid var(--line);border-radius:14px;padding:16px;margin-bottom:14px}
.card h2{font-size:11px;letter-spacing:.16em;text-transform:uppercase;margin:0 0 12px;color:var(--accent)}
.strip{display:flex;gap:2px;background:#050506;border:1px solid var(--line);border-radius:10px;padding:8px;margin-bottom:14px}
.strip>i{flex:1 1 0;aspect-ratio:1;max-height:18px;border-radius:50%;background:#16161c}
.grid{display:flex;flex-wrap:wrap;gap:8px}
.pat{flex:1 1 30%;min-width:120px;background:#0c0c11;border:1px solid var(--line);color:var(--txt);font-family:var(--mono);
  border-radius:9px;padding:11px 8px;font-size:13px;cursor:pointer;text-align:left}
.pat.on{border-color:var(--accent);background:rgba(25,195,125,.14);box-shadow:0 0 0 1px var(--accent) inset}
.row{display:flex;align-items:center;gap:12px;margin:10px 0}
.row label{font-size:11px;letter-spacing:.1em;text-transform:uppercase;color:var(--muted);min-width:90px}
input[type=range]{flex:1;accent-color:var(--accent)}
.val{min-width:42px;text-align:right;color:var(--accent)}
select,input[type=text],input[type=password],input[type=number]{background:#0b0b0f;border:1px solid var(--line);border-radius:8px;color:var(--txt);font-family:var(--mono);padding:9px;font-size:13px}
select,input[type=number]{flex:1}
.btn{background:var(--accent);color:#04140c;border:none;border-radius:9px;padding:10px 15px;font-family:var(--mono);font-weight:700;font-size:12px;letter-spacing:.06em;text-transform:uppercase;cursor:pointer}
.btn.ghost{background:transparent;border:1px solid var(--line);color:var(--txt)}
.btn:hover{filter:brightness(1.1)}
.hint{color:var(--muted);font-size:11.5px;margin:0 0 12px;line-height:1.5}
.pbar{background:#0b0b0f;border:1px solid var(--line);border-radius:8px;height:12px;overflow:hidden;margin-top:8px}
.pbar>div{height:100%;width:0;background:var(--accent);transition:width .2s}
.toast{position:fixed;left:50%;bottom:18px;transform:translateX(-50%);background:var(--accent);color:#04140c;padding:9px 16px;border-radius:9px;font-weight:700;opacity:0;transition:.25s;pointer-events:none}
.toast.show{opacity:1}
@media(max-width:520px){.pat{flex:1 1 46%}}
</style></head>
<body><div class="wrap">
  <header><div class="logo"></div><div><h1>PogLight</h1><div class="sub">controleur LED autonome</div></div><span class="chip" id="net">...</span></header>

  <div class="strip" id="preview"></div>

  <div class="card">
    <h2>Scene</h2>
    <div class="grid" id="patterns"></div>
    <div class="row"><label>Couleur A</label><input type="color" id="primaryColor"></div>
    <div class="row"><label>Couleur B</label><input type="color" id="secondaryColor"></div>
    <div class="row"><label>Luminosite</label><input type="range" id="brightness" min="5" max="255"><span class="val" id="brVal"></span></div>
    <div class="row"><label>Vitesse</label><input type="range" id="speed" min="0" max="100"><span class="val" id="spVal"></span></div>
  </div>

  <div class="card">
    <h2>Materiel de la bande</h2>
    <p class="hint">Pour identifier l'ordre des couleurs : lance le test "Ordre couleurs" et change l'ordre ci-dessous jusqu'a ce que Rouge soit rouge.</p>
    <div class="row"><label>Nb LED</label><input type="number" id="numLeds" min="1" max="300"></div>
    <div class="row"><label>Sortie</label><select id="ledPin"></select></div>
    <div class="row"><label>Ordre</label><select id="colorOrder"></select></div>
    <div class="row"><label>Sens</label><select id="reverse"><option value="0">Normal</option><option value="1">Inverse</option></select></div>
    <div class="row"><label>Courant max.</label><input type="number" id="maxMilliAmps" min="100" max="10000" step="100"><span class="val">mA</span></div>
    <div class="row"><label>Mode</label><select id="analog"><option value="0">Adressable (ARGB)</option><option value="1">Analogique (mono PWM)</option></select></div>
    <div class="row"><label>Ecran SDA/SCL</label><select id="oledSwap"><option value="0">Normal (SDA13/SCL11)</option><option value="1">Inverse (SDA11/SCL13)</option></select></div>
    <button class="btn" type="button" onclick="applyHw()">Appliquer (redemarre)</button>
    <span class="hint" style="margin-left:8px">Nb LED / sortie / mode necessitent un redemarrage.</span>
    <div class="hint" id="oledStat" style="margin-top:10px"></div>
  </div>

  <div class="card">
    <h2>Reseau & mise a jour</h2>
    <div class="row"><label>WiFi (SSID)</label><input type="text" id="ssid" placeholder="reseau"><button class="btn ghost" type="button" onclick="scanWifi()">Scan</button></div>
    <div id="wifiList"></div>
    <div class="row"><label>Mot de passe</label><input type="password" id="wifiPass"></div>
    <button class="btn ghost" type="button" onclick="saveWifi()">Connecter & redemarrer</button>
    <h2 style="margin-top:18px">Mise a jour OTA</h2>
    <div class="row"><label>firmware.bin</label><input type="file" id="ota" accept=".bin"></div>
    <button class="btn" type="button" onclick="doOta()">Mettre a jour</button>
    <div id="otaWrap" style="display:none"><div class="pbar"><div id="otaBar"></div></div><div class="hint" id="otaPct"></div></div>
    <div class="row" style="margin-top:14px"><button class="btn ghost" type="button" onclick="reboot()">Redemarrer</button></div>
  </div>
</div>
<div class="toast" id="toast"></div>
<script>
const PATS=["Couleur pleine","Ordre couleurs","Compter (defile)","Remplissage","Arc-en-ciel","Chenillard","Respiration","Feu","Scintillement","Degrade","Balayage","Blanc plein","Eteint"];
const ORDERS=["RGB","RBG","GRB","GBR","BRG","BGR"];
let PINS=[18,16,2,15,17,21,38,47,48];
let st=null;
function $(i){return document.getElementById(i)}
function api(p,o){return fetch(p,o).then(r=>r.json())}
function toast(m){const t=$('toast');t.textContent=m;t.className='toast show';setTimeout(()=>t.className='toast',1800)}

function rgbInt(v){return parseInt(v.slice(1),16)}
function rgbHex(v){return '#'+Number(v||0).toString(16).padStart(6,'0')}
function liveCfg(){return {pattern:cur.pattern,primaryColor:rgbInt($('primaryColor').value),secondaryColor:rgbInt($('secondaryColor').value),brightness:+$('brightness').value,speed:+$('speed').value,colorOrder:+$('colorOrder').value,reverse:$('reverse').value=='1',maxMilliAmps:+$('maxMilliAmps').value}}
let cur={pattern:4};
function pushLive(){api('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(liveCfg())})}

function renderPatterns(){
  $('patterns').innerHTML=PATS.map((n,i)=>`<button class="pat${i==cur.pattern?' on':''}" onclick="selPat(${i})">${n}</button>`).join('');
}
function selPat(i){cur.pattern=i;renderPatterns();pushLive()}

function fill(c){
  cur.pattern=c.pattern;
  $('primaryColor').value=rgbHex(c.primaryColor);
  $('secondaryColor').value=rgbHex(c.secondaryColor);
  $('brightness').value=c.brightness;$('brVal').textContent=c.brightness;
  $('speed').value=c.speed;$('spVal').textContent=c.speed;
  $('numLeds').value=c.numLeds;
  $('ledPin').innerHTML=PINS.map(p=>`<option value="${p}"${p==c.ledPin?' selected':''}>GPIO ${p}</option>`).join('');
  $('colorOrder').innerHTML=ORDERS.map((n,i)=>`<option value="${i}"${i==c.colorOrder?' selected':''}>${n}</option>`).join('');
  $('analog').value=c.analog?'1':'0';
  $('reverse').value=c.reverse?'1':'0';
  $('maxMilliAmps').value=c.maxMilliAmps;
  $('oledSwap').value=c.oledSwap?'1':'0';
  $('ssid').value=c.wifiSsid||'';
  renderPatterns();
}
$('brightness').addEventListener('change',pushLive);
$('brightness').addEventListener('input',()=>$('brVal').textContent=$('brightness').value);
$('speed').addEventListener('change',pushLive);
$('speed').addEventListener('input',()=>$('spVal').textContent=$('speed').value);
$('colorOrder').addEventListener('change',pushLive);
$('primaryColor').addEventListener('input',pushLive);
$('secondaryColor').addEventListener('input',pushLive);
$('reverse').addEventListener('change',pushLive);
$('maxMilliAmps').addEventListener('change',pushLive);

async function applyHw(){
  const cfg={numLeds:+$('numLeds').value,ledPin:+$('ledPin').value,analog:$('analog').value=='1',oledSwap:$('oledSwap').value=='1',reverse:$('reverse').value=='1',maxMilliAmps:+$('maxMilliAmps').value};
  const r=await api('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(cfg)});
  if(r.reboot){toast('Redemarrage...');setTimeout(()=>location.reload(),8000);}else toast('Applique');
}
async function scanWifi(){toast('Scan...');try{const n=await api('/api/scan');$('wifiList').innerHTML=n.sort((a,b)=>b.rssi-a.rssi).slice(0,12).map(x=>`<button class="pat" style="flex:1 1 100%" onclick="$('ssid').value='${(x.ssid||'').replace(/'/g,'')}';$('wifiPass').focus()">${x.ssid} (${x.rssi})</button>`).join('');}catch(e){toast('Scan KO')}}
async function saveWifi(){const s=$('ssid').value.trim();if(!s){toast('SSID ?');return}toast('Redemarrage...');try{await api('/api/wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({wifiSsid:s,wifiPass:$('wifiPass').value})})}catch(e){}}
async function reboot(){if(!confirm('Redemarrer ?'))return;try{await api('/api/reboot',{method:'POST'})}catch(e){}toast('Redemarrage...')}
function doOta(){const f=$('ota').files[0];if(!f){toast('Choisis un .bin');return}const fd=new FormData();fd.append('f',f);const x=new XMLHttpRequest();x.open('POST','/api/ota');$('otaWrap').style.display='block';
  x.upload.onprogress=e=>{if(e.lengthComputable){const p=Math.round(e.loaded/e.total*100);$('otaBar').style.width=p+'%';$('otaPct').textContent='Envoi '+p+'%'}};
  x.onload=()=>{let r={};try{r=JSON.parse(x.responseText)}catch(e){}if(r.ok){$('otaPct').textContent='OK, redemarrage...';setTimeout(()=>location.reload(),8000)}else toast('Echec maj')};
  x.onerror=()=>toast('Echec reseau');x.send(fd)}

async function pollPreview(){try{const h=await (await fetch('/api/leds')).text();const s=$('preview');const N=Math.floor(h.length/6);let o='';for(let i=0;i<N;i++){const c='#'+h.substr(i*6,6);o+= (c!='#000000')?`<i style="background:${c};box-shadow:0 0 7px ${c}"></i>`:'<i></i>'}s.innerHTML=o}catch(e){}}
function netInfo(s){
  $('net').textContent=s.apMode?'AP : 192.168.4.1':(s.ip||'-');
  const o=$('oledStat');
  if(o) o.innerHTML = s.oledFound
    ? ('Ecran OLED : <span style="color:var(--accent)">detecte</span> (0x'+(s.oledAddr||0).toString(16)+(s.oledSwapped?', cablage inverse':'')+')')
    : '<span style="color:#ff5a5a">Ecran OLED : non detecte</span> - reenfonce VCC(3.3V)/GND/SDA(13)/SCL(11)';
}
async function load(){st=await api('/api/state');if(st.board=='esp32c3')PINS=[2,3,4,5,6,7,10];else if(st.board=='esp32')PINS=[2,4,5,12,13,14,16,17,18,19,21,22,23];fill(st.config);netInfo(st)}
load();
setInterval(pollPreview,500);
setInterval(async()=>{try{netInfo(await api('/api/state'))}catch(e){}},5000);
</script></body></html>)HTML";
