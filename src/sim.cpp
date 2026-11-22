#include "sim.h"
#include <queue>

float distRect(float x, float y, const SimB& b) {
  float dx = std::max({b.x - x, 0.f, x - (b.x + b.w)}), dy = std::max({b.y - y, 0.f, y - (b.y + b.h)});
  return std::sqrt(dx * dx + dy * dy);
}
static bool countable(const SimB& s) { return !s.wall && !s.trap; }

void Sim::deploy(UType type, float x, float y, bool def) {
  const UDef& d = udef(type); float hp = d.hp * mul[def ? 1 : 0].hp;
  units.push_back({nid++, type, def, x, y, hp, hp, 0.f, -1, -1, 0.f});
  if (!def) { started = true; emit(EV_DEPLOY, x, y); }
}
void Sim::spawnGarrisons() {
  for (auto& c : b) {
    if (c.dead || c.type != B_CASTLE) continue;
    int n = 2 + c.lvl;
    for (int i = 0; i < n; i++) {
      float a = i * 6.2831853f / n; float r = c.w / 2.f + 1.1f;
      bool elite = c.lvl >= 6;
      deploy(elite ? ((i % 3 == 2) ? U_PALADIN : U_CHAMPION) : ((i % 3 == 2) ? U_KNIGHT : U_LONGSWORD), c.x + c.w / 2.f + std::cos(a) * r, c.y + c.h / 2.f + std::sin(a) * r, true);
    }
  }
}
int Sim::alive() const { int n = 0; for (auto& s : b) if (!s.dead && countable(s)) n++; return n; }
int Sim::fighters() const { int n = 0; for (auto& u : units) if (!u.def && u.type != U_MONK) n++; return n; }
int Sim::pct() const { int n = 0, tot = 0; for (auto& s : b) if (countable(s)) { tot++; if (s.dead) n++; } return tot ? (100 * n) / tot : 0; }
bool Sim::hqDead() const { for (auto& s : b) if (s.hq && s.dead) return true; return false; }
int Sim::stars() const { int p = pct(); return (p >= 50 ? 1 : 0) + (hqDead() ? 1 : 0) + (p >= 100 ? 1 : 0); }

static float strideOf(UType t) { switch (t) { case U_KNIGHT: case U_PALADIN: return 1.9f; case U_RAM: return 1.3f; case U_MANGONEL: case U_BOMBARD: return 1.2f; case U_TREBUCHET: return 1.4f; default: return 0.95f; } }
static void moveToward(SimUnit& u, float tx, float ty, float speed, float dt) {
  float vx = tx - u.x, vy = ty - u.y, l = std::sqrt(vx * vx + vy * vy); if (l < 1e-5f) return;
  if (u.slow > 0) speed *= 0.5f;
  float s = std::min(speed * dt, l); u.x += vx / l * s; u.y += vy / l * s; u.dx = vx / l; u.dy = vy / l; u.moving = true; u.aph += s / strideOf(u.type);
}

