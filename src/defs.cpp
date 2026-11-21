#include "defs.h"
#include <set>

const char* RES_NAME[NRES] = {"Wood", "Food", "Gold", "Stone"};
const uint32_t RES_COL[NRES] = {0x8a5a2b, 0xe6c84a, 0xf2c230, 0x9aa0a8};

// name, tag, w,h, color, hgt, hp[3], dps[3], range, hq, noBuild, unlock, maxn[3] (legacy), cost, time, cat, res, node, rate, slots[3], desc
static const BDef BD[B_COUNT] = {
  {"Town Center", "TC", 3, 3, 0xb5894f, 44, {1500, 1900, 2500}, {10, 13, 17}, 6, true, true, 1, {1, 1, 1}, {}, 0, -1, -1, -1, 0, {0, 0, 0}, "Heart of your village"},
  {"House", "H", 2, 2, 0xa8794a, 22, {250, 350, 450}, {0, 0, 0}, 0, false, false, 1, {2, 4, 6}, mkc(100), 8, 0, -1, -1, 0, {0, 0, 0}, "+5 villager capacity"},
  {"Lumber Camp", "L", 2, 2, 0x8a6a3a, 16, {300, 400, 500}, {0, 0, 0}, 0, false, false, 1, {1, 2, 3}, mkc(80), 8, 0, WOOD, N_TREE, 50, {3, 4, 5}, "Villagers chop nearby trees"},
  {"Gold Mining Camp", "G", 2, 2, 0x9a7d2e, 16, {300, 400, 500}, {0, 0, 0}, 0, false, false, 1, {1, 1, 2}, mkc(100), 10, 0, GOLD, N_GOLD, 32, {3, 4, 5}, "Villagers mine nearby gold"},
  {"Stone Mining Camp", "S", 2, 2, 0x7b7f86, 16, {300, 400, 500}, {0, 0, 0}, 0, false, false, 1, {1, 1, 2}, mkc(100), 10, 0, STONE, N_STONE, 30, {3, 4, 5}, "Villagers mine nearby stone"},
  {"Farm", "F", 2, 2, 0xc8b04a, 4, {150, 200, 250}, {0, 0, 0}, 0, false, false, 1, {2, 4, 6}, mkc(100), 8, 0, FOOD, -1, 40, {1, 1, 1}, "One farmer produces food"},
  {"Barracks", "B", 3, 3, 0x8c4a3a, 30, {500, 700, 900}, {0, 0, 0}, 0, false, false, 1, {1, 1, 2}, mkc(200, 0, 0, 50), 20, 1, -1, -1, 0, {0, 0, 0}, "Militia, Men-at-Arms, Long Swordsmen"},
  {"Army Camp", "A", 3, 3, 0x6f8a4a, 12, {300, 400, 500}, {0, 0, 0}, 0, false, false, 1, {1, 2, 3}, mkc(150), 15, 1, -1, -1, 0, {0, 0, 0}, "+20 army housing"},
  {"Archery Range", "R", 3, 3, 0x4a7a5a, 28, {500, 700, 900}, {0, 0, 0}, 0, false, false, 3, {0, 0, 1}, mkc(300, 0, 100), 25, 1, -1, -1, 0, {0, 0, 0}, "Trains Archers and Crossbowmen"},
  {"Watch Tower", "T", 2, 2, 0x8e8a80, 46, {300, 420, 560}, {12, 16, 21}, 6.5f, false, false, 1, {1, 2, 4}, mkc(100, 0, 0, 80), 15, 2, -1, -1, 0, {0, 0, 0}, "Fires arrows at raiders"},
  // ---- Castle Age ----
  {"Stable", "Sb", 3, 3, 0x7a5a3a, 30, {600, 800, 1000}, {0, 0, 0}, 0, false, false, 5, {0, 0, 1}, mkc(300, 0, 100), 30, 1, -1, -1, 0, {0, 0, 0}, "Trains Knights"},
  {"Siege Workshop", "Sw", 3, 3, 0x6a5a4a, 34, {600, 800, 1000}, {0, 0, 0}, 0, false, false, 5, {0, 0, 1}, mkc(400, 0, 150, 100), 40, 1, -1, -1, 0, {0, 0, 0}, "Builds Battering Rams and Mangonels"},
  {"Monastery", "Mo", 3, 3, 0xb0a58a, 40, {600, 800, 1000}, {0, 0, 0}, 0, false, false, 5, {0, 0, 1}, mkc(300, 0, 200, 150), 40, 1, -1, -1, 0, {0, 0, 0}, "Trains Monks who heal your troops"},
  {"Keep", "Kp", 2, 2, 0x8e8a80, 56, {1200, 1500, 1900}, {28, 34, 41}, 8.f, false, false, 5, {0, 0, 1}, mkc(200, 0, 100, 400), 45, 2, -1, -1, 0, {0, 0, 0}, "Strong stone tower, long range"},
  {"Blacksmith", "Bs", 2, 2, 0x5a4a44, 30, {500, 650, 800}, {0, 0, 0}, 0, false, false, 3, {0, 0, 1}, mkc(250, 0, 100, 100), 25, 1, -1, -1, 0, {0, 0, 0}, "Weapon and armor upgrades"},
  {"Market", "Mk", 3, 3, 0xa8794a, 30, {500, 650, 800}, {0, 0, 0}, 0, false, false, 4, {0, 0, 0}, mkc(300, 0, 100), 25, 0, -1, -1, 0, {0, 0, 0}, "Trade resources"},
  {"Castle", "Ca", 4, 4, 0x8d8c86, 70, {2000, 2600, 3300}, {0, 0, 0}, 0, false, false, 6, {0, 0, 0}, mkc(500, 0, 400, 800), 90, 2, -1, -1, 0, {0, 0, 0}, "Garrison: defenders sally out against raiders"},
  {"University", "Un", 3, 3, 0x9a9ab0, 46, {700, 900, 1100}, {0, 0, 0}, 0, false, false, 7, {0, 0, 0}, mkc(500, 0, 400, 300), 60, 1, -1, -1, 0, {0, 0, 0}, "Masonry and siege research"},
  // ---- Imperial Age ----
  {"Bombard Tower", "Bt", 2, 2, 0x8e8a80, 62, {1600, 2000, 2500}, {60, 72, 86}, 9.f, false, false, 8, {0, 0, 1}, mkc(400, 0, 300, 500), 60, 2, -1, -1, 0, {0, 0, 0}, "Cannon tower: heavy damage, pierces armor, splash"},
  // ---- walls & traps (1x1, built instantly, level shared per kind) ----
  {"Wall", "Wl", 1, 1, 0x8d8c86, 20, {120, 180, 450}, {0, 0, 0}, 0, false, false, 2, {0, 0, 0}, mkc(10), 0, 2, -1, -1, 0, {0, 0, 0}, "Drag to paint walls. Palisade, then stone, then fortified as the Wall level rises"},
  {"Gate", "Gt", 1, 1, 0x6a4a2a, 26, {400, 600, 1500}, {0, 0, 0}, 0, false, false, 3, {0, 0, 0}, mkc(150, 0, 0, 80), 0, 2, -1, -1, 0, {0, 0, 0}, "Tap a wall piece to turn it into a gate"},
  {"Spike Trap", "Sp", 1, 1, 0x7a6a50, 6, {100, 100, 100}, {100, 130, 170}, 1.4f, false, false, 4, {0, 0, 0}, mkc(80, 0, 40), 0, 2, -1, -1, 0, {0, 0, 0}, "Hidden spikes: heavy damage to nearby raiders once per raid"},
  {"Boiling Oil Trap", "Oi", 1, 1, 0x3a3a3a, 8, {100, 100, 100}, {35, 45, 58}, 1.7f, false, false, 6, {0, 0, 0}, mkc(0, 0, 200, 120), 0, 2, -1, -1, 0, {0, 0, 0}, "Scalds and slows raiders for several seconds"},
  {"Scorpion Emplacement", "Sc", 2, 2, 0x7a5a3a, 40, {800, 1000, 1250}, {45, 56, 69}, 8.f, false, false, 7, {0, 0, 1}, mkc(300, 0, 250, 300), 45, 2, -1, -1, 0, {0, 0, 0}, "Giant crossbow: one bolt pierces every raider in a line"},
  // ---- enemy-only ----
  {"Storehouse", "St", 2, 2, 0x9a7a4a, 18, {400, 600, 800}, {0, 0, 0}, 0, false, true, 1, {0, 0, 0}, {}, 0, -1, -1, -1, 0, {0, 0, 0}, ""},
  {"Barbarian Tent", "Tn", 2, 2, 0x7a5a3a, 16, {200, 200, 200}, {0, 0, 0}, 0, false, true, 1, {0, 0, 0}, {}, 0, -1, -1, -1, 0, {0, 0, 0}, ""},
  {"Chieftain's Hall", "Ch", 3, 3, 0x6a4a2a, 30, {700, 700, 700}, {0, 0, 0}, 0, true, true, 1, {0, 0, 0}, {}, 0, -1, -1, -1, 0, {0, 0, 0}, ""},
};

