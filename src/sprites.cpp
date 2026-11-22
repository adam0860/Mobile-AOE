#include "sprites.h"
#include <cstdio>
#include <sstream>
#include <unordered_map>
#include <vector>
#include "gfx.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "../third_party/stb/stb_image.h"

struct Page { SDL_Texture* t[2] = {nullptr, nullptr}; int w[2] = {1, 1}, h[2] = {1, 1}; };
static std::vector<Page> pages;
static std::unordered_map<std::string, SpriteDef> defs;
static bool ready = false;

static std::string assetPath(const std::string& name) {
#ifdef __ANDROID__
  return name;
#else
  char* bp = SDL_GetBasePath(); std::string base = bp ? bp : ""; if (bp) SDL_free(bp);
  return base + "assets/" + name;
#endif
}

static SDL_Texture* makeTex(const unsigned char* px, int w, int h) {
  SDL_Texture* t = SDL_CreateTexture(G.r, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, w, h);
  if (t) { SDL_UpdateTexture(t, nullptr, px, w * 4); SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND); SDL_SetTextureScaleMode(t, SDL_ScaleModeLinear); }
  return t;
}
// 2x2 box filter on premultiplied alpha (no dark fringes)
static std::vector<unsigned char> halve(const unsigned char* px, int w, int h, int& ow, int& oh) {
  ow = std::max(1, w / 2); oh = std::max(1, h / 2); std::vector<unsigned char> o((size_t)ow * oh * 4);
  for (int y = 0; y < oh; y++) for (int x = 0; x < ow; x++) {
    float r = 0, g = 0, b = 0, a = 0;
    for (int k = 0; k < 4; k++) {
      int sx = std::min(w - 1, x * 2 + (k & 1)), sy = std::min(h - 1, y * 2 + (k >> 1)); const unsigned char* p = px + ((size_t)sy * w + sx) * 4;
      float pa = p[3] / 255.f; r += p[0] * pa; g += p[1] * pa; b += p[2] * pa; a += pa;
    }
    unsigned char* q = &o[((size_t)y * ow + x) * 4];
    if (a > 1e-4f) { q[0] = (unsigned char)(r / a); q[1] = (unsigned char)(g / a); q[2] = (unsigned char)(b / a); q[3] = (unsigned char)(a * 0.25f * 255.f); } else q[0] = q[1] = q[2] = q[3] = 0;
  }
  return o;
}
// red team: the blue cloth/plume/caparison pixels become crimson (units are rendered once, in blue)
static void swapTeam(unsigned char* px, int w, int h) {
  for (size_t i = 0, n = (size_t)w * h; i < n; i++) {
    unsigned char* p = px + i * 4; int r = p[0], g = p[1], b = p[2];
    if (p[3] > 8 && b > 55 && b > r * 1.45f && b > g * 1.2f) { p[0] = (unsigned char)std::min(255, (int)(b * 1.02f)); p[1] = (unsigned char)(g * 0.5f); p[2] = (unsigned char)std::min(255, (int)(r * 0.8f + g * 0.1f)); }
  }
}
static std::vector<std::pair<int, std::string>> unitFiles;       // unit atlas pages (index, file) so the enemy team can be recoloured later
static void swapTeamColor(unsigned char* px, int w, int h, uint32_t rgb) {
  float tr = (float)((rgb >> 16) & 255), tg = (float)((rgb >> 8) & 255), tb = (float)(rgb & 255);
  for (size_t i = 0, n = (size_t)w * h; i < n; i++) {
    unsigned char* p = px + i * 4; int r = p[0], g = p[1], b = p[2];
    if (p[3] > 8 && b > 55 && b > r * 1.45f && b > g * 1.2f) { float lum = b / 255.f; p[0] = (unsigned char)std::min(255.f, tr * lum + 0.45f * g); p[1] = (unsigned char)std::min(255.f, tg * lum + 0.45f * g); p[2] = (unsigned char)std::min(255.f, tb * lum + 0.45f * g); }
  }
}
void setEnemyTeamColor(uint32_t rgb) {
  for (auto& uf : unitFiles) {
    size_t n = 0; void* data = SDL_LoadFile(assetPath(uf.second).c_str(), &n); if (!data) continue;
    int w, h, c; unsigned char* px = stbi_load_from_memory((const unsigned char*)data, (int)n, &w, &h, &c, 4); SDL_free(data); if (!px) continue;
    swapTeamColor(px, w, h, rgb); int ridx = uf.first + 1000; if (ridx >= (int)pages.size() || !pages[ridx].t[0]) { stbi_image_free(px); continue; }
    Page& rp = pages[ridx]; SDL_UpdateTexture(rp.t[0], nullptr, px, w * 4);
    int hw, hh; auto half = halve(px, w, h, hw, hh); if (rp.t[1]) SDL_UpdateTexture(rp.t[1], nullptr, half.data(), hw * 4);
    stbi_image_free(px);
  }
}
static bool loadPage(int idx, const std::string& file, bool units) {
  size_t n = 0; void* data = SDL_LoadFile(assetPath(file).c_str(), &n);
  if (!data) { std::fprintf(stderr, "missing asset %s\n", file.c_str()); return false; }
  int w, h, c; unsigned char* px = stbi_load_from_memory((const unsigned char*)data, (int)n, &w, &h, &c, 4); SDL_free(data);
  if (!px) { std::fprintf(stderr, "bad png %s\n", file.c_str()); return false; }
  if ((int)pages.size() <= idx) pages.resize(idx + 1);
  Page& pg = pages[idx]; pg.t[0] = makeTex(px, w, h); pg.w[0] = w; pg.h[0] = h;
  int hw, hh; auto half = halve(px, w, h, hw, hh);
  pg.t[1] = makeTex(half.data(), hw, hh); pg.w[1] = hw; pg.h[1] = hh;
  if (units) unitFiles.push_back({idx, file});
  if (units) {                                                // red variant lives on a second page index
    swapTeam(px, w, h);
    int ridx = idx + 1000; if ((int)pages.size() <= ridx) pages.resize(ridx + 1);
    Page& rp = pages[ridx]; rp.t[0] = makeTex(px, w, h); rp.w[0] = w; rp.h[0] = h;
    auto half2 = halve(px, w, h, hw, hh);
    rp.t[1] = makeTex(half2.data(), hw, hh); rp.w[1] = hw; rp.h[1] = hh;
  }
  stbi_image_free(px); return true;
}

