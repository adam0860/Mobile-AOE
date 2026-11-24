#include "game.h"
#include <SDL.h>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>

State S;
static const int GOLD_AMT = 1000, STONE_AMT = 800;       // deposits last twice as long as they used to
static const int TREE_WOOD = 400;                   // wood in one tree (a lumber camp's patch lasts a long time)
static std::vector<Lord> LORDS;
static bool lordsBuilt = false;

void newGame() {
  Meta keepMeta = S.meta; bool kMo = S.musicOn, kSo = S.sfxOn; float kMv = S.musicVol, kSv = S.sfxVol; S = State(); S.meta = keepMeta; S.musicOn = kMo; S.sfxOn = kSo; S.musicVol = kMv; S.sfxVol = kSv; LORDS.clear(); lordsBuilt = false;
  Rng r((uint32_t)std::time(nullptr)); S.seed = (uint32_t)(r.next() * 1e9);
  S.lastSeen = (long long)std::time(nullptr);
  addB(B_TOWNHALL, 13, 13, 1); addB(B_HOUSE, 18, 13, 1); addB(B_TOWER, 13, 18, 1);
  Rng rr(S.seed);
  for (int i = 3; i < 10; i++) for (int j = 3; j < 10; j++) if (rr.next() < 0.62) S.nodes.push_back({i, j, N_TREE, TREE_WOOD, TREE_WOOD, 0});
  int g[5][2] = {{22, 6}, {23, 6}, {22, 7}, {24, 7}, {23, 8}}, s[5][2] = {{6, 22}, {7, 22}, {6, 23}, {8, 23}, {7, 24}};
  for (auto& p : g) S.nodes.push_back({p[0], p[1], N_GOLD, GOLD_AMT, GOLD_AMT, 0});
  for (auto& p : s) S.nodes.push_back({p[0], p[1], N_STONE, STONE_AMT, STONE_AMT, 0});
  genWorld();
  topUpNodes();
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
int idleV() { int ev = 0; for (auto& e : S.expl) if (e.vill) ev++; return S.villagers - builders() - assigned() - ev; }
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
  if (b.type == B_TOWNHALL) { revealHome(); topUpNodes(); }
  sfx(b.type == B_TOWNHALL ? 13 : 2);
  if (b.type == B_TOWNHALL) banner(b.lvl == 3 ? "Feudal Age reached!" : b.lvl == 5 ? "Castle Age reached!" : b.lvl == 8 ? "Imperial Age reached!" : "Town Center Level " + std::to_string(b.lvl));
  else toast(std::string(d.name) + " ready");
}

