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
  // __ACCTS__ holds every baked-in account; __CHARS__ is whichever one is selected, so all
  // the existing readers of __CHARS__ carry on unchanged. Inventory is seeded identically for
  // everyone, but the row IDS differ per account - and those ids are what an export writes back
  // with - so switching account genuinely changes what a build resolves to.
  var CH = window.__CHARS__ || null;
  function acctList() { return (window.__ACCTS__ || {}).accounts || []; }
  function acctById(uid) {
    return acctList().filter(function (a) { return String(a.user) === String(uid); })[0] || null;
  }
  function activeAcct() { return CH; }
  function setAcct(uid) {
    var a = acctById(uid);
    if (!a) return false;
    CH = window.__CHARS__ = a;
    curChar = null; curProfile = null;   // the old character ids belong to the old account
    charArm = null; charGear = [];
    return true;
  }
  var curChar = null, curProfile = null, charArm = null, charGear = [];
  var craftName = '';       // what the build on the bench is called
  var craftEditing = null;  // id of the saved build being edited, if any
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
        + '">' + classIcon(c.cls, 'sm') + esc(c.cls) + '</button>';
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
  // Thirteen melee weapons carry a from-behind rider. Asked of the model so the card can show
  // the switch without having to build a sim device first.
  function hasBackstab(g, mode) {
    var dev = (window.__DEVMODEL__ || {})[String(g && g.id)];
    if (!dev) return false;
    return (dev.modes || []).some(function (m) {
      if (mode && m.kind && m.kind !== mode && m.kind !== 'SPAWN') return false;
      return (m.chips || []).some(function (c) { return /^Backstab: /.test(c[1]); });
    });
  }
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
        // A SPAWN mode describes the thing the device PUTS DOWN, not the person carrying it.
        // Force Wall and Dome Shield Boost each spawn a ForceField Device whose own equip
        // effects are +75 Physical and +1000 Bio/Ignite/Bleed/Disease - a structure shrugging
        // off damage-over-time. Folding those into the carrier gave a Robotics 189 Physical
        // against a rating of 100, which CalcProtection reads as flat immunity: it could be
        // shot all day and never lose a point of health.
        if (m.kind === 'SPAWN') return;
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
  // put a saved build back on the bench. Devices are matched to slots by category so a build
  // that came from the database drops into the same layout a hand-made one uses.
  function loadCraftFromBuild(b) {
    if (!b) return;
    craftMode = true;
    // Re-saving under a different account would stamp that account's inventory ids onto a build
    // belonging to another - switch back to the one it was made under.
    if (b.acct && CH && String(b.acct) !== String(CH.user)) setAcct(b.acct);
    craftEditing = b.id;
    craftName = b.name || '';
    curClass = b.cls;
    document.querySelectorAll('.tb-class').forEach(function (x) {
      x.classList.toggle('active', x.dataset.cls === curClass);
    });
    alloc = {};
    (b.skills || []).forEach(function (sid) { if (nodeIndex[sid]) alloc[sid] = 1; });
    if (b.armSlots) armSlots = b.armSlots.slice();
    charArm = (b.armour && b.armour.length) ? b.armour : null;
    craftSlots = blankSlots();
    (b.devices || []).forEach(function (d) {
      var slot = null;
      for (var i = 0; i < craftSlots.length; i++) {
        if (craftSlots[i].cat === d.cat && !craftSlots[i].id) { slot = craftSlots[i]; break; }
      }
      if (!slot) return;
      slot.id = String(d.id);
      var dev = (window.__DEVMODEL__ || {})[String(d.id)];
      var vs = (dev && dev.variants) || [];
      var vix = 0;
      vs.forEach(function (v, k) { if (v.sig === d.sig) vix = k; });
      slot.vix = vix;
    });
    craftToGear();
  }

  // Show the payload rather than pushing it anywhere - there is no backend yet, and this is
  // the shape a sync would POST. Refusals are explained instead of the button just not working.
  function showExport(b, target, rd) {
    var host = document.getElementById('tc-export');
    if (!host) return;
    if (!rd.ok) {
      host.innerHTML = '<div class="tcexpbox bad"><b>Cannot export</b> &mdash; ' + esc(rd.why)
        + '. <i>Builds can be made for any class, but only sent to a character that exists.</i>'
        + '</div>';
      return;
    }
    var pid = (b.origin && b.origin.profileId) || '1';
    var payload = exportPayload(b, target.id, pid);
    host.innerHTML = '<div class="tcexpbox">'
      + '<div class="tcexphead"><b>' + esc(b.name) + '</b> &rarr; '
        + classIcon(b.cls, 'sm')
        + (rd.targets.length > 1
            ? '<select id="tc-exp-char">'
              + rd.targets.map(function (t) {
                  return '<option value="' + t.id + '"'
                    + (String(t.id) === String(target.id) ? ' selected' : '') + '>'
                    + esc(b.cls) + ' #' + t.id + '</option>';
                }).join('') + '</select>'
            : esc(b.cls) + ' character #' + target.id)
        + ' on ' + esc((acctOf(b) || {}).name || '?')
        + ', item profile <select id="tc-exp-prof">'
        + ['1', '2', '3', '4', '5'].map(function (x) {
            return '<option' + (x === String(pid) ? ' selected' : '') + '>' + x + '</option>';
          }).join('') + '</select>'
        + (rd.missing.length
            ? '<span class="tcwarn">' + rd.missing.length + ' device(s) have no inventory row '
              + 'and are omitted: ' + esc(rd.missing.map(function (d) { return d.name; }).join(', '))
              + '</span>'
            : '')
        + '<button id="tc-exp-copy" class="tcbtn">copy</button></div>'
      + '<textarea id="tc-exp-json" readonly>' + esc(JSON.stringify(payload, null, 2))
      + '</textarea></div>';
    var ps = document.getElementById('tc-exp-prof');
    var cs = document.getElementById('tc-exp-char');
    function repaint() {
      document.getElementById('tc-exp-json').value = JSON.stringify(
        exportPayload(b, cs ? cs.value : target.id, ps ? ps.value : 1), null, 2);
    }
    if (ps) ps.addEventListener('change', repaint);
    if (cs) cs.addEventListener('change', repaint);
    var cp = document.getElementById('tc-exp-copy');
    if (cp) cp.addEventListener('click', function () {
      var ta = document.getElementById('tc-exp-json');
      ta.select();
      try { document.execCommand('copy'); cp.textContent = 'copied'; } catch (e) {}
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
    var saved = builds();
    host.innerHTML = '<div class="chhead"><h3>TheoryCrafter</h3>'
      + '<span class="chname">' + esc(curClass) + '</span>'
      + '<span class="tccount">' + filled + ' / ' + craftSlots.length + ' slots</span>'
      + '<button class="chclear" id="tc-clear">clear</button></div>'
      + '<div class="tcbar">'
      + '<input id="tc-name" class="tcname" placeholder="build name" value="'
        + esc(craftName || '') + '">'
      + '<button id="tc-save" class="tcbtn primary">save build</button>'
      + '<button id="tc-add-a" class="tcbtn">save &amp; send to Team A</button>'
      + '<button id="tc-add-b" class="tcbtn">save &amp; send to Team B</button>'
      + '<span class="tcsep"></span>'
      + (acctList().length > 1
          ? '<select id="tc-acct" title="which account\u2019s characters and inventory to use">'
            + acctList().map(function (a) {
                return '<option value="' + a.user + '"'
                  + (CH && String(a.user) === String(CH.user) ? ' selected' : '') + '>'
                  + esc(a.name) + '</option>';
              }).join('') + '</select>'
          : '')
      // No accounts baked in means nothing to import from and nowhere to export to, so the
      // whole live-profile apparatus disappears rather than sitting there refusing. This is
      // what the shareable build looks like: you make builds yourself.
      + (charList().length
          ? '<select id="tc-import"><option value="">import a live profile&hellip;</option>'
            + (charList() || []).map(function (c) {
                var dupe = (charList() || []).filter(function (x) { return x.cls === c.cls; }).length > 1;
                return Object.keys(c.profiles).sort().map(function (pid) {
                  return '<option value="' + c.id + '|' + pid + '">' + esc(c.cls)
                    + (dupe ? ' #' + c.id : '') + ' profile ' + pid + '</option>';
                }).join('');
              }).join('') + '</select>'
          : '')
      + '</div>'
      + (saved.length
          ? '<div class="tcsaved"><span class="aclab">saved builds</span>'
            + saved.map(function (b) {
                if (!acctList().length) {
                  return '<span class="tcsav"><button class="tcload" data-b="' + b.id + '">'
                    + classIcon(b.cls, 'sm') + esc(b.name) + '</button>'
                    + '<button class="tcdel" data-b="' + b.id + '" title="delete">&times;</button>'
                    + '</span>';
                }
                var rd = exportReadiness(b);
                return '<span class="tcsav"><button class="tcload" data-b="' + b.id + '">'
                  + classIcon(b.cls, 'sm') + esc(b.name) + '</button>'
                  + '<button class="tcexp' + (rd.ok ? '' : ' off') + '" data-b="' + b.id + '"'
                    + ' title="' + esc(rd.ok
                        ? ('export for ' + rd.targets.length + ' ' + b.cls + ' character'
                           + (rd.targets.length > 1 ? 's' : '')
                           + ' on ' + ((acctOf(b) || {}).name || '?')
                           + (rd.missing.length ? ' \u2014 ' + rd.missing.length
                              + ' device(s) have no inventory row and will be skipped' : ''))
                        : rd.why) + '">export</button>'
                  + '<button class="tcdel" data-b="' + b.id + '" title="delete">&times;</button>'
                  + '</span>';
              }).join('')
            + '</div>'
          : '<p class="chnote">No saved builds yet. Fill the slots, name it and save'
            + (charList().length
                ? ' &mdash; or import a live profile to start from something real.'
                : ', then send it to a side on the Combat tab.') + '</p>')
      + '<div id="tc-export"></div>'
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
    var nameBox = document.getElementById('tc-name');
    if (nameBox) nameBox.addEventListener('input', function () { craftName = nameBox.value; });

    function saveCurrent() {
      var b = buildFromCraft(nameBox && nameBox.value);
      if (craftEditing) b.id = craftEditing;          // keep editing the same build
      putBuild(b);
      craftEditing = b.id;
      return b;
    }
    var sv = document.getElementById('tc-save');
    if (sv) sv.addEventListener('click', function () { saveCurrent(); render(); });
    ['a', 'b'].forEach(function (side) {
      var btn = document.getElementById('tc-add-' + side);
      if (!btn) return;
      btn.addEventListener('click', function () {
        var b = saveCurrent();
        addActor(side.toUpperCase(), b.id);
        render();
        var tab = [].slice.call(document.querySelectorAll('.viewtab'))
          .filter(function (x) { return x.dataset.view === 'combat'; })[0];
        if (tab) tab.click();
      });
    });
    var ac = document.getElementById('tc-acct');
    if (ac) ac.addEventListener('change', function () {
      if (setAcct(ac.value)) { renderChar(); render(); }
    });
    var imp = document.getElementById('tc-import');
    if (imp) imp.addEventListener('change', function () {
      if (!imp.value) return;
      var parts = imp.value.split('|');
      var b = buildFromProfile(parts[0], parts[1]);
      if (b) { putBuild(b); loadCraftFromBuild(b); }
      render();
    });
    host.querySelectorAll('.tcload').forEach(function (b2) {
      b2.addEventListener('click', function () {
        var bb = buildById(b2.dataset.b);
        if (bb) { loadCraftFromBuild(bb); render(); }
      });
    });
    host.querySelectorAll('.tcexp').forEach(function (b2) {
      b2.addEventListener('click', function () {
        var b = buildById(b2.dataset.b);
        var rd = exportReadiness(b);
        if (!rd.ok) { showExport(b, null, rd); return; }
        showExport(b, rd.targets[0], rd);
      });
    });
    host.querySelectorAll('.tcdel').forEach(function (b2) {
      b2.addEventListener('click', function () {
        delBuild(b2.dataset.b);
        if (String(craftEditing) === String(b2.dataset.b)) craftEditing = null;
        render();
      });
    });
    var cl = document.getElementById('tc-clear');
    if (cl) cl.addEventListener('click', function () {
      craftSlots = blankSlots(); craftClass = curClass;
      alloc = {}; activeGear = {}; activeOrder = [];
      // Clearing means "start a new build". Without this the next save kept the id of the one
      // just saved and quietly overwrote it - build an Assault, send it, build a Recon, and the
      // Assault was gone. Only noticeable once building by hand is the whole workflow.
      craftEditing = null; craftName = '';
      craftToGear(); render();
    });
  }

  function treesFor(cls) { return D.classes[cls]; }
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
  // sched[actorId][slot] = { from: seconds|null, uses: [seconds] }
  //   uses non-empty -> that device fires ONLY at those moments (still gated by its cooldown)
  //   otherwise      -> it behaves as before, but not before `from`
  // Leaving a device alone keeps the old behaviour, so an untouched board still runs itself.
  var sim = { actors: [], focus: { from: null, to: null }, nextId: 1, drag: null, sched: {} };
  // A device that has a refire fires continuously once it starts, so placing a marker on it
  // means "open fire at this moment", not "loose a single round". Everything else - off-hands,
  // boosts, waves - is a discrete press, and each marker is one activation.
  function isContinuous(devId, mode) {
    var dev = (window.__DEVMODEL__ || {})[String(devId)];
    if (!dev) return false;
    var ms = (dev.modes || []);
    for (var i = 0; i < ms.length; i++) {
      if (mode && ms[i].kind && ms[i].kind !== mode) continue;
      var ch = ms[i].chips || [];
      for (var k = 0; k < ch.length; k++) {
        if (Array.isArray(ch[k][2]) && ch[k][2][0] === 53) return true;
      }
    }
    return false;
  }
  function schedOf(actorId, slot) {
    var a = sim.sched[actorId] || (sim.sched[actorId] = {});
    return a[slot] || (a[slot] = { from: null, uses: [] });
  }

  function charList() { var C = window.__CHARS__ || {}; return C.chars || (C.length ? C : []); }
  function charOf(cls) { return charList().filter(function (c) { return c.cls === cls; })[0]; }

  function addActor(team, buildId) {
    var b = buildById(buildId) || builds()[0];
    if (!b) return null;
    var a = { id: sim.nextId++, team: team, buildId: b.id, cls: b.cls, active: {}, aim: {} };
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
        // "Enemy and Self" means a grenade CAN catch its thrower, and in game it does. For a
        // build comparison that is noise - nobody plans to stand in their own poison - so the
        // console aims it at the other side only.
        return { kind: 'all', targets: enemies.map(idOf), label: 'all enemies' };
      }
      return { kind: 'all', targets: sim.actors.map(idOf), label: 'everyone' };
    }
    var picks = tgt === 'friend' ? mates
      : tgt === 'enemy' ? enemies
        : tgt === 'enemyself' ? enemies
          : sim.actors;
    return { kind: 'single', picks: picks, targets: [], label: '' };
  }
  function idOf(x) { return x.id; }

  // Class artwork, inlined by gen3 from assets/class-icons. classIcon('Recon') for a combatant,
  // classIcon('death') for the marker on a kill.
  function classIcon(name, extra) {
    var src = (window.__CLASSIMG__ || {})[name];
    if (!src) return '';
    return '<img class="clsic' + (extra ? ' ' + extra : '') + '" src="' + src + '" alt="'
      + esc(name) + '">';
  }

  // Actors are saved builds now, not live database profiles. The database is still the way
  // builds get INTO the console - "import" on the TheoryCrafter pulls a profile verbatim - but
  // the simulator only ever reads from the build store, so a build can be edited freely without
  // touching anything the game owns.
  function actorCtx(a) {
    return ctxFromBuild(buildById(a.buildId), a.active);
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
        var g = (ctx.charGear || [])[+slot]; if (!g) return;
        var dev = (window.__DEVMODEL__ || {})[String(g.id)]; if (!dev) return;
        if (GA.deviceTarget(dev, h.active[slot]) === 'enemy') return;
        // An AOE heal has no single aim - it reaches everyone in its scope. Matching only on
        // aim[slot] meant every wave, grenade and boost was invisible here, so a team being
        // kept alive by area healing reported "healing / s 0".
        var sc = deviceScope(h, g, dev, h.active[slot]);
        var reaches = sc.kind === 'all'
          ? sc.targets.some(function (x) { return String(x) === String(actorId); })
          : String(h.aim[slot]) === String(actorId);
        if (!reaches) return;
        var res = GA.resolve({ dev: dev, meta: (window.__DEVMETA__ || {})[String(g.id)] || {},
          ix: (window.__DEVFX__ || {})[String(g.id)] || [], alloc: col.alloc, situational: true,
          buffs: col.buffs, variant: { sig: g.sig, base: g.base, groups: g.groups, nums: g.nums } });
        (res.modes || []).forEach(function (m) {
          if (m.kind && h.active[slot] && m.kind !== h.active[slot]) return;
          // How often this thing can actually be used. A Healing Grenade has no refire and a
          // 60s cooldown, so treating its 320-point heal as "per second" reported 571/s for a
          // device that goes off once a minute.
          var refire = null, cooldown = null, persist = null;
          (m.chips || []).forEach(function (c) {
            if (c.prop === 53) refire = c.value;
            if (c.prop === 4) cooldown = c.value;
            if (c.prop === 150) persist = c.value;
          });
          var every = refire || cooldown || persist || null;
          (m.chips || []).forEach(function (c) {
            if (c.prop !== 51 || c.sign < 0 || !c.value) return;
            // "Self: Heal" is the medic topping THEMSELVES up while they beam. It only counts
            // toward the person being healed when that person is the medic.
            if (c.self && String(h.id) !== String(actorId)) return;
            // a heal-over-time delivers its value across its lifetime; a direct heal per refire
            // a heal-over-time delivers its value across its own lifetime; a direct heal
            // delivers it once per use, so it averages out over the interval between uses
            var per = c.life > 0 ? (c.value / c.life) : (every ? c.value / every : c.value);
            hps += per;
            srcs.push({ who: h.cls + ' #' + h.id, dev: g.name, hps: per, hot: c.life > 0,
                        every: every });
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

  // ---- export: can this build actually be pushed anywhere? ---------------------
  // A build is only exportable if the account has a character of that class to receive it. You
  // can theorycraft a Recon with no Recon character - that is useful - but there is nowhere to
  // send it, so the export is refused rather than producing a payload with no target.
  //
  // A device also needs its inventory row: ga_character_devices stores inventory_id, not a
  // device id, so a hand-built device that was never owned cannot be written back. Those are
  // listed rather than silently dropped.
  function acctOf(b) {
    return (b && b.acct && acctById(b.acct)) || activeAcct();
  }
  function targetsFor(cls, b) {
    var a = acctOf(b);
    return ((a && a.chars) || []).filter(function (c) { return c.cls === cls; });
  }
  function exportReadiness(b) {
    if (!b) return { ok: false, why: 'no build' };
    var a = acctOf(b);
    var targets = targetsFor(b.cls, b);
    if (!targets.length) {
      return { ok: false, targets: [],
               why: 'no ' + b.cls + ' character on ' + ((a && a.name) || 'this account')
                    + ' to sync to' };
    }
    var missing = (b.devices || []).filter(function (d) { return !d.inv; });
    return { ok: true, targets: targets, missing: missing };
  }

  // The payload is a profile plus enough identity to write it: ga_character_devices wants
  // (character_id, item_profile_id, inventory_id, equipped_slot) and ga_character_skills wants
  // (character_id, item_profile_id, skill_group_id, skill_id, points). points is always 1.
  function exportPayload(b, charId, profileId) {
    var devices = (b.devices || []).filter(function (d) { return d.inv; })
      .map(function (d) {
        return { equipped_slot: d.slot, inventory_id: d.inv,
                 device_id: d.id, mod_effect_group_ids: d.mods || '', name: d.name };
      });
    (b.armour || []).forEach(function (a2) {
      if (!a2.inv) return;
      devices.push({ equipped_slot: a2.eslot || a2.slot, inventory_id: a2.inv,
                     mod_effect_group_ids: a2.mods || '', name: a2.name });
    });
    return {
      format: 'ga-console-build/1',
      character_id: charId == null ? null : +charId,
      item_profile_id: profileId == null ? null : +profileId,
      cls: b.cls, name: b.name,
      origin: b.origin || null,
      devices: devices,
      skills: (b.skills || []).map(function (sid) {
        return { skill_group_id: groupOf(sid, b.cls), skill_id: sid, points: 1 };
      })
    };
  }

  // ======================= SAVED BUILDS =======================
  // A saved build IS a character profile, field for field: devices / armour / skills in exactly
  // the shape __CHARS__ delivers them, plus a name and where it came from. That is deliberate -
  // the intended end state is logging in with game credentials, pulling your live profiles,
  // editing them here and syncing back. Keeping one shape means import is a copy and export is a
  // copy, with no translation layer to drift.
  //
  //   { id, name, cls, origin: {charId, profileId} | null,
  //     devices: [{slot, id, name, oc, cat, sig, base, groups, nums}],
  //     armour:  [{slot, name, sig, base, groups, nums}] | null,
  //     skills:  [skillId, ...] }
  //
  // Strip id/name/origin and what remains is a profile ready to write back.
  var BKEY = 'ga.console.builds.v1';

  function builds() {
    try { return JSON.parse(localStorage.getItem(BKEY)) || []; } catch (e) { return []; }
  }
  function writeBuilds(list) {
    try { localStorage.setItem(BKEY, JSON.stringify(list)); } catch (e) {}
  }
  function buildById(id) {
    return builds().filter(function (b) { return String(b.id) === String(id); })[0] || null;
  }
  function putBuild(b) {
    var list = builds();
    var i = -1;
    list.forEach(function (x, k) { if (String(x.id) === String(b.id)) i = k; });
    if (i >= 0) list[i] = b; else list.push(b);
    writeBuilds(list);
    return b;
  }
  function delBuild(id) {
    writeBuilds(builds().filter(function (b) { return String(b.id) !== String(id); }));
  }
  function nextBuildId() {
    var n = 1;
    builds().forEach(function (b) {
      var m = /^b(\d+)$/.exec(String(b.id));
      if (m && +m[1] >= n) n = +m[1] + 1;
    });
    return 'b' + n;
  }

  // the profile half on its own - what would be written back to the game
  function buildToProfile(b) {
    return { devices: b.devices || [], armour: b.armour || null, skills: b.skills || [] };
  }

  // pull a live character profile in. This is the shape a credentialed login would hand us.
  function buildFromProfile(charId, profileId, name) {
    var c = (charList() || []).filter(function (x) { return String(x.id) === String(charId); })[0];
    if (!c) return null;
    var p = c.profiles[profileId];
    if (!p) return null;
    // YeXiuu has two Assaults, so "Assault p1" alone names two different builds. The import
    // list already tags the character id when a class is duplicated; the saved build has to
    // carry it too or the cards on the board are indistinguishable.
    var dupe = (charList() || []).filter(function (x) { return x.cls === c.cls; }).length > 1;
    var auto = c.cls + (dupe ? ' #' + c.id : '') + ' p' + profileId;
    return { id: nextBuildId(), name: name || auto, cls: c.cls,
             acct: (CH && CH.user) || null,
             origin: { charId: c.id, profileId: String(profileId) },
             devices: JSON.parse(JSON.stringify(p.devices || [])),
             armour: p.armour && p.armour.length ? JSON.parse(JSON.stringify(p.armour)) : null,
             skills: (p.skills || []).slice() };
  }

  // and the TheoryCrafter half - the same shape, assembled from what is on screen
  function buildFromCraft(name) {
    // An inventory row is a specific rolled instance, so it only carries over when the device
    // and its mod signature are both unchanged. Pick a different variant and you have described
    // a roll you do not own - the export drops it rather than writing the wrong item.
    // __CHARS__.inv is every inventory row the account owns, keyed device -> roll signature,
    // so any owned item resolves - not just one that happens to be equipped on a profile. Pick a
    // roll the account does not own and there is deliberately no match: the export drops that
    // slot rather than writing an item that does not exist.
    var OWNED = (window.__CHARS__ || {}).inv || {};
    var prevDevs = (craftEditing ? ((buildById(craftEditing) || {}).devices || []) : []).slice();
    function claimInv(id, sig) {
      var owned = OWNED[String(id)] && OWNED[String(id)][sig || ''];
      if (owned) return { inv: owned[0], mods: owned[1] };
      // an imported build may carry a row that predates this snapshot
      for (var k = 0; k < prevDevs.length; k++) {
        var pd = prevDevs[k];
        if (pd && +pd.id === +id && (pd.sig || '') === (sig || '') && pd.inv) {
          prevDevs[k] = null;
          return pd;
        }
      }
      return null;
    }
    var devices = [];
    craftSlots.forEach(function (sl, i) {
      if (!sl.id) return;
      var dev = (window.__DEVMODEL__ || {})[String(sl.id)];
      if (!dev) return;
      var v = (dev.variants || [])[sl.vix] || (dev.variants || [])[0] || {};
      var own = claimInv(sl.id, v.sig);
      var rec = { slot: i + 1, id: +sl.id, name: dev.name, oc: !!dev.oc, cat: dev.cat,
                  sig: v.sig || '', base: v.base || null,
                  groups: v.groups || [], nums: v.nums || [] };
      if (own) { rec.inv = own.inv; rec.mods = own.mods || ''; }
      devices.push(rec);
    });
    // Keep the real rolled armour and the origin when the bench came from an imported profile.
    // Dropping them made a round-trip lossy - a re-saved import fell back to preset armour (3637
    // HP became 3425) and forgot which character and profile it belonged to, which is exactly
    // what a future "sync to game" needs to write back to.
    var prev = craftEditing ? buildById(craftEditing) : null;
    return { id: nextBuildId(), name: name || (curClass + ' build'), cls: curClass,
             acct: (prev && prev.acct) || (CH && CH.user) || null,
             origin: prev ? (prev.origin || null) : null,
             devices: devices,
             armour: (charArm && charArm.length) ? JSON.parse(JSON.stringify(charArm))
                     : (prev && prev.armour ? prev.armour : null),
             armSlots: armSlots.slice(),
             skills: Object.keys(alloc).map(Number) };
  }

  // a saved build, in the form statsFor wants
  function ctxFromBuild(b, active) {
    if (!b) return null;
    var al = {};
    (b.skills || []).forEach(function (sid) { if (nodeIndex[sid]) al[sid] = 1; });
    return { curClass: b.cls, alloc: al,
             charArm: (b.armour && b.armour.length) ? b.armour : null,
             armSlots: b.armSlots || armSlots,
             charGear: b.devices || [], activeGear: active || {},
             activeOrder: Object.keys(active || {}) };
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
  // What a boost costs is fully in the data even though how fast morale accrues is not. Every
  // boost wants 15840 points except Healing Boost at 18480, and that cost is REDUCED by
  // prop 357 "Required Morale Points Modifier" - 25% from the skill Team Boost Increase, 10%
  // from Super Healer, 1% per 'm' mod letter on the device itself.
  //
  // So the timeline asks for one calibration instead of a flat time: how long a STANDARD boost
  // (15840, no reduction) takes to bank. Everything else follows from it, and a build that
  // invested in Team Boost Increase gets its boost a quarter sooner - which is the whole point
  // of the skill and was previously invisible.
  var MORALE_BASE = 15840;
  // The shared off-hand cooldown, from TgDevice.uc rather than estimated. Firing an off-hand
  // sets r_bInGlobalOffhandCooldown on the pawn and starts a timer:
  //   server 1.0s (authoritative), client 0.75s (prediction)   TgDevice.uc:1390/1400
  // and ApplyGlobalOffhandCooldown() is `IsOffhand() && !IsOffhandJetpack()`, so every off-hand
  // is gated except the jetpack. Boosts sit in their own slot (476 Morale Device, not 390
  // Off-Hand) and are not subject to it. We take the server figure - that is what actually
  // governs what happened.
  var OFFHAND_GCD = 1.0;

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
              refire: 0, cooldown: 0, shots: [], dots: [], heals: [], powers: [], buffs: [],
              maxLife: 0,
              scope: deviceScope(a, g, dev, mode), ready: 0 };
    (m.chips || []).forEach(function (c) {
      if (c.prop === 53) d.refire = c.value;
      if (c.prop === 4) d.cooldown = c.value;
      if (c.prop === 150) d.persist = c.value;        // Persist Time - how long a boost lasts
      if (c.prop === 318) d.morale = c.value;         // Required Points To Fire
    });
    // c.life is the printed duration; c.lifeVal is what it becomes after the build's Effect
    // Lifetime skills. projectedEffects already used lifeVal, so effects on OTHER people ran
    // for the buffed time while your own ran for the raw one - a Range Shield's protection
    // lapsing at 10.1s while its pool sat there until 14.1s was this.
    function lifeOf(c) { return c.lifeVal || c.life || 0; }
    // Skills that fire BECAUSE this device is up - Aegis Armament's +25 Physical while a
    // shield holds (prop 155, calc 67, egt 1104 REACTIVE). simDevice only ever read chips, so
    // none of these reached the run: no reactive or conditional skill was being applied at all.
    // Always-on passives are excluded because the skill tree already counts them.
    d.extra = (res.extra || []).filter(function (x) {
      return x.kind !== 'passive' && !(GA.LANDS_ON_OTHER || {})[x.egt] && GA.statName(x.prop);
    }).map(function (x) {
      var pos = (x.calc === 67 || x.calc === 68);
      return { p: x.prop, name: GA.statName(x.prop), v: pos ? x.v : -x.v,
               pct: (x.calc === 68 || x.calc === 69), cat: 0, src: g.name, skill: x.skill };
    });
    d.selfTimed = [];
    // A shield is a POOL plus the protection props it covers - prop 386 carries the pool, and
    // any protection chip sharing its category is what the pool stands behind.
    d.shields = [];
    (m.chips || []).forEach(function (c) {
      if (c.prop !== 386 || c.base === null) return;
      var covers = (m.chips || []).filter(function (x) {
        return x.prop !== 386 && x.cat === c.cat && !x.neg
          && (GA.PROT_PROPS || []).indexOf(x.prop) >= 0;
      });
      if (covers.length) {
        // Take the lifetime from the PROTECTION, not from the pool chip. They belong to one
        // effect group and must lapse together; reading the pool's own lifeVal had the
        // protection ending at 10.1s while the pool sat there until 14.1s.
        d.shields.push({ pool: c.value, props: covers.map(function (x) { return x.prop; }),
                         life: covers[0].lifeVal || covers[0].life || c.lifeVal || c.life || 0,
                         cat: c.cat });
      }
    });
    var onSelf = (mm.hit && mm.hit.tgt === 'self');
    // Backstab is far more than the power sap on the two maces: thirteen melee weapons carry
    // one, and between them they add flat damage, burns, slows, a healing-taken cut and a
    // protection shred. None of it was gated - some was being applied unconditionally, the
    // rest thrown away - so the card now carries a switch and the run honours it.
    d.hasBackstab = (m.chips || []).some(function (c) { return c.bs && c.base !== null; });
    d.backstab = !!(a.backstab && a.backstab[slot]);
    (m.chips || []).forEach(function (c) {
      if (c.base === null) return;
      // Mechanical-only payloads have no target in a fight between people. Left in the model
      // so the loadout page can still show them, dropped here so they never land.
      if ((GA.PLAYER_IMMUNE || {})[c.cat]) return;
      if (c.prop === 51 || c.prop === 211) {
        if (c.sign < 0) {
          // A damage chip with an apply interval is damage OVER TIME: it lands once per
          // interval for its lifetime. Treating it as one lump undercounted it badly - a
          // Life Stealer's "DoT 55 5.0s" is five ticks of 55, not a single 55 - and, because
          // it was never recorded on the target, nothing could cleanse it either.
          if (c.iv > 0 && lifeOf(c) > 0) {
            d.dots.push({ raw: c.value, cat: c.cat, life: lifeOf(c), iv: c.iv, bs: !!c.bs,
                          app: c.app || 0, appv: c.appv || 0 });
          } else {
            d.shots.push({ raw: c.value, cat: c.cat, life: lifeOf(c), bs: !!c.bs });
          }
        }
        else if (onSelf) d.selfHeals = (d.selfHeals || []).concat([{ v: c.value, life: lifeOf(c),
                                                                     iv: c.iv || 0 }]);
        else if (!c.self) d.heals.push({ v: c.value, life: lifeOf(c), iv: c.iv || 0,
                                        sit: c.sit || 0, sv: c.sv || 0,
                                        cat: c.cat, app: c.app || 0, appv: c.appv || 0 });
        else if (c.life > 0) d.selfTimed.push({ p: c.prop, name: GA.statName(c.prop) || 'self',
                                                v: c.value, pct: c.isPct, cat: c.cat,
                                                src: g.name, life: lifeOf(c) });
      } else if (c.prop === 243) {
        // Power Pool. Seven devices move it - Power Stim, Power Station, Power Wave, Triage
        // Wave and the two backstab maces - and the run applied none of them. Backstab drains
        // are skipped: the timeline has no positional model, so it cannot know you are behind.
        var pw = { v: c.sign < 0 ? -c.value : c.value, life: lifeOf(c),
                   sit: c.sit || 0, sv: c.sv || 0, bs: !!c.bs };
        if (c.self || onSelf) d.selfPower = (d.selfPower || []).concat([pw]);
        else d.powers.push(pw);
      } else if ((c.self || onSelf) && c.life > 0) {
        var nm2 = GA.statName(c.prop);
        if (nm2) d.selfTimed.push({ p: c.prop, name: nm2, v: c.sign < 0 ? -c.value : c.value,
                                    pct: c.isPct, cat: c.cat, src: g.name, life: lifeOf(c) });
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
      d.cadence = (d.cooldown || 0) > d.maxLife
        ? 'every ' + Math.round(d.interval) + 's (cooldown)'
        : 'every ' + Math.round(d.interval) + 's (on expiry)';
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
    var sched = sim.sched || {};
    var actors = sim.actors.map(function (a) {
      var ctx = actorCtx(a);
      if (!ctx) return null;
      // Switching a boost on in the board makes statsFor fold its buffs straight into the
      // actor - which is right for the static panel, but wrong here: the timeline decides WHEN
      // a boost goes off. Leaving them in meant a Sensor Boost gated to 14s was still buffing
      // the Scorpia from t=0, and the target died at 6.1s instead of 12.2s. So the baseline is
      // built WITHOUT boosts, and the timeline applies them at the moment they fire.
      // Boosts AND shields are owned by the run, not by the baseline. A shield's protection is
      // only real while its pool holds - a Range Shield puts Ranged at 144 against attack rating
      // 100, which is flat immunity, so leaving it in the baseline made the target invulnerable
      // to ranged damage for ever and breaking the shield could not take it away.
      var baseActive = {}, hasBoost = false;
      Object.keys(a.active).forEach(function (slot) {
        var g = (ctx.charGear || [])[+slot];
        if (g && g.cat === 'Boost') { hasBoost = true; return; }
        if (g && carriesShield(g.id, a.active[slot])) { hasBoost = true; return; }
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
      a.backstab = a.backstab || {};
      Object.keys(a.active).forEach(function (slot) {
        var g = (ctx.charGear || [])[+slot]; if (!g) return;
        var dev = (window.__DEVMODEL__ || {})[String(g.id)]; if (!dev) return;
        proj[slot] = GA.projectedEffects({ backstab: !!(a.backstab || {})[slot], dev: dev,
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
          d.cadence = (d.cooldown || 0) > L
            ? 'every ' + Math.round(d.interval) + 's (cooldown)'
            : 'every ' + Math.round(d.interval) + 's (on expiry)';
        }
      });
      // total morale-cost reduction this build carries (prop 357, a percentage decrease)
      var moraleCut = 0;
      Object.keys(col.stats).forEach(function (k) {
        var x = col.stats[k];
        if (x.p === 357) moraleCut += x.total;
      });
      return { id: a.id, team: a.team, cls: a.cls, pid: a.pid, moraleCut: moraleCut,
               maxHP: dv.totHP, hp: dv.totHP, baseMaxHP: dv.totHP,
               maxPW: dv.totPW, pw: dv.totPW, baseMaxPW: dv.totPW, spentUntil: 0,
               regen: baseRegen(col.stats),
               baseProt: GA.protectionFrom(col.stats),
               devs: devs, proj: proj, aim: a.aim, live: [], dead: false, spentThisStep: false,
               offhandReady: 0, sched: sched[a.id] || {} };
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
  // Everything landing on an actor was simply summed, which ignored the category rules the
  // rest of the console already honours. Category 986 "Additional Damage" is Strongest Wins and
  // holds most of the protection shred in the game - Ballista -10, Assassin Blade -15, Rusted
  // Machete -12, GammaBurst -5 and Killer Instinct's -10 - so two of them landing together were
  // adding up when only the largest should count.
  // Category 302 is scoped by effect-group id rather than category, so two different
  // <Local> burns never contend - same exemption the protection path uses.
  var NOT_A_BUCKET_DOT = { 302: 1 };

  // Burns and regens contend the same way, so they share this. Returns 'refresh' when the
  // incoming effect should just extend one already there, 'drop' when it loses, or 'take'
  // when it should displace the bucket.
  //
  // Same source re-applying is always a refresh - MEASURED for burns in game (a Life Stealer
  // backstab refreshes its own poison every swing and it keeps ticking), and assumed to hold
  // for heals by symmetry. Between DIFFERENT sources the group's rule decides, and for these
  // categories the priority is meaningful: a Poison group's application_value IS its per-tick
  // damage (15 -> 15, 55 -> 55, 158 -> 158) and a Regeneration group's is its per-tick heal
  // (66, 96, 115, 134, 154). So "Strongest Wins" really does mean the harder-hitting one.
  function bucketVerdict(list, src, cat, app, appv, life) {
    var mine = list.filter(function (x) { return x.src === src && x.cat === cat; })[0];
    if (mine) return { how: 'refresh', on: mine };
    if (NOT_A_BUCKET_DOT[cat] || !app || app === 155) return { how: 'take', displace: false };
    var held = list.filter(function (x) { return x.cat === cat; });
    if (!held.length) return { how: 'take', displace: false };
    if (app === 836) return { how: 'refresh', on: held[0] };
    if (app === 874) return { how: 'drop' };
    // 157 Strongest Wins behaves as NEWEST WINS in practice, priority ignored. MEASURED in
    // game 2026-08-03: a Healing Grenade landing on someone carrying a nanite heal-over-time
    // removes it and replaces it with its own, and niting them again replaces the grenade's.
    // Both directions displace, so the priority cannot be gating it - the Healing Grenade's 64
    // should have lost to a nanite's 86 and does not.
    //
    // This contradicts TgEffectManager::IsStrongest as written, which drops an incoming group
    // when one already applied has a higher application value. Why it does not bite is
    // unexplained; the same gap sits behind the burn-refresh result. Until something shows a
    // priority actually blocking an application, newest wins.
    return { how: 'take', displace: true };
  }
  function liveNow(act) {
    var live = act.live || [];
    if (!GA.applyStacking || live.length < 2) return live;
    var order = live.slice().sort(function (a, b) { return (a.at || 0) - (b.at || 0); })
      .map(function (f) { return f.src; });
    var res = GA.applyStacking(live.map(function (f) {
      return { cat: f.cat, app: f.app, appv: f.appv, src: f.src, v: f.v,
               life: (f.until || 0) - (f.at || 0) };
    }), order);
    if (!res.blocked.length) return live;
    var out = live.filter(function (f) { return res.blocked.indexOf(f.cat + '|' + f.src) < 0; });
    return out;
  }
  function protNow(act) {
    var p = {};
    Object.keys(act.baseProt).forEach(function (k) { p[k] = act.baseProt[k]; });
    liveNow(act).forEach(function (f) {
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
  // Healing RECEIVED, scaled by whatever anti-heal is on the target. Seven devices carry one
  // - Scorpia -40%, Life Stealer and Poison Injector -20% on backstab, Poison Aura and Poison
  // Grenade -15%, Agonizer and Pain Gun -5% - and none of them did anything: the debuff landed
  // and was tracked, but no heal ever consulted it.
  function liveHealMult(act) {
    var pct = 0;
    liveNow(act).forEach(function (f) {
      if (f.p === 210) pct += f.v;
    });
    return Math.max(0, 1 + pct / 100);
  }
  // Additional Damage Taken is applied BEFORE mitigation - GA.mitigate has taken an
  // extraTaken option all along, but nothing ever filled it in, so the Pain Gun's +15% did
  // nothing once it reached a target.
  function liveExtraTaken(act) {
    var pct = 0;
    liveNow(act).forEach(function (f) { if (f.p === 316) pct += f.v; });
    return pct;
  }
  function liveDamageMult(act, hit) {
    var want = DMG_MOD_BY_ATK[hit && hit.atk] || 0;
    var pct = 0;
    liveNow(act).forEach(function (f) {
      if (!f.pct) return;
      if (f.p === 65 || (want && f.p === want)) pct += f.v;
    });
    return 1 + pct / 100;
  }

  // Every point a protection axis takes off is submitted to whatever shield stands behind that
  // axis, exactly as CalcProtection does. When the pool empties the shield breaks immediately and
  // its protection stops applying - so the next hit lands unmitigated rather than waiting for the
  // 10s timer. A shield only covers the axes it names: a Range Shield does nothing about melee.
  function drainShields(v, m, times, t) {
    if (!v.shields || !v.shields.length || !m || !m.axes) return;
    (m.axes || []).forEach(function (ax) {
      if (!ax.prop || !(ax.absorbed > 0)) return;
      var hit = null;
      for (var i = 0; i < v.shields.length; i++) {
        if (v.shields[i].props.indexOf(ax.prop) >= 0) { hit = v.shields[i]; break; }
      }
      if (!hit) return;
      hit.pool -= ax.absorbed * (times || 1);
      if (hit.pool <= 0) {
        hit.pool = 0;
        hit.broke = t;
      }
    });
    var broken = v.shields.filter(function (x) { return x.broke != null; });
    if (!broken.length) return null;
    v.shields = v.shields.filter(function (x) { return x.broke == null; });
    broken.forEach(function (b) {
      // drop the protection the shield was providing
      v.live = v.live.filter(function (f) { return f.src !== b.src; });
    });
    return broken.map(function (b) {
      return { t: t, who: v.id, src: b.src, dev: b.devId };
    });
  }

  // does this device put up a finite-pool shield? prop 386 is the pool
  function carriesShield(devId, mode) {
    var dev = (window.__DEVMODEL__ || {})[String(devId)];
    if (!dev) return false;
    var ms = dev.modes || [];
    for (var i = 0; i < ms.length; i++) {
      if (mode && ms[i].kind && ms[i].kind !== mode) continue;
      var ch = ms[i].chips || [];
      for (var k = 0; k < ch.length; k++) {
        if (Array.isArray(ch[k][2]) && ch[k][2][0] === 386) return true;
      }
    }
    return false;
  }

  function catsNow(act) {
    var c = {};
    act.live.forEach(function (f) { if (f.cat && f.v > 0) c[f.cat] = 1; });
    return c;
  }

  function runTimeline(seconds) {
    var S = buildSim();
    if (!S.actors.length) return null;
    var events = [], series = {}, deaths = {}, shieldBreaks = [];
    S.actors.forEach(function (a) { series[a.id] = []; });

    function targetsOf(a, d) {
      if (d.scope.kind === 'all') return d.scope.targets.slice();
      var t = a.aim[d.slot];
      return t ? [t] : [];
    }
    function noteBreaks(list) {
      (list || []).forEach(function (b) { shieldBreaks.push(b); });
    }
    function ev(t, who, text, kind, devId, slot) {
      events.push({ t: Math.round(t * 10) / 10, who: who, text: text, kind: kind || '',
                    dev: devId || null, slot: slot == null ? null : slot });
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
          ev(t, a.id, f.src + ' ' + f.name + ' expires', 'expire', f.devId);
          return false;
        });
        if (before !== a.live.length) { /* mitigation recomputed below */ }
      });

      S.actors.forEach(function (a) { a.spentThisStep = false; });
      // Health Max and Power Pool Max can be raised for a while by something someone else
      // hands you - an Adrenaline Gun's flat +400 health for 10s, Bancroft's Rally and Fashion
      // Boost at +20%, Fashion Boost's +40 power. Every one of those landed on the target and
      // then did nothing, because the ceilings were fixed when the run started.
      //
      // The buff raises the CEILING; it does not hand you the health to fill it. That is the
      // conservative reading - it never creates hit points out of nothing - but it is a guess,
      // and the way to settle it is to hit a full-health team-mate with an Adrenaline Gun and
      // watch whether their current health jumps with the maximum or stays put.
      S.actors.forEach(function (a) {
        var hpPct = 0, hpFlat = 0, pwPct = 0, pwFlat = 0;
        liveNow(a).forEach(function (f) {
          if (f.p === 412) { if (f.pct) hpPct += f.v; else hpFlat += f.v; }
          else if (f.p === 255) { if (f.pct) pwPct += f.v; else pwFlat += f.v; }
        });
        a.maxHP = Math.max(1, Math.round(a.baseMaxHP * (1 + hpPct / 100) + hpFlat));
        a.maxPW = Math.max(1, Math.round(a.baseMaxPW * (1 + pwPct / 100) + pwFlat));
        // when the buff lapses the ceiling drops, and anything above it goes with it
        if (a.hp > a.maxHP) a.hp = a.maxHP;
        if (a.pw > a.maxPW) a.pw = a.maxPW;
      });

      // fire everything that is ready and affordable
      S.actors.forEach(function (a) {
        if (a.dead) return;
        a.devs.forEach(function (d) {
          var interval = d.interval || 0;
          if (!interval) return;                       // nothing that repeats
          var sc = a.sched[d.slot];
          // a start time for a continuous weapon, discrete presses for anything else
          var manual = sc && sc.uses && sc.uses.length && d.refire <= 0;
          if (sc && sc.uses && sc.uses.length && d.refire > 0
              && t + 1e-9 < sc.uses[0]) { d.ready = Math.max(d.ready, t); return; }
          if (manual) {
            // Where it was placed is when you PRESS it, not the only instant it may go off.
            // Matching a single 0.1s window meant a press that collided with the global off-hand
            // cooldown was silently dropped - put Vulture Vision just after Bionics and it never
            // fired at all. A press now waits its turn and goes as soon as it is allowed.
            d.used = d.used || {};
            var dueIdx = -1;
            for (var ui = 0; ui < sc.uses.length; ui++) {
              if (!d.used[ui] && t + 1e-9 >= sc.uses[ui]) { dueIdx = ui; break; }
            }
            if (dueIdx < 0) { d.ready = Math.max(d.ready, t); return; }
            if (t + 1e-9 < d.ready) { d.ready = Math.max(d.ready, t); return; }
            d.pendingIdx = dueIdx;
          } else {
            if (sc && sc.from != null && t + 1e-9 < sc.from) { d.ready = Math.max(d.ready, t); return; }
            if (t + 1e-9 < d.ready) return;
          }
          if (d.once) {
            // Boosts cost morale (prop 318 "Required Points To Fire"), not a cooldown. Nobody
            // opens a fight with one - you press it when you have banked enough. How fast morale
            // accrues is NOT in any data we can read: props 326/398 are unused in the asset DB,
            // AddMoralePoints is native so the UC only calls it, and the server reimplements the
            // replication but not the accrual. So the moment is an INPUT rather than a guess.
            if (!manual) {
              if (d.firedOnce) return;
              if (sim.moraleAt == null) { d.ready = Math.max(d.ready, t); return; }
              // scale the calibration by what THIS boost costs this build
              // prop 357 is a percentage DECREASE and the sheet already stores it signed
              // (-25 means a quarter cheaper), so it adds rather than subtracts here
              var cost = (d.morale || MORALE_BASE) * (1 + (a.moraleCut || 0) / 100);
              if (cost < 0) cost = 0;
              var when = sim.moraleAt * (cost / MORALE_BASE);
              d.moraleWhen = Math.round(when * 10) / 10;
              if (t + 1e-9 < when) { d.ready = Math.max(d.ready, t); return; }
            }
          }
          // Off-hands share a global cooldown - you cannot let three waves off at once, you press
          // them one after another. Without this the whole team's buffs land on the same tick.
          if (d.cat === 'Offhand' && t + 1e-9 < a.offhandReady) { d.ready = Math.max(d.ready, t); return; }
          // Shields share category 770 on "Newest Wins", so a second one displaces the first
          // rather than stacking. Firing it while the first is still holding would throw away
          // both the remaining pool and the cooldown, which nobody does - so a shield waits its
          // turn and goes up when the one before it breaks or lapses. An explicit placement
          // overrides this, same as every other automatic rule here.
          if ((d.shields || []).length && !manual) {
            var occupied = (a.shields || []).some(function (sh2) {
              return (d.shields || []).some(function (mine2) { return mine2.cat === sh2.cat; });
            });
            if (occupied) { d.ready = Math.max(d.ready, t); return; }
          }
          // A buff-stripper is held until there is something worth stripping. Nobody opens with
          // a Neutralize Wave; you wait until the other side has committed its buffs.
          if ((d.strip || []).length && !manual && d.hit.tgt === 'enemy') {
            var worth = targetsOf(a, d).some(function (tid) {
              var v = S.byId[tid];
              if (!v || v.dead || v.team === a.team) return false;
              var tc = catsNow(v);
              return (d.strip || []).some(function (sg) {
                return (sg.cats || []).some(function (cc) { return tc[cc]; });
              });
            });
            if (!worth) { d.ready = Math.max(d.ready, t); return; }    // hold fire
          }
          // A 0.1s step cannot represent a weapon that fires 20 times a second, so count how
          // many shots actually fall inside this step rather than allowing one.
          var volley = 0;
          if (manual) { volley = 1; d.ready = t + interval; }
          else {
            while (d.ready <= t + 1e-9 && volley < 200) { d.ready += interval; volley++; }
            if (d.ready < t) d.ready = t + interval;
          }
          var cost = d.power || 0;
          if (cost > 0) {
            // Running dry is not a per-step stutter. Firing the instant a single shot becomes
            // affordable spends again immediately, and because regen is suppressed on any step
            // that spent, the pool locks at zero and dribbles one shot per step forever - the
            // spiky flatline an Inferno-X used to leave behind. A player stops shooting and
            // lets it come back, so a starved device holds until the pool is usable again:
            // a second of sustained fire, never more than half the pool so it always recovers.
            var perSec = interval > 0 ? cost / interval : cost;
            var resumeAt = Math.min(a.maxPW * 0.5, Math.max(cost, perSec));
            // Running dry is one episode, not one event per duty cycle. A weapon that cannot
            // keep up sits near empty and re-starves every cycle - the Inferno-X reported it 16
            // times in 25s, and every report drew its own full-height marker even though the
            // icon row deduped down to a single icon. The episode only ends once the pool has
            // genuinely recovered, so a later, separate drain still gets reported.
            if (d.starved && a.pw < resumeAt) {
              d.ready = Math.max(d.ready, t);
              return;
            }
            var afford = Math.floor(a.pw / cost);
            if (afford <= 0) {
              if (!d.starved) {
                if (!d.reported) {
                  ev(t, a.id, d.name + ' out of power', 'power', d.id); d.reported = true;
                }
                d.starved = true;
              }
              d.ready = Math.max(d.ready, t);                        // no banking shots while the pool is empty
              return;
            }
            if (afford < volley) volley = afford;
            a.pw -= cost * volley; a.spentThisStep = true; d.starved = false;
            // Regen pausing for exactly the STEP the spend landed in made it an accident of
            // quantisation: a jetpack costs 5.9 every 0.25s, which is one step in every two and
            // a half, so regen ran on the other one and a half and refunded most of the cost.
            // Holding a jetpack is continuous consumption, so the pause covers the interval the
            // spend actually buys. Capped at a second: past that you are not holding a trigger,
            // you have thrown a grenade and moved on.
            a.spentUntil = Math.max(a.spentUntil || 0, t + Math.min(interval || STEP, 1));
          }

          if (d.cat === 'Offhand') a.offhandReady = t + OFFHAND_GCD;
          if (manual && d.pendingIdx != null) { d.used[d.pendingIdx] = 1; d.pendingIdx = null; }
          var tgts = targetsOf(a, d);
          if (!d.firedOnce) {
            ev(t, a.id, d.name + ' fires'
               + (/every/.test(d.cadence || '') ? ', ' + d.cadence : ''), 'fire', d.id, d.slot);
            d.firedOnce = true;
          }
          // damage
          d.shots.forEach(function (sh) {
            if (sh.bs && !d.backstab) return;
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
                ev(t, v.id, d.name + ' strips buffs (+' + Math.round(extra) + ')', 'strip', d.id);
              }
              // The anti-one-shot cap arms only at EXACTLY full health - ApplyHit:1287 divides
              // two ints, so the ">90%" condition collapses to "is the target at max" - and it
              // clamps that one hit to Health - ceil(maxHP/10). It is per-hit, so in a volley
              // only the opening shot can be capped; by the second the target is no longer full.
              var hitInfo = { cat: sh.cat, damageType: d.hit.dmg, attackType: d.hit.atk,
                              rating: d.hit.rating };
              var rawOne = sh.raw * liveDamageMult(a, d.hit) + extra;
              var atFull = v.hp >= v.maxHP;
              var xt = liveExtraTaken(v);
              var m = GA.mitigate(rawOne, hitInfo, protNow(v),
                atFull ? { healthCapArmed: 1, maxHP: v.maxHP, curHP: v.hp, extraTaken: xt }
                       : { extraTaken: xt });
              if (atFull && volley > 1) {
                var rest = GA.mitigate(rawOne, hitInfo, protNow(v), { extraTaken: xt });
                v.hp -= m.shown + rest.shown * (volley - 1);
                noteBreaks(drainShields(v, m, 1, t));
                noteBreaks(drainShields(v, rest, volley - 1, t));
              } else {
                v.hp -= m.shown * volley;
                noteBreaks(drainShields(v, m, volley, t));
              }
              if (v.hp <= 0 && !v.dead) {
                v.dead = true; v.hp = 0; deaths[v.id] = t;
                ev(t, v.id, v.cls + ' #' + v.id + ' dies', 'death', null);
              }
            });
          });
          // Damage over time. Registered on the target with the raw per-tick figure: the
          // ticks are mitigated as they land, not once up front, so protection that changes
          // mid-burn changes what the remaining ticks cost - which is how the game does it.
          d.dots.forEach(function (dt) {
            if (dt.bs && !d.backstab) return;
            tgts.forEach(function (tid) {
              var v = S.byId[tid];
              if (!v || v.dead || v.team === a.team) return;
              // What re-application does depends on the group's application rule, because
              // each rule takes a different path through TgEffectManager:
              //
              //   157 Strongest Wins -> IsStrongest, then RemoveAllEffectGroups + a fresh
              //       clone. RemoveAllEffectGroups cancels the timers armed on the group, so
              //       the tick clock RESTARTS. A Life Stealer swings every 0.63s and its
              //       poison ticks every 1.0s, so a spammed backstab burn never ticks at all
              //       until you stop swinging. I previously assumed that could not be right
              //       and made re-application preserve the clock; the server says otherwise.
              //   156 Newest Wins -> GetStackingEffectGroup, also a displacement, so also a
              //       restart.
              //   836 Refresh     -> GetRefreshedEffectGroup extends the existing group in
              //       place, so the tick schedule survives.
              //   155 Stackable   -> a genuinely separate instance, ticking alongside.
              //   874 Oldest Wins -> the incoming one is dropped while another holds.
              v.dots = (v.dots || []);
              // Burns resolve through the same bucket rules as regens - see bucketVerdict.
              v.dots = (v.dots || []);
              var dv = bucketVerdict(v.dots, d.name, dt.cat, dt.app, dt.appv, dt.life);
              if (dv.how === 'drop') return;
              if (dv.how === 'refresh') {
                dv.on.until = t + dt.life;
                dv.on.raw = dt.raw;
                return;
              }
              if (dv.displace) {
                v.dots = v.dots.filter(function (x) { return x.cat !== dt.cat; });
              }
              v.dots.push({ src: d.name, devId: d.id, cat: dt.cat, raw: dt.raw,
                            app: dt.app, appv: dt.appv, life: dt.life, at: t,
                            iv: dt.iv, next: t + dt.iv, until: t + dt.life,
                            hit: { cat: dt.cat, damageType: d.hit.dmg,
                                   attackType: d.hit.atk, rating: d.hit.rating } });
            });
          });
          // Strip / cleanse. Runs for every target in scope whether or not it paid damage:
          // Neutralize Wave tears buffs off an enemy, a Healing Grenade takes Poison, Disease and
          // Ignite off a team-mate. Same prop-140 mechanic, opposite intent.
          if ((d.strip || []).length) {
            tgts.forEach(function (tid) {
              var v2 = S.byId[tid];
              if (!v2 || v2.dead) return;
              var before = v2.live.length + (v2.dots || []).length;
              v2.live = v2.live.filter(function (f) {
                return !(d.strip || []).some(function (sg) {
                  return (sg.cats || []).indexOf(f.cat) >= 0;
                });
              });
              // A Healing Grenade takes Poison, Disease and Ignite off a team-mate, and those
              // ARE the damage-over-time categories - the cleanse is worth nothing if it only
              // reaches buffs.
              v2.dots = (v2.dots || []).filter(function (f) {
                var gone = (d.strip || []).some(function (sg) {
                  return (sg.cats || []).indexOf(f.cat) >= 0;
                });
                if (gone) ev(t, v2.id, f.src + ' burn cleansed by ' + d.name, 'strip', d.id);
                return !gone;
              });
              var gone = before - v2.live.length;
              if (gone > 0 && v2.team === a.team) {
                ev(t, v2.id, d.name + ' cleanses ' + gone + ' effect' + (gone > 1 ? 's' : ''),
                   'strip', d.id);
              }
            });
          }
          // healing
          (d.selfPower || []).forEach(function (q) {
            if (q.bs && !d.backstab) return;
            if (q.life > 0) {
              a.pregen = (a.pregen || []).filter(function (x) { return x.src !== d.name; });
              a.pregen.push({ src: d.name, devId: d.id, rate: q.v / q.life, until: t + q.life });
            } else {
              a.pw = Math.max(0, Math.min(a.maxPW, a.pw + q.v * volley));
            }
          });
          (d.selfHeals || []).forEach(function (h) {
            if (h.life > 0) {
              a.hots = (a.hots || []).filter(function (x) { return x.src !== d.name; });
              a.hots.push({ src: d.name, devId: d.id, raw: h.v, iv: h.iv || h.life,
                            next: t + (h.iv || h.life), until: t + h.life });
            } else {
              a.hp = Math.min(a.maxHP, a.hp + h.v * volley * liveHealMult(a));
            }
          });
          // One hit, one evaluation: the health test is taken BEFORE any of this device's
          // heals land, so Triage Wave's unconditional 600 cannot lift the target past 25% and
          // disqualify its own conditional 600.
          var hpAtHit = {};
          tgts.forEach(function (tid) {
            var v0 = S.byId[tid];
            if (v0) hpAtHit[tid] = v0.maxHP ? (v0.hp / v0.maxHP) * 100 : 100;
          });
          d.heals.forEach(function (h) {
            tgts.forEach(function (tid) {
              var v = S.byId[tid];
              if (!v || v.dead || v.team !== a.team) return;
              if (!GA.situationalOk(h.sit, h.sv, hpAtHit[tid])) return;
              if (h.life > 0) {
                // A heal-over-time contends for its category exactly as a burn does: two medics
                // regenerating one target do not simply add up when the category says otherwise.
                v.hots = (v.hots || []);
                var hv = bucketVerdict(v.hots, d.name, h.cat, h.app, h.appv, h.life);
                if (hv.how === 'drop') return;
                if (hv.how === 'refresh') {
                  hv.on.until = t + h.life;
                  hv.on.raw = h.v;
                  return;
                }
                if (hv.displace) {
                  v.hots = v.hots.filter(function (x) { return x.cat !== h.cat; });
                }
                // Like a burn, the value is PER TICK - not a total to spread over the
                // duration. "HoT +86 10.0s" at a 1s interval is 86 ten times over. Dividing by
                // the lifetime ran every heal-over-time in the game at a tenth of its strength.
                v.hots.push({ src: d.name, devId: d.id, raw: h.v, iv: h.iv || h.life,
                              next: t + (h.iv || h.life), until: t + h.life,
                              cat: h.cat, app: h.app, appv: h.appv, life: h.life, at: t });
              } else {
                v.hp = Math.min(v.maxHP, v.hp + h.v * volley * liveHealMult(v));
              }
            });
          });
          d.powers.forEach(function (q) {
            if (q.bs && !d.backstab) return;
            tgts.forEach(function (tid) {
              var v = S.byId[tid];
              if (!v || v.dead) return;
              // The side depends on the sign, not on a fixed assumption. This was written for
              // Triage Wave restoring an ally and hard-coded to team-mates, which threw away
              // every drain: the two backstab maces sap 30 power off an ENEMY.
              if (q.v < 0 ? v.team === a.team : v.team !== a.team) return;
              if (!GA.situationalOk(q.sit, q.sv, hpAtHit[tid])) return;
              if (q.life > 0) {
                v.pregen = (v.pregen || []).filter(function (x) { return x.src !== d.name; });
                v.pregen.push({ src: d.name, devId: d.id, rate: q.v / q.life, until: t + q.life });
              } else {
                v.pw = Math.max(0, Math.min(v.maxPW, v.pw + q.v * volley));
              }
            });
          });
          // raise any shield this device carries, on everyone it reaches
          if ((d.shields || []).length) {
            var shieldTo = (d.scope.kind === 'all' && d.scope.targets.length)
              ? d.scope.targets.concat([a.id]) : [a.id];
            d.shields.forEach(function (sh) {
              shieldTo.forEach(function (tid) {
                var v3 = S.byId[tid];
                if (!v3 || v3.dead) return;
                // Newest Wins: whatever was standing in this category comes down, and its
                // protection goes with it. No two shields overlap.
                var displaced = (v3.shields || []).filter(function (x) {
                  return x.cat === sh.cat && x.src !== d.name;
                });
                v3.shields = (v3.shields || []).filter(function (x) {
                  return x.cat !== sh.cat && x.src !== d.name;
                });
                displaced.forEach(function (old) {
                  v3.live = v3.live.filter(function (f) { return f.src !== old.src; });
                  ev(t, v3.id, old.src + ' gives way to ' + d.name, 'strip', d.id);
                });
                v3.shields.push({ src: d.name, devId: d.id, props: sh.props.slice(),
                                  cat: sh.cat, pool: sh.pool, max: sh.pool,
                                  until: t + (sh.life || d.maxLife || 10) });
              });
            });
          }
          // Conditional skills ride the thing that gates them: while the shield holds if this
          // device raises one, otherwise for the device's own effect duration. They carry the
          // device as their source, so a shield breaking takes them down with it.
          if ((d.extra || []).length) {
            var gate = 0;
            if ((d.shields || []).length) gate = d.shields[0].life || d.maxLife || 0;
            else gate = d.maxLife || 0;
            if (gate > 0) {
              d.extra.forEach(function (f) {
                a.live = a.live.filter(function (x) {
                  return !(x.src === f.src && x.p === f.p);
                });
                a.live.push({ p: f.p, name: f.name + ' (' + f.skill + ')', v: f.v, pct: f.pct,
                              cat: f.cat, src: f.src, until: t + gate, devId: d.id });
              });
            }
          }
          // the device's own timed self-effects (Oathbreaker's +20 protection for 10s)
          (d.selfTimed || []).forEach(function (f) {
            a.live = a.live.filter(function (x) { return !(x.src === f.src && x.p === f.p); });
            a.live.push({ p: f.p, name: f.name, v: f.v, pct: f.pct, cat: f.cat,
                          src: f.src, until: t + f.life, devId: d.id });
          });
          // buffs / debuffs with a lifetime
          (a.proj[d.slot] || []).forEach(function (f) {
            if (!f.life) return;                      // instantaneous riders are not tracked
            var recips = d.once && d.scope.kind === 'all'
              ? d.scope.targets.concat([a.id]) : tgts;
            recips.forEach(function (tid) {
              var v = S.byId[tid];
              if (!v || v.dead) return;
              var hp0 = hpAtHit[tid];
              if (hp0 === undefined) hp0 = v.maxHP ? (v.hp / v.maxHP) * 100 : 100;
              if (!GA.situationalOk(f.sit, f.sv, hp0)) return;
              v.live = v.live.filter(function (x) { return !(x.src === f.src && x.p === f.p); });
              v.live.push({ p: f.p, name: f.name, v: f.v, pct: f.pct, cat: f.cat,
                            app: f.app || 0, appv: f.appv || 0, at: t,
                            src: f.src, until: t + f.life, devId: d.id });
            });
          });
        });
      });

      // shields lapse on their own timer too, taking their protection with them
      S.actors.forEach(function (a) {
        if (!a.shields || !a.shields.length) return;
        var gone = a.shields.filter(function (sh) { return sh.until <= t; });
        if (!gone.length) return;
        a.shields = a.shields.filter(function (sh) { return sh.until > t; });
        gone.forEach(function (sh) {
          a.live = a.live.filter(function (f) { return f.src !== sh.src; });
          ev(t, a.id, sh.src + ' shield lapses (' + Math.round(sh.pool) + ' of '
             + Math.round(sh.max) + ' left)', 'expire', sh.devId);
        });
      });
      // heal-over-time ticks
      S.actors.forEach(function (a) {
        if (a.dots && a.dots.length && !a.dead) {
          a.dots = a.dots.filter(function (dt) {
            if (dt.until <= t) {
              ev(t, a.id, dt.src + ' burn ends', 'expire', dt.devId);
              return false;
            }
            return true;
          });
          a.dots.forEach(function (dt) {
            if (dt.next > t) return;
            dt.next += dt.iv;
            var atFull = a.hp >= a.maxHP;
            var m = GA.mitigate(dt.raw, dt.hit, protNow(a),
              atFull ? { healthCapArmed: 1, maxHP: a.maxHP, curHP: a.hp, extraTaken: liveExtraTaken(a) }
                     : { extraTaken: liveExtraTaken(a) });
            a.hp -= m.shown;
            noteBreaks(drainShields(a, m, 1, t));
            if (a.hp <= 0 && !a.dead) {
              a.dead = true; a.hp = 0; deaths[a.id] = t;
              ev(t, a.id, a.cls + ' #' + a.id + ' dies', 'death', null);
            }
          });
        }
        if (a.pregen && !a.dead) {
          a.pregen = a.pregen.filter(function (q) { return q.until > t; });
          a.pregen.forEach(function (q) {
            a.pw = Math.max(0, Math.min(a.maxPW, a.pw + q.rate * STEP));
          });
        }
        if (!a.hots || a.dead) return;
        a.hots = a.hots.filter(function (h) {
          if (h.until > t) return true;
          ev(t, a.id, h.src + ' heal over time expires', 'expire', h.devId);
          return false;
        });
        var hm = liveHealMult(a);
        a.hots.forEach(function (h) {
          if (h.next > t) return;
          h.next += h.iv;
          a.hp = Math.min(a.maxHP, a.hp + h.raw * hm);
        });
      });

      // power regen only when nothing was spent this step (confirmed in game, backlog C4)
      S.actors.forEach(function (a) {
        if (t >= (a.spentUntil || 0) && a.pw < a.maxPW) a.pw = Math.min(a.maxPW, a.pw + a.regen * STEP);
        // A starvation episode ends when the pool is genuinely healthy again, so a later drain
        // is reported afresh. Checked here rather than at the fire gate: a device waiting on a
        // cooldown while the pool refills never reaches that gate, and would stay silent.
        if (a.pw >= a.maxPW * 0.5) {
          (a.devs || []).forEach(function (d2) { d2.reported = false; });
        }
      });
    }
    shieldBreaks.forEach(function (b) {
      var who = S.byId[b.who];
      events.push({ t: Math.round(b.t * 10) / 10, who: b.who,
                    text: b.src + ' shield breaks', kind: 'strip', dev: b.dev, slot: null });
    });
    events.sort(function (x, y) { return x.t - y.t; });
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
    // Health thresholds, drawn over the plot rather than inside the SVG: the chart is
    // stretched with preserveAspectRatio="none", which would squash any label written into it.
    // A percentage of health sits at (1 - fraction) from the top.
    var THRESH = '<div class="tlthr">'
      + [[75, 'gate'], [50, ''], [25, 'gate']].map(function (k) {
          return '<span class="thr' + (k[1] ? ' ' + k[1] : '') + '" style="top:'
            + (100 - k[0]) + '%"><i>' + k[0] + '%</i></span>';
        }).join('') + '</div>';

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
      // activations, strips, power-outs and deaths first; expiries fill whatever room is left
      var mine = r.events.filter(function (e) { return String(e.who) === String(a.id); });
      var seenLab = {};
      function pick(kinds, cap) {
        return mine.filter(function (e) {
          if (kinds.indexOf(e.kind) < 0) return false;
          // One icon per device per moment. A Sensor Boost lapsing is a single event, not three
          // because it happened to carry three damage modifiers.
          var key = e.kind === 'expire'
            ? 'x|' + (e.dev || e.text.split(' ')[0]) + '|' + e.t
            : e.text;
          if (seenLab[key]) return false;
          seenLab[key] = 1; return true;
        }).slice(0, cap);
      }
      var labels = pick(['fire', 'strip', 'power', 'death'], 5)
        .concat(pick(['expire'], 3))
        .sort(function (x, y) { return x.t - y.t; });
      // Vertical pins stay in the SVG; the labels themselves become device icons in an HTML
      // overlay. The SVG is stretched with preserveAspectRatio="none", so anything drawn inside
      // it is squashed - an overlay keeps the icons square and legible where text was not.
      var labSvg = labels.map(function (e) {
        var x = (e.t / r.seconds) * W;
        return '<line class="tlpin ' + esc(e.kind) + '" x1="' + x.toFixed(1) + '" y1="0" x2="'
          + x.toFixed(1) + '" y2="' + H + '"/>';
      }).join('');
      // Every scheduled use gets its own icon so it can be dragged; a device left alone shows
      // the moment it first went off, and dragging THAT is what pins it to a schedule.
      var act0 = sim.actors.filter(function (x) { return String(x.id) === String(a.id); })[0];
      var fireIcons = [];
      if (act0) {
        Object.keys(act0.active).forEach(function (slot) {
          var sc = (sim.sched[act0.id] || {})[slot];
          var first = mine.filter(function (e) {
            return e.kind === 'fire' && String(e.slot) === String(slot);
          })[0];
          // fall back to the gear itself, so a device that has not gone off yet still shows its
          // own icon rather than a bare "!"
          var ctx0 = actorCtx(act0);
          var g0 = ctx0 && (ctx0.charGear || [])[+slot];
          var devId = first ? first.dev : (g0 ? g0.id : null);
          var nm = first ? first.text.replace(/ \(re-applies on expiry\)/, '') : 'fires';
          if (sc && sc.uses && sc.uses.length) {
            // A press can land later than it was placed - the global off-hand cooldown will hold
            // it. Draw the icon where it ACTUALLY went and say so, rather than leaving a marker
            // sitting at a moment nothing happened.
            var fires = mine.filter(function (e) {
              return e.kind === 'fire' && String(e.slot) === String(slot);
            }).sort(function (x, y) { return x.t - y.t; });
            sc.uses.forEach(function (u, ui) {
              var actual = fires[ui] ? fires[ui].t : null;
              var late = actual != null && Math.abs(actual - u) > 0.05;
              fireIcons.push({ t: actual == null ? u : actual, dev: devId, slot: slot, idx: ui,
                               sched: 1, pending: actual == null,
                               text: nm + (late ? ' - pressed ' + u.toFixed(1) + 's, fired '
                                                 + actual.toFixed(1) + 's (held)'
                                          : actual == null ? ' - pressed ' + u.toFixed(1)
                                                             + 's, never fired'
                                          : ' (scheduled)') });
            });
          } else if (first) {
            fireIcons.push({ t: first.t, dev: first.dev, slot: slot, idx: -1, sched: 0,
                             text: nm });
          } else {
            var dev0 = g0 && (window.__DEVMODEL__ || {})[String(g0.id)];
            var strips0 = dev0 && ((dev0.modes || [])[0] || {}).strip;
            var moraleTxt = '';
            if (g0 && g0.cat === 'Boost' && sim.moraleAt != null) {
              var st0 = r.S.byId[a.id];
              var dv0 = st0 && (st0.devs || []).filter(function (x) {
                return String(x.slot) === String(slot); })[0];
              if (dv0 && dv0.moraleWhen != null) {
                moraleTxt = ' - banked at ' + dv0.moraleWhen + 's, after this window';
              }
            }
            var reason = g0 && g0.cat === 'Boost'
              ? (moraleTxt || 'held - waiting on morale (set "boosts at", or drag this to force it)')
              : (strips0 && strips0.length
                  ? 'held - nothing on the other side worth stripping yet (drag to force it)'
                  : 'never fired in this window');
            fireIcons.push({ t: 0, dev: g0 ? g0.id : null, slot: slot, idx: -1, sched: 0,
                             held: 1, text: (g0 ? g0.name : 'device') + ' ' + reason });
          }
        });
      }
      var otherIcons = labels.filter(function (e) { return e.kind !== 'fire'; });

      var iconRow = fireIcons.map(function (e, k) {
        var pc = Math.max(0, Math.min(100, (e.t / r.seconds) * 100));
        var ic2 = e.dev && IMGT[String(e.dev)];
        return '<span class="tlic fire drag' + (e.sched ? ' sched' : '')
          + (e.pending ? ' pending' : '') + (e.held ? ' held' : '')
          + '" data-a="' + a.id + '" data-slot="' + e.slot + '" data-idx="' + e.idx + '"'
          + ' style="left:' + pc.toFixed(2) + '%;top:' + (2 + (k % 3) * 20) + 'px"'
          + ' title="' + esc(e.t.toFixed(1) + 's - ' + e.text)
          + ' (drag to move, click to reset)">'
          + (ic2 ? '<img src="' + ic2 + '" alt="">' : '<b>!</b>') + '</span>';
      }).concat(otherIcons.map(function (e, k) {
        var pc = Math.max(0, Math.min(100, (e.t / r.seconds) * 100));
        var ic2 = e.dev && IMGT[String(e.dev)];
        var same = e.kind === 'expire'
          ? mine.filter(function (o) { return o.kind === 'expire' && o.dev === e.dev && o.t === e.t; })
          : [];
        var tip = e.t.toFixed(1) + 's - ' + (same.length > 1
          ? same[0].text.replace(/ [^ ]+ expires$/, '') + ' expires (' + same.length + ' effects)'
          : e.text);
        return '<span class="tlic ' + esc(e.kind) + '" style="left:' + pc.toFixed(2)
          + '%;top:' + (2 + ((k + fireIcons.length) % 3) * 20) + 'px" title="' + esc(tip) + '">'
          + (ic2 ? '<img src="' + ic2 + '" alt="">'
              : (e.kind === 'death' ? (classIcon('death') || '<b>&times;</b>') : '<b>!</b>'))
          + '</span>';
      })).join('');
      var died = r.deaths[a.id];
      return '<div class="tlrow"><div class="tlwho"><b>' + classIcon(a.cls, 'sm')
        + esc(a.cls) + ' #' + a.id + '</b>'
        + '<i>' + (died != null ? 'dies ' + (Math.round(died * 10) / 10) + 's'
                                : Math.round(pts[pts.length - 1].hp) + ' HP left') + '</i></div>'
        + '<div class="tlplot">'
        + '<svg class="tlsvg" viewBox="0 0 ' + W + ' ' + H + '" preserveAspectRatio="none">'
        + grid
        + (show.marks ? marks : '')
        + (show.prot ? '<path class="tlprot" d="' + dr.join(' ') + '"/>' : '')
        + (show.pw ? '<path class="tlpw" d="' + dp.join(' ') + '"/>' : '')
        + (show.hp ? '<path class="tlhp" d="' + d.join(' ') + '"/>' : '')
        + (show.marks ? labSvg : '')
        + (died != null ? '<line class="tldead" x1="' + ((died / r.seconds) * W).toFixed(1)
            + '" y1="0" x2="' + ((died / r.seconds) * W).toFixed(1) + '" y2="' + H + '"/>' : '')
        + '</svg>'
        + (show.hp ? THRESH : '')
        + (show.marks ? '<div class="tlicons">' + iconRow + '</div>' : '')
        + '</div></div>';
    }

    var IMG2 = window.__DEVIMG__ || {}, IMGT = IMG2;
    // one line per distinct event, deduped - a 10s buff re-applied 30 times is not 30 events
    var seen = {}, evs = [];
    r.events.forEach(function (e) {
      var k = e.who + '|' + e.text;
      if (seen[k]) return;
      seen[k] = 1; evs.push(e);
    });
    evs.sort(function (a, b) { return a.t - b.t; });

    // What the run produced, for EVERYONE - not for whichever actor happened to be the focus.
    // The old pill described sim.focus.to, which defaults to the first combatant on team A, so it
    // reported an arbitrary body and said nothing about anyone else dying.
    var fallen = Object.keys(r.deaths).map(function (id) {
      return { a: r.S.byId[id], t: r.deaths[id] };
    }).filter(function (x) { return x.a; }).sort(function (x, y) { return x.t - y.t; });

    var verdict;
    if (!fallen.length) {
      verdict = '<i>nobody dies in ' + r.seconds + 's</i>';
    } else {
      verdict = fallen.map(function (f) {
        return '<span class="vdead">' + classIcon('death') + classIcon(f.a.cls, 'sm')
          + esc(f.a.cls) + ' #' + f.a.id + ' <b>' + (Math.round(f.t * 10) / 10) + 's</b></span>';
      }).join('');
      var standing = r.S.actors.filter(function (x) { return !x.dead; });
      if (standing.length) {
        verdict += '<span class="vlive">' + standing.length + ' still standing</span>';
      }
    }

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
      + '<label class="tllen">boost banked <input id="tl-morale" type="number" min="0" max="180" '
      + 'placeholder="never" value="' + (sim.moraleAt == null ? '' : sim.moraleAt) + '"'
      + ' title="how long a STANDARD boost (15840 points, no reduction) takes to bank. The earn '
      + 'rate is not in any readable data, so this one number calibrates it - each boost is then '
      + 'scaled by its own cost and by this build\'s Required Morale Points reduction.">s</label>'
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

    // Drag a device's icon along its own row to change when it goes off; a plain click puts
    // it back to firing by itself. This replaced a stack of one-track-per-device sliders.
    host.querySelectorAll('.tlic.drag').forEach(function (ic3) {
      var moved = false, startX = 0, plot = ic3.closest('.tlplot');
      ic3.addEventListener('mousedown', function (e) {
        e.preventDefault();
        moved = false; startX = e.clientX;
        var box = plot.getBoundingClientRect();
        function mv(e2) {
          if (Math.abs(e2.clientX - startX) > 2) moved = true;
          var f = (e2.clientX - box.left) / box.width;
          ic3.style.left = (Math.max(0, Math.min(1, f)) * 100).toFixed(2) + '%';
        }
        function up(e2) {
          document.removeEventListener('mousemove', mv);
          document.removeEventListener('mouseup', up);
          var sc = schedOf(ic3.dataset.a, ic3.dataset.slot);
          if (!moved) { sc.uses = []; sc.from = null; renderTimeline(seconds); return; }
          var f = (e2.clientX - box.left) / box.width;
          var when = Math.round(Math.max(0, Math.min(1, f)) * seconds * 10) / 10;
          var idx = +ic3.dataset.idx;
          if (idx >= 0 && sc.uses[idx] != null) sc.uses[idx] = when;
          else sc.uses = [when];
          sc.uses.sort(function (x, y) { return x - y; });
          renderTimeline(seconds);
        }
        document.addEventListener('mousemove', mv);
        document.addEventListener('mouseup', up);
      });
    });
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
    // The simulator reads saved builds only - the database is an import source on the
    // TheoryCrafter, not a live dependency here.
    var avail = builds();
    if (!avail.length) {
      host.innerHTML = '<p class="empty">No saved builds yet. Make one on the '
        + '<b>TheoryCrafter</b> tab and send it here'
        + (charList().length ? ', or import a live profile there to start from something real.'
                            : '.') + '</p>';
      var tl0 = document.getElementById('cb-timeline');
      if (tl0) tl0.innerHTML = '';
      return;
    }
    // drop actors whose build has been deleted
    sim.actors = sim.actors.filter(function (a) { return buildById(a.buildId); });
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
        // Weapons with a second fire mode - Inferno-X, iMinigun, Helot, BioFeedback Beam,
        // Boost Beam - were always run on PRI because activating hardcoded it. The sim already
        // keyed everything off a.active[slot]; it just had no way to say ALT.
        var mset = modesOf(g), msel = '';
        if (on && mset.length > 1) {
          var dm = (dev0(g).modes || []);
          msel = '<span class="acmode"><select class="acmd" data-a="' + a.id + '" data-i="' + i + '">'
            + mset.map(function (k) {
                var mn = dm.filter(function (x) { return x.kind === k; })[0];
                var nm3 = (mn && mn.name) || '';
                // both Inferno-X modes are called "Autofire", so the name alone cannot tell
                // them apart - fall back to primary/alt whenever it is not distinctive
                var dupe = nm3 && dm.filter(function (x) { return x.name === nm3; }).length > 1;
                var lab = (!nm3 || dupe) ? (MODELAB[k] || k) : nm3;
                return '<option value="' + k + '"'
                  + ((a.active[i] || 'PRI') === k ? ' selected' : '') + '>'
                  + esc(lab) + '</option>';
              }).join('') + '</select></span>';
        }
        var bsBox = '';
        if (on && hasBackstab(g, a.active[i])) {
          bsBox = '<label class="acbs' + ((a.backstab || {})[i] ? ' on' : '') + '"'
            + ' title="strike from behind - applies this weapon’s backstab rider">'
            + '<input type="checkbox" class="acbsx" data-a="' + a.id + '" data-i="' + i + '"'
            + ((a.backstab || {})[i] ? ' checked' : '') + '>back</label>';
        }
        return '<span class="acgcell"><span class="acg' + (on ? ' on' : '') + ' t-' + tgt + '"'
          + ' data-a="' + a.id + '" data-i="' + i + '" data-tgt="' + tgt + '"'
          + ' title="' + esc(g.name) + ' — ' + esc(g.cat || '') + ', targets ' + esc(tgt) + '">'
          + ic + esc(g.name) + '</span>' + msel + bsBox + sel + '</span>';
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
        + '<div class="acmain">'
        + '<div class="acleft">'
        + '<div class="achead">' + (classIcon(a.cls) || '<span class="acdot '
            + esc(a.cls.toLowerCase()) + '"></span>')
        + '<select class="acbuild" data-a="' + a.id + '">'
        + builds().map(function (b) {
            return '<option value="' + b.id + '"'
              + (String(b.id) === String(a.buildId) ? ' selected' : '') + '>'
              + esc(b.name) + '</option>';
          }).join('') + '</select>'
        + '<span class="acid">#' + a.id + '</span>'
        + '<button class="acswap" data-a="' + a.id + '" title="move to the other team">'
          + (a.team === 'A' ? '&rarr;' : '&larr;') + '</button>'
        + '<button class="acx" data-a="' + a.id + '" title="remove">&times;</button></div>'
        + '<div class="acstats"><span class="achp">' + st.maxHP + ' HP</span>' + protBits + '</div>'
        + '<div class="acskills">' + st.skills + ' pts &mdash; ' + trees2str(st.trees) + '</div>'
        + '</div>'
        + '<div class="acright"><div class="acgear">' + gear + '</div>'
        + (inb ? '<div class="acinb"><span class="aclab">incoming</span>' + inb + '</div>' : '')
        + '</div></div>'
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
      var opts = builds().map(function (b) {
        return '<option value="' + b.id + '">' + esc(b.name) + '</option>';
      }).join('');
      return '<div class="team t' + team + '"><div class="teamhead"><h4>' + label + '</h4>'
        + (opts
            ? '<select class="teamadd" data-team="' + team + '">'
              + '<option value="">+ add&hellip;</option>' + opts + '</select>'
            : '<span class="teamnone">no saved builds</span>')
        + '</div>'
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
    host.querySelectorAll('.acbuild').forEach(function (s2) {
      s2.addEventListener('change', function () {
        var a = actorById(s2.dataset.a); if (!a) return;
        var b = buildById(s2.value);
        if (!b) return;
        a.buildId = b.id; a.cls = b.cls; a.active = {}; a.aim = {};
        renderCombat();
      });
    });
    host.querySelectorAll('.acswap').forEach(function (b2) {
      b2.addEventListener('click', function () {
        var a = actorById(b2.dataset.a); if (!a) return;
        a.team = (a.team === 'A') ? 'B' : 'A';
        // aims that now point at a team-mate, or at an enemy that is now an ally, are stale
        a.aim = {};
        sim.actors.forEach(function (x) {
          Object.keys(x.aim).forEach(function (k) {
            if (String(x.aim[k]) === String(a.id)) delete x.aim[k];
          });
        });
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
      b.addEventListener('change', function () {
        if (!b.value) return;
        addActor(b.dataset.team, b.value);
        b.value = '';
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
        if (a.active[i]) { delete a.active[i]; delete a.aim[i];
                           if (a.backstab) delete a.backstab[i]; }
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
    host.querySelectorAll('.acbsx').forEach(function (b3) {
      b3.addEventListener('change', function () {
        var a = actorById(b3.dataset.a); if (!a) return;
        a.backstab = a.backstab || {};
        if (b3.checked) a.backstab[b3.dataset.i] = 1; else delete a.backstab[b3.dataset.i];
        renderCombat();
      });
    });
    host.querySelectorAll('.acmd').forEach(function (s3) {
      s3.addEventListener('change', function () {
        var a = actorById(s3.dataset.a); if (!a) return;
        var i = s3.dataset.i;
        if (!a.active[i]) return;
        a.active[i] = s3.value;
        // the mode decides who it reaches, so the aim has to be re-derived with it
        var ctx = actorCtx(a);
        var g2 = (ctx.charGear || [])[+i];
        var sc = deviceScope(a, g2, dev0(g2), s3.value);
        delete a.aim[i];
        if (sc.kind === 'single' && sc.picks.length) a.aim[i] = sc.picks[0].id;
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
      if (craftMode) {
        craftSlots = blankSlots(); craftClass = curClass;
        craftEditing = null; craftName = '';
        craftToGear();
      }
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