bool loadSprites() {
  size_t n = 0; char* txt = (char*)SDL_LoadFile(assetPath("atlas.txt").c_str(), &n); if (!txt) return false;
  std::istringstream in(std::string(txt, n)); SDL_free(txt);
  std::string line; std::vector<int> unitPages;
  std::vector<std::pair<std::string, SpriteDef>> entries;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    std::istringstream ls(line);
    if (line[0] == '@') {                                   // "@ <page> <file> [units]"
      std::string at, file, tag; int idx; ls >> at >> idx >> file >> tag;
      bool units = tag == "units"; if (!loadPage(idx, file, units)) return false; if (units) unitPages.push_back(idx);
    } else {
      std::string name; SpriteDef d; ls >> name >> d.atlas >> d.x >> d.y >> d.w >> d.h >> d.ax >> d.ay >> d.scale; if (ls) entries.push_back({name, d});
    }
  }
  for (auto& e : entries) {
    defs[e.first] = e.second;
    for (int up : unitPages) if (e.second.atlas == up) {         // mirror every unit sprite for the red team
      std::string rn = e.first; size_t p = rn.find("_blue_"); if (p != std::string::npos) { rn.replace(p, 6, "_red_"); SpriteDef r = e.second; r.atlas = up + 1000; defs[rn] = r; }
    }
  }
  ready = !defs.empty() && !pages.empty(); return ready;
}
bool spritesReady() { return ready; }
const SpriteDef* findSprite(const std::string& name) { auto it = defs.find(name); return it == defs.end() ? nullptr : &it->second; }

struct LodPick { SDL_Texture* t; float k; int lod; };
static LodPick pick(const SpriteDef& s, float zoom, float scaleMul) {
  Page& p = pages[s.atlas]; float k = zoom * scaleMul / s.scale;
  int lod = (k < 0.62f && p.t[1]) ? 1 : 0; return {p.t[lod], k, lod};
}
static SDL_Rect srcRect(const SpriteDef& s, int lod) {
  if (lod == 0) return {s.x, s.y, s.w, s.h};
  return {s.x / 2, s.y / 2, std::max(1, s.w / 2), std::max(1, s.h / 2)};
}

static Uint8 tintR = 255, tintG = 255, tintB = 255;
void setSpriteTint(uint32_t rgb) { tintR = (rgb >> 16) & 255; tintG = (rgb >> 8) & 255; tintB = rgb & 255; }
void drawSprite(const SpriteDef& s, float sx, float sy, float zoom, float alpha, float scaleMul) {
  LodPick lp = pick(s, zoom, scaleMul); SDL_Rect src = srcRect(s, lp.lod); float k = lp.k;
  SDL_FRect dst{sx - s.ax * k, sy - s.ay * k, s.w * k, s.h * k};
  SDL_SetTextureAlphaMod(lp.t, (Uint8)(clampf(alpha, 0, 1) * 255));
  SDL_SetTextureColorMod(lp.t, tintR, tintG, tintB);
  SDL_RenderCopyF(G.r, lp.t, &src, &dst);
  if (tintR != 255 || tintG != 255 || tintB != 255) SDL_SetTextureColorMod(lp.t, 255, 255, 255);
}