// ===================================================== exploration world, hunting, regrowing deposits
bool g_fogDirty = true;
static Rng gRng(0x5eed1234u);
static float frnd() { return (float)gRng.next(); }
int homeG() { return 2 * ((clampi(tcLvl(), 1, MAXTC) - 1) / 2); }
bool inHome(float x, float y) { int g = homeG(); return x >= -g && y >= -g && x < MAP + g && y < MAP + g; }
const uint32_t LORD_COL[NLORDCOL] = {0xd8372c, 0x3fae4f, 0xeacb2e, 0xf28a22, 0x9048cc, 0x8e939c};
const char* LORD_COL_NAME[NLORDCOL] = {"Red", "Green", "Yellow", "Orange", "Purple", "Grey"};
static bool homeRect(float x, float y, float m = 0) { int g = homeG(); return x >= -g - m && y >= -g - m && x < MAP + g + m && y < MAP + g + m; }
static bool fixedHome(int q, float x, float y, float m) { float hx = (float)(QUAD * (q % 2)), hy = (float)(QUAD * (q / 2)); return x >= hx - m && y >= hy - m && x < hx + MAP + m && y < hy + MAP + m; }
static bool inAnyHome(float x, float y, float m) { if (homeRect(x, y, m)) return true; for (int q = 1; q < 4; q++) if (fixedHome(q, x, y, m)) return true; return false; }
void revealHome() { if (S.fog.size() != (size_t)WORLD * WORLD) return; int g = homeG(); for (int y = -g - 1; y <= MAP + g; y++) for (int x = -g - 1; x <= MAP + g; x++) { uint8_t& f = S.fog[(y + WEXT) * WORLD + (x + WEXT)]; if (!f) { f = 1; g_fogDirty = true; } } }
bool inWorld(float x, float y) { return x >= -WEXT && y >= -WEXT && x < WHI && y < WHI; }
bool isExplored(int x, int y) {
  if (x < -WEXT || y < -WEXT || x >= WHI || y >= WHI || S.fog.size() != (size_t)WORLD * WORLD) return false;
  return S.fog[(y + WEXT) * WORLD + (x + WEXT)] != 0;
}
void revealAround(float cx, float cy, float r) {
  if (S.fog.size() != (size_t)WORLD * WORLD) return;
  int x0 = (int)std::floor(cx - r), x1 = (int)std::ceil(cx + r), y0 = (int)std::floor(cy - r), y1 = (int)std::ceil(cy + r);
  for (int y = std::max(y0, -WEXT); y <= std::min(y1, WHI - 1); y++) for (int x = std::max(x0, -WEXT); x <= std::min(x1, WHI - 1); x++)
    if (std::hypot(x + .5f - cx, y + .5f - cy) <= r) { uint8_t& f = S.fog[(y + WEXT) * WORLD + (x + WEXT)]; if (!f) { f = 1; g_fogDirty = true; } }
}
static bool tileFreeForNode(int x, int y) {
  for (auto& n : S.nodes) if (n.x == x && n.y == y) return false;
  for (auto& b : S.b) if (x >= b.x - 1 && x < b.x + b.w + 1 && y >= b.y - 1 && y < b.y + b.h + 1) return false;
  return true;
}
static const float ANIMAL_FOOD[3] = {140, 220, 90};
static const float ANIMAL_HP[3] = {50, 140, 80};
static void addAnimal(int type, float x, float y) {
  Animal a{}; a.hp = a.maxHp = ANIMAL_HP[type]; a.id = S.aid++; a.type = type; a.x = a.hx = a.tx = x; a.y = a.hy = a.ty = y; a.food = a.maxFood = ANIMAL_FOOD[type]; a.alive = true; a.wait = frnd() * 6; a.face = (int)(frnd() * 4); S.animals.push_back(a);
}
static void spawnRelicAt(float x, float y) { S.relics.push_back({S.aid++, x, y}); }
static void genQuadrant(int q, Rng& r) {
  float qlx = (float)(QUAD * (q % 2) - WEXT), qly = (float)(QUAD * (q / 2) - WEXT), hx = (float)(QUAD * (q % 2)), hy = (float)(QUAD * (q / 2));
  auto inHomeQ = [&](float x, float y, float m) { return x >= hx - m && y >= hy - m && x < hx + MAP + m && y < hy + MAP + m; };
  auto outerCentre = [&](float margin, float& cx, float& cy) {
    for (int k = 0; k < 200; k++) { cx = qlx + 3 + (float)r.next() * (QUAD - 6); cy = qly + 3 + (float)r.next() * (QUAD - 6); if (!inHomeQ(cx, cy, margin)) return; }
  };
  auto patch = [&](NodeType t, int count, float rad, float amt, float margin) {
    float cx, cy; outerCentre(margin, cx, cy); int placed = 0;
    for (int k = 0; k < count * 8 && placed < count; k++) {
      int x = (int)std::floor(cx + (r.next() * 2 - 1) * rad), y = (int)std::floor(cy + (r.next() * 2 - 1) * rad);
      if (inHomeQ((float)x, (float)y, 2) || x < qlx + 1 || y < qly + 1 || x >= qlx + QUAD - 1 || y >= qly + QUAD - 1 || x < -WEXT + 1 || y < -WEXT + 1 || x >= WHI - 1 || y >= WHI - 1) continue;
      bool dup = false; for (auto& n : S.nodes) if (n.x == x && n.y == y) { dup = true; break; }
      if (dup) continue; S.nodes.push_back({x, y, t, amt, amt, 0}); placed++;
    }
  };
  for (int i = 0; i < 16; i++) patch(N_TREE, 20, 3.4f, TREE_WOOD, 3);
  for (int i = 0; i < 7; i++) patch(N_GOLD, 5, 1.8f, GOLD_AMT * 1.4f, 3);
  for (int i = 0; i < 6; i++) patch(N_STONE, 5, 1.8f, STONE_AMT * 1.5f, 3);
  for (int i = 0; i < 10; i++) { float cx, cy; outerCentre(2, cx, cy); int n = 3 + (int)(r.next() * 3); for (int k = 0; k < n; k++) addAnimal(A_DEER, cx + (float)(r.next() - .5) * 3, cy + (float)(r.next() - .5) * 3); }
  for (int i = 0; i < 8; i++) { float cx, cy; outerCentre(3, cx, cy); int n = 1 + (int)(r.next() * 2); for (int k = 0; k < n; k++) addAnimal(A_BOAR, cx + (float)(r.next() - .5) * 2, cy + (float)(r.next() - .5) * 2); }
  for (int i = 0; i < 7; i++) { float cx, cy; outerCentre(q == 0 ? 7 : 4, cx, cy); int n = 2 + (int)(r.next() * 2); for (int k = 0; k < n; k++) addAnimal(A_WOLF, cx + (float)(r.next() - .5) * 2.4f, cy + (float)(r.next() - .5) * 2.4f); }
  for (int i = 0; i < 3; i++) { float cx, cy; outerCentre(3, cx, cy); spawnRelicAt(std::floor(cx) + .5f, std::floor(cy) + .5f); }
  for (int i = 0; i < 14; i++) {
    float cx, cy; outerCentre(2, cx, cy); Chest c{(int)std::floor(cx), (int)std::floor(cy), {}, false};
    int k = (int)(r.next() * 4), k2 = (k + 1 + (int)(r.next() * 3)) % 4; c.c.v[k] = 250 + (int)(r.next() * 450); c.c.v[k2] = 120 + (int)(r.next() * 300); if (k == GOLD || k2 == GOLD) c.c.v[GOLD] += 100;
    S.chests.push_back(c);
  }
}
// ---- rival lords ----
static double enemyLevelTime(int tc) { return 1100.0 + 250.0 * tc; }
static void spawnEnemy(int quad, int tc) {
  Enemy e; e.id = S.aid++; e.quad = quad; e.tc = clampi(tc, 1, MAXTC);
  Rng r(S.seed ^ (uint32_t)(e.id * 7919) ^ (uint32_t)(S.t * 3)); e.name = std::string(LORD_FIRST[(int)(r.next() * 30)]) + " " + LORD_TITLE[(int)(r.next() * 10)];
  e.seed = (uint32_t)(r.next() * 4e9); e.pers = genBase(e.seed, e.tc, false, 0).pers; e.nextLvl = S.t + enemyLevelTime(e.tc);
  { bool used[NLORDCOL] = {}; for (auto& o : S.enemies) if (o.color >= 0 && o.color < NLORDCOL) used[o.color] = true; std::vector<int> fr; for (int c = 0; c < NLORDCOL; c++) if (!used[c]) fr.push_back(c); e.color = fr.empty() ? (int)(r.next() * NLORDCOL) % NLORDCOL : fr[(size_t)(r.next() * fr.size()) % fr.size()]; }     // a colour no living lord has; kept until he is defeated
  e.stance = (e.pers == 1 || e.pers == 3) ? ST_WAR : ST_PEACE;          // Raiders and Warlords start at war with you, Turtles and Merchants keep the peace
  S.enemies.push_back(e);
}
static void spawnFirstEnemy(int tc) { spawnEnemy(1 + (int)(frnd() * 3), tc); }
Enemy* findEnemy(int id) { for (auto& e : S.enemies) if (e.id == id) return &e; return nullptr; }
void enemyOrigin(const Enemy& e, int& ox, int& oy) { ox = QUAD * (e.quad % 2); oy = QUAD * (e.quad / 2); }
Base enemyBaseAt(const Enemy& e, int tc, size_t deadCount, std::vector<int>* srcIdx) {
  Base full = genBase(e.seed, tc, false, 0), out; out.loot = full.loot; out.name = e.name; out.pers = full.pers;
  size_t nd = tc == e.tc ? std::min(deadCount, e.dead.size()) : 0;
  for (int i = 0; i < (int)full.buildings.size(); i++) { if (std::find(e.dead.begin(), e.dead.begin() + nd, i) != e.dead.begin() + nd) continue; out.buildings.push_back(full.buildings[i]); if (srcIdx) srcIdx->push_back(i); }
  return out;
}
Base enemyBase(const Enemy& e, std::vector<int>* srcIdx) { return enemyBaseAt(e, e.tc, e.dead.size(), srcIdx); }
bool enemyDiscovered(const Enemy& e) { return e.seen; }
static UType tierUnit(int tc, int i) {
  if (tc >= 8) return (i % 8 == 5) ? U_TREBUCHET : (tc >= 9 && i % 9 == 4) ? U_BOMBARD : (i % 5 == 1) ? U_PALADIN : (i % 3 == 0) ? ((i % 2) ? U_CANNONEER : U_ARBALEST) : (i % 2 == 0) ? U_CHAMPION : U_LONGSWORD;
  if (tc >= 5) return (i % 7 == 3) ? U_RAM : (i % 5 == 1) ? U_KNIGHT : (i % 3 == 0) ? U_CROSSBOW : (i % 2 == 0) ? U_LONGSWORD : U_MANATARMS;
  if (tc >= 3) return (i % 3 == 0) ? U_ARCHER : (i % 4 == 1) ? U_MANATARMS : U_MILITIA;
  return U_MILITIA;
}
void addEnemyDefenders(Sim& sim, const Enemy& e) {
  Rng r(e.seed ^ (uint32_t)(e.tc * 2654435761u)); float var = 0.7f + 0.6f * (float)r.next();
  int n = clampi(roundi((e.tc * 1.6f + 1.5f) * var), 2, 36); float c = SOFF + 15.f;
  for (int i = 0; i < n; i++) {
    UType t = tierUnit(e.tc, i); if (t == U_RAM || t == U_TREBUCHET || t == U_BOMBARD) t = U_LONGSWORD;           // siege stays in the attackers' hands
    float a = (float)(r.next() * 6.2831853), d = 2.5f + (float)r.next() * 4.f; sim.deploy(t, c + std::cos(a) * d, c + std::sin(a) * d, true);
  }
}
void enemyDamage(int id, const std::vector<int>& deadIdx) { if (Enemy* e = findEnemy(id)) for (int i : deadIdx) if (std::find(e->dead.begin(), e->dead.end(), i) == e->dead.end()) e->dead.push_back(i); }
void enemyWiped(int id) { for (auto it = S.enemies.begin(); it != S.enemies.end(); ++it) if (it->id == id) { S.enemies.erase(it); return; } }
void genWorld() {
  S.fog.assign((size_t)WORLD * WORLD, 0); S.animals.clear(); S.chests.clear(); S.expl.clear(); S.scq.clear(); S.scoutStock = 0; S.enemies.clear();
  for (int i = 0; i < 3; i++) { S.patchX[i] = S.patchY[i] = -1; S.patchN[i] = 0; }
  revealHome();
  Rng r(S.seed ^ 0xC0FFEEu);
  for (int q = 0; q < 4; q++) genQuadrant(q, r);
  spawnFirstEnemy(1); S.enemyCheck = S.t + 900; S.relicCheck = S.t + 1800;
  g_fogDirty = true;
}
static void genWorldOldSave() {                 // saves from before the four quadrants: keep the explored land, add the other three quadrants and a rival
  Rng r(S.seed ^ 0xBADC0DEu); for (int q = 1; q < 4; q++) genQuadrant(q, r);
  S.enemies.clear(); spawnFirstEnemy(std::max(1, roundi(tcLvl() * 0.6))); S.enemyCheck = S.t + 900; g_fogDirty = true;
}
// A depleted deposit regrows somewhere else (so camps have to be demolished and rebuilt next to the new patch).
static bool findNodeSpot(Node& n);
static void relocateNode(Node& n) {
  bool outer = !homeRect((float)n.x, (float)n.y);
  if (outer) {
    for (int k = 0; k < 200; k++) {
      int x = -WEXT + 1 + (int)(frnd() * (WORLD - 2)), y = -WEXT + 1 + (int)(frnd() * (WORLD - 2));
      if (inAnyHome((float)x, (float)y, 2) || !tileFreeForNode(x, y)) continue;
      n.x = x; n.y = y; break;
    }
    n.amt = n.max; return;
  }
  if (!findNodeSpot(n)) n.amt = n.max;
}
static bool findNodeSpot(Node& n) {
  int ti = (int)n.type;
  int cap = n.type == N_TREE ? 12 : 5;
  if (S.patchX[ti] < 0 || S.patchN[ti] >= cap) {                 // pick a new patch away from the old deposit and from existing camps of this kind
    BType camp = n.type == N_TREE ? B_LUMBER : n.type == N_GOLD ? B_GOLDCAMP : B_STONECAMP; float bestScore = -1; int bx = 8, by = 8;
    Bld& tc = tcB();
    for (int k = 0; k < 90; k++) {
      int hg = homeG(); int cx = -hg + 4 + (int)(frnd() * (MAP + 2 * hg - 8)), cy = -hg + 4 + (int)(frnd() * (MAP + 2 * hg - 8));
      if (std::hypot(cx - (tc.x + 1.5f), cy - (tc.y + 1.5f)) < 5.5f) continue;
      int freeCnt = 0; for (int dx = -3; dx <= 3; dx++) for (int dy = -3; dy <= 3; dy++) { int x = cx + dx, y = cy + dy; if (x >= -hg + 2 && y >= -hg + 2 && x < MAP + hg - 2 && y < MAP + hg - 2 && tileFreeForNode(x, y)) freeCnt++; }
      if (freeCnt < 6) continue;
      float d = 99; for (auto& b : S.b) if (b.type == camp) d = std::min(d, (float)std::hypot(cx - (b.x + b.w / 2.f), cy - (b.y + b.h / 2.f)));
      d = std::min(d, 12.f) + frnd() * 2.f + freeCnt * 0.15f; if (d > bestScore) { bestScore = d; bx = cx; by = cy; }
    }
    S.patchX[ti] = bx; S.patchY[ti] = by; S.patchN[ti] = 0;
  }
  for (int k = 0; k < 120; k++) {
    float rad = 3.2f + k * 0.03f; int x = (int)std::floor(S.patchX[ti] + (frnd() * 2 - 1) * rad), y = (int)std::floor(S.patchY[ti] + (frnd() * 2 - 1) * rad);
    if (x < -homeG() + 2 || y < -homeG() + 2 || x >= MAP + homeG() - 2 || y >= MAP + homeG() - 2 || !tileFreeForNode(x, y)) continue;
    n.x = x; n.y = y; n.amt = n.max; S.patchN[ti]++; return true;
  }
  for (int k = 0; k < 300; k++) {                                // the patch is full: any free spot in the village
    int hg = homeG(); int x = -hg + 2 + (int)(frnd() * (MAP + 2 * hg - 4)), y = -hg + 2 + (int)(frnd() * (MAP + 2 * hg - 4));
    if (tileFreeForNode(x, y)) { n.x = x; n.y = y; n.amt = n.max; return true; }
  }
  return false;      // no space anywhere
}
static void animalSpot(Animal& a) {
  for (int k = 0; k < 80; k++) {
    float x = -WEXT + 1 + frnd() * (WORLD - 2), y = -WEXT + 1 + frnd() * (WORLD - 2);
    if (inAnyHome(x, y, 3)) continue;
    if (a.type == A_WOLF && homeRect(x, y, 7)) continue;
    bool hidden = !isExplored((int)std::floor(x), (int)std::floor(y)); if (!hidden && k < 40) continue;      // prefer unexplored land
    a.x = a.hx = a.tx = x; a.y = a.hy = a.ty = y; return;
  }
}
static void computeVisibility();
static int g_arrSquad = -1, g_arrEnemy = -1;
static float villHp() { return S.meta.boon == BOON_HARDY ? 110.f : 30.f; }
static void worldTick(float dt) {
  double t = S.t;
  for (auto& a : S.animals) {
    if (!a.alive) { if (t >= a.respawn) { animalSpot(a); a.food = a.maxFood; a.hp = a.maxHp; a.alive = true; } continue; }
    float sp = a.type == A_DEER ? 1.0f : a.type == A_BOAR ? 0.8f : 1.1f; a.chasing = false;
    if (a.type != A_DEER) {                                       // wolves and boars attack any of your units that come near: explorers, villagers, monks, scouts, squads
      float aggro = a.type == A_WOLF ? 4.5f : 2.5f, best = aggro; Explorer* te = nullptr; Squad* ts = nullptr; float tx = 0, ty = 0;
      for (auto& ex : S.expl) { float d = std::hypot(ex.x - a.x, ex.y - a.y); if (d < best) { best = d; te = &ex; ts = nullptr; tx = ex.x; ty = ex.y; } }
      for (auto& sq : S.squads) { float d = std::hypot(sq.x - a.x, sq.y - a.y); if (d < best) { best = d; ts = &sq; te = nullptr; tx = sq.x; ty = sq.y; } }
      if ((te || ts) && std::hypot(a.hx - tx, a.hy - ty) < 11.f) {
        a.chasing = true; a.tx = tx; a.ty = ty; sp = a.type == A_WOLF ? 2.4f : 2.0f;
        if (best < 1.0f) { float dmg = (a.type == A_WOLF ? 7.f : 11.f) * dt; if (te) { if (te->maxHp <= 0) te->maxHp = te->hp = te->vill ? villHp() : te->monk ? 45.f : 70.f; te->hp -= dmg; te->hurt = 2.5f; if (te->vill && S.meta.boon == BOON_HARDY) { a.hp -= 20.f * dt; if (a.hp <= 0) { a.alive = false; a.chasing = false; a.respawn = t + 240; a.food = 0; toast(std::string("Your villager killed the ") + (a.type == A_WOLF ? "wolf" : "boar") + "!"); sfx(3); } } } else { float rest = dmg; while (rest > 0 && !ts->u.empty()) { SqUnit& f0 = ts->u.front(); if (f0.hp > rest) { f0.hp -= rest; rest = 0; } else { rest -= f0.hp; ts->u.erase(ts->u.begin()); } } ts->fight = 1.f; } }
      }
    }
    float dx = a.tx - a.x, dy = a.ty - a.y, L = std::hypot(dx, dy);
    if (L > 0.08f) { float s = std::min(sp * dt, L); a.x += dx / L * s; a.y += dy / L * s; a.aph += s / 0.7f; a.face = std::fabs(dx) > std::fabs(dy) ? (dx > 0 ? 0 : 2) : (dy > 0 ? 1 : 3); }
    else if (!a.chasing) { a.wait -= dt; if (a.wait <= 0) { a.wait = 3 + frnd() * 7; a.tx = a.hx + (frnd() * 2 - 1) * 3.2f; a.ty = a.hy + (frnd() * 2 - 1) * 3.2f; } }
  }
  for (auto& e : S.enemies) if (t >= e.nextLvl && e.tc < MAXTC) { e.tc++; e.dead.clear(); e.nextLvl = t + enemyLevelTime(e.tc); }
  // trade routes pay out, relics slowly make gold, fresh relics turn up, treaties with Raiders / Warlords may not last
  { int allowed = tradeRoutesAllowed(), active = 0;
    for (auto& e : S.enemies) if (e.stance >= ST_TRADE) { if (active++ >= allowed) continue; Cost inc = tradeIncome(e); for (int k = 0; k < NRES; k++) { e.tacc[k] += inc.v[k] * dt / 60.f; int w = (int)std::floor(e.tacc[k]); if (w > 0) { e.tacc[k] -= w; S.res[k] = std::min((float)cap(), S.res[k] + w); } } } }
  { float gm = relicGoldPerMin(); if (gm > 0) S.res[GOLD] = std::min((float)cap(), S.res[GOLD] + gm * dt / 60.f); }
  if (t >= S.relicCheck) { S.relicCheck = t + 1200 + frnd() * 1200; int carrying = 0; for (auto& ex : S.expl) if (ex.carry) carrying++;
    if ((int)S.relics.size() + S.relicsHeld + carrying < 14) for (int k = 0; k < 60; k++) { float x = -WEXT + 2 + frnd() * (WORLD - 4), y = -WEXT + 2 + frnd() * (WORLD - 4); if (inAnyHome(x, y, 3)) continue; spawnRelicAt(std::floor(x) + .5f, std::floor(y) + .5f); break; } }
  if (t >= S.enemyCheck) {                                          // now and then a new lord settles an empty quadrant, a bit stronger the further you have come
    S.enemyCheck = t + 600 + frnd() * 600; bool used[4] = {true, false, false, false}; for (auto& e : S.enemies) used[e.quad] = true;
    std::vector<int> freeQ; for (int q = 1; q < 4; q++) if (!used[q]) freeQ.push_back(q);
    for (auto& e : S.enemies) if (e.stance == ST_PEACE && (e.pers == 1 || e.pers == 3) && frnd() < 0.12f) { e.stance = ST_WAR; e.trust = 0; banner(e.name + " has broken the peace!"); }
    if (!freeQ.empty() && frnd() < 0.3f) spawnEnemy(freeQ[(int)(frnd() * freeQ.size()) % freeQ.size()], std::max(1, roundi(tcLvl() * 0.7f) + (frnd() < 0.5f ? 1 : 0)));
  }
  if (!S.scq.empty()) { S.scq[0] -= dt; if (S.scq[0] <= 0) { S.scq.erase(S.scq.begin()); S.scoutStock++; toast("A scout is ready - open Explore"); sfx(2); } }
  for (size_t i = 0; i < S.expl.size();) {
    Explorer& e = S.expl[i]; bool kill = false, home = false;
    float sp = e.vill ? 1.5f : e.monk ? 2.1f : 3.6f;
    if (e.go) {
      float dx = e.tx - e.x, dy = e.ty - e.y, L = std::hypot(dx, dy);
      if (L < 0.12f) { e.go = false; if (e.recall) home = true; }
      else { float s = std::min(sp * dt, L); e.x += dx / L * s; e.y += dy / L * s; e.aph += s / (e.vill ? 0.95f : 1.9f); e.face = (int)std::lround(std::atan2(-dy, dx) / 0.785398f); e.face = ((e.face % 8) + 8) % 8; }
    } else if (e.autoX) {
      float bd = 1e9f; int bx = 0, by = 0; bool found = false;
      for (int y = -WEXT + 1; y < WHI - 1; y++) for (int x = -WEXT + 1; x < WHI - 1; x++) {
        if (isExplored(x, y) || !(isExplored(x + 1, y) || isExplored(x - 1, y) || isExplored(x, y + 1) || isExplored(x, y - 1))) continue;
        float d = std::hypot(x + .5f - e.x, y + .5f - e.y) + frnd() * 3.f; if (d < bd) { bd = d; bx = x; by = y; found = true; }
      }
      if (found) { e.tx = bx + .5f; e.ty = by + .5f; e.go = true; } else { e.autoX = false; toast("Everything reachable is explored"); }
    }
    revealAround(e.x, e.y, e.vill ? 4.2f : e.monk ? 5.f : 6.5f);
    if (e.monk && !e.carry) for (size_t k = 0; k < S.relics.size(); k++) if (std::hypot(S.relics[k].x - e.x, S.relics[k].y - e.y) < 1.3f) {
      S.relics.erase(S.relics.begin() + k); e.carry = true; e.autoX = false; e.recall = true; e.go = true;
      Bld* mon = nullptr; for (auto& b : S.b) if (b.type == B_MONASTERY && isDone(b)) mon = &b; Bld& dst = mon ? *mon : tcB(); e.tx = dst.x + dst.w / 2.f; e.ty = dst.y + dst.h + 0.8f;
      toast("The monk picked up a relic and carries it home"); sfx(13); break;
    }
    for (auto& c : S.chests) if (!c.taken && std::hypot(c.x + .5f - e.x, c.y + .5f - e.y) < 1.1f) {
      c.taken = true; for (int k = 0; k < NRES; k++) S.res[k] = std::min((float)cap(), S.res[k] + c.c.v[k]); toast("Treasure found!"); sfx(3);
    }
    if (e.maxHp <= 0) e.maxHp = e.hp = e.vill ? villHp() : e.monk ? 45.f : 70.f;
    if (e.hurt > 0) { e.hurt -= dt; if ((e.vill || e.monk) && !e.recall && !e.carry && !(e.vill && S.meta.boon == BOON_HARDY && e.hp > 0.4f * e.maxHp)) { Bld* mon = nullptr; if (e.monk) for (auto& b : S.b) if (b.type == B_MONASTERY && isDone(b)) mon = &b; Bld& dst = mon ? *mon : tcB(); e.tx = dst.x + dst.w / 2.f; e.ty = dst.y + dst.h + 0.8f; e.go = true; e.recall = true; e.autoX = false; } }
    else if (e.hp < e.maxHp) e.hp = std::min(e.maxHp, e.hp + 1.5f * dt);
    if (e.hp <= 0) kill = true;
    if (kill) { if (e.vill) S.villagers = std::max(0, S.villagers - 1); if (e.carry) S.relics.push_back({S.aid++, e.x, e.y}); toast(e.monk ? "A monk was killed by wild animals!" : e.vill ? "A villager was killed by wild animals!" : "Your scout was killed by wild animals!"); S.expl.erase(S.expl.begin() + i); continue; }
    if (home) {
      if (e.monk) { S.army[U_MONK]++; if (e.carry) { S.relicsHeld++; toast(buildCount(B_MONASTERY) ? "Relic delivered to the monastery!" : "Relic stored - build a Monastery to benefit from it"); sfx(14); } }
      else if (!e.vill) S.scoutStock++;
      S.expl.erase(S.expl.begin() + i); continue;
    }
    i++;
  }
  // ---- field squads
  for (size_t i = 0; i < S.squads.size();) {
    Squad& q = S.squads[i]; if (q.u.empty()) { S.squads.erase(S.squads.begin() + i); continue; }
    float spd = squadSpeed(q); q.fight = std::max(0.f, q.fight - dt);
    float mulM = techMul().melee, mulR = techMul().ranged;
    if (q.mode == SQ_ESCORT) { Explorer* ex = findExplorer(q.target); if (!ex) { q.mode = SQ_FREE; q.go = false; } else { float d = std::hypot(ex->x - q.x, ex->y - q.y); if (d > 1.7f) { q.tx = ex->x; q.ty = ex->y; q.go = true; spd = std::max(spd, ex->vill ? 1.7f : ex->monk ? 2.2f : spd); } else q.go = false; } }
    else if (q.mode == SQ_GUARD) { if (std::hypot(q.gx - q.x, q.gy - q.y) > 0.7f) { q.tx = q.gx; q.ty = q.gy; q.go = true; } else q.go = false; }
    else if (q.mode == SQ_ATTACK) { Enemy* en = findEnemy(q.target); if (!en) { q.mode = SQ_FREE; q.go = false; } else { int ox, oy; enemyOrigin(*en, ox, oy); q.tx = ox + 15.f; q.ty = oy + 15.f; q.go = true; if (std::hypot(q.tx - q.x, q.ty - q.y) <= 14.f) { g_arrSquad = q.id; g_arrEnemy = en->id; q.mode = SQ_FREE; q.go = false; } } }
    // fight wild animals in reach (melee up close, archers from range); idle troops walk up to animals that threaten them
    Animal* tgt = nullptr; float bd = 6.f; for (auto& a : S.animals) if (a.alive) { float d = std::hypot(a.x - q.x, a.y - q.y); if (d < bd && (a.type != A_DEER || d < 1.8f)) { bd = d; tgt = &a; } }
    if (tgt) {
      float dm = 0, dr = 0; for (auto& u : q.u) { const UDef& d = udef(u.type); if (d.siege) continue; float dps = d.dmg / std::max(0.5f, d.rate); if (d.ranged) dr += dps * mulR; else dm += dps * mulM; }
      float dmg = (bd <= 1.8f ? dm + dr : bd <= 4.8f ? dr : 0.f) * dt;
      if (dmg > 0) { tgt->hp -= dmg; q.fight = 0.6f; if (tgt->hp <= 0) { tgt->alive = false; tgt->respawn = S.t + 240 + frnd() * 120; S.res[FOOD] = std::min((float)cap(), S.res[FOOD] + tgt->food * 0.4f); toast(std::string("Your troops killed a ") + (tgt->type == A_DEER ? "deer" : tgt->type == A_BOAR ? "boar" : "wolf") + " (+" + std::to_string((int)(tgt->food * 0.4f)) + " food)"); tgt->hp = tgt->maxHp; } }
      if (!q.go && bd > 1.4f && tgt->type != A_DEER && q.mode != SQ_ESCORT) { q.tx = tgt->x; q.ty = tgt->y; q.go = true; spd = std::min(spd, 2.2f); }
    }
    if (q.go) { float dx = q.tx - q.x, dy = q.ty - q.y, L = std::hypot(dx, dy); if (L < 0.15f) { q.go = false; if (q.recall) { for (auto& u : q.u) S.army[u.type]++; toast("The squad is back home"); S.squads.erase(S.squads.begin() + i); continue; } } else { float s = std::min(spd * dt, L); q.x += dx / L * s; q.y += dy / L * s; q.aph += s / 1.6f; q.face = (int)std::lround(std::atan2(-dy, dx) / 0.785398f); q.face = ((q.face % 8) + 8) % 8; } }
    revealAround(q.x, q.y, 6.5f); i++;
  }
  computeVisibility();
}
bool g_visDirty = true;
bool isVisible(int x, int y) { if (x < -WEXT || y < -WEXT || x >= WHI || y >= WHI || S.vis.size() != (size_t)WORLD * WORLD) return false; return S.vis[(y + WEXT) * WORLD + (x + WEXT)] != 0; }
static void computeVisibility() {
  static std::vector<uint8_t> nv; nv.assign((size_t)WORLD * WORLD, 0);
  auto mark = [&](float cx, float cy, float r) {
    revealAround(cx, cy, r);
    int x0 = (int)std::floor(cx - r), x1 = (int)std::ceil(cx + r), y0 = (int)std::floor(cy - r), y1 = (int)std::ceil(cy + r);
    for (int y = std::max(y0, -WEXT); y <= std::min(y1, WHI - 1); y++) for (int x = std::max(x0, -WEXT); x <= std::min(x1, WHI - 1); x++) if (std::hypot(x + .5f - cx, y + .5f - cy) <= r) nv[(size_t)(y + WEXT) * WORLD + (x + WEXT)] = 1;
  };
  for (int y = -WEXT; y < QUAD - WEXT; y++) for (int x = -WEXT; x < QUAD - WEXT; x++) if (isExplored(x, y)) nv[(size_t)(y + WEXT) * WORLD + (x + WEXT)] = 1;      // your own quadrant is always live
  for (auto& b : S.b) { float cx = b.x + b.w / 2.f, cy = b.y + b.h / 2.f; if (cx < QUAD - WEXT && cy < QUAD - WEXT) continue; const BDef& d = bdef(b.type);
    mark(cx, cy, isPiece(b.type) ? 2.5f : b.type == B_KEEP ? 11.f : b.type == B_BOMBARD ? 10.f : d.dps[0] > 0 ? 9.f : 6.f); }
  for (auto& e : S.expl) mark(e.x, e.y, e.vill ? 4.2f : e.monk ? 5.f : 6.5f);
  for (auto& q : S.squads) mark(q.x, q.y, 6.5f);
  for (auto& en : S.enemies) if (en.stance == ST_ALLY) { int ox, oy; enemyOrigin(en, ox, oy); mark(ox + 15.f, oy + 15.f, 22.f); }               // allies share their view
  if (nv != S.vis) { S.vis = nv; g_visDirty = true; }
  for (auto& en : S.enemies) {
    int ox, oy; enemyOrigin(en, ox, oy); bool v = false;
    for (int y = oy + 3; y < oy + 28 && !v; y += 4) for (int x = ox + 3; x < ox + 28; x += 4) if (isVisible(x, y)) { v = true; break; }
    en.visNow = v; if (v) { if (!en.seen) { banner("You have found " + en.name + "!"); sfx(13); } en.seen = true; en.seenTc = en.tc; en.seenDead = en.dead.size(); }
  }
}
static void huntTick(Bld& b, const BDef& d, float amount, float dt) {
  float cx = b.x + b.w / 2.f, cy = b.y + b.h / 2.f; std::vector<Animal*> as;
  for (auto& a : S.animals) if (a.alive && std::hypot(a.x - cx, a.y - cy) <= 9.f) as.push_back(&a);
  if (as.empty()) { b.status = 4; return; }
  std::sort(as.begin(), as.end(), [&](Animal* p, Animal* q) { return std::hypot(p->x - cx, p->y - cy) < std::hypot(q->x - cx, q->y - cy); });
  float got = 0; Animal* cur = nullptr;
  for (Animal* a : as) { float take = std::min(a->food, amount - got); a->food -= take; got += take; if (!cur) cur = a; if (a->food <= 0) { a->alive = false; a->respawn = S.t + 240 + frnd() * 120; } if (got >= amount - 1e-6f) break; }
  b.status = 2; b.acc += got; int whole = (int)std::floor(b.acc);
  if (whole) { b.acc -= whole; S.res[d.res] = std::min((float)cap(), S.res[d.res] + whole); }
  if (cur && cur->type != A_DEER && b.workers > 0) {               // boars and wolves fight back; towers nearby protect the hunters
    int guards = 0; for (auto& q : S.squads) if (std::hypot(q.x - cx, q.y - cy) <= 9.f) guards += (int)q.u.size();
    int towers = 0; for (auto& o : S.b) if (isDone(o) && bdef(o.type).dps[0] > 0 && !isPiece(o.type) && std::hypot(o.x + o.w / 2.f - cx, o.y + o.h / 2.f - cy) <= 9.f) towers++;
    float lam = (cur->type == A_BOAR ? 1.f / 130 : 1.f / 100) * std::min(b.workers, 3) * dt * std::max(0.1f, std::pow(0.5f, (float)towers + guards / 3.f));
    if (frnd() < lam) { b.workers--; S.villagers = std::max(0, S.villagers - 1); toast(cur->type == A_BOAR ? "A hunter was gored by a boar!" : "A hunter was killed by wolves!"); }
  }
}
int scoutTotal() { int n = S.scoutStock + (int)S.scq.size(); for (auto& e : S.expl) if (!e.vill) n++; return n; }
void trainScout() {
  if (scoutTotal() >= 4) { toast("You can field at most 4 scouts"); return; }
  if (!canAfford(mkc(0, 60, 30))) { toast("Need 60 food and 30 gold"); return; }
  pay(mkc(0, 60, 30)); S.scq.push_back(15); sfx(1);
}
bool sendExplorer(bool villager) {
  Bld& tc = tcB();
  if (villager) { if (idleV() < 1) { toast("No idle villager to send"); return false; } }
  else { if (S.scoutStock < 1) { toast("No scout available - train one first"); return false; } S.scoutStock--; }
  Explorer e{}; e.id = S.aid++; e.vill = villager; e.maxHp = e.hp = villager ? 30.f : 70.f; e.x = e.tx = tc.x + 1.5f; e.y = e.ty = tc.y + 3.6f; e.face = 3; S.expl.push_back(e);
  toast(villager ? "Villager sent out - tap the map to choose where" : "Scout sent out - tap the map to choose where"); return true;
}
Explorer* findExplorer(int id) { for (auto& e : S.expl) if (e.id == id) return &e; return nullptr; }
void orderExplorer(int id, float x, float y) {
  Explorer* e = findExplorer(id); if (!e) return;
  e->tx = clampf(x, -WEXT + 0.5f, WHI - 0.5f); e->ty = clampf(y, -WEXT + 0.5f, WHI - 0.5f); e->go = true; e->recall = false; e->autoX = false;
}
void recallExplorer(int id) { Explorer* e = findExplorer(id); if (!e) return; Bld* mon = nullptr; if (e->monk) for (auto& b : S.b) if (b.type == B_MONASTERY && isDone(b)) mon = &b; Bld& tc = mon ? *mon : tcB(); e->tx = tc.x + 1.5f; e->ty = tc.y + 3.6f; e->go = true; e->recall = true; e->autoX = false; }
bool demolish(int id) {
  for (auto it = S.b.begin(); it != S.b.end(); ++it) if (it->id == id && it->type != B_TOWNHALL && !isPiece(it->type)) {
    const BDef& d = bdef(it->type); for (int i = 0; i < NRES; i++) S.res[i] = std::min((float)cap(), S.res[i] + d.cost.v[i] / 2);
    S.b.erase(it); return true;
  }
  return false;
}
static bool bOutside(const Bld& b) { return !inHome(b.x + b.w / 2.f, b.y + b.h / 2.f); }
int outsideBuildings() { int n = 0; for (auto& b : S.b) if (bOutside(b)) n++; return n; }

