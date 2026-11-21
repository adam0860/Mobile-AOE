# GAME DESIGN DOCUMENT — "DAWN OF KINGS" (working title)

*A medieval base-builder / raid strategy game for Android and Windows PC.*
Version 0.1 · 2026-11-20

---

## 1. High Concept

**Dawn of Kings** is a persistent, single-player-first base-building and raiding game set in the medieval world of Age of Empires 2 (Dark → Feudal → Castle → Imperial). The player starts with a Town Center, one House and one Watch Tower, then grows a village, gathers resources, trains armies, raids barbarian camps and rival lords (NPC "players"), and defends against AI raids. Every raid won or lost, offence or defence, moves the player's **Renown** score, and the goal is to climb the ranked ladder from Peasant to Emperor.

**Elevator pitch:** *Clash of Clans' loop wearing Age of Empires 2's clothes — real villagers, real ages, real medieval armies.*

### 1.1 Design Pillars

1. **Age progression is the fantasy.** Advancing from Dark Age militia to Imperial Age champions and bombard cannons must *feel* like an AoE2 age-up: new look, new units, new tactics.
2. **Raid, build, defend — a tight loop.** Every session answers three questions: What can I afford? What can I attack? What threatens me?
3. **Villagers matter.** Unlike pure timer-based builders, villagers are a visible, limited workforce that you assign to build, chop, mine and farm.
4. **Short sessions, deep decisions.** 3–5 minute play sessions on phone; hours of long-term planning.
5. **Fair and offline-friendly.** Opponents are NPC lords, so the game works fully offline, has no pay-to-win PvP imbalance, and ships without server cost in v1.

### 1.2 Target Audience & Platforms

| | |
|---|---|
| Primary | Mobile strategy fans, 16–40, familiar with Clash of Clans / AoE |
| Secondary | PC players who want a lighter, session-based AoE experience |
| Platforms | Android 8.0+ (primary), Windows 10/11 (secondary). Same codebase, same save format |
| Session length | 3–5 min (mobile), 15–30 min (PC) |
| Monetisation | See §14 (premium-light; optional cosmetics and time-savers) |

### 1.3 IP Note (important)
"Age of Empires" and its art, names, civilisation branding and audio belong to Microsoft/Xbox Game Studios. This game is **inspired by** the era and genre but must ship with **original art, original title, original audio and generic unit names** (Militia, Archer, Knight are generic historical terms; avoid AoE-specific logos, sprites, voice lines and civ trademarks). If an official AoE license is ever intended, that is a separate legal project.

---

## 2. Core Gameplay Loop

```
          ┌────────────────────────────────────────────┐
          │                                            │
   GATHER ──► SPEND (build / upgrade / train) ──► RAID ──► LOOT
   (wood, food,        │                              ▲        │
    gold, stone)       ▼                              │        │
                  DEFEND ◄── AI raids ── RENOWN ◄─────┘        │
                       │      (rank ↑↓)                        │
                       └────────────► Climb the ladder ◄───────┘
```

**Minute-to-minute (mobile session):** collect resources → queue upgrades/training with idle villagers → scout & launch one raid → review any raid reports on your base → rebuild/repair → log off.

**Hour-to-hour:** Reach the next Town Center level, new defence, new unit.
**Week-to-week:** Age up, rank up, season rewards.

---

## 3. Starting State & First-Time User Experience (FTUE)

### 3.1 New Game State
- **Town Center (TC) Level 1** — Dark Age
- **1 House** (+5 villager capacity); TC itself provides +3
- **1 Watch Tower** (basic arrow tower)
- **3 Villagers**
- Starting resources: 400 Wood, 300 Food, 100 Gold, 100 Stone
- Map contains: a forest patch, a gold deposit, a stone deposit, a berry bush patch (free food), buildable grass.

### 3.2 Scripted Tutorial (first ~10 minutes)
| Step | Action | Teaches |
|---|---|---|
| 1 | Send villagers to chop trees (build Lumber Camp) | Gathering, camps |
| 2 | Build a Farm (+ Mill) | Food economy |
| 3 | Train a 4th villager (needs 2nd House) | Population cap |
| 4 | Build Barracks | Army |
| 5 | Train 6 Militia, attack a tiny Barbarian camp | Deploying troops, loot |
| 6 | Upgrade TC to Level 2 (instant for tutorial) | Progression |
| 7 | Scripted AI raid on player's base (tutorial raiders, weak) | Defence, Renown |
| 8 | Unlock Ranked Raid menu | Matchmaking |

