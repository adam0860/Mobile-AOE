// ===== Isometric canvas renderer =====
const cvs = document.getElementById('c');
const ctx = cvs.getContext('2d');
let DPR = 1;
const cam = { x: 0, y: 0, zoom: 1 };

function resize() {
  DPR = window.devicePixelRatio || 1;
  cvs.width = innerWidth * DPR; cvs.height = innerHeight * DPR;
}
addEventListener('resize', resize); resize();

const iso = (gx, gy) => ({ x: (gx - gy) * TW / 2, y: (gx + gy) * TH / 2 });
function screenToGrid(sx, sy) {
  const wx = (sx - cam.x) / cam.zoom, wy = (sy - cam.y) / cam.zoom;
  return { x: (wx / (TW / 2) + wy / (TH / 2)) / 2, y: (wy / (TH / 2) - wx / (TW / 2)) / 2 };
}
function centerCamera() {
  cam.zoom = Math.min(1, Math.max(0.45, Math.min(innerWidth / (MAP * TW * 0.8), innerHeight / (MAP * TH * 1.1))));
  const c = iso(MAP / 2, MAP / 2);
  cam.x = innerWidth / 2 - c.x * cam.zoom; cam.y = innerHeight / 2 - c.y * cam.zoom + 10;
}

function mix(a, b, t) {
  const pa = [1, 3, 5].map(i => parseInt(a.substr(i, 2), 16)), pb = [1, 3, 5].map(i => parseInt(b.substr(i, 2), 16));
  return '#' + pa.map((v, i) => Math.round(v + (pb[i] - v) * t).toString(16).padStart(2, '0')).join('');
}
function shade(c, f) { return mix(c, f < 0 ? '#000000' : '#ffffff', Math.abs(f)); }
function poly(pts, fill, stroke) {
  ctx.beginPath(); ctx.moveTo(pts[0].x, pts[0].y); for (let i = 1; i < pts.length; i++) ctx.lineTo(pts[i].x, pts[i].y); ctx.closePath();
  if (fill) { ctx.fillStyle = fill; ctx.fill(); } if (stroke) { ctx.strokeStyle = stroke; ctx.lineWidth = 1; ctx.stroke(); }
}
function box(x, y, w, h, hgt, color, alpha) {
  const n = iso(x, y), e = iso(x + w, y), s = iso(x + w, y + h), wv = iso(x, y + h);
  const up = p => ({ x: p.x, y: p.y - hgt });
  ctx.globalAlpha = alpha == null ? 1 : alpha;
  poly([wv, s, up(s), up(wv)], shade(color, -0.25), 'rgba(0,0,0,.25)');
  poly([s, e, up(e), up(s)], shade(color, -0.42), 'rgba(0,0,0,.25)');
  poly([up(n), up(e), up(s), up(wv)], shade(color, 0.12), 'rgba(0,0,0,.25)');
  ctx.globalAlpha = 1;
  const c = iso(x + w / 2, y + h / 2); return { x: c.x, y: c.y - hgt };
}
function bar(px, py, w, f, col) {
  ctx.fillStyle = 'rgba(0,0,0,.6)'; ctx.fillRect(px - w / 2, py, w, 5);
  ctx.fillStyle = col; ctx.fillRect(px - w / 2 + 1, py + 1, Math.max(0, (w - 2) * f), 3);
}
function glyph(txt, x, y, size) { ctx.font = size + 'px serif'; ctx.textAlign = 'center'; ctx.textBaseline = 'middle'; ctx.fillStyle = '#fff'; ctx.fillText(txt, x, y); }

function drawUnit(x, y, color, size, swing, enemy) {
  const p = iso(x, y), bob = Math.sin(performance.now() / 120 + x * 7) * 1.2;
  ctx.fillStyle = 'rgba(0,0,0,.28)'; ctx.beginPath(); ctx.ellipse(p.x, p.y, size * .9, size * .4, 0, 0, 7); ctx.fill();
  ctx.fillStyle = color; ctx.beginPath(); ctx.arc(p.x, p.y - size * 1.1 + bob - (swing ? 3 : 0), size, 0, 7); ctx.fill();
  ctx.strokeStyle = enemy ? '#ff5a4a' : '#222'; ctx.lineWidth = 1.5; ctx.stroke();
  ctx.fillStyle = '#f1d3b0'; ctx.beginPath(); ctx.arc(p.x, p.y - size * 2.3 + bob, size * .6, 0, 7); ctx.fill();
}

