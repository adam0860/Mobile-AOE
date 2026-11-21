// Battle simulation shared by player raids, NPC raids and offline raids
#pragma once
#include <functional>
#include "defs.h"

struct SimUnit {
  int id; UType type; bool def; float x, y, hp, maxhp, cd; int tgtB, tgtU; float swing;
  float dx = 0, dy = 1; bool moving = false; float hfx = 0;
  float aph = 0, hitT = 0, lastHp = -1, swingMax = 0.4f; mutable int face = 3;     // animation: walk phase (cycles), hit flash, facing with hysteresis
  // pathing (walls): tile path to the current building target
  std::vector<int> path; int pidx = 1, pver = -1, ptgt = -2; float slow = 0;
};
struct Proj { float x0, y0, x1, y1, life, max; uint32_t col; };
struct Corpse { float x, y, dx, dy, age; UType type; bool def; };
struct SimEvent { int type; float x, y; };
struct Spawn { float t; UType type; float x, y; };
enum SimEv { EV_HIT, EV_ARROW, EV_BOLT, EV_CANNON, EV_COLLAPSE, EV_TRAP, EV_HEAL, EV_DEPLOY, EV_DEATH, EV_COUNT };
struct SideMul { float melee = 1, ranged = 1, siege = 1, hp = 1; };

float distRect(float x, float y, const SimB& b);

struct Sim {
  std::vector<SimB> b; std::vector<SimUnit> units; std::vector<Proj> proj; std::vector<Spawn> sq;
  float t = 0, timeLeft = 180; bool started = false, over = false, defence = false; int nid = 1;
  SideMul mul[2];                       // [0] = attackers, [1] = defenders
  int atkTc = 1, atkDiff = 0;           // NPC raids: the attacker's Town Center level and how far it is above (+) / below (-) yours
  std::vector<SimEvent> events;         // audio/visual events raised this step (drained by the app)
  std::vector<Corpse> corpses;          // fallen units fading out
  void emit(int type, float x, float y) { events.push_back({type, x, y}); }
  std::function<int()> reserve;         // undeployed fighters the attacker still holds
  // wall grid (rebuilt every step): index of the standing wall/gate piece on each tile, or -1
  std::vector<int> wallIdx = std::vector<int>(MAP * MAP, -1); int gridVer = 0, lastWalls = -1;

  void deploy(UType type, float x, float y, bool def = false);
  void spawnGarrisons();                // castle garrisons come out to fight
  int alive() const;                    // standing buildings that count for destruction (walls and traps do not)
  int fighters() const;                 // attacking units that can still fight (monks don't count)
  int pct() const;
  bool hqDead() const;
  int stars() const;
  void step(float dt);
  bool findPath(const SimUnit& u, const SimB& target, float reach, std::vector<int>& out) const;
};