// A* over the 30x30 tile grid. Standing wall/gate tiles are passable but expensive, so raiders walk through gaps
// when the detour is short and otherwise breach the wall in their way.
bool Sim::findPath(const SimUnit& u, const SimB& tgt, float reach, std::vector<int>& out) const {
  const int N = SIMN * SIMN; const float WALLC = 30.f;
  static float g[SIMN * SIMN]; static int parent[SIMN * SIMN]; static char closed[SIMN * SIMN];
  for (int i = 0; i < N; i++) { g[i] = 1e9f; parent[i] = -1; closed[i] = 0; }
  int sx = clampi((int)u.x, 0, SIMN - 1), sy = clampi((int)u.y, 0, SIMN - 1), start = sy * SIMN + sx;
  float tcx = tgt.x + tgt.w / 2.f, tcy = tgt.y + tgt.h / 2.f;
  auto heur = [&](int i) { return std::max(0.f, std::hypot((i % SIMN) + .5f - tcx, (i / SIMN) + .5f - tcy) - tgt.w / 2.f - reach); };
  typedef std::pair<float, int> QI; std::priority_queue<QI, std::vector<QI>, std::greater<QI>> pq;
  g[start] = 0; pq.push({heur(start), start});
  static const int DX[8] = {1, -1, 0, 0, 1, 1, -1, -1}, DY[8] = {0, 0, 1, -1, 1, -1, 1, -1};
  int goal = -1;
  while (!pq.empty()) {
    int cur = pq.top().second; pq.pop(); if (closed[cur]) continue; closed[cur] = 1;
    int cx = cur % SIMN, cy = cur / SIMN;
    if (distRect(cx + .5f, cy + .5f, tgt) <= reach) { goal = cur; break; }
    for (int k = 0; k < 8; k++) {
      int nx = cx + DX[k], ny = cy + DY[k]; if (nx < 0 || ny < 0 || nx >= SIMN || ny >= SIMN) continue;
      int ni = ny * SIMN + nx; if (closed[ni]) continue;
      if (k >= 4 && (wallIdx[cy * SIMN + nx] >= 0 || wallIdx[ny * SIMN + cx] >= 0)) continue;     // no corner cutting through walls
      float cost = (k < 4 ? 1.f : 1.414f) + (wallIdx[ni] >= 0 ? WALLC : 0.f);
      if (g[cur] + cost < g[ni]) { g[ni] = g[cur] + cost; parent[ni] = cur; pq.push({g[ni] + heur(ni), ni}); }
    }
  }
  out.clear(); if (goal < 0) return false;
  for (int i = goal; i >= 0; i = parent[i]) out.push_back(i);
  std::reverse(out.begin(), out.end()); return true;
}