// scene: {buildings:[{type,lvl,x,y,w,h,hp,maxhp,dead,build?}], nodes, sim, villagers}
function render(R) {
  ctx.setTransform(1, 0, 0, 1, 0, 0);
  const g = ctx.createLinearGradient(0, 0, 0, cvs.height); g.addColorStop(0, '#20324a'); g.addColorStop(1, '#14202e');
  ctx.fillStyle = g; ctx.fillRect(0, 0, cvs.width, cvs.height);
  ctx.setTransform(DPR * cam.zoom, 0, 0, DPR * cam.zoom, DPR * cam.x, DPR * cam.y);
  const age = ageOf(R.tc || 1);
  // ground
  const A = iso(0, 0), B = iso(MAP, 0), C = iso(MAP, MAP), D = iso(0, MAP);
  poly([A, B, C, D], '#4f7a3a', null);
  poly([{ x: D.x, y: D.y }, { x: C.x, y: C.y }, { x: C.x, y: C.y + 16 }, { x: D.x, y: D.y + 16 }], '#4a3a26');
  ctx.fillStyle = '#5a3f26'; ctx.beginPath(); ctx.moveTo(D.x, D.y); ctx.lineTo(C.x, C.y); ctx.lineTo(C.x, C.y + 18); ctx.lineTo(D.x, D.y + 18); ctx.closePath(); ctx.fill();
  for (let i = 0; i < MAP; i++) for (let j = 0; j < MAP; j++) {
    if ((i + j) % 2) continue;
    poly([iso(i, j), iso(i + 1, j), iso(i + 1, j + 1), iso(i, j + 1)], 'rgba(255,255,255,.035)');
  }
  if (R.showGrid) {
    ctx.strokeStyle = 'rgba(255,255,255,.12)'; ctx.lineWidth = 1; ctx.beginPath();
    for (let i = 0; i <= MAP; i++) { const a = iso(i, 0), b = iso(i, MAP), c = iso(0, i), d = iso(MAP, i); ctx.moveTo(a.x, a.y); ctx.lineTo(b.x, b.y); ctx.moveTo(c.x, c.y); ctx.lineTo(d.x, d.y); }
    ctx.stroke();
  }
  // collect drawables
  const items = [];
  const t = performance.now() / 1000;
  if (R.nodes) for (const n of R.nodes) if (n.amt > 0) items.push({ d: n.x + n.y + 1, f: () => drawNode(n) });
  for (const b of R.buildings) {
    if (b.dead && R.sim) { items.push({ d: b.x + b.y + b.w * 0.5 + b.h * 0.5 - 0.5, f: () => drawRubble(b) }); continue; }
    items.push({ d: b.x + b.y + (b.w + b.h) / 2, f: () => drawBuilding(b, R, age) });
  }
  if (R.villagers) for (const v of R.villagers) items.push({ d: v.x + v.y, f: () => drawUnit(v.x, v.y, v.c, 3.2, v.s, false) });
  if (R.sim) for (const u of R.sim.units) items.push({ d: u.x + u.y, f: () => {
    const d = UDEF[u.type]; drawUnit(u.x, u.y, d.color, 3.6, u.swing, R.sim.mode === 'defence');
    const p = iso(u.x, u.y); if (u.hp < u.maxhp) bar(p.x, p.y - 20, 18, u.hp / u.maxhp, '#6fe06f'); } });
  if (R.ghost) items.push({ d: 999, f: () => drawGhost(R.ghost) });
  items.sort((a, b) => a.d - b.d);
  for (const it of items) it.f();
  if (R.sim) for (const p of R.sim.proj) {
    const k = 1 - p.life / p.max, a = iso(p.x0, p.y0), b = iso(p.x1, p.y1);
    const x = a.x + (b.x - a.x) * k, y = a.y + (b.y - a.y) * k - 14 - Math.sin(k * Math.PI) * 10;
    ctx.strokeStyle = p.c; ctx.lineWidth = 1.6; ctx.beginPath(); ctx.moveTo(x, y); ctx.lineTo(x - (b.x - a.x) * 0.06, y - ((b.y - a.y) * 0.06)); ctx.stroke();
  }
  if (R.deployZone) drawDeployHint(R);
}