// buildings allowed per Town Center level (index = tc-1)
static const int MAXN[B_COUNT][MAXTC] = {
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},         // town center
  {2, 4, 6, 8, 10, 12, 14, 16, 18, 20},   // house
  {1, 2, 3, 3, 4, 4, 5, 5, 6, 6},         // lumber
  {1, 1, 2, 2, 3, 3, 4, 4, 5, 5},         // gold camp
  {1, 1, 2, 2, 3, 3, 4, 4, 5, 5},         // stone camp
  {2, 4, 6, 8, 10, 12, 14, 16, 18, 20},   // farm
  {1, 1, 2, 2, 3, 3, 3, 3, 4, 4},         // barracks
  {1, 2, 3, 4, 5, 6, 7, 8, 8, 8},         // army camp
  {0, 0, 1, 1, 2, 2, 2, 3, 3, 3},         // archery
  {1, 2, 4, 4, 6, 6, 8, 8, 10, 10},       // tower
  {0, 0, 0, 0, 1, 1, 2, 2, 2, 3},         // stable
  {0, 0, 0, 0, 1, 1, 1, 1, 2, 2},         // siege workshop
  {0, 0, 0, 0, 1, 1, 1, 1, 1, 1},         // monastery
  {0, 0, 0, 0, 1, 2, 3, 3, 4, 4},         // keep
  {0, 0, 1, 1, 1, 1, 1, 1, 1, 1},         // blacksmith
  {0, 0, 0, 1, 1, 1, 1, 1, 1, 1},         // market
  {0, 0, 0, 0, 0, 1, 1, 1, 1, 1},         // castle
  {0, 0, 0, 0, 0, 0, 1, 1, 1, 1},         // university
  {0, 0, 0, 0, 0, 0, 0, 1, 2, 3},         // bombard tower
  {0, 30, 60, 100, 140, 180, 220, 260, 300, 300},   // wall pieces
  {0, 0, 2, 2, 3, 3, 4, 4, 5, 6},          // gates
  {0, 0, 0, 4, 6, 8, 10, 12, 14, 16},      // spike traps
  {0, 0, 0, 0, 0, 2, 3, 4, 5, 6},          // oil traps
  {0, 0, 0, 0, 0, 0, 1, 2, 3, 4},          // scorpion emplacements
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