void Sim::step(float dt) {
  if (over) return;
  if (events.size() > 2000) events.clear();
  if (events.size() > 2000) events.clear();
  t += dt;
  // wall grid
  {
    std::fill(wallIdx.begin(), wallIdx.end(), -1); int walls = 0;
    for (int i = 0; i < (int)b.size(); i++) if (b[i].wall && !b[i].dead) { int x = (int)b[i].x, y = (int)b[i].y; if (x >= 0 && y >= 0 && x < SIMN && y < SIMN) { wallIdx[y * SIMN + x] = i; walls++; } }
    if (walls != lastWalls) { gridVer++; lastWalls = walls; }
  }
  while (!sq.empty() && sq.front().t <= t) { Spawn s = sq.front(); sq.erase(sq.begin()); deploy(s.type, s.x, s.y, false); }
  if (started) timeLeft -= dt;
  auto findUnit = [&](int id) -> SimUnit* { for (auto& q : units) if (q.id == id && q.hp > 0) return &q; return nullptr; };

  // one swing at a building (target or wall in the way)
  auto strike = [&](SimUnit& u, const UDef& d, const SideMul& m, SimB& tb) {
    u.cd -= dt; if (u.cd > 0) return;
    u.cd = d.rate; u.swing = u.swingMax = std::min(0.45f, d.rate * 0.8f);
    float dmg = d.dmg * (d.siege ? m.siege : (d.ranged ? m.ranged : m.melee));
    if (tb.wall) { if (u.type == U_RAM) dmg *= (tb.lvl >= 6 ? 1.0f : 2.0f); }
    tb.hp -= dmg; float ex = tb.x + tb.w / 2, ey = tb.y + tb.h / 2; if (tb.hp <= 0) { tb.hp = 0; tb.dead = true; emit(EV_COLLAPSE, ex, ey); }
    emit(d.splash > 0 || d.gun ? EV_CANNON : d.ranged ? EV_ARROW : EV_HIT, ex, ey);
    if (d.splash > 0) {
      float cx = tb.x + tb.w / 2, cy = tb.y + tb.h / 2;
      for (auto& o : b) if (&o != &tb && !o.dead && !o.trap && std::hypot(o.x + o.w / 2 - cx, o.y + o.h / 2 - cy) <= d.splash + o.w / 2.f) { o.hp -= dmg * 0.5f; if (o.hp <= 0) { o.hp = 0; o.dead = true; } }
      for (auto& e : units) if (e.hp > 0 && e.def != u.def && std::hypot(e.x - cx, e.y - cy) <= d.splash) e.hp -= dmg * 0.5f;
    }
    if (d.ranged) proj.push_back({u.x, u.y, tb.x + tb.w / 2, tb.y + tb.h / 2, d.splash > 0 ? 0.55f : 0.25f, d.splash > 0 ? 0.55f : 0.25f, (uint32_t)(d.splash > 0 ? 0xffa040 : 0xffffff)});
  };

  for (auto& u : units) {
    if (u.hp <= 0) continue;
    const UDef& d = udef(u.type); const SideMul& m = mul[u.def ? 1 : 0];
    u.moving = false;
    if (u.swing > 0) u.swing = std::max(0.f, u.swing - dt);
    if (u.slow > 0) u.slow = std::max(0.f, u.slow - dt);

    // ---- monks: heal wounded allies, otherwise stay close to the army
    if (u.type == U_MONK) {
      SimUnit* wounded = nullptr; float bd = 3.4f;
      for (auto& a : units) if (&a != &u && a.hp > 0 && a.def == u.def && a.hp < a.maxhp) { float dd = std::hypot(a.x - u.x, a.y - u.y); if (dd < bd) { bd = dd; wounded = &a; } }
      if (wounded) {
        wounded->hp = std::min(wounded->maxhp, wounded->hp + d.heal * dt); u.hfx -= dt;
        u.swing = 0.45f; u.swingMax = 0.45f;
        if (u.hfx <= 0) { emit(EV_HEAL, wounded->x, wounded->y); u.hfx = 0.45f; proj.push_back({u.x, u.y, wounded->x, wounded->y, 0.35f, 0.35f, 0x7bffa0}); }
        u.dx = wounded->x - u.x; u.dy = wounded->y - u.y;
      } else {
        SimUnit* ally = nullptr; float ad = 1e9f;
        for (auto& a : units) if (&a != &u && a.hp > 0 && a.def == u.def && a.type != U_MONK) { float dd = std::hypot(a.x - u.x, a.y - u.y); if (dd < ad) { ad = dd; ally = &a; } }
        if (ally && ad > 2.2f) moveToward(u, ally->x, ally->y, d.speed, dt);
      }
      continue;
    }

    // ---- choose a unit target (fight whoever is near), else a building
    SimUnit* tu = u.tgtU >= 0 ? findUnit(u.tgtU) : nullptr;
    if (!d.siege) {
      float aggro = u.def ? 9.f : (d.ranged ? d.range + 0.6f : (u.type == U_KNIGHT || u.type == U_PALADIN ? 2.2f : 2.6f));
      if (!tu || std::hypot(tu->x - u.x, tu->y - u.y) > aggro + 2) {
        tu = nullptr; float bd = aggro;
        for (auto& e : units) if (e.hp > 0 && e.def != u.def && e.type != U_MONK) { float dd = std::hypot(e.x - u.x, e.y - u.y); if (dd < bd) { bd = dd; tu = &e; } }
        u.tgtU = tu ? tu->id : -1;
      }
    } else tu = nullptr;

    if (tu) {
      float dist = std::hypot(tu->x - u.x, tu->y - u.y);
      u.dx = tu->x - u.x; u.dy = tu->y - u.y;
      if (dist > d.range) moveToward(u, tu->x, tu->y, d.speed, dt);
      else {
        u.cd -= dt;
        if (u.cd <= 0) {
          u.cd = d.rate; u.swing = u.swingMax = std::min(0.45f, d.rate * 0.8f);
          float dmg = d.dmg * (d.ranged ? m.ranged : m.melee);
          if (d.ranged && !d.gun) dmg *= udef(tu->type).arrowArmor;
          tu->hp -= dmg; emit(d.gun ? EV_CANNON : d.ranged ? EV_ARROW : EV_HIT, tu->x, tu->y);
          if (d.ranged) proj.push_back({u.x, u.y, tu->x, tu->y, 0.25f, 0.25f, 0xffffff});
        }
      }
      continue;
    }
    if (u.def) continue;           // defenders only fight units

    SimB* tb = (u.tgtB >= 0 && !b[u.tgtB].dead) ? &b[u.tgtB] : nullptr;
    if (!tb) {
      int best = -1; float bd = 1e9f;
      if (d.prefDef) for (int i = 0; i < (int)b.size(); i++) if (!b[i].dead && countable(b[i]) && b[i].dps > 0) { float dd = distRect(u.x, u.y, b[i]); if (dd < bd) { bd = dd; best = i; } }
      if (best < 0) { bd = 1e9f; for (int i = 0; i < (int)b.size(); i++) if (!b[i].dead && countable(b[i])) { float dd = distRect(u.x, u.y, b[i]); if (dd < bd) { bd = dd; best = i; } } }
      u.tgtB = best; if (best < 0) continue; tb = &b[best];
    }
    float dist = distRect(u.x, u.y, *tb);
    if (dist <= d.range) {                        // in range: hit the target (ranged units shoot over walls)
      u.dx = tb->x + tb->w / 2 - u.x; u.dy = tb->y + tb->h / 2 - u.y;
      strike(u, d, m, *tb);
      continue;
    }
    // ---- walk (path through the wall grid); breach a wall that is in the way
    float reach = std::max(0.7f, d.range * 0.9f);
    if (u.pver != gridVer || u.ptgt != u.tgtB || u.path.size() < 2 || u.pidx >= (int)u.path.size()) {
      findPath(u, *tb, reach, u.path); u.pver = gridVer; u.ptgt = u.tgtB; u.pidx = 1;
    }
    if (u.path.size() < 2 || u.pidx >= (int)u.path.size()) {          // no path: walk straight
      moveToward(u, clampf(u.x, tb->x, tb->x + tb->w), clampf(u.y, tb->y, tb->y + tb->h), d.speed, dt);
      continue;
    }
    int wi = u.path[u.pidx]; float wx = (wi % SIMN) + .5f, wy = (wi / SIMN) + .5f;
    if (wallIdx[wi] >= 0) {
      SimB& w = b[wallIdx[wi]];
      if (distRect(u.x, u.y, w) <= std::max(d.range, 0.85f)) { u.dx = wx - u.x; u.dy = wy - u.y; strike(u, d, m, w); }
      else moveToward(u, wx, wy, d.speed, dt);
    } else {
      moveToward(u, wx, wy, d.speed, dt);
      if (std::hypot(wx - u.x, wy - u.y) < 0.18f) u.pidx++;
    }
  }

  // ---- defensive buildings shoot at attackers
  for (auto& bd : b) {
    if (bd.dead || bd.trap || bd.wall || bd.dps <= 0) continue;
    bd.cd -= dt; if (bd.cd > 0) continue;
    float cx = bd.x + bd.w / 2, cy = bd.y + bd.h / 2; int best = -1; float bdist = bd.range;
    for (int i = 0; i < (int)units.size(); i++) if (!units[i].def && units[i].hp > 0) { float dd = distRect(units[i].x, units[i].y, bd); if (dd <= bdist) { bdist = dd; best = i; } }      // range is measured from the building's edge, like the raiders' own ranges
    if (best >= 0 && bd.type == B_SCORPION) {
      // piercing bolt: hits every raider within 0.4 tiles of the line from the scorpion through the target (to +2 tiles beyond)
      float tx = units[best].x - cx, ty = units[best].y - cy, tl = std::max(0.01f, std::hypot(tx, ty)); float ux = tx / tl, uy = ty / tl, reachL = std::min(bd.range + 1.5f, tl + 2.0f);
      int hits = 0;
      for (auto& e : units) if (!e.def && e.hp > 0) {
        float px = e.x - cx, py = e.y - cy, along = px * ux + py * uy, side = std::fabs(px * uy - py * ux);
        if (along >= 0 && along <= reachL && side <= 0.4f) { e.hp -= bd.dps * udef(e.type).arrowArmor * mul[1].ranged; hits++; }
      }
      bd.cd = 2.2f; emit(EV_BOLT, cx, cy);
      proj.push_back({cx, cy, cx + ux * reachL, cy + uy * reachL, 0.3f, 0.3f, 0xffe9b0});
      continue;
    }
    if (best >= 0) {
      emit(bd.type == B_BOMBARD ? EV_CANNON : EV_ARROW, cx, cy);
      bool cannon = bd.type == B_BOMBARD;
      float dmg = bd.dps * (cannon ? 1.f : udef(units[best].type).arrowArmor) * mul[1].ranged;
      units[best].hp -= dmg; bd.cd = cannon ? 1.6f : 1;
      if (cannon) for (int j = 0; j < (int)units.size(); j++) if (j != best && !units[j].def && std::hypot(units[j].x - units[best].x, units[j].y - units[best].y) <= 1.3f) units[j].hp -= dmg * 0.5f;
      proj.push_back({cx, cy, units[best].x, units[best].y, 0.2f, 0.2f, 0xffd9a0});
    }
  }
  // ---- traps: spikes fire once on contact, boiling oil scalds + slows for a few seconds
  for (auto& tr : b) {
    if (!tr.trap || tr.dead) continue;
    float cx = tr.x + .5f, cy = tr.y + .5f;
    if (tr.aux > 0) {
      tr.aux -= dt;
      for (auto& e : units) if (!e.def && e.hp > 0 && std::hypot(e.x - cx, e.y - cy) <= tr.range) { e.hp -= tr.dps * dt; e.slow = 0.35f; }
      if (tr.aux <= 0) tr.dead = true;
      continue;
    }
    bool hit = false; for (auto& e : units) if (!e.def && e.hp > 0 && std::hypot(e.x - cx, e.y - cy) <= 0.8f) { hit = true; break; }
    if (!hit) continue;
    if (tr.type == B_SPIKE) {
      for (auto& e : units) if (!e.def && e.hp > 0 && std::hypot(e.x - cx, e.y - cy) <= tr.range) e.hp -= tr.dps;
      proj.push_back({cx, cy, cx, cy, 0.4f, 0.4f, 0xff7a5a}); tr.dead = true; emit(EV_TRAP, cx, cy);
    } else { tr.aux = 3.0f; emit(EV_TRAP, cx, cy); }
  }
  for (auto& u : units) { if (u.lastHp < 0) u.lastHp = u.hp; if (u.hp < u.lastHp - 0.01f) u.hitT = 0.16f; u.lastHp = u.hp; if (u.hitT > 0) u.hitT = std::max(0.f, u.hitT - dt); }
  for (auto& u : units) if (u.hp <= 0) { corpses.push_back({u.x, u.y, u.dx, u.dy, 0.f, u.type, u.def}); emit(EV_DEATH, u.x, u.y); }
  for (auto& c : corpses) c.age += dt;
  corpses.erase(std::remove_if(corpses.begin(), corpses.end(), [](const Corpse& c) { return c.age > 1.1f; }), corpses.end());
  for (auto& bb : b) if (bb.dead) bb.deadT += dt;
  units.erase(std::remove_if(units.begin(), units.end(), [](const SimUnit& u) { return u.hp <= 0; }), units.end());
  for (auto& p : proj) p.life -= dt;
  proj.erase(std::remove_if(proj.begin(), proj.end(), [](const Proj& p) { return p.life <= 0; }), proj.end());
  int res = reserve ? reserve() : 0;
  if (alive() == 0 || timeLeft <= 0 || (started && fighters() == 0 && sq.empty() && res <= 0)) over = true;
}
