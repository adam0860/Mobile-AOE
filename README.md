# Dawn of Kings (native C++)

A medieval base-builder / raid game (Age of Empires 2 flavour, Clash of Clans loop). **Single-player, fully offline.**
Design: see [GDD.md](GDD.md). The browser prototype in `prototype/` is the original design test-bed.

* Language: C++17. Libraries: SDL2 + SDL2_ttf (vendored in `third_party/`, statically linked on Windows).
* One codebase for **Windows** and **Android**. No network code, and the Android manifest has **no INTERNET permission**.
* Saves locally (Windows: `%APPDATA%\DawnOfKings\DawnOfKings\save.txt`, Android: app-private storage).

## Layout
```
src/defs.*   data tables (buildings, units), base generator
src/sim.*    battle simulation (player raids, NPC raids, offline raids)
src/game.*   state, economy, raids, ladder, save/load
src/gfx.*    SDL2 drawing layer (polygons, text)
src/app.cpp  input, isometric renderer, UI, --smoke self-test
assets/      font.ttf (Open Sans, SIL OFL)
android/     Gradle project that wraps the same CMake build
```

## Game content implemented
* **Town Center levels 1-7**: Dark (1-2), Feudal (3-4), **Castle Age (5-7)**. Costs are the GDD numbers scaled down for faster play-testing.
* **Castle Age buildings**: Stable (Knights), Siege Workshop (Rams, Mangonels), Monastery (Monks), Keep, Blacksmith (TC3), Market (TC4), Castle garrison (TC6), University (TC7).
* **Castle Age units**: Long Swordsman, Crossbowman, Knight, Monk (heals), Battering Ram (arrow-resistant, buildings only), Mangonel (splash, targets defences).
* **Upgrades**: Blacksmith (Forging, Scale Armor, Fletching) and University (Masonry, Siege Engineers), 3 levels each, one research at a time.
* **Battles**: defenders now fight back (Castle garrisons), Knights and Mangonels prefer defences, Monks heal, rival bases up to TC7 with keeps/castles, NPC raiders bring rams and knights.
* **Imperial Age (TC 8-10)**: marble-and-gold Town Center palace, Bombard Tower (armor-piercing splash cannon), Champion, Arbalester, Paladin, Hand Cannoneer, Trebuchet, Bombard Cannon; Chemistry research; blacksmith/university techs go to level 5; every building has a fourth (Imperial) look.
* **Walls, gates and traps**: Wall (TC2), Gate (TC3), Spike Trap (TC4), Boiling Oil Trap (TC6). Build > Defence, then tap or drag to paint pieces (built instantly; "Switch to Pan" to move the camera). Wall level is shared by all pieces (palisade -> stone -> fortified -> marble) and upgraded in one go from the selected piece's panel; traps likewise. Tap a wall with the Gate tool to turn it into a gate.
* Raiders path around walls through gaps or gates when the detour is short and otherwise breach the wall (rams do double damage to walls, fortified walls resist them); ranged units shoot over walls; you can only deploy troops outside the walls. Walls and traps do not count towards destruction %. Spikes fire once per raid; oil scalds and slows. Rival bases build wall rings with gates and traps.
* **Scorpion Emplacement (TC7)**: a giant crossbow whose single bolt pierces every raider standing in a line behind its target. Rival bases from Castle Age up have them.
* **Seasons**: a season lasts 7 days of game time (Ladder and Options show the countdown; **F4** ends it for testing). At the end Renown above 3000 is halved towards 3000, you get a rank-scaled resource chest (bonus for the top 10 / #1), the lords' ladder is reshuffled, and a summary is shown. Raids won / defences held are tracked per season.
* **Audio**: the 17 sound effects are synthesised at start-up. Music is a rotating playlist of mp3 files in `assets/` (6 village tracks `music_village*.mp3`, 6 battle tracks `music_raid.mp3` / `music_battle_*.mp3`), played through two decks that crossfade: when a track is about to end (or the game switches between village and battle) the next one is picked at random, weighted by situation - village: Dark Age calm start / Feudal-Castle dignified / Imperial grand, daytime busy village, evening-night at night (by your clock); battle: light skirmish, standard raid and big castle siege by battle size, a defending theme when you are attacked, gunpowder-age theme for Imperial lords - and never repeating the last two. Missing files are skipped; with no mp3 at all the synthesised loops are used. `DawnOfKings.exe --export-audio <folder>` writes the effects and synth loops as WAV.
* Save format is v5 (older saves still load).
* Dev keys: **F1** resources/troops, **F2** trigger a raid, **F3** raise the Town Center one level, **Esc** cancel.

## Version 2 content: more raids, exploration, outposts
* **40 barbarian camps** (Raid menu): camps unlock as the Town Center rises (3 at TC1 up to all 40 at TC10) and scale from tents and palisade towers to walled fortresses with keeps, scorpions, castles and bombards. Clearing a camp again pays **50%, then 25%, then 10%** of its loot (the menu shows "cleared xN" and the reduced reward). Camps recover for 3 minutes after a win.
* **Wider deploy zone**: during a raid you may deploy troops up to 7 tiles beyond the map edge (highlighted in yellow), not just on the thin strip inside it. The battle camera now shows the whole zone.
* **Deposits never run out for good**: when a tree/gold/stone deposit is used up it regrows about 2 minutes later in a different part of the village (preferably away from your existing camps). Demolish the old camp (building panel > *Demolish*, refunds half) and build a new one next to the new patch. Deposits in the wilderness regrow elsewhere in the wilderness.
* **Exploration & fog of war**: the world is now 60x60 tiles; your village is the lit 30x30 middle and the rest is hidden by fog. The **Explore** button lets you train **Scouts** (fast cavalry, 60F 30G, max 4) or send an idle **Villager** (slow, killed by wolves). Tap the explorer, then tap the map to send it; *Auto-explore* walks the fog frontier by itself; *Recall* brings it home. Explorers find forests, gold and stone, treasure chests, and wildlife.
* **Hunting**: build a **Mill** near deer, boars or wolves; its hunters gather food from animals within 9 tiles. Boars and wolves fight back and can kill hunters unless towers stand nearby. Animals respawn elsewhere after being hunted out.
* **Outposts are vulnerable**: you may build anything on explored land outside the village. When a raid comes, there is a chance it hits your outposts instead (auto-resolved with the normal battle rules, so towers/keeps/walls there help). Buildings it destroys are **lost forever**, workers inside die, and explorers nearby are caught. Outposts hold no loot, so these raids cost no Renown.
* **Rotate the view**: the `<` / `>` buttons above the minimap (or **Q** / **E**) swivel the camera in 90-degree steps, in the village and in raids, so you can see and build on the far side of your base. Walls re-join correctly in every orientation. (Building sprites keep their art orientation; the world, depth order and minimap rotate.)
* **Workers at a glance**: each resource chip in the top bar shows how many villagers are working on it (wood, food incl. farms and mills, gold, stone), and every camp/farm/mill shows `workers/slots` over its roof (red when nobody works there).
* **Balance pass**: farms yield less (28/min) and lumber camps more (64/min); villagers cost more food (and wood from the 8th) and train slower the more you own, instead of a flat 25 food. The village always stocks plenty of trees (48 at start, more as the Town Center grows; depleted trees regrow elsewhere). **The Dark Age has no gold or stone**: camps and deposits appear at Feudal (TC3), and everything up to Feudal costs only wood and food (militia 25W 40F, barracks and tower wood only).
* **Raids scale with you**: NPC raids grow with your Town Center *and* your Renown (more troops, tougher troops) and vary a lot - roughly half to double the average size, with an occasional big one. Attackers (and rival lords you can raid) are within **two Town Center levels** of yours. Relative level drives the rewards: raiding a higher lord pays up to x1.5 loot and big Renown, a lower lord pays down to x0.5 loot and tiny Renown, and losing to a lower-level opponent costs far more Renown than losing to a higher one. The same applies when defending.
* **Battle replays**: the last 10 battles are saved (Ladder > *Battle replays*, or *Watch replay* on a result screen / the welcome-back screen): your raids, defences, outpost raids and raids that happened while the game was closed. A replay stores the battle's starting setup plus your deploy log, and the battle simulation is deterministic and runs on a fixed time step, so the viewer re-plays it exactly (pause, restart, x1/x2/x4/x8, rotate and zoom the camera). Save format is v7.
* **Troops are spent**: every unit you deploy in a raid is gone afterwards, win or lose, even if it survived; only troops you never deployed stay home. Send the fewest you need.
* **Army presets**: four saved army compositions at the top of the Army menu. *Edit* sets the unit counts (or copies your current army); *Train* queues the whole army at your barracks, ranges, stables and so on, with exactly the same cost and training time as training each unit by hand (it queues as many as you can afford/house and tells you why it stopped). Saved with the game (format v8).
* **Towers outrange archers**: defensive buildings measure their range from their edge (like attackers do), and no archer-type unit (Archer 5, Crossbowman 5.5, Arbalester 6.2) reaches as far as a Watch Tower (6.5) or Keep (8); only siege (Mangonel, Trebuchet, Bombard) outranges towers. **Raid shields** after being raided: 10 minutes at Town Center 1 (to 20 min with 3 stars), rising to 3-6 hours at Town Center 10.
* **Remodelled resource camps** (`art/models_camps.py`): open timber sheds with blue-trimmed plank roofs, in three clearly different colour themes - the lumber camp has a log crane, sawdust and big brown log piles; the stone camp has grey cut-stone stacks and a wheelbarrow on gravel; the gold camp has yellow ingot stacks, a nugget cart and golden sand. Every camp also shows its resource icon next to its level badge.
* **Bigger trees**: each tree holds 400 wood (was 120) so a lumber camp's patch lasts a long time before you need to move it. Older saves are upgraded when loaded.
* **Gathering upgrades**, one per age, researched at the camp itself (only one research runs at a time): Lumber Camp - *Double-Bit Axe* (Feudal), *Bone Saw* (Castle), *Two-Man Saw* (Imperial); Gold Mining Camp - *Gold Mining*, *Gold Shaft Mining*, *Deep Shaft Mining*; Stone Mining Camp - *Stone Mining*, *Stone Shaft Mining*, *Deep Quarrying*; Mill - *Horse Collar*, *Heavy Plow*, *Crop Rotation* (food from farms and hunting). Each is +25%; later ones need the previous one.
* **Pacing and costs**: gold and stone deposits hold twice as much; villagers cost only food; the first Town Center levels take longer (Lv2 80 s, Feudal 200 s, Lv4 300 s) and cost about a third more.
* **Four quadrants and rival lords**: the world is now 120x120 tiles = four quadrants of 60x60, each with a 30x30 home area in its middle. You hold one. At the start a **rival lord** settles at Town Center 1 in another quadrant (the fog hides him until a scout finds his base; his buildings show in red, tap them to see the lord and attack). More lords may settle empty quadrants now and then, each a bit stronger the further you have come, and every lord keeps levelling up. They defend themselves with troops, towers and walls, **and attack you** (the raids on your village and outposts now come from them). Destroyed buildings stay destroyed; **wipe a base out completely and the lord is gone for good** (plus a bonus chest); a new one may settle there later. You can never build inside another quadrant's home area.
* **Outposts**: until Town Center 4 you can only build inside your own quadrant; after that, anywhere you have explored (except lords' home areas). Outside your village there is a **separate limit per building type** that grows with Town Center level (lumber/gold/stone camps, mills, farms, houses, towers, keeps, bombard towers, scorpions, walls, gates, spike and oil traps), and every tower/keep/bombard tower there has a small garrison that fights raiders.
* **Bigger village**: the safe home area grows by 2 tiles on every side at Town Center 3, 5, 7 and 9 (30x30 up to 46x46).
* **Fog of war with memory**: your own quadrant is always live. Elsewhere you only see what your units, outposts (towers and keeps see further) or allies currently watch. Land you have explored but are not watching is dimmed: you remember buildings and resources as you last saw them, but not what has changed since, and animals are only shown where you can see. A rival lord appears on your map (and in the Raid menu) the first time something of yours sees his base; after that you only know his level and defences *as last seen*, so keep scouting - a new lord may have settled where you saw empty land.
* **Diplomacy** (tap a lord's building > *Talk*, or *Talk* next to him in the Raid menu): a lord's attitude depends on his personality - Turtles and Merchants start at peace, Raiders and Warlords at war (and a peace with a Raider/Warlord may be broken). You can **propose peace** (his raiders stop attacking you), **open a trade route** (needs a Market; a Market allows 1 + its level routes; each route pays gold plus wood/food/stone by quadrant every minute), **propose an alliance** (he sends troops to defend your village, pays more trade and shares his view of the map), **send gifts** (build trust), **declare war**, or **attack without warning** - which breaks every treaty with him and makes all lords less willing to deal with you (stackable). Odds are shown as percentages and rise with trust and with how much stronger you are; a refusal means he will not talk for a few minutes.
* **Relics** lie scattered over the world (more appear over time). Train **Monks** at a Monastery, send one out from *Explore*, tap the map beside a relic and he carries it back to the Monastery; each relic held yields gold slowly (9/min each, needs a Monastery). Wolves kill monks (and drop the relic).
* **Restart and prestige**: *Options > Restart the game* wipes your realm and starts again at Town Center 1 in a brand new world with new rival lords (tap twice to confirm; prestige is kept). At Town Center 10 you can **ascend** (also in Options): the realm restarts, your prestige level goes up and you earn a point to spend on permanent unlocks in *Prestige rewards*. The unlock list is a set of placeholder slots for now - the framework (points, saved unlock flags, `prestigeHas(i)` hook) is in place so rewards can be added later.
* **Game icon**: a crowned heater shield over crossed swords, modelled and rendered in Blender (`art/make_icon.py`), then turned into every size by `tools/make_icons.py`: Windows `.ico` (embedded in the exe through `src/app.rc`, plus the window icon), and Android legacy + adaptive launcher icons.
* **Dangerous wildlife**: wolves and boars attack any of your units that come near - villagers, monks, scouts and troops - chasing them (wolves 4.5 tiles, boars 2.5) and biting for real damage (explorers have hit points; villagers and monks run for home when hurt, scouts can usually outrun them). Deer are harmless prey.
* **Field squads** (Army or Explore menu > *Field squads*): form troops from your army into a squad that walks the map in real time. Tap a squad, then tap the map to move it, or choose **Escort** (then tap a villager, monk or scout), **Guard here**, **Attack <lord>** (it marches to his gates and the attack screen opens, with only that squad available to deploy; troops held back return home) or **Recall home**. Squads kill wild animals (meat goes to your food), keep Mill hunters safe, defend nearby outposts when raided (survivors stay alive) and see the land around them. Troops in the field still use housing and, as always, troops you send into a battle do not come back.
* **Lord colours**: every rival lord gets a colour - red, green, yellow, orange, purple or grey - that no other living lord has, and keeps it until he is defeated (a new lord may then take it). His buildings are tinted that colour on the map and in battle, his name, panel, minimap marker and Raid-menu entry use it, and his troops fight in his colour (the unit sprites are recoloured when a battle with a differently coloured lord starts, which takes about a second).
* **Total ruin**: if a neighbouring lord destroys 100% of your village in a raid, you lose **70% of every resource**, and **everything outside your safe zone is lost** - all outpost buildings (and the villagers working there), explorers and squads in the field. Everything inside the safe zone is kept (the village is rebuilt for free as usual). Raids by ladder lords and partial defeats work as before.
* **Volume sliders**: Options has separate *Music volume* and *Effects volume* sliders (drag; saved with the game and kept through restarts) next to the ON/OFF toggles.
* **Hold to deploy**: press and hold (mouse or finger) on the grass during a raid and troops keep dropping every ~0.1 s, slightly spread out, until you release, switch unit type, or run out. A quick tap still drops one; dragging still pans.
* **Battle speed**: a *Speed x1/x2/x4* button (top right) fast-forwards raids and lets you watch a defence at higher speed; the choice sticks between battles.
* **Move buildings**: select a finished building > *Move building* (costs a quarter of its build price), tap the new spot and press *Move here*. Workers and training queues stay with it. Handy for relocating camps next to regrown deposits. (Walls, gates and traps are removed and repainted instead; the Town Center can't be moved.)
* Saves are versioned; older saves still load (v6 added the wilderness, v7 the replays).
* New Blender art: windmill, deer, boar, wolf, treasure chest, scout rider (`art/models_explore.py`).

## Visual overhaul (v2 art + animation + UI)
* **Art**: hand-painted-style procedural materials (UV-mapped planks, masonry, shingles, thatch, stucco) with ambient occlusion; every building has timber framing, eaves, window shutters, striped awnings, packed-earth pads and props; trees are full broadleaf/pine/palm models; the 30x30 map is one lush meadow rendered from 3D grass, flowers and boulders.
* **Units**: jointed rigs posed per frame (8 facings x idle, 6-frame move cycle, 3-frame attack; villagers also hammer / chop / mine / hoe). Horses gallop, wheels roll, trebuchets and mangonels swing. Units are rendered once in blue; the red team is a palette swap done at load time.
* **Animation**: distance-driven walk cycles, facing hysteresis, hit flashes, fading corpses, collapsing buildings, projectile arcs, particles (chimney smoke, dust, sparks, flames, healing motes), tree sway, screen shake, inertial camera pan, smooth zoom, eased counters.
* **UI**: leather-and-gold panels, serif title font, animated buttons, icon command bar, resource banner with icons, a diamond minimap (click or drag to move the camera), icon cards in the build/army menus, animated star results.
* Memory note: the sprite atlases are ~25 MB of PNG and about 150 MB of GPU memory once the red-team copies and half-resolution mip copies are created; fine on PC and recent phones, tight on very old devices.

## Art pipeline (Blender -> sprites)
All world art is procedural and generated by script, so it can be regenerated or restyled:
```powershell
# 1. render isometric sprites (Blender 5.x, headless; ~2 min for everything)
& "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" -b -P art/gen_assets.py -- --out art/renders
#    optional:  --only house_0,townhall_3   or   --only units   or   --engine CYCLES --samples 64
# 2. pack the renders into texture atlases the game loads
python tools/pack_atlas.py        # needs Pillow:  pip install pillow
```
`art/gen_assets.py` = camera/lighting/materials/render driver, `art/models_buildings.py` and `art/models_world.py` = the models
(houses, town center in 3 levels, towers, camps, farms, barracks, trees, ore, 4 unit types x 8 directions x 3 walk frames, ground).
The camera is orthographic at 30 deg elevation / 45 deg azimuth (2:1 dimetric), so sprites line up with the game's 64x32 tile grid.
Shadows are drawn in-game by shearing each sprite's silhouette. If the atlases are missing the game falls back to flat-shaded boxes.

## Build & run — Windows
Requires Visual Studio (C++ workload) and CMake.
```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release --target DawnOfKings
build\bin\Release\DawnOfKings.exe
```
Self-test (scripted playthrough, writes `smoke.log` + screenshots next to the exe, leaves your save alone):
```powershell
build\bin\Release\DawnOfKings.exe --smoke
```
Dev keys in game: **F1** = +resources/+6 militia, **F2** = trigger an NPC raid, **Esc** = cancel.

## Build — Android
Requires Android Studio's JDK, the Android SDK (platform 35, build-tools, NDK 28, CMake 3.22).
```powershell
cd android
$env:JAVA_HOME="C:\Program Files\Android\Android Studio\jbr"
.\gradlew.bat assembleDebug
```
APK: `android/app/build/outputs/apk/debug/app-debug.apk`. Install with `adb install -r`.

## Controls
Drag = pan, wheel / pinch = zoom, tap = select / place / deploy troops.
