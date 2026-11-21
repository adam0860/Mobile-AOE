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
  if (b.type == B_TOWNHALL) topUpNodes();
  sfx(b.type == B_TOWNHALL ? 13 : 2);
  if (b.type == B_TOWNHALL) banner(b.lvl == 3 ? "Feudal Age reached!" : b.lvl == 5 ? "Castle Age reached!" : b.lvl == 8 ? "Imperial Age reached!" : "Town Center Level " + std::to_string(b.lvl));
  else toast(std::string(d.name) + " ready");
}

// ===================================================== exploration world, hunting, regrowing deposits
bool g_fogDirty = true;
static Rng gRng(0x5eed1234u);
static float frnd() { return (float)gRng.next(); }
static bool homeRect(float x, float y, float m = 0) { return x >= -m && y >= -m && x < MAP + m && y < MAP + m; }
bool inWorld(float x, float y) { return x >= -WEXT && y >= -WEXT && x < MAP + WEXT && y < MAP + WEXT; }
bool isExplored(int x, int y) {
  if (x < -WEXT || y < -WEXT || x >= MAP + WEXT || y >= MAP + WEXT || S.fog.size() != (size_t)WORLD * WORLD) return false;
  return S.fog[(y + WEXT) * WORLD + (x + WEXT)] != 0;
}
void revealAround(float cx, float cy, float r) {
  if (S.fog.size() != (size_t)WORLD * WORLD) return;
  int x0 = (int)std::floor(cx - r), x1 = (int)std::ceil(cx + r), y0 = (int)std::floor(cy - r), y1 = (int)std::ceil(cy + r);
  for (int y = std::max(y0, -WEXT); y <= std::min(y1, MAP + WEXT - 1); y++) for (int x = std::max(x0, -WEXT); x <= std::min(x1, MAP + WEXT - 1); x++)
    if (std::hypot(x + .5f - cx, y + .5f - cy) <= r) { uint8_t& f = S.fog[(y + WEXT) * WORLD + (x + WEXT)]; if (!f) { f = 1; g_fogDirty = true; } }
}
static bool tileFreeForNode(int x, int y) {
  for (auto& n : S.nodes) if (n.x == x && n.y == y) return false;
  for (auto& b : S.b) if (x >= b.x - 1 && x < b.x + b.w + 1 && y >= b.y - 1 && y < b.y + b.h + 1) return false;
  return true;
}
static const float ANIMAL_FOOD[3] = {140, 220, 90};
static void addAnimal(int type, float x, float y) {
  Animal a{}; a.id = S.aid++; a.type = type; a.x = a.hx = a.tx = x; a.y = a.hy = a.ty = y; a.food = a.maxFood = ANIMAL_FOOD[type]; a.alive = true; a.wait = frnd() * 6; a.face = (int)(frnd() * 4); S.animals.push_back(a);
}
void genWorld() {
  S.fog.assign((size_t)WORLD * WORLD, 0); S.animals.clear(); S.chests.clear(); S.expl.clear(); S.scq.clear(); S.scoutStock = 0;
  for (int i = 0; i < 3; i++) { S.patchX[i] = S.patchY[i] = -1; S.patchN[i] = 0; }
  for (int y = -1; y <= MAP; y++) for (int x = -1; x <= MAP; x++) S.fog[(y + WEXT) * WORLD + (x + WEXT)] = 1;
  Rng r(S.seed ^ 0xC0FFEEu);
  auto outerCentre = [&](float margin, float& cx, float& cy) {
    for (int k = 0; k < 200; k++) {
      cx = -WEXT + 3 + (float)r.next() * (WORLD - 6); cy = -WEXT + 3 + (float)r.next() * (WORLD - 6);
      if (!homeRect(cx, cy, margin)) return;
    }
  };
  auto patch = [&](NodeType t, int count, float rad, float amt, float margin) {
    float cx, cy; outerCentre(margin, cx, cy); int placed = 0;
    for (int k = 0; k < count * 8 && placed < count; k++) {
      int x = (int)std::floor(cx + (r.next() * 2 - 1) * rad), y = (int)std::floor(cy + (r.next() * 2 - 1) * rad);
      if (homeRect((float)x, (float)y, 2) || x < -WEXT + 1 || y < -WEXT + 1 || x >= MAP + WEXT - 1 || y >= MAP + WEXT - 1) continue;
      bool dup = false; for (auto& n : S.nodes) if (n.x == x && n.y == y) { dup = true; break; }
      if (dup) continue; S.nodes.push_back({x, y, t, amt, amt, 0}); placed++;
    }
  };
  for (int i = 0; i < 16; i++) patch(N_TREE, 20, 3.4f, 120, 3);
  for (int i = 0; i < 7; i++) patch(N_GOLD, 5, 1.8f, 700, 3);
  for (int i = 0; i < 6; i++) patch(N_STONE, 5, 1.8f, 600, 3);
  for (int i = 0; i < 10; i++) { float cx, cy; outerCentre(2, cx, cy); int n = 3 + (int)(r.next() * 3); for (int k = 0; k < n; k++) addAnimal(A_DEER, cx + (float)(r.next() - .5) * 3, cy + (float)(r.next() - .5) * 3); }
  for (int i = 0; i < 8; i++) { float cx, cy; outerCentre(3, cx, cy); int n = 1 + (int)(r.next() * 2); for (int k = 0; k < n; k++) addAnimal(A_BOAR, cx + (float)(r.next() - .5) * 2, cy + (float)(r.next() - .5) * 2); }
  for (int i = 0; i < 7; i++) { float cx, cy; outerCentre(7, cx, cy); int n = 2 + (int)(r.next() * 2); for (int k = 0; k < n; k++) addAnimal(A_WOLF, cx + (float)(r.next() - .5) * 2.4f, cy + (float)(r.next() - .5) * 2.4f); }
  for (int i = 0; i < 14; i++) {
    float cx, cy; outerCentre(2, cx, cy); Chest c{(int)std::floor(cx), (int)std::floor(cy), {}, false};
    int k = (int)(r.next() * 4), k2 = (k + 1 + (int)(r.next() * 3)) % 4; c.c.v[k] = 250 + (int)(r.next() * 450); c.c.v[k2] = 120 + (int)(r.next() * 300); if (k == GOLD || k2 == GOLD) c.c.v[GOLD] += 100;
    S.chests.push_back(c);
  }
  g_fogDirty = true;
}
// A depleted deposit regrows somewhere else (so camps have to be demolished and rebuilt next to the new patch).
static bool findNodeSpot(Node& n);
static void relocateNode(Node& n) {
  bool outer = !homeRect((float)n.x, (float)n.y);
  if (outer) {
    for (int k = 0; k < 200; k++) {
      int x = -WEXT + 1 + (int)(frnd() * (WORLD - 2)), y = -WEXT + 1 + (int)(frnd() * (WORLD - 2));
      if (homeRect((float)x, (float)y, 2) || !tileFreeForNode(x, y)) continue;
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
      int cx = 4 + (int)(frnd() * (MAP - 8)), cy = 4 + (int)(frnd() * (MAP - 8));
      if (std::hypot(cx - (tc.x + 1.5f), cy - (tc.y + 1.5f)) < 5.5f) continue;
      int freeCnt = 0; for (int dx = -3; dx <= 3; dx++) for (int dy = -3; dy <= 3; dy++) { int x = cx + dx, y = cy + dy; if (x >= 2 && y >= 2 && x < MAP - 2 && y < MAP - 2 && tileFreeForNode(x, y)) freeCnt++; }
      if (freeCnt < 6) continue;
      float d = 99; for (auto& b : S.b) if (b.type == camp) d = std::min(d, (float)std::hypot(cx - (b.x + b.w / 2.f), cy - (b.y + b.h / 2.f)));
      d = std::min(d, 12.f) + frnd() * 2.f + freeCnt * 0.15f; if (d > bestScore) { bestScore = d; bx = cx; by = cy; }
    }
    S.patchX[ti] = bx; S.patchY[ti] = by; S.patchN[ti] = 0;
  }
  for (int k = 0; k < 120; k++) {
    float rad = 3.2f + k * 0.03f; int x = (int)std::floor(S.patchX[ti] + (frnd() * 2 - 1) * rad), y = (int)std::floor(S.patchY[ti] + (frnd() * 2 - 1) * rad);
    if (x < 2 || y < 2 || x >= MAP - 2 || y >= MAP - 2 || !tileFreeForNode(x, y)) continue;
    n.x = x; n.y = y; n.amt = n.max; S.patchN[ti]++; return true;
  }
  for (int k = 0; k < 300; k++) {                                // the patch is full: any free spot in the village
    int x = 2 + (int)(frnd() * (MAP - 4)), y = 2 + (int)(frnd() * (MAP - 4));
    if (tileFreeForNode(x, y)) { n.x = x; n.y = y; n.amt = n.max; return true; }
  }
  return false;      // no space anywhere
}
static void animalSpot(Animal& a) {
  for (int k = 0; k < 80; k++) {
    float x = -WEXT + 1 + frnd() * (WORLD - 2), y = -WEXT + 1 + frnd() * (WORLD - 2);
    if (homeRect(x, y, 3)) continue;
    if (a.type == A_WOLF && homeRect(x, y, 7)) continue;
    bool hidden = !isExplored((int)std::floor(x), (int)std::floor(y)); if (!hidden && k < 40) continue;      // prefer unexplored land
    a.x = a.hx = a.tx = x; a.y = a.hy = a.ty = y; return;
  }
}
static void worldTick(float dt) {
  double t = S.t;
  for (auto& a : S.animals) {
    if (!a.alive) { if (t >= a.respawn) { animalSpot(a); a.food = a.maxFood; a.alive = true; } continue; }
    float sp = a.type == A_DEER ? 1.0f : a.type == A_BOAR ? 0.8f : 1.1f;
    float dx = a.tx - a.x, dy = a.ty - a.y, L = std::hypot(dx, dy);
    if (L > 0.08f) { float s = std::min(sp * dt, L); a.x += dx / L * s; a.y += dy / L * s; a.aph += s / 0.7f; a.face = std::fabs(dx) > std::fabs(dy) ? (dx > 0 ? 0 : 2) : (dy > 0 ? 1 : 3); }
    else { a.wait -= dt; if (a.wait <= 0) { a.wait = 3 + frnd() * 7; a.tx = a.hx + (frnd() * 2 - 1) * 3.2f; a.ty = a.hy + (frnd() * 2 - 1) * 3.2f; } }
  }
  if (!S.scq.empty()) { S.scq[0] -= dt; if (S.scq[0] <= 0) { S.scq.erase(S.scq.begin()); S.scoutStock++; toast("A scout is ready - open Explore"); sfx(2); } }
  for (size_t i = 0; i < S.expl.size();) {
    Explorer& e = S.expl[i]; bool kill = false, home = false;
    float sp = e.vill ? 1.5f : 3.6f;
    if (e.go) {
      float dx = e.tx - e.x, dy = e.ty - e.y, L = std::hypot(dx, dy);
      if (L < 0.12f) { e.go = false; if (e.recall) home = true; }
      else { float s = std::min(sp * dt, L); e.x += dx / L * s; e.y += dy / L * s; e.aph += s / (e.vill ? 0.95f : 1.9f); e.face = (int)std::lround(std::atan2(-dy, dx) / 0.785398f); e.face = ((e.face % 8) + 8) % 8; }
    } else if (e.autoX) {
      float bd = 1e9f; int bx = 0, by = 0; bool found = false;
      for (int y = -WEXT + 1; y < MAP + WEXT - 1; y++) for (int x = -WEXT + 1; x < MAP + WEXT - 1; x++) {
        if (isExplored(x, y) || !(isExplored(x + 1, y) || isExplored(x - 1, y) || isExplored(x, y + 1) || isExplored(x, y - 1))) continue;
        float d = std::hypot(x + .5f - e.x, y + .5f - e.y) + frnd() * 3.f; if (d < bd) { bd = d; bx = x; by = y; found = true; }
      }
      if (found) { e.tx = bx + .5f; e.ty = by + .5f; e.go = true; } else { e.autoX = false; toast("Everything reachable is explored"); }
    }
    revealAround(e.x, e.y, e.vill ? 4.2f : 6.5f);
    for (auto& c : S.chests) if (!c.taken && std::hypot(c.x + .5f - e.x, c.y + .5f - e.y) < 1.1f) {
      c.taken = true; for (int k = 0; k < NRES; k++) S.res[k] = std::min((float)cap(), S.res[k] + c.c.v[k]); toast("Treasure found!"); sfx(3);
    }
    if (e.vill) for (auto& a : S.animals) if (a.alive && a.type == A_WOLF && std::hypot(a.x - e.x, a.y - e.y) < 2.1f) { kill = true; break; }
    if (kill) { S.villagers = std::max(0, S.villagers - 1); toast("A villager was killed by wolves!"); S.expl.erase(S.expl.begin() + i); continue; }
    if (home) { if (!e.vill) S.scoutStock++; S.expl.erase(S.expl.begin() + i); continue; }
    i++;
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
    int towers = 0; for (auto& o : S.b) if (isDone(o) && bdef(o.type).dps[0] > 0 && !isPiece(o.type) && std::hypot(o.x + o.w / 2.f - cx, o.y + o.h / 2.f - cy) <= 9.f) towers++;
    float lam = (cur->type == A_BOAR ? 1.f / 130 : 1.f / 100) * std::min(b.workers, 3) * dt * std::max(0.1f, std::pow(0.5f, (float)towers));
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
  Explorer e{}; e.id = S.aid++; e.vill = villager; e.x = e.tx = tc.x + 1.5f; e.y = e.ty = tc.y + 3.6f; e.face = 3; S.expl.push_back(e);
  toast(villager ? "Villager sent out - tap the map to choose where" : "Scout sent out - tap the map to choose where"); return true;
}
Explorer* findExplorer(int id) { for (auto& e : S.expl) if (e.id == id) return &e; return nullptr; }
void orderExplorer(int id, float x, float y) {
  Explorer* e = findExplorer(id); if (!e) return;
  e->tx = clampf(x, -WEXT + 0.5f, MAP + WEXT - 0.5f); e->ty = clampf(y, -WEXT + 0.5f, MAP + WEXT - 0.5f); e->go = true; e->recall = false; e->autoX = false;
}
void recallExplorer(int id) { Explorer* e = findExplorer(id); if (!e) return; Bld& tc = tcB(); e->tx = tc.x + 1.5f; e->ty = tc.y + 3.6f; e->go = true; e->recall = true; e->autoX = false; }
bool demolish(int id) {
  for (auto it = S.b.begin(); it != S.b.end(); ++it) if (it->id == id && it->type != B_TOWNHALL && !isPiece(it->type)) {
    const BDef& d = bdef(it->type); for (int i = 0; i < NRES; i++) S.res[i] = std::min((float)cap(), S.res[i] + d.cost.v[i] / 2);
    S.b.erase(it); return true;
  }
  return false;
}
static bool bOutside(const Bld& b) { float cx = b.x + b.w / 2.f, cy = b.y + b.h / 2.f; return !(cx >= 0 && cy >= 0 && cx < MAP && cy < MAP); }
int outsideBuildings() { int n = 0; for (auto& b : S.b) if (bOutside(b)) n++; return n; }

bool nodeVisible(const Node& n) { return n.type == N_TREE || tcLvl() >= 3; }
void topUpNodes() {
  int tc = tcLvl(); int want[3] = {std::min(120, 48 + 10 * (tc - 1)), tc >= 3 ? std::min(22, 5 + 2 * (tc - 3)) : 0, tc >= 3 ? std::min(20, 5 + 2 * (tc - 3)) : 0};
  for (int ty = 0; ty < 3; ty++) {
    int have = 0; for (auto& n : S.nodes) if ((int)n.type == ty && homeRect((float)n.x, (float)n.y)) have++;
    float amt = ty == N_TREE ? 130.f : ty == N_GOLD ? 500.f : 400.f;
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
    float amount = b.workers * d.rate * (1.f + 0.15f * (b.lvl - 1)) * dt / 60.f;
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

Cost villagerCost() { int n = S.villagers + (int)S.vq.size(); return mkc(n >= 8 ? 6 * (n - 7) : 0, 50 + 12 * std::max(0, n - 3)); }
float villagerTime() { int n = S.villagers + (int)S.vq.size(); return 12.f + std::min(20.f, n * 0.25f); }
void trainVillager() {
  if (S.villagers + (int)S.vq.size() >= vCap()) { toast("Build more Houses for villagers"); return; }
  Cost c = villagerCost(); if (!canAfford(c)) { toast("Need " + std::to_string(c.v[FOOD]) + " food" + (c.v[WOOD] ? " and " + std::to_string(c.v[WOOD]) + " wood" : std::string())); return; }
  pay(c); S.vq.push_back(villagerTime());
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
bool placeOk(BType t, int x, int y, int ignore) {
  const BDef& d = bdef(t);
  if (x < -WEXT + 1 || y < -WEXT + 1 || x + d.w > MAP + WEXT - 1 || y + d.h > MAP + WEXT - 1) return false;
  for (int i = x; i < x + d.w; i++) for (int j = y; j < y + d.h; j++) if (!isExplored(i, j)) return false;
  { float cx = x + d.w / 2.f, cy = y + d.h / 2.f; if (cx >= 0 && cy >= 0 && cx < MAP && cy < MAP && (x < 0 || y < 0 || x + d.w > MAP || y + d.h > MAP)) return false; }   // village buildings stay inside the village
  for (auto& b : S.b) if (b.id != ignore && x < b.x + b.w && x + d.w > b.x && y < b.y + b.h && y + d.h > b.y) return false;
  for (auto& n : S.nodes) if (n.amt > 0 && nodeVisible(n) && n.x >= x && n.x < x + d.w && n.y >= y && n.y < y + d.h) return false;
  return true;
}
int workersOn(int res) { int n = 0; for (auto& b : S.b) if (isDone(b) && bdef(b.type).res == res) n += b.workers; return n; }
Cost moveCost(const Bld& b) { Cost c = bdef(b.type).cost; for (int i = 0; i < NRES; i++) c.v[i] = c.v[i] / 4; return c; }
bool moveBuilding(int id, int x, int y) {
  Bld* b = findB(id); if (!b || b->type == B_TOWNHALL || isPiece(b->type)) return false;
  if (b->building) { toast("Wait until it is finished"); return false; }
  if (!placeOk(b->type, x, y, id)) { toast("Can't move it there"); return false; }
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
  if (x < -WEXT + 1 || y < -WEXT + 1 || x >= MAP + WEXT - 1 || y >= MAP + WEXT - 1) return "Outside the world";
  if (!isExplored(x, y)) return "Explore this area first";
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
  int s = (int)(r.next() * 4); float p = 2 + (float)r.next() * (MAP - 4);
  if (s == 0) { x = p; y = 0.6f; } else if (s == 1) { x = p; y = MAP - 0.6f; } else if (s == 2) { x = 0.6f; y = p; } else { x = MAP - 0.6f; y = p; }
}
static float raidVariation(Rng& r) { float f = 0.55f + 0.9f * (float)r.next(); if (r.next() < 0.15) f += 0.4f + 0.4f * (float)r.next(); return f; }    // 0.55 .. 2.25, mean ~1.1
static int raidCount(int atkTc, float f) { float base = 3.f + atkTc * 2.2f + S.renown / 150.f; return clampi(roundi(base * f), 3, 60); }
static SideMul raidMul(int atkTc, float f) { float s = 1.f + 0.07f * (atkTc - 1) + std::min(0.7f, S.renown / 5000.f * 0.7f); s *= 0.8f + 0.2f * f; SideMul m; m.melee = m.ranged = m.siege = m.hp = s; return m; }
static void addRaiders(Sim& sim, Rng& r, int n, int tc) {
  float ex, ey; edgePoint(r, ex, ey);
  for (int i = 0; i < n; i++) {
    UType ty = U_MILITIA;
    if (tc >= 8) ty = (i % 8 == 5) ? U_TREBUCHET : (tc >= 9 && i % 9 == 4) ? U_BOMBARD : (i % 5 == 1) ? U_PALADIN : (i % 3 == 0) ? ((i % 2) ? U_CANNONEER : U_ARBALEST) : (i % 2 == 0) ? U_CHAMPION : U_LONGSWORD;
    else if (tc >= 5) ty = (i % 7 == 3) ? U_RAM : (i % 5 == 1) ? U_KNIGHT : (i % 3 == 0) ? U_CROSSBOW : (i % 2 == 0) ? U_LONGSWORD : U_MANATARMS;
    else if (tc >= 3) ty = (i % 3 == 0) ? U_ARCHER : (i % 4 == 1) ? U_MANATARMS : U_MILITIA;
    float x, y;
    if (r.next() < 0.7) { x = ex + (float)(r.next() - .5) * 3; y = ey + (float)(r.next() - .5) * 3; } else edgePoint(r, x, y);
    sim.sq.push_back({i * 0.5f, ty, clampf(x, .5f, MAP - .5f), clampf(y, .5f, MAP - .5f)});
  }
}
Sim makeDefenceSim() {
  Sim sim; sim.defence = true; sim.started = true;
  Rng r((uint32_t)S.t + S.seed);
  int diff = rollTcDiff(r), atkTc = clampi(tcLvl() + diff, 1, MAXTC); diff = atkTc - tcLvl(); float var = raidVariation(r);
  sim.atkTc = atkTc; sim.atkDiff = diff; sim.mul[0] = raidMul(atkTc, var); sim.mul[1] = techMul();
  float masonry = 1 + 0.12f * S.tech[T_MASONRY];
  for (auto& b : S.b) if (isDone(b) && !bOutside(b)) {          // outposts are raided separately (see outpostRaid)
    int lv = (b.type == B_WALL || b.type == B_GATE) ? S.wallLvl : (b.type == B_SPIKE || b.type == B_OIL) ? S.trapLvl : b.lvl;
    SimB sb = makeSimB(b.type, lv, b.x, b.y); sb.hp *= masonry; sb.maxhp *= masonry; sim.b.push_back(sb);
  }
  addRaiders(sim, r, raidCount(atkTc, var), atkTc);
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
  int ox = 15 - roundi(cx), oy = 15 - roundi(cy);
  std::vector<Bld*> cl; for (Bld* b : out) if (std::hypot(b->x + b->w / 2.f - cx, b->y + b->h / 2.f - cy) <= 10.f) cl.push_back(b);
  int odiff = rollTcDiff(r), oTc = clampi(tcLvl() + odiff, 1, MAXTC); float ovar = raidVariation(r);
  Sim sim; sim.defence = true; sim.started = true; sim.mul[0] = raidMul(oTc, ovar); sim.mul[1] = techMul();
  float masonry = 1 + 0.12f * S.tech[T_MASONRY]; std::vector<int> ids;
  for (Bld* b : cl) if (isDone(*b)) {
    int lv = (b->type == B_WALL || b->type == B_GATE) ? S.wallLvl : (b->type == B_SPIKE || b->type == B_OIL) ? S.trapLvl : b->lvl;
    SimB sb = makeSimB(b->type, lv, clampi(b->x + ox, 1, MAP - 3), clampi(b->y + oy, 1, MAP - 3)); sb.hp *= masonry; sb.maxhp *= masonry; sim.b.push_back(sb); ids.push_back(b->id);
  }
  char wh[64]; std::snprintf(wh, sizeof wh, "near (%d, %d)", roundi(cx), roundi(cy)); rp.where = wh;
  bool anyDead = false;
  if (!sim.b.empty()) {
    addRaiders(sim, r, std::max(3, roundi(raidCount(oTc, ovar) * 0.7f)), oTc);
    Replay orp = replayCapture(sim, 3, 0.05f);
    int n = 0; while (!sim.over && n++ < 20000) sim.step(0.05f);
    { orp.title = "Raid on your outpost"; orp.who = "Raiders (Town Center Lv" + std::to_string(oTc) + ")"; orp.stars = 0; orp.pct = sim.pct(); orp.dr = 0; orp.tcMine = tcLvl(); orp.tcEnemy = oTc; orp.tcBuild = tcLvl(); bool anyLost = false; for (auto& sb : sim.b) if (sb.dead) anyLost = true; orp.win = !anyLost; replayPush(orp); rp.replayed = true; }
    std::vector<int> lost;
    for (size_t i = 0; i < sim.b.size(); i++) if (sim.b[i].dead) lost.push_back(ids[i]); else rp.held++;
    for (int id : lost) { Bld* b = findB(id); if (!b) continue; rp.lostV += std::min(b->workers, S.villagers); S.villagers = std::max(0, S.villagers - b->workers); rp.lostB++; anyDead = true; for (auto it = S.b.begin(); it != S.b.end(); ++it) if (it->id == id) { S.b.erase(it); break; } }
  } else anyDead = true;
  if (anyDead) {                                                                   // unfinished buildings and explorers nearby are caught too
    for (Bld* b : cl) if (!isDone(*b)) { int id = b->id; rp.lostB++; for (auto it = S.b.begin(); it != S.b.end(); ++it) if (it->id == id) { S.b.erase(it); break; } }
    for (size_t i = 0; i < S.expl.size();) { Explorer& e = S.expl[i]; if (!homeRect(e.x, e.y) && std::hypot(e.x - cx, e.y - cy) <= 9.f) { if (e.vill) S.villagers = std::max(0, S.villagers - 1); rp.lostE++; S.expl.erase(S.expl.begin() + i); } else i++; }
  }
  S.nextRaid = S.t + 200 + std::rand() % 120;
  return rp;
}
Report applyDefence(const Sim& sim) {
  Report rp; rp.stars = sim.stars(); rp.pct = sim.pct();
  double u1 = (double)(std::rand() % 1000) / 1000.0;
  int opp = std::max(0, S.renown + roundi((u1 - 0.5) * 200)); double E = expectedScore(S.renown, opp);
  rp.who = lords()[std::rand() % 300].name;
  for (int k = 0; k < NRES; k++) { float lootable = std::max(0.f, S.res[k] - prot()); int v = (int)std::floor(lootable * 0.2f * rp.pct / 100); rp.stolen.v[k] = v; S.res[k] -= v; }
  rp.held = rp.stars <= 1; if (rp.held) S.sDef++;
  rp.atkTc = sim.atkTc; rp.diff = sim.atkDiff;
  int dr = rp.held ? std::max(1, roundi(0.5 * 40 * (1 - E) * winFactor(rp.diff))) : -std::max(1, roundi(0.5 * 40 * E * loseFactor(rp.diff)));
  int before = S.renown; S.renown = std::max(0, S.renown + dr); rp.dr = S.renown - before;
  static const int sh[4] = {0, 60, 120, 180};
  S.shieldUntil = S.t + sh[rp.stars]; S.nextRaid = S.t + 200 + std::rand() % 120;
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
  o << "DOK7 " << S.t << ' ' << S.villagers << ' ' << S.renown << ' ' << S.shieldUntil << ' ' << S.nextRaid << ' ' << S.nid << ' ' << S.seed << ' ' << S.rivalSeq << ' ' << (int)S.seenIntro << ' ' << S.lastSeen << '\n';
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
  o << S.expl.size() << '\n'; for (auto& e : S.expl) o << e.id << ' ' << (int)e.vill << ' ' << e.x << ' ' << e.y << ' ' << e.tx << ' ' << e.ty << ' ' << (int)e.go << ' ' << (int)e.recall << ' ' << (int)e.autoX << '\n';
  o << S.replays.size() << '\n';
  for (auto& r : S.replays) {
    o << "RPL " << r.kind << ' ' << (int)r.win << ' ' << r.stars << ' ' << r.pct << ' ' << r.dr << ' ' << r.tcMine << ' ' << r.tcEnemy << ' ' << r.tcBuild << ' ' << (int)r.defence << ' ' << r.reserve0 << ' ' << r.endStep << ' ' << r.dt << ' ' << r.when;
    for (int k = 0; k < NRES; k++) o << ' ' << r.loot.v[k];
    for (int i = 0; i < 2; i++) o << ' ' << r.mul[i].melee << ' ' << r.mul[i].ranged << ' ' << r.mul[i].siege << ' ' << r.mul[i].hp;
    o << '\n' << r.title << '\n' << r.who << '\n';
    o << r.b.size() << '\n'; for (auto& b : r.b) o << (int)b.type << ' ' << b.lvl << ' ' << (int)b.x << ' ' << (int)b.y << ' ' << b.hp << ' ' << b.maxhp << '\n';
    o << r.units.size() << '\n'; for (auto& u : r.units) o << u.type << ' ' << (int)u.def << ' ' << u.x << ' ' << u.y << '\n';
    o << r.sq.size() << '\n'; for (auto& s : r.sq) o << s.t << ' ' << (int)s.type << ' ' << s.x << ' ' << s.y << '\n';
    o << r.deploys.size() << '\n'; for (auto& d : r.deploys) o << d.step << ' ' << d.type << ' ' << d.x << ' ' << d.y << '\n';
  }
  std::ofstream f(savePath(), std::ios::binary | std::ios::trunc); f << o.str();
}
bool loadGame() {
  std::ifstream f(savePath(), std::ios::binary); if (!f) return false;
  std::string magic; f >> magic; if (magic != "DOK1" && magic != "DOK2" && magic != "DOK3" && magic != "DOK4" && magic != "DOK5" && magic != "DOK6" && magic != "DOK7") return false;
  bool dok7 = magic == "DOK7"; if (dok7) magic = "DOK6";
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
    std::string fs; f >> fs; n.fog.assign((size_t)WORLD * WORLD, 0); for (size_t i = 0; i < fs.size() && i < n.fog.size(); i++) n.fog[i] = fs[i] == '1';
    size_t c2; f >> c2; for (size_t i = 0; i < c2; i++) { Animal a{}; int al; f >> a.id >> a.type >> a.x >> a.y >> a.hx >> a.hy >> a.food >> al >> a.respawn; a.alive = al; a.tx = a.x; a.ty = a.y; a.maxFood = a.type == A_DEER ? 140 : a.type == A_BOAR ? 220 : 90; a.wait = 2; n.animals.push_back(a); }
    f >> c2; for (size_t i = 0; i < c2; i++) { Chest ch{}; int tk; f >> ch.x >> ch.y >> tk; ch.taken = tk; for (int k = 0; k < NRES; k++) f >> ch.c.v[k]; n.chests.push_back(ch); }
    f >> c2; for (size_t i = 0; i < c2; i++) { Explorer e{}; int v, g2, rc, au; f >> e.id >> v >> e.x >> e.y >> e.tx >> e.ty >> g2 >> rc >> au; e.vill = v; e.go = g2; e.recall = rc; e.autoX = au; e.face = 3; n.expl.push_back(e); }
  }
  if (dok7) {
    size_t rc = 0; f >> rc;
    for (size_t i = 0; i < rc && f; i++) {
      Replay r; std::string tag; int win, def, k; f >> tag >> r.kind >> win >> r.stars >> r.pct >> r.dr >> r.tcMine >> r.tcEnemy >> r.tcBuild >> def >> r.reserve0 >> r.endStep >> r.dt >> r.when; r.win = win; r.defence = def;
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
  if (!f || n.b.empty()) return false;
  S = n; if (magic != "DOK6") genWorld(); g_fogDirty = true; LORDS.clear(); lordsBuilt = false; return true;
}
