# Standalone, disposable icon-collection page.
# One tile per DEVICE (keyed by device_id, so every device gets its own image regardless of
# whatever icon_id the game data shares between them). Same class/category grouping and order
# as the loadout page, but names + drop zones only.
import json, html, os
BASE = os.path.dirname(os.path.abspath(__file__))
D = json.load(open(os.path.join(BASE, 'inv_model.json'), encoding='utf-8'))
model = D['model']
ORDER = ['Assault', 'Medic', 'Recon', 'Robotics']
CATORD = ['Melee', 'Ranged', 'Specialty', 'Offhand', 'Boost', 'Jetpack']
ACC = {'Assault': '#ff7a45', 'Medic': '#37d3a6', 'Recon': '#a882ff', 'Robotics': '#3fb4ff'}
def esc(s): return html.escape(str(s))
ICONOF = json.load(open(os.path.join(BASE, 'devicon.json'), encoding='utf-8'))

panes, total = [], 0
for cls in ORDER:
    cats = model[cls]
    order = [c for c in CATORD if c in cats] + [c for c in cats if c not in CATORD]
    blocks = []
    n = 0
    for cat in order:
        tiles = []
        for d in sorted(cats[cat], key=lambda x: x['name']):
            n += 1; total += 1
            tiles.append(
                '<div class="t" data-dev="%s" title="Click, drop an image, or paste while hovering">'
                '<div class="slot"><span class="add">+ image</span></div>'
                '<div class="nm">%s%s</div><div class="id">#%s</div></div>'
                % (d['id'], esc(d['name']), ' <b>OC</b>' if d['oc'] else '', d['id']))
        blocks.append('<section><h3>%s <span>%d</span></h3><div class="grid">%s</div></section>'
                      % (esc(cat), len(cats[cat]), "".join(tiles)))
    panes.append('<div class="pane" style="--acc:%s"><h2>%s <span>%d devices</span></h2>%s</div>'
                 % (ACC[cls], cls.upper(), n, "".join(blocks)))

