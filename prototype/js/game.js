// ===== Dawn of Kings prototype: state, economy, UI, raids =====
const $ = id => document.getElementById(id);
const SAVE_KEY = 'dok_proto_v1';
let S, view = 'home', battle = null, sel = null, buildMode = null, speed = 1, modalOpen = false;
let incoming = null; // {at}
let scout = null;

// ---------- state ----------
function newGame() {
  const r = rng(Date.now());
  S = { v: 1, t: 0, res: { wood: 400, food: 300, gold: 100, stone: 100 }, villagers: 3, vq: [], army: { militia: 0, manatarms: 0, archer: 0 },
    renown: 0, shieldUntil: 0, nextRaid: 0, nid: 1, b: [], nodes: [], camp: {}, seed: Math.floor(r() * 1e9), lastSeen: Date.now(), report: null, rivalSeq: 0 };
  addB('townhall', 13, 13, 1); addB('house', 18, 13, 1); addB('tower', 13, 18, 1);
  const rr = rng(S.seed);
  for (let i = 3; i < 10; i++) for (let j = 3; j < 10; j++) if (rr() < 0.62) S.nodes.push({ x: i, y: j, type: 'tree', amt: 120, max: 120, respawn: 0 });
  [[22, 6], [23, 6], [22, 7], [24, 7], [23, 8]].forEach(([x, y]) => S.nodes.push({ x, y, type: 'gold', amt: 500, max: 500, respawn: 0 }));
  [[6, 22], [7, 22], [6, 23], [8, 23], [7, 24]].forEach(([x, y]) => S.nodes.push({ x, y, type: 'stone', amt: 400, max: 400, respawn: 0 }));
}
function addB(type, x, y, lvl) { const d = BDEF[type]; const b = { id: S.nid++, type, x, y, w: d.w, h: d.h, lvl, workers: 0, build: null, q: [] }; S.b.push(b); return b; }
const tcB = () => S.b.find(b => b.type === 'townhall');
const tcLvl = () => tcB().lvl;
const cap = () => BDEF.townhall.cap[tcLvl() - 1];
const prot = () => BDEF.townhall.prot[tcLvl() - 1];
const done = b => b.lvl >= 1;
const builders = () => S.b.filter(b => b.build).length;
const assigned = () => S.b.reduce((a, b) => a + b.workers, 0);
const idle = () => S.villagers - builders() - assigned();
const vCap = () => 3 + S.b.filter(b => b.type === 'house' && done(b)).length * 5;
const armyCount = () => Object.values(S.army).reduce((a, b) => a + b, 0);
const queuedPop = () => S.b.reduce((a, b) => a + b.q.length, 0);
const housing = () => 5 + S.b.filter(b => b.type === 'armycamp' && done(b)).length * 20;
const canAfford = c => Object.entries(c).every(([k, v]) => S.res[k] >= v);
const pay = c => Object.entries(c).forEach(([k, v]) => (S.res[k] -= v));
const costHtml = c => Object.entries(c).map(([k, v]) => `<span class="${S.res[k] >= v ? '' : 'bad'}">${ICON[k]}${v}</span>`).join(' ');
const scaleCost = (c, k) => Object.fromEntries(Object.entries(c).map(([a, v]) => [a, Math.round(v * k)]));

function save() { try { S.lastSeen = Date.now(); localStorage.setItem(SAVE_KEY, JSON.stringify(S)); } catch (e) {} }
function load() { try { const s = localStorage.getItem(SAVE_KEY); if (s) { S = JSON.parse(s); return true; } } catch (e) {} return false; }

