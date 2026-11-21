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
* **Audio**: the 17 sound effects are synthesised at start-up (no sample files). Music is streamed from `assets/music_village.mp3` (home) and `assets/music_raid.mp3` (raids), looped and crossfaded over 1.5 s; if either file is missing the game falls back to synthesised loops. Options has Music / Sound effects toggles. `DawnOfKings.exe --export-audio <folder>` writes the effects as WAV files.
* Save format is v5 (older saves still load).
* Dev keys: **F1** resources/troops, **F2** trigger a raid, **F3** raise the Town Center one level, **Esc** cancel.

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