CSS = """
*{box-sizing:border-box}
body{margin:0;background:#0d1014;color:#c9d2de;font-family:system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
.wrap{max-width:1500px;margin:0 auto;padding:0 20px 80px}
header{position:sticky;top:0;z-index:20;background:rgba(13,16,20,.94);backdrop-filter:blur(10px);
 border-bottom:1px solid #252c37;padding:14px 20px}
.hin{max-width:1500px;margin:0 auto;display:flex;align-items:center;gap:10px;flex-wrap:wrap}
h1{font:600 17px/1 ui-monospace,Consolas,monospace;letter-spacing:.1em;margin:0;text-transform:uppercase}
.prog{font:600 15px/1 ui-monospace,monospace;color:#e3b53c;font-variant-numeric:tabular-nums}
button{font-family:ui-monospace,Consolas,monospace;font-size:11px;letter-spacing:.06em;background:#11151b;
 color:#8a94a3;border:1px solid #252c37;border-radius:6px;padding:5px 11px;cursor:pointer}
button:hover{color:#c9d2de;border-color:#e3b53c}
#msg{font-family:ui-monospace,monospace;font-size:11px;color:#7ee081;opacity:0;transition:.2s}
#msg.on{opacity:1} #msg.warn{color:#ff5d6c}
.note{flex-basis:100%;font-size:11.5px;color:#5d6673;line-height:1.5;margin-top:2px}
.pane{margin-top:26px}
h2{font:600 14px/1 ui-monospace,Consolas,monospace;letter-spacing:.16em;color:var(--acc);margin:0 0 4px;
 padding-bottom:8px;border-bottom:1px solid #252c37}
h2 span{color:#5d6673;letter-spacing:.05em;font-weight:400;margin-left:8px;font-size:11px}
section{margin-top:16px}
h3{font:600 11px/1 ui-monospace,Consolas,monospace;letter-spacing:.14em;text-transform:uppercase;
 color:#c9d2de;margin:0 0 9px}
h3 span{color:#5d6673;border:1px solid #252c37;border-radius:100px;padding:1px 7px;margin-left:6px;font-size:9px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:10px}
.t{background:linear-gradient(180deg,#161b23,#1b212b);border:1px solid #252c37;border-radius:10px;
 padding:9px;display:flex;flex-direction:column;align-items:center;gap:6px;cursor:pointer;transition:.15s;position:relative}
.t:hover{border-color:#5d6673}
.t.has{border-color:color-mix(in srgb,var(--acc) 55%,#252c37);background:linear-gradient(180deg,#161b23,#1b212b)}
.slot{width:78px;height:78px;border-radius:9px;border:1px dashed #252c37;background:#0d1014;
 display:flex;align-items:center;justify-content:center;overflow:hidden;flex:none}
.t:hover .slot,.t.drop .slot{border-color:var(--acc);background:color-mix(in srgb,var(--acc) 12%,#0d1014)}
.t.has .slot{border-style:solid;border-color:color-mix(in srgb,var(--acc) 45%,#252c37)}
.slot img{width:100%;height:100%;object-fit:contain}
.add{font-family:ui-monospace,monospace;font-size:8.5px;letter-spacing:.06em;color:#5d6673}
.nm{font-size:11.5px;line-height:1.3;text-align:center;color:#c9d2de}
.nm b{font-family:ui-monospace,monospace;font-size:8px;color:#06090d;background:#e3b53c;
 border-radius:3px;padding:1px 3px;font-weight:700;vertical-align:middle}
.id{font-family:ui-monospace,monospace;font-size:8px;color:#5d6673}
.t.has .id{color:var(--acc)}
.rep{margin:12px 20px 0;padding:10px 13px;border-radius:8px;font-size:12px;line-height:1.5;
 border:1px solid #37d3a6;background:rgba(55,211,166,.1);color:#c9d2de}
.rep.warn{border-color:#ff5d6c;background:rgba(255,93,108,.1)}
"""