// ---------- economy ----------
function upgradeInfo(b) {
  const d = BDEF[b.type]; const to = b.lvl + 1;
  if (b.build) return { err: 'Busy' };
  if (to > MAXLVL) return { err: 'Max level (prototype)' };
  if (b.type === 'townhall') { const u = TCUP[to]; return { cost: u.cost, time: u.time, label: u.label || 'Upgrade to Level ' + to }; }
  if (to > tcLvl()) return { err: 'Needs Town Center Lv' + to };
  const k = Math.pow(1.6, b.lvl); const base = d.cost || { wood: 100 };
  return { cost: scaleCost(base, k), time: Math.round(d.time * Math.pow(1.9, b.lvl)), label: 'Upgrade to Level ' + to };
}
function startUpgrade(b) {
  const u = upgradeInfo(b); if (u.err) return toast(u.err);
  if (idle() < 1) return toast('No idle villager to build!');
  if (!canAfford(u.cost)) return toast('Not enough resources');
  pay(u.cost); b.build = { start: S.t, end: S.t + u.time, to: b.lvl + 1 }; refreshPanel(); toast('Construction started');
}
function completeBuild(b) {
  const first = b.lvl === 0; b.lvl = b.build.to; b.build = null;
  const d = BDEF[b.type];
  if (d.res && first && idle() > 0 && b.workers === 0) b.workers = 1;
  if (b.type === 'townhall' && b.lvl === 2 && !S.nextRaid) S.nextRaid = S.t + 60;
  if (b.type === 'townhall') banner(b.lvl === 3 ? '🏰 Feudal Age reached!' : '🏰 Town Center Level ' + b.lvl);
  else toast(d.name + ' ready');
  if (sel === b) refreshPanel();
}
function econTick(dt) {
  const t = S.t;
  for (const b of S.b) if (b.build && t >= b.build.end) completeBuild(b);
  for (const n of S.nodes) if (n.amt <= 0 && t >= n.respawn) n.amt = n.max;
  const over = idle(); if (over < 0) { let k = -over; for (const b of S.b) { const m = Math.min(b.workers, k); b.workers -= m; k -= m; if (!k) break; } }
  for (const b of S.b) {
    const d = BDEF[b.type]; if (!d.res || !done(b)) { b.status = ''; continue; }
    if (!b.workers) { b.status = 'idle'; continue; }
    if (S.res[d.res] >= cap()) { b.status = 'full'; continue; }
    let amount = b.workers * d.rate * (1 + 0.15 * (b.lvl - 1)) * dt / 60;
    if (d.node) {
      const cx = b.x + b.w / 2, cy = b.y + b.h / 2;
      const ns = S.nodes.filter(n => n.type === d.node && n.amt > 0 && Math.hypot(n.x + .5 - cx, n.y + .5 - cy) <= 7).sort((a, c) => Math.hypot(a.x - cx, a.y - cy) - Math.hypot(c.x - cx, c.y - cy));
      if (!ns.length) { b.status = 'none'; continue; }
      let got = 0;
      for (const n of ns) { const take = Math.min(n.amt, amount - got); n.amt -= take; got += take; if (n.amt <= 0) n.respawn = t + 300; if (got >= amount - 1e-9) break; }
      amount = got;
    }
    b.status = 'work'; b.acc = (b.acc || 0) + amount;
    const whole = Math.floor(b.acc); if (whole) { b.acc -= whole; S.res[d.res] = Math.min(cap(), S.res[d.res] + whole); }
  }
  // villager queue
  if (S.vq.length) { S.vq[0] -= dt; if (S.vq[0] <= 0 && S.villagers < vCap()) { S.villagers++; S.vq.shift(); } else if (S.vq[0] < 0) S.vq[0] = 0; }
  // unit queues
  for (const b of S.b) if (b.q.length) { const q = b.q[0]; q.left -= dt; if (q.left <= 0) { S.army[q.type]++; b.q.shift(); } }
}

function trainVillager() {
  if (S.villagers + S.vq.length >= vCap()) return toast('Build more Houses for villagers');
  if (!canAfford({ food: 25 })) return toast('Need 🌾25');
  pay({ food: 25 }); S.vq.push(10); refreshPanel();
}
function trainUnit(b, type) {
  const d = UDEF[type];
  if (armyCount() + queuedPop() + d.pop > housing()) return toast('Army housing full — build an Army Camp');
  if (b.q.length >= 8) return toast('Queue full');
  if (!canAfford(d.cost)) return toast('Not enough resources');
  pay(d.cost); b.q.push({ type, left: d.time }); refreshPanel();
}

// ---------- build placement ----------
function buildCount(type) { return S.b.filter(b => b.type === type).length; }
function canBuildInfo(type) {
  const d = BDEF[type];
  if ((d.unlock || 1) > tcLvl()) return { err: 'Needs Town Center Lv' + d.unlock };
  const mx = d.max[tcLvl() - 1];
  if (buildCount(type) >= mx) return { err: mx === 0 ? 'Locked' : 'Limit reached (' + mx + ')' };
  return {};
}
function placeOk(type, x, y) {
  const d = BDEF[type];
  if (x < 1 || y < 1 || x + d.w > MAP - 1 || y + d.h > MAP - 1) return false;
  for (const b of S.b) if (x < b.x + b.w && x + d.w > b.x && y < b.y + b.h && y + d.h > b.y) return false;
  for (const n of S.nodes) if (n.amt > 0 && n.x >= x && n.x < x + d.w && n.y >= y && n.y < y + d.h) return false;
  return true;
}
function enterBuildMode(type) {
  const info = canBuildInfo(type); if (info.err) return toast(info.err);
  const d = BDEF[type];
  if (!canAfford(d.cost)) return toast('Not enough resources');
  if (idle() < 1) return toast('No idle villager available');
  closeModal(); select(null);
  const c = screenToGrid(innerWidth / 2, innerHeight / 2);
  buildMode = { type, x: Math.round(c.x - d.w / 2), y: Math.round(c.y - d.h / 2) };
  buildMode.ok = placeOk(type, buildMode.x, buildMode.y);
  $('homeBtns').style.display = 'none'; $('rightBtns').style.display = 'none'; $('buildBtns').style.display = 'flex';
}
function exitBuildMode() { buildMode = null; $('homeBtns').style.display = 'flex'; $('rightBtns').style.display = 'flex'; $('buildBtns').style.display = 'none'; }
function confirmBuild() {
  const m = buildMode; if (!m) return;
  if (!m.ok) return toast('Cannot build there');
  const d = BDEF[m.type];
  if (!canAfford(d.cost) || idle() < 1) { exitBuildMode(); return toast('Not enough resources / villagers'); }
  pay(d.cost);
  const b = addB(m.type, m.x, m.y, 0); b.build = { start: S.t, end: S.t + d.time, to: 1 };
  exitBuildMode(); save();
}

