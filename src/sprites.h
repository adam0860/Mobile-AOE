// Sprite atlas loading + drawing (sprites are Blender renders, see art/ and tools/pack_atlas.py)
#pragma once
#include <SDL.h>
#include <string>
#include "defs.h"

struct SpriteDef { int atlas, x, y, w, h; float ax, ay, scale; };

void setEnemyTeamColor(uint32_t rgb);   // recolours the 'red team' unit sprites (the enemy lord's colour); takes about a second
void setSpriteTint(uint32_t rgb);   // colour multiplier for the following drawSprite calls (0xffffff = none)
bool loadSprites();                 // false => game falls back to the flat-shaded renderer
bool spritesReady();
// ---- civilizations (cosmetic building sets) and the gold outline
bool loadCivSprites(const char* code, int slot);   // slot 1..6; loads assets/civ_<code>.txt (sprites named <code>_house_1 ...)
void unloadCivSprites(const char* code);
bool civSpritesLoaded(const char* code);
void setOutlineEnabled(bool on);                    // builds / frees the silhouette textures
bool outlineIsEnabled();
void setSpriteOutline(bool mark);                   // when on, drawSprite adds a gold outline (only while the option is enabled)
const SpriteDef* findSprite(const std::string& name);
// Draw so the sprite's ground anchor lands on screen point (sx,sy); zoom = world zoom.  A half-resolution copy is used when zoomed out (no shimmer).
void drawSprite(const SpriteDef& s, float sx, float sy, float zoom, float alpha = 1.f, float scaleMul = 1.f);
// Same, with a horizontal sway of the top edge (trees, bushes) -- amp is a fraction of the sprite height, phase in radians.
void drawSpriteSway(const SpriteDef& s, float sx, float sy, float zoom, float amp, float phase, float scaleMul = 1.f, float alpha = 1.f);
// Directional shadow: the sprite silhouette sheared along the light direction.
void drawSpriteShadow(const SpriteDef& s, float sx, float sy, float zoom, float alpha = 0.3f, float scaleMul = 1.f);
// Additive re-draw of the sprite (hit flash / glow).
void drawSpriteGlow(const SpriteDef& s, float sx, float sy, float zoom, float alpha, float scaleMul = 1.f);
// Draw a sprite as a UI icon centred at (cx,cy), fitted into a size x size box (ignores the world anchor).
void drawIcon(const SpriteDef& s, float cx, float cy, float size, float alpha = 1.f);