// ===================================================== field squads
Squad* findSquad(int id) { for (auto& q : S.squads) if (q.id == id) return &q; return nullptr; }
int fieldPop() { int n = 0; for (auto& q : S.squads) for (auto& u : q.u) n += udef(u.type).pop; return n; }
float squadSpeed(const Squad& s) { float m = 99; for (auto& u : s.u) m = std::min(m, udef(u.type).speed); return (m > 90 ? 1.f : m) * 0.9f; }
float squadHpFrac(const Squad& s) { float cur = 0, mx = 0; for (auto& u : s.u) { cur += u.hp; mx += udef(u.type).hp * techMul().hp; } return mx > 0 ? clampf(cur / mx, 0, 1) : 0; }
bool formSquad(const int counts[U_COUNT]) {
  Squad q; q.id = S.aid++; Bld& tc = tcB(); q.x = q.tx = tc.x + 1.5f + (float)(S.squads.size() % 3) * 1.2f; q.y = q.ty = tc.y + 4.6f; float hpm = techMul().hp;
  for (int t = 0; t < U_COUNT; t++) { int k = std::min(counts[t], S.army[t]); for (int i = 0; i < k; i++) q.u.push_back({t, udef(t).hp * hpm}); S.army[t] -= k; }
  if (q.u.empty()) { toast("Choose some troops first"); return false; }
  std::stable_sort(q.u.begin(), q.u.end(), [](const SqUnit& a, const SqUnit& b) { return udef(a.type).hp > udef(b.type).hp; });      // tough units in front
  S.squads.push_back(q); toast("Squad formed - tap it, then tap the map to move it"); return true;
}
void orderSquad(int id, float x, float y) { Squad* q = findSquad(id); if (!q) return; q->tx = clampf(x, -WEXT + 0.5f, WHI - 0.5f); q->ty = clampf(y, -WEXT + 0.5f, WHI - 0.5f); q->go = true; q->mode = SQ_FREE; q->recall = false; }
void squadEscort(int id, int ex) { Squad* q = findSquad(id); if (!q || !findExplorer(ex)) return; q->mode = SQ_ESCORT; q->target = ex; q->recall = false; toast("The squad will escort and protect it"); }
void squadGuard(int id) { Squad* q = findSquad(id); if (!q) return; q->mode = SQ_GUARD; q->gx = q->x; q->gy = q->y; q->go = false; q->recall = false; toast("The squad holds this ground"); }
void squadAttack(int id, int enemyId) { Squad* q = findSquad(id); if (!q || !findEnemy(enemyId)) return; q->mode = SQ_ATTACK; q->target = enemyId; q->recall = false; toast("The squad marches on the enemy"); }
void squadRecall(int id) { Squad* q = findSquad(id); if (!q) return; Bld& tc = tcB(); q->tx = tc.x + 1.5f; q->ty = tc.y + 4.6f; q->go = true; q->recall = true; q->mode = SQ_FREE; }
bool squadArrival(int& sid, int& eid) { if (g_arrSquad < 0) return false; sid = g_arrSquad; eid = g_arrEnemy; g_arrSquad = g_arrEnemy = -1; return true; }
void squadRemove(int id) { for (auto it = S.squads.begin(); it != S.squads.end(); ++it) if (it->id == id) { S.squads.erase(it); return; } }