The player is shielded from AI raids for the first 3 days / until TC 3.

---

## 4. Ages & Town Center Progression

The **Town Center level (1–10)** is the master progression gate (like Town Hall in CoC). Specific levels trigger an **Age advancement**, which changes the visual theme of the village, the music, and the unit/building roster.

| Age | TC Levels | Theme | Army Identity |
|---|---|---|---|
| **Dark Age** | 1–2 | Timber huts, palisade | **Militia only** — cheap melee swarm |
| **Feudal Age** | 3–4 | Thatch→wood+stone, towers | Men-at-Arms, Archers, Skirmishers, Scouts |
| **Castle Age** | 5–7 | Stone keeps, banners | Knights, Crossbowmen, Long Swordsmen, Monks, Rams, Mangonels |
| **Imperial Age** | 8–10 | Grand fortifications | Champions, Paladins, Arbalesters, Hand Cannoneers, Trebuchets, Bombard Cannons |

**Age-up cost** (TC 2→3, 4→5, 7→8) is a large one-time resource sink and uses the label "Advance to X Age" in the UI. Age-up has a cinematic flourish and sets a new village art set.

### 4.1 Town Center Level Table (draft values — tune in balance phase)

| TC Lvl | Age | Cost (Wood / Food / Gold / Stone) | Build Time | TC HP | Max Villagers cap* | Resource Protected in TC |
|---|---|---|---|---|---|---|
| 1 | Dark | — | — | 1,500 | 8 | 200 each |
| 2 | Dark | 400 W / 300 F | 2 min | 1,900 | 12 | 400 each |
| 3 | Feudal | 1,200 W / 800 F / 400 G | 1 hr | 2,500 | 18 | 800 each |
| 4 | Feudal | 3,000 W / 2,000 F / 800 G / 500 S | 4 hr | 3,200 | 24 | 1,500 each |
| 5 | Castle | 7,000 W / 5,000 F / 2,500 G / 1,500 S | 1 d | 4,200 | 30 | 3,000 each |
| 6 | Castle | 15 k / 10 k / 6 k / 4 k | 2 d | 5,200 | 36 | 5,000 each |
| 7 | Castle | 30 k / 20 k / 12 k / 8 k | 3 d | 6,500 | 42 | 8,000 each |
| 8 | Imperial | 60 k / 40 k / 25 k / 15 k | 5 d | 8,000 | 48 | 12,000 each |
| 9 | Imperial | 100 k / 70 k / 45 k / 28 k | 7 d | 10,000 | 54 | 18,000 each |
| 10 | Imperial | 160 k / 110 k / 75 k / 50 k | 10 d | 12,500 | 60 | 25,000 each |

\*Capped by the number of Houses actually built.

**The TC also:** shoots arrows (damage/range scale with level), can be garrisoned for extra defence, produces villagers, researches **Eco Techs** (see §6.4), and hosts the **Rank banner**.

---

## 5. Resources

Four resources, matching AoE2.

| Resource | Source | Primary Uses |
|---|---|---|
| **Wood** | Trees → Lumber Camp | Buildings, palisades, siege, archers |
| **Food** | Farms (Mill), berry bushes, (Feudal+) fishing/hunt cabins | Villagers, units, age-ups, upgrades |
| **Gold** | Gold deposits → Mining Camp | Advanced units, techs, defences |
| **Stone** | Stone deposits → Mining Camp | Towers, walls, Keeps, age-ups |

