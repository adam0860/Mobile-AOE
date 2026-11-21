# Dawn of Kings — M1 Prototype

Playable HTML5 prototype (no dependencies, no build step) of the GDD core loop.

## Run
```
cd prototype
python -m http.server 8123
```
Open http://localhost:8123 (desktop browser, or a phone on the same network). Progress autosaves to localStorage.

## What is in it (GDD §3–§11, TC 1–3)
- Town Center / House / Watch Tower start; Dark Age → Feudal Age (TC Lv3)
- Villagers: build, chop, mine, farm; idle villager accounting; houses raise the cap
- Finite resource nodes (trees, gold, stone) that regrow after 5 min
- Build, upgrade (Lv1–3), build limits and unlocks by TC level
- Barracks (Militia, Man-at-Arms), Archery Range (Archer), Army Camps for housing
- Raids: 5 Barbarian Camps + ranked Rival Lords (procedural bases, personalities), stars, loot, Renown (Elo-style)
- AI raids on your village (warning, live defence battle, shield, loot loss, report), also resolved offline
- Ladder with 300 simulated lords; save/load with 8h offline accumulation
- ⏩ button (top right) speeds up timers ×5 / ×20 for testing

## Not yet implemented
Walls/traps, spells, Castle/Imperial ages, unit upgrades, Market, Commander, audio, real art, seasons.

## Controls
Drag = pan, wheel/pinch = zoom, tap = select / place / deploy.

## Shipping
- Windows: wrap with Electron/Tauri, or keep as browser build.
- Android: wrap with Capacitor (`npx cap add android`) or a Trusted Web Activity.
- Note: the GDD recommends Unity; this prototype is a fast design-validation build. Port the data tables (js/data.js) and sim (js/sim.js) when moving to Unity.