// ---------- UI ----------
let toastT = 0;
function toast(msg) { const e = $('toast'); e.textContent = msg; e.style.opacity = 1; clearTimeout(toastT); toastT = setTimeout(() => (e.style.opacity = 0), 1800); }
let bannerT = 0;
function banner(msg) { const e = $('banner'); e.textContent = msg; e.style.opacity = 1; clearTimeout(bannerT); bannerT = setTimeout(() => (e.style.opacity = 0), 2800); }
function showModal(html) { $('modal').innerHTML = html; $('modalWrap').style.display = 'flex'; modalOpen = true; }
function closeModal() { $('modalWrap').style.display = 'none'; modalOpen = false; }
$('modalWrap').addEventListener('pointerdown', e => { if (e.target.id === 'modalWrap' && view === 'home') closeModal(); });

function select(b) {
  if (sel) sel.sel = false; sel = b; if (b) b.sel = true;
  $('panel').style.display = b ? 'block' : 'none'; if (b) refreshPanel();
}
function setWorkers(b, d) {
  const def = BDEF[b.type], slots = def.slots[b.lvl - 1];
  if (d > 0 && (idle() < 1 || b.workers >= slots)) return toast(idle() < 1 ? 'No idle villagers' : 'Camp is full');
  if (d < 0 && b.workers < 1) return;
  b.workers += d; refreshPanel();
}
function refreshPanel() {
  const b = sel; if (!b) return; const d = BDEF[b.type];
  let h = `<h3>${d.icon} ${d.name} <span class="dim">Lv${Math.max(b.lvl, b.build ? b.build.to : b.lvl)}</span></h3><div class="dim">${d.desc || ''}</div>`;
  if (b.build) {
    const left = b.build.end - S.t, f = clamp(1 - left / (b.build.end - b.build.start), 0, 1);
    h += `<div class="row">🔨 ${b.lvl === 0 ? 'Constructing' : 'Upgrading to Lv' + b.build.to}… ${fmtTime(left)}</div><div class="prog"><i style="width:${f * 100}%"></i></div>`;
  }
  if (d.res && done(b)) {
    const slots = d.slots[b.lvl - 1], mult = (1 + 0.15 * (b.lvl - 1));
    h += `<div class="row">👷 Workers <button onclick="setWorkers(sel,-1)">−</button><b>${b.workers}/${slots}</b><button onclick="setWorkers(sel,1)">+</button><span class="dim">≈ ${(b.workers * d.rate * mult).toFixed(0)} ${ICON[d.res]}/min</span></div>`;
    if (b.status === 'none') h += '<div class="bad">No resources left nearby (they regrow in 5 min).</div>';
    if (b.status === 'full') h += '<div class="bad">Storage full — spend some!</div>';
  }
  if (b.type === 'townhall') {
    h += `<div class="row"><button onclick="trainVillager()">Train Villager ${costHtml({ food: 25 })}</button><span class="dim">${S.villagers}/${vCap()} • queue ${S.vq.length}</span></div>`;
  }
  const trains = Object.entries(UDEF).filter(([k, u]) => u.bld === b.type && u.unlock <= tcLvl());
  if (done(b) && trains.length) {
    h += `<div class="row">` + trains.map(([k, u]) => `<button onclick="trainUnit(sel,'${k}')">${u.icon} ${u.name} ${costHtml(u.cost)}</button>`).join('') + '</div>';
    h += `<div class="dim">Army ${armyCount()}+${queuedPop()} / ${housing()} housing</div>`;
    if (b.q.length) h += `<div class="dim">Training: ${b.q.map(q => UDEF[q.type].icon).join(' ')} (${Math.ceil(b.q[0].left)}s)</div>`;
  }
  if (done(b) && (d.dps)) h += `<div class="dim">🎯 ${d.dps[b.lvl - 1]} dmg/s • range ${d.range}</div>`;
  if (done(b) && !b.build) {
    const u = upgradeInfo(b);
    h += `<div class="row">` + (u.err ? `<button disabled>${u.err}</button>` : `<button class="pri" onclick="startUpgrade(sel)">${u.label} ${costHtml(u.cost)} ⏱${fmtTime(u.time)}</button>`) + `</div>`;
  }
  $('panel').innerHTML = h;
}

function openBuildMenu() {
  let h = '<h2>🔨 Build</h2><div class="dim">Idle villagers: ' + idle() + '. Each build needs one villager for its duration.</div>';
  for (const cat of ['Economy', 'Military', 'Defence']) {
    h += `<h3>${cat}</h3><div class="list">`;
    for (const [k, d] of Object.entries(BDEF)) if (d.cat === cat) {
      const info = canBuildInfo(k);
      h += `<button ${info.err ? 'disabled' : ''} onclick="enterBuildMode('${k}')"><span>${d.icon} ${d.name}<br><small class="dim">${info.err || d.desc}</small></span><span>${costHtml(d.cost)}<br><small class="dim">⏱${fmtTime(d.time)} • ${buildCount(k)}/${d.max[tcLvl() - 1]}</small></span></button>`;
    }
    h += '</div>';
  }
  showModal(h + '<div class="row"><button onclick="closeModal()">Close</button></div>');
}
function openArmy() {
  let h = `<h2>🛡️ Army</h2><div class="dim">Housing ${armyCount()}+${queuedPop()} / ${housing()}. Train units at the Barracks / Archery Range.</div><div class="list">`;
  for (const [k, u] of Object.entries(UDEF)) h += `<div class="row">${u.icon} <b>${u.name}</b> × ${S.army[k]} <span class="dim">HP ${u.hp} • DMG ${u.dmg} • ${u.ranged ? 'ranged' : 'melee'}${u.unlock > tcLvl() ? ' • 🔒 TC Lv' + u.unlock : ''}</span></div>`;
  showModal(h + '</div><div class="row"><button onclick="closeModal()">Close</button></div>');
}