// Town Center upgrade costs (scaled down from the GDD for the prototype's faster pace)
static const TcUp TCU[11] = {
  {}, {},
  {mkc(400, 300), 30, "Upgrade to Level 2"},
  {mkc(1200, 800, 400), 90, "Advance to Feudal Age"},
  {mkc(1600, 1000, 400, 250), 150, "Upgrade to Level 4"},
  {mkc(3500, 2500, 1200, 800), 300, "Advance to Castle Age"},
  {mkc(7000, 5000, 3000, 2000), 500, "Upgrade to Level 6"},
  {mkc(14000, 10000, 6000, 4000), 800, "Upgrade to Level 7"},
  {mkc(24000, 16000, 10000, 6000), 1200, "Advance to Imperial Age"},
  {mkc(40000, 28000, 18000, 11000), 1800, "Upgrade to Level 9"},
  {mkc(64000, 44000, 30000, 20000), 2600, "Upgrade to Level 10"},
};
static const int CAP[MAXTC] = {2000, 4000, 6000, 9000, 16000, 28000, 45000, 70000, 100000, 140000};
static const int PROT[MAXTC] = {200, 400, 800, 1500, 3000, 5000, 8000, 12000, 18000, 25000};

//   name, tag, hp, dmg, rate, range, speed, cost, time, pop, unlock, bld, ranged, colour, heal, splash, arrowArmor, prefDef, siege
static const UDef UD[U_COUNT] = {
  {"Militia", "Mi", 60, 8, 1, 0.9f, 1.7f, mkc(0, 40, 20), 8, 1, 1, B_BARRACKS, false, 0xd9b45a},
  {"Man-at-Arms", "MA", 130, 12, 1, 0.9f, 1.5f, mkc(0, 60, 20), 12, 1, 3, B_BARRACKS, false, 0x9aa3b5},
  {"Archer", "Ar", 40, 7, 1, 5.0f, 1.7f, mkc(25, 0, 45), 10, 1, 3, B_ARCHERY, true, 0x5fae6a},
  {"Long Swordsman", "LS", 190, 17, 1, 0.9f, 1.5f, mkc(0, 70, 30), 14, 1, 5, B_BARRACKS, false, 0xb8c0d0},
  {"Crossbowman", "Cb", 55, 12, 1, 5.5f, 1.7f, mkc(30, 0, 60), 12, 1, 5, B_ARCHERY, true, 0x4c9a6a},
  {"Knight", "Kn", 240, 22, 1.1f, 1.0f, 3.0f, mkc(0, 80, 90), 22, 3, 5, B_STABLE, false, 0xd0d4e0, 0, 0, 0.8f, true, false},
  {"Monk", "Mk", 45, 0, 1, 0.9f, 1.5f, mkc(0, 0, 120), 20, 2, 5, B_MONASTERY, false, 0xe8e0d0, 9.f, 0, 1.f, false, false},
  {"Battering Ram", "Rm", 300, 45, 1.5f, 1.0f, 1.1f, mkc(180, 0, 80), 30, 5, 5, B_SIEGE, false, 0x8a6a40, 0, 0, 0.15f, false, true},
  {"Mangonel", "Mg", 130, 38, 3.0f, 7.0f, 1.0f, mkc(200, 0, 140), 34, 5, 5, B_SIEGE, true, 0x9a7a50, 0, 1.6f, 0.6f, true, true},
  // ---- Imperial Age ----
  {"Champion", "Ch", 330, 30, 1, 0.9f, 1.6f, mkc(0, 90, 60), 20, 2, 8, B_BARRACKS, false, 0xe0e4ee, 0, 0, 0.85f},
  {"Arbalester", "Ab", 75, 19, 1, 6.2f, 1.7f, mkc(35, 0, 75), 16, 1, 8, B_ARCHERY, true, 0x3f8a5a},
  {"Paladin", "Pl", 400, 34, 1.1f, 1.0f, 3.2f, mkc(0, 100, 130), 28, 3, 8, B_STABLE, false, 0xf0f0f8, 0, 0, 0.7f, true, false},
  {"Hand Cannoneer", "HC", 65, 42, 2.4f, 4.6f, 1.6f, mkc(60, 0, 100), 20, 2, 8, B_ARCHERY, true, 0x7a6a5a, 0, 0, 1.f, false, false, true},
  {"Trebuchet", "Tb", 190, 75, 5.0f, 10.0f, 0.9f, mkc(260, 0, 220), 40, 6, 8, B_SIEGE, true, 0x9a7a50, 0, 1.9f, 0.6f, true, true},
  {"Bombard Cannon", "Bc", 240, 100, 6.0f, 8.0f, 0.9f, mkc(280, 0, 280), 44, 6, 9, B_SIEGE, true, 0x55585c, 0, 1.5f, 0.6f, true, true, true},
};