### 5.1 Gathering Model
- **Villagers** are the workforce and are always in one of: *Idle, Builder, Gatherer, Farmer*.
- **Camps** (Lumber Camp / Mining Camp) are built next to a resource node; each camp holds up to 5 villagers (more at higher camp levels). Gathering rate per villager is fixed; **camp level boosts rate** and **Eco Techs** boost it further (Wheelbarrow, Gold Mining, etc.).
- **Resource nodes are finite** (a forest has N wood, a gold deposit M gold). When depleted, nodes *respawn* after a delay (forests regrow, deposits slowly refill) **or** the player can pay to clear/relocate. This preserves the "mine the nearby gold and stone" fantasy while keeping the economy sustainable.
- **Farms** are infinite but yield slowly; 1 villager per farm. Farms must be **reseeded** by tapping (costs small wood, mirrors AoE2). Mill adjacent increases yield.
- **Offline accumulation** is capped at 8 hours of production and by storage capacity.

### 5.2 Storage & Protection
- **Storehouses** (one per resource; upgradeable) set the max stored amount.
- The Town Center holds a **protected portion** (see table 4.1) that can never be looted.
- Everything else is **lootable** (see §9).

---

## 6. Base Building

### 6.1 Base Layout
- Isometric 2D grid base, **40×40 tiles** at TC1 expanding to **60×60** at TC10 by clearing obstacles.
- Buildings occupy tiles; walls snap to grid. Villagers physically walk between nodes and camps (visual only; economy rates are formula-based so it is cheap on mobile).
- Edit mode: drag-and-drop, save layout presets (up to 3).

### 6.2 Building Catalogue & Unlock by TC Level

**Economy**
| Building | Unlock TC | Notes |
|---|---|---|
| House | 1 | +5 villagers each. Max 5 at TC1 → 20 at TC10 |
| Lumber Camp | 1 | Wood gathering |
| Mining Camp | 1 | Gold or stone (placed next to node) |
| Farm | 1 | Food. Max 5 → 25 |
| Mill | 2 | Boosts adjacent farms; drop-off for food |
| Storehouse (Wood/Food/Gold/Stone) | 1 | Capacity |
| Market | 4 | Trade resources at a loss; daily offers |
| Blacksmith | 3 | Unit attack/armour upgrades |
| University | 7 | Advanced techs, siege upgrades |

**Military (training)**
| Building | Unlock TC | Trains |
|---|---|---|
| Barracks | 1 | Militia → Man-at-Arms → Long Swordsman → Champion (line) |
| Archery Range | 3 | Archer → Crossbow → Arbalester; Skirmisher; Hand Cannoneer |
| Stable | 3 | Scout → Knight → Paladin |
| Siege Workshop | 5 | Ram, Mangonel, Trebuchet, Bombard Cannon |
| Monastery | 5 | Monk (healer), research Healing/Fervor |
| Army Camp | 1 | Housing for troops (each holds 20 pop; max 4 → 8) |
| Castle (Garrison) | 6 | **Defensive reinforcement + Hero/Commander slot** (see §10.4) |

**Defences** — see §8.

### 6.3 Builders = Villagers
- Each construction/upgrade requires **one idle villager** for its duration.
- More Houses → more villagers → more simultaneous builds *or* gatherers: the central economy vs. expansion tension.
- Under-the-hood: 1 villager can only build *or* gather. Villager training (25 Food, 15 s) is done at the TC.

### 6.4 Eco Techs (researched at TC / camps)
Wheelbarrow, Hand Cart, Double-Bit Axe, Bow Saw, Gold Mining, Stone Mining, Horse Collar, Heavy Plow, Crop Rotation, Loom (villager HP), Town Watch, Town Patrol. Each is a one-time upgrade with age/TC gates, giving +10–25% to a rate or capacity.

---

## 7. Units

### 7.1 Unit Roster & Age Unlocks