// ===================================================== diplomacy
const char* stanceName(int s) { static const char* n[4] = {"At war", "At peace", "Trade route", "Allied"}; return n[clampi(s, 0, 3)]; }
int tradeRoutesAllowed() { int lv = 0; for (auto& b : S.b) if (b.type == B_MARKET && isDone(b)) lv = std::max(lv, b.lvl); return lv > 0 ? 1 + lv : 0; }
int tradeRoutesActive() { int n = 0; for (auto& e : S.enemies) if (e.stance >= ST_TRADE) n++; return n; }
Cost tradeIncome(const Enemy& e) {
  Cost c; float gold = 16.f + 5.f * e.tc + 3.f * tcLvl(); if (e.stance == ST_ALLY) gold *= 1.3f; c.v[GOLD] = roundi(gold);
  int second = e.quad == 1 ? WOOD : e.quad == 2 ? FOOD : STONE; c.v[second] = roundi(gold * 0.6f); return c;
}
const char* diploBlock(const Enemy& e, int p) {
  if (S.t < e.talkAt && S.meta.boon != BOON_DIPLOMAT) return "They will not talk to you right now";
  if (p == DP_PEACE && e.stance >= ST_PEACE) return "Already at peace";
  if (p == DP_TRADE) { if (e.stance < ST_PEACE) return "Make peace first"; if (e.stance >= ST_TRADE) return "A trade route already exists"; if (tradeRoutesAllowed() == 0) return "You need a Market to trade"; if (tradeRoutesActive() >= tradeRoutesAllowed()) return "Your Market cannot support another route (upgrade it)"; }
  if (p == DP_ALLY) { if (e.stance < (S.meta.boon == BOON_DIPLOMAT ? ST_PEACE : ST_TRADE)) return S.meta.boon == BOON_DIPLOMAT ? "Make peace first" : "Open a trade route first"; if (e.stance >= ST_ALLY) return "Already allies"; }
  return nullptr;
}
float diploChance(const Enemy& e, int p) {
  static const float base[3][4] = {{0.70f, 0.35f, 0.80f, 0.25f}, {0.50f, 0.30f, 0.85f, 0.20f}, {0.40f, 0.15f, 0.50f, 0.10f}};     // by personality: Turtle, Raider, Merchant, Warlord
  float c = base[clampi(p, 0, 2)][clampi(e.pers, 0, 3)] + 0.07f * e.trust + 0.04f * clampi(tcLvl() - e.tc, -4, 4) - (S.meta.boon == BOON_DIPLOMAT ? 0.03f : 0.10f) * S.betrayals;
  if (S.meta.boon == BOON_DIPLOMAT) c += 0.30f;
  if (p == DP_PEACE && e.stance == ST_WAR) c *= S.meta.boon == BOON_DIPLOMAT ? 0.95f : 0.8f; return clampf(c, 0.03f, S.meta.boon == BOON_DIPLOMAT ? 0.98f : 0.95f);
}
bool diploPropose(int id, int p, std::string& msg) {
  Enemy* e = findEnemy(id); if (!e) return false;
  if (const char* why = diploBlock(*e, p)) { msg = why; return false; }
  bool ok = (float)gRng.next() < diploChance(*e, p);
  if (ok) {
    if (p == DP_PEACE) { e->stance = ST_PEACE; msg = e->name + " agrees to peace. His raiders will leave you alone."; }
    else if (p == DP_TRADE) { e->stance = ST_TRADE; msg = "A trade route is open with " + e->name + "."; }
    else { e->stance = ST_ALLY; msg = e->name + " swears to protect you - and shares his view of the map."; }
    e->trust = std::min(5.f, e->trust + 1.f); sfx(13);
  } else { e->talkAt = S.t + (S.meta.boon == BOON_DIPLOMAT ? 0 : 240); e->trust = std::max(0.f, e->trust - (S.meta.boon == BOON_DIPLOMAT ? 0.1f : 0.5f)); msg = e->name + (S.meta.boon == BOON_DIPLOMAT ? " refuses - for now. Try again." : " refuses. He will not talk for a few minutes."); sfx(15); }
  return ok;
}
Cost diploGiftCost(const Enemy& e) { return mkc(0, 0, 100 * e.tc); }
void diploGift(int id, std::string& msg) {
  Enemy* e = findEnemy(id); if (!e) return; Cost c = diploGiftCost(*e);
  if (!canAfford(c)) { msg = "You cannot afford that gift"; return; }
  pay(c); e->trust = std::min(5.f, e->trust + 1.f); e->talkAt = std::min(e->talkAt, S.t); msg = e->name + " is pleased with the gift (trust " + std::to_string((int)e->trust) + "/5)."; sfx(3);
}
void diploWar(int id, bool treachery) {
  Enemy* e = findEnemy(id); if (!e) return; bool hadTreaty = e->stance >= ST_PEACE;
  if (treachery && hadTreaty) S.betrayals += e->stance == ST_ALLY ? 2 : 1;
  e->stance = ST_WAR; e->trust = 0; e->talkAt = S.t + 600;
}
float relicGoldPerMin() { int mon = 0; for (auto& b : S.b) if (b.type == B_MONASTERY && isDone(b)) mon++; return mon > 0 ? 9.f * S.relicsHeld : 0.f; }
bool sendMonk() {
  if (S.army[U_MONK] < 1) { toast("No monk available - train one at the Monastery"); return false; }
  Bld* mon = nullptr; for (auto& b : S.b) if (b.type == B_MONASTERY && isDone(b)) mon = &b; Bld& src = mon ? *mon : tcB();
  S.army[U_MONK]--; Explorer e{}; e.id = S.aid++; e.monk = true; e.maxHp = e.hp = 45.f; e.x = e.tx = src.x + src.w / 2.f; e.y = e.ty = src.y + src.h + 0.8f; e.face = 3; S.expl.push_back(e);
  toast("Monk sent out - tap the map, then tap a relic to fetch it"); return true;
}
// ---- prestige: once the Town Center is maxed you may ascend: pick a civilization (how your buildings look) and one reward for the new realm
const CivDef CIVS[NCIV] = {
  {"", "Byzantines", "Plastered walls, terracotta and blue slate roofs, stone castles with round towers."},
  {"sl", "Slavs", "Log cabins with carved trim, red-brick Kremlin walls, tent roofs and gleaming onion domes."},
  {"fr", "Franks", "Pale limestone, half-timbered houses, steep blue slate roofs and soaring spires."},
  {"ce", "Celts", "Round thatched houses, drystone towers, deep thatch roofs, standing stones and high crosses."},
  {"ja", "Japanese", "White plaster and dark timber under sweeping, upswept tile roofs with stone lanterns."},
  {"sp", "Spanish", "Whitewashed walls, sandstone castles, low terracotta roofs, domes and arched windows."},
  {"mo", "Mongols", "Felt gers and pavilion tents, painted lattice, spiked palisades and horsetail standards."},
};
const BoonDef BOONS[NBOON] = {
  {"No reward", ""},
  {"Bountiful Lands", "Resource production is 75% higher for your whole next realm."},
  {"Veteran's Return", "Troops that survive a raid march back to your Army Camp instead of being spent."},
  {"Silver Tongue", "A huge boost to diplomacy: rival lords agree to peace, trade and alliances far more often, never stay angry for long, and will ally with you straight after making peace."},
  {"Hardy Villagers", "Villagers have over three times the health and fight back: one villager can beat a wolf on his own."},
};
bool prestigeAvailable() { return tcLvl() >= MAXTC; }
bool outlineOwned() { return (S.meta.unlocked & UNLOCK_OUTLINE) != 0; }
bool outlineChoiceOpen(int prestigeAfter) { return prestigeAfter >= 2 && !outlineOwned(); }
const char* civCode(int civ) { return CIVS[clampi(civ, 0, NCIV - 1)].code; }
bool nodeVisible(const Node& n) { return n.type == N_TREE || tcLvl() >= 3; }
void rebalanceNodes() { for (auto& n : S.nodes) { float want = n.type == N_TREE ? (float)TREE_WOOD : n.type == N_GOLD ? (float)GOLD_AMT : (float)STONE_AMT; if (!homeRect((float)n.x, (float)n.y)) want = n.type == N_TREE ? (float)TREE_WOOD : n.type == N_GOLD ? GOLD_AMT * 1.4f : STONE_AMT * 1.5f; if (n.max < want) { float f = n.max > 0 ? n.amt / n.max : 1.f; n.max = want; n.amt = f * want; } } }
float gatherMul(int res) {
  int n = res == WOOD ? S.tech[T_WOOD1] + S.tech[T_WOOD2] + S.tech[T_WOOD3] : res == GOLD ? S.tech[T_GOLD1] + S.tech[T_GOLD2] + S.tech[T_GOLD3] : res == STONE ? S.tech[T_STONE1] + S.tech[T_STONE2] + S.tech[T_STONE3] : S.tech[T_FARM1] + S.tech[T_FARM2] + S.tech[T_FARM3];
  return (1.f + 0.25f * n) * (S.meta.boon == BOON_PROSPER ? 1.75f : 1.f);
}   // older saves: refill trees to the new size, keeping how much is left
void topUpNodes() {
  int tc = tcLvl(); int want[3] = {std::min(120, 48 + 10 * (tc - 1)), tc >= 3 ? std::min(22, 5 + 2 * (tc - 3)) : 0, tc >= 3 ? std::min(20, 5 + 2 * (tc - 3)) : 0};
  for (int ty = 0; ty < 3; ty++) {
    int have = 0; for (auto& n : S.nodes) if ((int)n.type == ty && homeRect((float)n.x, (float)n.y)) have++;
    float amt = ty == N_TREE ? (float)TREE_WOOD : ty == N_GOLD ? (float)GOLD_AMT : (float)STONE_AMT;
    for (int guard = 0; have < want[ty] && guard < 60; guard++) { Node nn{0, 0, (NodeType)ty, amt, amt, 0}; if (!findNodeSpot(nn)) break; S.nodes.push_back(nn); have++; }
  }
}
int rollTcDiff(Rng& r) { double u = r.next(); return u < 0.14 ? -2 : u < 0.36 ? -1 : u < 0.64 ? 0 : u < 0.86 ? 1 : 2; }