JS = """
var TOTAL = %TOTAL%;
var ICONOF = %ICONOF%;
var KEY = 'ga_device_icons_by_dev_v1';
var store = {};
try { store = JSON.parse(localStorage.getItem(KEY) || '{}'); } catch (e) { store = {}; }
function save(){ try{ localStorage.setItem(KEY, JSON.stringify(store)); }
  catch(e){ msg('Storage full - Export now, then Clear.', 1); } }
function n(){ return Object.keys(store).length; }
var mt;
function msg(t, w){ var e=document.getElementById('msg'); e.textContent=t;
  e.className = w ? 'warn on' : 'on'; clearTimeout(mt); mt=setTimeout(function(){e.className='';},2600); }
function shrink(file, cb){
  var fr=new FileReader();
  fr.onload=function(){ var im=new Image();
    im.onload=function(){ var S=128,c=document.createElement('canvas'); c.width=S;c.height=S;
      var x=c.getContext('2d'); var r=Math.min(S/im.width,S/im.height);
      var w=Math.round(im.width*r), h=Math.round(im.height*r);
      x.drawImage(im,(S-w)/2,(S-h)/2,w,h); cb(c.toDataURL('image/png')); };
    im.onerror=function(){ msg('Could not read that image',1); };
    im.src=fr.result; };
  fr.readAsDataURL(file);
}
function paint(){
  document.querySelectorAll('.t').forEach(function(t){
    var d=store[t.dataset.dev];
    t.querySelector('.slot').innerHTML = d ? '<img src="'+d+'" alt="">' : '<span class="add">+ image</span>';
    t.classList.toggle('has', !!d);
  });
  document.getElementById('prog').textContent = n()+' / '+TOTAL;
}
function assign(dev, file){ shrink(file, function(u){ store[dev]=u; save(); paint(); msg('Saved #'+dev+' - '+n()+' / '+TOTAL); }); }
document.querySelectorAll('.t').forEach(function(t){
  var inp=document.createElement('input'); inp.type='file'; inp.accept='image/*'; inp.style.display='none';
  t.appendChild(inp);
  t.addEventListener('click', function(){ inp.click(); });
  inp.addEventListener('change', function(){ if(inp.files[0]) assign(t.dataset.dev, inp.files[0]); inp.value=''; });
  t.addEventListener('dragover', function(e){ e.preventDefault(); t.classList.add('drop'); });
  t.addEventListener('dragleave', function(){ t.classList.remove('drop'); });
  t.addEventListener('drop', function(e){ e.preventDefault(); t.classList.remove('drop');
    var f=e.dataTransfer.files&&e.dataTransfer.files[0]; if(f) assign(t.dataset.dev,f); });
  t.addEventListener('mouseenter', function(){ window.__h=t.dataset.dev; });
  t.addEventListener('mouseleave', function(){ if(window.__h===t.dataset.dev) window.__h=null; });
});
document.addEventListener('paste', function(e){
  if(!window.__h) return;
  var it=(e.clipboardData||{}).items||[];
  for(var i=0;i<it.length;i++){ if(it[i].type.indexOf('image')===0){ assign(window.__h, it[i].getAsFile()); e.preventDefault(); return; } }
});
function showText(text){
  var d=document.getElementById('dump'); d.style.display='block';
  var ta=d.querySelector('textarea'); ta.value=text; ta.focus(); ta.select();
  msg('Download blocked - copy this text into a .json file', 1);
}
function fallback(text){
  try {
    var b=new Blob([text],{type:'application/json'});
    var u=URL.createObjectURL(b);
    var a=document.createElement('a'); a.href=u; a.download='device-icons.json';
    document.body.appendChild(a); a.click(); a.remove();
    setTimeout(function(){URL.revokeObjectURL(u);},4000);
    msg('Saved to your Downloads folder'); return;
  } catch(e){}
  showText(text);
}
document.getElementById('exp').addEventListener('click', function(){
  var text = JSON.stringify(store);
  if (window.showSaveFilePicker) {
    window.showSaveFilePicker({ suggestedName:'device-icons.json',
      types:[{description:'JSON', accept:{'application/json':['.json']}}] })
      .then(function(h){ return h.createWritable(); })
      .then(function(w){ return w.write(text).then(function(){ return w.close(); }); })
      .then(function(){ msg('Saved '+n()+' icons'); })
      .catch(function(err){ if(err && err.name==='AbortError') return; fallback(text); });
    return;
  }
  fallback(text);
});
document.getElementById('cop').addEventListener('click', function(){
  var t=JSON.stringify(store);
  if(navigator.clipboard) navigator.clipboard.writeText(t)
    .then(function(){ msg('Copied '+n()+' icons to clipboard'); }, function(){ showText(t); });
  else showText(t);
});
document.getElementById('dumpx').addEventListener('click', function(){
  document.getElementById('dump').style.display='none';
});
document.getElementById('imp').addEventListener('click', function(){ document.getElementById('impf').click(); });
document.getElementById('impf').addEventListener('change', function(e){
  var f=e.target.files[0]; if(!f) return; var fr=new FileReader();
  fr.onload=function(){ try{ ingest(JSON.parse(fr.result)); }
    catch(err){ msg('Not valid JSON',1); } };

  fr.readAsText(f); e.target.value='';
});
document.getElementById('clr').addEventListener('click', function(){
  if(!confirm('Clear all '+n()+' collected images?')) return;
  store={}; save(); paint(); msg('Cleared');
});
// The first collector keyed on icon_id; this one keys on device_id. Detect an old-format
// file and fan each icon out to every device that uses it, so earlier work is not lost.
function ingest(j){
  var keys = (j && typeof j === 'object') ? Object.keys(j) : [];
  var known={}; document.querySelectorAll('.t').forEach(function(t){ known[t.dataset.dev]=1; });
  var byDev=0, byIcon=0, iconSet={};
  Object.keys(ICONOF).forEach(function(d){ iconSet[String(ICONOF[d])]=1; });
  keys.forEach(function(k){ if(known[k]) byDev++; if(iconSet[k]) byIcon++; });
  var imgs = keys.filter(function(k){ return typeof j[k]==='string' && j[k].indexOf('data:image')===0; }).length;

  if (!keys.length) { return report('That file contained no entries at all ({}). '
      + 'The old Export button was blocked by the sandbox, so it very likely never wrote your data. '
      + 'Recover it with the console snippet instead.', 1); }
  if (!imgs) { return report('File has ' + keys.length + ' entries but none are images '
      + '(expected values starting "data:image"). Wrong file?', 1); }

  var added=0, mode;
  if (byDev >= byIcon) {
    mode = 'device-id';
    keys.forEach(function(k){ if(known[k]){ store[k]=j[k]; added++; } });
  } else {
    mode = 'icon-id';
    Object.keys(ICONOF).forEach(function(dev){
      var ic=String(ICONOF[dev]);
      if(j[ic] && !store[dev]){ store[dev]=j[ic]; added++; }
    });
  }
  save(); paint();
  report('Read ' + keys.length + ' entries (' + imgs + ' images). Detected ' + mode
    + ' keys — matched ' + byDev + ' device ids, ' + byIcon + ' icon ids. '
    + 'Filled ' + added + ' tiles. Now at ' + n() + ' / ' + TOTAL + '.', added ? 0 : 1);
}
// import feedback must persist - it is the only thing telling you whether it worked
function report(t, warn){
  var d=document.getElementById('rep');
  d.style.display='block';
  d.className = warn ? 'rep warn' : 'rep';
  d.textContent = t;
}
paint();
"""

