// Dawn of Kings — game data definitions (C++17)
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

constexpr int TW = 64, TH = 32, MAP = 30, MAXLVL = 10, MAXTC = 10;
constexpr int WEXT = 15, WORLD = MAP + 2 * WEXT;   // the home area is 0..MAP; the explorable world reaches WEXT tiles further on every side
constexpr int NCAMP = 40;                          // barbarian camps
int campTier(int level);                           // strength tier (1..MAXTC) of a barbarian camp
int campUnlockTc(int level);                       // Town Center level needed for camp `level`

enum Res { WOOD, FOOD, GOLD, STONE, NRES };
// NOTE: order is part of the save format (player-buildable types first); append new types, never reorder.
enum BType {
  B_TOWNHALL, B_HOUSE, B_LUMBER, B_GOLDCAMP, B_STONECAMP, B_FARM, B_BARRACKS, B_ARMYCAMP, B_ARCHERY, B_TOWER,
  B_STABLE, B_SIEGE, B_MONASTERY, B_KEEP, B_BLACKSMITH, B_MARKET, B_CASTLE, B_UNIVERSITY, B_BOMBARD, B_WALL, B_GATE, B_SPIKE, B_OIL, B_SCORPION,
  B_STOREHOUSE, B_TENT, B_CHIEF, B_MILL, B_COUNT
};
enum UType { U_MILITIA, U_MANATARMS, U_ARCHER, U_LONGSWORD, U_CROSSBOW, U_KNIGHT, U_MONK, U_RAM, U_MANGONEL,
  U_CHAMPION, U_ARBALEST, U_PALADIN, U_CANNONEER, U_TREBUCHET, U_BOMBARD, U_COUNT };
enum NodeType { N_TREE, N_GOLD, N_STONE, N_GAME };   // N_GAME = hunted animals (Mill)
enum TechId { T_FORGING, T_ARMOR, T_FLETCH, T_MASONRY, T_SIEGE, T_CHEMISTRY, T_COUNT };

extern const char* RES_NAME[NRES];
extern const uint32_t RES_COL[NRES];

struct Cost {
  int v[NRES] = {0, 0, 0, 0};
  int& operator[](int i) { return v[i]; }
  int operator[](int i) const { return v[i]; }
};
inline Cost mkc(int w = 0, int f = 0, int g = 0, int s = 0) { Cost c; c.v[0] = w; c.v[1] = f; c.v[2] = g; c.v[3] = s; return c; }

struct BDef {
  const char* name; const char* tag; int w, h; uint32_t color; float hgt;
  int hp[3]; int dps[3]; float range; bool hq, noBuild;
  int unlock; int maxn[3]; Cost cost; int time; int cat; // cat: 0 eco,1 mil,2 def,-1
  int res; int node; int rate; int slots[3]; const char* desc;
};
struct UDef {
  const char* name; const char* tag; int hp, dmg; float rate, range, speed; Cost cost; int time, pop, unlock; BType bld; bool ranged; uint32_t color;
  // Castle Age additions
  float heal = 0;        // hp/s healed to nearby allies (monk)
  float splash = 0;      // area damage radius in tiles (mangonel)
  float arrowArmor = 1;  // multiplier on damage taken from arrows/bolts (<1 = resistant)
  bool prefDef = false;  // attacks defensive buildings first (knight, mangonel)
  bool siege = false;    // only attacks buildings (ram, mangonel)
  bool gun = false;      // gunpowder: ignores arrow resistance
};
struct TcUp { Cost cost; int time; const char* label; };
struct TechDef { const char* name; const char* desc; BType bld; int maxLvl; Cost base; int time; };

const BDef& bdef(int t);
const UDef& udef(int t);
const TechDef& techdef(int t);
const TcUp& tcup(int lvl);
// level-dependent stats (levels beyond the table grow geometrically)
int bhp(int type, int lvl);
int bdps(int type, int lvl);
int bmaxn(int type, int tc);
int bslots(int type, int lvl);
int capFor(int tc);
int protFor(int tc);
inline bool isPiece(int t) { return t == B_WALL || t == B_GATE || t == B_SPIKE || t == B_OIL; }   // 1x1 instant-build defences
const char* ageName(int tcLvl);
inline int ageOf(int l) { return l <= 2 ? 0 : l <= 4 ? 1 : l <= 7 ? 2 : 3; }
const char* rankOf(int renown);
int rankIdx(int renown);
double expectedScore(double my, double opp);
std::string fmtTime(double s);
inline float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }
inline int clampi(int v, int a, int b) { return std::max(a, std::min(b, v)); }
inline int roundi(double v) { return (int)std::floor(v + 0.5); }

struct Rng {
  uint32_t a;
  explicit Rng(uint32_t s = 1) : a(s) {}
  double next() {
    a += 0x6D2B79F5u; uint32_t t = a;
    t = (t ^ (t >> 15)) * (1u | t);
    t = (t + (t ^ (t >> 7)) * (61u | t)) ^ t;
    return ((t ^ (t >> 14))) / 4294967296.0;
  }
};

struct SimB {
  BType type; int lvl; float x, y; int w, h; float hp, maxhp, dps, range, cd; bool dead, hq;
  bool wall = false, gate = false, trap = false; float aux = 0; float deadT = 0;   // walls/gates block movement; traps trigger on contact
};
SimB makeSimB(BType type, int lvl, int x, int y);

struct Base { std::vector<SimB> buildings; Cost loot; std::string name; int pers = 0; };
Base genBase(uint32_t seed, int tc, bool barbarian, int level);

extern const char* LORD_FIRST[30];
extern const char* LORD_TITLE[10];
extern const char* PERSONALITY[4];