static const TechDef TD[T_COUNT] = {
  {"Forging", "+10% melee damage per level", B_BLACKSMITH, 5, mkc(0, 200, 150), 30},
  {"Scale Armor", "+10% unit health per level", B_BLACKSMITH, 5, mkc(0, 250, 100), 30},
  {"Fletching", "+10% ranged damage per level", B_BLACKSMITH, 5, mkc(150, 0, 150), 30},
  {"Masonry", "+12% building health per level", B_UNIVERSITY, 5, mkc(250, 0, 0, 200), 45},
  {"Siege Engineers", "+15% siege damage per level", B_UNIVERSITY, 5, mkc(300, 0, 250), 45},
  {"Chemistry", "+8% ranged and siege damage per level (gunpowder)", B_UNIVERSITY, 3, mkc(400, 0, 350, 200), 60},
};

const BDef& bdef(int t) { return BD[t]; }
const UDef& udef(int t) { return UD[t]; }
const TechDef& techdef(int t) { return TD[t]; }
const TcUp& tcup(int l) { return TCU[clampi(l, 2, 10)]; }
int bhp(int t, int lvl) { const BDef& d = BD[t]; lvl = clampi(lvl, 1, MAXLVL); return lvl <= 3 ? d.hp[lvl - 1] : roundi(d.hp[2] * std::pow(1.25, lvl - 3)); }
int bdps(int t, int lvl) { const BDef& d = BD[t]; lvl = clampi(lvl, 1, MAXLVL); return lvl <= 3 ? d.dps[lvl - 1] : roundi(d.dps[2] * std::pow(1.18, lvl - 3)); }
int bmaxn(int t, int tc) { return MAXN[t][clampi(tc, 1, MAXTC) - 1]; }
int bslots(int t, int lvl) { const BDef& d = BD[t]; lvl = clampi(lvl, 1, MAXLVL); return lvl <= 3 ? d.slots[lvl - 1] : (d.slots[2] > 1 ? d.slots[2] + (lvl - 3) : d.slots[2]); }
int capFor(int tc) { return CAP[clampi(tc, 1, MAXTC) - 1]; }
int protFor(int tc) { return PROT[clampi(tc, 1, MAXTC) - 1]; }
const char* ageName(int tc) { static const char* n[4] = {"Dark Age", "Feudal Age", "Castle Age", "Imperial Age"}; return n[ageOf(tc)]; }
const char* rankOf(int r) {
  static const char* n[9] = {"Peasant", "Squire", "Knight", "Baron", "Earl", "Duke", "Prince", "King", "Emperor"};
  static const int v[9] = {0, 200, 600, 1200, 2000, 3000, 4200, 5500, 7500};
  int k = 0; for (int i = 0; i < 9; i++) if (r >= v[i]) k = i; return n[k];
}
int rankIdx(int r) { static const int v[9] = {0, 200, 600, 1200, 2000, 3000, 4200, 5500, 7500}; int k = 0; for (int i = 0; i < 9; i++) if (r >= v[i]) k = i; return k; }
double expectedScore(double my, double opp) { return 1.0 / (1.0 + std::pow(10.0, (opp - my) / 400.0)); }
std::string fmtTime(double s) {
  int t = (int)std::ceil(std::max(0.0, s));
  if (t >= 3600) return std::to_string(t / 3600) + "h " + std::to_string((t % 3600) / 60) + "m";
  if (t >= 60) return std::to_string(t / 60) + "m " + std::to_string(t % 60) + "s";
  return std::to_string(t) + "s";
}