HTML = ('<!doctype html><html><head><meta charset="utf-8">'
        '<title>GA \u2014 Device Icon Collector</title><style>%s</style></head><body>'
        '<header><div class="hin"><h1>Device Icon Collector</h1>'
        '<span class="prog" id="prog">0 / %d</span>'
        '<button id="exp">Export JSON</button><button id="imp">Import</button><button id="clr">Clear</button>'
        '<input type="file" id="impf" accept="application/json" style="display:none">'
        '<span id="msg"></span><button id="cop">Copy to clipboard</button>'
        '<span class="note">One tile per device, keyed by <b>device id</b> so every device gets its own image. '
        'Click a tile, drag an image onto it, or paste while hovering. Saved in this browser as you go &mdash; '
        '<b>Export JSON</b> when finished (it lands in your Downloads folder) and send me the file. '
        'Temporary tool; delete when done.</span></div></header>'
        '<div id="rep" class="rep" style="display:none"></div>'
        '<div id="dump" style="display:none;padding:12px 20px;border-bottom:1px solid #252c37">'
        '<button id="dumpx" style="float:right">close</button>'
        '<p style="margin:0 0 6px;font-size:11.5px;color:#8a94a3">Select all, copy, and save as <b>device-icons.json</b></p>'
        '<textarea style="width:100%%;height:130px;background:#0d1014;color:#c9d2de;border:1px solid #252c37;'
        'border-radius:6px;font-family:ui-monospace,monospace;font-size:10px"></textarea></div>'
        '<div class="wrap">%s</div><script>%s</script></body></html>'
        ) % (CSS, total, "".join(panes), JS.replace('%TOTAL%', str(total)).replace('%ICONOF%', json.dumps(ICONOF)))

out = r"E:\GA_LOCAL\Repo\docs\claude\theorycraft-console\device-icons.html"
open(out, "w", encoding="utf-8").write(HTML)
print("wrote", out, len(HTML), "bytes;", total, "devices")