| Age | Unit | Role | Pop | Cost | Notes |
|---|---|---|---|---|---|
| Dark | **Militia** | Cheap melee swarm | 1 | 40 F, 20 G | Only unit in Dark Age. Targets nearest building |
| Feudal | **Man-at-Arms** | Tanky melee | 1 | 60 F, 20 G | Armoured; good vs buildings & archers |
| Feudal | **Archer** | Ranged DPS | 1 | 25 W, 45 G | Fragile, targets nearest unit/building |
| Feudal | **Skirmisher** | Anti-archer ranged | 1 | 25 F, 35 W | Bonus damage vs archers |
| Feudal | **Scout Cavalry** | Fast, targets resource storage first | 2 | 80 F | Loot-grabber |
| Castle | **Long Swordsman** | Upgraded melee | 1 | 50 F, 25 G | Upgrade of Man-at-Arms |
| Castle | **Crossbowman** | Upgraded ranged | 1 | 25 W, 45 G | Upgrade of Archer |
| Castle | **Knight** | Heavy cavalry, goes for defences | 3 | 60 F, 75 G | Strong vs archers |
| Castle | **Monk** | Heals troops | 2 | 100 G | Fragile |
| Castle | **Battering Ram** | Siege vs buildings | 5 | 160 W, 75 G | Immune-ish to arrows, weak to melee |
| Castle | **Mangonel** | Area siege | 5 | 160 W, 135 G | Splash vs clusters |
| Imperial | **Champion** | Elite melee | 2 | 70 F, 45 G | |
| Imperial | **Arbalester** | Elite ranged | 1 | 25 W, 45 G | |
| Imperial | **Paladin** | Elite cavalry | 3 | 60 F, 75 G | |
| Imperial | **Hand Cannoneer** | High-burst gunpowder | 2 | 45 F, 50 G | |
| Imperial | **Trebuchet** | Long-range anti-defence siege | 6 | 200 W, 200 G | Slow, outranges towers |
| Imperial | **Bombard Cannon** | Anti-wall/anti-keep siege | 6 | 225 W, 225 G | |

> All numbers are placeholders for the balance phase. Unit names are generic medieval terms.

### 7.2 Unit Upgrades (Blacksmith / Barracks / Monastery)
Each unit line has **levels 1–5** unlocked by the corresponding production-building level + a Blacksmith tech (attack, armour, pierce armour, HP, speed). Upgrades take real time; **only one unit-line upgrade can run at a time** (a "research slot", expandable with Eco Tech).

### 7.3 Army Housing
- Army size is limited by **Army Camps** (capacity) and each unit's pop cost.
- Troops are **trained in batches** and kept at the camp until deployed; surviving troops return after a raid, dead troops are lost (like CoC).
- Queue size per production building: 5 → 10 as it levels.

---

## 8. Defences

Defences unlock by TC level; each can be upgraded (1–10 levels).

| Defence | Unlock TC | Behaviour |
|---|---|---|
| **Watch Tower** *(starting)* | 1 | Single-target arrows, short range; garrison 1 villager for +dmg |
| **Palisade Wall** | 2 | Cheap wooden wall; weak |
| **Stone Wall / Gate** | 3 | Strong wall; gate lets your own villagers pass |
| **Guard Tower** | 3 | Faster fire, more HP |
| **Spike Trap (Caltrops)** | 4 | Hidden damage trap vs ground units |
| **Keep** | 5 | Strong ranged, splash arrows, high HP |
| **Fortified Wall** | 6 | Very high HP; resists rams |
| **Boiling Oil Trap** | 6 | Damages & slows units at gate |
| **Mangonel Emplacement** | 6 | Area damage vs clusters |
| **Bombard Tower** | 8 | Anti-siege, long range |
| **Scorpion Emplacement** | 7 | Piercing bolts vs lines of units |
| **Castle (Garrison)** | 6 | Holds reinforcement troops that sally out when attackers approach |

- **Town Center** also defends (arrow count scales with TC level and garrisoned villagers).
- **Damage Model:** all defences have HP; at 0 HP they are "destroyed" for the duration of the raid and **rebuilt for free in a few minutes** after (repair cost is a small resource fee for walls only — keeps the loop non-punishing).

---

## 9. Combat & Raiding

### 9.1 Raid Types
1. **Barbarian Camps** (PvE, §10.1)
2. **Rival Lord Raids** (offensive vs NPC bases, ranked, §10.2)
3. **Defence Raids** (NPCs attack you, §11)
4. **Special Events** (weekend sieges, siege bosses; §16)

### 9.2 Offensive Flow
1. Tap **Raid**. Choose **Barbarian Camp** or **Find Rival** (costs gold per search, scales with rank).
2. Scout the base (30 seconds free look; can skip unlimited times for a gold cost).
3. Deploy troops by tapping/dragging along the base edge. Troops auto-attack using the AI targeting rules below.
4. 3:00 timer starts at first deploy. Raid ends when the timer ends, the TC & all buildings fall, or you retreat.
5. Results screen: stars, loot gained, Renown Δ, units lost, replay.