// ---------- ladder ----------
let LORDS = null;
function lords() {
  if (LORDS) return LORDS; const r = rng(S.seed ^ 0x9e37); LORDS = [];
  for (let i = 0; i < 300; i++) LORDS.push({ i, name: LORD_NAMES[Math.floor(r() * LORD_NAMES.length)] + ' ' + LORD_TITLES[Math.floor(r() * LORD_TITLES.length)], r0: Math.floor(Math.pow(r(), 1.8) * 7000), ph: r() * 6.28 });
  return LORDS;
}
const lordRenown = l => Math.max(0, Math.round(l.r0 + 70 * Math.sin(S.t / 600 + l.ph)));
function openLadder() {
  const all = lords().map(l => ({ n: l.name, r: lordRenown(l), me: false })); all.push({ n: 'You', r: S.renown, me: true });
  all.sort((a, b) => b.r - a.r || (a.me ? -1 : 1)); const me = all.findIndex(x => x.me);
  const from = clamp(me - 5, 0, all.length - 11);
  let h = `<h2>🏆 Ladder</h2><div>Your rank: <b class="me">${rankOf(S.renown)}</b> • #${me + 1} of ${all.length} • ${S.renown} Renown</div><table class="tbl">`;
  for (let i = from; i < from + 11; i++) { const x = all[i]; h += `<tr class="${x.me ? 'me' : ''}"><td>#${i + 1}</td><td>${x.n}</td><td>${rankOf(x.r)}</td><td align="right">${x.r}</td></tr>`; }
  showModal(h + '</table><div class="row dim">Next ranks: ' + RANKS.filter(r => r[1] > S.renown).slice(0, 2).map(r => r[0] + ' @ ' + r[1]).join(', ') + '</div><div class="row"><button onclick="closeModal()">Close</button></div>');
}

