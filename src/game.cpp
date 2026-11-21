#include "game.h"
#include <SDL.h>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>

State S;
static std::vector<Lord> LORDS;
static bool lordsBuilt = false;

void newGame() {
  S = State(); LORDS.clear(); lordsBuilt = false;
  Rng r((uint32_t)std::time(nullptr)); S.seed = (uint32_t)(r.next() * 1e9);
  S.lastSeen = (long long)std::time(nullptr);
  addB(B_TOWNHALL, 13, 13, 1); addB(B_HOUSE, 18, 13, 1); addB(B_TOWER, 13, 18, 1);
  Rng rr(S.seed);
  for (int i = 3; i < 10; i++) for (int j = 3; j < 10; j++) if (rr.next() < 0.62) S.nodes.push_back({i, j, N_TREE, 120, 120, 0});
  int g[5][2] = {{22, 6}, {23, 6}, {22, 7}, {24, 7}, {23, 8}}, s[5][2] = {{6, 22}, {7, 22}, {6, 23}, {8, 23}, {7, 24}};
  for (auto& p : g) S.nodes.push_back({p[0], p[1], N_GOLD, 500, 500, 0});
  for (auto& p : s) S.nodes.push_back({p[0], p[1], N_STONE, 400, 400, 0});
}

Bld& addB(BType t, int x, int y, int lvl) {
  const BDef& d = bdef(t);
  Bld b{}; b.id = S.nid++; b.type = t; b.x = x; b.y = y; b.w = d.w; b.h = d.h; b.lvl = lvl; b.workers = 0; b.building = false;
  S.b.push_back(b); return S.b.back();
}
Bld* findB(int id) { for (auto& b : S.b) if (b.id == id) return &b; return nullptr; }
Bld& tcB() { for (auto& b : S.b) if (b.type == B_TOWNHALL) return b; return S.b.front(); }
int tcLvl() { return tcB().lvl; }
int cap() { return capFor(tcLvl()); }
int prot() { return protFor(tcLvl()); }
bool isDone(const Bld& b) { return b.lvl >= 1; }
int builders() { int n = 0; for (auto& b : S.b) if (b.building) n++; return n; }
int assigned() { int n = 0; for (auto& b : S.b) n += b.workers; return n; }
int idleV() { return S.villagers - builders() - assigned(); }
bool takeVillager() {
  if (idleV() >= 1) return true;
  Bld* best = nullptr; for (auto& b : S.b) if (b.workers > 0 && (!best || b.workers > best->workers)) best = &b;
  if (!best) return false;
  best->workers--; toast("A worker left their job to build"); return true;
}
int vCap() { int n = 3; for (auto& b : S.b) if (b.type == B_HOUSE && isDone(b)) n += 5; return n; }
int armyCount() { int n = 0; for (int i = 0; i < U_COUNT; i++) n += S.army[i]; return n; }
int queuedPop() { int n = 0; for (auto& b : S.b) n += (int)b.q.size(); return n; }
int housing() { int n = 5; for (auto& b : S.b) if (b.type == B_ARMYCAMP && isDone(b)) n += 20; return n; }
bool canAfford(const Cost& c) { for (int i = 0; i < NRES; i++) if (S.res[i] < c.v[i]) return false; return true; }
void pay(const Cost& c) { for (int i = 0; i < NRES; i++) S.res[i] -= (float)c.v[i]; }