// ===================================================== barbarian camps
float campMul(int c) { int w = (c >= 1 && c <= NCAMP) ? S.campWins[c] : 0; return w <= 0 ? 1.f : w == 1 ? 0.5f : w == 2 ? 0.25f : 0.1f; }
int campsUnlocked() { int tc = tcLvl(); return std::min(NCAMP, 3 + 4 * (tc - 1) + (tc >= MAXTC ? 1 : 0)); }
const Base& campBase(int c) {
  static std::vector<Base> cache; static std::vector<char> have;
  if (cache.empty()) { cache.resize(NCAMP + 1); have.assign(NCAMP + 1, 0); }
  c = clampi(c, 1, NCAMP); if (!have[c]) { cache[c] = genBase(1000 + c, 1, true, c); have[c] = 1; } return cache[c];
}

void econTick(float dt) {
  double t = S.t;
  for (auto& b : S.b) if (b.building && t >= b.bEnd) completeBuild(b);
  for (auto& n : S.nodes) if (n.amt <= 0 && t >= n.respawn) relocateNode(n);
  worldTick(dt);
  if (S.researchTech >= 0 && t >= S.researchEnd) { S.tech[S.researchTech]++; sfx(2); toast(std::string(techdef(S.researchTech).name) + " researched"); S.researchTech = -1; }
  int over = idleV();
  if (over < 0) { int k = -over; for (auto& b : S.b) { int m = std::min(b.workers, k); b.workers -= m; k -= m; if (!k) break; } }
  for (auto& b : S.b) {
    const BDef& d = bdef(b.type);
    if (d.res < 0 || !isDone(b)) { b.status = 0; continue; }
    if (!b.workers) { b.status = 1; continue; }
    if (S.res[d.res] >= cap()) { b.status = 3; continue; }
    float amount = b.workers * d.rate * (1.f + 0.15f * (b.lvl - 1)) * gatherMul(d.res) * dt / 60.f;
    if (d.node == N_GAME) { huntTick(b, d, amount, dt); continue; }
    if (d.node >= 0) {
      float cx = b.x + b.w / 2.f, cy = b.y + b.h / 2.f;
      std::vector<Node*> ns;
      for (auto& n : S.nodes) if (n.type == d.node && n.amt > 0 && std::hypot(n.x + .5f - cx, n.y + .5f - cy) <= 7) ns.push_back(&n);
      if (ns.empty()) { b.status = 4; continue; }
      std::sort(ns.begin(), ns.end(), [&](Node* a, Node* c) { return std::hypot(a->x - cx, a->y - cy) < std::hypot(c->x - cx, c->y - cy); });
      float got = 0;
      for (Node* n : ns) { float take = std::min(n->amt, amount - got); n->amt -= take; got += take; if (n->amt <= 0) n->respawn = (float)t + 120; if (got >= amount - 1e-6f) break; }
      amount = got;
    }
    b.status = 2; b.acc += amount;
    int whole = (int)std::floor(b.acc);
    if (whole) { b.acc -= whole; S.res[d.res] = std::min((float)cap(), S.res[d.res] + whole); }
  }
  if (!S.vq.empty()) { S.vq[0] -= dt; if (S.vq[0] <= 0 && S.villagers < vCap()) { S.villagers++; S.vq.erase(S.vq.begin()); } else if (S.vq[0] < 0) S.vq[0] = 0; }
  for (auto& b : S.b) if (!b.q.empty()) { b.q[0].left -= dt; if (b.q[0].left <= 0) { S.army[b.q[0].type]++; b.q.erase(b.q.begin()); } }
}