// ---------- raiding (offence) ----------
function barbCooldown(i) { return Math.max(0, ((S.camp[i] || 0) - S.t)); }
function openRaid() {
  if (armyCount() < 1) { toast('Train some troops first (Barracks)'); }
  let h = '<h2>⚔️ Raid</h2><div class="dim">Army: ' + Object.entries(S.army).filter(([k, v]) => v).map(([k, v]) => UDEF[k].icon + '×' + v).join(' ') + (armyCount() ? '' : 'none') + '</div><h3>Barbarian Camps</h3><div class="list">';
  for (let i = 1; i <= 5; i++) {
    const g = genBase(1000 + i, 1, 'barb', i), locked = i > tcLvl() + 1, cd = barbCooldown(i);
    h += `<button ${locked || cd > 0 || !armyCount() ? 'disabled' : ''} onclick="startBarb(${i})"><span>🏕️ Camp ${i}<br><small class="dim">${locked ? 'Needs TC Lv' + (i - 1) : cd > 0 ? 'Recovering ' + fmtTime(cd) : g.buildings.length + ' buildings'}</small></span><span>${costHtml(g.loot)}</span></button>`;
  }
  h += '</div><h3>Rival Lords</h3><div class="list"><button ' + (armyCount() ? '' : 'disabled') + ' onclick="findRival(true)"><span>🏰 Find a Rival<br><small class="dim">Ranked: wins raise your Renown, losses lower it</small></span><span>free</span></button></div>';
  showModal(h + '<div class="row"><button onclick="closeModal()">Close</button></div>');
}
function startBarb(i) {
  const g = genBase(1000 + i, 1, 'barb', i);
  scout = { kind: 'barb', i, base: g, title: g.name, renown: { win: 4 + 2 * i, lose: 0 } }; showScout();
}
function findRival(first) {
  if (!first) { if (S.res.gold < 30) return toast('Need 🪙30 to search again'); S.res.gold -= 30; }
  S.rivalSeq++;
  const L = lords(), pool = L.filter(l => Math.abs(lordRenown(l) - S.renown) <= 250);
  const arr = pool.length ? pool : L.slice().sort((a, b) => Math.abs(lordRenown(a) - S.renown) - Math.abs(lordRenown(b) - S.renown)).slice(0, 8);
  const r = rng(S.rivalSeq * 7919 + S.seed); const lord = arr[Math.floor(r() * arr.length)];
  const tc = clamp(tcLvl() + (r() < 0.3 ? 1 : r() < 0.3 ? -1 : 0), 1, 3);
  const g = genBase(lord.i * 131 + S.rivalSeq, tc, 'rival');
  const opp = lordRenown(lord);
  const E = expected(S.renown, opp);
  scout = { kind: 'rival', base: g, title: lord.name, opp, tc, renown: { win: Math.round(40 * (1 - E)), lose: Math.round(40 * E) }, oppE: E };
  showScout();
}
function showScout() {
  const s = scout, g = s.base;
  const lootShown = Object.fromEntries(Object.entries(g.loot).map(([k, v]) => [k, s.kind === 'rival' ? Math.round(v * 0.2) : v]));
  let h = `<h2>${s.kind === 'rival' ? '🏰 ' : '🏕️ '}${s.title}</h2>`;
  if (s.kind === 'rival') h += `<div class="dim">${g.pers} • ${rankOf(s.opp)} • ${s.opp} Renown • Town Center Lv${s.tc}</div>`;
  h += `<div class="row">Available loot: ${costHtml(lootShown)}</div>`;
  h += `<div class="row dim">Defences: ${g.buildings.filter(b => b.dps).length} towers • ${g.buildings.length} buildings</div>`;
  h += `<div class="row dim">Renown: <span style="color:var(--ok)">+${s.renown.win}</span> win / <span class="bad">−${s.renown.lose}</span> lose</div>`;
  h += `<div class="row"><button class="pri" onclick="beginBattle()">⚔️ Attack</button>${s.kind === 'rival' ? '<button onclick="findRival(false)">Next 🪙30</button>' : ''}<button onclick="closeModal()">Cancel</button></div>`;
  showModal(h);
  // preview behind modal
  battle = null;
}
function beginBattle() {
  closeModal(); const s = scout;
  const sim = new Sim(s.base.buildings, { mode: 'attack', reserve: () => armyCount() });
  battle = { sim, kind: s.kind, scout: s, sel: null, lootFrac: 1, done: false };
  pickTroop(); view = 'battle'; select(null);
  centerCamera(); setBattleUI(true);
}
function troopTypes() { return Object.keys(UDEF).filter(k => S.army[k] > 0); }
function pickTroop() { if (!battle) return; const t = troopTypes(); if (!t.includes(battle.sel)) battle.sel = t[0] || null; renderTroopBar(); }
function renderTroopBar() {
  $('troopBtns').innerHTML = troopTypes().map(k => `<button class="${battle.sel === k ? 'pri' : ''}" onclick="battle.sel='${k}';renderTroopBar()">${UDEF[k].icon} ${S.army[k]}</button>`).join('');
}
function setBattleUI(on, defence) {
  $('homeBtns').style.display = on ? 'none' : 'flex'; $('rightBtns').style.display = on ? 'none' : 'flex';
  $('troops').style.display = on && !defence ? 'flex' : 'none'; $('troops').style.gap = '8px';
  $('battleTop').style.display = on ? 'flex' : 'none';
  $('bar').style.justifyContent = on ? 'center' : 'space-between';
}
function deployAt(gx, gy) {
  const bt = battle; if (!bt || bt.sim.over || bt.kind === 'defence') return;
  if (!bt.sel || S.army[bt.sel] <= 0) return toast('No troops selected');
  if (gx < 0.3 || gy < 0.3 || gx > MAP - 0.3 || gy > MAP - 0.3) return;
  for (const b of bt.sim.b) if (!b.dead && distRect(gx, gy, b) < 2.5) return toast('Deploy farther from buildings');
  S.army[bt.sel]--; bt.sim.deploy(bt.sel, gx, gy); pickTroop();
}
function endBattleNow() { if (battle) { battle.sim.over = true; } }

function finishAttack() {
  const bt = battle, sim = bt.sim, s = bt.scout; bt.done = true;
  const stars = sim.stars(), pct = sim.pct();
  const survivors = {}; for (const u of sim.units) survivors[u.type] = (survivors[u.type] || 0) + 1;
  const sent = {}; // lost = deployed - survivors; we track via counts snapshot
  Object.entries(survivors).forEach(([k, v]) => (S.army[k] += v));
  const lootFull = Object.fromEntries(Object.entries(s.base.loot).map(([k, v]) => [k, s.kind === 'rival' ? Math.round(v * 0.2) : v]));
  const got = {}; for (const k of RES) { const v = Math.floor(lootFull[k] * pct / 100); got[k] = Math.max(0, Math.min(v, cap() - S.res[k])); S.res[k] += got[k]; }
  const win = stars >= 1; let dr = 0;
  if (s.kind === 'rival') { dr = win ? s.renown.win + [0, 0, 2, 4][stars] : -s.renown.lose; }
  else dr = win ? s.renown.win : 0;
  S.renown = Math.max(0, S.renown + dr);
  if (s.kind === 'barb' && win) S.camp[s.i] = S.t + 180;
  let h = `<h2>${win ? '🏆 Victory!' : '💀 Defeat'}</h2><div style="font-size:28px">${'★'.repeat(stars)}${'☆'.repeat(3 - stars)}</div><div>${pct}% destroyed</div>`;
  h += `<div class="row">Loot: ${costHtml(got)}</div><div class="row">Renown: <b style="color:${dr >= 0 ? 'var(--ok)' : 'var(--bad)'}">${dr >= 0 ? '+' : ''}${dr}</b> → ${S.renown} (${rankOf(S.renown)})</div>`;
  h += `<div class="row dim">Surviving troops return home: ${Object.entries(survivors).map(([k, v]) => UDEF[k].icon + '×' + v).join(' ') || 'none'}</div><div class="row"><button class="pri" onclick="goHome()">Return Home</button></div>`;
  showModal(h);
}
function goHome() { closeModal(); battle = null; view = 'home'; setBattleUI(false); centerCamera(); save(); }

