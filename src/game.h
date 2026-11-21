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
struct Lord { std::string name; int r0; double ph; };
struct Report { std::string who; int stars, pct; Cost stolen; int dr; bool held; };

struct State {
  double t = 0; float res[NRES] = {400, 300, 100, 100}; int villagers = 3; std::vector<float> vq;
  int army[U_COUNT] = {}; int renown = 0; double shieldUntil = 0, nextRaid = 0; int nid = 1; uint32_t seed = 1;
  double camp[6] = {0, 0, 0, 0, 0, 0}; int rivalSeq = 0; bool seenIntro = false; long long lastSeen = 0;
  int season = 1; double seasonStart = 0; int sWins = 0, sDef = 0; bool musicOn = true, sfxOn = true;
  int wallLvl = 1, trapLvl = 1;      // shared level of all wall pieces / all traps
  int tech[T_COUNT] = {}; int researchTech = -1; double researchEnd = 0;
  std::deque<Bld> b; std::vector<Node> nodes;
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
bool placeOk(BType t, int x, int y);
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