void drawSpriteSway(const SpriteDef& s, float sx, float sy, float zoom, float amp, float phase, float scaleMul, float alpha) {
  LodPick lp = pick(s, zoom, scaleMul); Page& pg = pages[s.atlas]; int lod = lp.lod; float k = lp.k;
  float W = (float)pg.w[lod], H = (float)pg.h[lod]; SDL_Rect sr = srcRect(s, lod);
  float u0 = sr.x / W, v0 = sr.y / H, u1 = (sr.x + sr.w) / W, v1 = (sr.y + sr.h) / H;
  float x0 = sx - s.ax * k, y0 = sy - s.ay * k, x1 = x0 + s.w * k, y1 = y0 + s.h * k, hgt = s.ay * k;
  float sway = std::sin(phase) * amp * hgt;                 // top edge moves, the base stays planted
  SDL_Color c{255, 255, 255, (Uint8)(clampf(alpha, 0, 1) * 255)}; SDL_Vertex v[4];
  v[0].position = {x0 + sway, y0}; v[1].position = {x1 + sway, y0}; v[2].position = {x1, y1}; v[3].position = {x0, y1};
  float uu[4] = {u0, u1, u1, u0}, vv[4] = {v0, v0, v1, v1};
  for (int i = 0; i < 4; i++) { v[i].color = c; v[i].tex_coord = {uu[i], vv[i]}; }
  int idx[6] = {0, 1, 2, 0, 2, 3};
  SDL_SetTextureAlphaMod(lp.t, 255);
  SDL_RenderGeometry(G.r, lp.t, v, 4, idx, 6);
}

void drawSpriteShadow(const SpriteDef& s, float sx, float sy, float zoom, float alpha, float scaleMul) {
  LodPick lp = pick(s, zoom, scaleMul); Page& pg = pages[s.atlas]; int lod = lp.lod; float k = lp.k;
  float W = (float)pg.w[lod], H = (float)pg.h[lod]; SDL_Rect sr = srcRect(s, lod);
  float u0 = sr.x / W, v0 = sr.y / H, u1 = (sr.x + sr.w) / W, v1 = (sr.y + sr.h) / H;
  float cx[4] = {-s.ax * k, (s.w - s.ax) * k, (s.w - s.ax) * k, -s.ax * k};
  float cy[4] = {-s.ay * k, -s.ay * k, (s.h - s.ay) * k, (s.h - s.ay) * k};
  SDL_Vertex v[4]; SDL_Color col{0, 0, 0, (Uint8)(clampf(alpha, 0, 1) * 255)};
  float uu[4] = {u0, u1, u1, u0}, vv[4] = {v0, v0, v1, v1};
  for (int i = 0; i < 4; i++) {
    float h = -cy[i];                        // height above the anchor line in screen px
    if (h >= 0) v[i].position = {sx + cx[i] + h * 0.55f, sy - h * 0.30f};
    else v[i].position = {sx + cx[i], sy - h * 0.30f * -1.f + 0.f};   // below the anchor: flattened ground contact
    v[i].color = col; v[i].tex_coord = {uu[i], vv[i]};
  }
  int idx[6] = {0, 1, 2, 0, 2, 3};
  SDL_SetTextureAlphaMod(lp.t, 255);
  SDL_RenderGeometry(G.r, lp.t, v, 4, idx, 6);
}

void drawSpriteGlow(const SpriteDef& s, float sx, float sy, float zoom, float alpha, float scaleMul) {
  LodPick lp = pick(s, zoom, scaleMul); SDL_Rect src = srcRect(s, lp.lod); float k = lp.k;
  SDL_FRect dst{sx - s.ax * k, sy - s.ay * k, s.w * k, s.h * k};
  SDL_SetTextureBlendMode(lp.t, SDL_BLENDMODE_ADD); SDL_SetTextureAlphaMod(lp.t, (Uint8)(clampf(alpha, 0, 1) * 255));
  SDL_RenderCopyF(G.r, lp.t, &src, &dst);
  SDL_SetTextureBlendMode(lp.t, SDL_BLENDMODE_BLEND); SDL_SetTextureAlphaMod(lp.t, 255);
}

void drawIcon(const SpriteDef& s, float cx, float cy, float size, float alpha) {
  Page& p = pages[s.atlas]; float k = size / (float)std::max(s.w, s.h); int lod = (k < 0.55f && p.t[1]) ? 1 : 0; SDL_Rect src = srcRect(s, lod);
  SDL_FRect dst{cx - s.w * k / 2, cy - s.h * k / 2, s.w * k, s.h * k};
  SDL_SetTextureAlphaMod(p.t[lod], (Uint8)(clampf(alpha, 0, 1) * 255));
  SDL_RenderCopyF(G.r, p.t[lod], &src, &dst);
}