// ---------- raids on the player ----------
function raiderList() {
  const tc = tcLvl(), n = 3 + tc * 2 + Math.floor(S.renown / 250), out = [];
  for (let i = 0; i < n; i++) out.push(tc >= 3 && i % 3 === 0 ? 'archer' : tc >= 3 && i % 4 === 1 ? 'manatarms' : 'militia');
  return out;
}
function makeDefenceSim() {
  const bs = S.b.filter(done).map(b => makeSimB(b.type, b.lvl, b.x, b.y));
  const sim = new Sim(bs, { mode: 'defence', started: true });
  const r = rng(Math.floor(S.t) + S.seed); const edge = () => { const s = Math.floor(r() * 4), p = 2 + r() * (MAP - 4); return s === 0 ? [p, 0.6] : s === 1 ? [p, MAP - 0.6] : s === 2 ? [0.6, p] : [MAP - 0.6, p]; };
  const [ex, ey] = edge(); raiderList().forEach((type, i) => { const [x, y] = r() < 0.7 ? [ex + (r() - .5) * 3, ey + (r() - .5) * 3] : edge(); sim.sq.push({ t: i * 0.5, type, x: clamp(x, .5, MAP - .5), y: clamp(y, .5, MAP - .5) }); });
  return sim;
}
function applyDefence(sim) {
  const stars = sim.stars(), pct = sim.pct();
  const opp = Math.max(0, S.renown + Math.round((Math.random() - 0.5) * 200)), E = expected(S.renown, opp);
  const lord = lords()[Math.floor(Math.random() * 300)];
  const stolen = {}; for (const k of RES) { const lootable = Math.max(0, S.res[k] - prot()); const v = Math.floor(lootable * 0.2 * pct / 100); stolen[k] = v; S.res[k] -= v; }
  const held = stars <= 1; let dr = held ? Math.round(0.5 * 40 * (1 - E)) : -Math.round(0.5 * 40 * E);
  const before = S.renown; S.renown = Math.max(0, S.renown + dr); dr = S.renown - before;
  S.shieldUntil = S.t + [0, 60, 120, 180][stars]; S.nextRaid = S.t + 200 + Math.random() * 120;
  return { who: lord.name, stars, pct, stolen, dr, held };
}
function reportHtml(r, away) {
  return `<h2>${r.held ? '🛡️ Raid repelled!' : '🔥 Village raided!'}</h2><div class="dim">${away ? 'While you were away, ' : ''}${r.who} attacked your village.</div><div style="font-size:26px">${'★'.repeat(r.stars)}${'☆'.repeat(3 - r.stars)}</div><div>${r.pct}% of your buildings were damaged (they have been rebuilt).</div><div class="row">Stolen: ${costHtml(r.stolen)}</div><div class="row">Renown: <b style="color:${r.dr >= 0 ? 'var(--ok)' : 'var(--bad)'}">${r.dr >= 0 ? '+' : ''}${r.dr}</b> → ${S.renown}</div><div class="dim">Tip: more towers and upgrades near your storages reduce losses. A shield protects you for a short while.</div>`;
}
function startDefence() {
  incoming = null; closeModal(); select(null); exitBuildMode();
  battle = { sim: makeDefenceSim(), kind: 'defence', done: false }; view = 'battle'; centerCamera(); setBattleUI(true, true);
  $('alert').style.display = 'none';
}
function finishDefence() {
  const bt = battle; bt.done = true; const r = applyDefence(bt.sim);
  showModal(reportHtml(r) + '<div class="row"><button class="pri" onclick="goHome()">Continue</button></div>');
}
function headlessRaid() {
  const sim = makeDefenceSim(); let n = 0; while (!sim.over && n++ < 20000) sim.step(0.05);
  return applyDefence(sim);
}
function raidCheck() {
  if (tcLvl() < 2 || !S.nextRaid || modalOpen || buildMode) return;
  if (!incoming && S.t >= S.nextRaid && S.t >= S.shieldUntil) incoming = { at: S.t + 20 };
  if (incoming) {
    const left = incoming.at - S.t; $('alert').style.display = 'block'; $('alert').textContent = `⚠️ Raiders approach! ${Math.ceil(left)}s — garrison up!`;
    if (left <= 0) startDefence();
  }
}