UpInfo upgradeInfo(const Bld& b) {
  UpInfo u; const BDef& d = bdef(b.type); int to = b.lvl + 1;
  if (b.building) { u.err = "Busy"; return u; }
  if (to > MAXLVL) { u.err = "Max level"; return u; }
  if (b.type == B_TOWNHALL) { const TcUp& t = tcup(to); u.cost = t.cost; u.time = t.time; u.label = t.label; return u; }
  static const char* need[11] = {"", "", "Needs Town Center Lv2", "Needs Town Center Lv3", "Needs Town Center Lv4", "Needs Town Center Lv5", "Needs Town Center Lv6", "Needs Town Center Lv7", "Needs Town Center Lv8", "Needs Town Center Lv9", "Needs Town Center Lv10"};
  if (to > tcLvl()) { u.err = need[clampi(to, 0, 10)]; return u; }
  double k = std::pow(1.6, b.lvl);
  for (int i = 0; i < NRES; i++) u.cost.v[i] = roundi(d.cost.v[i] * k);
  u.time = roundi(d.time * std::pow(1.9, b.lvl)); u.label = "Upgrade to Level " + std::to_string(to); return u;
}
void startUpgrade(Bld& b) {
  UpInfo u = upgradeInfo(b); if (u.err) { toast(u.err); return; }
  if (idleV() < 1 && assigned() < 1) { toast("No villager available to build!"); return; }
  if (!canAfford(u.cost)) { toast("Not enough resources"); return; }
  if (!takeVillager()) return;
  pay(u.cost); b.building = true; b.bStart = (float)S.t; b.bEnd = (float)S.t + u.time; b.bTo = b.lvl + 1; toast("Construction started");
}
static void completeBuild(Bld& b) {
  bool first = b.lvl == 0; b.lvl = b.bTo; b.building = false; const BDef& d = bdef(b.type);
  if (d.res >= 0 && first && idleV() > 0 && b.workers == 0) b.workers = 1;
  if (b.type == B_TOWNHALL && b.lvl == 2 && S.nextRaid <= 0) S.nextRaid = S.t + 60;
  sfx(b.type == B_TOWNHALL ? 13 : 2);
  if (b.type == B_TOWNHALL) banner(b.lvl == 3 ? "Feudal Age reached!" : b.lvl == 5 ? "Castle Age reached!" : b.lvl == 8 ? "Imperial Age reached!" : "Town Center Level " + std::to_string(b.lvl));
  else toast(std::string(d.name) + " ready");
}

void econTick(float dt) {
  double t = S.t;
  for (auto& b : S.b) if (b.building && t >= b.bEnd) completeBuild(b);
  for (auto& n : S.nodes) if (n.amt <= 0 && t >= n.respawn) n.amt = n.max;
  if (S.researchTech >= 0 && t >= S.researchEnd) { S.tech[S.researchTech]++; sfx(2); toast(std::string(techdef(S.researchTech).name) + " researched"); S.researchTech = -1; }
  int over = idleV();
  if (over < 0) { int k = -over; for (auto& b : S.b) { int m = std::min(b.workers, k); b.workers -= m; k -= m; if (!k) break; } }
  for (auto& b : S.b) {
    const BDef& d = bdef(b.type);
    if (d.res < 0 || !isDone(b)) { b.status = 0; continue; }
    if (!b.workers) { b.status = 1; continue; }
    if (S.res[d.res] >= cap()) { b.status = 3; continue; }
    float amount = b.workers * d.rate * (1.f + 0.15f * (b.lvl - 1)) * dt / 60.f;
    if (d.node >= 0) {
      float cx = b.x + b.w / 2.f, cy = b.y + b.h / 2.f;
      std::vector<Node*> ns;
      for (auto& n : S.nodes) if (n.type == d.node && n.amt > 0 && std::hypot(n.x + .5f - cx, n.y + .5f - cy) <= 7) ns.push_back(&n);
      if (ns.empty()) { b.status = 4; continue; }
      std::sort(ns.begin(), ns.end(), [&](Node* a, Node* c) { return std::hypot(a->x - cx, a->y - cy) < std::hypot(c->x - cx, c->y - cy); });
      float got = 0;
      for (Node* n : ns) { float take = std::min(n->amt, amount - got); n->amt -= take; got += take; if (n->amt <= 0) n->respawn = (float)t + 300; if (got >= amount - 1e-6f) break; }
      amount = got;
    }
    b.status = 2; b.acc += amount;
    int whole = (int)std::floor(b.acc);
    if (whole) { b.acc -= whole; S.res[d.res] = std::min((float)cap(), S.res[d.res] + whole); }
  }
  if (!S.vq.empty()) { S.vq[0] -= dt; if (S.vq[0] <= 0 && S.villagers < vCap()) { S.villagers++; S.vq.erase(S.vq.begin()); } else if (S.vq[0] < 0) S.vq[0] = 0; }
  for (auto& b : S.b) if (!b.q.empty()) { b.q[0].left -= dt; if (b.q[0].left <= 0) { S.army[b.q[0].type]++; b.q.erase(b.q.begin()); } }
}