const char* LORD_FIRST[30] = {"Aldric", "Bertrand", "Cedric", "Dagobert", "Eadric", "Fulk", "Godfrey", "Harold", "Ingvar", "Jorund", "Kazimir", "Leofric", "Mathis", "Osric", "Piers", "Ragnar", "Saladin", "Temur", "Ulric", "Wulfstan", "Yaroslav", "Zoran", "Alaric", "Baldwin", "Conrad", "Dietrich", "Edmund", "Frederick", "Gunther", "Hugo"};
const char* LORD_TITLE[10] = {"the Bold", "the Grim", "the Fair", "the Red", "of the Marsh", "Ironhand", "the Wise", "Stormborn", "the Pale", "Wolfsbane"};
const char* PERSONALITY[4] = {"Turtle", "Raider", "Merchant", "Warlord"};

SimB makeSimB(BType type, int lvl, int x, int y) {
  const BDef& d = BD[type]; float hp = (float)bhp(type, lvl);
  SimB s{type, lvl, (float)x, (float)y, d.w, d.h, hp, hp, (float)bdps(type, lvl), d.range, 0.f, false, d.hq};
  s.wall = type == B_WALL || type == B_GATE; s.gate = type == B_GATE; s.trap = type == B_SPIKE || type == B_OIL;
  return s;
}