### 9.3 Troop AI Targeting
| Unit | Priority target |
|---|---|
| Militia, Man-at-Arms, Long Swordsman, Champion | Nearest building (walls only if blocked) |
| Archers / Skirmishers / Crossbow / Arbalester / Hand Cannoneer | Nearest enemy unit, else nearest building |
| Scout Cavalry | Nearest **resource storage / farm** |
| Knight / Paladin | Defences (towers, keeps) |
| Ram / Battering | Walls & gates, then buildings |
| Mangonel / Trebuchet / Bombard | Defences (ignores walls) |
| Monk | Follows and heals lowest-HP ally in range |

### 9.4 Stars & Victory
- **1 ★** — Destroy 50% of buildings
- **1 ★** — Destroy the Town Center
- **1 ★** — Destroy 100%
- A **win = ≥ 1 ★**. Loss = 0 ★.

### 9.5 Loot Formula (from rival bases)
```
Available_Loot(res) = clamp( Lootable(res) × 0.20, 0, LootCap(TC_level_of_defender) )
Gained(res) = Available_Loot(res) × (0.5 if 1★, 0.75 if 2★, 1.0 if 3★)
```
- Lootable = resources in storage minus the TC-protected amount.
- If attacker is far above defender in TC level, loot is scaled down (**loot penalty**) to avoid farming weak bases.
- Barbarian camps have fixed loot tables (§10.1).

### 9.6 Abilities (CoC-style "spells")
Brought to a raid in limited slots from a **Command Tent** building (unlock TC 4):
- **Rally Banner** — speed/attack buff in radius
- **Heal Prayer** — AoE heal over time (Monastery tech)
- **Fire Arrows** — AoE damage on a point
- **Siege Ladders** — lets troops ignore walls for 10 s
- **Smoke Bomb** — blocks tower line-of-sight

---

## 10. NPC World

### 10.1 Barbarian Camps (PvE)
- 40-level **Barbarian Campaign** on a world map; camps are fixed scenarios (hand-designed bases with scripted layouts, 2–3 waves of defenders).
- Reward: **resource loot (esp. gold/stone/wood)**, small Renown bonus, first-clear bonus (rare tech scroll), repeatable at lower loot.
- Barbarians **do not take** loot from you; they only appear as targets. Camps get tougher with level and include **Barbarian Chieftains** (mini-bosses).
- Camp reset: 1 attack per camp per 8 hours.

### 10.2 Rival Lords (NPC "Players")
- A roster of **~300 procedurally-generated named lords**, each with a name, banner, civilisation flavour (Britons, Franks, Mongols, Goths, Saracens… *stylised, original names*), TC level, Renown, and a **base layout** generated from templates that obey TC-level rules.
- Each has a **personality** that shapes base layout and raid behaviour:
  | Personality | Base style | Raid style (when attacking you) |
  |---|---|---|
  | Turtle | Walls, towers, heavy defence | Rare, strong siege |
  | Raider | Lean defences, rich storage | Frequent, fast cavalry |
  | Merchant | Large lootable stores, thin army | Rarely attacks |
  | Warlord | Balanced | Most raids, mixed army |
- Rival Renown **drifts over time** (simulated daily with an Elo-style random walk) so that the leaderboard feels alive.
- Matchmaking picks rivals within ±10% of your Renown and ≤ 1 TC level difference most of the time.
- **Revenge:** after being raided, you can raid that lord at a loot bonus once.

### 10.3 Ranked Ladder & Renown
Renown is a single number (min 0) with leagues:

| Rank | Renown | Perks |
|---|---|---|
| Peasant | 0–199 | |
| Squire | 200–599 | Unlock Barbarian Campaign tier 2 |
| Knight | 600–1,199 | Daily rank chest |
| Baron | 1,200–1,999 | Rival search discount |
| Earl | 2,000–2,999 | Banner cosmetics |
| Duke | 3,000–4,199 | Bonus loot cap |
| Prince | 4,200–5,499 | Seasonal rewards ↑ |
| King | 5,500–7,499 | Top-1000 leaderboard entry |
| Emperor | 7,500+ | Global NPC & friends leaderboard |