void trainVillager() {
  if (S.villagers + (int)S.vq.size() >= vCap()) { toast("Build more Houses for villagers"); return; }
  if (!canAfford(mkc(0, 25))) { toast("Need 25 food"); return; }
  pay(mkc(0, 25)); S.vq.push_back(10);
}
void trainUnit(Bld& b, UType t) {
  const UDef& d = udef(t);
  if (armyCount() + queuedPop() + d.pop > housing()) { toast("Army housing full - build an Army Camp"); return; }
  if (b.q.size() >= 8) { toast("Queue full"); return; }
  if (!canAfford(d.cost)) { toast("Not enough resources"); return; }
  pay(d.cost); b.q.push_back({t, (float)d.time});
}
int buildCount(BType t) { int n = 0; for (auto& b : S.b) if (b.type == t) n++; return n; }
const char* canBuildErr(BType t) {
  const BDef& d = bdef(t);
  static const char* need[11] = {"", "", "Needs Town Center Lv2", "Needs Town Center Lv3", "Needs Town Center Lv4", "Needs Town Center Lv5", "Needs Town Center Lv6", "Needs Town Center Lv7", "Needs Town Center Lv8", "Needs Town Center Lv9", "Needs Town Center Lv10"};
  if (d.unlock > tcLvl()) return need[clampi(d.unlock, 0, 10)];
  int mx = bmaxn(t, tcLvl());
  if (buildCount(t) >= mx) return mx == 0 ? "Locked" : "Limit reached";
  return nullptr;
}
bool placeOk(BType t, int x, int y) {
  const BDef& d = bdef(t);
  if (x < 1 || y < 1 || x + d.w > MAP - 1 || y + d.h > MAP - 1) return false;
  for (auto& b : S.b) if (x < b.x + b.w && x + d.w > b.x && y < b.y + b.h && y + d.h > b.y) return false;
  for (auto& n : S.nodes) if (n.amt > 0 && n.x >= x && n.x < x + d.w && n.y >= y && n.y < y + d.h) return false;
  return true;
}
void setWorkers(Bld& b, int d) {
  const BDef& def = bdef(b.type); int slots = bslots(b.type, b.lvl);
  if (d > 0 && (idleV() < 1 || b.workers >= slots)) { toast(idleV() < 1 ? "No idle villagers" : "Camp is full"); return; }
  if (d < 0 && b.workers < 1) return;
  b.workers += d;
}

// ---- Castle Age: research, market, multipliers ----
Cost techCost(int tech) { Cost c = techdef(tech).base; int l = S.tech[tech] + 1; for (int i = 0; i < NRES; i++) c.v[i] *= l; return c; }
int techTime(int tech) { return techdef(tech).time * (S.tech[tech] + 1); }
const char* techBlock(int tech) {
  const TechDef& d = techdef(tech); int next = S.tech[tech] + 1;
  if (S.tech[tech] >= d.maxLvl) return "Fully researched";
  if (S.researchTech >= 0) return "Another upgrade is in progress";
  int bl = 0; for (auto& b : S.b) if (b.type == d.bld && isDone(b)) bl = std::max(bl, b.lvl);
  if (bl == 0) return "Building missing";
  if (bl < next) return bl == 1 ? "Needs building Lv2" : bl == 2 ? "Needs building Lv3" : "Needs a higher level building";
  return nullptr;
}
void startResearch(int tech) {
  const char* e = techBlock(tech); if (e) { toast(e); return; }
  Cost c = techCost(tech); if (!canAfford(c)) { toast("Not enough resources"); return; }
  pay(c); S.researchTech = tech; S.researchEnd = S.t + techTime(tech); toast("Research started"); sfx(1);
}
// market: 0 wood->gold, 1 food->gold, 2 gold->stone, 3 gold->wood
Cost tradeGive(int k) { return k == 0 ? mkc(500) : k == 1 ? mkc(0, 500) : mkc(0, 0, 200); }
Cost tradeGet(int k) { return k == 0 ? mkc(0, 0, 200) : k == 1 ? mkc(0, 0, 200) : k == 2 ? mkc(0, 0, 0, 300) : mkc(400); }
const char* tradeLabel(int k) { static const char* n[4] = {"Sell wood for gold", "Sell food for gold", "Buy stone with gold", "Buy wood with gold"}; return n[clampi(k, 0, 3)]; }
void trade(int k) {
  Cost give = tradeGive(k), get = tradeGet(k);
  if (!canAfford(give)) { toast("Not enough resources"); return; }
  pay(give); sfx(3); for (int i = 0; i < NRES; i++) S.res[i] = std::min((float)cap(), S.res[i] + get.v[i]);
}
SideMul techMul() {
  SideMul m; m.melee = 1 + 0.10f * S.tech[T_FORGING]; m.ranged = 1 + 0.10f * S.tech[T_FLETCH];
  m.siege = 1 + 0.15f * S.tech[T_SIEGE] + 0.08f * S.tech[T_CHEMISTRY]; m.ranged += 0.08f * S.tech[T_CHEMISTRY]; m.hp = 1 + 0.10f * S.tech[T_ARMOR]; return m;
}
SideMul enemyMul(int tc) { SideMul m; float s = 1 + 0.06f * (std::max(1, tc) - 1); m.melee = m.ranged = m.siege = m.hp = s; return m; }

