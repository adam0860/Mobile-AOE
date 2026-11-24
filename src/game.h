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
struct Animal { int id; int type; float x, y, hx, hy, tx, ty, wait, food, maxFood, aph; int face; bool alive; double respawn; float hp = 0, maxHp = 0; bool chasing = false; };
struct Explorer { int id; bool vill; float x, y, tx, ty; bool go, recall, autoX; float aph; int face; bool monk = false, carry = false; float hp = 0, maxHp = 0, hurt = 0; };   // scout (default), villager (vill) or monk (carries relics)
struct Relic { int id; float x, y; };
// ---- field squads: troops taken out of the army that walk around the map in real time
struct SqUnit { int type; float hp; };
enum SquadMode { SQ_FREE, SQ_ESCORT, SQ_GUARD, SQ_ATTACK };
struct Squad { int id = 0; std::vector<SqUnit> u; float x = 0, y = 0, tx = 0, ty = 0; bool go = false, recall = false; int mode = SQ_FREE, target = -1; float gx = 0, gy = 0, aph = 0, fight = 0; int face = 3; };
struct Meta { int prestige = 0, points = 0; uint32_t unlocked = 0; int civ = 0, boon = 0; bool outlineOn = false; };    // survives restarts (unlocked bit 0 = gold outline owned; civ 0 = Byzantines; boon = reward for this realm)
enum Stance { ST_WAR, ST_PEACE, ST_TRADE, ST_ALLY };
enum Proposal { DP_PEACE, DP_TRADE, DP_ALLY };
struct Chest { int x, y; Cost c; bool taken; };
struct Lord { std::string name; int r0; double ph; };
// ---- battle replays: the starting setup + the deploy log is enough to re-run a battle exactly (the sim is deterministic) ----
struct RDeploy { int step, type; float x, y; };
struct RUnit { int type; bool def; float x, y; };
struct Replay {
  int kind = 0;                        // 0 raid on a barbarian camp, 1 raid on a rival lord, 2 defence of the village, 3 defence of an outpost
  std::string title, who; int stars = 0, pct = 0, dr = 0; bool win = false; int tcMine = 1, tcEnemy = 1, tcBuild = 1; long long when = 0; Cost loot; int lordColor = -1;
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
// ---- rival lords living in the other three quadrants of the map ----
struct Enemy { int id = 0, quad = 1, tc = 1, pers = 0; std::string name; uint32_t seed = 1; double nextLvl = 0; std::vector<int> dead;
  int stance = ST_WAR; float trust = 0; double talkAt = 0;                      // diplomacy
  int color = 0;                                                                  // lord colour (index into LORD_COL), kept until he is defeated
  bool seen = false; int seenTc = 0; size_t seenDead = 0; bool visNow = false; float tacc[NRES] = {};   // last seen state of the base (fog of war memory)
};
struct Report { std::string who; int stars, pct; Cost stolen; int dr; bool held; int atkTc = 0, diff = 0; bool ruined = false; int lostB = 0, lostV = 0, lostU = 0; };

constexpr int NPRESET = 4;                        // saved army compositions

struct State {
  double t = 0; float res[NRES] = {400, 300, 100, 100}; int villagers = 3; std::vector<float> vq;
  int army[U_COUNT] = {}; int renown = 0; double shieldUntil = 0, nextRaid = 0; int nid = 1; uint32_t seed = 1;
  double camp[NCAMP + 1] = {}; int campWins[NCAMP + 1] = {}; int rivalSeq = 0; bool seenIntro = false; long long lastSeen = 0;
  int season = 1; double seasonStart = 0; int sWins = 0, sDef = 0; bool musicOn = true, sfxOn = true; float musicVol = 0.85f, sfxVol = 1.f;
  int wallLvl = 1, trapLvl = 1;      // shared level of all wall pieces / all traps
  int tech[T_COUNT] = {}; int researchTech = -1; double researchEnd = 0;
  std::deque<Bld> b; std::vector<Node> nodes;
  // ---- exploration world ----
  std::vector<uint8_t> fog;                      // WORLD*WORLD, 1 = explored
  std::vector<Animal> animals; std::vector<Explorer> expl; std::vector<Chest> chests;
  std::vector<float> scq; int scoutStock = 0;    // scouts being trained / waiting at home
  int patchX[3] = {-1, -1, -1}, patchY[3] = {-1, -1, -1}, patchN[3] = {0, 0, 0};   // where depleted deposits regrow
  int aid = 1;
  std::vector<Squad> squads;
  Meta meta; std::vector<Relic> relics; int relicsHeld = 0, betrayals = 0; double relicCheck = 0; std::vector<uint8_t> vis;   // vis: tiles you can currently see (not saved)
  std::vector<Enemy> enemies; double enemyCheck = 0;   // rival lords on the map (quadrant 1..3; yours is 0)
  std::deque<Replay> replays;                    // newest first, at most 10
  int presets[NPRESET][U_COUNT] = {};            // saved army compositions (unit counts)
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
Cost presetCost(const int counts[U_COUNT]);
int presetPop(const int counts[U_COUNT]);
void queuePreset(int slot);              // queues every unit of a preset exactly like training them one by one (same cost, same time); toasts the outcome
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
void rebalanceNodes();
float gatherMul(int res);                // research bonus for gathering a resource (1 + 25% per upgrade)
void topUpNodes();                       // keeps the village stocked with trees / deposits as the Town Center grows
// ---- quadrants, enemy lords, village size ----
int homeG();                                           // the village grows by this many tiles on every side (2 per two Town Center levels)
bool inHome(float x, float y);                         // inside the current village rectangle
void revealHome();                                     // lifts the fog over the (possibly grown) village
const char* placeBlockReason(BType t, int x, int y, int ignore = -1, bool limits = false);   // nullptr when the building may go there
int buildCountAt(BType t, bool outside, int ignore = -1);
Enemy* findEnemy(int id);
void enemyOrigin(const Enemy& e, int& ox, int& oy);    // world position of the (0,0) corner of the enemy's 30x30 base
Base enemyBase(const Enemy& e, std::vector<int>* srcIdx = nullptr);   // the buildings still standing (local 30x30 coordinates)
bool enemyDiscovered(const Enemy& e);
void addEnemyDefenders(Sim& sim, const Enemy& e);      // the lord's own troops (battle-map coordinates)
void enemyDamage(int id, const std::vector<int>& deadIdx);
void enemyWiped(int id);
// ---- lord colours: red, green, yellow, orange, purple, grey ----
constexpr int NLORDCOL = 6;
extern const uint32_t LORD_COL[NLORDCOL];
extern const char* LORD_COL_NAME[NLORDCOL];
// ---- diplomacy, relics, visibility, prestige ----
const char* stanceName(int s);
float diploChance(const Enemy& e, int proposal);               // 0..1
const char* diploBlock(const Enemy& e, int proposal);          // why this cannot even be proposed right now (nullptr = fine)
bool diploPropose(int enemyId, int proposal, std::string& msg);
Cost diploGiftCost(const Enemy& e);
void diploGift(int enemyId, std::string& msg);
void diploWar(int enemyId, bool treachery);                   // treachery = attacking while at peace / trading / allied
int tradeRoutesAllowed();
int tradeRoutesActive();
Cost tradeIncome(const Enemy& e);                              // per minute for one route
float relicGoldPerMin();
bool sendMonk();
bool formSquad(const int counts[U_COUNT]);                 // takes troops out of the army and puts them on the map next to the Town Center
Squad* findSquad(int id);
void orderSquad(int id, float x, float y);                  // walk there (free movement)
void squadEscort(int id, int explorerId);                  // follow and protect an explorer
void squadGuard(int id);                                    // hold this spot and fight whatever comes
void squadAttack(int id, int enemyId);                      // march on a rival lord's base; an arrival event is raised at his gates
void squadRecall(int id);                                   // walk home; the troops rejoin the army
bool squadArrival(int& squadId, int& enemyId);              // true once when an attacking squad reaches its target
void squadRemove(int id);
int fieldPop();                                             // troops currently out on the map (they still need housing)
float squadSpeed(const Squad& s);
float squadHpFrac(const Squad& s);
bool isVisible(int x, int y);
extern bool g_visDirty;
Base enemyBaseAt(const Enemy& e, int tc, size_t deadCount, std::vector<int>* srcIdx = nullptr);
// ---- civilizations (cosmetic building styles) and the rewards chosen when you ascend
constexpr int NCIV = 7;
struct CivDef { const char* code; const char* name; const char* desc; };
extern const CivDef CIVS[NCIV];                                 // 0 = Byzantines, the default look
enum Boon { BOON_NONE, BOON_PROSPER, BOON_VETERANS, BOON_DIPLOMAT, BOON_HARDY, NBOON };
struct BoonDef { const char* name; const char* desc; };
extern const BoonDef BOONS[NBOON];
constexpr uint32_t UNLOCK_OUTLINE = 1u;
bool outlineOwned();
bool outlineChoiceOpen(int prestigeAfter);                      // the gold outline is offered from the second ascension, once
const char* civCode(int civ);                                   // "" for the Byzantines (stock sprites)
bool prestigeAvailable();                                      // Town Center at the maximum level
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
double shieldSeconds(int tc, int stars);   // raid shield granted after a defence
Sim makeDefenceSim();
Report applyDefence(const Sim& sim);
Report headlessRaid();