function drawNode(n) {
  const p = iso(n.x + 0.5, n.y + 0.5), f = n.amt / n.max;
  if (n.type === 'tree') {
    ctx.fillStyle = '#4a2f18'; ctx.fillRect(p.x - 2, p.y - 8, 4, 8);
    const s = 7 + 6 * f;
    ctx.fillStyle = '#2f6a2f'; ctx.beginPath(); ctx.moveTo(p.x, p.y - 8 - s * 2); ctx.lineTo(p.x - s, p.y - 6); ctx.lineTo(p.x + s, p.y - 6); ctx.closePath(); ctx.fill();
    ctx.fillStyle = '#3b8a3b'; ctx.beginPath(); ctx.moveTo(p.x, p.y - 8 - s * 2); ctx.lineTo(p.x + s, p.y - 6); ctx.lineTo(p.x, p.y - 6); ctx.closePath(); ctx.fill();
  } else {
    const col = n.type === 'gold' ? '#f2c230' : '#9aa0a8', s = 9 + 6 * f;
    poly([{ x: p.x, y: p.y - s * 1.3 }, { x: p.x + s, y: p.y - s * .3 }, { x: p.x, y: p.y + 2 }, { x: p.x - s, y: p.y - s * .3 }], col, 'rgba(0,0,0,.4)');
    poly([{ x: p.x, y: p.y - s * 1.3 }, { x: p.x + s, y: p.y - s * .3 }, { x: p.x, y: p.y - s * .5 }], shade(col, .3));
  }
}
function drawRubble(b) {
  const c = iso(b.x + b.w / 2, b.y + b.h / 2);
  poly([iso(b.x, b.y), iso(b.x + b.w, b.y), iso(b.x + b.w, b.y + b.h), iso(b.x, b.y + b.h)], 'rgba(30,20,10,.55)');
  ctx.fillStyle = 'rgba(255,120,40,.7)'; ctx.beginPath(); ctx.arc(c.x, c.y - 4, 4 + Math.sin(performance.now() / 90) * 1.5, 0, 7); ctx.fill();
}
function drawBuilding(b, R, age) {
  const d = BDEF[b.type];
  const col = mix(d.color, '#a8a8a8', age * 0.18);
  const constructing = b.build && (b.lvl === 0 || !R.sim);
  let hgt = d.hgt * (b.lvl >= 2 ? 1.12 : 1) * (b.lvl >= 3 ? 1.1 : 1);
  if (b.lvl === 0) hgt *= 0.4;
  const top = box(b.x, b.y, b.w, b.h, hgt, col, b.lvl === 0 ? 0.6 : 1);
  if (b.lvl >= 1) glyph(d.icon, top.x, top.y + 2, 18 + b.w * 2);
  if (b.lvl >= 1 && !R.sim) { ctx.font = 'bold 10px sans-serif'; ctx.fillStyle = '#ffe9a8'; ctx.strokeStyle = '#000'; ctx.lineWidth = 3; ctx.strokeText('Lv' + b.lvl, top.x, top.y - 12); ctx.fillText('Lv' + b.lvl, top.x, top.y - 12); }
  if (b.sel) { poly([iso(b.x - .1, b.y - .1), iso(b.x + b.w + .1, b.y - .1), iso(b.x + b.w + .1, b.y + b.h + .1), iso(b.x - .1, b.y + b.h + .1)], null, '#ffe27a'); }
  if (constructing && b.build) { const f = clamp((R.t - b.build.start) / (b.build.end - b.build.start), 0, 1); bar(top.x, top.y - 24, 40, f, '#ffcf4a'); glyph('🔨', top.x + 20, top.y - 14 + Math.sin(performance.now() / 150) * 2, 14); }
  if (R.sim && b.hp < b.maxhp) bar(top.x, top.y - 20, 34, b.hp / b.maxhp, b.dead ? '#a33' : '#e25555');
  if (!R.sim && b.status === 'full') glyph('📦', top.x - 18, top.y - 20, 13);
  if (!R.sim && b.status === 'none') glyph('❓', top.x - 18, top.y - 20, 13);
}
function drawGhost(g) {
  const d = BDEF[g.type];
  poly([iso(g.x, g.y), iso(g.x + d.w, g.y), iso(g.x + d.w, g.y + d.h), iso(g.x, g.y + d.h)], g.ok ? 'rgba(80,255,120,.45)' : 'rgba(255,70,70,.5)', g.ok ? '#6f6' : '#f66');
  box(g.x, g.y, d.w, d.h, d.hgt, g.ok ? '#6fd88a' : '#d86f6f', 0.55);
}
function drawDeployHint(R) {
  ctx.strokeStyle = 'rgba(255,230,120,.35)'; ctx.lineWidth = 2; ctx.setLineDash([8, 6]);
  poly([iso(1, 1), iso(MAP - 1, 1), iso(MAP - 1, MAP - 1), iso(1, MAP - 1)], null, 'rgba(255,230,120,.35)'); ctx.setLineDash([]);
}