// ---------- hud / hints ----------
function villagersDraw() {
  const out = [], t = performance.now() / 1000; let nIdle = Math.max(0, idle());
  const tc = tcB();
  for (const b of S.b) {
    const d = BDEF[b.type]; const cx = b.x + b.w / 2, cy = b.y + b.h / 2;
    if (b.build) out.push({ x: b.x + b.w + 0.2, y: cy + Math.sin(t * 2) * 0.3, c: '#9fc5ff', s: 1 });
    if (d.res && done(b) && b.workers && b.status === 'work') {
      let tgt = null, bd = 99;
      if (d.node) for (const n of S.nodes) if (n.type === d.node && n.amt > 0) { const dd = Math.hypot(n.x + .5 - cx, n.y + .5 - cy); if (dd < bd && dd <= 7) { bd = dd; tgt = n; } }
      for (let k = 0; k < b.workers; k++) {
        if (tgt) { const ph = ((t * 0.35 + k * 0.31) % 2), f = ph < 1 ? ph : 2 - ph; out.push({ x: cx + (tgt.x + .5 - cx) * f * 0.92 + k * 0.1, y: cy + (tgt.y + .5 - cy) * f * 0.92, c: '#f0e4c8' }); }
        else out.push({ x: cx + 1.4 + Math.sin(t + k) * 0.3, y: cy + Math.cos(t) * 0.3, c: '#f0e4c8' });
      }
    }
  }
  for (let k = 0; k < Math.min(nIdle, 8); k++) out.push({ x: tc.x + 1.5 + Math.cos(t * 0.5 + k * 1.7) * 2.8, y: tc.y + 3.6 + Math.sin(t * 0.4 + k * 2.3) * 0.8, c: '#ffffff' });
  return out;
}
function updateHud() {
  if (view === 'home') {
    let h = RES.map(k => `<div class="chip">${ICON[k]} ${Math.floor(S.res[k])}<small>/${cap()}</small></div>`).join('');
    $('res').innerHTML = h;
    const shield = S.shieldUntil > S.t ? ` • 🛡️ ${fmtTime(S.shieldUntil - S.t)}` : '';
    $('meta').innerHTML = `<div class="chip">👷 ${idle()} idle <small>/ ${S.villagers}/${vCap()}</small></div><div class="chip">${AGES[ageOf(tcLvl())]} <small>TC${tcLvl()}</small></div><div class="chip">🏆 ${S.renown} <small>${rankOf(S.renown)}${shield}</small></div><button id="btnSpeed" style="min-height:34px;padding:4px 10px">⏩ ×${speed}</button>`;
    $('btnSpeed').onclick = () => { speed = speed === 1 ? 5 : speed === 5 ? 20 : 1; };
    $('hint').textContent = hint();
  } else if (battle) {
    const sim = battle.sim;
    $('res').innerHTML = ''; $('meta').innerHTML = ''; $('hint').textContent = battle.kind === 'defence' ? '🛡️ Your village is under attack!' : (sim.started ? '' : 'Tap the grass to deploy troops (away from buildings)');
    $('bTime').textContent = '⏱ ' + fmtTime(sim.timeLeft); $('bPct').textContent = '💥 ' + sim.pct() + '%'; $('bStars').textContent = '★'.repeat(sim.stars()) + '☆'.repeat(3 - sim.stars());
  }
}
function hint() {
  if (buildMode) return 'Tap the map to move the building, then ✔ Place';
  const has = t => S.b.some(b => b.type === t);
  if (!has('lumber')) return '1️⃣ Build a Lumber Camp (🔨 Build) next to the trees';
  if (!has('farm')) return '2️⃣ Build a Farm for food';
  if (!has('barracks')) return '3️⃣ Build a Barracks to train Militia';
  if (armyCount() + queuedPop() < 3 && S.b.find(b => b.type === 'barracks' && done(b))) return '4️⃣ Select the Barracks and train some Militia';
  if (S.renown === 0 && armyCount() >= 3) return '5️⃣ Press ⚔️ Raid and attack Barbarian Camp 1';
  if (tcLvl() < 3 && S.b.every(b => !b.build) ) return '🏰 Select the Town Center and upgrade it (→ Feudal Age at Lv3)';
  return '';
}

