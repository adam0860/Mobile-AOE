// Game state, economy and raid logic
#pragma once
#include <deque>
#include "defs.h"
#include "sim.h"

struct QItem { UType type; float left; };
struct Bld {
  int id; BType type; int x, y, w, h; int lvl; int workers;
  bool building; float bStart, bEnd; int bTo; std::vector<QItem> q;
  int status; float acc; // status: 0 none,1 idle,2 work,3 full,4 no nodes
};
struct Node { int x, y; NodeType type; float amt, max, respawn; };
enum AType { A_DEER, A_BOAR, A_WOLF };
struct Animal { int id; int type; float x, y, hx, hy, tx, ty, wait, food, maxFood, aph; int face; bool alive; double respawn; };
struct Explorer { int id; bool vill; float x, y, tx, ty; bool go, recall, autoX; float aph; int face; };
struct Chest { int x, y; Cost c; bool taken; };
struct Lord { std::string name; int r0; double ph; };
// ---- battle replays: the starting setup + the deploy log is enough to re-run a battle exactly (the sim is deterministic) ----
struct RDeploy { int step, type; float x, y; };
struct RUnit { int type; bool def; float x, y; };
struct Replay {
  int kind = 0;                        // 0 raid on a barbarian camp, 1 raid on a rival lord, 2 defence of the village, 3 defence of an outpost
  std::string title, who; int stars = 0, pct = 0, dr = 0; bool win = false; int tcMine = 1, tcEnemy = 1, tcBuild = 1; long long when = 0; Cost loot;
  bool defence = false; float dt = 1.f / 60.f; int reserve0 = 0, endStep = -1;
  std::vector<SimB> b; std::vector<RUnit> units; std::vector<Spawn> sq; SideMul mul[2]; std::vector<RDeploy> deploys;
};
struct ReplayPlayer {                   // steps a replay exactly like the live battle did (also used headless by the self-test)
  const Replay* r = nullptr; Sim* sim = nullptr; size_t di = 0; int step = 0, nonMonk = 0; float acc = 0;
  ReplayPlayer() {} ReplayPlayer(const ReplayPlayer&) = delete; ReplayPlayer& operator=(const ReplayPlayer&) = delete;
  void start(const Replay& rp, Sim& s);
  void stepOnce();
};
Replay replayCapture(const Sim& init, int kind, float dt);
void replayPush(const Replay& r);
void replayRun(const Replay& r, int& stars, int& pct, int maxSteps = 400000);
struct Report { std::string who; int stars, pct; Cost stolen; int dr; bool held; int atkTc = 0, diff = 0; };

struct State {
  double t = 0; float res[NRES] = {400, 300, 100, 100}; int villagers = 3; std::vector<float> vq;
  int army[U_COUNT] = {}; int renown = 0; double shieldUntil = 0, nextRaid = 0; int nid = 1; uint32_t seed = 1;
  double camp[NCAMP + 1] = {}; int campWins[NCAMP + 1] = {}; int rivalSeq = 0; bool seenIntro = false; long long lastSeen = 0;
  int season = 1; double seasonStart = 0; int sWins = 0, sDef = 0; bool musicOn = true, sfxOn = true;
  int wallLvl = 1, trapLvl = 1;      // shared level of all wall pieces / all traps
  int tech[T_COUNT] = {}; int researchTech = -1; double researchEnd = 0;
  std::deque<Bld> b; std::vector<Node> nodes;
  // ---- exploration world ----
  std::vector<uint8_t> fog;                      // WORLD*WORLD, 1 = explored
  std::vector<Animal> animals; std::vector<Explorer> expl; std::vector<Chest> chests;
  std::vector<float> scq; int scoutStock = 0;    // scouts being trained / waiting at home
  int patchX[3] = {-1, -1, -1}, patchY[3] = {-1, -1, -1}, patchN[3] = {0, 0, 0};   // where depleted deposits regrow
  int aid = 1;
  std::deque<Replay> replays;                    // newest first, at most 10
};
extern State S;

// UI feedback hooks (implemented by the app layer)
void toast(const std::string& msg);
void sfx(int id);                       // plays a sound effect (implemented by the app layer; ids are the Sfx enum)
void banner(const std::string& msg);

struct UpInfo { const char* err = nullptr; Cost cost; int time = 0; std::string label; };