// ---- walls, gates and traps ----
static Bld* pieceAt(int x, int y) { for (auto& b : S.b) if (isPiece(b.type) && b.x == x && b.y == y) return &b; return nullptr; }
const char* placePieceBlock(BType t, int x, int y) {
  const BDef& d = bdef(t);
  if (d.unlock > tcLvl()) return "Locked: raise your Town Center level";
  if (x < 1 || y < 1 || x >= MAP - 1 || y >= MAP - 1) return "Outside the village";
  Bld* ex = pieceAt(x, y);
  if (t == B_GATE && ex && ex->type == B_WALL) { if (buildCount(B_GATE) >= bmaxn(B_GATE, tcLvl())) return "Gate limit reached"; return nullptr; }
  if (buildCount(t) >= bmaxn(t, tcLvl())) return "Limit reached";
  if (!placeOk(t, x, y)) return "Tile is occupied";
  return nullptr;
}
bool placePiece(BType t, int x, int y) {
  const char* e = placePieceBlock(t, x, y); if (e) { toast(e); return false; }
  const BDef& d = bdef(t);
  if (!canAfford(d.cost)) { toast("Not enough resources"); return false; }
  pay(d.cost); sfx(1);
  Bld* ex = pieceAt(x, y);
  if (t == B_GATE && ex && ex->type == B_WALL) { ex->type = B_GATE; return true; }
  Bld& b = addB(t, x, y, 1); b.workers = 0; return true;
}
void removePiece(int id) {
  for (auto it = S.b.begin(); it != S.b.end(); ++it) if (it->id == id && isPiece(it->type)) {
    const BDef& d = bdef(it->type); for (int i = 0; i < NRES; i++) S.res[i] = std::min((float)cap(), S.res[i] + d.cost.v[i] / 2);
    S.b.erase(it); return;
  }
}
int pieceCount(bool traps) { int n = 0; for (auto& b : S.b) { if (!traps && b.type == B_WALL) n++; else if (!traps && b.type == B_GATE) n += 5; else if (traps && (b.type == B_SPIKE || b.type == B_OIL)) n++; } return n; }
static Cost perWallCost(int toLvl) { Cost c; c.v[WOOD] = roundi(12 * std::pow(1.5, toLvl - 2)); c.v[STONE] = toLvl >= 3 ? roundi(8 * std::pow(1.5, toLvl - 3)) : 0; return c; }
static Cost perTrapCost(int toLvl) { Cost c; c.v[GOLD] = roundi(50 * std::pow(1.5, toLvl - 1)); c.v[STONE] = roundi(30 * std::pow(1.5, toLvl - 1)); return c; }
Cost wallUpgradeCost() { Cost p = perWallCost(S.wallLvl + 1); int n = pieceCount(false); Cost c; for (int i = 0; i < NRES; i++) c.v[i] = p.v[i] * n; return c; }
Cost trapUpgradeCost() { Cost p = perTrapCost(S.trapLvl + 1); int n = pieceCount(true); Cost c; for (int i = 0; i < NRES; i++) c.v[i] = p.v[i] * n; return c; }
const char* wallUpgradeBlock() {
  if (S.wallLvl >= MAXLVL) return "Max level";
  if (S.wallLvl + 1 > tcLvl()) return "Needs a higher Town Center";
  if (pieceCount(false) == 0) return "No walls yet";
  return nullptr;
}
const char* trapUpgradeBlock() {
  if (S.trapLvl >= MAXLVL) return "Max level";
  if (S.trapLvl + 1 > tcLvl()) return "Needs a higher Town Center";
  if (pieceCount(true) == 0) return "No traps yet";
  return nullptr;
}
void upgradeWalls() { if (const char* e = wallUpgradeBlock()) { toast(e); return; } Cost c = wallUpgradeCost(); if (!canAfford(c)) { toast("Not enough resources"); return; } pay(c); S.wallLvl++; toast("Walls upgraded to level " + std::to_string(S.wallLvl)); }
void upgradeTraps() { if (const char* e = trapUpgradeBlock()) { toast(e); return; } Cost c = trapUpgradeCost(); if (!canAfford(c)) { toast("Not enough resources"); return; } pay(c); S.trapLvl++; toast("Traps upgraded to level " + std::to_string(S.trapLvl)); }