// ---------- input ----------
const ptrs = new Map(); let downAt = null, pinch = null;
cvs.addEventListener('pointerdown', e => {
  cvs.setPointerCapture(e.pointerId); ptrs.set(e.pointerId, { x: e.clientX, y: e.clientY });
  if (ptrs.size === 1) downAt = { x: e.clientX, y: e.clientY, moved: false };
  if (ptrs.size === 2) { const [a, b] = [...ptrs.values()]; pinch = { d: Math.hypot(a.x - b.x, a.y - b.y), z: cam.zoom }; downAt = null; }
});
cvs.addEventListener('pointermove', e => {
  const p = ptrs.get(e.pointerId); if (!p) return;
  const dx = e.clientX - p.x, dy = e.clientY - p.y; p.x = e.clientX; p.y = e.clientY;
  if (ptrs.size === 2 && pinch) { const [a, b] = [...ptrs.values()]; const d = Math.hypot(a.x - b.x, a.y - b.y); zoomAt((a.x + b.x) / 2, (a.y + b.y) / 2, pinch.z * d / pinch.d); return; }
  if (downAt) { if (Math.hypot(e.clientX - downAt.x, e.clientY - downAt.y) > 6) downAt.moved = true; if (downAt.moved) { cam.x += dx; cam.y += dy; } }
});
function endPtr(e) {
  if (downAt && !downAt.moved && ptrs.size === 1) onTap(e.clientX, e.clientY);
  ptrs.delete(e.pointerId); if (ptrs.size < 2) pinch = null; if (ptrs.size === 0) downAt = null;
}
cvs.addEventListener('pointerup', endPtr); cvs.addEventListener('pointercancel', e => { ptrs.delete(e.pointerId); pinch = null; downAt = null; });
cvs.addEventListener('wheel', e => { e.preventDefault(); zoomAt(e.clientX, e.clientY, cam.zoom * (e.deltaY < 0 ? 1.1 : 0.9)); }, { passive: false });
function zoomAt(x, y, z) { z = clamp(z, 0.35, 2); const g = screenToGrid(x, y), k = z / cam.zoom; cam.x = x - (x - cam.x) * k; cam.y = y - (y - cam.y) * k; cam.zoom = z; }
function onTap(sx, sy) {
  if (modalOpen) return; const g = screenToGrid(sx, sy);
  if (view === 'battle') { if (battle && battle.kind !== 'defence') deployAt(g.x, g.y); return; }
  if (buildMode) { const d = BDEF[buildMode.type]; buildMode.x = Math.round(g.x - d.w / 2); buildMode.y = Math.round(g.y - d.h / 2); buildMode.ok = placeOk(buildMode.type, buildMode.x, buildMode.y); return; }
  const b = S.b.find(b => g.x >= b.x && g.x < b.x + b.w && g.y >= b.y && g.y < b.y + b.h) || S.b.find(b => g.x >= b.x - .6 && g.x < b.x + b.w + .6 && g.y >= b.y - .6 && g.y < b.y + b.h + .6 && false);
  if (b) { select(b); return; }
  const n = S.nodes.find(n => n.amt > 0 && g.x >= n.x && g.x < n.x + 1.2 && g.y >= n.y && g.y < n.y + 1.2);
  if (n) { select(null); return toast(`${n.type === 'tree' ? '🌲 Tree' : n.type === 'gold' ? '🪙 Gold' : '🪨 Stone'}: ${Math.floor(n.amt)} left`); }
  select(null);
}
$('btnBuild').onclick = openBuildMenu; $('btnArmy').onclick = openArmy; $('btnLadder').onclick = openLadder; $('btnRaid').onclick = openRaid;
$('btnOk').onclick = confirmBuild; $('btnCancel').onclick = exitBuildMode; $('btnEnd').onclick = endBattleNow;

// ---------- main loop ----------
let last = performance.now(), panelT = 0, saveT = 0;
function frame(now) {
  const dt = Math.min(0.1, (now - last) / 1000); last = now;
  if (view === 'home') {
    S.t += dt * speed; econTick(dt * speed); raidCheck();
    panelT += dt; if (panelT > 0.4) { panelT = 0; if (sel && !modalOpen) refreshPanel(); }
    saveT += dt; if (saveT > 5) { saveT = 0; save(); }
  } else if (battle && !battle.done) {
    battle.sim.step(dt * (battle.kind === 'defence' ? Math.min(speed, 5) : 1));
    if (battle.sim.over) { battle.kind === 'defence' ? finishDefence() : finishAttack(); }
  }
  updateHud();
  if (view === 'home') {
    if (buildMode) buildMode.ok = placeOk(buildMode.type, buildMode.x, buildMode.y);
    render({ buildings: S.b, nodes: S.nodes, villagers: villagersDraw(), tc: tcLvl(), t: S.t, ghost: buildMode, showGrid: !!buildMode });
  } else if (battle) {
    render({ buildings: battle.sim.b, sim: battle.sim, tc: battle.kind === 'defence' ? tcLvl() : (battle.scout.tc || 1), t: S.t, deployZone: battle.kind !== 'defence' });
  }
  requestAnimationFrame(frame);
}

// ---------- boot ----------
function boot() {
  if (!load()) newGame();
  centerCamera();
  const away = clamp((Date.now() - S.lastSeen) / 1000, 0, 28800);
  if (away > 10) {
    const before = { ...S.res };
    for (let left = away; left > 0; left -= 10) { const dt = Math.min(10, left); S.t += dt; econTick(dt); }
    const gain = {}; for (const k of RES) gain[k] = Math.floor(S.res[k] - before[k]);
    let h = `<h2>👋 Welcome back</h2><div class="dim">You were away ${fmtTime(away)}.</div><div class="row">Gathered: ${costHtml(gain)}</div>`;
    if (tcLvl() >= 2 && S.nextRaid && S.t >= S.nextRaid && S.t >= S.shieldUntil) { const r = headlessRaid(); h += '<hr>' + reportHtml(r, true); }
    showModal(h + '<div class="row"><button class="pri" onclick="closeModal()">Continue</button></div>');
  }
  if (!S.seenIntro) { S.seenIntro = true; showModal('<h2>🏰 Dawn of Kings</h2><div>Your Town Center, a House and a Watch Tower are all you have. Gather resources, train troops, raid barbarians and rival lords, and climb the ranks from Peasant to Emperor.</div><div class="row dim">Drag to pan • wheel / pinch to zoom • tap buildings to select.<br>Use ⏩ (top right) to speed up timers while testing.</div><div class="row"><button class="pri" onclick="closeModal()">Begin</button></div>'); }
  requestAnimationFrame(frame);
}
addEventListener('beforeunload', save);
document.addEventListener('visibilitychange', () => { if (document.hidden) save(); });
window.DOK = { get S() { return S; }, newGame, save };
boot();