Base genBase(uint32_t seed, int tc, bool barb, int level) {
  Rng r(seed); Base out; std::set<std::pair<int, int>> occ;
  auto freeAt = [&](int x, int y, int w, int h) {
    if (x < 2 || y < 2 || x + w > MAP - 2 || y + h > MAP - 2) return false;
    for (int i = x - 1; i < x + w + 1; i++) for (int j = y - 1; j < y + h + 1; j++) if (occ.count({i, j})) return false;
    return true;
  };
  auto put = [&](BType t, int lvl, int x, int y) {
    const BDef& d = BD[t]; if (!freeAt(x, y, d.w, d.h)) return false;
    for (int i = x; i < x + d.w; i++) for (int j = y; j < y + d.h; j++) occ.insert({i, j});
    out.buildings.push_back(makeSimB(t, lvl, x, y)); return true;
  };
  double rcapGlobal = 99.0;
  auto scatter = [&](BType t, int lvl, double rmin, double rmax) {
    const BDef& d = BD[t]; if (!isPiece(t)) rmax = std::min(rmax, rcapGlobal); if (rmin > rmax) rmin = rmax - 1;
    for (int k = 0; k < 120; k++) {
      double a = r.next() * 6.2831853, rad = rmin + r.next() * (rmax - rmin);
      if (put(t, lvl, roundi(15 + std::cos(a) * rad - d.w / 2.0), roundi(15 + std::sin(a) * rad - d.h / 2.0))) return;
    }
  };
  if (barb) {
    put(B_CHIEF, 1, 14, 14);
    int tents = 2 + level, towers = (int)std::floor(level / 1.5);
    for (int i = 0; i < tents; i++) scatter(B_TENT, 1, 4, 8);
    for (int i = 0; i < towers; i++) scatter(B_TOWER, level >= 4 ? 2 : 1, 3.5, 7);
    out.loot = mkc(250 + 220 * level, 100 + 100 * level, 120 + 150 * level, 80 + 100 * level);
    out.name = "Barbarian Camp " + std::to_string(level);
  } else {
    tc = clampi(tc, 1, MAXTC);
    put(B_TOWNHALL, tc, 14, 14);
    out.pers = (int)(r.next() * 4);
    int houses = 3 + tc * 2, towers = tc + 1, stores = 2 + tc;
    if (out.pers == 0) { towers += 2; houses -= 1; }
    if (out.pers == 2) { stores += 2; towers -= 1; }
    if (out.pers == 1) houses += 1;
    towers = std::min(towers, 10);
    bool walled = tc >= 2 && (out.pers == 0 || r.next() < 0.35 + 0.06 * tc);
    rcapGlobal = walled ? 7.2 : 99.0;
    if (walled) {
      int wl = clampi(tc, 1, MAXLVL), lo = 5, hi = 25;
      int gate1 = (int)(r.next() * 4), gate2 = (gate1 + 1 + (int)(r.next() * 3)) % 4;           // two gate sides: 0 N,1 E,2 S,3 W
      auto isGate = [&](int side, int a) { return (side == gate1 || side == gate2) && std::abs(a - 15) <= 1; };
      auto ringPut = [&](int x, int y, bool gate) { occ.insert({x, y}); out.buildings.push_back(makeSimB(gate ? B_GATE : B_WALL, wl, x, y)); };
      for (int a = lo; a <= hi; a++) {
        ringPut(a, lo, isGate(0, a) && a == 15); ringPut(a, hi, isGate(2, a) && a == 15);
        if (a != lo && a != hi) { ringPut(lo, a, isGate(3, a) && a == 15); ringPut(hi, a, isGate(1, a) && a == 15); }
      }
      if (tc >= 4) { int sp = 2 + tc / 2; for (int i = 0; i < sp; i++) scatter(B_SPIKE, clampi(tc - 3, 1, 8), 7.2, 8.6); }
      if (tc >= 6) { int oi = 1 + (tc - 6) / 2; for (int i = 0; i < oi; i++) scatter(B_OIL, clampi(tc - 5, 1, 8), 7.2, 8.6); }
    }
    if (tc >= 7) { int sc = 1 + (tc - 7) / 2 + (out.pers == 0 ? 1 : 0); for (int i = 0; i < sc; i++) scatter(B_SCORPION, tc - 4, 4, 8); }
    if (tc >= 8) { int bombs = 1 + (tc - 8) + (out.pers == 0 ? 1 : 0); for (int i = 0; i < bombs; i++) scatter(B_BOMBARD, tc - 2, 4, 8); }
    if (tc >= 6) scatter(B_CASTLE, tc - 3, 5, 8);                       // garrison castle
    if (tc >= 5) { int keeps = 1 + (tc - 5) + (out.pers == 0 ? 1 : 0); for (int i = 0; i < keeps; i++) scatter(B_KEEP, tc - 3, 4, 8); }
    for (int i = 0; i < towers; i++) scatter(B_TOWER, std::min(tc, MAXLVL), 3.5, 7.5);
    if (tc >= 5) { scatter(B_BARRACKS, tc - 2, 5, 9); scatter(B_STABLE, tc - 3, 5, 9); scatter(B_BLACKSMITH, tc - 2, 5, 9); if (tc >= 4) scatter(B_MARKET, tc - 2, 5, 9); }
    for (int i = 0; i < stores; i++) scatter(B_STOREHOUSE, 1, 4, 10);
    for (int i = 0; i < houses; i++) scatter(B_HOUSE, std::max(1, tc - 2), 4, 11);
    double base = 500 * std::pow((double)tc, 1.6) * (out.pers == 2 ? 1.4 : 1.0) * (0.8 + r.next() * 0.4);
    out.loot = mkc(roundi(base), roundi(base * 0.7), roundi(base * 0.55), roundi(base * 0.4));
  }
  return out;
}
