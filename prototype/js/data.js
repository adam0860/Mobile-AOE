// ===== Dawn of Kings — prototype data (M1: TC 1-3, Dark -> Feudal) =====
const TW = 64, TH = 32, MAP = 30;
const RES = ['wood', 'food', 'gold', 'stone'];
const ICON = { wood: '🪵', food: '🌾', gold: '🪙', stone: '🪨' };
const AGES = ['Dark Age', 'Feudal Age', 'Castle Age', 'Imperial Age'];
const ageOf = l => (l <= 2 ? 0 : l <= 4 ? 1 : l <= 7 ? 2 : 3);
const MAXLVL = 3;
const RANKS = [['Peasant', 0], ['Squire', 200], ['Knight', 600], ['Baron', 1200], ['Earl', 2000], ['Duke', 3000], ['Prince', 4200], ['King', 5500], ['Emperor', 7500]];

// type -> definition. Arrays are indexed by (level-1).
const BDEF = {
  townhall: { name: 'Town Center', icon: '🏰', w: 3, h: 3, color: '#b5894f', hgt: 44, hp: [1500, 1900, 2500], dps: [10, 13, 17], range: 6, hq: true, cap: [2000, 4000, 6000], prot: [200, 400, 800], noBuild: true },
  house: { name: 'House', icon: '🏠', w: 2, h: 2, color: '#a8794a', hgt: 22, hp: [250, 350, 450], unlock: 1, max: [2, 4, 6], cost: { wood: 100 }, time: 8, cat: 'Economy', desc: '+5 villager capacity' },
  lumber: { name: 'Lumber Camp', icon: '🪓', w: 2, h: 2, color: '#8a6a3a', hgt: 16, hp: [300, 400, 500], unlock: 1, max: [1, 2, 3], cost: { wood: 80 }, time: 8, cat: 'Economy', res: 'wood', node: 'tree', rate: 50, slots: [3, 4, 5], desc: 'Villagers chop nearby trees' },
  goldcamp: { name: 'Gold Mining Camp', icon: '⛏️', w: 2, h: 2, color: '#9a7d2e', hgt: 16, hp: [300, 400, 500], unlock: 1, max: [1, 1, 2], cost: { wood: 100 }, time: 10, cat: 'Economy', res: 'gold', node: 'gold', rate: 32, slots: [3, 4, 5], desc: 'Villagers mine nearby gold' },
  stonecamp: { name: 'Stone Mining Camp', icon: '⛏️', w: 2, h: 2, color: '#7b7f86', hgt: 16, hp: [300, 400, 500], unlock: 1, max: [1, 1, 2], cost: { wood: 100 }, time: 10, cat: 'Economy', res: 'stone', node: 'stone', rate: 30, slots: [3, 4, 5], desc: 'Villagers mine nearby stone' },
  farm: { name: 'Farm', icon: '🌾', w: 2, h: 2, color: '#c8b04a', hgt: 4, hp: [150, 200, 250], unlock: 1, max: [2, 4, 6], cost: { wood: 100 }, time: 8, cat: 'Economy', res: 'food', rate: 40, slots: [1, 1, 1], desc: 'One farmer produces food' },
  barracks: { name: 'Barracks', icon: '⚔️', w: 3, h: 3, color: '#8c4a3a', hgt: 30, hp: [500, 700, 900], unlock: 1, max: [1, 1, 2], cost: { wood: 200, stone: 50 }, time: 20, cat: 'Military', desc: 'Trains Militia (and Men-at-Arms in Feudal)' },
  armycamp: { name: 'Army Camp', icon: '⛺', w: 3, h: 3, color: '#6f8a4a', hgt: 12, hp: [300, 400, 500], unlock: 1, max: [1, 2, 3], cost: { wood: 150 }, time: 15, cat: 'Military', desc: '+20 army housing' },
  archery: { name: 'Archery Range', icon: '🏹', w: 3, h: 3, color: '#4a7a5a', hgt: 28, hp: [500, 700, 900], unlock: 3, max: [0, 0, 1], cost: { wood: 300, gold: 100 }, time: 25, cat: 'Military', desc: 'Trains Archers (Feudal Age)' },
  tower: { name: 'Watch Tower', icon: '🗼', w: 2, h: 2, color: '#8e8a80', hgt: 46, hp: [300, 420, 560], dps: [12, 16, 21], range: 6.5, unlock: 1, max: [1, 2, 4], cost: { wood: 100, stone: 80 }, time: 15, cat: 'Defence', desc: 'Fires arrows at raiders' },
  // enemy-only
  storehouse: { name: 'Storehouse', icon: '📦', w: 2, h: 2, color: '#9a7a4a', hgt: 18, hp: [400, 600, 800], noBuild: true },
  tent: { name: 'Barbarian Tent', icon: '⛺', w: 2, h: 2, color: '#7a5a3a', hgt: 16, hp: [200, 200, 200], noBuild: true },
  chief: { name: "Chieftain's Hall", icon: '🛖', w: 3, h: 3, color: '#6a4a2a', hgt: 30, hp: [700, 700, 700], hq: true, noBuild: true },
};

const TCUP = {
  2: { cost: { wood: 400, food: 300 }, time: 30 },
  3: { cost: { wood: 1200, food: 800, gold: 400 }, time: 90, label: 'Advance to Feudal Age' },
};