// ---- lords / ladder / raids ----
const std::vector<Lord>& lords() {
  if (lordsBuilt) return LORDS;
  Rng r(S.seed ^ 0x9e37u); LORDS.clear();
  for (int i = 0; i < 300; i++) {
    Lord l; l.name = std::string(LORD_FIRST[(int)(r.next() * 30)]) + " " + LORD_TITLE[(int)(r.next() * 10)];
    l.r0 = (int)(std::pow(r.next(), 1.8) * 7000); l.ph = r.next() * 6.28;
    for (int s = 1; s < S.season; s++) if (l.r0 > 3000) l.r0 = 3000 + (l.r0 - 3000) / 2;       // soft reset each season
    LORDS.push_back(l);
  }
  lordsBuilt = true; return LORDS;
}
int lordRenown(const Lord& l) { return std::max(0, roundi(l.r0 + 70 * std::sin(S.t / 600 + l.ph))); }

Sim makeDefenceSim() {
  Sim sim; sim.defence = true; sim.started = true;
  sim.mul[0] = enemyMul(tcLvl()); sim.mul[1] = techMul();
  float masonry = 1 + 0.12f * S.tech[T_MASONRY];
  for (auto& b : S.b) if (isDone(b)) {
    int lv = (b.type == B_WALL || b.type == B_GATE) ? S.wallLvl : (b.type == B_SPIKE || b.type == B_OIL) ? S.trapLvl : b.lvl;
    SimB sb = makeSimB(b.type, lv, b.x, b.y); sb.hp *= masonry; sb.maxhp *= masonry; sim.b.push_back(sb);
  }
  Rng r((uint32_t)S.t + S.seed);
  auto edge = [&](float& x, float& y) {
    int s = (int)(r.next() * 4); float p = 2 + (float)r.next() * (MAP - 4);
    if (s == 0) { x = p; y = 0.6f; } else if (s == 1) { x = p; y = MAP - 0.6f; } else if (s == 2) { x = 0.6f; y = p; } else { x = MAP - 0.6f; y = p; }
  };
  float ex, ey; edge(ex, ey);
  int tc = tcLvl(), n = 3 + tc * 2 + S.renown / 250;
  for (int i = 0; i < n; i++) {
    UType ty = U_MILITIA;
    if (tc >= 8) ty = (i % 8 == 5) ? U_TREBUCHET : (tc >= 9 && i % 9 == 4) ? U_BOMBARD : (i % 5 == 1) ? U_PALADIN : (i % 3 == 0) ? ((i % 2) ? U_CANNONEER : U_ARBALEST) : (i % 2 == 0) ? U_CHAMPION : U_LONGSWORD;
    else if (tc >= 5) ty = (i % 7 == 3) ? U_RAM : (i % 5 == 1) ? U_KNIGHT : (i % 3 == 0) ? U_CROSSBOW : (i % 2 == 0) ? U_LONGSWORD : U_MANATARMS;
    else if (tc >= 3) ty = (i % 3 == 0) ? U_ARCHER : (i % 4 == 1) ? U_MANATARMS : U_MILITIA;
    float x, y;
    if (r.next() < 0.7) { x = ex + (float)(r.next() - .5) * 3; y = ey + (float)(r.next() - .5) * 3; } else edge(x, y);
    sim.sq.push_back({i * 0.5f, ty, clampf(x, .5f, MAP - .5f), clampf(y, .5f, MAP - .5f)});
  }
  sim.spawnGarrisons();
  return sim;
}
Report applyDefence(const Sim& sim) {
  Report rp; rp.stars = sim.stars(); rp.pct = sim.pct();
  double u1 = (double)(std::rand() % 1000) / 1000.0;
  int opp = std::max(0, S.renown + roundi((u1 - 0.5) * 200)); double E = expectedScore(S.renown, opp);
  rp.who = lords()[std::rand() % 300].name;
  for (int k = 0; k < NRES; k++) { float lootable = std::max(0.f, S.res[k] - prot()); int v = (int)std::floor(lootable * 0.2f * rp.pct / 100); rp.stolen.v[k] = v; S.res[k] -= v; }
  rp.held = rp.stars <= 1; if (rp.held) S.sDef++;
  int dr = rp.held ? roundi(0.5 * 40 * (1 - E)) : -roundi(0.5 * 40 * E);
  int before = S.renown; S.renown = std::max(0, S.renown + dr); rp.dr = S.renown - before;
  static const int sh[4] = {0, 60, 120, 180};
  S.shieldUntil = S.t + sh[rp.stars]; S.nextRaid = S.t + 200 + std::rand() % 120;
  return rp;
}
Report headlessRaid() { Sim sim = makeDefenceSim(); int n = 0; while (!sim.over && n++ < 20000) sim.step(0.05f); return applyDefence(sim); }

