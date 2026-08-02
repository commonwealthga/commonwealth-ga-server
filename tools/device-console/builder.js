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
  var nodeByGroup = {};                 // grp -> {skillId -> node}
  Object.keys(D.trees).forEach(function (g) {
    nodeByGroup[g] = {};
    D.trees[g].forEach(function (n) {
      nodeIndex[n.id] = { n: n, g: g };
      nodeByGroup[g][n.id] = n;
    });
  });

  // Some skills appear in TWO class trees under the same id - "Combat Off-Hand Utility" (806) and
  // "Combat Off-Hand Power" (807) are in both Medic Poison (157) and Assault Destroyer (159).
  // nodeIndex is keyed by id alone, so whichever tree is walked last wins, and a Medic's build
  // summary read "Destroyer 2". Resolve within the class the build actually belongs to.
  function groupOf(skillId, cls) {
    var gs = (D.classes && D.classes[cls]) || [];
    for (var i = 0; i < gs.length; i++) {
      if (nodeByGroup[gs[i]] && nodeByGroup[gs[i]][skillId]) return gs[i];
    }
    var e = nodeIndex[skillId];
    return e ? e.g : null;
  }
  function treeNameOf(skillId, cls) {
    return (D.names && D.names[groupOf(skillId, cls)]) || '?';
  }

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
    // Suppress the losing EFFECTS, not the whole device. A Boost Beam that loses the proximity
    // damage buff to a Frenzy Wave is still a working heal - switching the device off threw the
    // heal away with it. applyStacking already reports losers as "category|source", which is
    // exactly the granularity needed.
    var blocked = {};
    (st.blocked || []).forEach(function (k) { blocked[k] = 1; });
    function keep(x) { return !blocked[x.cat + '|' + x.src]; }
    effects = effects.filter(keep);
    shields = shields.filter(keep);
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
  // is this value good for the player? not the same question as "is it positive?"
  function benefit(prop, v) {
    if (!v) return 'pos';
    var G = window.GA || {};
    if ((G.ALWAYS_GOOD || {})[prop]) return 'pos';
    if ((G.LOWER_BETTER || {})[prop]) return v < 0 ? 'pos' : 'neg';
    return v < 0 ? 'neg' : 'pos';
  }
  function fmt(v) { v = Math.round(v * 100) / 100; return (v > 0 ? '+' : '') + v; }

  // The aggregation used to live inside renderSheet, which meant it could only ever describe
  // whichever build the globals happened to hold - fine for one player, useless for a fight.
  // Split out so a defender can be aggregated too. See statsFor() below.
  function collectStats() {
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
        st.srcs.push({ skill: e.n.name, tree: treeNameOf(e.n.id, curClass), val: v, kind: f.kind, life: f.life,
                       dev: dev, dormant: cond ? 1 : 0 });
      });
    });
    return { stats: stats, picked: picked, act: act, shieldBy: shieldBy, eqb: eqb };
  }

  // Aggregate an ARBITRARY build rather than the one on screen. The build state is spread over
  // several module globals that equipBuffs()/activeContribution() also read, so rather than
  // rewrite five signatures we swap the globals, collect, and put them back. Contained here,
  // and restored on the way out even if collection throws.
  var CTXKEYS = ['curClass', 'alloc', 'charArm', 'armSlots', 'charGear', 'activeGear',
                 'activeOrder', 'stackNotes', 'playerBuffs', 'blockedNames'];
  function statsFor(ctx) {
    var g = { curClass: curClass, alloc: alloc, charArm: charArm, armSlots: armSlots,
              charGear: charGear, activeGear: activeGear, activeOrder: activeOrder,
              stackNotes: stackNotes, playerBuffs: playerBuffs, blockedNames: blockedNames };
    try {
      if ('curClass' in ctx) curClass = ctx.curClass;
      if ('alloc' in ctx) alloc = ctx.alloc || {};
      if ('charArm' in ctx) charArm = ctx.charArm;
      if ('armSlots' in ctx) armSlots = ctx.armSlots || [];
      if ('charGear' in ctx) charGear = ctx.charGear || [];
      if ('activeGear' in ctx) activeGear = ctx.activeGear || {};
      if ('activeOrder' in ctx) activeOrder = ctx.activeOrder || [];
      var res = collectStats();
      res.buffs = playerBuffs;        // captured before the globals are put back
      res.alloc = alloc;
      res.notes = stackNotes;         // why a device refused to stay switched on
      return res;
    } finally {
      CTXKEYS.forEach(function (k) {
        switch (k) {
          case 'curClass': curClass = g.curClass; break;
          case 'alloc': alloc = g.alloc; break;
          case 'charArm': charArm = g.charArm; break;
          case 'armSlots': armSlots = g.armSlots; break;
          case 'charGear': charGear = g.charGear; break;
          case 'activeGear': activeGear = g.activeGear; break;
          case 'activeOrder': activeOrder = g.activeOrder; break;
          case 'stackNotes': stackNotes = g.stackNotes; break;
          case 'playerBuffs': playerBuffs = g.playerBuffs; break;
          case 'blockedNames': blockedNames = g.blockedNames; break;
        }
      });
    }
  }
  // builder.js loads BEFORE bench.js, so the namespace may not exist yet. bench.js does
  // `window.GA = window.GA || {}` too, so creating it here is safe - it extends, not replaces.
  window.GA = window.GA || {};
  window.GA.statsFor = statsFor;

  // Headline health and power, derived once so every surface agrees. The combat sandbox got
  // this wrong by reimplementing it: the armour's +70% health is prop 390, not 51 or 412, so a
  // Recon read 1319 HP instead of 2229. Layers MULTIPLY (TgPawn::ApplyBuff: v1 = base*(1+item),
  // v2 = v1*(1+skill)) and the game TRUNCATES the result rather than rounding.
  var HP_PCT = [412, 390, 304];
  var PW_PCT = [255, 243];
  function deriveTotals(stats) {
    var keys = Object.keys(stats);
    function baseOf(p) {
      var st = stats[p + '|0|0'];
      if (!st) return 0;
      var v = 0;
      st.srcs.forEach(function (x) { if (x.layer === 'base') v += x.val; });
      return v;
    }
    function pctFor(list, layer) {
      var sum = 0;
      keys.forEach(function (k) {
        var st = stats[k];
        if (!st.pct || list.indexOf(st.p) < 0) return;
        st.srcs.forEach(function (x) {
          if ((x.layer || 'skill') === layer) sum += x.val;
        });
      });
      return sum;
    }
    var baseHP = baseOf(51), basePW = baseOf(243);
    var hpItem = pctFor(HP_PCT, 'item'), hpSkill = pctFor(HP_PCT, 'skill');
    var pwItem = pctFor(PW_PCT, 'item'), pwSkill = pctFor(PW_PCT, 'skill');
    return {
      baseOf: baseOf, pctFor: pctFor,
      baseHP: baseHP, basePW: basePW,
      hpItem: hpItem, hpSkill: hpSkill, pwItem: pwItem, pwSkill: pwSkill,
      totHP: Math.floor(baseHP * (1 + hpItem / 100) * (1 + hpSkill / 100)),
      totPW: Math.floor(basePW * (1 + pwItem / 100) * (1 + pwSkill / 100))
    };
  }

  // ============================ COMBAT SANDBOX ============================
  // Not a 1v1. A roster of combatants on two teams, each an ordinary saved build, with every
  // device aimable at a specific actor. Two medics healing an assault while two recons shoot it
  // is the shape this has to support, so nothing here assumes one attacker and one defender.
  //
  // Who a device may be aimed at comes from the device mode's target type, not from guesswork:
  // 'enemy' devices only reach the other team, 'friend' devices only their own, 'self' devices
  // project nothing at all.
  var sim = { actors: [], focus: { from: null, to: null }, nextId: 1, drag: null };

  function charList() { var C = window.__CHARS__ || {}; return C.chars || (C.length ? C : []); }
  function charOf(cls) { return charList().filter(function (c) { return c.cls === cls; })[0]; }

  function addActor(team, cls) {
    var c = charOf(cls) || charList()[0];
    if (!c) return null;
    var a = { id: sim.nextId++, team: team, cls: c.cls,
              pid: Object.keys(c.profiles).sort()[0], active: {}, aim: {} };
    sim.actors.push(a);
    return a;
  }
  function actorById(id) {
    return sim.actors.filter(function (a) { return String(a.id) === String(id); })[0];
  }
  function teamOf(id) { var a = actorById(id); return a ? a.team : null; }
  function foes(a) { return sim.actors.filter(function (x) { return x.team !== a.team; }); }
  function allies(a) { return sim.actors.filter(function (x) { return x.team === a.team; }); }

  // Melee, Ranged and Specialty share your hands - only one is ever out. Off-hands, boosts and
  // the jetpack are independent and can all run at once.
  var HANDS = { Melee: 1, Ranged: 1, Specialty: 1 };
  function stowOtherWeapons(a, ctx, keepSlot) {
    var g = (ctx.charGear || [])[+keepSlot];
    if (!g || !HANDS[g.cat]) return;
    Object.keys(a.active).forEach(function (slot) {
      if (String(slot) === String(keepSlot)) return;
      var o = (ctx.charGear || [])[+slot];
      if (o && HANDS[o.cat]) { delete a.active[slot]; delete a.aim[slot]; }
    });
  }

  // Who a device actually reaches. Three things decide it, and none of them is a guess:
  //   - a jetpack reaches nobody. It is movement; it only costs power. (Its data says "enemy",
  //     which is meaningless for a thruster, so the category overrides.)
  //   - a splash radius means it hits EVERYONE it is allowed to hit, not one chosen body. That
  //     is what makes Frenzy/Power/Protection Wave team-wide and Poison Aura hit every enemy.
  //   - a melee weapon's radius is its swing arc, not splash, so it stays single-target.
  // "Enemy and Self" (grenades) means it can hurt the thrower, never a team-mate.
  function deviceScope(a, g, dev, mode) {
    var cat = g.cat || '';
    if (cat === 'Jetpack') return { kind: 'none', targets: [], label: 'self only' };
    var tgt = dev ? GA.deviceTarget(dev, mode) : 'enemy';
    if (tgt === 'self') return { kind: 'none', targets: [a.id], label: 'self' };
    var rad = 0;
    (dev && dev.modes || []).forEach(function (m) {
      if ((!mode || !m.kind || m.kind === mode) && m.hit && m.hit.rad > rad) rad = m.hit.rad;
    });
    var splash = rad > 0 && cat !== 'Melee';
    var enemies = foes(a), mates = allies(a);
    if (splash) {
      if (tgt === 'friend') return { kind: 'all', targets: mates.map(idOf), label: 'all allies' };
      if (tgt === 'enemy') return { kind: 'all', targets: enemies.map(idOf), label: 'all enemies' };
      if (tgt === 'enemyself') {
        return { kind: 'all', targets: enemies.map(idOf).concat([a.id]),
                 label: 'all enemies + self' };
      }
      return { kind: 'all', targets: sim.actors.map(idOf), label: 'everyone' };
    }
    var picks = tgt === 'friend' ? mates
      : tgt === 'enemy' ? enemies
        : tgt === 'enemyself' ? enemies.concat([a])
          : sim.actors;
    return { kind: 'single', picks: picks, targets: [], label: '' };
  }
  function idOf(x) { return x.id; }

  function actorCtx(a) {
    var c = charOf(a.cls); if (!c) return null;
    var p = c.profiles[a.pid] || c.profiles[Object.keys(c.profiles)[0]];
    if (!p) return null;
    var al = {};
    p.skills.forEach(function (sid) { if (nodeIndex[sid]) al[sid] = 1; });
    return { curClass: c.cls, alloc: al,
             charArm: (p.armour && p.armour.length) ? p.armour : null,
             charGear: p.devices || [], activeGear: a.active,
             activeOrder: Object.keys(a.active) };
  }

  // everything an actor's switched-on gear throws at somebody else, tagged with its aim
  function actorOutgoing(a) {
    var ctx = actorCtx(a);
    if (!ctx || !GA.projectedEffects) return [];
    var col = statsFor(ctx), out = [];
    if ((col.notes || []).length) a.lastNotes = col.notes;
    Object.keys(a.active).forEach(function (slot) {
      var g = (ctx.charGear || [])[+slot];
      if (!g) return;
      var dev = (window.__DEVMODEL__ || {})[String(g.id)];
      if (!dev) return;
      var sc = deviceScope(a, g, dev, a.active[slot]);
      if (sc.kind === 'none') return;
      var recipients = sc.kind === 'all' ? sc.targets
        : (a.aim[slot] && actorById(a.aim[slot]) ? [a.aim[slot]] : []);
      if (!recipients.length) return;
      var fx = GA.projectedEffects({ dev: dev, meta: (window.__DEVMETA__ || {})[String(g.id)] || {},
        ix: (window.__DEVFX__ || {})[String(g.id)] || [], alloc: col.alloc, name: g.name,
        mode: a.active[slot], buffs: col.buffs,
        variant: { sig: g.sig, base: g.base, groups: g.groups, nums: g.nums } });
      recipients.forEach(function (at) {
        // a boost is not in the carrier's baseline, so it must land on them as well
        if (String(at) === String(a.id) && g.cat !== 'Boost') return;
        fx.forEach(function (f) {
          out.push({ p: f.p, name: f.name, v: f.v, pct: f.pct, src: f.src, kind: f.kind,
                     life: f.life, cat: f.cat, from: a.id, to: at, slot: slot });
        });
      });
    });
    return out;
  }

  function allOutgoing() {
    var out = [];
    sim.actors.forEach(function (a) { out = out.concat(actorOutgoing(a)); });
    return out;
  }

  // an actor's resolved state, with whatever is currently landing on them folded in
  function actorState(a, inbound) {
    var ctx = actorCtx(a); if (!ctx) return null;
    var col = statsFor(ctx);
    if ((col.notes || []).length) a.lastNotes = col.notes;
    var dv = deriveTotals(col.stats);
    var prot = GA.protectionFrom(col.stats);
    var buffs = (col.buffs || []).slice();
    (inbound || []).forEach(function (f) {
      if (f.pct) { buffs.push({ p: f.p, v: f.v, pct: 1, src: f.src, kind: f.kind }); return; }
      if ((GA.PROT_PROPS || []).indexOf(f.p) >= 0) prot[f.p] = (prot[f.p] || 0) + f.v;
    });
    // skill spend per tree, so a card says what kind of build it is at a glance
    var trees = {};
    Object.keys(col.alloc || {}).forEach(function (sid) {
      var t = treeNameOf(sid, a.cls);
      trees[t] = (trees[t] || 0) + 1;
    });
    // every buff category live on this actor - what a Neutralize Wave would find to strip
    var cats = {};
    (inbound || []).forEach(function (f) { if (f.cat && f.v > 0) cats[f.cat] = 1; });
    (col.buffs || []).forEach(function (f) { if (f.cat && f.v > 0) cats[f.cat] = 1; });
    ((col.act && col.act.effects) || []).forEach(function (f) { if (f.cat && f.v > 0) cats[f.cat] = 1; });
    return { a: a, ctx: ctx, col: col, prot: prot, buffs: buffs, inbound: inbound || [],
             maxHP: dv.totHP, pool: dv.totPW, cats: cats, notes: a.lastNotes || [],
             skills: (col.picked || []).length, trees: trees };
  }

  // every damaging shot an actor can throw, with its refire so DPS is available
  function actorShots(st) {
    var out = [];
    (st.ctx.charGear || []).forEach(function (g, gi) {
      var dev = (window.__DEVMODEL__ || {})[String(g.id)];
      if (!dev) return;
      var res = GA.resolve({ dev: dev, meta: (window.__DEVMETA__ || {})[String(g.id)] || {},
        ix: (window.__DEVFX__ || {})[String(g.id)] || [], alloc: st.ctx.alloc, situational: true,
        buffs: st.buffs, variant: { sig: g.sig, base: g.base, groups: g.groups, nums: g.nums } });
      (res.modes || []).forEach(function (m, mi) {
        var mm = (dev.modes || [])[mi] || {};
        var hit = mm.hit || {};
        // resolve() rebuilds mode objects, so the strip table has to come off the model
        var strip = mm.strip || [];
        if (hit.tgt === 'friend' || hit.tgt === 'self') return;   // not a weapon
        var refire = null, power = m.power && m.power.value != null ? m.power.value : null;
        (m.chips || []).forEach(function (c) { if (c.prop === 53) refire = c.value; });
        (m.chips || []).forEach(function (c) {
          if ((c.prop !== 51 && c.prop !== 211) || c.sign >= 0 || !c.value) return;
          out.push({ id: g.id, name: g.name, mode: m.kind || 'PRI', label: c.label,
                     raw: c.value, cat: c.cat, hit: hit, dot: c.life > 0,
                     refire: refire, power: power, strip: strip });
        });
      });
    });
    return out;
  }

  // healing an actor is receiving per second, from allies aiming heals at them
  function healingOn(actorId, thrown) {
    var hps = 0, srcs = [];
    sim.actors.forEach(function (h) {
      var ctx = actorCtx(h); if (!ctx) return;
      var col = statsFor(ctx);
      Object.keys(h.active).forEach(function (slot) {
        if (String(h.aim[slot]) !== String(actorId)) return;
        var g = (ctx.charGear || [])[+slot]; if (!g) return;
        var dev = (window.__DEVMODEL__ || {})[String(g.id)]; if (!dev) return;
        if (GA.deviceTarget(dev, h.active[slot]) === 'enemy') return;
        var res = GA.resolve({ dev: dev, meta: (window.__DEVMETA__ || {})[String(g.id)] || {},
          ix: (window.__DEVFX__ || {})[String(g.id)] || [], alloc: col.alloc, situational: true,
          buffs: col.buffs, variant: { sig: g.sig, base: g.base, groups: g.groups, nums: g.nums } });
        (res.modes || []).forEach(function (m) {
          if (m.kind && h.active[slot] && m.kind !== h.active[slot]) return;
          var refire = null;
          (m.chips || []).forEach(function (c) { if (c.prop === 53) refire = c.value; });
          (m.chips || []).forEach(function (c) {
            if (c.prop !== 51 || c.sign < 0 || !c.value) return;
            // "Self: Heal" is the medic topping THEMSELVES up while they beam. It only counts
            // toward the person being healed when that person is the medic.
            if (c.self && String(h.id) !== String(actorId)) return;
            // a heal-over-time delivers its value across its lifetime; a direct heal per refire
            var per = c.life > 0 ? (c.value / c.life) : (refire ? c.value / refire : c.value);
            hps += per;
            srcs.push({ who: h.cls + ' #' + h.id, dev: g.name, hps: per, hot: c.life > 0 });
          });
        });
      });
    });
    return { hps: hps, srcs: srcs };
  }

  function fmt1(v) { return Math.round(v * 10) / 10; }

  function trees2str(t) {
    return Object.keys(t).sort(function (a, b) { return t[b] - t[a]; })
      .map(function (k) { return esc(k) + ' ' + t[k]; }).join(' · ') || 'no skills';
  }

  // ============================== TIMELINE ==============================
  // A "what if" run, not a battle simulator. Everyone fires from t=0 at whatever they have
  // switched on and aimed; nobody moves, dodges or misses. What it DOES model is the thing a
  // single damage-per-second figure cannot: support runs out. Buffs expire, power empties,
  // cooldowns come back, and mitigation changes underneath the damage as they do. That is why
  // an unkillable target can still have a real time to kill.
  //
  // Stepped rather than solved: buff expiry changes mitigation, mitigation changes damage, and
  // damage decides when someone dies. There is no closed form for that.
  var STEP = 0.1;
  // The shared off-hand cooldown. Roughly half a second in game - you press protection, frenzy
  // and power wave one after another, never together.
  var OFFHAND_GCD = 0.5;

  // Everything about one device that matters over time.
  function simDevice(a, ctx, slot, col) {
    var g = (ctx.charGear || [])[+slot];
    if (!g) return null;
    var dev = (window.__DEVMODEL__ || {})[String(g.id)];
    if (!dev) return null;
    var mode = a.active[slot];
    var res = GA.resolve({ dev: dev, meta: (window.__DEVMETA__ || {})[String(g.id)] || {},
      ix: (window.__DEVFX__ || {})[String(g.id)] || [], alloc: col.alloc, situational: true,
      buffs: col.buffs, variant: { sig: g.sig, base: g.base, groups: g.groups, nums: g.nums } });
    var mi = -1;
    (res.modes || []).forEach(function (m, k) { if (mi < 0 && (!m.kind || m.kind === mode)) mi = k; });
    if (mi < 0) mi = 0;
    var m = (res.modes || [])[mi];
    if (!m) return null;
    var mm = (dev.modes || [])[mi] || {};
    var d = { slot: slot, name: g.name, cat: g.cat, id: g.id,
              hit: mm.hit || {}, strip: mm.strip || [],
              power: (m.power && m.power.value != null) ? m.power.value : 0,
              refire: 0, cooldown: 0, shots: [], heals: [], buffs: [], maxLife: 0,
              scope: deviceScope(a, g, dev, mode), ready: 0 };
    (m.chips || []).forEach(function (c) {
      if (c.prop === 53) d.refire = c.value;
      if (c.prop === 4) d.cooldown = c.value;
      if (c.prop === 150) d.persist = c.value;        // Persist Time - how long a boost lasts
    });
    d.selfTimed = [];
    (m.chips || []).forEach(function (c) {
      if (c.base === null) return;
      if (c.prop === 51 || c.prop === 211) {
        if (c.sign < 0) d.shots.push({ raw: c.value, cat: c.cat, life: c.life });
        else if (!c.self) d.heals.push({ v: c.value, life: c.life });
        else if (c.life > 0) d.selfTimed.push({ p: c.prop, name: GA.statName(c.prop) || 'self',
                                                v: c.value, pct: c.isPct, cat: c.cat,
                                                src: g.name, life: c.life });
      } else if (c.self && c.life > 0) {
        var nm2 = GA.statName(c.prop);
        if (nm2) d.selfTimed.push({ p: c.prop, name: nm2, v: c.sign < 0 ? -c.value : c.value,
                                    pct: c.isPct, cat: c.cat, src: g.name, life: c.life });
      }
    });
    // How often it is sensible to use this thing.
    //
    // A support device that applies timed effects should be re-fired when they EXPIRE, not as
    // fast as it will physically fire. An Adrenaline Gun refires every 0.5s but its Regeneration
    // and Health-Max buffs last 10s; spamming it just restarts the same timers and empties the
    // power pool for nothing. Waiting for the buff to lapse is what a player actually does, and
    // it is what lets power regenerate in between.
    //
    // Weapons are the other way round: their damage is the point, so they fire at their refire
    // rate whatever incidental timed effects they carry (the Agonizer's 4s debuffs do not slow
    // the gun down). A BioFeedback Beam has no timed effects at all - it is a per-tick heal - so
    // it also stays on refire.
    var supportish = (d.hit.tgt === 'friend' || d.hit.tgt === 'self');
    if (supportish && d.maxLife > 0) {
      d.interval = Math.max(d.cooldown || 0, d.maxLife);
      d.cadence = 'on expiry';
    } else {
      d.interval = d.refire > 0 ? d.refire : (d.cooldown > 0 ? d.cooldown : 0);
      d.cadence = d.refire > 0 ? 'refire' : 'cooldown';
    }
    // Every Boost carries a Persist Time and NOTHING else - no refire, no cooldown. They are
    // bought with morale during a match rather than being cooldown-gated, so there is no repeat
    // interval to read. Without this they fell through "no interval, never fires" and did nothing
    // at all: Healing Boost never healed, Oathbreaker never landed its damage or heal debuff.
    if (!d.interval && d.persist > 0) {
      d.once = true;
      d.interval = d.persist;
      d.cadence = 'once (' + d.persist + 's)';
    }
    return d;
  }

  function buildSim() {
    var actors = sim.actors.map(function (a) {
      var ctx = actorCtx(a);
      if (!ctx) return null;
      // Switching a boost on in the board makes statsFor fold its buffs straight into the
      // actor - which is right for the static panel, but wrong here: the timeline decides WHEN
      // a boost goes off. Leaving them in meant a Sensor Boost gated to 14s was still buffing
      // the Scorpia from t=0, and the target died at 6.1s instead of 12.2s. So the baseline is
      // built WITHOUT boosts, and the timeline applies them at the moment they fire.
      var baseActive = {}, hasBoost = false;
      Object.keys(a.active).forEach(function (slot) {
        var g = (ctx.charGear || [])[+slot];
        if (g && g.cat === 'Boost') { hasBoost = true; return; }
        baseActive[slot] = a.active[slot];
      });
      var baseCtx = ctx;
      if (hasBoost) {
        baseCtx = {};
        Object.keys(ctx).forEach(function (k) { baseCtx[k] = ctx[k]; });
        baseCtx.activeGear = baseActive;
        baseCtx.activeOrder = Object.keys(baseActive);
      }
      var col = statsFor(baseCtx);
      var dv = deriveTotals(col.stats);
      var devs = Object.keys(a.active).map(function (slot) { return simDevice(a, ctx, slot, col); })
        .filter(Boolean);
      // projected effects, resolved once - what this actor hands out when a device fires
      var proj = {};
      Object.keys(a.active).forEach(function (slot) {
        var g = (ctx.charGear || [])[+slot]; if (!g) return;
        var dev = (window.__DEVMODEL__ || {})[String(g.id)]; if (!dev) return;
        proj[slot] = GA.projectedEffects({ dev: dev,
          meta: (window.__DEVMETA__ || {})[String(g.id)] || {},
          ix: (window.__DEVFX__ || {})[String(g.id)] || [], alloc: col.alloc, name: g.name,
          mode: a.active[slot], buffs: col.buffs,
          variant: { sig: g.sig, base: g.base, groups: g.groups, nums: g.nums } });
      });
      devs.forEach(function (d) {
        var L = 0;
        (proj[d.slot] || []).forEach(function (f) { if (f.life > L) L = f.life; });
        d.maxLife = L;
        var supportish = (d.hit.tgt === 'friend' || d.hit.tgt === 'self');
        if (supportish && L > 0) {
          d.interval = Math.max(d.cooldown || 0, L);
          d.cadence = 'on expiry';
        }
      });
      return { id: a.id, team: a.team, cls: a.cls, pid: a.pid,
               maxHP: dv.totHP, hp: dv.totHP,
               maxPW: dv.totPW, pw: dv.totPW,
               regen: baseRegen(col.stats),
               baseProt: GA.protectionFrom(col.stats),
               devs: devs, proj: proj, aim: a.aim, live: [], dead: false, spentThisStep: false,
               offhandReady: 0 };
    }).filter(Boolean);
    var byId = {};
    actors.forEach(function (x) { byId[x.id] = x; });
    return { actors: actors, byId: byId };
  }

  function baseRegen(stats) {
    var r = 0;
    Object.keys(stats).forEach(function (k) {
      var x = stats[k];
      if (x.p === 244 && !x.pct) r += x.total;
    });
    return r;
  }

  // protections right now = base plus whatever timed buffs are still live
  function protNow(act) {
    var p = {};
    Object.keys(act.baseProt).forEach(function (k) { p[k] = act.baseProt[k]; });
    act.live.forEach(function (f) {
      if (f.pct) return;
      if ((GA.PROT_PROPS || []).indexOf(f.p) >= 0) p[f.p] = (p[f.p] || 0) + f.v;
    });
    return p;
  }
  // A weapon's damage is resolved once, before the run starts, so a buff that lands mid-fight
  // (a Sensor Boost bought at 3s) would otherwise never reach it. Apply the live percentage
  // damage modifiers at the moment of firing instead.
  //   attack type 1 melee -> 212, 2 ranged -> 214, 3 AOE -> 321; 65 is the general one
  var DMG_MOD_BY_ATK = { 1: 212, 2: 214, 3: 321 };
  function liveDamageMult(act, hit) {
    var want = DMG_MOD_BY_ATK[hit && hit.atk] || 0;
    var pct = 0;
    act.live.forEach(function (f) {
      if (!f.pct) return;
      if (f.p === 65 || (want && f.p === want)) pct += f.v;
    });
    return 1 + pct / 100;
  }

  function catsNow(act) {
    var c = {};
    act.live.forEach(function (f) { if (f.cat && f.v > 0) c[f.cat] = 1; });
    return c;
  }

  function runTimeline(seconds) {
    var S = buildSim();
    if (!S.actors.length) return null;
    var events = [], series = {}, deaths = {};
    S.actors.forEach(function (a) { series[a.id] = []; });

    function targetsOf(a, d) {
      if (d.scope.kind === 'all') return d.scope.targets.slice();
      var t = a.aim[d.slot];
      return t ? [t] : [];
    }
    function ev(t, who, text, kind) {
      events.push({ t: Math.round(t * 10) / 10, who: who, text: text, kind: kind || '' });
    }

    for (var t = 0; t <= seconds + 1e-9; t += STEP) {
      // sample first so t=0 is the opening state
      S.actors.forEach(function (a) {
        var pr = protNow(a);
        var protSum = 0;
        (GA.PROT_PROPS || []).forEach(function (k) { protSum += (pr[k] || 0); });
        series[a.id].push({ t: t, hp: Math.max(0, a.hp), pw: Math.max(0, a.pw), prot: protSum });
      });

      // expire timed effects
      S.actors.forEach(function (a) {
        var before = a.live.length;
        a.live = a.live.filter(function (f) {
          if (f.until > t) return true;
          ev(t, a.id, f.src + ' ' + f.name + ' expires', 'expire');
          return false;
        });
        if (before !== a.live.length) { /* mitigation recomputed below */ }
      });

      S.actors.forEach(function (a) { a.spentThisStep = false; });

      // fire everything that is ready and affordable
      S.actors.forEach(function (a) {
        if (a.dead) return;
        a.devs.forEach(function (d) {
          var interval = d.interval || 0;
          if (!interval) return;                       // nothing that repeats
          if (t + 1e-9 < d.ready) return;
          if (d.once) {
            // Boosts cost morale (prop 318 "Required Points To Fire"), not a cooldown. Nobody
            // opens a fight with one - you press it when you have banked enough. How fast morale
            // accrues is NOT in any data we can read: props 326/398 are unused in the asset DB,
            // AddMoralePoints is native so the UC only calls it, and the server reimplements the
            // replication but not the accrual. So the moment is an INPUT rather than a guess.
            if (d.firedOnce) return;
            if (t + 1e-9 < (sim.moraleAt == null ? 1e9 : sim.moraleAt)) return;
          }
          // Off-hands share a global cooldown - you cannot let three waves off at once, you press
          // them one after another. Without this the whole team's buffs land on the same tick.
          if (d.cat === 'Offhand' && t + 1e-9 < a.offhandReady) return;
          // A buff-stripper is held until there is something worth stripping. Nobody opens with
          // a Neutralize Wave; you wait until the other side has committed its buffs.
          if ((d.strip || []).length) {
            var worth = targetsOf(a, d).some(function (tid) {
              var v = S.byId[tid];
              if (!v || v.dead || v.team === a.team) return false;
              var tc = catsNow(v);
              return (d.strip || []).some(function (sg) {
                return (sg.cats || []).some(function (cc) { return tc[cc]; });
              });
            });
            if (!worth) return;                        // hold fire
          }
          // A 0.1s step cannot represent a weapon that fires 20 times a second, so count how
          // many shots actually fall inside this step rather than allowing one.
          var volley = 0;
          while (d.ready <= t + 1e-9 && volley < 200) { d.ready += interval; volley++; }
          if (d.ready < t) d.ready = t + interval;
          var cost = d.power || 0;
          if (cost > 0) {
            var afford = Math.floor(a.pw / cost);
            if (afford <= 0) {
              if (!d.starved) { ev(t, a.id, d.name + ' out of power', 'power'); d.starved = true; }
              return;
            }
            if (afford < volley) volley = afford;
            a.pw -= cost * volley; a.spentThisStep = true; d.starved = false;
          }

          if (d.cat === 'Offhand') a.offhandReady = t + OFFHAND_GCD;
          var tgts = targetsOf(a, d);
          if (!d.firedOnce) {
            ev(t, a.id, d.name + ' fires' + (d.cadence === 'on expiry' ? ' (re-applies on expiry)' : ''),
               (d.strip || []).length ? 'strip' : 'fire');
            d.firedOnce = true;
          }
          // damage
          d.shots.forEach(function (sh) {
            tgts.forEach(function (tid) {
              var v = S.byId[tid];
              if (!v || v.dead || v.team === a.team) return;
              var extra = 0;
              var tc = catsNow(v);
              (d.strip || []).forEach(function (sg) {
                if ((sg.cats || []).some(function (cc) { return tc[cc]; })) extra += sg.dmg;
              });
              if (extra) {
                // the strip lands: those buffs are gone
                v.live = v.live.filter(function (f) {
                  return !(d.strip || []).some(function (sg) {
                    return (sg.cats || []).indexOf(f.cat) >= 0;
                  });
                });
                ev(t, v.id, d.name + ' strips buffs (+' + Math.round(extra) + ')', 'strip');
              }
              var m = GA.mitigate(sh.raw * liveDamageMult(a, d.hit) + extra,
                { cat: sh.cat, damageType: d.hit.dmg, attackType: d.hit.atk, rating: d.hit.rating },
                protNow(v), {});
              v.hp -= m.shown * volley;
              if (v.hp <= 0 && !v.dead) {
                v.dead = true; v.hp = 0; deaths[v.id] = t;
                ev(t, v.id, v.cls + ' #' + v.id + ' dies', 'death');
              }
            });
          });
          // healing
          d.heals.forEach(function (h) {
            tgts.forEach(function (tid) {
              var v = S.byId[tid];
              if (!v || v.dead || v.team !== a.team) return;
              if (h.life > 0) {
                // spread across its duration rather than landing whole on the first tick
                v.hots = (v.hots || []).filter(function (x) { return x.src !== d.name; });
                v.hots.push({ src: d.name, rate: h.v / h.life, until: t + h.life });
              } else {
                v.hp = Math.min(v.maxHP, v.hp + h.v * volley);
              }
            });
          });
          // the device's own timed self-effects (Oathbreaker's +20 protection for 10s)
          (d.selfTimed || []).forEach(function (f) {
            a.live = a.live.filter(function (x) { return !(x.src === f.src && x.p === f.p); });
            a.live.push({ p: f.p, name: f.name, v: f.v, pct: f.pct, cat: f.cat,
                          src: f.src, until: t + f.life });
          });
          // buffs / debuffs with a lifetime
          (a.proj[d.slot] || []).forEach(function (f) {
            if (!f.life) return;                      // instantaneous riders are not tracked
            var recips = d.once && d.scope.kind === 'all'
              ? d.scope.targets.concat([a.id]) : tgts;
            recips.forEach(function (tid) {
              var v = S.byId[tid];
              if (!v || v.dead) return;
              v.live = v.live.filter(function (x) { return !(x.src === f.src && x.p === f.p); });
              v.live.push({ p: f.p, name: f.name, v: f.v, pct: f.pct, cat: f.cat,
                            src: f.src, until: t + f.life });
            });
          });
        });
      });

      // heal-over-time ticks
      S.actors.forEach(function (a) {
        if (!a.hots || a.dead) return;
        a.hots = a.hots.filter(function (h) { return h.until > t; });
        a.hots.forEach(function (h) { a.hp = Math.min(a.maxHP, a.hp + h.rate * STEP); });
      });

      // power regen only when nothing was spent this step (confirmed in game, backlog C4)
      S.actors.forEach(function (a) {
        if (!a.spentThisStep && a.pw < a.maxPW) a.pw = Math.min(a.maxPW, a.pw + a.regen * STEP);
      });
    }
    return { S: S, events: events, series: series, deaths: deaths, seconds: seconds };
  }

  function renderTimeline(seconds) {
    var host = document.getElementById('cb-timeline');
    if (!host) return;
    var r = runTimeline(seconds);
    if (!r) { host.innerHTML = ''; return; }

    var W = 900, H = 170;
    var show = sim.tlShow || (sim.tlShow = { hp: 1, pw: 1, prot: 1, marks: 1 });
    // a readable tick spacing for whatever window is being shown
    var tick = seconds <= 15 ? 1 : seconds <= 40 ? 5 : seconds <= 90 ? 10 : 20;
    var grid = '';
    for (var gt = 0; gt <= seconds + 1e-9; gt += tick) {
      var gx = (gt / seconds) * W;
      grid += '<line class="tlgrid" x1="' + gx.toFixed(1) + '" y1="0" x2="' + gx.toFixed(1)
        + '" y2="' + H + '"/>';
    }
    function spark(a) {
      var pts = r.series[a.id];
      var step = Math.max(1, Math.floor(pts.length / W));
      var d = [], dp = [], dr = [];
      var maxProt = 1;
      pts.forEach(function (q) { if (q.prot > maxProt) maxProt = q.prot; });
      for (var i = 0; i < pts.length; i += step) {
        var x = (i / (pts.length - 1)) * W;
        d.push((i ? 'L' : 'M') + x.toFixed(1) + ' ' + (H - (pts[i].hp / a.maxHP) * H).toFixed(1));
        dp.push((i ? 'L' : 'M') + x.toFixed(1) + ' ' + (H - (a.maxPW ? pts[i].pw / a.maxPW : 0) * H).toFixed(1));
        dr.push((i ? 'L' : 'M') + x.toFixed(1) + ' ' + (H - (pts[i].prot / maxProt) * H).toFixed(1));
      }
      // a tick wherever something lapsed on this actor
      var marks = r.events.filter(function (e) {
        return String(e.who) === String(a.id) && (e.kind === 'expire' || e.kind === 'power' || e.kind === 'strip');
      }).map(function (e) {
        var x = (e.t / r.seconds) * W;
        return '<line class="tlmark ' + esc(e.kind) + '" x1="' + x.toFixed(1) + '" y1="0" x2="'
          + x.toFixed(1) + '" y2="' + H + '"><title>' + esc(e.t + 's ' + e.text) + '</title></line>';
      }).join('');
      // the handful worth naming in place: what was used, what was stripped, what ran dry
      // activations, strips, power-outs and deaths first; expiries fill whatever room is left
      var mine = r.events.filter(function (e) { return String(e.who) === String(a.id); });
      var seenLab = {};
      function pick(kinds, cap) {
        return mine.filter(function (e) {
          if (kinds.indexOf(e.kind) < 0) return false;
          if (seenLab[e.text]) return false;
          seenLab[e.text] = 1; return true;
        }).slice(0, cap);
      }
      var labels = pick(['fire', 'strip', 'power', 'death'], 5)
        .concat(pick(['expire'], 3))
        .sort(function (x, y) { return x.t - y.t; });
      var labSvg = labels.map(function (e, k) {
        var x = (e.t / r.seconds) * W;
        var y = 14 + (k % 3) * 15;
        var anchor = x > W * 0.75 ? 'end' : 'start';
        var dx = anchor === 'end' ? -4 : 4;
        return '<line class="tlpin ' + esc(e.kind) + '" x1="' + x.toFixed(1) + '" y1="0" x2="'
          + x.toFixed(1) + '" y2="' + H + '"/>'
          + '<text class="tllab ' + esc(e.kind) + '" x="' + (x + dx).toFixed(1) + '" y="' + y
          + '" text-anchor="' + anchor + '">' + esc(e.text.replace(/ \(re-applies on expiry\)/, '')) + '</text>';
      }).join('');
      var died = r.deaths[a.id];
      return '<div class="tlrow"><div class="tlwho"><b>' + esc(a.cls) + ' #' + a.id + '</b>'
        + '<i>' + (died != null ? 'dies ' + (Math.round(died * 10) / 10) + 's'
                                : Math.round(pts[pts.length - 1].hp) + ' HP left') + '</i></div>'
        + '<svg class="tlsvg" viewBox="0 0 ' + W + ' ' + H + '" preserveAspectRatio="none">'
        + grid
        + (show.marks ? marks : '')
        + (show.prot ? '<path class="tlprot" d="' + dr.join(' ') + '"/>' : '')
        + (show.pw ? '<path class="tlpw" d="' + dp.join(' ') + '"/>' : '')
        + (show.hp ? '<path class="tlhp" d="' + d.join(' ') + '"/>' : '')
        + (show.marks ? labSvg : '')
        + (died != null ? '<line class="tldead" x1="' + ((died / r.seconds) * W).toFixed(1)
            + '" y1="0" x2="' + ((died / r.seconds) * W).toFixed(1) + '" y2="' + H + '"/>' : '')
        + '</svg></div>';
    }

    // one line per distinct event, deduped - a 10s buff re-applied 30 times is not 30 events
    var seen = {}, evs = [];
    r.events.forEach(function (e) {
      var k = e.who + '|' + e.text;
      if (seen[k]) return;
      seen[k] = 1; evs.push(e);
    });
    evs.sort(function (a, b) { return a.t - b.t; });

    // the honest time to kill: what the run actually produced for the focused target, which
    // accounts for support drying up. The KPI panel's figure assumes everything stays up forever.
    var focusDied = r.deaths[sim.focus.to];
    var survived = r.S.byId[sim.focus.to];
    var verdict = focusDied != null
      ? '<b>' + (Math.round(focusDied * 10) / 10) + 's</b> to kill '
        + esc(survived ? survived.cls + ' #' + survived.id : 'the target')
      : (survived
          ? esc(survived.cls + ' #' + survived.id) + ' survives ' + r.seconds + 's on <b>'
            + Math.round(survived.hp) + ' HP</b>'
          : 'no target selected');

    host.innerHTML = '<div class="tlhead"><h4>Timeline</h4>'
      + '<span class="tlverdict">' + verdict + '</span>'
      + '<span class="tlnote">everyone fires from t=0 at what they have switched on; no movement, '
      + 'no misses. Buffs expire, power drains and cooldowns come back as the clock runs.</span>'
      + '<span class="tltog">'
        + [['hp', 'health'], ['pw', 'power'], ['prot', 'protection'], ['marks', 'events']]
            .map(function (k) {
              return '<button class="tlt k-' + k[0] + (show[k[0]] ? ' on' : '') + '" data-k="'
                + k[0] + '">' + k[1] + '</button>';
            }).join('') + '</span>'
      + '<label class="tllen">boosts at <input id="tl-morale" type="number" min="0" max="180" '
      + 'placeholder="never" value="' + (sim.moraleAt == null ? '' : sim.moraleAt) + '"'
      + ' title="when enough morale is banked to fire a boost - the earn rate is not in the data, '
      + 'so this is yours to set">s</label>'
      + '<label class="tllen">seconds <input id="tl-secs" type="number" min="5" max="180" value="'
      + seconds + '"></label>'
      + '<button id="tl-run">run</button></div>'
      + '<div class="tlrows">' + r.S.actors.map(spark).join('') + '</div>'
      + '<div class="tlaxis"><span class="tlwho"></span><div class="tlticks">'
        + (function () {
            var out = '';
            for (var q = 0; q <= seconds + 1e-9; q += tick) {
              out += '<i style="left:' + ((q / seconds) * 100).toFixed(2) + '%">' + q + 's</i>';
            }
            return out;
          })() + '</div></div>'
      + '<div class="tlevents">' + (evs.length
          ? evs.slice(0, 40).map(function (e) {
              var a = r.S.byId[e.who];
              return '<span class="tlev ' + esc(e.kind) + '"><i>' + e.t.toFixed(1) + 's</i>'
                + esc(a ? a.cls + ' #' + a.id : '') + ' &mdash; ' + esc(e.text) + '</span>';
            }).join('')
          : '<span class="tlev">nothing expires or runs out in this window</span>') + '</div>';

    host.querySelectorAll('.tlt').forEach(function (t2) {
      t2.addEventListener('click', function () {
        show[t2.dataset.k] = show[t2.dataset.k] ? 0 : 1;
        renderTimeline(seconds);
      });
    });
    var mi = document.getElementById('tl-morale');
    if (mi) mi.addEventListener('change', function () {
      sim.moraleAt = mi.value === '' ? null : Math.max(0, +mi.value);
      renderTimeline(seconds);
    });
    var b = document.getElementById('tl-run');
    if (b) b.addEventListener('click', function () {
      sim.moraleAt = mi && mi.value !== '' ? Math.max(0, +mi.value) : null;
      var v = +document.getElementById('tl-secs').value || 30;
      sim.tlSecs = Math.max(5, Math.min(180, v));
      renderTimeline(sim.tlSecs);
    });
  }

  function renderCombat() {
    var host = document.getElementById('cb-body'); if (!host) return;
    if (!charList().length) { host.innerHTML = '<p class="empty">No saved characters loaded.</p>'; return; }
    if (!sim.actors.length) {          // a sensible opening board
      addActor('A', 'Recon'); addActor('B', 'Assault');
    }
    sim.actors.forEach(function (a) { a.lastNotes = null; });   // recomputed every pass
    var thrown = allOutgoing();
    function inbound(id) {
      return thrown.filter(function (f) { return String(f.to) === String(id); });
    }
    var states = {};
    sim.actors.forEach(function (a) { states[a.id] = actorState(a, inbound(a.id)); });

    if (!actorById(sim.focus.from)) {
      var f = sim.actors.filter(function (a) { return a.team === 'A'; })[0];
      sim.focus.from = f ? f.id : null;
    }
    if (!actorById(sim.focus.to) || teamOf(sim.focus.to) === teamOf(sim.focus.from)) {
      var t = sim.actors.filter(function (a) { return a.team !== teamOf(sim.focus.from); })[0];
      sim.focus.to = t ? t.id : null;
    }

    var IMG = window.__DEVIMG__ || {};
    function actorCard(a) {
      var st = states[a.id]; if (!st) return '';
      var c = charOf(a.cls);
      var profs = c ? Object.keys(c.profiles).sort() : [];
      var protBits = [155, 156, 157, 324, 217, 218, 219].filter(function (p) { return st.prot[p]; })
        .map(function (p) {
          var short = (GA.statName(p) || '').replace('Protection - ', '');
          return '<span class="acp">' + esc(short) + ' <b>' + Math.floor(st.prot[p]) + '</b></span>';
        }).join('');
      var gear = (st.ctx.charGear || []).map(function (g, i) {
        var on = !!a.active[i];
        var dev = (window.__DEVMODEL__ || {})[String(g.id)];
        var tgt = dev ? GA.deviceTarget(dev, a.active[i]) : 'enemy';
        var aimed = a.aim[i] && actorById(a.aim[i]);
        var ic = IMG[String(g.id)] ? '<img src="' + IMG[String(g.id)] + '" alt="">' : '';
        var sc = deviceScope(a, g, dev, a.active[i]);
        var sel = '';
        if (on && sc.kind === 'all') {
          sel = '<span class="acall">&rarr; ' + esc(sc.label) + '</span>';
        } else if (on && sc.kind === 'single' && sc.picks.length) {
          sel = '<span class="actgt"><i>target</i>'
            + '<select class="acsel" data-a="' + a.id + '" data-i="' + i + '">'
            + sc.picks.map(function (t) {
                var me = String(t.id) === String(a.id);
                return '<option value="' + t.id + '"'
                  + (String(a.aim[i]) === String(t.id) ? ' selected' : '') + '>'
                  + esc(t.cls) + ' #' + t.id + (me ? ' (self)' : '') + '</option>';
              }).join('') + '</select></span>';
        }
        return '<span class="acgcell"><span class="acg' + (on ? ' on' : '') + ' t-' + tgt + '"'
          + ' data-a="' + a.id + '" data-i="' + i + '" data-tgt="' + tgt + '"'
          + ' title="' + esc(g.name) + ' — ' + esc(g.cat || '') + ', targets ' + esc(tgt) + '">'
          + ic + esc(g.name) + '</span>' + sel + '</span>';
      }).join('');
      var inb = st.inbound.map(function (f) {
        var from = actorById(f.from);
        return '<span class="acfx ' + f.kind + '">' + esc(f.name) + ' '
          + (f.v > 0 ? '+' : '') + fmt1(f.v) + (f.pct ? '%' : '')
          + '<i>' + esc(from ? from.cls + ' #' + from.id : '?') + '</i></span>';
      }).join('');
      var isFrom = String(sim.focus.from) === String(a.id);
      var isTo = String(sim.focus.to) === String(a.id);
      return '<div class="actor' + (isFrom ? ' isfrom' : '') + (isTo ? ' isto' : '') + '"'
        + ' data-drop="' + a.id + '">'
        + '<div class="achead"><span class="acdot ' + esc(a.cls.toLowerCase()) + '"></span>'
        + '<select class="accls" data-a="' + a.id + '">'
        + ['Assault', 'Medic', 'Recon', 'Robotics'].map(function (x) {
            return '<option' + (x === a.cls ? ' selected' : '') + '>' + x + '</option>';
          }).join('') + '</select>'
        + '<select class="acprof" data-a="' + a.id + '">'
        + profs.map(function (x) {
            return '<option value="' + x + '"' + (String(x) === String(a.pid) ? ' selected' : '')
              + '>p' + x + '</option>';
          }).join('') + '</select>'
        + '<span class="acid">#' + a.id + '</span>'
        + '<button class="acx" data-a="' + a.id + '" title="remove">&times;</button></div>'
        + '<div class="acstats"><span class="achp">' + st.maxHP + ' HP</span>' + protBits + '</div>'
        + '<div class="acskills">' + st.skills + ' pts &mdash; ' + trees2str(st.trees) + '</div>'
        + '<div class="acgear">' + gear + '</div>'
        + (inb ? '<div class="acinb"><span class="aclab">incoming</span>' + inb + '</div>' : '')
        + ((st.notes || []).length ? '<div class="acnote">'
            + st.notes.map(function (n) {
                // why a device refused to stay on: two effects in the same real category
                var what = (GA.CAT_NAMES && GA.CAT_NAMES[n.cat]) || ('category ' + n.cat);
                return esc(what.replace(/\s+$/, '')) + ': ' + esc(n.win) + ' wins, '
                  + esc((n.lost || []).join(', ')) + ' suppressed (' + esc(n.rule) + ')';
              }).join(' · ')
            + '</div>' : '')
        + '<div class="acfocus">'
        + '<button class="acf' + (isFrom ? ' on' : '') + '" data-f="from" data-a="' + a.id + '">attacking</button>'
        + '<button class="acf' + (isTo ? ' on' : '') + '" data-f="to" data-a="' + a.id + '">target</button>'
        + '</div></div>';
    }

    function teamCol(team, label) {
      var mine = sim.actors.filter(function (a) { return a.team === team; });
      return '<div class="team t' + team + '"><div class="teamhead"><h4>' + label + '</h4>'
        + '<button class="teamadd" data-team="' + team + '">+ add</button></div>'
        + (mine.map(actorCard).join('') || '<p class="empty">empty side</p>') + '</div>';
    }

    // ---- the solve: focus attacker shooting focus target ----
    var solve = '<p class="empty">Pick an attacker and a target.</p>';
    var A = actorById(sim.focus.from), T = actorById(sim.focus.to);
    if (A && T && states[A.id] && states[T.id]) {
      var sa = states[A.id], stt = states[T.id];
      var heal = healingOn(T.id, thrown);
      var shots = actorShots(sa).map(function (w) {
        // A buff-stripper's damage is its base hit plus its per-category payout for every buff
        // category actually on the target. Against a heavily-buffed target this dwarfs the base.
        var extra = 0, stripped = [];
        (w.strip || []).forEach(function (sg) {
          var hit = (sg.cats || []).filter(function (cc) { return stt.cats[cc]; });
          if (hit.length) { extra += sg.dmg; stripped = stripped.concat(hit); }
        });
        var m = GA.mitigate(w.raw + extra, { cat: w.cat, damageType: w.hit.dmg,
                                             attackType: w.hit.atk, rating: w.hit.rating },
                            stt.prot, {});
        var dps = (w.refire && w.refire > 0) ? m.dealt / w.refire : null;
        return { w: w, m: m, dps: dps, extra: extra, stripped: stripped };
      }).sort(function (x, y) { return (y.dps || 0) - (x.dps || 0); });

      var best = shots.filter(function (s) { return s.dps; })[0] || shots[0];
      var rows = shots.map(function (r) {
        var w = r.w, m = r.m;
        var ic = IMG[String(w.id)];
        return '<tr' + (r === best ? ' class="bestrow"' : '') + '>'
          + '<td>' + (ic ? '<img src="' + ic + '" alt="">' : '') + esc(w.name)
          + ' <i>' + esc(w.mode) + '</i>'
          + (r.extra ? '<span class="strip">+' + Math.round(r.extra) + ' stripping '
              + r.stripped.length + ' buff' + (r.stripped.length === 1 ? '' : 's') + '</span>' : '')
          + '</td>'
          + '<td class="num">' + Math.round(w.raw + (r.extra || 0)) + '</td>'
          + '<td class="num"><b>' + m.shown + '</b></td>'
          + '<td class="num">' + m.mitPct.toFixed(1) + '%</td>'
          + '<td class="num">' + (w.refire ? (Math.round(w.refire * 100) / 100) + 's' : '&mdash;') + '</td>'
          + '<td class="num">' + (r.dps ? fmt1(r.dps) : '&mdash;') + '</td>'
          + '<td class="axes">' + m.axes.map(function (x) {
              return esc((GA.statName(x.prop) || '').replace('Protection - ', ''))
                + ' ' + x.protection + ' &rarr; &minus;' + (x.reduction * 100).toFixed(0) + '%';
            }).join(' &times; ') + '</td></tr>';
      }).join('');

      // power sustain: the pool divided by what firing costs per second
      var pool = sa.pool;
      var drain = (best && best.w.power && best.w.refire) ? best.w.power / best.w.refire : 0;
      var sustain = drain > 0 ? pool / drain : null;

      var dps = best && best.dps ? best.dps : 0;
      var net = dps - heal.hps;
      var ttk = net > 0 ? stt.maxHP / net : null;

      solve = '<div class="solvehead">'
        + '<b>' + esc(A.cls) + ' #' + A.id + '</b> &rarr; <b>' + esc(T.cls) + ' #' + T.id + '</b>'
        + '<span class="solvenote">best sustained weapon, target at full health</span></div>'
        + '<div class="solvekpi">'
        + '<div class="kpi"><i>damage / s</i><b>' + fmt1(dps) + '</b></div>'
        + '<div class="kpi' + (heal.hps ? ' heal' : '') + '"><i>healing / s</i><b>'
          + fmt1(heal.hps) + '</b></div>'
        + '<div class="kpi"><i>net / s</i><b>' + fmt1(net) + '</b></div>'
        + '<div class="kpi"><i>power lasts</i><b>'
          + (sustain === null ? '&mdash;' : fmt1(sustain) + 's') + '</b></div>'
        + '<div class="kpi"><i>target</i><b>' + stt.maxHP + ' HP</b></div>'
        + '</div>'
        + (heal.srcs.length ? '<div class="healsrc">healed by '
            + heal.srcs.map(function (h) {
                return '<span>' + esc(h.who) + ' ' + esc(h.dev) + ' ' + fmt1(h.hps) + '/s'
                  + (h.hot ? ' <i>hot</i>' : '') + '</span>';
              }).join('') + '</div>' : '')
        + (ttk === null && heal.hps > 0
            ? '<p class="solvewarn">Healing meets or beats the incoming damage &mdash; this target '
              + 'does not go down to this weapon alone.</p>' : '')
        + '<table class="shottab"><thead><tr><th>weapon</th><th>raw</th><th>dealt</th>'
        + '<th>mit</th><th>refire</th><th>dps</th><th>axes</th></tr></thead><tbody>'
        + (rows || '<tr><td colspan="7">no damaging weapon</td></tr>') + '</tbody></table>';
    }

    host.innerHTML =
      '<div class="simboard">' + teamCol('A', 'Team A') + teamCol('B', 'Team B') + '</div>'
      + '<div id="cb-timeline" class="timeline"></div>'
      + '<div class="solve">' + solve + '</div>'
      + '<p class="cbtnote">Drag a switched-on device onto a combatant to aim it there &mdash; a '
      + 'weapon only drops on the other team, a heal or buff only on its own. Protection is flat, '
      + 'divided by the weapon&rsquo;s attack rating, and the axes <b>multiply</b>. One attack-type '
      + 'axis applies per shot: a splash radius puts it on AOE <em>instead of</em> ranged. '
      + '<b>Not yet modelled:</b> buffs expiring mid-fight, cooldowns, and reloads &mdash; DPS here '
      + 'is sustained fire with everything up.</p>';

    wireCombat(host);
    renderTimeline(sim.tlSecs || 30);
  }

  function wireCombat(host) {
    host.querySelectorAll('.accls').forEach(function (s) {
      s.addEventListener('change', function () {
        var a = actorById(s.dataset.a); if (!a) return;
        a.cls = s.value; a.active = {}; a.aim = {};
        var c = charOf(a.cls);
        a.pid = c ? Object.keys(c.profiles).sort()[0] : null;
        renderCombat();
      });
    });
    host.querySelectorAll('.acprof').forEach(function (s) {
      s.addEventListener('change', function () {
        var a = actorById(s.dataset.a); if (!a) return;
        a.pid = s.value; a.active = {}; a.aim = {};
        renderCombat();
      });
    });
    host.querySelectorAll('.acx').forEach(function (b) {
      b.addEventListener('click', function () {
        sim.actors = sim.actors.filter(function (x) { return String(x.id) !== String(b.dataset.a); });
        // anything aimed at the departed loses its aim
        sim.actors.forEach(function (x) {
          Object.keys(x.aim).forEach(function (k) {
            if (String(x.aim[k]) === String(b.dataset.a)) delete x.aim[k];
          });
        });
        renderCombat();
      });
    });
    host.querySelectorAll('.teamadd').forEach(function (b) {
      b.addEventListener('click', function () {
        addActor(b.dataset.team, b.dataset.team === 'A' ? 'Recon' : 'Assault');
        renderCombat();
      });
    });
    host.querySelectorAll('.acf').forEach(function (b) {
      b.addEventListener('click', function () {
        sim.focus[b.dataset.f] = +b.dataset.a;
        renderCombat();
      });
    });
    // click a device to switch it on/off; default aim is the obvious one
    host.querySelectorAll('.acg').forEach(function (g) {
      g.addEventListener('click', function () {
        var a = actorById(g.dataset.a); if (!a) return;
        var i = g.dataset.i;
        if (a.active[i]) { delete a.active[i]; delete a.aim[i]; }
        else {
          var ctx = actorCtx(a);
          a.active[i] = 'PRI';
          stowOtherWeapons(a, ctx, i);
          var dev0 = (window.__DEVMODEL__ || {})[String((ctx.charGear || [])[+i].id)];
          var sc0 = deviceScope(a, (ctx.charGear || [])[+i], dev0, 'PRI');
          if (sc0.kind === 'single' && sc0.picks.length) a.aim[i] = sc0.picks[0].id;
        }
        renderCombat();
      });
    });
    host.querySelectorAll('.acsel').forEach(function (s2) {
      s2.addEventListener('change', function () {
        var a = actorById(s2.dataset.a); if (!a) return;
        a.aim[s2.dataset.i] = +s2.value;
        renderCombat();
      });
    });
  }

  function renderSheet() {
    var host = document.getElementById('tb-sheet');
    if (!host) return;
    var col = collectStats();
    var stats = col.stats, picked = col.picked, act = col.act, shieldBy = col.shieldBy;
    var eqb = col.eqb;
    var keys = Object.keys(stats);

    // ---- derived headline totals -------------------------------------------
    // Layers multiply (TgPawn::ApplyBuff: v1 = base*(1+itemPct), v2 = v1*(1+skillPct)).
    // Skills are one layer today; armour/devices will slot in as their own layer later.
    var dv = deriveTotals(stats);
    var baseOf = dv.baseOf, pctFor = dv.pctFor;
    var baseHP = dv.baseHP, basePW = dv.basePW;
    var hpItem = dv.hpItem, hpSkill = dv.hpSkill, pwItem = dv.pwItem, pwSkill = dv.pwSkill;
    var totHP = dv.totHP, totPW = dv.totPW;
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
        // Colour by whether the number HELPS you, not by its sign: a cooldown or power-cost
        // reduction is a gain, and it was reading as a loss.
        var cls = benefit(st.p, st.total);
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
          var scls = benefit(st.p, s.val);
          html += '<div class="srcline' + (s.base ? ' isbase' : '') + (s.armour ? ' isarm' : '')
            + (s.active ? ' isact' : '') + '"><span class="srctree' + (s.base ? ' basetag' : '')
            + (s.active ? ' acttag' : '') + '">' + esc(s.tree) + '</span>'
            + '<span class="srcskill">' + esc(s.skill) + '</span>'
            + (s.kind !== 'passive' ? '<span class="srckind' + (s.dormant ? ' dormant' : '') + '">'
                + s.kind + (s.dormant ? ' · not active' : '') + '</span>' : '')
            + (s.life ? '<span class="srckind">' + s.life + 's</span>' : '')
            + '<span class="srcval ' + (s.dormant ? 'off' : scls) + '">'
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
      var cbv = document.getElementById('view-combat');
      if (cbv) cbv.style.display = (v === 'combat' ? '' : 'none');
      if (v === 'combat') renderCombat();
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
