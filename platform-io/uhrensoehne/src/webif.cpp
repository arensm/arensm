#include "webif.h"
#include "config.h"
#include "settings.h"
#include "ntp.h"
#include <WiFiManager.h>
#include <vector>
#include <algorithm>

#ifdef ESP32
WebServer webServer(80);
static HTTPUpdateServer _httpUpdater;
#else
ESP8266WebServer webServer(80);
static ESP8266HTTPUpdateServer _httpUpdater;
#endif

static bool _inDimWindow(uint16_t mins) {
  if (!gDimEnabled) return false;
  if (gDimStart <= gDimEnd) return (mins >= gDimStart && mins < gDimEnd);
  return (mins >= gDimStart || mins < gDimEnd);
}
static uint8_t _effectiveBrightness() {
  return _inDimWindow(currentMinutesSinceMidnight()) ? gDimLevel : gBrightness;
}
static int _dispH() { return gPreviewEnabled ? gPreviewHour   : (int)gHour; }
static int _dispM() { return gPreviewEnabled ? gPreviewMinute : (int)gMinute; }

// ============================================================
//  HTML HAUPTSEITE
// ============================================================
static const char HTML_INDEX[] PROGMEM = R"HTML(<!DOCTYPE html><html lang="de"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>__DEVICE_NAME__ – Setup</title>
<style>
  body{font-family:system-ui,Arial,sans-serif;margin:20px auto;max-width:900px}
  .row{margin:12px 0}.val{font-weight:600}.hint{color:#666}
  .card{border:1px solid #ddd;border-radius:12px;padding:16px;box-shadow:2px 2px 8px rgba(0,0,0,.05);margin-bottom:14px}
  .meta{color:#666;text-align:center;margin:-6px 0 14px 0;font-size:.9rem}
  .title{text-align:center;margin-bottom:4px}
  .grid{display:grid;grid-template-columns:1fr;gap:16px}
  @media(min-width:700px){.grid{grid-template-columns:1fr 1fr}}
  button{cursor:pointer;padding:5px 12px;border:1px solid #ccc;border-radius:6px;background:#f5f5f5}
  button:hover{background:#e8e8e8}
  .topbar{text-align:right;margin-bottom:12px}
  .topbar a{color:#555;text-decoration:none;font-size:.95rem;padding:5px 12px;border:1px solid #ccc;border-radius:6px;background:#f5f5f5}
  .topbar a:hover{background:#e8e8e8}
  .overlay{display:none;position:fixed;inset:0;background:rgba(0,0,0,.45);z-index:100;align-items:center;justify-content:center}
  .overlay.show{display:flex}
  .modal{background:#fff;border-radius:12px;padding:24px 28px;box-shadow:0 8px 32px rgba(0,0,0,.2);min-width:260px}
  .modal h3{margin:0 0 14px 0;font-size:1.1rem}
  .modal input{width:100%;padding:7px 10px;border:1px solid #ccc;border-radius:6px;font-size:1rem;box-sizing:border-box;margin-bottom:10px}
  .modal input:focus{outline:none;border-color:#888}
  .modal .err{color:#c00;font-size:.85rem;margin-bottom:8px;min-height:1.1em}
  .modal .btns{display:flex;gap:8px;justify-content:flex-end}
  .sect-title{font-weight:600;font-size:1.05rem;margin-bottom:8px}
  .color-row{display:flex;align-items:center;gap:10px;margin:8px 0}
  .color-row label{min-width:70px}
  .color-row input[type=color]{width:48px;height:32px;border:1px solid #ccc;border-radius:6px;padding:2px;cursor:pointer}
  .color-prev{width:32px;height:32px;border-radius:6px;border:1px solid #ccc}
</style></head><body>
<div class="overlay" id="pwOverlay">
  <div class="modal">
    <h3>&#x1F527; Debug &ndash; Passwort</h3>
    <input type="password" id="pwInput" placeholder="Passwort"
      onkeydown="if(event.key==='Enter')checkPw();if(event.key==='Escape')closePw();">
    <div class="err" id="pwErr"></div>
    <div class="btns">
      <button onclick="closePw()">Abbrechen</button>
      <button onclick="checkPw()" style="background:#333;color:#fff;border-color:#333">OK</button>
    </div>
  </div>
</div>
<div class="topbar"><a href="#" onclick="openPw();return false;">&#x1F527; Debug</a></div>
<h1 id="devName" class="title">__DEVICE_NAME__</h1>
<div class="meta">
  Firmware: <span id="fw"></span> &bull; Build: <span id="build"></span><br>
  Uptime: <span id="uptime"></span> &bull; IP: <span id="ip"></span> &bull; RSSI: <span id="rssi"></span>
</div>

<div class="card grid">
  <div>
    <div class="row">Zeit: <span class="val" id="timeDisp">--:--</span></div>
    <div class="row" style="font-size:1.1rem;font-weight:600" id="wordsDisp"></div>
    <div style="color:#888;font-size:.8rem;margin:-6px 0 6px 0">(Satz wird jede Minute aktualisiert)</div>
    <div class="row">
      Helligkeit:
      <input type="range" id="br" min="__BMIN__" max="__BMAX__" step="1">
      <span class="val" id="brv"></span>
      <span class="hint" id="effv"></span>
    </div>
  </div>
  <div>
    <div class="val">Dimmen (Zeitfenster)</div>
    <div class="row"><label><input type="checkbox" id="dimEn"> Dimmen aktiv</label></div>
    <div class="row">
      Von: <input type="time" id="tStart" step="60">
      &nbsp;Bis: <input type="time" id="tEnd" step="60">
    </div>
    <div class="row">
      Dim-Helligkeit:
      <input type="range" id="dimBr" min="1" max="255" step="1">
      <span class="val" id="dimBrv"></span>
    </div>
    <div class="row"><button id="saveDim">Speichern</button></div>
  </div>
</div>

<div class="card" id="colorCard" style="display:none">
  <div class="sect-title">&#x1F3A8; Farben</div>
  <div class="color-row">
    <label>Wortfarbe</label>
    <input type="color" id="cTime" value="#ffffff">
    <div class="color-prev" id="prevTime"></div>
  </div>
  <div class="color-row" id="frameColorRow" style="display:none">
    <label>Rahmenfarbe</label>
    <input type="color" id="cFrame" value="#002805">
    <div class="color-prev" id="prevFrame"></div>
  </div>
  <div class="row">
    <button id="saveColor">Speichern &amp; anzeigen</button>
    <span class="hint" id="colorHint" style="margin-left:10px"></span>
  </div>
</div>

<div class="card" id="dotsCard" style="display:none">
  <div class="sect-title">&#x22EF; Doppelpunkt</div>
  <div class="row"><label><input type="checkbox" id="dotsEn"> Sekundentakt (blinkt jede Sekunde)</label></div>
  <div class="row"><button id="saveDots">Speichern</button> <span class="hint" id="dotsHint" style="margin-left:10px"></span></div>
</div>

<div class="card" id="h12Card" style="display:none">
  <div class="sect-title">&#x1F552; Zeitformat</div>
  <div class="row"><label><input type="checkbox" id="h12En"> 12h-Modus &ndash; AM/PM aktiv (sonst 24h, AM/PM aus)</label></div>
  <div class="row"><button id="saveH12">Speichern</button> <span class="hint" id="h12Hint" style="margin-left:10px"></span></div>
</div>

<div class="card" id="bttf3Card" style="display:none">
  <div class="sect-title">&#x23F1; BTTF-3 Zeiten</div>
  <div style="margin-bottom:10px">
    <div style="font-size:.85rem;font-weight:bold;color:#555;margin-bottom:6px">&#x1F534; Destination Time</div>
    <div style="display:flex;gap:6px;align-items:center;flex-wrap:wrap">
      <input type="number" id="destMon"  min="1" max="12" placeholder="MM"   style="width:52px;padding:5px 8px;border:1px solid #ccc;border-radius:6px">
      <span>.</span>
      <input type="number" id="destDay"  min="1" max="31" placeholder="DD"   style="width:52px;padding:5px 8px;border:1px solid #ccc;border-radius:6px">
      <span>.</span>
      <input type="number" id="destYear" min="1" max="9999" placeholder="YYYY" style="width:72px;padding:5px 8px;border:1px solid #ccc;border-radius:6px">
      &nbsp;
      <input type="time"   id="destTime" step="60" style="padding:5px 8px;border:1px solid #ccc;border-radius:6px">
    </div>
  </div>
  <div style="margin-bottom:10px">
    <div style="font-size:.85rem;font-weight:bold;color:#555;margin-bottom:6px">&#x1F7E1; Last Time Departed</div>
    <div style="display:flex;gap:6px;align-items:center;flex-wrap:wrap">
      <input type="number" id="lastMon"  min="1" max="12" placeholder="MM"   style="width:52px;padding:5px 8px;border:1px solid #ccc;border-radius:6px">
      <span>.</span>
      <input type="number" id="lastDay"  min="1" max="31" placeholder="DD"   style="width:52px;padding:5px 8px;border:1px solid #ccc;border-radius:6px">
      <span>.</span>
      <input type="number" id="lastYear" min="1" max="9999" placeholder="YYYY" style="width:72px;padding:5px 8px;border:1px solid #ccc;border-radius:6px">
      &nbsp;
      <input type="time"   id="lastTime" step="60" style="padding:5px 8px;border:1px solid #ccc;border-radius:6px">
    </div>
  </div>
  <div class="row"><button id="saveBttf3Times">Speichern</button> <span class="hint" id="bttf3TimesHint" style="margin-left:10px"></span></div>
</div>

<script>
function toHex(r,g,b){return '#'+[r,g,b].map(x=>x.toString(16).padStart(2,'0')).join('');}
async function poll(){
  try{
    let r=await fetch('/state'); if(!r.ok) return; let j=await r.json();
    document.getElementById('devName').textContent=j.name||'__DEVICE_NAME__';
    document.getElementById('timeDisp').textContent=j.time;
    if(j.words) document.getElementById('wordsDisp').textContent=j.words;
    document.getElementById('ip').textContent=j.ip;
    document.getElementById('rssi').textContent=j.rssi+'dBm';
    document.getElementById('fw').textContent=j.fw;
    document.getElementById('build').textContent=j.build;
    document.getElementById('uptime').textContent=j.uptime;
    let br=document.getElementById('br');
    if(!br.dataset.init){br.value=j.brightness;br.dataset.init=1;}
    document.getElementById('brv').textContent=' '+br.value;
    document.getElementById('effv').textContent=' (effektiv: '+j.effective+')';
    let de=document.getElementById('dimEn'); de.checked=!!j.dimEnabled;
    let ts=document.getElementById('tStart'); if(!ts.dataset.init){ts.value=j.dimStart;ts.dataset.init=1;}
    let te=document.getElementById('tEnd');   if(!te.dataset.init){te.value=j.dimEnd;  te.dataset.init=1;}
    let db=document.getElementById('dimBr');  if(!db.dataset.init){db.value=j.dimBrightness;db.dataset.init=1;}
    document.getElementById('dimBrv').textContent=' '+db.value;
    if(j.hasColors){
      document.getElementById('colorCard').style.display='';
      if(j.hasFrame) document.getElementById('frameColorRow').style.display='';
      let ct=document.getElementById('cTime');
      let cf=document.getElementById('cFrame');
      if(!ct.dataset.init){ct.value=toHex(j.cTimeR,j.cTimeG,j.cTimeB);ct.dataset.init=1;}
      if(!cf.dataset.init){cf.value=toHex(j.cFrameR,j.cFrameG,j.cFrameB);cf.dataset.init=1;}
      document.getElementById('prevTime').style.background=ct.value;
      if(j.hasFrame) document.getElementById('prevFrame').style.background=cf.value;
    }
    if(j.hasDots){
      document.getElementById('dotsCard').style.display='';
      let de=document.getElementById('dotsEn');
      if(!de.dataset.init){de.checked=!!j.dotsEnabled;de.dataset.init=1;}
    }
    if(j.has12h){
      document.getElementById('h12Card').style.display='';
      let h=document.getElementById('h12En');
      if(!h.dataset.init){h.checked=!!j.mode12h;h.dataset.init=1;}
    }
    if(j.hasBttf3Times){
      document.getElementById('bttf3Card').style.display='';
      const b=document.getElementById('destMon');
      if(!b.dataset.init){
        document.getElementById('destMon').value=j.destMon||'';
        document.getElementById('destDay').value=j.destDay||'';
        document.getElementById('destYear').value=j.destYear||'';
        const dh=String(j.destHour||0).padStart(2,'0');
        const dm=String(j.destMin||0).padStart(2,'0');
        document.getElementById('destTime').value=dh+':'+dm;
        document.getElementById('lastMon').value=j.lastMon||'';
        document.getElementById('lastDay').value=j.lastDay||'';
        document.getElementById('lastYear').value=j.lastYear||'';
        const lh=String(j.lastHour||0).padStart(2,'0');
        const lm=String(j.lastMin||0).padStart(2,'0');
        document.getElementById('lastTime').value=lh+':'+lm;
        b.dataset.init=1;
      }
    }
  }catch(e){}
  setTimeout(poll,30000);
}
poll();
document.getElementById('br').addEventListener('input',async(e)=>{
  document.getElementById('brv').textContent=' '+e.target.value;
  await fetch('/brightness',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'v='+e.target.value});
});
document.getElementById('dimBr').addEventListener('input',(e)=>{
  document.getElementById('dimBrv').textContent=' '+e.target.value;
});
document.getElementById('saveDim').addEventListener('click',async()=>{
  let en=document.getElementById('dimEn').checked?1:0;
  let s=document.getElementById('tStart').value;
  let e=document.getElementById('tEnd').value;
  let lv=document.getElementById('dimBr').value;
  await fetch('/dim',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'en='+en+'&start='+encodeURIComponent(s)+'&end='+encodeURIComponent(e)+'&lvl='+lv});
});
document.getElementById('cTime').addEventListener('input',(e)=>{
  document.getElementById('prevTime').style.background=e.target.value;
});
document.getElementById('cFrame').addEventListener('input',(e)=>{
  document.getElementById('prevFrame').style.background=e.target.value;
});
document.getElementById('saveColor').addEventListener('click',async()=>{
  let ct=document.getElementById('cTime').value;
  let cf=document.getElementById('cFrame').value;
  let parse=h=>[parseInt(h.slice(1,3),16),parseInt(h.slice(3,5),16),parseInt(h.slice(5,7),16)];
  let [tr,tg,tb]=parse(ct); let [fr,fg,fb]=parse(cf);
  let r=await fetch('/color',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'tr='+tr+'&tg='+tg+'&tb='+tb+'&fr='+fr+'&fg='+fg+'&fb='+fb});
  document.getElementById('colorHint').textContent=await r.text();
  setTimeout(()=>document.getElementById('colorHint').textContent='',3000);
});
document.getElementById('saveDots').addEventListener('click',async()=>{
  let en=document.getElementById('dotsEn').checked?1:0;
  let r=await fetch('/dots',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'en='+en});
  document.getElementById('dotsHint').textContent=await r.text();
  setTimeout(()=>document.getElementById('dotsHint').textContent='',3000);
});
document.getElementById('saveH12').addEventListener('click',async()=>{
  let en=document.getElementById('h12En').checked?1:0;
  let r=await fetch('/12h',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'en='+en});
  document.getElementById('h12Hint').textContent=await r.text();
  setTimeout(()=>document.getElementById('h12Hint').textContent='',3000);
});
document.getElementById('saveBttf3Times').addEventListener('click',async()=>{
  const dt=document.getElementById('destTime').value||'00:00';
  const lt=document.getElementById('lastTime').value||'00:00';
  const [dh,dm]=dt.split(':'); const [lh,lm]=lt.split(':');
  const body='destMon='+document.getElementById('destMon').value
    +'&destDay='+document.getElementById('destDay').value
    +'&destYear='+document.getElementById('destYear').value
    +'&destHour='+dh+'&destMin='+dm
    +'&lastMon='+document.getElementById('lastMon').value
    +'&lastDay='+document.getElementById('lastDay').value
    +'&lastYear='+document.getElementById('lastYear').value
    +'&lastHour='+lh+'&lastMin='+lm;
  const r=await fetch('/bttf3times',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
  document.getElementById('bttf3TimesHint').textContent=await r.text();
  document.getElementById('destMon').dataset.init='';
  setTimeout(()=>document.getElementById('bttf3TimesHint').textContent='',3000);
});
function openPw(){document.getElementById('pwOverlay').classList.add('show');setTimeout(()=>document.getElementById('pwInput').focus(),50);}
function closePw(){document.getElementById('pwOverlay').classList.remove('show');document.getElementById('pwInput').value='';document.getElementById('pwErr').textContent='';}
function checkPw(){
  if(document.getElementById('pwInput').value==='__DEBUG_PW__'){location='/debug';}
  else{document.getElementById('pwErr').textContent='Falsches Passwort';document.getElementById('pwInput').value='';document.getElementById('pwInput').focus();}
}
</script>
</body></html>)HTML";

// ============================================================
//  HTML DEBUG-SEITE
// ============================================================
static const char HTML_DEBUG[] PROGMEM = R"HTML(<!DOCTYPE html><html lang="de"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>__DEVICE_NAME__ – Debug</title>
<style>
  body{font-family:system-ui,Arial,sans-serif;margin:20px auto;max-width:900px}
  .row{margin:12px 0}.hint{color:#666;font-size:.9rem}
  .card{border:1px solid #ddd;border-radius:12px;padding:16px;box-shadow:2px 2px 8px rgba(0,0,0,.05);margin-bottom:14px}
  .meta{color:#666;text-align:center;margin:-6px 0 14px 0;font-size:.9rem}
  .title{text-align:center;margin-bottom:4px}
  .topbar{display:flex;justify-content:flex-end;margin-bottom:12px}
  .topbar a{color:#555;text-decoration:none;font-size:.95rem;padding:5px 12px;border:1px solid #ccc;border-radius:6px;background:#f5f5f5}
  .topbar a:hover{background:#e8e8e8}
  button{cursor:pointer;padding:5px 12px;border:1px solid #ccc;border-radius:6px;background:#f5f5f5}
  button:hover{background:#e8e8e8}
  .btn-danger{border-color:#c00;color:#c00}
  .btn-danger:hover{background:#fff0f0}
  .sect-title{font-weight:600;font-size:1.05rem;margin-bottom:8px}
  hr.sep{border:none;border-top:1px solid #eee;margin:10px 0}
  .ledgrid{display:grid;grid-template-columns:repeat(11,30px);gap:3px;margin-top:8px}
  .ledgrid .dot{width:30px;height:30px;border-radius:50%;background:#1a1a1a;border:1px solid #444;
    display:flex;align-items:center;justify-content:center;font-size:9px;color:#666;
    cursor:pointer;user-select:none;font-weight:600}
  .ledgrid .dot:hover{border-color:#aaa;color:#aaa}
  .ledgrid .dot.on{background:var(--col-time,#e8a020);color:#000;border-color:var(--col-time,#e8a020)}
  .dotrow{display:flex;gap:8px;margin-top:8px}
  .dotrow .dot{width:30px;height:30px;border-radius:50%;background:#1a1a1a;border:1px solid #444;
    display:flex;align-items:center;justify-content:center;font-size:9px;color:#666;
    cursor:pointer;user-select:none;font-weight:600}
  .dotrow .dot:hover{border-color:#aaa;color:#aaa}
  .dotrow .dot.on{background:var(--col-time,#e8a020);color:#000;border-color:var(--col-time,#e8a020)}
  .fwcdot{min-width:52px;height:32px;border-radius:8px;background:#1a1a1a;border:1px solid #444;
    display:flex;align-items:center;justify-content:center;font-size:9px;color:#666;
    cursor:pointer;user-select:none;font-weight:600;padding:0 6px;box-sizing:border-box;}
  .fwcdot:hover{border-color:#aaa;color:#aaa}
  .fwcdot.on{background:var(--col-time,#e8a020);color:#000;border-color:var(--col-time,#e8a020)}
  .fwcdot.frame.on{background:var(--col-frame,#004010);color:#fff;border-color:var(--col-frame,#004010)}
  .fwc-group{display:flex;gap:5px;flex-wrap:wrap;margin:3px 0}
  .obdot{width:18px;height:18px;border-radius:3px;background:#1a1a1a;border:1px solid #333;
    cursor:pointer;box-sizing:border-box;}
  .obdot:hover{border-color:#888}
  .obdot.on{background:var(--col-time,#e8e0c0);border-color:var(--col-time,#e8e0c0)}
  /* 7-Segment */
  .seg-digit{display:grid;grid-template-columns:10px 10px 10px;grid-template-rows:10px 10px 10px 10px 10px;gap:2px;position:relative}
  .seg{border-radius:3px;background:#1a1a1a;border:1px solid #2a2a2a;cursor:pointer;box-sizing:border-box}
  .seg:hover{border-color:#888}
  .seg.on{background:var(--col-time,#e8c84a);border-color:var(--col-time,#e8c84a)}
  .seg-h{width:26px;height:10px;grid-column:span 3}  /* horizontal: oben/mitte/unten */
  .seg-v{width:10px;height:26px;grid-row:span 2}     /* vertikal: links/rechts oben+unten */
  .seg-colon{display:flex;flex-direction:column;justify-content:center;gap:16px;padding:4px 0}
  .seg-dot{width:10px;height:10px;border-radius:50%;background:#1a1a1a;border:1px solid #2a2a2a;cursor:pointer}
  .seg-dot:hover{border-color:#888}
  .seg-dot.on{background:var(--col-time,#e8c84a);border-color:var(--col-time,#e8c84a)}
  .seg-label{font-size:9px;color:#555;text-align:center;margin-top:4px}
  .ntp-info{font-size:.9rem;color:#444;margin:6px 0}
</style></head><body>
<div class="topbar"><a href="/">&#x2699;&#xFE0F; Setup</a></div>
<h1 id="devName" class="title">&#x1F527; __DEVICE_NAME__ – Debug</h1>
<div class="meta">
  Firmware: <span id="fw">...</span> &bull; Build: <span id="build">...</span><br>
  Uptime: <span id="uptime">...</span> &bull; IP: <span id="ip">...</span> &bull; RSSI: <span id="rssi">...</span>
</div>

<div class="card">
  <div class="sect-title">&#x1F6E0; System</div>

  <div class="row">
    <button class="btn-danger" onclick="if(confirm('WLAN-Daten loeschen und neu starten?')) location='/reset'">WLAN zur&uuml;cksetzen</button>
    <button style="margin-left:10px" onclick="toggleWlanForm()">&#x1F4F6; Neues WLAN konfigurieren</button>
    <a href="/update" style="margin-left:10px;padding:5px 12px;border:1px solid #ccc;border-radius:6px;background:#f5f5f5;color:#333;text-decoration:none">&#x1F4E5; Firmware-Update</a>
  </div>

  <div id="wlanForm" style="display:none;margin-top:12px;border-top:1px solid #eee;padding-top:12px">
    <div id="wlanScanStatus" style="color:#666;font-size:.9rem;margin-bottom:8px">Scanne Netzwerke...</div>
    <select id="wlanSSID" style="width:100%;padding:7px 10px;border:1px solid #ccc;border-radius:6px;font-size:1rem;box-sizing:border-box;margin-bottom:8px;display:none">
      <option value="">– Netzwerk w&auml;hlen –</option>
    </select>
    <input type="text" id="wlanManual" placeholder="Oder SSID manuell eingeben" style="width:100%;padding:7px 10px;border:1px solid #ccc;border-radius:6px;font-size:.95rem;box-sizing:border-box;margin-bottom:8px">
    <input type="password" id="wlanPw" placeholder="Passwort" style="width:100%;padding:7px 10px;border:1px solid #ccc;border-radius:6px;font-size:.95rem;box-sizing:border-box;margin-bottom:8px">
    <div id="wlanErr" style="color:#c00;font-size:.85rem;min-height:1.1em;margin-bottom:6px"></div>
    <div>
      <button id="wlanConnBtn" onclick="doWlanConnect()" style="background:#333;color:#fff;border-color:#333">Verbinden &amp; Neustart</button>
      <button onclick="toggleWlanForm()" style="margin-left:8px">Abbrechen</button>
    </div>
  </div>

  <hr style="border:none;border-top:1px solid #eee;margin:12px 0">
  <div style="display:flex;align-items:center;gap:8px;flex-wrap:wrap">
    <span style="font-size:.9rem;color:#555">&#x1F512; Debug-Passwort:</span>
    <input type="password" id="newPw1" placeholder="Neues Passwort" style="padding:5px 10px;border:1px solid #ccc;border-radius:6px;font-size:.95rem;width:150px">
    <input type="password" id="newPw2" placeholder="Wiederholen" style="padding:5px 10px;border:1px solid #ccc;border-radius:6px;font-size:.95rem;width:150px">
    <button onclick="changeDebugPw()">Speichern</button>
    <span id="pwChangeMsg" style="font-size:.85rem"></span>
  </div>
</div>

<div class="card">
  <div class="sect-title">&#x1F552; NTP &amp; Zeit</div>
  <div class="ntp-info">
    Aktuelle Zeit: <strong id="ntpTime">...</strong>
    &bull; <span id="ntpWords" style="font-style:italic;color:#444"></span>
  </div>
  <div class="ntp-info">Letzter Sync: <span id="ntpLastSync">...</span></div>
  <div class="row">
    <button id="btnNtpSync">NTP jetzt synchronisieren</button>
    <span class="hint" id="ntpStatus" style="margin-left:10px"></span>
  </div>
  <hr class="sep">
  <div class="row" id="prevRowTime">
    Testzeit: <input type="time" id="forceTime" step="60">
    <button id="applyPrev">Anzeigen</button>
    <button id="clearPrev" style="margin-left:6px">Zur&uuml;ck zur Echtzeit</button>
    <span class="hint" id="prv"></span>
  </div>
  <div class="row" id="prevRowDateTime" style="display:none;flex-wrap:wrap;gap:6px;align-items:center">
    Testzeit:
    <input type="number" id="forceMon" min="1" max="12" placeholder="MM" style="width:52px;padding:4px 6px;border:1px solid #ccc;border-radius:6px">
    <span>.</span>
    <input type="number" id="forceDay" min="1" max="31" placeholder="DD" style="width:52px;padding:4px 6px;border:1px solid #ccc;border-radius:6px">
    <span>.</span>
    <input type="number" id="forceYear" min="1985" max="2999" placeholder="YYYY" style="width:72px;padding:4px 6px;border:1px solid #ccc;border-radius:6px">
    &nbsp;
    <input type="time" id="forceTime2" step="60" style="padding:4px 6px;border:1px solid #ccc;border-radius:6px">
    <button id="applyPrev2">Anzeigen</button>
    <button id="clearPrev2" style="margin-left:4px">Zur&uuml;ck zur Echtzeit</button>
    <span class="hint" id="prv2"></span>
  </div>
</div>

<div class="card">
  <div class="sect-title">&#x1F4A1; LED-Debug</div>
  <div class="row">
    <button id="btnAllOn">Alle LEDs AN</button>
    <button id="btnAllOff" style="margin-left:8px">Alle LEDs AUS</button>
    <span class="hint" style="margin-left:10px" id="dbgStatus"></span>
  </div>
  <div id="gridSection" style="display:none">
    <hr class="sep">
    <div class="hint">Einzelne LED anklicken (leuchtet wei&szlig;, alle anderen aus):</div>
    <div class="ledgrid" id="ledGrid"></div>
    <div class="hint" style="margin-top:10px">Minutenpunkte (110&ndash;113):</div>
    <div class="dotrow" id="dotRow"></div>
  </div>
  <div id="fwcSection" style="display:none">
    <hr class="sep">
    <div class="hint">Einzelne LED anklicken (leuchtet wei&szlig;, alle anderen aus):</div>
    <div id="fwcGrid" style="display:flex;flex-wrap:wrap;gap:5px;margin-top:8px;max-width:520px"></div>
  </div>
  <div id="obSection" style="display:none">
    <hr class="sep">
    <div class="hint">16&times;16 Matrix (physikalischer Index, Klick = LED einzeln an):</div>
    <div id="obGrid" style="display:inline-grid;grid-template-columns:repeat(16,18px);gap:2px;margin-top:8px"></div>
  </div>
  <div id="seg7Section" style="display:none">
    <hr class="sep">
    <div class="hint">7-Segment-Anzeige (Klick = LED einzeln an):</div>
    <div id="seg7Grid" style="margin-top:10px;display:flex;align-items:center;gap:12px"></div>
  </div>
  <div id="bttf1Section" style="display:none">
    <hr class="sep">
    <div class="hint">BTTF-1 Displays &ndash; Aktuelle Anzeige:</div>
    <div id="bttf1Grid" style="margin-top:10px;display:flex;gap:18px;align-items:flex-start;flex-wrap:wrap"></div>
  </div>
</div>

<script>
const ROWS=10,COLS=11;
const matrix=["ESMISTAFÜNF","ZEHNZWANZIG","DREIVIERTEL","ANNACHMVORA","HALBMZWÖLFA","ZWEINSIEBEN","DREIARIFÜNF","ELFNEUNVIER","MACHTAZEHNA","SECHSARIUHR"];
function ledIdx(r,c){return(r%2===0)?r*COLS+c:r*COLS+(COLS-1-c);}
const grid=document.getElementById('ledGrid');
const dotRow=document.getElementById('dotRow');
const dbgStatus=document.getElementById('dbgStatus');
for(let r=0;r<ROWS;r++){
  for(let c=0;c<COLS;c++){
    const idx=ledIdx(r,c);
    const d=document.createElement('div');
    d.className='dot';d.title='LED '+idx+' (R'+r+'/C'+c+')';
    d.textContent=matrix[r][c];d.dataset.idx=idx;
    d.addEventListener('click',()=>litOne(idx,d));
    grid.appendChild(d);
  }
}
for(let i=0;i<4;i++){
  const d=document.createElement('div');
  d.className='dot';d.title='Dot '+i+' (LED '+(110+i)+')';
  d.textContent=i;d.dataset.idx=110+i;
  d.addEventListener('click',()=>litOne(110+i,d));
  dotRow.appendChild(d);
}
async function litOne(idx,el){
  document.querySelectorAll('.dot.on,.fwcdot.on,.obdot.on').forEach(d=>d.classList.remove('on'));
  el.classList.add('on');
  const r=await fetch('/ledon?i='+idx);
  dbgStatus.textContent=await r.text();
}
function updateGrid(activeArr){
  document.querySelectorAll('.dot,.fwcdot,.obdot,.seg,.seg-dot').forEach(d=>d.classList.remove('on'));
  if(!activeArr||!activeArr.length)return;
  const activeSet=new Set(activeArr);
  document.querySelectorAll('#ledGrid .dot,#dotRow .dot,#obGrid .obdot,#fwcGrid .fwcdot,#seg7Grid .seg,#seg7Grid .seg-dot').forEach(d=>{
    if(activeSet.has(Number(d.dataset.idx)))d.classList.add('on');
  });
}
document.getElementById('btnAllOn').addEventListener('click',async()=>{
  document.querySelectorAll('.dot,.fwcdot,.obdot').forEach(d=>d.classList.add('on'));
  const r=await fetch('/allleds',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'on=1'});
  dbgStatus.textContent=await r.text();
});
document.getElementById('btnAllOff').addEventListener('click',async()=>{
  document.querySelectorAll('.dot.on,.fwcdot.on,.obdot.on').forEach(d=>d.classList.remove('on'));
  const r=await fetch('/allleds',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'on=0'});
  dbgStatus.textContent=await r.text();
});
(async()=>{
  try{
    let r=await fetch('/state');
    if(!r.ok){
      document.getElementById('fw').textContent='HTTP '+r.status;
      document.getElementById('build').textContent='state-Fehler';
      return;
    }
    let j=await r.json();
    document.getElementById('devName').textContent='🔧 '+(j.name||'__DEVICE_NAME__')+' – Debug';
    document.getElementById('fw').textContent=j.fw;
    document.getElementById('build').textContent=j.build;
    document.getElementById('uptime').textContent=j.uptime;
    document.getElementById('ip').textContent=j.ip;
    document.getElementById('rssi').textContent=j.rssi+'dBm';
    document.getElementById('ntpTime').textContent=j.time;
    if(j.words) document.getElementById('ntpWords').textContent=j.words;
    document.getElementById('ntpLastSync').textContent=j.ntpLastSync||'unbekannt';
    // Testzeit-Zeile: BTTF1 zeigt Datum+Zeit, andere nur Zeit
    if(j.bttf1Grid){
      document.getElementById('prevRowTime').style.display='none';
      document.getElementById('prevRowDateTime').style.display='';
      if(j.preview && j.previewFull){
        const pf=j.previewFull; // "MM DD YYYY HH:MM"
        const parts=pf.split(' ');
        if(parts.length>=4){
          document.getElementById('forceMon').value=parts[0];
          document.getElementById('forceDay').value=parts[1];
          document.getElementById('forceYear').value=parts[2];
          document.getElementById('forceTime2').value=parts[3];
        }
        document.getElementById('prv2').textContent=' (aktiv: '+pf+')';
      }
    } else {
      document.getElementById('forceTime').value=j.preview?(j.previewTime||''):'';
      if(j.preview)document.getElementById('prv').textContent=' (aktiv: '+(j.previewTime||'')+')';
    }
    if(j.hasColors){
      const h2=(r,g,b)=>'#'+[r,g,b].map(x=>x.toString(16).padStart(2,'0')).join('');
      document.documentElement.style.setProperty('--col-time', h2(j.cTimeR,j.cTimeG,j.cTimeB));
      if(j.hasFrame) document.documentElement.style.setProperty('--col-frame', h2(j.cFrameR,j.cFrameG,j.cFrameB));
    }
    if(j.ledGrid){document.getElementById('gridSection').style.display='';updateGrid(j.activeLeds||[]);}
    if(j.fwcGrid){
      const fwcLabels={
        1:'bungard',2:'ihr',3:'weg',
        4:'ein',
        5:'viertel①',6:'viertel②',7:'zehn',8:'fünf',9:'gleich①',10:'gleich②',
        11:'zwanzig①',12:'zwanzig②',13:'kurz',14:'nach',15:'vor',
        16:'vier',17:'drei',18:'zwei',19:'eins',20:'halb',
        21:'fünf',22:'sechs①',23:'sechs②',24:'sieben①',25:'sieben②',26:'acht',
        27:'zwölf②',28:'zwölf①',29:'elf',30:'zehn',31:'neun',
        32:'uhr',33:'mittags①',34:'mittags②',35:'nachts①',36:'nachts②',
        37:'platte③',38:'ter②',39:'lei①',40:'zur'
      };
      const fwcRows=[
        {label:'Z1 – Frame oben / ein',                  ids:[1,2,3,4]},
        {label:'Z2 – gleich / fünf / zehn / viertel',    ids:[10,9,8,7,6,5]},
        {label:'Z3 – zwanzig / kurz / nach / vor',       ids:[11,12,13,14,15]},
        {label:'Z4 – halb / eins / zwei / drei / vier',  ids:[20,19,18,17,16]},
        {label:'Z5 – fünf / sechs / sieben / acht',      ids:[21,22,23,24,25,26]},
        {label:'Z6 – neun / zehn / elf / zwölf',         ids:[31,30,29,28,27]},
        {label:'Z7 – uhr / mittags / nachts',            ids:[32,33,34,35,36]},
        {label:'Z8 – zur / leiterplatte (Frame unten)',  ids:[40,39,38,37]}
      ];
      const container=document.getElementById('fwcGrid');
      fwcRows.forEach(row=>{
        const rowWrap=document.createElement('div');
        rowWrap.style.cssText='width:100%;margin-bottom:2px';
        const lbl=document.createElement('div');
        lbl.style.cssText='font-size:9px;color:#555;margin-bottom:2px;text-transform:uppercase;letter-spacing:1px';
        lbl.textContent=row.label;
        rowWrap.appendChild(lbl);
        const rowDiv=document.createElement('div');
        rowDiv.className='fwc-group';
        row.ids.forEach(id=>{
          const d=document.createElement('div');
          const isFrame=(id>=1&&id<=3)||(id>=37&&id<=40);
          d.className='fwcdot'+(isFrame?' frame':'');
          d.title='LED '+id;
          d.dataset.idx=id;
          d.textContent=fwcLabels[id]||('#'+id);
          d.addEventListener('click',()=>litOne(id,d));
          rowDiv.appendChild(d);
        });
        rowWrap.appendChild(rowDiv);
        container.appendChild(rowWrap);
      });
      document.getElementById('fwcSection').style.display='';
      updateGrid(j.activeLeds||[]);
    }
    if(j.obGrid){
      // LUT: [Zeile][Spalte] → physikalischer Index (identisch mit C++ lut[][])
      const lut=[
        [23,22,21,20,19,18,17,16,7,6,5,4,3,2,1,0],
        [24,25,26,27,28,29,30,31,8,9,10,11,12,13,14,15],
        [39,38,37,36,35,34,33,32,55,54,53,52,51,50,49,48],
        [40,41,42,43,44,45,46,47,56,57,58,59,60,61,62,63],
        [87,86,85,84,83,82,81,80,71,70,69,68,67,66,65,64],
        [88,89,90,91,92,93,94,95,72,73,74,75,76,77,78,79],
        [103,102,101,100,99,98,97,96,119,118,117,116,115,114,113,112],
        [104,105,106,107,108,109,110,111,120,121,122,123,124,125,126,127],
        [151,150,149,148,147,146,145,144,135,134,133,132,131,130,129,128],
        [152,153,154,155,156,157,158,159,136,137,138,139,140,141,142,143],
        [167,166,165,164,163,162,161,160,183,182,181,180,179,178,177,176],
        [168,169,170,171,172,173,174,175,184,185,186,187,188,189,190,191],
        [215,214,213,212,211,210,209,208,199,198,197,196,195,194,193,192],
        [216,217,218,219,220,221,222,223,200,201,202,203,204,205,206,207],
        [231,230,229,228,227,226,225,224,247,246,245,244,243,242,241,240],
        [232,233,234,235,236,237,238,239,248,249,250,251,252,253,254,255]
      ];
      const og=document.getElementById('obGrid');
      for(let y=0;y<16;y++){
        for(let x=0;x<16;x++){
          const physIdx=lut[y][x];
          const d=document.createElement('div');
          d.className='obdot';
          d.title='LED '+physIdx+' ('+y+'/'+x+')';
          d.dataset.idx=physIdx;  // physikalischer Index für /ledon und updateGrid
          d.addEventListener('click',()=>litOne(physIdx,d));
          og.appendChild(d);
        }
      }
      document.getElementById('obSection').style.display='';
      updateGrid(j.activeLeds||[]);
    }
    if(j.seg7Grid){
      // 7-Segment-Layout: Digit 3=Std-Zehner, Digit 2=Std-Einer, Doppelpunkt, Digit 1=Min-Zehner, Digit 0=Min-Einer
      // Segment-Mapping: [pos][seg] wobei seg: 0=a(oben),1=b(r-oben),2=c(r-unten),3=d(unten),4=e(l-unten),5=f(l-oben),6=g(mitte)
      const segDigits=[
        [ 2, 1, 0, 6, 5, 3, 4],  // Digit 0: Min-Einer
        [ 9, 8, 7,13,12,10,11],  // Digit 1: Min-Zehner
        [16,15,14,20,19,17,18],  // Digit 2: Std-Einer
        [23,22,21,27,26,24,25],  // Digit 3: Std-Zehner
      ];
      const segColon=[28,29];
      const segLabels=['Min-Einer','Min-Zehner','Std-Einer','Std-Zehner'];
      const container=document.getElementById('seg7Grid');
      // Ziffern von links nach rechts: Digit 3, Digit 2, Doppelpunkt, Digit 1, Digit 0
      [3,2,'colon',1,0].forEach(pos=>{
        if(pos==='colon'){
          const col=document.createElement('div');
          col.className='seg-colon';
          segColon.forEach((id,ci)=>{
            const d=document.createElement('div');
            d.className='seg-dot';d.title='LED '+id;d.dataset.idx=id;
            d.addEventListener('click',()=>litOne(id,d));
            col.appendChild(d);
          });
          container.appendChild(col);
          return;
        }
        const wrap=document.createElement('div');
        wrap.style.cssText='display:flex;flex-direction:column;align-items:center';
        // Digit grid: 3 Zeilen × 3 Spalten
        // Zeile 0: [leer][a][leer]
        // Zeile 1: [f][leer][b]
        // Zeile 2: [leer][g][leer]
        // Zeile 3: [e][leer][c]
        // Zeile 4: [leer][d][leer]
        const dg=document.createElement('div');
        dg.className='seg-digit';
        const ids=segDigits[pos];
        // Segmente: 0=a(oben-H), 5=f(links-oben-V), 1=b(rechts-oben-V), 6=g(mitte-H), 4=e(links-unten-V), 2=c(rechts-unten-V), 3=d(unten-H)
        const layout=[
          {seg:0,cls:'seg seg-h',col:'1/4',row:'1'},   // a: oben
          {seg:5,cls:'seg seg-v',col:'1',row:'2/4'},    // f: links-oben
          {seg:1,cls:'seg seg-v',col:'3',row:'2/4'},    // b: rechts-oben
          {seg:6,cls:'seg seg-h',col:'1/4',row:'3'},    // g: mitte
          {seg:4,cls:'seg seg-v',col:'1',row:'4/6'},    // e: links-unten
          {seg:2,cls:'seg seg-v',col:'3',row:'4/6'},    // c: rechts-unten
          {seg:3,cls:'seg seg-h',col:'1/4',row:'5'},    // d: unten
        ];
        layout.forEach(({seg,cls,col,row})=>{
          const d=document.createElement('div');
          d.className=cls;
          d.style.cssText='grid-column:'+col+';grid-row:'+row;
          d.title='LED '+ids[seg];d.dataset.idx=ids[seg];
          d.addEventListener('click',()=>litOne(ids[seg],d));
          dg.appendChild(d);
        });
        wrap.appendChild(dg);
        const lbl=document.createElement('div');
        lbl.className='seg-label';lbl.textContent=segLabels[pos];
        wrap.appendChild(lbl);
        container.appendChild(wrap);
      });
      document.getElementById('seg7Section').style.display='';
      updateGrid(j.activeLeds||[]);
    }
    if(j.bttf1Grid){
      const container=document.getElementById('bttf1Grid');
      container.innerHTML='';
      const labels=['Monat / Tag','Jahr','Stunde / Minute'];
      const values=[j.bttf1d1||'--:--', j.bttf1d2||'----', j.bttf1d3||'--:--'];
      labels.forEach((lbl,i)=>{
        const wrap=document.createElement('div');
        wrap.style.cssText='text-align:center';
        const disp=document.createElement('div');
        disp.style.cssText='background:#111;color:#f00;font-family:monospace;font-size:2rem;font-weight:bold;padding:10px 16px;border-radius:6px;letter-spacing:4px;min-width:100px';
        if(i===1) disp.style.color='#fa0';
        if(i===2) disp.style.color='#00f';
        disp.id='bttf1disp'+i;
        disp.textContent=values[i];
        const l=document.createElement('div');
        l.style.cssText='font-size:.8rem;color:#666;margin-top:4px';
        l.textContent=lbl;
        wrap.appendChild(disp);wrap.appendChild(l);
        container.appendChild(wrap);
        // AM/PM LEDs nach Display 2 (Jahr)
        if(i===1){
          const isPM=j.bttf1pm||false;
          const ampm=document.createElement('div');
          ampm.style.cssText='display:flex;flex-direction:column;align-items:center;justify-content:center;gap:10px;padding:0 4px';
          ampm.id='bttf1ampm';
          const active12=j.bttf1amactive||false;
          ['AM','PM'].forEach(lbl2=>{
            const row=document.createElement('div');
            row.style.cssText='display:flex;flex-direction:column;align-items:center;gap:3px';
            const led=document.createElement('div');
            const isOn=active12&&((lbl2==='AM'&&!isPM)||(lbl2==='PM'&&isPM));
            led.style.cssText='width:18px;height:18px;border-radius:50%;border:2px solid #555;background:'+(isOn?'#ff2200':(active12?'#330000':'#222'))+';box-shadow:'+(isOn?'0 0 8px #ff2200':'none');
            led.style.opacity=active12?'1':'0.3';
            led.id='bttf1led'+lbl2;
            const ltxt=document.createElement('div');
            ltxt.style.cssText='font-size:.65rem;font-weight:bold;color:#888';
            ltxt.textContent=lbl2;
            row.appendChild(led);row.appendChild(ltxt);
            ampm.appendChild(row);
          });
          container.appendChild(ampm);
        }
      });
      document.getElementById('bttf1Section').style.display='';
    }
  }catch(e){
    document.getElementById('fw').textContent='JS-Fehler: '+e.message;
    document.getElementById('build').textContent=String(e);
  }
})();
document.getElementById('btnNtpSync').addEventListener('click',async()=>{
  document.getElementById('ntpStatus').textContent='Synchronisiere...';
  const r=await fetch('/ntpsync',{method:'POST'});
  const txt=await r.text();
  document.getElementById('ntpStatus').textContent='✓ '+txt+' – Seite wird neu geladen...';
  document.getElementById('ntpTime').textContent=txt;
  setTimeout(()=>location.reload(),1500);
});
async function changeDebugPw(){
  const p1=document.getElementById('newPw1').value;
  const p2=document.getElementById('newPw2').value;
  const msg=document.getElementById('pwChangeMsg');
  if(!p1){msg.style.color='#c00';msg.textContent='Bitte Passwort eingeben.';return;}
  if(p1!==p2){msg.style.color='#c00';msg.textContent='Passwörter stimmen nicht überein.';return;}
  if(p1.length<3){msg.style.color='#c00';msg.textContent='Mindestens 3 Zeichen.';return;}
  const r=await fetch('/debugpw',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'pw='+encodeURIComponent(p1)});
  if(r.ok){
    msg.style.color='#060';msg.textContent='✓ Gespeichert.';
    document.getElementById('newPw1').value='';
    document.getElementById('newPw2').value='';
    setTimeout(()=>msg.textContent='',3000);
  } else { msg.style.color='#c00';msg.textContent='Fehler beim Speichern.'; }
}
async function refreshGrid(){
  try{
    const r=await fetch('/state');if(!r.ok)return;
    const j=await r.json();
    updateGrid(j.activeLeds||[]);
    document.getElementById('ntpTime').textContent=j.time;
    if(j.words)document.getElementById('ntpWords').textContent=j.words;
    if(j.bttf1Grid){
      const d1=document.getElementById('bttf1disp0');
      const d2=document.getElementById('bttf1disp1');
      const d3=document.getElementById('bttf1disp2');
      if(d1) d1.textContent=j.bttf1d1||'--:--';
      if(d2) d2.textContent=j.bttf1d2||'----';
      if(d3) d3.textContent=j.bttf1d3||'--:--';
      const isPM=j.bttf1pm||false;
      const active=j.bttf1amactive||false;
      const ledAM=document.getElementById('bttf1ledAM');
      const ledPM=document.getElementById('bttf1ledPM');
      if(ledAM){
        const on=active&&!isPM;
        ledAM.style.background=on?'#ff2200':(active?'#330000':'#222');
        ledAM.style.boxShadow=on?'0 0 8px #ff2200':'none';
        ledAM.style.opacity=active?'1':'0.3';
      }
      if(ledPM){
        const on=active&&isPM;
        ledPM.style.background=on?'#ff2200':(active?'#330000':'#222');
        ledPM.style.boxShadow=on?'0 0 8px #ff2200':'none';
        ledPM.style.opacity=active?'1':'0.3';
      }
    }
  }catch(e){}
}
document.getElementById('applyPrev').addEventListener('click',async()=>{
  let t=document.getElementById('forceTime').value;
  if(!t){alert('Bitte Zeit waehlen');return;}
  await fetch('/preview',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'en=1&t='+encodeURIComponent(t)});
  document.getElementById('prv').textContent=' (Testzeit aktiv: '+t+')';
  await refreshGrid();
});
document.getElementById('clearPrev').addEventListener('click',async()=>{
  await fetch('/preview',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'en=0'});
  document.getElementById('forceTime').value='';
  document.getElementById('prv').textContent='';
  await refreshGrid();
});
document.getElementById('applyPrev2').addEventListener('click',async()=>{
  const mo=document.getElementById('forceMon').value;
  const da=document.getElementById('forceDay').value;
  const yr=document.getElementById('forceYear').value;
  const ti=document.getElementById('forceTime2').value;
  if(!mo||!da||!yr||!ti){alert('Bitte alle Felder ausfüllen');return;}
  const body='en=1&t='+encodeURIComponent(ti)+'&mo='+mo+'&da='+da+'&yr='+yr;
  await fetch('/preview',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
  document.getElementById('prv2').textContent=' (aktiv: '+mo+'/'+da+'/'+yr+' '+ti+')';
  await refreshGrid();
});
document.getElementById('clearPrev2').addEventListener('click',async()=>{
  await fetch('/preview',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'en=0'});
  document.getElementById('forceMon').value='';
  document.getElementById('forceDay').value='';
  document.getElementById('forceYear').value='';
  document.getElementById('forceTime2').value='';
  document.getElementById('prv2').textContent='';
  await refreshGrid();
});
function toggleWlanForm(){
  const f=document.getElementById('wlanForm');
  const open=f.style.display==='none';
  f.style.display=open?'':'none';
  if(open){
    document.getElementById('wlanErr').textContent='';
    document.getElementById('wlanManual').value='';
    document.getElementById('wlanPw').value='';
    document.getElementById('wlanScanStatus').textContent='Scanne Netzwerke...';
    document.getElementById('wlanSSID').style.display='none';
    document.getElementById('wlanSSID').innerHTML='<option value="">– Netzwerk wählen –</option>';
    fetch('/wlanscan').then(r=>r.json()).then(nets=>{
      const sel=document.getElementById('wlanSSID');
      nets.forEach(n=>{
        const o=document.createElement('option');
        o.value=n.ssid; o.textContent=n.ssid+' ('+n.rssi+'dBm · '+n.band+(n.enc?' 🔒':' 🔓')+')';
        sel.appendChild(o);
      });
      sel.style.display='';
      document.getElementById('wlanScanStatus').textContent=nets.length+' Netzwerke gefunden:';
    }).catch(()=>{
      document.getElementById('wlanScanStatus').textContent='Scan fehlgeschlagen – SSID manuell eingeben.';
    });
  }
}
async function doWlanConnect(){
  const ssid=(document.getElementById('wlanSSID').value||document.getElementById('wlanManual').value).trim();
  const pw=document.getElementById('wlanPw').value;
  if(!ssid){document.getElementById('wlanErr').textContent='Bitte SSID eingeben.';return;}
  document.getElementById('wlanConnBtn').disabled=true;
  document.getElementById('wlanConnBtn').textContent='Verbinde...';
  try{
    const r=await fetch('/wlanconnect',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ssid='+encodeURIComponent(ssid)+'&pw='+encodeURIComponent(pw)});
    const t=await r.text();
    document.getElementById('wlanErr').style.color='#060';
    document.getElementById('wlanErr').textContent=t+' – Neustart...';
  }catch(e){
    document.getElementById('wlanErr').style.color='#c00';
    document.getElementById('wlanErr').textContent='Fehler: '+e;
    document.getElementById('wlanConnBtn').disabled=false;
    document.getElementById('wlanConnBtn').textContent='Verbinden & Neustart';
  }
}
</script>
</body></html>)HTML";

// ============================================================
//  HTTP HANDLER
// ============================================================
static void handleIndex() {
  String page = String(FPSTR(HTML_INDEX));
  page.replace("__DEBUG_PW__",    gDebugPassword);
  page.replace("__DEVICE_NAME__", DEVICE_NAME);
  page.replace("__BMIN__", String(BRIGHTNESS_MIN));
  page.replace("__BMAX__", String(BRIGHTNESS_MAX));
  webServer.send(200, "text/html; charset=utf-8", page);
}

static void handleDebug() {
  String page = String(FPSTR(HTML_DEBUG));
  page.replace("__DEVICE_NAME__", DEVICE_NAME);
  webServer.send(200, "text/html; charset=utf-8", page);
}

static void handleState() {
  int H = _dispH(), M = _dispM();
  uint8_t eff = _effectiveBrightness();
  String ip = (WiFi.status() == WL_CONNECTED)
              ? WiFi.localIP().toString()
              : WiFi.softAPIP().toString();
  char ts[8]; snprintf(ts, sizeof(ts), "%02d:%02d", H % 24, M % 60);
  String prv = gPreviewEnabled
               ? formatHHMM((uint16_t)(gPreviewHour * 60 + gPreviewMinute))
               : String("");
  extern String user_timeToWords(int h, int m);
  String words = user_timeToWords(H, M);
  // Umlaute und Sonderzeichen für JSON escapen
  words.replace("\\", "\\\\");
  words.replace("\"", "\\\"");
  words.replace("ä", "\\u00e4");
  words.replace("ö", "\\u00f6");
  words.replace("ü", "\\u00fc");
  words.replace("Ä", "\\u00c4");
  words.replace("Ö", "\\u00d6");
  words.replace("Ü", "\\u00dc");
  words.replace("ß", "\\u00df");
  // JSON zusammenbauen
  String json = "{";
  json += "\"name\":\""     + String(DEVICE_NAME)          + "\",";
  json += "\"time\":\""     + String(ts)                   + "\",";
  json += "\"words\":\""    + words                        + "\",";
  json += "\"ip\":\""       + ip                           + "\",";
  json += "\"rssi\":"       + String(WiFi.RSSI())          + ",";
  json += "\"brightness\":" + String(gBrightness)          + ",";
  json += "\"effective\":"  + String(eff)                  + ",";
  json += "\"dimEnabled\":" + String(gDimEnabled?"true":"false") + ",";
  json += "\"dimStart\":\""  + formatHHMM(gDimStart)       + "\",";
  json += "\"dimEnd\":\""    + formatHHMM(gDimEnd)         + "\",";
  json += "\"dimBrightness\":" + String(gDimLevel)         + ",";
  json += "\"preview\":"    + String(gPreviewEnabled?"true":"false") + ",";
  json += "\"previewTime\":\"" + prv                       + "\",";
  json += "\"fw\":\""       + String(FW_VERSION)           + "\",";
  json += "\"build\":\""    + String(__DATE__) + " " + String(__TIME__) + "\",";
  json += "\"uptime\":\""   + uptimeString()               + "\",";
  json += "\"ntpServer\":\"" + String(NTP_SERVER1)         + "\",";
  char syncBuf[12]; snprintf(syncBuf, sizeof(syncBuf), "%02d:%02d", (int)gHour, (int)gMinute);
  json += "\"ntpLastSync\":\"" + String(syncBuf)           + "\",";
#if ACTIVE_CLOCK == WORDCLOCK
  json += "\"ledGrid\":true,";
  json += "\"activeLeds\":" + user_getActiveLeds() + ",";
#elif ACTIVE_CLOCK == OBEGRANSAD
  json += "\"ledGrid\":false,";
  json += "\"activeLeds\":" + user_getActiveLeds() + ",";
#elif ACTIVE_CLOCK == TIME_2_WORDS
  json += "\"ledGrid\":false,";
  json += "\"activeLeds\":" + user_getActiveLeds() + ",";
#elif ACTIVE_CLOCK == SIEBEN_SEGMENTE
  json += "\"ledGrid\":false,";
  json += "\"activeLeds\":" + user_getActiveLeds() + ",";
#else
  json += "\"ledGrid\":false,";
  json += "\"activeLeds\":[],";
#endif
#if ACTIVE_CLOCK == OBEGRANSAD
  json += "\"obGrid\":true,";
#else
  json += "\"obGrid\":false,";
#endif
#if ACTIVE_CLOCK == SIEBEN_SEGMENTE
  json += "\"seg7Grid\":true,";
#else
  json += "\"seg7Grid\":false,";
#endif
#if ACTIVE_CLOCK == BTTF1
  json += "\"bttf1Grid\":true,";
  {
    // Datum für Anzeige: Preview-Datum oder aktuelle Systemzeit
    time_t _now = time(nullptr);
    struct tm *_t = localtime(&_now);
    int _mon  = gPreviewEnabled ? gPreviewMonth : (_t->tm_mon + 1);
    int _day  = gPreviewEnabled ? gPreviewDay   : _t->tm_mday;
    int _year = gPreviewEnabled ? gPreviewYear  : (_t->tm_year + 1900);
    char d1buf[6], d2buf[5], d3buf[6];
    snprintf(d1buf, sizeof(d1buf), "%02d:%02d", _mon, _day);
    snprintf(d2buf, sizeof(d2buf), "%04d", _year);
    int _h3 = H % 24;
    if (g12hMode) { _h3 = _h3 % 12; if (_h3 == 0) _h3 = 12; }
    snprintf(d3buf, sizeof(d3buf), "%02d:%02d", _h3, M % 60);
    json += "\"bttf1d1\":\"" + String(d1buf) + "\",";
    json += "\"bttf1d2\":\"" + String(d2buf) + "\",";
    json += "\"bttf1d3\":\"" + String(d3buf) + "\",";
    // previewFull für Testzeit-Felder
    char pfbuf[20];
    snprintf(pfbuf, sizeof(pfbuf), "%02d %02d %04d %02d:%02d",
      _mon, _day, _year, H % 24, M % 60);
    json += "\"previewFull\":\"" + String(pfbuf) + "\",";
    json += "\"bttf1pm\":" + String((g12hMode && (H % 24) >= 12) ? "true" : "false") + ",";
    json += "\"bttf1amactive\":" + String(g12hMode ? "true" : "false") + ",";
  }
#else
  json += "\"bttf1Grid\":false,";
  json += "\"bttf1d1\":\"\",\"bttf1d2\":\"\",\"bttf1d3\":\"\",";
  json += "\"previewFull\":\"\",";
  json += "\"bttf1pm\":false,";
  json += "\"bttf1amactive\":false,";
#endif
#if ACTIVE_CLOCK == TIME_2_WORDS
  json += "\"fwcGrid\":true,";
#else
  json += "\"fwcGrid\":false,";
#endif
#if ACTIVE_CLOCK == TIME_2_WORDS
  json += "\"hasFrame\":true,";
#else
  json += "\"hasFrame\":false,";
#endif
#if ACTIVE_CLOCK == TIME_2_WORDS || ACTIVE_CLOCK == WORDCLOCK || ACTIVE_CLOCK == SIEBEN_SEGMENTE
  json += "\"hasColors\":true,";
#else
  json += "\"hasColors\":false,";
#endif
#if ACTIVE_CLOCK == SIEBEN_SEGMENTE
  json += "\"hasDots\":true,";
  json += "\"dotsEnabled\":" + String(gDotsEnabled ? "true" : "false") + ",";
#else
  json += "\"hasDots\":false,";
  json += "\"dotsEnabled\":false,";
#if ACTIVE_CLOCK == BTTF1 || ACTIVE_CLOCK == BTTF3
  json += "\"has12h\":true,";
  json += "\"mode12h\":" + String(g12hMode ? "true" : "false") + ",";
#else
  json += "\"has12h\":false,";
  json += "\"mode12h\":false,";
#endif
#if ACTIVE_CLOCK == BTTF3
  json += "\"hasBttf3Times\":true,";
  json += "\"destMon\":"  + String(gBttf3DestMon)  + ",";
  json += "\"destDay\":"  + String(gBttf3DestDay)  + ",";
  json += "\"destYear\":" + String(gBttf3DestYear) + ",";
  json += "\"destHour\":" + String(gBttf3DestHour) + ",";
  json += "\"destMin\":"  + String(gBttf3DestMin)  + ",";
  json += "\"lastMon\":"  + String(gBttf3LastMon)  + ",";
  json += "\"lastDay\":"  + String(gBttf3LastDay)  + ",";
  json += "\"lastYear\":" + String(gBttf3LastYear) + ",";
  json += "\"lastHour\":" + String(gBttf3LastHour) + ",";
  json += "\"lastMin\":"  + String(gBttf3LastMin)  + ",";
#else
  json += "\"hasBttf3Times\":false,";
#endif
#endif
  json += "\"cTimeR\":"  + String(gColorTimeR)  + ",";
  json += "\"cTimeG\":"  + String(gColorTimeG)  + ",";
  json += "\"cTimeB\":"  + String(gColorTimeB)  + ",";
  json += "\"cFrameR\":" + String(gColorFrameR) + ",";
  json += "\"cFrameG\":" + String(gColorFrameG) + ",";
  json += "\"cFrameB\":" + String(gColorFrameB) + "}";
  webServer.send(200, "application/json; charset=utf-8", json);
}

static void handleBrightness() {
  if (!webServer.hasArg("v")) { webServer.send(400, "text/plain", "missing v"); return; }
  int v = constrain(webServer.arg("v").toInt(), BRIGHTNESS_MIN, BRIGHTNESS_MAX);
  settingsSaveBrightness((uint8_t)v);
  user_onBrightnessChange((uint8_t)v);
  webServer.send(200, "text/plain", "OK");
}

static void handleDim() {
  if (!webServer.hasArg("en") || !webServer.hasArg("start") ||
      !webServer.hasArg("end") || !webServer.hasArg("lvl")) {
    webServer.send(400, "text/plain", "Missing params"); return;
  }
  gDimEnabled = (webServer.arg("en").toInt() == 1);
  uint16_t st, ed;
  if (!parseHHMM(webServer.arg("start"), st) || !parseHHMM(webServer.arg("end"), ed)) {
    webServer.send(400, "text/plain", "Bad HH:MM"); return;
  }
  gDimStart = st; gDimEnd = ed;
  gDimLevel = (uint8_t)constrain(webServer.arg("lvl").toInt(), BRIGHTNESS_MIN, BRIGHTNESS_MAX);
  settingsSaveDim();
  user_onBrightnessChange(_effectiveBrightness());
  webServer.send(200, "text/plain", "OK");
}

static void handleColor() {
  if (!webServer.hasArg("tr") || !webServer.hasArg("fr")) {
    webServer.send(400, "text/plain", "Missing params"); return;
  }
  gColorTimeR  = (uint8_t)constrain(webServer.arg("tr").toInt(), 0, 255);
  gColorTimeG  = (uint8_t)constrain(webServer.arg("tg").toInt(), 0, 255);
  gColorTimeB  = (uint8_t)constrain(webServer.arg("tb").toInt(), 0, 255);
  gColorFrameR = (uint8_t)constrain(webServer.arg("fr").toInt(), 0, 255);
  gColorFrameG = (uint8_t)constrain(webServer.arg("fg").toInt(), 0, 255);
  gColorFrameB = (uint8_t)constrain(webServer.arg("fb").toInt(), 0, 255);
  settingsSaveColors();
  user_onColorsChange();
  webServer.send(200, "text/plain", "Farben gespeichert");
}

static void handleDots() {
  if (!webServer.hasArg("en")) { webServer.send(400, "text/plain", "Missing en"); return; }
  gDotsEnabled = (webServer.arg("en").toInt() == 1);
  settingsSave();
  webServer.send(200, "text/plain", gDotsEnabled ? "Sekundentakt an" : "Doppelpunkt dauerhaft an");
}

static void handlePreview() {
  if (!webServer.hasArg("en")) { webServer.send(400, "text/plain", "Missing en"); return; }
  if (webServer.arg("en").toInt() == 1) {
    if (!webServer.hasArg("t")) { webServer.send(400, "text/plain", "Missing t"); return; }
    uint16_t mins;
    if (!parseHHMM(webServer.arg("t"), mins)) { webServer.send(400, "text/plain", "Bad HH:MM"); return; }
    gPreviewEnabled = true;
    gPreviewHour    = mins / 60;
    gPreviewMinute  = mins % 60;
    // Datum-Parameter (optional, für BTTF1)
    if (webServer.hasArg("mo")) gPreviewMonth = (uint8_t)constrain(webServer.arg("mo").toInt(), 1, 12);
    if (webServer.hasArg("da")) gPreviewDay   = (uint8_t)constrain(webServer.arg("da").toInt(), 1, 31);
    if (webServer.hasArg("yr")) gPreviewYear  = (uint16_t)constrain(webServer.arg("yr").toInt(), 1, 9999);
    user_onTimeChange(gPreviewHour, gPreviewMinute);
  } else {
    gPreviewEnabled = false;
    user_onTimeChange(gHour, gMinute);
  }
  webServer.send(200, "text/plain", gPreviewEnabled ? "on" : "off");
}

static void handleNtpSync() {
  extern void ntpForceSync();
  ntpForceSync();
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", (int)gHour, (int)gMinute);
  webServer.send(200, "text/plain", buf);
}

static void handleReset() {
  webServer.send(200, "text/html; charset=utf-8",
    "<html><body><h2>Setze WLAN zurueck...</h2><p>Neustart in 2s.</p></body></html>");
  delay(1500);
  WiFiManager wm; wm.resetSettings(); delay(300); ESP.restart();
}

static void handleDebugPw() {
  String pw = webServer.arg("pw");
  if (pw.length() < 3) { webServer.send(400, "text/plain", "Zu kurz"); return; }
  gDebugPassword = pw;
  settingsSave();
  webServer.send(200, "text/plain", "OK");
}

static void handleBttf3Times() {
  auto gi = [&](const char* k, int def) {
    return webServer.hasArg(k) ? webServer.arg(k).toInt() : def;
  };
  gBttf3DestMon  = (uint8_t)constrain(gi("destMon",  gBttf3DestMon),  1, 12);
  gBttf3DestDay  = (uint8_t)constrain(gi("destDay",  gBttf3DestDay),  1, 31);
  gBttf3DestYear = (uint16_t)constrain(gi("destYear", gBttf3DestYear), 1, 9999);
  gBttf3DestHour = (uint8_t)constrain(gi("destHour", gBttf3DestHour), 0, 23);
  gBttf3DestMin  = (uint8_t)constrain(gi("destMin",  gBttf3DestMin),  0, 59);
  gBttf3LastMon  = (uint8_t)constrain(gi("lastMon",  gBttf3LastMon),  1, 12);
  gBttf3LastDay  = (uint8_t)constrain(gi("lastDay",  gBttf3LastDay),  1, 31);
  gBttf3LastYear = (uint16_t)constrain(gi("lastYear", gBttf3LastYear), 1, 9999);
  gBttf3LastHour = (uint8_t)constrain(gi("lastHour", gBttf3LastHour), 0, 23);
  gBttf3LastMin  = (uint8_t)constrain(gi("lastMin",  gBttf3LastMin),  0, 59);
  settingsSave();
  extern void user_onTimeChange(int, int);
  user_onTimeChange(gPreviewEnabled ? gPreviewHour : (int)gHour,
                    gPreviewEnabled ? gPreviewMinute : (int)gMinute);
  webServer.send(200, "text/plain", "Gespeichert");
}

static void handle12h() {
  if (!webServer.hasArg("en")) { webServer.send(400, "text/plain", "Missing en"); return; }
  g12hMode = (webServer.arg("en").toInt() == 1);
  settingsSave();
  // AM/PM sofort aktualisieren
  extern void user_onTimeChange(int, int);
  user_onTimeChange(gPreviewEnabled ? gPreviewHour : (int)gHour,
                    gPreviewEnabled ? gPreviewMinute : (int)gMinute);
  webServer.send(200, "text/plain", g12hMode ? "12h-Modus aktiv" : "24h-Modus aktiv");
}

static void handleWlanScan() {
  int n = WiFi.scanNetworks();
  // Deduplizieren: pro SSID nur den stärksten Eintrag behalten
  struct Net { String ssid; int rssi; bool enc; int ch; };
  std::vector<Net> nets;
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    int rssi    = WiFi.RSSI(i);
    bool enc    = WiFi.encryptionType(i) != 0;
    int ch      = WiFi.channel(i);
    bool found  = false;
    for (auto &e : nets) {
      if (e.ssid == ssid) { if (rssi > e.rssi) { e.rssi=rssi; e.enc=enc; e.ch=ch; } found=true; break; }
    }
    if (!found) nets.push_back({ssid, rssi, enc, ch});
  }
  // Sortieren: stärkster zuerst
  std::sort(nets.begin(), nets.end(), [](const Net &a, const Net &b){ return a.rssi > b.rssi; });
  String json = "[";
  for (size_t i = 0; i < nets.size(); i++) {
    if (i > 0) json += ",";
    // Channel 1-13 = 2.4GHz, 36+ = 5GHz
    String band = (nets[i].ch >= 36) ? "5GHz" : "2.4GHz";
    json += "{\"ssid\":\"" + nets[i].ssid + "\","
            "\"rssi\":"    + String(nets[i].rssi) + ","
            "\"enc\":"     + String(nets[i].enc ? "true" : "false") + ","
            "\"band\":\"" + band + "\"}";
  }
  json += "]";
  webServer.send(200, "application/json", json);
}

static void handleWlanConnect() {
  String ssid = webServer.arg("ssid");
  String pw   = webServer.arg("pw");
  if (ssid.isEmpty()) { webServer.send(400, "text/plain", "Keine SSID"); return; }
  webServer.send(200, "text/plain", "OK – verbinde mit " + ssid);
  delay(300);
  WiFi.begin(ssid.c_str(), pw.c_str());
  delay(1500);
  ESP.restart();
}

// Debug-Restore: nach 10s zurück zur Uhrzeit
static unsigned long _debugRestoreAt = 0;  // 0 = kein Restore ausstehend

static void _scheduleRestore() {
  _debugRestoreAt = millis() + 10000UL;
}
static void _cancelRestore() {
  _debugRestoreAt = 0;
}

static void handleAllLeds() {
  bool on = (webServer.arg("on") == "1");
  if (on) {
    user_onBrightnessChange(255);
    user_allLedsOn();
  } else {
    user_allLedsOff();
  }
  _scheduleRestore();
  webServer.send(200, "text/plain", on ? "ALL ON" : "ALL OFF");
}

static void handleLedOn() {
  int idx = -1;
  if (webServer.hasArg("i")) idx = webServer.arg("i").toInt();
  if (idx < 0) { webServer.send(400, "text/plain", "bad idx"); return; }
  bool ok = user_ledOn(idx);
  if (ok) _scheduleRestore();
  webServer.send(200, "text/plain", ok ? String("LED ") + idx + " ON" : "not supported");
}

// ============================================================
//  PUBLIC API
// ============================================================
void webifSetup() {
  webServer.on("/",           HTTP_GET,  handleIndex);
  webServer.on("/debug",      HTTP_GET,  handleDebug);
  webServer.on("/state",      HTTP_GET,  handleState);
  webServer.on("/brightness", HTTP_POST, handleBrightness);
  webServer.on("/dim",        HTTP_POST, handleDim);
  webServer.on("/color",      HTTP_POST, handleColor);
  webServer.on("/dots",       HTTP_POST, handleDots);
  webServer.on("/12h",        HTTP_POST, handle12h);
  webServer.on("/bttf3times",  HTTP_POST, handleBttf3Times);
  webServer.on("/preview",    HTTP_POST, handlePreview);
  webServer.on("/ntpsync",    HTTP_POST, handleNtpSync);
  webServer.on("/reset",       HTTP_ANY,  handleReset);
  webServer.on("/debugpw",     HTTP_POST, handleDebugPw);
  webServer.on("/wlanscan",    HTTP_GET,  handleWlanScan);
  webServer.on("/wlanconnect", HTTP_POST, handleWlanConnect);
  webServer.on("/allleds",    HTTP_POST, handleAllLeds);
  webServer.on("/ledon",      HTTP_GET,  handleLedOn);
  webServer.on("/update", HTTP_GET, [](){
    webServer.send(200, "text/html; charset=utf-8", F(
      "<!DOCTYPE html><html><head><meta charset='utf-8'>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<title>Firmware-Update</title>"
      "<style>"
      "body{font-family:sans-serif;background:#111;color:#ccc;display:flex;"
        "flex-direction:column;align-items:center;justify-content:center;"
        "min-height:100vh;margin:0;padding:20px;box-sizing:border-box}"
      "h2{margin-bottom:24px;color:#fff}"
      ".card{background:#1e1e1e;border:1px solid #333;border-radius:10px;"
        "padding:32px;width:100%;max-width:420px;box-sizing:border-box}"
      "label{display:block;margin-bottom:8px;font-size:.9rem;color:#aaa}"
      "input[type=file]{display:block;width:100%;padding:8px;"
        "background:#2a2a2a;border:1px solid #444;border-radius:6px;"
        "color:#ccc;margin-bottom:16px;box-sizing:border-box;cursor:pointer}"
      "button{width:100%;padding:12px;background:#3a7bd5;border:none;"
        "border-radius:6px;color:#fff;font-size:1rem;cursor:pointer;transition:.2s}"
      "button:hover{background:#2f68be}"
      "button:disabled{background:#444;cursor:not-allowed}"
      "#progressWrap{display:none;margin-top:20px}"
      "#bar{width:0%;height:18px;background:#3a7bd5;border-radius:4px;"
        "transition:width .15s;text-align:center;font-size:11px;line-height:18px;color:#fff}"
      "#barTrack{background:#2a2a2a;border-radius:4px;border:1px solid #444}"
      "#status{margin-top:12px;font-size:.9rem;text-align:center;color:#aaa}"
      ".ok{color:#4caf50!important}.err{color:#f44!important}"
      "a{color:#aaa;font-size:.85rem;display:block;text-align:center;margin-top:20px}"
      "</style></head><body>"
      "<div class='card'>"
      "<h2>&#x1F4E5; Firmware-Update</h2>"
      "<label for='fwfile'>Firmware-Datei (.bin):</label>"
      "<input type='file' id='fwfile' accept='.bin'>"
      "<button id='btnUp' onclick='startUpload()'>Update starten</button>"
      "<div id='progressWrap'>"
      "  <div id='barTrack'><div id='bar'>0%</div></div>"
      "  <div id='status'>Vorbereitung...</div>"
      "</div>"
      "</div>"
      "<a href='/'>&#x2190; Zur&uuml;ck zur Hauptseite</a>"
      "<script>"
      "function startUpload(){"
      "  const f=document.getElementById('fwfile').files[0];"
      "  if(!f){alert('Bitte zuerst eine .bin-Datei ausw\\u00e4hlen.');return;}"
      "  const btn=document.getElementById('btnUp');"
      "  btn.disabled=true;"
      "  document.getElementById('progressWrap').style.display='block';"
      "  const xhr=new XMLHttpRequest();"
      "  xhr.upload.addEventListener('progress',function(e){"
      "    if(e.lengthComputable){"
      "      const pct=Math.round(e.loaded/e.total*100);"
      "      document.getElementById('bar').style.width=pct+'%';"
      "      document.getElementById('bar').textContent=pct+'%';"
      "      document.getElementById('status').textContent='Hochladen... '+pct+'%';"
      "    }"
      "  });"
      "  xhr.addEventListener('load',function(){"
      "    if(xhr.status===200){"
      "      document.getElementById('bar').style.width='100%';"
      "      document.getElementById('bar').textContent='100%';"
      "      document.getElementById('status').className='ok';"
      "      document.getElementById('status').textContent='\\u2713 Update erfolgreich! Ger\\u00e4t startet neu...';"
      "    }else{"
      "      document.getElementById('status').className='err';"
      "      document.getElementById('status').textContent='\\u2717 Fehler: '+xhr.status+' '+xhr.responseText;"
      "      btn.disabled=false;"
      "    }"
      "  });"
      "  xhr.addEventListener('error',function(){"
      "    document.getElementById('status').className='err';"
      "    document.getElementById('status').textContent='\\u2717 Verbindungsfehler';"
      "    btn.disabled=false;"
      "  });"
      "  const fd=new FormData();"
      "  fd.append('firmware',f,f.name);"
      "  xhr.open('POST','/update');"
      "  xhr.send(fd);"
      "}"
      "</script></body></html>"
    ));
  });
  _httpUpdater.setup(&webServer, "/update");
  webServer.begin();
  Serial.printf("[WEB] http://%s/\n", WiFi.localIP().toString().c_str());
}

void webifLoop() {
  webServer.handleClient();
  // Debug-Restore: nach 10s zurück zur Uhrzeit
  if (_debugRestoreAt && millis() >= _debugRestoreAt) {
    _debugRestoreAt = 0;
    user_onBrightnessChange(_effectiveBrightness());
    user_onTimeChange(_dispH(), _dispM());
  }
}