// ---- seasons (7 days of game time): soft reset toward 3000 Renown + a rank chest ----
double seasonLen() { return 7 * 24 * 3600.0; }
double seasonLeft() { return std::max(0.0, S.seasonStart + seasonLen() - S.t); }
int ladderPosition() { int pos = 1; for (auto& l : lords()) if (lordRenown(l) > S.renown) pos++; return pos; }
bool seasonCheck(SeasonResult& r) {
  if (S.t < S.seasonStart + seasonLen()) return false;
  r.season = S.season; r.finalRenown = S.renown; r.pos = ladderPosition(); r.rankIdx = rankIdx(S.renown); r.wins = S.sWins; r.defs = S.sDef;
  float f = 0.04f * (r.rankIdx + 1);
  for (int i = 0; i < NRES; i++) r.reward.v[i] = (int)std::min((float)cap(), cap() * f);
  if (r.pos <= 10) r.reward.v[GOLD] = (int)std::min((float)cap(), r.reward.v[GOLD] + cap() * 0.10f);
  if (r.pos == 1) r.reward.v[GOLD] = (int)std::min((float)cap(), r.reward.v[GOLD] + cap() * 0.10f);
  for (int i = 0; i < NRES; i++) S.res[i] = std::min((float)cap(), S.res[i] + r.reward.v[i]);
  S.renown = S.renown > 3000 ? 3000 + (S.renown - 3000) / 2 : S.renown; r.newRenown = S.renown;
  S.season++; S.seasonStart += seasonLen(); S.sWins = S.sDef = 0; S.shieldUntil = 0;
  lordsBuilt = false; LORDS.clear();
  return true;
}

