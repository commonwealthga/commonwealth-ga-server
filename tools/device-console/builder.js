// Device artwork: one shared base64 map applied to every placeholder. Lives here rather
// than app.js because app.js is emitted before window.__DEVIMG__ exists.
(function () {
  var IMG = window.__DEVIMG__ || {};
  document.querySelectorAll('img.devimg[data-i]').forEach(function (im) {
    var d = IMG[im.dataset.i];
    if (d) im.src = d; else im.remove();
  });
})();

(function () {
  var D = window.__TREE__;
  if (!D) return;
  var MAXP = D.maxPoints;
  var curClass = 'Assault';
  var alloc = {};                       // skillId -> points
  var nodeIndex = {};                   // skillId -> {node, grp}
  Object.keys(D.trees).forEach(function (g) {
    D.trees[g].forEach(function (n) { nodeIndex[n.id] = { n: n, g: g }; });
  });

  // ---- armour: 7 identical slots, default all-RRRRRR (what most players run) ----
  var ARM = D.armour || { configs: {}, slots: 7, default: 'RRRRRR' };
  // When a real character is loaded its armour replaces the preset dropdowns: each piece
  // carries its OWN rolled mods from ga_players_inventory, not a representative config.
  var CH = window.__CHARS__ || null;
  var curChar = null, curProfile = null, charArm = null, charGear = [];
  var craftMode = false;    // TheoryCrafter: gear is hand-picked, not loaded from a profile
  var craftSlots = [];      // [{cat, id, vix}] - one entry per equipment slot
  var craftClass = null;    // the class those slots were picked for
  var activeGear = {};      // gear index -> chosen mode
  var activeOrder = [];     // device names, oldest first (decides Newest/Oldest Wins)
  var stackNotes = [], blockedNames = {};
  var playerBuffs = [];     // what active gear is currently buffing the player with
  var armSlots = [];
  for (var i = 0; i < (ARM.slots || 7); i++) armSlots.push(ARM.default);
  function armConfigs() { return Object.keys(ARM.configs).sort(); }

  function renderArmour() {
    var host = document.getElementById('tb-armour');
    if (!host) return;
    if (charArm) {
      var icon0 = ARM.icon ? 'data:image/png;base64,' + ARM.icon : '';
      host.innerHTML = '<div class="armhead"><h3>Armour</h3>'
        + '<span class="armsub">' + charArm.length + ' equipped pieces &middot; real rolls</span></div>'
        + '<div class="armrow">' + charArm.map(function (a) {
            var tip = (a.base ? a.base + '\n' : '') + (a.groups || []).join('\n');
            return '<div class="armslot real"><span class="armnum">' + esc(a.slot) + '</span>'
              + (icon0 ? '<img src="' + icon0 + '" alt="">' : '')
              + '<span class="armsig" title="' + esc(tip) + '">' + esc(String(a.sig).toLowerCase()) + '</span></div>';
          }).join('') + '</div>';
      return;
    }
    var icon = ARM.icon ? 'data:image/png;base64,' + ARM.icon : '';
    var quick = armConfigs().map(function (c) {
      return '<button class="armall" data-cfg="' + c + '" title="Set every slot to ' + c + '">' + c.toLowerCase() + '</button>';
    }).join('');
    var slots = armSlots.map(function (cfg, idx) {
      var opts = armConfigs().map(function (c) {
        return '<option value="' + c + '"' + (c === cfg ? ' selected' : '') + '>' + c.toLowerCase() + '</option>';
      }).join('');
      return '<div class="armslot"><span class="armnum">' + (idx + 1) + '</span>'
        + (icon ? '<img src="' + icon + '" alt="">' : '')
        + '<select data-slot="' + idx + '">' + opts + '</select></div>';
    }).join('');
    host.innerHTML = '<div class="armhead"><h3>Armour</h3><span class="armsub">7 slots &middot; body part is cosmetic</span>'
      + '<div class="armquick"><span>set all</span>' + quick + '</div></div>'
      + '<div class="armrow">' + slots + '</div>';
    host.querySelectorAll('select').forEach(function (s) {
      s.addEventListener('change', function () { armSlots[+s.dataset.slot] = s.value; renderSheet(); });
    });
    host.querySelectorAll('.armall').forEach(function (b) {
      b.addEventListener('click', function () {
        for (var i = 0; i < armSlots.length; i++) armSlots[i] = b.dataset.cfg;
        renderArmour(); renderSheet();
      });
    });
  }

  // ---------------- MY CHARACTER: load a real saved build ----------------------
  function charById(id) {
    var f = null;
    (CH ? CH.chars : []).forEach(function (c) { if (String(c.id) === String(id)) f = c; });
    return f;
  }
  function renderChar() {
    var host = document.getElementById('tb-char');
    if (!host || !CH || !CH.chars.length) return;
    var tabs = CH.chars.map(function (c) {
      return '<button class="chbtn' + (curChar === String(c.id) ? ' active' : '') + '" data-ch="' + c.id
        + '">' + esc(c.cls) + '</button>';
    }).join('');
    var profs = '';
    if (curChar) {
      var c = charById(curChar);
      profs = '<span class="chsep">profile</span>' + Object.keys(c.profiles).sort().map(function (pid) {
        return '<button class="chprof' + (curProfile === pid ? ' active' : '') + '" data-p="' + pid + '"'
          + (String(c.current) === pid ? ' title="in use in game"' : '') + '>' + pid
          + (String(c.current) === pid ? '<i>&#9733;</i>' : '') + '</button>';
      }).join('');
    }
    host.innerHTML = '<div class="chhead"><h3>My Character</h3>'
      + '<span class="chname">' + esc(CH.name) + '</span>' + tabs + profs
      + (curChar ? '<button class="chclear" id="ch-clear">clear</button>' : '')
      + '</div><p class="chnote">' + (curChar
          ? 'Loaded from the live database &mdash; skills, equipped devices and armour are this profile&rsquo;s '
            + 'actual saved build, with each item&rsquo;s own rolled mods. Edit the tree to explore variations; '
            + '<b>clear</b> returns to a blank build.'
          : 'Pick a character to load its real saved build &mdash; skills, gear and armour &mdash; from the database.')
      + '</p>';
    host.querySelectorAll('.chbtn').forEach(function (b) {
      b.addEventListener('click', function () { loadChar(b.dataset.ch, null); });
    });
    host.querySelectorAll('.chprof').forEach(function (b) {
      b.addEventListener('click', function () { loadChar(curChar, b.dataset.p); });
    });
    var cl = document.getElementById('ch-clear');
    if (cl) cl.addEventListener('click', function () {
      curChar = null; curProfile = null; charArm = null; charGear = [];
      activeGear = {}; activeOrder = []; stackNotes = []; blockedNames = {};
      alloc = {}; resetArm(); render();
    });
  }
  function loadChar(id, pid) {
    var c = charById(id); if (!c) return;
    craftMode = false;
    var keys = Object.keys(c.profiles).sort();
    pid = (pid && c.profiles[pid]) ? pid : (c.profiles[String(c.current)] ? String(c.current) : keys[0]);
    var p = c.profiles[pid];
    curChar = String(id); curProfile = pid;
    curClass = c.cls;
    document.querySelectorAll('.tb-class').forEach(function (x) {
      x.classList.toggle('active', x.dataset.cls === curClass);
    });
    alloc = {};
    p.skills.forEach(function (sid) { if (nodeIndex[sid]) alloc[sid] = 1; });
    charArm = (p.armour && p.armour.length) ? p.armour : null;
    charGear = p.devices || [];
    activeGear = {}; activeOrder = []; stackNotes = []; blockedNames = {};
    render();
  }
  function resetArm() {
    armSlots = [];
    for (var i = 0; i < (ARM.slots || 7); i++) armSlots.push(ARM.default);
  }

  // ---------------- equipped gear: off / primary / alt --------------------------
  // A weapon fires primary OR alt, never both, so the control is tri-state rather than a
  // checkbox. Devices with only one mode just toggle on and off.
  function dev0(g) { return (window.__DEVMODEL__ || {})[String(g.id)] || {}; }
  function modesOf(g) {
    var dev = (window.__DEVMODEL__ || {})[String(g.id)];
    var out = [];
    (dev && dev.modes || []).forEach(function (m) {
      // BLOCK counts as a selectable mode: on a melee weapon with right_click_behavior 894
      // holding block IS the right mouse button, so it is that weapon's second mode.
      if ((m.kind === 'PRI' || m.kind === 'ALT' || m.kind === 'BLOCK') && out.indexOf(m.kind) < 0) out.push(m.kind);
    });
    return out.length ? out : ['PRI'];
  }
  function cycle(g, cur) {
    var ms = modesOf(g);
    if (!cur) return ms[0];
    if (ms.length < 2) return null;          // single mode: the tile is a plain on/off
    var i = ms.indexOf(cur);
    return ms[(i + 1) % ms.length];          // multi-mode: wrap pri -> alt -> pri, never off
  }
  var MODELAB = { PRI: 'primary', ALT: 'alt', BLOCK: 'block' };
  // You carry a melee, a ranged and a specialty weapon but only ONE is in your hands at a
  // time - switching to another puts the previous one away. Not a heuristic; the game does
  // not let you have two of the three out at once.
  var WEAPON_SLOTS = { Melee: 1, Ranged: 1, Specialty: 1 };

  // skills the build allocates that actually touch this device
  function skillsFor(g) {
    var fx = (window.__DEVFX__ || {})[String(g.id)] || [];
    var seen = {}, out = [];
    fx.forEach(function (e) {
      if (!alloc[e.sid] || seen[e.sid]) return;
      seen[e.sid] = 1;
      out.push(e);
    });
    return out;
  }

  // Resolved stat chips for one equipped item - base -> modified, with the contributors
  // in the tooltip. Same rendering the standalone bench used to do.
  // Which repair arms are switched on right now - they buff any active turret/drone.
  function activeArms() {
    var out = [];
    Object.keys(activeGear).forEach(function (i) {
      var g = charGear[+i]; if (!g || !/Repair Arm|Nanite Repair/.test(g.name)) return;
      var dev = (window.__DEVMODEL__ || {})[String(g.id)];
      if (!dev || !window.GA) return;
      var res = window.GA.resolve({ dev: dev, meta: (window.__DEVMETA__ || {})[String(g.id)] || {},
        ix: (window.__DEVFX__ || {})[String(g.id)] || [], alloc: alloc, situational: true,
        variant: { sig: g.sig, base: g.base, groups: g.groups, nums: g.nums } });
      // only the arm's SELECTED mode counts - the Focused arm is +15% on primary and
      // +40% on alt, so scanning every mode would always report the best one
      var want = activeGear[i];
      var modes = res.modes.filter(function (m) { return !m.kind || m.kind === want; });
      var chips = window.GA.flatChips({ modes: modes });
      out.push({ name: g.name + (want === 'ALT' ? ' (alt)' : ''), chips: chips });
    });
    return out;
  }
  function isPet(g) {
    var m = (window.__DEVMETA__ || {})[String(g.id)];
    return !!(m && m.pet);
  }

  function chipsFor(g, mode) {
    if (!window.GA || !window.GA.deviceChipsHTML) return '';
    var dev = (window.__DEVMODEL__ || {})[String(g.id)];
    if (!dev) return '';
    var sup = isPet(g) ? window.GA.supportFromArms(activeArms()) : null;
    return window.GA.deviceChipsHTML({
      support: sup,
      dev: dev, meta: (window.__DEVMETA__ || {})[String(g.id)] || {},
      ix: (window.__DEVFX__ || {})[String(g.id)] || [], alloc: alloc,
      mode: mode, situational: !!mode, buffs: playerBuffs,
      variant: { sig: g.sig, base: g.base, groups: g.groups, nums: g.nums }
    });
  }

  function renderGear() {
    if (window.GA) window.GA.lastDrain = 0;   // recomputed by the chips below
    var host = document.getElementById('tb-gear');
    if (!host) return;
    if (!charGear.length) { host.innerHTML = ''; return; }
    var rows = charGear.map(function (g, i) {
      var mode = activeGear[i] || null;
      var ms = modesOf(g);
      var sk = skillsFor(g);
      var tip = (g.base ? g.base + '\n' : '') + (g.groups || []).join('\n');
      // a scope ALT reads better as "scoped" than "alt" - you are still firing the same gun
      var isZoom = (dev0(g).modes || []).some(function (m) { return m.kind === 'ALT' && m.zoom; });
      var lab = mode ? (ms.length > 1 ? (mode === 'ALT' && isZoom ? 'scoped' : MODELAB[mode]) : 'on') : 'off';
      return '<div class="gitem' + (mode ? ' on' : '') + '" data-i="' + i + '"'
        + ' title="' + (ms.length > 1
            ? (isZoom ? 'Click to scope in and out. Scoping does not replace the shot - the '
                      + 'primary damage still applies, with the scope effects on top. '
                      + 'Use × to switch it off.'
                      : 'Click to switch between primary and alt. Use × to switch it off.')
            : 'Click to switch on or off.') + '">'
        + ((window.__DEVIMG__ || {})[String(g.id)]
            ? '<img class="gimg" src="' + window.__DEVIMG__[String(g.id)] + '" alt="">' : '')
        + '<span class="gcat' + (WEAPON_SLOTS[g.cat] ? ' wep' : '') + '"'
        + (WEAPON_SLOTS[g.cat] ? ' title="Melee, Ranged and Specialty share your hands - only one can be out at a time"' : '')
        + '>' + esc(g.cat || '') + '</span>'
        + '<span class="gname">' + esc(g.name) + (g.oc ? ' <span class="oc">OC</span>' : '') + '</span>'
        + '<span class="gsig" title="' + esc(tip) + '">' + esc(String(g.sig).toLowerCase()) + '</span>'
        + '<span class="gtog' + (mode === 'ALT' ? ' alt' : '') + '">' + lab + '</span>'
        + (mode && ms.length > 1 ? '<button class="goff" data-off="' + i
            + '" title="Switch this device off">&times;</button>' : '')
        + (sk.length
            ? '<span class="gskills">' + sk.map(function (e) {
                return '<span class="gsk" title="' + esc(e.detail + '  -  ' + e.how) + '">'
                  + esc(e.skill) + '</span>';
              }).join('') + '</span>'
            : '<span class="gskills"><span class="gsknone">no allocated skill affects this</span></span>')
        + '<div class="gchips">' + chipsFor(g, mode) + '</div>'
        + '</div>';
    }).join('');
    var seenNote = {}, uniq = stackNotes.filter(function (n) {
      var k = n.win + '|' + n.lost.join(',');
      if (seenNote[k]) return false;
      seenNote[k] = 1; return true;
    });
    var note = uniq.length
      ? '<div class="stacknote">' + uniq.map(function (n) {
          return '<b>' + esc(n.lost.join(', ')) + '</b> switched off &mdash; <em>' + esc(n.win)
            + '</em> occupies the same effect category and only one can run '
            + '(<em>' + esc(n.rule) + '</em>).';
        }).join('<br>') + '</div>'
      : '';
    host.innerHTML = '<div class="ghead"><h3>Equipped</h3><span class="gsub">'
      + charGear.length + ' devices &middot; profile ' + esc(curProfile || '')
      + ' &middot; click to activate</span></div>'
      + '<div class="gitems">' + rows + '</div>' + note;
    host.querySelectorAll('.goff').forEach(function (b) {
      b.addEventListener('click', function (ev) {
        ev.stopPropagation();                 // must not also fire the tile's cycle
        var i = +b.dataset.off, nm = charGear[i].name;
        stackNotes = [];
        delete activeGear[i];
        activeOrder = activeOrder.filter(function (x) { return x !== nm; });
        render();
      });
    });
    host.querySelectorAll('.gitem').forEach(function (el) {
      el.addEventListener('click', function () {
        var i = +el.dataset.i, g = charGear[i], nm = g.name;
        var next = cycle(g, activeGear[i] || null);
        if (next) {
          if (!activeGear[i]) { activeOrder.push(nm); stackNotes = []; }   // fresh action, fresh slate
          activeGear[i] = next;
          // holstering the other two weapons
          if (WEAPON_SLOTS[g.cat]) {
            Object.keys(activeGear).forEach(function (k) {
              if (+k === i) return;
              var o = charGear[+k];
              if (o && WEAPON_SLOTS[o.cat]) {
                delete activeGear[k];
                activeOrder = activeOrder.filter(function (x) { return x !== o.name; });
              }
            });
          }
        } else {
          delete activeGear[i];
          activeOrder = activeOrder.filter(function (x) { return x !== nm; });
          stackNotes = [];
        }
        render();
      });
    });
  }

  // "Equip:" passives apply for carrying the device, not for having it switched on - the
  // tooltip says Equip, and Targeting System's +5% Range Damage is live whether or not you
  // are currently using it. So these are collected from every slot, active or not.
  function equipBuffs() {
    var out = [];
    charGear.forEach(function (g) {
      var dev = (window.__DEVMODEL__ || {})[String(g.id)];
      if (!dev) return;
      (dev.modes || []).forEach(function (m) {
        (m.chips || []).forEach(function (c) {
          if (!/^Equip: /.test(c[1])) return;
          var num = c[2];
          if (!num || !num[0]) return;
          var pct = (num[2] === 68 || num[2] === 69);
          var v = num[1];
          if (pct && Math.abs(v) > 0 && Math.abs(v) < 1) v *= 100;
          if (num[2] === 69 || num[2] === 70) v = -v;
          out.push({ p: num[0], v: v, pct: pct ? 1 : 0, src: g.name, kind: 'equip' });
        });
      });
    });
    return out;
  }

  // Everything switched on, run through the game's stacking rules. Anything overridden is
  // switched OFF outright rather than left on and greyed - the game would not have it running.
  function activeContribution() {
    var effects = [], shields = [];
    if (!window.GA || !window.GA.playerEffects) return { effects: effects, shields: shields, notes: [] };
    Object.keys(activeGear).forEach(function (i) {
      var g = charGear[+i]; if (!g) return;
      var dev = (window.__DEVMODEL__ || {})[String(g.id)];
      if (!dev) return;
      var r = window.GA.playerEffects({ dev: dev, meta: (window.__DEVMETA__ || {})[String(g.id)] || {},
        ix: (window.__DEVFX__ || {})[String(g.id)] || [], alloc: alloc, name: g.name,
        mode: activeGear[i],
        variant: { sig: g.sig, base: g.base, groups: g.groups, nums: g.nums } });
      effects = effects.concat(r.effects);
      shields = shields.concat(r.shields);
    });
    var st = window.GA.applyStacking(effects.concat(shields), activeOrder);
    // switch the losers off for real
    var lost = {};
    st.notes.forEach(function (n) { n.lost.forEach(function (l) { lost[l] = 1; }); });
    var changed = false;
    Object.keys(activeGear).forEach(function (i) {
      var g = charGear[+i];
      if (g && lost[g.name]) {
        delete activeGear[i];
        activeOrder = activeOrder.filter(function (x) { return x !== g.name; });
        changed = true;
      }
    });
    if (changed) {
      function keep(x) { return !lost[x.src]; }
      effects = effects.filter(keep); shields = shields.filter(keep);
    }
    return { effects: effects, shields: shields, notes: st.notes };
  }

  // ---------------- TheoryCrafter: build a loadout from nothing -----------------
  // Same slot layout the game gives you: one weapon of each kind, three off-hands, a boost
  // and a jetpack. Nothing is written back to the database - this is a scratch build.
  var SLOTPLAN = [['Melee', 1], ['Ranged', 1], ['Specialty', 1], ['Offhand', 3], ['Boost', 1], ['Jetpack', 1]];

  function blankSlots() {
    var out = [];
    SLOTPLAN.forEach(function (p) {
      for (var i = 0; i < p[1]; i++) out.push({ cat: p[0], id: null, vix: 0 });
    });
    return out;
  }
  // every device of this class that fits the slot
  function devicesFor(cls, cat) {
    var M = window.__DEVMODEL__ || {}, T = window.__DEVMETA__ || {}, out = [];
    Object.keys(M).forEach(function (id) {
      var m = T[id] || {};
      if (m.cls && m.cls !== 'Shared' && m.cls !== cls) return;
      if (M[id].cat !== cat) return;
      out.push({ id: id, name: M[id].name, oc: M[id].oc });
    });
    return out.sort(function (a, b) { return a.name < b.name ? -1 : 1; });
  }
  // turn the slot picks into the same shape a loaded character produces
  function craftToGear() {
    charGear = [];
    craftSlots.forEach(function (sl) {
      if (!sl.id) return;
      var dev = (window.__DEVMODEL__ || {})[sl.id];
      if (!dev) return;
      var v = (dev.variants || [])[sl.vix] || (dev.variants || [])[0] || { sig: '\u2014', base: null, groups: [], nums: [] };
      charGear.push({ slot: 0, id: +sl.id, name: dev.name, oc: dev.oc, cat: dev.cat,
                      sig: v.sig, base: v.base, groups: v.groups, nums: v.nums });
    });
  }
  function renderCraft() {
    var host = document.getElementById('tc-build');
    if (!host) return;
    if (!craftSlots.length) craftSlots = blankSlots();
    var rows = craftSlots.map(function (sl, i) {
      var pool = devicesFor(curClass, sl.cat).filter(function (d) {
        // you cannot carry the same off-hand three times
        return String(sl.id) === String(d.id)
          || !craftSlots.some(function (o, k) { return k !== i && String(o.id) === String(d.id); });
      });
      var opts = '<option value="">&mdash; empty &mdash;</option>' + pool.map(function (d) {
        return '<option value="' + d.id + '"' + (String(sl.id) === String(d.id) ? ' selected' : '') + '>'
          + esc(d.name) + (d.oc ? ' (OC)' : '') + '</option>';
      }).join('');
      var rolls = '';
      if (sl.id) {
        var dev = (window.__DEVMODEL__ || {})[sl.id] || {};
        var vs = dev.variants || [];
        if (vs.length > 1) {
          rolls = '<select class="tcroll" data-i="' + i + '">' + vs.map(function (v, k) {
            return '<option value="' + k + '"' + (k === sl.vix ? ' selected' : '') + '>'
              + esc(String(v.sig).toLowerCase()) + '</option>';
          }).join('') + '</select>';
        } else if (vs.length === 1) {
          rolls = '<span class="tcsig">' + esc(String(vs[0].sig).toLowerCase()) + '</span>';
        }
      }
      return '<div class="tcslot"><span class="tccat">' + esc(sl.cat) + '</span>'
        + '<select class="tcdev" data-i="' + i + '">' + opts + '</select>' + rolls + '</div>';
    }).join('');
    var filled = craftSlots.filter(function (s2) { return s2.id; }).length;
    host.innerHTML = '<div class="chhead"><h3>TheoryCrafter</h3>'
      + '<span class="chname">' + esc(curClass) + '</span>'
      + '<span class="tccount">' + filled + ' / ' + craftSlots.length + ' slots</span>'
      + '<button class="chclear" id="tc-clear">clear</button></div>'
      + '<p class="chnote">Devices are filtered to what this class can equip, and each keeps its '
      + 'own mod roll. Melee, Ranged and Specialty share your hands &mdash; only one is out at a time.</p>'
      + '<div class="tcslots">' + rows + '</div>';
    host.querySelectorAll('.tcdev').forEach(function (sel) {
      sel.addEventListener('change', function () {
        var sl = craftSlots[+sel.dataset.i];
        sl.id = sel.value || null; sl.vix = 0;
        activeGear = {}; activeOrder = [];
        craftToGear(); render();
      });
    });
    host.querySelectorAll('.tcroll').forEach(function (sel) {
      sel.addEventListener('change', function () {
        craftSlots[+sel.dataset.i].vix = +sel.value;
        craftToGear(); render();
      });
    });
    var cl = document.getElementById('tc-clear');
    if (cl) cl.addEventListener('click', function () {
      craftSlots = blankSlots(); craftClass = curClass;
      alloc = {}; activeGear = {}; activeOrder = [];
      craftToGear(); render();
    });
  }

  function treesFor(cls) { return D.classes[cls]; }  function treesFor(cls) { return D.classes[cls]; }  function treesFor(cls) { return D.classes[cls]; }
  function spentIn(g) {
    var s = 0;
    (D.trees[g] || []).forEach(function (n) { s += alloc[n.id] || 0; });
    return s;
  }
  function total() { var s = 0; Object.keys(alloc).forEach(function (k) { s += alloc[k]; }); return s; }

  // A node is legal if the OTHER points in its tree meet its tier gate,
  // and its prerequisite skill has enough points.
  function legal(n, g) {
    if ((spentIn(g) - (alloc[n.id] || 0)) < n.gp) return false;
    if (n.psk && (alloc[n.psk] || 0) < (n.psp || 1)) return false;
    return true;
  }
  function why(n, g) {
    if (total() >= MAXP && !alloc[n.id]) return 'No points left (' + MAXP + ' max)';
    var have = spentIn(g);
    if (have < n.gp) return 'Needs ' + n.gp + ' points in ' + D.names[g] + ' (you have ' + have + ')';
    if (n.psk && (alloc[n.psk] || 0) < (n.psp || 1)) {
      var p = nodeIndex[n.psk];
      return 'Requires ' + (p ? p.n.name : 'skill ' + n.psk);
    }
    return '';
  }
  // remove anything that became illegal after a deallocation
  function prune() {
    var changed = true;
    while (changed) {
      changed = false;
      Object.keys(alloc).forEach(function (id) {
        if (!alloc[id]) return;
        var e = nodeIndex[id];
        if (e && !legal(e.n, e.g)) { delete alloc[id]; changed = true; }
      });
    }
  }
  function add(n, g) {
    if (alloc[n.id]) return;
    if (total() >= MAXP) return flash('Point limit reached — ' + MAXP + ' maximum');
    if (spentIn(g) < n.gp) return flash(why(n, g));
    if (n.psk && (alloc[n.psk] || 0) < (n.psp || 1)) return flash(why(n, g));
    alloc[n.id] = 1; render();
  }
  function remove(n) {
    if (!alloc[n.id]) return;
    delete alloc[n.id]; prune(); render();
  }
  var flashT;
  function flash(msg) {
    var el = document.getElementById('tb-msg');
    if (!el || !msg) return;
    el.textContent = msg; el.classList.add('on');
    clearTimeout(flashT); flashT = setTimeout(function () { el.classList.remove('on'); }, 2200);
  }

  function iconFor(n) {
    var b = D.icons[String(n.icon)];
    return b ? 'data:image/png;base64,' + b : '';
  }
  // effects are structured objects — render one as readable text
  function fxText(f) {
    var v = (f.neg ? '-' : '+') + f.v + (f.pct ? '%' : '');
    return f.n + ' ' + v
      + (f.rskn ? '  (' + f.rskn + ')' : '')
      + (f.life ? '  [' + f.life + 's]' : '')
      + (f.kind && f.kind !== 'passive' ? '  [' + f.kind + ']' : '');
  }

  function render() {
    var host = document.getElementById('tb-trees');
    if (!host) return;
    host.innerHTML = '';
    treesFor(curClass).forEach(function (g) {
      var list = D.trees[g] || [];
      var col = document.createElement('div');
      col.className = 'tcol';
      col.innerHTML = '<div class="thead"><h3>' + D.names[g] + '</h3><span class="tpts">' + spentIn(g) + '</span></div>';
      var grid = document.createElement('div');
      grid.className = 'tgrid';
      // Centre the tree on the columns it actually uses (trees don't all fill x=0..4).
      var xs = list.map(function (n) { return n.x; });
      var minX = Math.min.apply(null, xs), maxX = Math.max.apply(null, xs);
      var used = (maxX - minX + 1);
      // Size the grid to exactly the columns in use and let `margin:0 auto` centre it.
      var span = (used - 1) * 56 + 44;
      var offX = -minX * 56;
      grid.style.width = span + 'px';
      // connector lines (prereq skill -> dependent) inside this tree
      var svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
      svg.setAttribute('class', 'tlines');
      list.forEach(function (n) {
        if (!n.psk) return;
        var p = list.filter(function (m) { return m.id === n.psk; })[0];
        if (!p) return;
        var ln = document.createElementNS('http://www.w3.org/2000/svg', 'line');
        ln.setAttribute('x1', p.x * 56 + 22 + offX); ln.setAttribute('y1', p.y * 56 + 22);
        ln.setAttribute('x2', n.x * 56 + 22 + offX); ln.setAttribute('y2', n.y * 56 + 22);
        ln.setAttribute('class', alloc[n.id] ? 'on' : '');
        svg.appendChild(ln);
      });
      grid.appendChild(svg);
      list.forEach(function (n) {
        var on = !!alloc[n.id];
        var ok = legal(n, g) && (on || total() < MAXP);
        var d = document.createElement('button');
        d.className = 'tnode' + (on ? ' on' : '') + (ok ? '' : ' locked');
        d.style.left = (n.x * 56 + offX) + 'px';
        d.style.top = (n.y * 56) + 'px';
        var ic = iconFor(n);
        d.innerHTML = (ic ? '<img src="' + ic + '" alt="">' : '<span class="noic">?</span>')
          + '<span class="rank">' + (on ? '1/1' : (n.gp ? n.gp : '')) + '</span>';
        var tip = n.name + (n.gp ? '  [' + n.gp + ' pts in ' + D.names[g] + ']' : '')
          + (n.desc ? '\n\n' + n.desc : '')
          + (n.fx && n.fx.length ? '\n\n' + n.fx.map(fxText).join('\n') : '')
          + (ok ? '' : '\n\n⚠ ' + why(n, g));
        d.title = tip;
        d.addEventListener('click', function (ev) {
          if (ev.shiftKey || ev.altKey) remove(n); else add(n, g);
        });
        d.addEventListener('contextmenu', function (ev) { ev.preventDefault(); remove(n); });
        grid.appendChild(d);
      });
      col.appendChild(grid);
      host.appendChild(col);
    });
    var c = document.getElementById('tb-count');
    if (c) { c.textContent = total() + ' / ' + MAXP; c.className = total() >= MAXP ? 'full' : ''; }
    if (craftMode) renderCraft(); else renderChar();
    renderArmour();
    renderSheet();   // computes the stacking result
    renderGear();    // ...which renderGear needs, so it runs after
  }

  // Public handle so the test bench (and later the full build simulator) can read the
  // current allocation and drive the class selector.
  window.GA_BUILD = {
    alloc: function () { return alloc; },
    cls: function () { return curClass; },
    armour: function () { return armSlots.slice(); },
    render: function () { render(); },
    setClass: function (c) {
      if (!c || c === curClass || !D.classes[c]) return;
      document.querySelectorAll('.tb-class').forEach(function (x) {
        x.classList.toggle('active', x.dataset.cls === c);
      });
      curClass = c; alloc = {}; render();
    }
  };

  // ---------------- MY PLAYER: aggregate the allocated skills ----------------
  // Group by property; flat (calc ADD/SUB) and percent (PERC_INC/DEC) are summed
  // separately because the game applies them at different layers.
  var HEALTHP = [51, 211, 304, 306, 390, 412, 339, 366];
  var POWERP  = [243, 244, 255, 242, 285, 322];
  var DEFP    = [155,156,157,158,159,160,163,168,217,218,219,233,235,266,324,328,371,316,386];
  // Protection is stored FLAT, never as a percentage: every skill effect on these props uses
  // calc "Add (+)" / "Subtract (-)", never "Increase (+%)". It becomes a percentage only via
  // CalcProtection: mitigation = protection / attacker's attack rating. At the reference
  // rating of 100 the two numbers coincide, which is why some tooltips (Aegis Armament,
  // "an additional 25% Physical resistance") are written as percentages.
  var PROTP   = [155,156,157,158,159,160,163,168,217,218,219,233,235,266,324,328,371];
  var REF_RATING = 100;
  var OFFP    = [65,212,214,321,336,350,372,373,374,375,388,389,361,362,363,364,369,370,376,385,210,330,215,232];
  var GRP = [
    ['Health',  function (p) { return HEALTHP.indexOf(p) >= 0; }],
    ['Power',   function (p) { return POWERP.indexOf(p) >= 0; }],
    ['Defence', function (p) { return DEFP.indexOf(p) >= 0; }],
    ['Offence', function (p) { return OFFP.indexOf(p) >= 0; }],
    ['Utility', function () { return true; }]
  ];
  function esc(s) { return String(s).replace(/[&<>"]/g, function (c) { return ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' })[c]; }); }
  function fmt(v) { v = Math.round(v * 100) / 100; return (v > 0 ? '+' : '') + v; }

  function renderSheet() {
    var host = document.getElementById('tb-sheet');
    if (!host) return;
    var picked = Object.keys(alloc).map(function (id) { return nodeIndex[id]; }).filter(Boolean);
    // stat -> {flat, pct, srcs:[{skill,tree,val,pct,kind,life,dev}]}
    var stats = {};
    // Seed with Base Player Stats — identical for every class (archetype bot + the
    // un-unequippable Human Base Attributes). Skills add on top of these.
    // Props 235 EMP-Stun / 328 EMP-Burn sit at 1000 on every player = flat immunity to the
    // machine-only EMP channels. Kept in the data, hidden here so the sheet isn't confusing.
    var HIDE = [235, 328];
    (D.base || []).forEach(function (b) {
      if (HIDE.indexOf(b.p) >= 0) return;
      var key = b.p + '|' + b.pct + '|0';
      var st = stats[key] || (stats[key] = { p: b.p, name: b.n, pct: b.pct, rsk: 0, scope: '', total: 0, srcs: [] });
      st.total += b.v;
      st.srcs.push({ skill: b.src, tree: 'BASE', val: b.v, kind: 'passive', life: 0, dev: [], base: 1, layer: 'base' });
    });
    // Armour = the ITEM layer. Percentages here multiply with the skill layer, they don't add
    // (verified: 1300 x 1.70 armour x 1.25 skills = 2762, the in-game value).
    var armAgg = {};
    if (charArm) {
      // real equipped pieces: nums are [prop, signedTotal, calc, name] per mod group
      charArm.forEach(function (piece) {
        (piece.nums || []).forEach(function (f) {
          var prop = f[0], v = f[1], calc = f[2], nm = f[3];
          var pct = (calc === 68 || calc === 69) ? 1 : 0;
          if (calc === 69 || calc === 70) v = -v;
          var k = prop + '|' + pct;
          if (!armAgg[k]) armAgg[k] = { p: prop, n: nm, pct: pct, v: 0, slots: 0 };
          armAgg[k].v += v; armAgg[k].slots++;
        });
      });
    } else {
      armSlots.forEach(function (cfg) {
        (ARM.configs[cfg] || []).forEach(function (f) {
          var k = f.p + '|' + f.pct;
          if (!armAgg[k]) armAgg[k] = { p: f.p, n: f.n, pct: f.pct, v: 0, slots: 0 };
          armAgg[k].v += f.v; armAgg[k].slots++;
        });
      });
    }
    Object.keys(armAgg).forEach(function (k) {
      var a = armAgg[k];
      var key = a.p + '|' + a.pct + '|0';
      var st = stats[key] || (stats[key] = { p: a.p, name: a.n, pct: a.pct, rsk: 0, scope: '', total: 0, srcs: [] });
      st.total += a.v;
      st.srcs.push({ skill: 'Armour (' + a.slots + ' slot' + (a.slots > 1 ? 's' : '') + ')', tree: 'ARMOUR',
                     val: a.v, kind: 'passive', life: 0, dev: [], armour: 1, layer: 'item' });
    });
    // ---- ACTIVE DEVICE layer -------------------------------------------------
    // Whatever is on the bench and switched ON contributes to the player: the device's own
    // buffs, plus any skill gated on it being up (Aegis Armament fires while a Personal
    // Shield effect is active). Shield pools are tracked separately so a covered damage
    // type can be shown as absorbed rather than as a plain number.
    var contrib = activeContribution();
    if (contrib.notes.length) stackNotes = contrib.notes;   // sticky: see note below
    // percentage buffs feed back into every OTHER device's numbers, not just this summary
    var eqb = equipBuffs();
    playerBuffs = eqb.filter(function (f) { return f.pct; })
      .concat(contrib.effects.filter(function (f) { return f.pct; })
        .map(function (f) { return { p: f.p, v: f.v, pct: 1, src: f.src, kind: f.kind }; }));
    var act = contrib.effects.length || contrib.shields.length
      ? { effects: contrib.effects, shields: contrib.shields,
          dev: Object.keys(activeGear).map(function (i) { return charGear[+i].name; }).join(', ') }
      : null;
    // equip passives are a layer of their own on the sheet
    eqb.forEach(function (f) {
      var nm = (window.GA && window.GA.statName) ? window.GA.statName(f.p) : null;
      if (!nm) return;
      var key = f.p + '|' + (f.pct ? 1 : 0) + '|0';
      var st = stats[key] || (stats[key] = { p: f.p, name: nm, pct: f.pct ? 1 : 0,
                                             rsk: 0, scope: '', total: 0, srcs: [] });
      st.total += f.v;
      st.srcs.push({ skill: f.src, tree: 'EQUIP', val: f.v, kind: 'equipped', life: 0,
                     dev: [], layer: 'equip' });
    });
    var shieldBy = {}, liveNow = {};
    if (act) {
      act.effects.forEach(function (f) {
        liveNow[f.src + '|' + f.p] = 1;      // this skill's conditional effect is firing
        var key = f.p + '|' + (f.pct ? 1 : 0) + '|0';
        var st = stats[key] || (stats[key] = { p: f.p, name: f.name, pct: f.pct ? 1 : 0,
                                               rsk: 0, scope: '', total: 0, srcs: [] });
        st.total += f.v;
        st.srcs.push({ skill: f.src, tree: 'ACTIVE', val: f.v, kind: f.kind, life: f.life || 0,
                       dev: [], layer: 'active', active: 1 });
      });
      act.shields.forEach(function (sh) {
        sh.types.forEach(function (p) { shieldBy[p] = sh; });
      });
    }
    picked.forEach(function (e) {
      (e.n.fx || []).forEach(function (f) {
        // Key on the GATE too: the same property gated to different device classes is a
        // different stat and must not be summed (Shield Strength's +40% lifetime applies to
        // Shields; Point Tank's +30% applies to Boosts — they are never +70% on anything).
        var key = f.p + '|' + f.pct + '|' + (f.rsk || 0);
        var st = stats[key] || (stats[key] = {
          p: f.p, name: f.n, pct: f.pct, rsk: f.rsk || 0, scope: f.rskn || '', total: 0, srcs: []
        });
        var v = f.neg ? -f.v : f.v;
        // Conditional / reactive / on-hit skill effects are NOT always-on, so they must not
        // sit in the passive total. They are counted only while their trigger is live, and the
        // ACTIVE-device layer above supplies them then (Aegis Armament while a shield is up).
        // Counting both would double them.
        var cond = (f.kind && f.kind !== 'passive');
        if (!cond) st.total += v;
        else st.cond = 1;
        // devices: gated effects reach exactly their gate's devices; ungated fall back to
        // the parent skill's semantic device list.
        var dev = (f.rsk && D.devbyskill[String(f.rsk)]) ? D.devbyskill[String(f.rsk)] : (e.n.dev || []);
        if (cond && liveNow[e.n.name + '|' + f.p]) return;   // already shown as ACTIVE
        st.srcs.push({ skill: e.n.name, tree: D.names[e.g], val: v, kind: f.kind, life: f.life,
                       dev: dev, dormant: cond ? 1 : 0 });
      });
    });
    var keys = Object.keys(stats);

    // ---- derived headline totals -------------------------------------------
    // Layers multiply (TgPawn::ApplyBuff: v1 = base*(1+itemPct), v2 = v1*(1+skillPct)).
    // Skills are one layer today; armour/devices will slot in as their own layer later.
    function baseOf(p) {
      var s = stats[p + '|0|0'];
      if (!s) return 0;
      var v = 0;
      s.srcs.forEach(function (x) { if (x.layer === 'base') v += x.val; });
      return v;
    }
    // percent contributions split by layer, because layers multiply
    function pctFor(list, layer) {
      var sum = 0;
      keys.forEach(function (k) {
        var st = stats[k];
        if (!st.pct || list.indexOf(st.p) < 0) return;
        st.srcs.forEach(function (x) {
          var l = x.layer || 'skill';
          if (l === layer) sum += x.val;
        });
      });
      return sum;
    }
    var baseHP = baseOf(51), basePW = baseOf(243);
    var hpItem = pctFor([412, 390, 304], 'item'), hpSkill = pctFor([412, 390, 304], 'skill');
    var pwItem = pctFor([255, 243], 'item'), pwSkill = pctFor([255, 243], 'skill');
    // game truncates rather than rounds: 1300 x1.70 x1.25 = 2762.5 shows in-game as 2762
    var totHP = Math.floor(baseHP * (1 + hpItem / 100) * (1 + hpSkill / 100));
    var totPW = Math.floor(basePW * (1 + pwItem / 100) * (1 + pwSkill / 100));
    // Anything active that raises the CEILING counts toward sustain (Fashion Boost's Max Power
    // +40). A flat Power restore (prop 243, Power Stim's +140) does not - it refills the pool,
    // it does not enlarge it.
    var actPool = 0, actPoolPct = 0;
    if (act) act.effects.forEach(function (f) { if (f.p === 255) { if (f.pct) actPoolPct += f.v; else actPool += f.v; } });
    eqb.forEach(function (f) { if (f.p === 255) { if (f.pct) actPoolPct += f.v; else actPool += f.v; } });
    // the headline Power figure must be the pool the sustain maths uses, equip layer included
    totPW = Math.floor((totPW + actPool) * (1 + actPoolPct / 100));
    window.__PWPOOL__ = totPW;
    // the POWER tile carries the sustain figure for whatever is currently firing
    // Every active device with a per-shot cost and a refire spends power at once - flying
    // while firing drains both - so these SUM. Taking the maximum meant a jetpack at 23.6/s
    // masked the weapon entirely, and switching the weapon's fire mode changed nothing.
    function activeDrain() {
      var parts = [];
      if (!window.GA || !window.GA.resolve) return parts;
      Object.keys(activeGear).forEach(function (i) {
        var g = charGear[+i]; if (!g) return;
        var dev = (window.__DEVMODEL__ || {})[String(g.id)]; if (!dev) return;
        var r = window.GA.resolve({ dev: dev, meta: (window.__DEVMETA__ || {})[String(g.id)] || {},
          ix: (window.__DEVFX__ || {})[String(g.id)] || [], alloc: alloc, situational: true,
          buffs: playerBuffs, variant: { sig: g.sig, base: g.base, groups: g.groups, nums: g.nums } });
        var scoped = activeGear[i] === 'ALT' && r.modes.some(function (m) { return m.kind === 'ALT' && m.zoom; });
        var best = 0;
        r.modes.forEach(function (m) {
          var want = scoped && m.kind === 'PRI' ? 'PRI' : activeGear[i];
          if (m.kind && activeGear[i] && m.kind !== want && m.kind !== 'SPAWN') return;
          if (!m.power || !m.power.value) return;
          var rf = null;
          m.chips.forEach(function (c) { if (c.prop === 53 && c.base !== null) rf = c.value; });
          if (rf && rf > 0) best = Math.max(best, m.power.value / rf);   // one mode per device
        });
        if (best) parts.push({ name: g.name, rate: best });
      });
      return parts;
    }
    function drainNote() {
      var parts = activeDrain();
      if (!parts.length) return '';
      var d = 0;
      parts.forEach(function (p) { d += p.rate; });
      var secs = Math.round((window.__PWPOOL__ || 0) / d * 10) / 10;
      var tip = 'Everything active spends power at once:\n'
        + parts.map(function (p) { return '  ' + p.name + '  ' + (Math.round(p.rate * 10) / 10) + '/s'; }).join('\n')
        + (parts.length > 1 ? '\n  = ' + (Math.round(d * 10) / 10) + '/s' : '')
        + '\n\nPassive regeneration does not run while power is being spent.';
      return '<span class="totdrain" title="' + esc(tip) + '">&minus;'
        + (Math.round(d * 10) / 10) + '/s &middot; ' + secs + 's</span>';
    }
    function tile(lab, val, base, item, skill) {
      var parts = [];
      if (item) parts.push('armour ' + (item > 0 ? '+' : '') + Math.round(item) + '%');
      if (skill) parts.push('skills ' + (skill > 0 ? '+' : '') + Math.round(skill) + '%');
      if (lab === 'Power' && actPoolPct) parts.push('equip +' + Math.round(actPoolPct) + '%');
      return '<div class="tot"><span class="totlab">' + lab + (lab === 'Power' ? drainNote() : '') + '</span>'
        + '<span class="totval">' + val + '</span>'
        + '<span class="totsub">' + base + (parts.length ? ' &times; ' + parts.join(' &times; ') : ' base') + '</span></div>';
    }
    var html = '<div class="shhead"><h3>My Player</h3><span class="shsub">' + curClass + ' · '
      + picked.length + ' skills · ' + total() + ' pts</span></div>'
      + (act ? '<div class="actbar"><span class="actdot"></span><b>' + esc(act.dev) + '</b> active'
              + (act.shields.length ? ' &mdash; shield up' : '') + '</div>' : '')
      + '<div class="totrow">' + tile('Health', totHP, baseHP, hpItem, hpSkill)
      + tile('Power', totPW, basePW, pwItem, pwSkill) + '</div>'
      + (picked.length ? '' : '<p class="empty">Base stats shown. Allocate skills to build on top &mdash; each row expands to show every source.</p>');
    GRP.forEach(function (grp) {
      var mine = keys.filter(function (k) { return grp[1](stats[k].p) && !stats[k].done; });
      if (!mine.length) return;
      mine.forEach(function (k) { stats[k].done = 1; });
      html += '<div class="shgrp"><span class="shgrplab">' + grp[0] + '</span>';
      mine.sort(function (a, b) { return Math.abs(stats[b].total) - Math.abs(stats[a].total); }).forEach(function (k) {
        var st = stats[k];
        var hasBase = st.srcs.some(function (x) { return x.base; });
        var val = (hasBase && !st.pct ? (Math.round(st.total*100)/100) : fmt(st.total)) + (st.pct ? '%' : '');
        // percentages that scale a known base also show the absolute gain (+40% -> +40)
        var absBase = 0;
        if (st.pct) {
          if ([412, 390, 304].indexOf(st.p) >= 0) absBase = baseHP;
          else if ([255, 243].indexOf(st.p) >= 0) absBase = basePW;
        }
        if (absBase) val += ' <em class="abs">' + fmt(Math.round(absBase * st.total / 100)) + '</em>';
        // show what a flat protection actually mitigates at the reference attack rating
        var isProt = !st.pct && PROTP.indexOf(st.p) >= 0 && st.total !== 0;
        if (isProt) {
          // Mitigation cannot exceed 100% -- damage does not go negative. Protection at or
          // above the attack rating is total immunity on that axis, which is exactly how
          // AOE Shield's +100 produces its "Immune to AOE damage" tooltip.
          var raw = st.total * 100 / REF_RATING;
          var capped = raw >= 100;
          val += ' <em class="mit' + (capped ? ' full' : '') + '" title="Mitigation = protection / attack'
            + ' rating. At rating ' + REF_RATING + ' (both OC weapons) a flat ' + Math.round(st.total)
            + ' gives ' + (Math.round(raw * 10) / 10) + '%'
            + (capped ? ', i.e. total immunity on this axis -- protection at or above the attack rating'
                      + ' negates it entirely. Against a rating-' + Math.round(st.total) + '+ weapon it'
                      + ' would stop being absolute.' : '; against rating 200 it is half that.')
            + ' Protection axes multiply, they do not add.">'
            + (capped ? 'immune' : (Math.round(raw * 10) / 10) + '%') + '</em>';
        }
        var cls = st.total < 0 ? 'neg' : 'pos';
        // only list devices the CURRENT class can actually equip
        var devs = {};
        st.srcs.forEach(function (s) {
          s.dev.forEach(function (d) {
            var nm = Array.isArray(d) ? d[0] : d, cls = Array.isArray(d) ? d[1] : '';
            if (!cls || cls === 'Shared' || cls === curClass) devs[nm] = 1;
          });
        });
        var devlist = Object.keys(devs).sort();
        var scope = st.scope ? '<span class="stscope">' + esc(st.scope) + '</span>' : '';
        // A shield pool ABSORBS a fixed amount; it does not confer immunity. Immunity is a
        // separate fact (protection >= attack rating) and gets its own badge on the value.
        // Protection Boost has a 2000 pool but only +25 protection, so it must not read as immune.
        var sh = shieldBy[st.p];
        var badge = sh ? '<span class="shieldbadge">+' + Math.round(sh.pool) + ' shield</span>' : '';
        html += '<details class="statrow' + (sh ? ' shielded' : '') + '"><summary><span class="stname">'
          + esc(st.name) + scope + badge + '</span>'
          + '<span class="stval ' + cls + '">' + val + '</span></summary><div class="stbody">';
        if (sh) {
          html += '<div class="shieldline"><span class="srctree acttag">SHIELD</span>'
            + '<span class="srcskill">' + esc(sh.src) + '</span>'
            + '<span class="shterms">absorbs <b>' + Math.round(sh.pool) + '</b> damage of this type'
            + (sh.life ? ', or lasts <b>' + (Math.round(sh.life * 10) / 10) + 's</b>' : '')
            + ' &mdash; whichever runs out first. This is a damage <em>pool</em>, not immunity: '
            + 'the protection value above is what decides mitigation.</span></div>';
        }
        st.srcs.sort(function (a, b) { return Math.abs(b.val) - Math.abs(a.val); }).forEach(function (s) {
          html += '<div class="srcline' + (s.base ? ' isbase' : '') + (s.armour ? ' isarm' : '')
            + (s.active ? ' isact' : '') + '"><span class="srctree' + (s.base ? ' basetag' : '')
            + (s.active ? ' acttag' : '') + '">' + esc(s.tree) + '</span>'
            + '<span class="srcskill">' + esc(s.skill) + '</span>'
            + (s.kind !== 'passive' ? '<span class="srckind' + (s.dormant ? ' dormant' : '') + '">'
                + s.kind + (s.dormant ? ' · not active' : '') + '</span>' : '')
            + (s.life ? '<span class="srckind">' + s.life + 's</span>' : '')
            + '<span class="srcval ' + (s.dormant ? 'off' : (s.val < 0 ? 'neg' : 'pos')) + '">'
            + fmt(s.val) + (st.pct ? '%' : '') + '</span></div>';
        });
        if (devlist.length) {
          html += '<div class="devline"><span class="devlab">Affects</span><span class="devnames">'
            + devlist.map(esc).join(' · ') + '</span></div>';
        }
        html += '</div></details>';
      });
      html += '</div>';
    });
    html += '<p class="shfoot"><b>Protection is a flat value, not a percentage.</b> Mitigation = '
      + 'protection &divide; the attacker&rsquo;s attack rating, so the <span class="mit">%</span> shown is '
      + 'against rating ' + REF_RATING + ' (what both OC weapons carry) &mdash; against a rating-200 weapon it '
      + 'is half. Some tooltips are written as percentages anyway: <em>Aegis Armament</em> says &ldquo;an '
      + 'additional 25% Physical resistance&rdquo; but stores a flat <b>+25</b> (calc method &ldquo;Add&rdquo;), '
      + 'so it <em>adds</em> to your protection rather than scaling it. Protection at or above the '
      + 'attack rating shows as <span class="mit full">immune</span> &mdash; that is how AOE Shield&rsquo;s '
      + 'flat +100 produces its &ldquo;Immune to AOE damage&rdquo; tooltip. Protection axes multiply.</p>';
    host.innerHTML = html;
  }

  renderArmour();
  document.querySelectorAll('.tb-class').forEach(function (b) {
    b.addEventListener('click', function () {
      document.querySelectorAll('.tb-class').forEach(function (x) { x.classList.remove('active'); });
      b.classList.add('active');
      // switching class by hand drops the loaded character - it is a different build
      curClass = b.dataset.cls; alloc = {};
      curChar = null; curProfile = null; charArm = null; charGear = []; resetArm();
      activeGear = {}; activeOrder = []; stackNotes = []; blockedNames = {};
      // a different class is a different build, so the scratch slots start over too
      if (craftMode) { craftSlots = blankSlots(); craftClass = curClass; craftToGear(); }
      render();
    });
  });
  var rst = document.getElementById('tb-reset');
  if (rst) rst.addEventListener('click', function () { alloc = {}; render(); });

  // top-level view switch
  document.querySelectorAll('.viewtab').forEach(function (b) {
    b.addEventListener('click', function () {
      document.querySelectorAll('.viewtab').forEach(function (x) { x.classList.remove('active'); });
      b.classList.add('active');
      var v = b.dataset.view;
      document.getElementById('view-loadout').style.display = (v === 'loadout' ? '' : 'none');
      document.getElementById('view-tree').style.display = (v === 'tree' ? '' : 'none');
      document.getElementById('view-craft').style.display = (v === 'craft' ? '' : 'none');
      document.getElementById('loadout-nav').style.display = (v === 'loadout' ? '' : 'none');
      // gear / armour / trees / sheet are one shared panel - move it to the visible view
      // rather than maintaining two copies of the builder
      var wrap = document.getElementById('tbwrap');
      if (wrap && (v === 'tree' || v === 'craft')) {
        document.getElementById(v === 'craft' ? 'view-craft' : 'view-tree').appendChild(wrap);
      }
      if (v === 'craft') {
        if (!craftMode) {                 // entering the scratch build
          craftMode = true;
          curChar = null; curProfile = null; charArm = null;
          // slots picked for another class are not valid here
          if (!craftSlots.length || craftClass !== curClass) {
            craftSlots = blankSlots(); craftClass = curClass;
            alloc = {}; activeGear = {}; activeOrder = [];
          }
          resetArm();
          craftToGear();
        }
        render();
      } else if (v === 'tree') {
        if (craftMode) {                  // back to the real characters
          craftMode = false; charGear = []; activeGear = {}; activeOrder = [];
        }
        render();
      }
    });
  });
  render();
})();

/* ---------- mini skill trees on device cards ----------
   Players recognise a node by its position/icon, not its name. Draw each of the three
   trees that can affect a device at thumbnail size and light up the affecting nodes. */
/* ---------------------------------------------------------------------------
 * Per-card mini skill trees - DISABLED 2026-08-01.
 * Rendering ~4400 positioned nodes with inline base64 icons across 115 cards made
 * the loadout page slow to browse. The loadout cards use the text interaction chips
 * instead. Kept here (and __DEVIX__ kept in gen3) so it can be revisited.
 *
(function () {
  var D = window.__TREE__, IX = window.__DEVIX__;
  if (!D || !IX) return;
  var CELL = 25, DOT = 21;
  var KIND = { 'REACTIVE': 'k-react', 'CONDITIONAL': 'k-cond', 'ON-HIT': 'k-hit', 'PASSIVE': 'k-amp' };
  // which class does a card belong to? use its enclosing class pane
  function classOf(el) {
    var p = el.closest('.classpane');
    return p ? p.dataset.class : null;
  }
  function build(host) {
    var did = host.dataset.dev;
    var list = IX[did];
    if (!list || !list.length) return;
    var cls = classOf(host);
    var trees = (D.classes[cls] || []);
    var byId = {};
    list.forEach(function (e) { byId[e[0]] = e; });
    var html = '';
    // Always draw all three trees on the full 5-wide grid — players read the Agent Profile
    // screen as a fixed three-column layout, so hiding an uninvolved tree is disorienting.
    trees.forEach(function (g) {
      var nodes = D.trees[g] || [];
      var xs = nodes.map(function (n) { return n.x; });
      var minX = Math.min.apply(null, xs), maxX = Math.max.apply(null, xs);
      var w = (maxX - minX) * CELL + DOT;
      var ys = nodes.map(function (n) { return n.y; });
      var maxY = Math.max.apply(null, ys);
      var h = maxY * CELL + DOT;
      var inner = nodes.map(function (n) {
        var hit = byId[n.id];
        var cl = 'mn' + (hit ? ' on ' + (KIND[hit[1]] || 'k-amp') : '');
        var ic = hit && D.icons[String(n.icon)] ? '<img src="data:image/png;base64,' + D.icons[String(n.icon)] + '" alt="">' : '';
        var tip = hit ? (hit[2] + ' — ' + hit[1].toLowerCase() + '\n' + hit[3]) : n.name;
        return '<span class="' + cl + '" style="left:' + ((n.x - minX) * CELL) + 'px;top:' + (n.y * CELL) + 'px" title="' + tip.replace(/"/g, '&quot;') + '">' + ic + '</span>';
      }).join('');
      var n = nodes.filter(function (x) { return byId[x.id]; }).length;
      html += '<div class="mtree' + (n ? '' : ' idle') + '"><div class="mgrid" style="width:' + w + 'px;height:' + h + 'px">' + inner + '</div>'
        + '<span class="mtlab">' + D.names[g] + (n ? ' <b>' + n + '</b>' : '') + '</span></div>';
    });
    host.innerHTML = html;
  }
  document.querySelectorAll('.minitrees').forEach(build);
})();
 */