void newGame();
bool loadGame();
void saveGame();
extern std::string g_savePath;
Bld& addB(BType t, int x, int y, int lvl);
Bld* findB(int id);
Bld& tcB();
int tcLvl();
int cap();
int prot();
bool isDone(const Bld& b);
int builders();
int assigned();
int idleV();
bool takeVillager();   // true if a villager is free to build; pulls one off a work site when none is idle
int vCap();
int armyCount();
int queuedPop();
int housing();
bool canAfford(const Cost& c);
void pay(const Cost& c);
UpInfo upgradeInfo(const Bld& b);
void startUpgrade(Bld& b);
void econTick(float dt);
void trainVillager();
void trainUnit(Bld& b, UType t);
int buildCount(BType t);
const char* canBuildErr(BType t);
bool placeOk(BType t, int x, int y, int ignore = -1);   // `ignore` = a building id that is being moved
int workersOn(int res);                                   // villagers currently working on a resource (wood/food/gold/stone)
Cost moveCost(const Bld& b);                              // moving costs a quarter of the build cost
bool moveBuilding(int id, int x, int y);
void setWorkers(Bld& b, int d);
// Castle Age systems
Cost techCost(int tech);
int techTime(int tech);
const char* techBlock(int tech);   // nullptr if it can be researched now
void startResearch(int tech);
void trade(int kind);
const char* tradeLabel(int kind);
Cost tradeGive(int kind);
Cost tradeGet(int kind);
// walls, gates, traps (1x1 pieces, instant)
const char* placePieceBlock(BType t, int x, int y);   // nullptr if the piece can be placed on this tile
bool placePiece(BType t, int x, int y);                // places (or turns a wall into a gate); toasts on failure
void removePiece(int id);
int pieceCount(bool traps);                            // wall pieces (gates count x5) or traps
Cost wallUpgradeCost();
Cost trapUpgradeCost();
const char* wallUpgradeBlock();
const char* trapUpgradeBlock();
void upgradeWalls();
void upgradeTraps();
SideMul techMul();
// ---- matchmaking by Town Center level (diff = enemy level - yours, -2..+2) ----
inline float winFactor(int diff) { static const float f[5] = {0.15f, 0.4f, 1.f, 1.6f, 2.2f}; return f[clampi(diff, -2, 2) + 2]; }     // Renown for a win (or a held defence)
inline float loseFactor(int diff) { static const float f[5] = {2.2f, 1.6f, 1.f, 0.5f, 0.25f}; return f[clampi(diff, -2, 2) + 2]; }   // Renown lost when beaten
inline float lootFactor(int diff) { static const float f[5] = {0.5f, 0.75f, 1.f, 1.25f, 1.5f}; return f[clampi(diff, -2, 2) + 2]; }  // loot when you raid a lord of that level
int rollTcDiff(Rng& r);                  // random relative level for a rival / raider, within +-2
Cost villagerCost();                     // villagers get dearer the more you have
float villagerTime();
bool nodeVisible(const Node& n);         // gold and stone only exist from the Feudal Age
void topUpNodes();                       // keeps the village stocked with trees / deposits as the Town Center grows
// ---- barbarian camps ----
float campMul(int camp);                 // loot multiplier: 1, then 0.5, 0.25, 0.1 for repeated clears
int campsUnlocked();
const Base& campBase(int camp);          // cached
// ---- exploration ----
bool isExplored(int x, int y);
bool inWorld(float x, float y);
extern bool g_fogDirty;
void genWorld();
void revealAround(float x, float y, float r);
void trainScout();
int scoutTotal();
bool sendExplorer(bool villager);
Explorer* findExplorer(int id);
void orderExplorer(int id, float x, float y);
void recallExplorer(int id);
bool demolish(int id);
int outsideBuildings();
struct OutpostReport { bool replayed = false, any = false; int lostB = 0, lostV = 0, lostE = 0, held = 0; std::string where; };
OutpostReport outpostRaid();
SideMul enemyMul(int tc);

// ---- seasons ----
struct SeasonResult { int season, finalRenown, pos, rankIdx, newRenown, wins, defs; Cost reward; };
double seasonLen();
double seasonLeft();
int ladderPosition();                  // 1-based position among the 300 lords
bool seasonCheck(SeasonResult& r);     // true when a season just ended (state already rolled over)
const std::vector<Lord>& lords();
int lordRenown(const Lord& l);
Sim makeDefenceSim();
Report applyDefence(const Sim& sim);
Report headlessRaid();