**Renown formula (attack & defence):**
```
Expected = 1 / (1 + 10^((Opp_Renown - My_Renown) / 400))
Win  : +round(K * (1 - Expected))      K = 40
Loss : -round(K * Expected)            (floor Renown at 0)
Stars bonus: +0 / +2 / +4 for 1★/2★/3★ (attack only)
Defence success (attacker ≤ 1★): +round(0.5 * K * (1-Expected))
Defence failure: -round(0.5 * K * Expected)
```
- **Seasons** last 4 weeks: ranks soft-reset (compress toward 3,000), rewards by final rank.

### 10.4 Commander / Hero (Castle unlock)
One Commander (e.g., "The Lionheart", "The Khan" — original names) can be assigned to your army or garrisoned in the Castle for defence. Has a unique ability and levels with Renown. Optional but planned for Phase 2.

---

## 11. AI Raids on the Player

- **Raid scheduler** picks an NPC lord with Renown within ±15% of yours and ≥ TC-level parity. Raids occur **every 8–36 hours** (based on how active the player is; fewer when you're offline for days).
- **Warning:** 5-minute push notification + in-app banner ("Lord Aldric's raiders march on your village!"). Player can use this time to **garrison the TC/Castle** and **heal/repair**.
- **Resolution:** the raid is simulated with the same deterministic combat engine used in offence; the player can watch the **replay**.
- **Loot taken:** same formula as §9.5, capped at 20% per resource and **capped at 40% per 24 h**. TC-protected resources are safe.
- **Shields:** after a raid, you receive a **shield** (6 h for 1★, 12 h for 2★, 16 h for 3★). Attacking while shielded removes it (CoC-style).
- **Defender's reward:** survive an NPC raid (attacker gets ≤ 1★) → gain Renown + a small loot bonus.
- **Raid report** screen includes the replay, what was destroyed, and **"Suggested fix"** hints (e.g., "Your east side has no archers' cover").
- **Anti-frustration:** no raids during FTUE; max 3 raids / day; **"Peace Pact"** item to pause raids for 24 h.

---

## 12. Progression Summary

| TC | New Defences | New Buildings | New Units | Notes |
|---|---|---|---|---|
| 1 | Watch Tower | House, Lumber/Mining Camp, Farm, Barracks, Army Camp, Storehouse | Militia | Dark Age |
| 2 | Palisade | Mill | Militia (upgrade 2) | Prepare for Feudal |
| 3 | Guard Tower, Stone Wall | Blacksmith, Archery Range, Stable | Man-at-Arms, Archer, Skirmisher, Scout | **Feudal Age** |
| 4 | Spike Trap | Market, Command Tent | Heal Prayer spell | |
| 5 | Keep | Siege Workshop, Monastery | Knight, Crossbow, Long Sword, Monk, Ram, Mangonel | **Castle Age** |
| 6 | Fortified Wall, Oil Trap, Mangonel Emplacement | Castle (Garrison) | | |
| 7 | Scorpion | University | Unit level-4 upgrades | |
| 8 | Bombard Tower | | Champion, Paladin, Arbalester, Trebuchet, Hand Cannoneer | **Imperial Age** |
| 9 | Tower upgrades | | Bombard Cannon | |
| 10 | Max levels | | Final unit upgrades | |

---

## 13. UI / UX & Controls

### 13.1 Screens
1. **Village (home):** base view, resource bar (top), villager count, build menu (bottom-left), raid button (bottom-right), Renown/Rank (top-left), notifications.
2. **Build / Shop:** categories (Economy, Military, Defence, Decorations), grayed items show required TC level.
3. **Army screen:** training queue, camp capacity, upgrades.
4. **Raid Map:** list of barbarian camps + "Find Rival".
5. **Battle:** troop bar bottom, ability bar, timer, % destroyed and stars top.
6. **Reports & Replays.**
7. **Ranked Ladder:** your position among nearby lords.
8. **Settings, Help, Store.**

### 13.2 Controls
| Action | Mobile | PC |
|---|---|---|
| Pan | One-finger drag | Right-drag / WASD / edge scroll |
| Zoom | Pinch | Mouse wheel |
| Select | Tap | Left click |
| Deploy troops | Tap/hold on deploy edge | Click / hold; hotkeys 1–9 |
| Cancel/Back | Back gesture | Esc |

One input abstraction layer supports touch, mouse and optional gamepad.

### 13.3 Accessibility
Colour-blind modes, UI scale 80–150%, subtitle-first audio cues, haptic toggle.

---

## 14. Monetisation & Retention

> The design target is **fair, optional spending**. Since opponents are NPCs, spending cannot harm any other player.

- **Premium currency: "Crowns"** — speed up builds/training, buy extra builder villagers (up to +2), instant refill of army.
- **Season Pass** (cosmetics, banners, extra daily chests).
- **Cosmetics:** building skins, unit skins, village themes, banners.
- **Optional rewarded ads** (mobile): double a raid's loot, free speed-up of 5 min.
- **No pay-to-win unit sales**; units and techs can only be earned in-game.
- **Retention:** daily login chest, daily quests (e.g., "Win 2 raids"), achievements, weekly siege event, season ladders, collection book of lords.
- **PC version:** Premium one-time purchase option (no ads) with Crowns grants.

---

## 15. Art, Audio & Tone

- **Visual style:** isometric 2D, hand-painted/pixel-hybrid with a rich, readable medieval palette. Each Age has its own **building tint/material set** so progress is visible at a glance (thatch/wood → timber-and-stone → castle stone → marble/banners).
- **Villagers:** animated work loops (chop, mine, farm, build); units have attack/idle/death animations with 4–8 facing directions.
- **Audio:** original orchestral/lute medieval score; ambient village SFX (birds, anvil, mill); distinct raid horn; unit grunts. Age-up fanfare. *No use of AoE audio assets.*
- **Tone:** light-hearted but grounded; no gore. Defeated units fade in a puff of dust.

---

## 16. Live-Ops & Content Roadmap (post-launch)

- Weekend **Siege Events** vs. giant boss lords.
- **Seasonal cosmetics** (Winter Siege, Harvest Festival).
- **New lords** and barbarian chapters each month.
- **Commander** roster expansion.
- **Optional online layer (v2):** friends' bases, real-player ghost bases (async), clan features. The v1 data model leaves room for server-hosted bases.

---

## 17. Technical Design

### 17.1 Engine & Stack
- **Engine (decided):** custom **C++17** on **SDL2 + SDL2_ttf**, built with CMake. One codebase builds to **Windows (x64, static exe)** and **Android (arm64-v8a / x86_64 via Gradle + NDK)**. No Unity, no Godot. The game is **fully offline**: no network code and no INTERNET permission in the Android manifest.
- **Data-driven design:** all buildings, units, techs, lords and loot tables defined in JSON/ScriptableObjects → no hard-coded balance.
- **Save:** local encrypted JSON/Protobuf with versioned migrations; cloud save via Google Play Games (Android) / Steam Cloud or file (PC).
- **Backend (v1):** none required. Optional lightweight BaaS (e.g., PlayFab/Firebase) for cloud save, analytics, remote config, and push notifications.

### 17.2 Simulation Architecture
- **Deterministic combat simulation:** fixed-timestep (30 Hz), seeded RNG, no floating-point nondeterminism where avoidable (fixed-point maths for damage/movement targeting). The same sim runs live battles, NPC raids, and replays (replay = seed + inputs).
- **Offline economy:** resource production computed from `lastSeen` timestamp (clamped to 8 h) rather than per-frame simulation. Use server time when online to deter clock cheating; otherwise local with soft-anti-cheat.
- **Pathfinding:** grid A\* with flow-fields for large crowds; wall-aware.
- **NPC base generation:** template-based layout generator using TC-level defence budgets, personality weights and a validity pass (no overlaps, reachable TC).

### 17.3 Performance Targets
| Metric | Target |
|---|---|
| Frame rate | 60 FPS PC; 30–60 FPS mobile |
| Min mobile spec | 3 GB RAM, Snapdragon 660-class |
| Battle units on screen | ≤ 150 units, ≤ 60 defence entities |
| APK / AAB size | < 150 MB base (rest as Addressables) |
| Cold start | < 6 s on mid-range phone |
| Battery | < 6%/10 min of battle on reference device |

### 17.4 Analytics & Anti-Cheat
Events: FTUE steps, session length, raid win rates by TC level, resource sinks, upgrade abandonment. Local save tamper detection with checksums; server-validated purchases.

### 17.5 Localisation
EN at launch; string tables for ES, FR, DE, PT-BR, RU, ZH-CN, JA, KO later.

---

## 18. Development Roadmap

| Milestone | Duration (est., 3–5 person team) | Deliverable |
|---|---|---|
| **M0 Pre-production** | 4 weeks | GDD signed off, art direction, tech spike (iso grid, 100 units battle) |
| **M1 Prototype** | 8 weeks | Home base build/upgrade, villagers gather, one barracks, militia raid vs one barbarian camp, save/load |
| **M2 Vertical slice** | 10 weeks | TC 1–3 complete, Dark → Feudal age-up, ranked loop vs 20 NPC lords, AI raids on base, first art pass |
| **M3 Alpha** | 14 weeks | TC 1–7 content, all unit types to Castle, Barbarian campaign 1–20, Renown/ranks, tutorial, analytics |
| **M4 Beta** | 10 weeks | TC 8–10 (Imperial), full roster, seasons, soft-launch Android in test regions, PC build |
| **M5 Launch** | 6 weeks | Balance pass, optimisation, store assets, localisation, certification |
| **Post-launch** | ongoing | Live-ops (§16) |

---

## 19. Risks & Mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| IP / trademark (AoE) | Legal | Original art/name/audio (§1.3) |
| NPC opponents feel "fake" | Retention | Procedural personalities, drifting Renown, revenge system, replays, named lords |
| Economy inflation / grind walls | Churn | Telemetry-driven tuning, loot caps, barbarian catch-up loot |
| Deterministic sim complexity | Schedule | Prototype in M0; keep unit count capped |
| Scope creep (Commander, clans, online) | Schedule | Phase gating (§16) |
| Mobile performance in big battles | UX | Pooling, sprite atlases, fixed unit caps, LOD for shadows/effects |
| Offline-only exploit (clock change) | Economy | Server time when online, soft clamps |

---

## 20. Open Questions (to decide before M1)

1. ~~Engine~~ — decided: native C++ / SDL2 (see §17.1).
2. **Art style:** hand-painted isometric vs pixel art? Budget and outsourcing?
3. **Online:** purely offline v1, or ghost-bases of real players from day one?
4. **Premium currency:** keep it at launch or go premium-only on PC?
5. **Civilisations:** cosmetic-only, or do they offer unique units/bonuses?
6. **Heroes (Commanders):** Phase 1 or Phase 2?
7. **Walls:** include full wall-building (CoC-style) or auto-wall from palisade pieces to simplify mobile UX?

---

## Appendix A — Sample Balance Formulas

```
Villager gather rate (per min):   wood 25, food 20 (farm), gold 20, stone 18
Camp level multiplier:            1.0 + 0.10 × (CampLevel - 1)
Eco tech multiplier:              product of researched techs (each 1.10–1.25)
Upgrade cost curve:               Cost(L) = Base × 1.6^(L-1)
Upgrade time curve:               Time(L) = BaseTime × 1.9^(L-1)
Unit HP scaling per upgrade level: +8%, attack +6%
Defence DPS scaling per level:     +12%
Loot cap vs TC level:             LootCap = 800 × 1.9^(TC-1)  (per resource)
Raid frequency (hrs):             clamp(36 - 4 × SessionsLast3Days, 8, 36)
```

## Appendix B — Example Day-1 Player Journey (TC1→TC2)
1. Chop wood → build 2nd House → train villager #4.
2. Build Farm + Barracks → train 6 militia.
3. Beat Barbarian Camp 1 (loot 300 W, 150 G) → Renown +10.
4. Upgrade Watch Tower → upgrade TC to Level 2 (2 min).
5. Shield remains active for 3 days; player explores Market preview and rank screen.