const UDEF = {
  militia: { name: 'Militia', icon: '🗡️', hp: 60, dmg: 8, rate: 1, range: 0.9, speed: 1.7, cost: { food: 40, gold: 20 }, time: 8, pop: 1, unlock: 1, bld: 'barracks', ranged: false, color: '#d9b45a' },
  manatarms: { name: 'Man-at-Arms', icon: '🛡️', hp: 130, dmg: 12, rate: 1, range: 0.9, speed: 1.5, cost: { food: 60, gold: 20 }, time: 12, pop: 1, unlock: 3, bld: 'barracks', ranged: false, color: '#9aa3b5' },
  archer: { name: 'Archer', icon: '🏹', hp: 40, dmg: 7, rate: 1, range: 5, speed: 1.7, cost: { wood: 25, gold: 45 }, time: 10, pop: 1, unlock: 3, bld: 'archery', ranged: true, color: '#5fae6a' },
};

const LORD_NAMES = ['Aldric', 'Bertrand', 'Cedric', 'Dagobert', 'Eadric', 'Fulk', 'Godfrey', 'Harold', 'Ingvar', 'Jorund', 'Kazimir', 'Leofric', 'Mathis', 'Osric', 'Piers', 'Ragnar', 'Saladin', 'Temur', 'Ulric', 'Wulfstan', 'Yaroslav', 'Zoran', 'Alaric', 'Baldwin', 'Conrad', 'Dietrich', 'Edmund', 'Frederick', 'Gunther', 'Hugo'];
const LORD_TITLES = ['the Bold', 'the Grim', 'the Fair', 'the Red', 'of the Marsh', 'Ironhand', 'the Wise', 'Stormborn', 'the Pale', 'Wolfsbane'];
const PERSONALITIES = ['Turtle', 'Raider', 'Merchant', 'Warlord'];

// ----- helpers -----
function rng(seed) { let a = seed >>> 0; return () => { a |= 0; a = (a + 0x6D2B79F5) | 0; let t = Math.imul(a ^ (a >>> 15), 1 | a); t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t; return ((t ^ (t >>> 14)) >>> 0) / 4294967296; }; }
const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
function rankOf(r) { let k = RANKS[0][0]; for (const [n, v] of RANKS) if (r >= v) k = n; return k; }
function expected(my, opp) { return 1 / (1 + Math.pow(10, (opp - my) / 400)); }
function fmtTime(s) { s = Math.max(0, Math.ceil(s)); if (s >= 3600) return Math.floor(s / 3600) + 'h ' + Math.floor((s % 3600) / 60) + 'm'; if (s >= 60) return Math.floor(s / 60) + 'm ' + (s % 60) + 's'; return s + 's'; }

function makeSimB(type, lvl, x, y) {
  const d = BDEF[type];
  const hp = d.hp[lvl - 1];
  return { type, lvl, x, y, w: d.w, h: d.h, hp, maxhp: hp, dps: d.dps ? d.dps[lvl - 1] : 0, range: d.range || 0, cd: 0, dead: false, hq: !!d.hq };
}

// Generate an enemy base. kind: 'barb' (level 1-5) or 'rival' (tc 1-3)
function genBase(seed, tc, kind, level) {
  const r = rng(seed), bs = [], occ = new Set();
  const free = (x, y, w, h) => { if (x < 2 || y < 2 || x + w > MAP - 2 || y + h > MAP - 2) return false; for (let i = x - 1; i < x + w + 1; i++) for (let j = y - 1; j < y + h + 1; j++) if (occ.has(i + ',' + j)) return false; return true; };
  const put = (type, lvl, x, y) => { const d = BDEF[type]; if (!free(x, y, d.w, d.h)) return false; for (let i = x; i < x + d.w; i++) for (let j = y; j < y + d.h; j++) occ.add(i + ',' + j); bs.push(makeSimB(type, lvl, x, y)); return true; };
  const scatter = (type, lvl, rmin, rmax) => { for (let k = 0; k < 80; k++) { const a = r() * Math.PI * 2, rad = rmin + r() * (rmax - rmin); const d = BDEF[type]; if (put(type, lvl, Math.round(15 + Math.cos(a) * rad - d.w / 2), Math.round(15 + Math.sin(a) * rad - d.h / 2))) return true; } return false; };
  let loot, name, pers = null;
  if (kind === 'barb') {
    put('chief', 1, 14, 14);
    const tents = 2 + level, towers = Math.floor(level / 1.5);
    for (let i = 0; i < tents; i++) scatter('tent', 1, 4, 8);
    for (let i = 0; i < towers; i++) scatter('tower', level >= 4 ? 2 : 1, 3.5, 7);
    loot = { wood: 250 + 220 * level, food: 100 + 100 * level, gold: 120 + 150 * level, stone: 80 + 100 * level };
    name = 'Barbarian Camp ' + level;
  } else {
    put('townhall', tc, 14, 14);
    pers = PERSONALITIES[Math.floor(r() * 4)];
    let houses = 3 + tc * 2, towers = tc + 1, stores = 2 + tc;
    if (pers === 'Turtle') { towers += 2; houses -= 1; }
    if (pers === 'Merchant') { stores += 2; towers -= 1; }
    if (pers === 'Raider') { houses += 1; }
    for (let i = 0; i < towers; i++) scatter('tower', Math.min(tc, 3), 3.5, 7);
    for (let i = 0; i < stores; i++) scatter('storehouse', 1, 4, 9);
    for (let i = 0; i < houses; i++) scatter('house', 1, 4, 10);
    const base = 500 * Math.pow(tc, 1.6) * (pers === 'Merchant' ? 1.4 : 1) * (0.8 + r() * 0.4);
    loot = { wood: Math.round(base), food: Math.round(base * 0.7), gold: Math.round(base * 0.55), stone: Math.round(base * 0.4) };
  }
  return { buildings: bs, loot, name, pers };
}
