// ---------------------------------------------------------------------------
// Temporary icon-collection tool (loadout page).
//
// Devices are keyed by asm_data_set_items.icon_id, NOT by name: 128 devices share
// 105 distinct icon ids, so assigning one device fills every sibling that uses the
// same icon. Images are downscaled to 128px PNG and held in localStorage, so the
// job can be done over several sittings. Export writes device-icons.json
// ({ icon_id: dataURL }) which the generator turns into baked-in icons.
// ---------------------------------------------------------------------------
(function () {
  var MAP = window.__DEVICON__ || {};          // device_id -> icon_id
  var KEY = 'ga_device_icons_v1';
  var store = {};
  try { store = JSON.parse(localStorage.getItem(KEY) || '{}'); } catch (e) { store = {}; }

  var allIcons = {};
  Object.keys(MAP).forEach(function (d) { if (MAP[d]) allIcons[MAP[d]] = 1; });
  var TOTAL = Object.keys(allIcons).length;

  function save() {
    try { localStorage.setItem(KEY, JSON.stringify(store)); }
    catch (e) { bar('Storage full - export now, then Clear.', true); }
  }
  function count() { return Object.keys(store).length; }

  // downscale to keep the payload (and the eventual page) sane
  function shrink(file, cb) {
    var fr = new FileReader();
    fr.onload = function () {
      var img = new Image();
      img.onload = function () {
        var S = 128, c = document.createElement('canvas');
        c.width = S; c.height = S;
        var x = c.getContext('2d');
        x.clearRect(0, 0, S, S);
        var r = Math.min(S / img.width, S / img.height);
        var w = Math.round(img.width * r), h = Math.round(img.height * r);
        x.drawImage(img, (S - w) / 2, (S - h) / 2, w, h);
        cb(c.toDataURL('image/png'));
      };
      img.onerror = function () { bar('Could not read that image.', true); };
      img.src = fr.result;
    };
    fr.readAsDataURL(file);
  }

  function assign(devId, file) {
    var icon = MAP[devId];
    if (!icon) return bar('No icon_id for device ' + devId, true);
    shrink(file, function (durl) {
      store[icon] = durl;
      save(); paint();
      bar('Saved icon ' + icon + ' — ' + count() + ' / ' + TOTAL);
    });
  }

  function paint() {
    document.querySelectorAll('.card[data-dev]').forEach(function (card) {
      var dev = card.dataset.dev, icon = MAP[dev];
      var slot = card.querySelector('.icoslot');
      if (!slot) return;
      var d = icon && store[icon];
      slot.innerHTML = d
        ? '<img src="' + d + '" alt=""><span class="icoid">' + icon + '</span>'
        : '<span class="icoadd">+ image</span><span class="icoid">' + (icon || '?') + '</span>';
      slot.classList.toggle('has', !!d);
    });
    var p = document.getElementById('ico-prog');
    if (p) p.textContent = count() + ' / ' + TOTAL;
  }

  var barT;
  function bar(msg, warn) {
    var el = document.getElementById('ico-msg');
    if (!el) return;
    el.textContent = msg;
    el.className = warn ? 'warn on' : 'on';
    clearTimeout(barT); barT = setTimeout(function () { el.className = ''; }, 2600);
  }

  function wire() {
    document.querySelectorAll('.card[data-dev]').forEach(function (card) {
      var slot = document.createElement('div');
      slot.className = 'icoslot';
      slot.title = 'Click, drop an image here, or paste while hovering';
      card.insertBefore(slot, card.firstChild);
      var inp = document.createElement('input');
      inp.type = 'file'; inp.accept = 'image/*'; inp.style.display = 'none';
      slot.appendChild(inp);
      slot.addEventListener('click', function (e) { e.stopPropagation(); inp.click(); });
      inp.addEventListener('change', function () {
        if (inp.files && inp.files[0]) assign(card.dataset.dev, inp.files[0]);
        inp.value = '';
      });
      slot.addEventListener('dragover', function (e) { e.preventDefault(); slot.classList.add('drop'); });
      slot.addEventListener('dragleave', function () { slot.classList.remove('drop'); });
      slot.addEventListener('drop', function (e) {
        e.preventDefault(); slot.classList.remove('drop');
        var f = e.dataTransfer.files && e.dataTransfer.files[0];
        if (f) assign(card.dataset.dev, f);
      });
      card.addEventListener('mouseenter', function () { window.__icoHover = card.dataset.dev; });
      card.addEventListener('mouseleave', function () {
        if (window.__icoHover === card.dataset.dev) window.__icoHover = null;
      });
    });
    // paste straight onto whichever tile is under the cursor
    document.addEventListener('paste', function (e) {
      if (!window.__icoHover) return;
      var it = (e.clipboardData || {}).items || [];
      for (var i = 0; i < it.length; i++) {
        if (it[i].type.indexOf('image') === 0) {
          assign(window.__icoHover, it[i].getAsFile());
          e.preventDefault(); return;
        }
      }
    });
  }

  function toolbar() {
    var host = document.getElementById('ico-bar');
    if (!host) return;
    host.innerHTML = '<span class="icolab">Icon collector</span>'
      + '<span class="icoprog" id="ico-prog">0 / ' + TOTAL + '</span>'
      + '<button id="ico-exp">Export JSON</button>'
      + '<button id="ico-imp">Import</button>'
      + '<button id="ico-clr">Clear</button>'
      + '<input type="file" id="ico-impf" accept="application/json" style="display:none">'
      + '<span id="ico-msg"></span>'
      + '<span class="iconote">Click a tile’s image slot, or drag an image onto it. '
      + 'Keyed by <b>icon_id</b>, so one image fills every device sharing that icon. '
      + 'Saved in this browser — Export when done (or part-way) and send me the file.</span>';
    document.getElementById('ico-exp').addEventListener('click', function () {
      var blob = new Blob([JSON.stringify(store)], { type: 'application/json' });
      var a = document.createElement('a');
      a.href = URL.createObjectURL(blob);
      a.download = 'device-icons.json';
      a.click();
      setTimeout(function () { URL.revokeObjectURL(a.href); }, 4000);
      bar('Exported ' + count() + ' icons');
    });
    document.getElementById('ico-imp').addEventListener('click', function () {
      document.getElementById('ico-impf').click();
    });
    document.getElementById('ico-impf').addEventListener('change', function (e) {
      var f = e.target.files[0]; if (!f) return;
      var fr = new FileReader();
      fr.onload = function () {
        try {
          var j = JSON.parse(fr.result);
          Object.keys(j).forEach(function (k) { store[k] = j[k]; });
          save(); paint(); bar('Imported — ' + count() + ' / ' + TOTAL);
        } catch (err) { bar('That file is not valid JSON', true); }
      };
      fr.readAsText(f);
      e.target.value = '';
    });
    document.getElementById('ico-clr').addEventListener('click', function () {
      if (!confirm('Clear all ' + count() + ' collected icons from this browser?')) return;
      store = {}; save(); paint(); bar('Cleared');
    });
  }

  wire(); toolbar(); paint();
})();