Cost villagerCost() { int n = S.villagers + (int)S.vq.size(); return mkc(0, 50 + 12 * std::max(0, n - 3) + (n >= 8 ? 6 * (n - 7) : 0)); }   // food only
float villagerTime() { int n = S.villagers + (int)S.vq.size(); return 12.f + std::min(20.f, n * 0.25f); }
void trainVillager() {
  if (S.villagers + (int)S.vq.size() >= vCap()) { toast("Build more Houses for villagers"); return; }
  Cost c = villagerCost(); if (!canAfford(c)) { toast("Need " + std::to_string(c.v[FOOD]) + " food" + (c.v[WOOD] ? " and " + std::to_string(c.v[WOOD]) + " wood" : std::string())); return; }
  pay(c); S.vq.push_back(villagerTime());
}
void trainUnit(Bld& b, UType t) {
  const UDef& d = udef(t);
  if (armyCount() + fieldPop() + queuedPop() + d.pop > housing()) { toast("Army housing full - build an Army Camp"); return; }
  if (b.q.size() >= 8) { toast("Queue full"); return; }
  if (!canAfford(d.cost)) { toast("Not enough resources"); return; }
  pay(d.cost); b.q.push_back({t, (float)d.time});
}
Cost presetCost(const int c[U_COUNT]) { Cost t; for (int i = 0; i < U_COUNT; i++) for (int k = 0; k < NRES; k++) t.v[k] += udef(i).cost.v[k] * c[i]; return t; }
int presetPop(const int c[U_COUNT]) { int n = 0; for (int i = 0; i < U_COUNT; i++) n += udef(i).pop * c[i]; return n; }
void queuePreset(int slot) {
  if (slot < 0 || slot >= NPRESET) return;
  int want = 0, done = 0; std::string why;
  for (int ty = 0; ty < U_COUNT && why.empty(); ty++) for (int k = 0; k < S.presets[slot][ty]; k++) {
    want++; const UDef& d = udef(ty);
    if (d.unlock > tcLvl()) { why = std::string(d.name) + " needs Town Center Lv" + std::to_string(d.unlock); break; }
    Bld* best = nullptr; for (auto& b : S.b) if (b.type == d.bld && isDone(b) && b.q.size() < 8 && (!best || b.q.size() < best->q.size())) best = &b;
    if (!best) { why = std::string("no free ") + bdef(d.bld).name + " queue"; break; }
    if (armyCount() + fieldPop() + queuedPop() + d.pop > housing()) { why = "army housing is full"; break; }
    if (!canAfford(d.cost)) { why = "not enough resources"; break; }
    pay(d.cost); best->q.push_back({(UType)ty, (float)d.time}); done++;
  }
  for (int ty = 0; ty < U_COUNT; ty++) want += 0;
  int total = 0; for (int ty = 0; ty < U_COUNT; ty++) total += S.presets[slot][ty];
  if (total == 0) { toast("This preset is empty - tap Edit"); return; }
  want = total;
  if (done == want) { toast("Queued " + std::to_string(done) + " units"); sfx(1); }
  else if (done == 0) toast("Couldn't queue anything: " + why);
  else { toast("Queued " + std::to_string(done) + " of " + std::to_string(want) + " - " + why); sfx(1); }
}
int buildCount(BType t) { int n = 0; for (auto& b : S.b) if (b.type == t) n++; return n; }
int buildCountAt(BType t, bool outside, int ignore) { int n = 0; for (auto& b : S.b) if (b.type == t && b.id != ignore) { float cx = b.x + b.w / 2.f, cy = b.y + b.h / 2.f; if (inHome(cx, cy) != outside) n++; } return n; }
const char* canBuildErr(BType t) {
  const BDef& d = bdef(t);
  static const char* need[11] = {"", "", "Needs Town Center Lv2", "Needs Town Center Lv3", "Needs Town Center Lv4", "Needs Town Center Lv5", "Needs Town Center Lv6", "Needs Town Center Lv7", "Needs Town Center Lv8", "Needs Town Center Lv9", "Needs Town Center Lv10"};
  if (d.unlock > tcLvl()) return need[clampi(d.unlock, 0, 10)];
  int mx = bmaxn(t, tcLvl()), mo = bmaxOut(t, tcLvl());
  if (buildCountAt(t, false) >= mx && buildCountAt(t, true) >= mo) return (mx == 0 && mo == 0) ? "Locked" : "Limit reached";
  return nullptr;
}
const char* placeBlockReason(BType t, int x, int y, int ignore, bool limits) {
  const BDef& d = bdef(t);
  if (x < -WEXT + 1 || y < -WEXT + 1 || x + d.w > WHI - 1 || y + d.h > WHI - 1) return "Outside the world";
  for (int i = x; i < x + d.w; i++) for (int j = y; j < y + d.h; j++) if (!isExplored(i, j)) return "Explore this area first";
  float cx = x + d.w / 2.f, cy = y + d.h / 2.f; bool inside = inHome(cx, cy); int g = homeG();
  if (inside && (x < -g || y < -g || x + d.w > MAP + g || y + d.h > MAP + g)) return "Keep it inside your village";
  if (tcLvl() < 4 && (x < -WEXT || y < -WEXT || x + d.w > QUAD - WEXT || y + d.h > QUAD - WEXT)) return "Town Center Lv4 is needed to build outside your quadrant";
  for (int q = 1; q < 4; q++) { float hx = (float)(QUAD * (q % 2)), hy = (float)(QUAD * (q / 2)); if (x < hx + MAP + 2 && x + d.w > hx - 2 && y < hy + MAP + 2 && y + d.h > hy - 2) return "That is another lord's home base"; }
  for (auto& b : S.b) if (b.id != ignore && x < b.x + b.w && x + d.w > b.x && y < b.y + b.h && y + d.h > b.y) return "Tile is occupied";
  for (auto& n : S.nodes) if (n.amt > 0 && nodeVisible(n) && n.x >= x && n.x < x + d.w && n.y >= y && n.y < y + d.h) return "Tile is occupied";
  if (limits) {
    if (inside) { if (buildCountAt(t, false, ignore) >= bmaxn(t, tcLvl())) return "Village limit reached for this building"; }
    else { int mo = bmaxOut(t, tcLvl()); if (mo == 0) return "That can't be built outside your village"; if (buildCountAt(t, true, ignore) >= mo) return "Outpost limit reached for this building"; }
  }
  return nullptr;
}
bool placeOk(BType t, int x, int y, int ignore) { return placeBlockReason(t, x, y, ignore, false) == nullptr; }
int workersOn(int res) { int n = 0; for (auto& b : S.b) if (isDone(b) && bdef(b.type).res == res) n += b.workers; return n; }
Cost moveCost(const Bld& b) { Cost c = bdef(b.type).cost; for (int i = 0; i < NRES; i++) c.v[i] = c.v[i] / 4; return c; }
bool moveBuilding(int id, int x, int y) {
  Bld* b = findB(id); if (!b || b->type == B_TOWNHALL || isPiece(b->type)) return false;
  if (b->building) { toast("Wait until it is finished"); return false; }
  if (const char* why = placeBlockReason(b->type, x, y, id, true)) { toast(why); return false; }
  Cost c = moveCost(*b); if (!canAfford(c)) { toast("Not enough resources"); return false; }
  pay(c); b->x = x; b->y = y; b->acc = 0; toast("Moved"); return true;
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
  if (d.minTc > tcLvl()) return d.minTc >= 8 ? "Needs the Imperial Age" : d.minTc >= 5 ? "Needs the Castle Age" : "Needs the Feudal Age";
  if (d.prereq >= 0 && S.tech[d.prereq] < 1) return "Research the previous upgrade first";
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
  if (x < -WEXT + 1 || y < -WEXT + 1 || x >= WHI - 1 || y >= WHI - 1) return "Outside the world";
  if (!isExplored(x, y)) return "Explore this area first";
  Bld* ex = pieceAt(x, y);
  bool outside = !inHome(x + .5f, y + .5f);
  if (t == B_GATE && ex && ex->type == B_WALL) { int cnt = buildCountAt(B_GATE, outside), lim = outside ? bmaxOut(B_GATE, tcLvl()) : bmaxn(B_GATE, tcLvl()); if (cnt >= lim) return outside ? "Outpost gate limit reached" : "Gate limit reached"; return nullptr; }
  { int cnt = buildCountAt(t, outside), lim = outside ? bmaxOut(t, tcLvl()) : bmaxn(t, tcLvl()); if (cnt >= lim) return outside ? (lim == 0 ? "That can't be built outside your village" : "Outpost limit reached") : "Limit reached"; }
  if (const char* why = placeBlockReason(t, x, y)) return why;
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

// ---- battle replays ----
Replay replayCapture(const Sim& init, int kind, float dt) {
  Replay r; r.kind = kind; r.dt = dt; r.defence = init.defence; r.b = init.b; r.sq = init.sq; r.mul[0] = init.mul[0]; r.mul[1] = init.mul[1]; r.when = (long long)std::time(nullptr);
  for (auto& u : init.units) r.units.push_back({(int)u.type, u.def, u.x, u.y});
  return r;
}
void replayPush(const Replay& r) { S.replays.push_front(r); while (S.replays.size() > 10) S.replays.pop_back(); }
void ReplayPlayer::start(const Replay& rp, Sim& s) {
  r = &rp; sim = &s; di = 0; step = 0; nonMonk = 0; acc = 0;
  Sim n; n.defence = rp.defence; n.started = rp.defence; n.mul[0] = rp.mul[0]; n.mul[1] = rp.mul[1]; n.b = rp.b; n.sq = rp.sq;
  for (auto& u : rp.units) n.deploy((UType)u.type, u.x, u.y, u.def);
  *sim = n;
  if (!rp.defence) sim->reserve = [this] { return r->reserve0 - nonMonk; };
}
void ReplayPlayer::stepOnce() {
  while (di < r->deploys.size() && r->deploys[di].step <= step) { const RDeploy& d = r->deploys[di++]; sim->deploy((UType)d.type, d.x, d.y); if (d.type != U_MONK) nonMonk++; }
  if (r->endStep >= 0 && step >= r->endStep) { sim->over = true; return; }
  sim->step(r->dt); step++;
}
void replayRun(const Replay& r, int& stars, int& pct, int maxSteps) {
  Sim s; ReplayPlayer p; p.start(r, s); int n = 0; while (!s.over && n++ < maxSteps) p.stepOnce(); stars = s.stars(); pct = s.pct();
}
static void fillDefenceReplay(Replay& rp, const Sim& sim, const Report& r, const char* title) {
  rp.title = title; rp.who = r.who; rp.stars = r.stars; rp.pct = r.pct; rp.dr = r.dr; rp.win = r.held; rp.loot = r.stolen; rp.tcMine = tcLvl(); rp.tcEnemy = sim.atkTc; rp.tcBuild = tcLvl();
}

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

static void edgePoint(Rng& r, float& x, float& y) {
  int s = (int)(r.next() * 4); float p = 2 + (float)r.next() * (SIMN - 4);
  if (s == 0) { x = p; y = 0.6f; } else if (s == 1) { x = p; y = SIMN - 0.6f; } else if (s == 2) { x = 0.6f; y = p; } else { x = SIMN - 0.6f; y = p; }
}
static float raidVariation(Rng& r) { float f = 0.55f + 0.9f * (float)r.next(); if (r.next() < 0.15) f += 0.4f + 0.4f * (float)r.next(); return f; }    // 0.55 .. 2.25, mean ~1.1
static int raidCount(int atkTc, float f) { float base = 3.f + atkTc * 2.2f + S.renown / 150.f; return clampi(roundi(base * f), 3, 60); }
static SideMul raidMul(int atkTc, float f) { float s = 1.f + 0.07f * (atkTc - 1) + std::min(0.7f, S.renown / 5000.f * 0.7f); s *= 0.8f + 0.2f * f; SideMul m; m.melee = m.ranged = m.siege = m.hp = s; return m; }
static void addRaiders(Sim& sim, Rng& r, int n, int tc) {
  float ex, ey; edgePoint(r, ex, ey);
  for (int i = 0; i < n; i++) {
    UType ty = tierUnit(tc, i); float x, y;
    if (r.next() < 0.7) { x = ex + (float)(r.next() - .5) * 3; y = ey + (float)(r.next() - .5) * 3; } else edgePoint(r, x, y);
    sim.sq.push_back({i * 0.5f, ty, clampf(x, .5f, SIMN - .5f), clampf(y, .5f, SIMN - .5f)});
  }
}
Sim makeDefenceSim() {
  Sim sim; sim.defence = true; sim.started = true;
  Rng r((uint32_t)S.t + S.seed);
  int diff = rollTcDiff(r), atkTc = clampi(tcLvl() + diff, 1, MAXTC); float var = raidVariation(r);
  std::vector<Enemy*> warLords; for (auto& en : S.enemies) if (en.stance == ST_WAR) warLords.push_back(&en);
  if (!warLords.empty() && r.next() < 0.9) { Enemy& src = *warLords[(size_t)(r.next() * warLords.size()) % warLords.size()]; atkTc = src.tc; sim.atkEnemy = src.id; sim.atkName = src.name; }
  diff = atkTc - tcLvl();
  sim.atkTc = atkTc; sim.atkDiff = diff; sim.mul[0] = raidMul(atkTc, var); sim.mul[1] = techMul();
  float masonry = 1 + 0.12f * S.tech[T_MASONRY];
  for (auto& b : S.b) if (isDone(b) && !bOutside(b)) {          // outposts are raided separately (see outpostRaid)
    int lv = (b.type == B_WALL || b.type == B_GATE) ? S.wallLvl : (b.type == B_SPIKE || b.type == B_OIL) ? S.trapLvl : b.lvl;
    SimB sb = makeSimB(b.type, lv, b.x + SOFF, b.y + SOFF); sb.hp *= masonry; sb.maxhp *= masonry; sim.b.push_back(sb);      // the village sits SOFF tiles in from the corner of the battle map
  }
  addRaiders(sim, r, raidCount(atkTc, var), atkTc);
  for (auto& al : S.enemies) if (al.stance == ST_ALLY) {                                  // allies send troops to help defend the village
    int n = clampi(al.tc + 1, 2, 12); float c = SOFF + 15.f;
    for (int i = 0; i < n; i++) { UType t = tierUnit(al.tc, i); if (t == U_RAM || t == U_TREBUCHET || t == U_BOMBARD) t = U_LONGSWORD; float a = (float)(r.next() * 6.2831853), d = 4.f + (float)r.next() * 4.f; sim.deploy(t, c + std::cos(a) * d, c + std::sin(a) * d, true); }
  }
  sim.spawnGarrisons();
  return sim;
}
// Raiders hit a cluster of buildings outside the home area. Resolved instantly with the normal battle sim; what is destroyed is lost for good.
OutpostReport outpostRaid() {
  OutpostReport rp; std::vector<Bld*> out; for (auto& b : S.b) if (bOutside(b)) out.push_back(&b);
  if (out.empty()) return rp;
  rp.any = true; Rng r((uint32_t)(S.t * 13) + S.seed * 7u);
  Bld* pick = out[(size_t)(r.next() * out.size()) % out.size()];
  float cx = pick->x + pick->w / 2.f, cy = pick->y + pick->h / 2.f;
  int ox = SIMN / 2 - roundi(cx), oy = SIMN / 2 - roundi(cy);
  std::vector<Bld*> cl; for (Bld* b : out) if (std::hypot(b->x + b->w / 2.f - cx, b->y + b->h / 2.f - cy) <= 10.f) cl.push_back(b);
  int odiff = rollTcDiff(r), oTc = clampi(tcLvl() + odiff, 1, MAXTC); float ovar = raidVariation(r); std::string oName;
  { std::vector<Enemy*> wl; for (auto& en : S.enemies) if (en.stance == ST_WAR) wl.push_back(&en); if (!wl.empty() && r.next() < 0.9) { Enemy& src = *wl[(size_t)(r.next() * wl.size()) % wl.size()]; oTc = src.tc; oName = src.name; } }
  Sim sim; sim.defence = true; sim.started = true; sim.mul[0] = raidMul(oTc, ovar); sim.mul[1] = techMul();
  float masonry = 1 + 0.12f * S.tech[T_MASONRY]; std::vector<int> ids;
  for (Bld* b : cl) if (isDone(*b)) {
    int lv = (b->type == B_WALL || b->type == B_GATE) ? S.wallLvl : (b->type == B_SPIKE || b->type == B_OIL) ? S.trapLvl : b->lvl;
    SimB sb = makeSimB(b->type, lv, clampi(b->x + ox, 1, SIMN - 3), clampi(b->y + oy, 1, SIMN - 3)); sb.hp *= masonry; sb.maxhp *= masonry; sim.b.push_back(sb); ids.push_back(b->id);
  }
  {                                                                   // guards: each tower / keep / bombard tower has a garrison that comes out to fight (the older the age, the better)
    UType gt = tcLvl() >= 8 ? U_CHAMPION : tcLvl() >= 5 ? U_LONGSWORD : tcLvl() >= 3 ? U_MANATARMS : U_MILITIA; int n0 = (int)sim.b.size();
    for (int i = 0; i < n0; i++) { const SimB& sb = sim.b[i]; int cnt = sb.type == B_TOWER ? 1 : sb.type == B_KEEP ? 2 : sb.type == B_BOMBARD ? 1 : 0; for (int k = 0; k < cnt; k++) sim.deploy(gt, sb.x + sb.w / 2.f + (k ? 1.2f : -1.2f), sb.y + sb.h + 0.9f, true); }
  }
  std::vector<std::pair<int, std::vector<int>>> sqIds;                       // field squads near the outpost fight too (their survivors stay alive)
  for (auto& q : S.squads) if (std::hypot(q.x - cx, q.y - cy) <= 12.f) { std::vector<int> ids2; for (size_t k = 0; k < q.u.size(); k++) { float a = (float)k * 2.399f; UType t = (UType)q.u[k].type; sim.deploy(t, SIMN / 2.f + std::cos(a) * (1.5f + k * 0.15f), SIMN / 2.f + 3.5f + std::sin(a) * 1.2f, true); if (!sim.units.empty()) { sim.units.back().hp = std::min(sim.units.back().hp, q.u[k].hp); ids2.push_back(sim.units.back().id); } else ids2.push_back(-1); } sqIds.push_back({q.id, ids2}); }
  char wh[64]; std::snprintf(wh, sizeof wh, "near (%d, %d)", roundi(cx), roundi(cy)); rp.where = wh;
  bool anyDead = false;
  if (!sim.b.empty()) {
    addRaiders(sim, r, std::max(3, roundi(raidCount(oTc, ovar) * 0.7f)), oTc);
    Replay orp = replayCapture(sim, 3, 0.05f);
    int n = 0; while (!sim.over && n++ < 20000) sim.step(0.05f);
    { orp.title = "Raid on your outpost"; orp.who = oName.empty() ? "Raiders (Town Center Lv" + std::to_string(oTc) + ")" : oName + " (Town Center Lv" + std::to_string(oTc) + ")"; orp.stars = 0; orp.pct = sim.pct(); orp.dr = 0; orp.tcMine = tcLvl(); orp.tcEnemy = oTc; orp.tcBuild = tcLvl(); bool anyLost = false; for (auto& sb : sim.b) if (sb.dead) anyLost = true; orp.win = !anyLost; replayPush(orp); rp.replayed = true; }
    std::vector<int> lost;
    for (auto& pr : sqIds) if (Squad* q = findSquad(pr.first)) { std::vector<SqUnit> keep; for (size_t k = 0; k < q->u.size() && k < pr.second.size(); k++) { for (auto& su : sim.units) if (su.id == pr.second[k] && su.hp > 0) { keep.push_back({q->u[k].type, su.hp}); break; } } q->u = keep; }
    for (size_t i = 0; i < sim.b.size(); i++) if (sim.b[i].dead) lost.push_back(ids[i]); else rp.held++;
    for (int id : lost) { Bld* b = findB(id); if (!b) continue; rp.lostV += std::min(b->workers, S.villagers); S.villagers = std::max(0, S.villagers - b->workers); rp.lostB++; anyDead = true; for (auto it = S.b.begin(); it != S.b.end(); ++it) if (it->id == id) { S.b.erase(it); break; } }
  } else anyDead = true;
  if (anyDead) {                                                                   // unfinished buildings and explorers nearby are caught too
    for (Bld* b : cl) if (!isDone(*b)) { int id = b->id; rp.lostB++; for (auto it = S.b.begin(); it != S.b.end(); ++it) if (it->id == id) { S.b.erase(it); break; } }
    for (size_t i = 0; i < S.expl.size();) { Explorer& e = S.expl[i]; if (!homeRect(e.x, e.y) && std::hypot(e.x - cx, e.y - cy) <= 9.f) { if (e.vill) S.villagers = std::max(0, S.villagers - 1); if (e.carry) S.relics.push_back({S.aid++, e.x, e.y}); rp.lostE++; S.expl.erase(S.expl.begin() + i); } else i++; }
  }
  S.nextRaid = S.t + 200 + std::rand() % 120;
  return rp;
}
// shield after being raided: at least 10 minutes at Town Center 1, rising to several hours at the top, longer the more damage you took
double shieldSeconds(int tc, int stars) {
  static const int baseMin[MAXTC] = {10, 15, 25, 40, 60, 85, 110, 140, 165, 190}; static const double mult[4] = {1.0, 1.25, 1.6, 2.0};
  return baseMin[clampi(tc, 1, MAXTC) - 1] * 60.0 * mult[clampi(stars, 0, 3)];
}
Report applyDefence(const Sim& sim) {
  Report rp; rp.stars = sim.stars(); rp.pct = sim.pct();
  double u1 = (double)(std::rand() % 1000) / 1000.0;
  int opp = std::max(0, S.renown + roundi((u1 - 0.5) * 200)); double E = expectedScore(S.renown, opp);
  rp.who = sim.atkName.empty() ? lords()[std::rand() % 300].name : sim.atkName;
  bool ruined = sim.atkEnemy >= 0 && rp.pct >= 100;                     // a neighbouring lord razed everything: huge losses, and every outpost goes with it
  for (int k = 0; k < NRES; k++) {
    float lootable = std::max(0.f, ruined ? S.res[k] : S.res[k] - prot());      // a razed village loses 70% of everything, protection or not
    int v = (int)std::floor(lootable * (ruined ? 0.7f : 0.2f * rp.pct / 100)); rp.stolen.v[k] = v; S.res[k] -= v;
  }
  if (ruined) {
    rp.ruined = true;
    for (auto it = S.b.begin(); it != S.b.end();) { if (bOutside(*it)) { int w = std::min(it->workers, S.villagers); S.villagers -= w; rp.lostV += w; rp.lostB++; it = S.b.erase(it); } else ++it; }
    for (size_t i = 0; i < S.expl.size();) { Explorer& ex = S.expl[i]; if (!inHome(ex.x, ex.y)) { if (ex.vill) { S.villagers = std::max(0, S.villagers - 1); rp.lostV++; } else rp.lostU++; S.expl.erase(S.expl.begin() + i); } else i++; }
    for (size_t i = 0; i < S.squads.size();) { Squad& q = S.squads[i]; if (!inHome(q.x, q.y)) { rp.lostU += (int)q.u.size(); S.squads.erase(S.squads.begin() + i); } else i++; }
  }
  rp.held = rp.stars <= 1; if (rp.held) S.sDef++;
  rp.atkTc = sim.atkTc; rp.diff = sim.atkDiff;
  int dr = rp.held ? std::max(1, roundi(0.5 * 40 * (1 - E) * winFactor(rp.diff))) : -std::max(1, roundi(0.5 * 40 * E * loseFactor(rp.diff)));
  int before = S.renown; S.renown = std::max(0, S.renown + dr); rp.dr = S.renown - before;
  S.shieldUntil = S.t + shieldSeconds(tcLvl(), rp.stars); S.nextRaid = S.t + 200 + std::rand() % 120;
  return rp;
}
Report headlessRaid() {
  Sim sim = makeDefenceSim(); Replay rp = replayCapture(sim, 2, 0.05f);
  int n = 0; while (!sim.over && n++ < 20000) sim.step(0.05f);
  Report r = applyDefence(sim); fillDefenceReplay(rp, sim, r, "Raid on your village"); replayPush(rp); return r;
}

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
  o << "DOK14 " << S.t << ' ' << S.villagers << ' ' << S.renown << ' ' << S.shieldUntil << ' ' << S.nextRaid << ' ' << S.nid << ' ' << S.seed << ' ' << S.rivalSeq << ' ' << (int)S.seenIntro << ' ' << S.lastSeen << '\n';
  for (int i = 0; i < NRES; i++) o << S.res[i] << ' '; o << '\n';
  o << (int)U_COUNT << ' '; for (int i = 0; i < U_COUNT; i++) o << S.army[i] << ' '; o << '\n';
  o << NCAMP << ' '; for (int i = 0; i <= NCAMP; i++) o << S.camp[i] << ' '; for (int i = 0; i <= NCAMP; i++) o << S.campWins[i] << ' '; o << '\n';
  o << (int)T_COUNT << ' '; for (int i = 0; i < T_COUNT; i++) o << S.tech[i] << ' '; o << S.researchTech << ' ' << S.researchEnd << '\n';
  o << S.wallLvl << ' ' << S.trapLvl << '\n';
  o << S.season << ' ' << S.seasonStart << ' ' << S.sWins << ' ' << S.sDef << ' ' << (int)S.musicOn << ' ' << (int)S.sfxOn << '\n';
  o << S.vq.size() << ' '; for (float v : S.vq) o << v << ' '; o << '\n';
  o << S.b.size() << '\n';
  for (auto& b : S.b) { o << b.id << ' ' << b.type << ' ' << b.x << ' ' << b.y << ' ' << b.lvl << ' ' << b.workers << ' ' << (int)b.building << ' ' << b.bStart << ' ' << b.bEnd << ' ' << b.bTo << ' ' << b.acc << ' ' << b.q.size(); for (auto& q : b.q) o << ' ' << q.type << ' ' << q.left; o << '\n'; }
  o << S.nodes.size() << '\n';
  for (auto& n : S.nodes) o << n.x << ' ' << n.y << ' ' << n.type << ' ' << n.amt << ' ' << n.max << ' ' << n.respawn << '\n';
  o << "WORLD " << S.aid << ' ' << S.scoutStock << ' ' << S.scq.size() << ' '; for (float v : S.scq) o << v << ' '; for (int i = 0; i < 3; i++) o << S.patchX[i] << ' ' << S.patchY[i] << ' ' << S.patchN[i] << ' '; o << '\n';
  { std::string fs(S.fog.size(), '0'); for (size_t i = 0; i < S.fog.size(); i++) fs[i] = S.fog[i] ? '1' : '0'; o << fs << '\n'; }
  o << S.animals.size() << '\n'; for (auto& a : S.animals) o << a.id << ' ' << a.type << ' ' << a.x << ' ' << a.y << ' ' << a.hx << ' ' << a.hy << ' ' << a.food << ' ' << (int)a.alive << ' ' << a.respawn << '\n';
  o << S.chests.size() << '\n'; for (auto& c : S.chests) { o << c.x << ' ' << c.y << ' ' << (int)c.taken; for (int k = 0; k < NRES; k++) o << ' ' << c.c.v[k]; o << '\n'; }
  o << S.expl.size() << '\n'; for (auto& e : S.expl) o << e.id << ' ' << (int)e.vill << ' ' << e.x << ' ' << e.y << ' ' << e.tx << ' ' << e.ty << ' ' << (int)e.go << ' ' << (int)e.recall << ' ' << (int)e.autoX << ' ' << (int)e.monk << ' ' << (int)e.carry << '\n';
  o << S.replays.size() << '\n';
  for (auto& r : S.replays) {
    o << "RPL " << r.kind << ' ' << (int)r.win << ' ' << r.stars << ' ' << r.pct << ' ' << r.dr << ' ' << r.tcMine << ' ' << r.tcEnemy << ' ' << r.tcBuild << ' ' << (int)r.defence << ' ' << r.reserve0 << ' ' << r.endStep << ' ' << r.dt << ' ' << r.when << ' ' << r.lordColor;
    for (int k = 0; k < NRES; k++) o << ' ' << r.loot.v[k];
    for (int i = 0; i < 2; i++) o << ' ' << r.mul[i].melee << ' ' << r.mul[i].ranged << ' ' << r.mul[i].siege << ' ' << r.mul[i].hp;
    o << '\n' << r.title << '\n' << r.who << '\n';
    o << r.b.size() << '\n'; for (auto& b : r.b) o << (int)b.type << ' ' << b.lvl << ' ' << (int)b.x << ' ' << (int)b.y << ' ' << b.hp << ' ' << b.maxhp << '\n';
    o << r.units.size() << '\n'; for (auto& u : r.units) o << u.type << ' ' << (int)u.def << ' ' << u.x << ' ' << u.y << '\n';
    o << r.sq.size() << '\n'; for (auto& s : r.sq) o << s.t << ' ' << (int)s.type << ' ' << s.x << ' ' << s.y << '\n';
    o << r.deploys.size() << '\n'; for (auto& d : r.deploys) o << d.step << ' ' << d.type << ' ' << d.x << ' ' << d.y << '\n';
  }
  o << S.enemyCheck << ' ' << S.enemies.size() << '\n';
  for (auto& e : S.enemies) { o << e.id << ' ' << e.quad << ' ' << e.tc << ' ' << e.pers << ' ' << e.seed << ' ' << e.nextLvl << ' ' << e.dead.size(); for (int d : e.dead) o << ' ' << d; o << '\n' << e.name << '\n' << e.stance << ' ' << e.trust << ' ' << e.talkAt << ' ' << (int)e.seen << ' ' << e.seenTc << ' ' << e.seenDead << ' ' << e.color << '\n'; }
  o << NPRESET << '\n'; for (int p = 0; p < NPRESET; p++) { for (int i = 0; i < U_COUNT; i++) o << S.presets[p][i] << ' '; o << '\n'; }
  o << S.meta.prestige << ' ' << S.meta.points << ' ' << S.meta.unlocked << ' ' << S.relicsHeld << ' ' << S.betrayals << ' ' << S.relicCheck << ' ' << S.relics.size() << '\n';
  for (auto& rl : S.relics) o << rl.id << ' ' << rl.x << ' ' << rl.y << '\n';
  o << S.squads.size() << '\n'; for (auto& q : S.squads) { o << q.id << ' ' << q.x << ' ' << q.y << ' ' << q.tx << ' ' << q.ty << ' ' << (int)q.go << ' ' << (int)q.recall << ' ' << q.mode << ' ' << q.target << ' ' << q.gx << ' ' << q.gy << ' ' << q.u.size(); for (auto& u : q.u) o << ' ' << u.type << ' ' << u.hp; o << '\n'; }
  o << S.musicVol << ' ' << S.sfxVol << '\n';
  o << S.meta.civ << ' ' << S.meta.boon << ' ' << (S.meta.outlineOn ? 1 : 0) << '\n';
  std::ofstream f(savePath(), std::ios::binary | std::ios::trunc); f << o.str();
}
bool loadGame() {
  std::ifstream f(savePath(), std::ios::binary); if (!f) return false;
  std::string magic; f >> magic; if (magic != "DOK1" && magic != "DOK2" && magic != "DOK3" && magic != "DOK4" && magic != "DOK5" && magic != "DOK6" && magic != "DOK7" && magic != "DOK8" && magic != "DOK9" && magic != "DOK10" && magic != "DOK11" && magic != "DOK12" && magic != "DOK13" && magic != "DOK14") return false;
  bool dok14 = magic == "DOK14", dok13 = magic == "DOK13" || dok14, dok12 = magic == "DOK12" || dok13, dok11 = magic == "DOK11" || dok12, dok10 = magic == "DOK10" || dok11, dok9 = magic == "DOK9" || dok10, dok8 = magic == "DOK8" || dok9, dok7 = magic == "DOK7" || dok8; if (dok7) magic = "DOK6";
  int armyN = magic == "DOK1" ? 3 : 9;
  State n; int seen = 0;
  f >> n.t >> n.villagers >> n.renown >> n.shieldUntil >> n.nextRaid >> n.nid >> n.seed >> n.rivalSeq >> seen >> n.lastSeen; n.seenIntro = seen;
  for (int i = 0; i < NRES; i++) f >> n.res[i];
  if (magic == "DOK3" || magic == "DOK4" || magic == "DOK5" || magic == "DOK6") f >> armyN;
  for (int i = 0; i < armyN; i++) { int v; f >> v; if (i < U_COUNT) n.army[i] = v; }
  if (magic == "DOK6") { int nc = 0; f >> nc; for (int i = 0; i <= nc; i++) { double v; f >> v; if (i <= NCAMP) n.camp[i] = v; } for (int i = 0; i <= nc; i++) { int v; f >> v; if (i <= NCAMP) n.campWins[i] = v; } }
  else for (int i = 0; i < 6; i++) f >> n.camp[i];
  if (magic != "DOK1") { int techN = 5; if (magic == "DOK3" || magic == "DOK4" || magic == "DOK5" || magic == "DOK6") f >> techN; for (int i = 0; i < techN; i++) { int v; f >> v; if (i < T_COUNT) n.tech[i] = v; } f >> n.researchTech >> n.researchEnd; }
  if (magic == "DOK4" || magic == "DOK5" || magic == "DOK6") f >> n.wallLvl >> n.trapLvl;
  if (magic == "DOK5" || magic == "DOK6") { int mo, so; f >> n.season >> n.seasonStart >> n.sWins >> n.sDef >> mo >> so; n.musicOn = mo; n.sfxOn = so; }
  size_t c; f >> c; for (size_t i = 0; i < c; i++) { float v; f >> v; n.vq.push_back(v); }
  f >> c; for (size_t i = 0; i < c; i++) {
    Bld b{}; int ty, bd; size_t qc; f >> b.id >> ty >> b.x >> b.y >> b.lvl >> b.workers >> bd >> b.bStart >> b.bEnd >> b.bTo >> b.acc >> qc;
    b.type = (BType)ty; b.building = bd; b.w = bdef(b.type).w; b.h = bdef(b.type).h;
    for (size_t k = 0; k < qc; k++) { int ut; float l; f >> ut >> l; b.q.push_back({(UType)ut, l}); }
    n.b.push_back(b);
  }
  f >> c; for (size_t i = 0; i < c; i++) { Node nd; int ty; f >> nd.x >> nd.y >> ty >> nd.amt >> nd.max >> nd.respawn; nd.type = (NodeType)ty; n.nodes.push_back(nd); }
  if (magic == "DOK6") {
    std::string tag; f >> tag; size_t sc = 0; f >> n.aid >> n.scoutStock >> sc; for (size_t i = 0; i < sc; i++) { float v; f >> v; n.scq.push_back(v); }
    for (int i = 0; i < 3; i++) f >> n.patchX[i] >> n.patchY[i] >> n.patchN[i];
    std::string fs; f >> fs; n.fog.assign((size_t)WORLD * WORLD, 0);
    if (fs.size() == 3600) { for (int yy = 0; yy < 60; yy++) for (int xx = 0; xx < 60; xx++) n.fog[(size_t)yy * WORLD + xx] = fs[(size_t)yy * 60 + xx] == '1'; }      // the old 60x60 world sits in the corner of the new one
    else for (size_t i = 0; i < fs.size() && i < n.fog.size(); i++) n.fog[i] = fs[i] == '1';
    size_t c2; f >> c2; for (size_t i = 0; i < c2; i++) { Animal a{}; int al; f >> a.id >> a.type >> a.x >> a.y >> a.hx >> a.hy >> a.food >> al >> a.respawn; a.alive = al; a.tx = a.x; a.ty = a.y; a.maxFood = a.type == A_DEER ? 140 : a.type == A_BOAR ? 220 : 90; a.hp = a.maxHp = a.type == A_DEER ? 50.f : a.type == A_BOAR ? 140.f : 80.f; a.wait = 2; n.animals.push_back(a); }
    f >> c2; for (size_t i = 0; i < c2; i++) { Chest ch{}; int tk; f >> ch.x >> ch.y >> tk; ch.taken = tk; for (int k = 0; k < NRES; k++) f >> ch.c.v[k]; n.chests.push_back(ch); }
    f >> c2; for (size_t i = 0; i < c2; i++) { Explorer e{}; int v, g2, rc, au; f >> e.id >> v >> e.x >> e.y >> e.tx >> e.ty >> g2 >> rc >> au; if (dok10) { int mk2, cr2; f >> mk2 >> cr2; e.monk = mk2; e.carry = cr2; } e.vill = v; e.go = g2; e.recall = rc; e.autoX = au; e.face = 3; n.expl.push_back(e); }
  }
  if (dok7) {
    size_t rc = 0; f >> rc;
    for (size_t i = 0; i < rc && f; i++) {
      Replay r; std::string tag; int win, def, k; f >> tag >> r.kind >> win >> r.stars >> r.pct >> r.dr >> r.tcMine >> r.tcEnemy >> r.tcBuild >> def >> r.reserve0 >> r.endStep >> r.dt >> r.when; if (dok12) f >> r.lordColor; r.win = win; r.defence = def;
      for (int q = 0; q < NRES; q++) f >> r.loot.v[q];
      for (int q = 0; q < 2; q++) f >> r.mul[q].melee >> r.mul[q].ranged >> r.mul[q].siege >> r.mul[q].hp;
      std::string line; std::getline(f, line); std::getline(f, r.title); std::getline(f, r.who);
      size_t c3; f >> c3; for (size_t j = 0; j < c3; j++) { int ty, lv, x, y; float hp, mh; f >> ty >> lv >> x >> y >> hp >> mh; SimB sb = makeSimB((BType)ty, lv, x, y); sb.hp = hp; sb.maxhp = mh; r.b.push_back(sb); }
      f >> c3; for (size_t j = 0; j < c3; j++) { RUnit u; int d2; f >> u.type >> d2 >> u.x >> u.y; u.def = d2; r.units.push_back(u); }
      f >> c3; for (size_t j = 0; j < c3; j++) { Spawn sp; int ty; f >> sp.t >> ty >> sp.x >> sp.y; sp.type = (UType)ty; r.sq.push_back(sp); }
      f >> c3; for (size_t j = 0; j < c3; j++) { RDeploy d; f >> d.step >> d.type >> d.x >> d.y; r.deploys.push_back(d); }
      (void)k; if (f) n.replays.push_back(r);
    }
  }
  if (dok9) {
    size_t ne = 0; f >> n.enemyCheck >> ne; std::string line; std::getline(f, line);
    for (size_t i = 0; i < ne && f; i++) { Enemy e; size_t nd = 0; f >> e.id >> e.quad >> e.tc >> e.pers >> e.seed >> e.nextLvl >> nd; for (size_t k = 0; k < nd; k++) { int d; f >> d; e.dead.push_back(d); } std::getline(f, line); std::getline(f, e.name); if (dok10) { int sn; f >> e.stance >> e.trust >> e.talkAt >> sn >> e.seenTc >> e.seenDead; if (dok12) f >> e.color; e.seen = sn; std::getline(f, line); } else { e.seen = true; e.seenTc = e.tc; e.seenDead = e.dead.size(); e.stance = (e.pers == 1 || e.pers == 3) ? ST_WAR : ST_PEACE; } n.enemies.push_back(e); }
  }
  if (dok9 && !dok12) { bool used[NLORDCOL] = {}; for (auto& e : n.enemies) { int c = 0; while (c < NLORDCOL - 1 && used[c]) c++; e.color = c; used[c] = true; } }      // older saves: give the lords their colours now
  if (dok8) { int np = 0, uc = U_COUNT; f >> np; for (int p = 0; p < np; p++) for (int i = 0; i < uc; i++) { int v; f >> v; if (p < NPRESET) n.presets[p][i] = v; } }
  if (dok10) { size_t nr = 0; f >> n.meta.prestige >> n.meta.points >> n.meta.unlocked >> n.relicsHeld >> n.betrayals >> n.relicCheck >> nr; for (size_t i = 0; i < nr; i++) { Relic rl; f >> rl.id >> rl.x >> rl.y; n.relics.push_back(rl); } }
  if (dok11) { size_t ns = 0; f >> ns; for (size_t i = 0; i < ns && f; i++) { Squad q; int go, rc; size_t nu; f >> q.id >> q.x >> q.y >> q.tx >> q.ty >> go >> rc >> q.mode >> q.target >> q.gx >> q.gy >> nu; q.go = go; q.recall = rc; for (size_t k = 0; k < nu; k++) { SqUnit u; f >> u.type >> u.hp; q.u.push_back(u); } n.squads.push_back(q); } }
  if (dok13) f >> n.musicVol >> n.sfxVol;
  if (dok14) { int oo = 0; f >> n.meta.civ >> n.meta.boon >> oo; n.meta.outlineOn = oo != 0; n.meta.civ = clampi(n.meta.civ, 0, NCIV - 1); n.meta.boon = clampi(n.meta.boon, 0, NBOON - 1); }
  if (!f || n.b.empty()) return false;
  S = n;
  if (magic != "DOK6") genWorld(); else if (!dok9) genWorldOldSave();
  if (!dok9) S.replays.clear();        // the battle map frame changed: old replays cannot be shown
  if (!dok10) S.relicCheck = S.t + 600;
  g_fogDirty = true; revealHome(); LORDS.clear(); lordsBuilt = false; return true;
}
