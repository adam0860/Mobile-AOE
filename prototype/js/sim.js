// ===== Deterministic-ish battle simulation (shared by player raids, NPC raids and offline raids) =====
const distRect = (x, y, b) => Math.hypot(Math.max(b.x - x, 0, x - (b.x + b.w)), Math.max(b.y - y, 0, y - (b.y + b.h)));

class Sim {
  constructor(buildings, opts = {}) {
    this.b = buildings; this.units = []; this.proj = []; this.sq = [];
    this.t = 0; this.timeLeft = 180; this.started = !!opts.started; this.over = false;
    this.reserve = opts.reserve || (() => 0); this.nid = 1; this.mode = opts.mode || 'attack';
  }
  deploy(type, x, y) {
    const d = UDEF[type];
    this.units.push({ id: this.nid++, type, x, y, hp: d.hp, maxhp: d.hp, cd: 0, tgt: null });
    this.started = true;
  }
  alive() { return this.b.filter(b => !b.dead); }
  pct() { return Math.floor((100 * this.b.filter(b => b.dead).length) / this.b.length); }
  hqDead() { return this.b.some(b => b.hq && b.dead); }
  stars() { const p = this.pct(); return (p >= 50 ? 1 : 0) + (this.hqDead() ? 1 : 0) + (p >= 100 ? 1 : 0); }
  step(dt) {
    if (this.over) return;
    this.t += dt;
    while (this.sq.length && this.sq[0].t <= this.t) { const s = this.sq.shift(); this.deploy(s.type, s.x, s.y); }
    if (this.started) this.timeLeft -= dt;
    const alive = this.alive();
    for (const u of this.units) {
      const d = UDEF[u.type];
      if (!u.tgt || u.tgt.dead) {
        let best = null, bd = 1e9;
        for (const b of alive) { if (b.dead) continue; const dd = distRect(u.x, u.y, b); if (dd < bd) { bd = dd; best = b; } }
        u.tgt = best;
      }
      const b = u.tgt; if (!b) continue;
      const dist = distRect(u.x, u.y, b);
      if (dist > d.range) {
        const px = clamp(u.x, b.x, b.x + b.w), py = clamp(u.y, b.y, b.y + b.h);
        const vx = px - u.x, vy = py - u.y, l = Math.hypot(vx, vy) || 1, s = Math.min(d.speed * dt, l);
        u.x += (vx / l) * s; u.y += (vy / l) * s; u.walk = (u.walk || 0) + dt;
      } else {
        u.cd -= dt;
        if (u.cd <= 0) {
          u.cd = d.rate; b.hp -= d.dmg; u.swing = 0.2;
          if (b.hp <= 0) { b.hp = 0; b.dead = true; }
          if (d.ranged) this.proj.push({ x0: u.x, y0: u.y, x1: b.x + b.w / 2, y1: b.y + b.h / 2, life: 0.25, max: 0.25, c: '#fff' });
        }
      }
      if (u.swing) u.swing = Math.max(0, u.swing - dt);
    }
    for (const b of this.b) {
      if (b.dead || !b.dps) continue;
      b.cd -= dt;
      if (b.cd > 0) continue;
      const cx = b.x + b.w / 2, cy = b.y + b.h / 2;
      let best = null, bd = b.range;
      for (const u of this.units) { const dd = Math.hypot(u.x - cx, u.y - cy); if (dd <= bd) { bd = dd; best = u; } }
      if (best) {
        best.hp -= b.dps; b.cd = 1;
        this.proj.push({ x0: cx, y0: cy, x1: best.x, y1: best.y, life: 0.2, max: 0.2, c: '#ffd9a0' });
      }
    }
    this.units = this.units.filter(u => u.hp > 0);
    this.proj = this.proj.filter(p => (p.life -= dt) > 0);
    if (!this.alive().length || this.timeLeft <= 0 || (this.started && !this.units.length && !this.sq.length && this.reserve() <= 0)) this.over = true;
  }
}