// ---- persistence (plain text, versioned) ----
std::string g_savePath; // override used by the --smoke self-test so it never touches the real save
static std::string savePath() {
  if (!g_savePath.empty()) return g_savePath;
  char* p = SDL_GetPrefPath("DawnOfKings", "DawnOfKings"); std::string s = p ? std::string(p) + "save.txt" : "save.txt"; if (p) SDL_free(p); return s;
}
void saveGame() {
  S.lastSeen = (long long)std::time(nullptr);
  std::ostringstream o; o.precision(9);
  o << "DOK5 " << S.t << ' ' << S.villagers << ' ' << S.renown << ' ' << S.shieldUntil << ' ' << S.nextRaid << ' ' << S.nid << ' ' << S.seed << ' ' << S.rivalSeq << ' ' << (int)S.seenIntro << ' ' << S.lastSeen << '\n';
  for (int i = 0; i < NRES; i++) o << S.res[i] << ' '; o << '\n';
  o << (int)U_COUNT << ' '; for (int i = 0; i < U_COUNT; i++) o << S.army[i] << ' '; o << '\n';
  for (int i = 0; i < 6; i++) o << S.camp[i] << ' '; o << '\n';
  o << (int)T_COUNT << ' '; for (int i = 0; i < T_COUNT; i++) o << S.tech[i] << ' '; o << S.researchTech << ' ' << S.researchEnd << '\n';
  o << S.wallLvl << ' ' << S.trapLvl << '\n';
  o << S.season << ' ' << S.seasonStart << ' ' << S.sWins << ' ' << S.sDef << ' ' << (int)S.musicOn << ' ' << (int)S.sfxOn << '\n';
  o << S.vq.size() << ' '; for (float v : S.vq) o << v << ' '; o << '\n';
  o << S.b.size() << '\n';
  for (auto& b : S.b) { o << b.id << ' ' << b.type << ' ' << b.x << ' ' << b.y << ' ' << b.lvl << ' ' << b.workers << ' ' << (int)b.building << ' ' << b.bStart << ' ' << b.bEnd << ' ' << b.bTo << ' ' << b.acc << ' ' << b.q.size(); for (auto& q : b.q) o << ' ' << q.type << ' ' << q.left; o << '\n'; }
  o << S.nodes.size() << '\n';
  for (auto& n : S.nodes) o << n.x << ' ' << n.y << ' ' << n.type << ' ' << n.amt << ' ' << n.max << ' ' << n.respawn << '\n';
  std::ofstream f(savePath(), std::ios::binary | std::ios::trunc); f << o.str();
}
bool loadGame() {
  std::ifstream f(savePath(), std::ios::binary); if (!f) return false;
  std::string magic; f >> magic; if (magic != "DOK1" && magic != "DOK2" && magic != "DOK3" && magic != "DOK4" && magic != "DOK5") return false;
  int armyN = magic == "DOK1" ? 3 : 9;
  State n; int seen = 0;
  f >> n.t >> n.villagers >> n.renown >> n.shieldUntil >> n.nextRaid >> n.nid >> n.seed >> n.rivalSeq >> seen >> n.lastSeen; n.seenIntro = seen;
  for (int i = 0; i < NRES; i++) f >> n.res[i];
  if (magic == "DOK3" || magic == "DOK4" || magic == "DOK5") f >> armyN;
  for (int i = 0; i < armyN; i++) { int v; f >> v; if (i < U_COUNT) n.army[i] = v; }
  for (int i = 0; i < 6; i++) f >> n.camp[i];
  if (magic != "DOK1") { int techN = 5; if (magic == "DOK3" || magic == "DOK4" || magic == "DOK5") f >> techN; for (int i = 0; i < techN; i++) { int v; f >> v; if (i < T_COUNT) n.tech[i] = v; } f >> n.researchTech >> n.researchEnd; }
  if (magic == "DOK4" || magic == "DOK5") f >> n.wallLvl >> n.trapLvl;
  if (magic == "DOK5") { int mo, so; f >> n.season >> n.seasonStart >> n.sWins >> n.sDef >> mo >> so; n.musicOn = mo; n.sfxOn = so; }
  size_t c; f >> c; for (size_t i = 0; i < c; i++) { float v; f >> v; n.vq.push_back(v); }
  f >> c; for (size_t i = 0; i < c; i++) {
    Bld b{}; int ty, bd; size_t qc; f >> b.id >> ty >> b.x >> b.y >> b.lvl >> b.workers >> bd >> b.bStart >> b.bEnd >> b.bTo >> b.acc >> qc;
    b.type = (BType)ty; b.building = bd; b.w = bdef(b.type).w; b.h = bdef(b.type).h;
    for (size_t k = 0; k < qc; k++) { int ut; float l; f >> ut >> l; b.q.push_back({(UType)ut, l}); }
    n.b.push_back(b);
  }
  f >> c; for (size_t i = 0; i < c; i++) { Node nd; int ty; f >> nd.x >> nd.y >> ty >> nd.amt >> nd.max >> nd.respawn; nd.type = (NodeType)ty; n.nodes.push_back(nd); }
  if (!f || n.b.empty()) return false;
  S = n; LORDS.clear(); lordsBuilt = false; return true;
}
