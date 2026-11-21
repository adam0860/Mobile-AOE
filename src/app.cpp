// Dawn of Kings â€” application layer: input, rendering of the isometric world, immediate-mode UI, raid flow
#include <SDL.h>
#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <unordered_map>
#include "game.h"
#include "gfx.h"
#include "sprites.h"
#include "audio.h"
#include "fx.h"

// ============================================================ app state
enum View { V_HOME, V_BATTLE };
enum Modal { M_NONE, M_INTRO, M_BUILD, M_ARMY, M_LADDER, M_RAID, M_SCOUT, M_ATK_RESULT, M_DEF_RESULT, M_WELCOME, M_SEASON, M_SETTINGS };
enum Kind { K_BARB, K_RIVAL, K_DEFENCE };

struct Scout { bool rival = false; int i = 0; Base base; std::string title; int opp = 0, tc = 1, win = 0, lose = 0; };
struct AtkResult { bool win; int stars, pct; Cost got; int dr; int surv[U_COUNT]; Kind kind; };

static View view = V_HOME;
static Modal modal = M_NONE;
static float modalScroll = 0, modalContentH = 0, panelContentH = 120;
static int selId = -1;
static bool buildMode = false; static BType bmType = B_HOUSE; static int bmx = 0, bmy = 0; static bool bmOk = false;
static bool bmPaint = false, paintOn = true;   // wall/gate/trap 'paint' mode: tap or drag to place instantly
static float speedMul = 1;
static bool incoming = false; static double incomingAt = 0;
static bool running = true;

static Scout scout;
static SeasonResult seasonRes;
void sfx(int id) { audioPlay((Sfx)id); }
static struct { Sim sim; Kind kind = K_BARB; UType sel = U_MILITIA; bool done = false; } B;
static AtkResult atk; static Report defRep; static bool hasDef = false;
static struct { double away = 0; Cost gain; Report rep; bool raided = false; } welcome;

static struct { float x = 0, y = 0, zoom = 1, vx = 0, vy = 0, zt = 1, zax = 0, zay = 0; } cam;
static float frameDt = 0.016f;                                   // seconds of the current frame (for UI easing)
static float modalAnim = 0, modalAge = 0;                        // open animation (0..1) and seconds since a modal opened
static float dispRes[NRES]; static bool dispInit = false;        // resource counters ease towards their real values
static float easeOut(float t) { t = clampf(t, 0, 1); return 1 - (1 - t) * (1 - t) * (1 - t); }
static float easeBack(float t) { t = clampf(t, 0, 1); const float c1 = 1.70158f, c3 = c1 + 1; return 1 + c3 * std::pow(t - 1, 3) + c1 * std::pow(t - 1, 2); }

static std::string toastMsg; static float toastT = 0;
static std::string bannerMsg; static float bannerT = 0;
void toast(const std::string& m) { toastMsg = m; toastT = 2.0f; }
void banner(const std::string& m) { bannerMsg = m; bannerT = 3.0f; }

static float U(float v) { return v * G.u; }
static Pt iso(float gx, float gy) { return {(gx - gy) * TW / 2 * cam.zoom + cam.x, (gx + gy) * TH / 2 * cam.zoom + cam.y}; }
static Pt toGrid(float sx, float sy) {
  float wx = (sx - cam.x) / cam.zoom, wy = (sy - cam.y) / cam.zoom;
  return {(wx / (TW / 2.f) + wy / (TH / 2.f)) / 2, (wy / (TH / 2.f) - wx / (TW / 2.f)) / 2};
}
static void centerCamera() {
  float fit = std::min(G.W / (MAP * TW * 0.8f), G.H / (MAP * TH * 1.15f));
  cam.zoom = clampf(fit * 1.7f, 0.3f, 3.f); cam.zt = cam.zoom; cam.vx = cam.vy = 0;                  // start zoomed in on the village
  Pt c = {(15.f - 15.f) * TW / 2.f, (15.f + 15.f) * TH / 2.f};    // world px of grid (15,15)
  cam.x = G.W / 2.f - c.x * cam.zoom; cam.y = G.H / 2.f - c.y * cam.zoom + U(8);
}
static void zoomAt(float x, float y, float z) {
  z = clampf(z, 0.25f, 3.5f); float k = z / cam.zoom; cam.x = x - (x - cam.x) * k; cam.y = y - (y - cam.y) * k; cam.zoom = z; cam.zt = clampf(cam.zt, 0.25f, 3.5f);
}
static void zoomSmooth(float x, float y, float z) { cam.zt = clampf(z, 0.25f, 3.5f); cam.zax = x; cam.zay = y; }
static void clampCam() {                                        // keep the map in view: its centre may not leave the screen
  Pt c = {cam.x, cam.y + (MAP * TH / 2.f) * cam.zoom};
  cam.x = clampf(cam.x, -G.W * 0.45f, G.W * 1.45f); cam.y = clampf(c.y, -G.H * 0.35f, G.H * 1.35f) - (MAP * TH / 2.f) * cam.zoom;
}

// ============================================================ input
struct Input {
  float mx = 0, my = 0; bool down = false, pressed = false, released = false, dragged = false, uiGesture = false;
  float px = 0, py = 0, wheel = 0, lastx = 0, lasty = 0; bool tap = false;
} in;
static std::vector<SDL_FRect> blocks, prevBlocks;
static bool inRect(const SDL_FRect& r, float x, float y) { return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h; }
static bool inBlocks(const std::vector<SDL_FRect>& v, float x, float y) { for (auto& r : v) if (inRect(r, x, y)) return true; return false; }
static void block(float x, float y, float w, float h) { blocks.push_back({x, y, w, h}); }

// ============================================================ UI primitives
static const uint32_t C_PANEL = 0x141c28, C_LINE = 0x3a4a60, C_GOLD = 0xffd976, C_TXT = 0xeef2f7, C_DIM = 0x9db0c6, C_BAD = 0xff7a6a, C_OK = 0x7be08a;

static bool clickGate = true; // false while a modal covers the HUD, so HUD buttons ignore clicks
static std::unordered_map<uint64_t, float> hoverT;
static const SpriteDef* spriteNamed(const char* n) { return spritesReady() ? findSprite(n) : nullptr; }
static void icon(const char* name, float cx, float cy, float size, float a = 1.f) { if (const SpriteDef* sp = spriteNamed(name)) drawIcon(*sp, cx, cy, size, a); }
static const char* RES_ICON[NRES] = {"icon_wood", "icon_food", "icon_gold", "icon_stone"};
static void resIcon(int i, float cx, float cy, float size) {
  if (const SpriteDef* sp = spriteNamed(RES_ICON[i])) drawIcon(*sp, cx, cy, size); else G.circle(cx, cy, size * 0.4f, RES_COL[i]);
}
static void panelBox(float x, float y, float w, float h, int style = 0, float a = 1.f) {
  float r = U(style == 2 ? 8.f : 13.f);
  G.shadowBox(x, y, w, h, r, 0.5f * a, U(style == 2 ? 8.f : 16.f));
  G.rrect(x - U(1.6f), y - U(1.6f), w + U(3.2f), h + U(3.2f), r + U(1.6f), 0x0b0704, a);
  G.gradRRect(x, y, w, h, r, 0x4a382a, 0x20160f, a);
  G.noiseOverlay(x, y, w, h, 0.10f * a);
  G.rrect(x + U(2.6f), y + U(2.6f), w - U(5.2f), h - U(5.2f), r - U(1.5f), 0xcfa650, 0.85f * a);                        // gold trim
  G.gradRRect(x + U(3.8f), y + U(3.8f), w - U(7.6f), h - U(7.6f), r - U(2.5f), 0x33271c, 0x1b130d, a);
  G.noiseOverlay(x + U(4), y + U(4), w - U(8), h - U(8), 0.07f * a);
  G.rect(x + r, y + U(4.6f), w - 2 * r, U(1.2f), 0xffe9b0, 0.10f * a);                                                    // top highlight
  if (style != 2) {
    float s = U(style == 1 ? 3.6f : 2.6f);
    const float cx[4] = {x + U(8), x + w - U(8), x + U(8), x + w - U(8)}, cy[4] = {y + U(8), y + U(8), y + h - U(8), y + h - U(8)};
    for (int i = 0; i < 4; i++) { Pt d[4] = {{cx[i], cy[i] - s}, {cx[i] + s, cy[i]}, {cx[i], cy[i] + s}, {cx[i] - s, cy[i]}}; G.fillPoly(d, 4, 0xe9c466, a); G.strokePoly(d, 4, 0x5a3c10, 0.8f * a); }
  }
}
static bool uiButton(float x, float y, float w, float h, const std::string& label, int style = 0, bool enabled = true, FontSize f = F_M) {
  SDL_FRect r{x, y, w, h}; block(x, y, w, h);
  bool hover = inRect(r, in.mx, in.my) && clickGate, pressed = hover && in.down && enabled;
  uint64_t key = ((uint64_t)(int)x << 40) ^ ((uint64_t)(int)y << 20) ^ (uint64_t)(int)w; if (hoverT.size() > 600) hoverT.clear();
  float& ht = hoverT[key]; ht += (((hover && enabled) ? 1.f : 0.f) - ht) * std::min(1.f, frameDt * 14.f);
  uint32_t top, bot, border, trim, txt;
  if (style == 1) { top = 0xf4c658; bot = 0xa86e18; border = 0x4a2c08; trim = 0xffe7a0; txt = 0x2b1806; }
  else if (style == 2) { top = 0xd2523e; bot = 0x7e241a; border = 0x2a0c08; trim = 0xf0a08a; txt = 0xfff0e8; }
  else { top = 0x735a3e; bot = 0x3f2f20; border = 0x120b06; trim = 0xb89258; txt = C_TXT; }
  if (!enabled) { top = mixc(top, 0x2a2622, 0.65f); bot = mixc(bot, 0x1a1714, 0.65f); trim = mixc(trim, 0x3a342c, 0.7f); txt = 0x80786c; }
  top = shadec(top, 0.16f * ht); bot = shadec(bot, 0.10f * ht);
  float oy = pressed ? U(1.6f) : -ht * U(1.2f), rad = U(8);
  if (!pressed) G.rrect(x, y + U(2.4f), w, h, rad, 0x000000, 0.4f);
  G.rrect(x - U(1.4f), y + oy - U(1.4f), w + U(2.8f), h + U(2.8f), rad + U(1.4f), border, 1.f);
  G.gradRRect(x, y + oy, w, h, rad, top, bot, 1.f);
  G.gradRRect(x + U(2), y + oy + U(2), w - U(4), h * 0.46f, rad - U(1.5f), 0xffffff, 0xffffff, 0.0f + 0.16f);
  G.gradRRect(x + U(2), y + oy + U(2), w - U(4), h * 0.46f, rad - U(1.5f), 0xffffff, 0xffffff, 0.0f);
  G.rrect(x + U(1.2f), y + oy + h - U(2.4f), w - U(2.4f), U(1.4f), U(1), 0x000000, 0.28f);
  if (ht > 0.02f) G.ring(x + w / 2, y + oy + h / 2, 0, 0, trim, 0);   // keep ring() linked for hover glow variants
  if (ht > 0.02f) G.rrect(x - U(2.4f), y + oy - U(2.4f), w + U(4.8f), U(1.2f), U(1), trim, 0.65f * ht);
  Pt ts = G.textSize(label, f);
  if (style == 1 && enabled) G.text(label, x + w / 2, y + oy + (h - ts.y) / 2 + U(1), f, 0xffe9a8, A_CENTER, 0.5f);
  else if (enabled) G.text(label, x + w / 2 + U(1), y + oy + (h - ts.y) / 2 + U(1.4f), f, 0x000000, A_CENTER, 0.55f);
  G.text(label, x + w / 2, y + oy + (h - ts.y) / 2, f, txt, A_CENTER);
  bool clicked = clickGate && in.released && !in.dragged && enabled && inRect(r, in.mx, in.my) && inRect(r, in.px, in.py);
  if (clicked) audioPlay(SFX_CLICK);
  return clicked;
}
// square icon button with a caption underneath (bottom command bar)
static bool iconButton(float x, float y, float w, float h, const char* ic, const char* label, int style, bool enabled = true) {
  bool c = uiButton(x, y, w, h, "", style, enabled);
  float oy = 0; uint64_t key = ((uint64_t)(int)x << 40) ^ ((uint64_t)(int)y << 20) ^ (uint64_t)(int)w; auto it = hoverT.find(key); if (it != hoverT.end()) oy = -it->second * U(1.2f);
  icon(ic, x + w / 2, y + oy + h * 0.38f, h * 0.52f, enabled ? 1.f : 0.45f);
  Pt ts = G.textSize(label, F_S); uint32_t tc = style == 1 ? 0x2b1806 : C_TXT;
  G.text(label, x + w / 2, y + oy + h - ts.y - U(4), F_S, tc, A_CENTER, enabled ? 1.f : 0.55f);
  return c;
}

static std::string costStr(const Cost& c) {
  static const char* L = "WFGS"; std::string s;
  for (int i = 0; i < NRES; i++) if (c.v[i]) { if (!s.empty()) s += " "; s += std::to_string(c.v[i]) + L[i]; }
  return s.empty() ? "free" : s;
}
// resource cost display with icons; returns width
static float drawCost(float x, float y, const Cost& c, Align al, bool checkAfford = true) {
  float total = 0;
  for (int i = 0; i < NRES; i++) if (c.v[i]) total += U(17) + G.textSize(std::to_string(c.v[i]), F_S).x + U(8);
  float cx = al == A_RIGHT ? x - total : x;
  for (int i = 0; i < NRES; i++) if (c.v[i]) {
    resIcon(i, cx + U(8), y + U(8), U(17));
    std::string s = std::to_string(c.v[i]); bool ok = !checkAfford || S.res[i] >= c.v[i];
    G.text(s, cx + U(18), y, F_S, ok ? C_TXT : C_BAD); cx += U(17) + G.textSize(s, F_S).x + U(8);
  }
  return total;
}

struct Flow { float x0, x, y, maxW, rowH, gap; };
static Flow flowAt(float x, float y, float w) { return {x, x, y, w, 0, U(6)}; }
static void flowNl(Flow& f, float extra = 0) { f.y += f.rowH + f.gap + extra; f.x = f.x0; f.rowH = 0; }
static bool flowBtn(Flow& f, const std::string& label, int style = 0, bool enabled = true, float minW = 0) {
  float w = std::max(minW, G.textSize(label, F_M).x + U(24)), h = U(36);
  if (f.x + w > f.x0 + f.maxW && f.x > f.x0) flowNl(f);
  bool c = uiButton(f.x, f.y, w, h, label, style, enabled); f.x += w + f.gap; f.rowH = std::max(f.rowH, h); return c;
}
static void flowText(Flow& f, const std::string& s, uint32_t col = C_TXT, FontSize fs = F_M) {
  Pt ts = G.textSize(s, fs); if (f.x + ts.x > f.x0 + f.maxW && f.x > f.x0) flowNl(f);
  G.text(s, f.x, f.y + U(7), fs, col); f.x += ts.x + f.gap; f.rowH = std::max(f.rowH, U(36));
}
static std::vector<std::string> wrap(const std::string& s, float maxW, FontSize fs) {
  std::vector<std::string> out; std::string line, word; size_t i = 0;
  auto flushWord = [&]() { if (word.empty()) return; std::string t = line.empty() ? word : line + " " + word; if (G.textSize(t, fs).x > maxW && !line.empty()) { out.push_back(line); line = word; } else line = t; word.clear(); };
  for (; i < s.size(); i++) { if (s[i] == ' ') flushWord(); else if (s[i] == '\n') { flushWord(); out.push_back(line); line.clear(); } else word += s[i]; }
  flushWord(); if (!line.empty()) out.push_back(line); return out;
}
static void flowPara(Flow& f, const std::string& s, uint32_t col = C_DIM, FontSize fs = F_M) {
  if (f.x > f.x0) flowNl(f);
  for (auto& ln : wrap(s, f.maxW, fs)) { G.text(ln, f.x0, f.y, fs, col); f.y += G.textSize("Ag", fs).y + U(2); }
  f.y += U(4);
}
static void flowTitle(Flow& f, const std::string& s, uint32_t col = C_TXT, FontSize fs = F_L) {
  if (f.x > f.x0) flowNl(f);
  if (fs == F_L) {                                              // ornate heading: serif face, shadow, gold rule with a diamond
    FontSize tf = F_T; G.textShadow(s, f.x0, f.y, tf, col == C_TXT ? 0xf7e6bd : col);
    float h = G.textSize("Ag", tf).y; float ry = f.y + h + U(1);
    G.gradRect(f.x0, ry, f.maxW * 0.62f, U(1.6f), 0xe0b860, 0x6a4a18, 0.9f); Pt d[4] = {{f.x0 - U(1), ry + U(0.8f) - U(3)}, {f.x0 + U(3), ry + U(0.8f)}, {f.x0 - U(1), ry + U(0.8f) + U(3)}, {f.x0 - U(5), ry + U(0.8f)}}; G.fillPoly(d, 4, 0xf0c866, 1);
    f.y += h + U(10); return;
  }
  G.text(s, f.x0, f.y, fs, col); f.y += G.textSize("Ag", fs).y + U(6);
}
static void flowCost(Flow& f, const std::string& label, const Cost& c, bool check = true) {
  if (f.x > f.x0) flowNl(f);
  float lw = G.textSize(label, F_M).x; G.text(label, f.x0, f.y, F_M, C_TXT); drawCost(f.x0 + lw + U(8), f.y + U(2), c, A_LEFT, check); f.y += U(26);
}

// ============================================================ world rendering
static void polyFill(std::vector<Pt> p, uint32_t c, float a = 1) { G.fillPoly(p.data(), (int)p.size(), c, a); }
static void polyLine(std::vector<Pt> p, uint32_t c, float a = 1) { G.strokePoly(p.data(), (int)p.size(), c, a); }
static float nowSec() { return SDL_GetTicks() / 1000.f; }
static void bar(float px, float py, float w, float f, uint32_t col) {
  float h = U(5.5f); G.rrect(px - w / 2 - 1, py - 1, w + 2, h + 2, U(3), 0x000000, 0.75f);
  G.gradRRect(px - w / 2, py, w, h, U(2.5f), 0x2a2420, 0x15110e, 1.f);
  if (f > 0.01f) { G.gradRRect(px - w / 2, py, std::max(U(3), w * f), h, U(2.5f), shadec(col, 0.3f), shadec(col, -0.15f), 1.f); }
}
static Pt boxDraw(float x, float y, int w, int h, float hgt, uint32_t color, float alpha) {
  Pt n = iso(x, y), e = iso(x + w, y), s = iso(x + w, y + h), wv = iso(x, y + h); float H = hgt * cam.zoom;
  auto up = [&](Pt p) { return Pt{p.x, p.y - H}; };
  polyFill({wv, s, up(s), up(wv)}, shadec(color, -0.25f), alpha); polyLine({wv, s, up(s), up(wv)}, 0x000000, 0.25f * alpha);
  polyFill({s, e, up(e), up(s)}, shadec(color, -0.42f), alpha); polyLine({s, e, up(e), up(s)}, 0x000000, 0.25f * alpha);
  polyFill({up(n), up(e), up(s), up(wv)}, shadec(color, 0.12f), alpha); polyLine({up(n), up(e), up(s), up(wv)}, 0x000000, 0.25f * alpha);
  Pt c = iso(x + w / 2.f, y + h / 2.f); return {c.x, c.y - H};
}
static void drawUnitFig(float x, float y, uint32_t color, float size, bool swing, bool enemy) {
  Pt p = iso(x, y); float z = cam.zoom, s = size * z, bob = std::sin(SDL_GetTicks() / 120.f + x * 7) * 1.2f * z;
  G.ellipse(p.x, p.y, s * .9f, s * .4f, 0x000000, 0.28f);
  G.circle(p.x, p.y - s * 1.1f + bob - (swing ? 3 * z : 0), s, color);
  G.circle(p.x, p.y - s * 2.3f + bob, s * .6f, 0xf1d3b0);
  if (enemy) G.circle(p.x, p.y - s * 1.1f + bob, s * .35f, 0xff5a4a);
}

// ---- unit sprites: 8 facings, frames 0 idle / 1-6 move / 7-9 attack (+ villager tool work frames)
static const char* KIND_NAME[16] = {"villager", "militia", "manatarms", "archer", "longsword", "crossbow", "knight", "monk", "ram", "mangonel", "champion", "arbalester", "paladin", "handcannon", "trebuchet", "bombard"};
static const char* TOOL_NAME[4] = {"hammer", "axe", "pick", "hoe"};
static const SpriteDef* unitSpriteFor(int kind, bool red, int dir, int frame, int tool = -1) {
  char nm[72]; int ki = kind < 0 ? 0 : kind + 1; const char* team = (red && kind >= 0) ? "red" : "blue";
  if (tool >= 0) { std::snprintf(nm, sizeof nm, "uw_%s_blue_%d_%d", TOOL_NAME[tool], dir, frame); return findSprite(nm); }
  std::snprintf(nm, sizeof nm, "u_%s_%s_%d_%d", KIND_NAME[ki], team, dir, frame);
  const SpriteDef* sp = findSprite(nm);
  if (!sp && frame) { std::snprintf(nm, sizeof nm, "u_%s_%s_%d_0", KIND_NAME[ki], team, dir); sp = findSprite(nm); }
  return sp;
}
static int dirIndex(float dx, float dy) {
  if (std::fabs(dx) < 1e-4f && std::fabs(dy) < 1e-4f) return 3;
  float a = std::atan2(-dy, dx); int k = (int)std::lround(a / (3.14159265f / 4)); return ((k % 8) + 8) % 8;
}
static int simFace(const SimUnit& u) {                       // facing with hysteresis so units don't flicker at bin edges
  float a = std::atan2(-u.dy, u.dx), d = a - u.face * 0.785398f; while (d > 3.14159f) d -= 6.28318f; while (d < -3.14159f) d += 6.28318f;
  if (std::fabs(d) > 0.5f) { int k = (int)std::lround(a / 0.785398f); u.face = ((k % 8) + 8) % 8; }
  return u.face;
}
static int simFrame(const SimUnit& u) {
  if (u.swing > 0) { float p = 1.f - u.swing / std::max(0.05f, u.swingMax); return 7 + clampi((int)(p * 3), 0, 2); }
  if (u.moving) { float ph = u.aph - std::floor(u.aph); return 1 + (int)(ph * 6) % 6; }
  return 0;
}
static void drawUnitSpr(float gx, float gy, int kind, bool red, int dir, int frame, float alpha = 1.f, float hit = 0.f, int tool = -1, float yoff = 0.f) {
  Pt p = iso(gx, gy); p.y += yoff * cam.zoom;
  if (const SpriteDef* sp = unitSpriteFor(kind, red, dir, frame, tool)) {
    drawSpriteShadow(*sp, p.x, p.y, cam.zoom, 0.30f * alpha); drawSprite(*sp, p.x, p.y, cam.zoom, alpha);
    if (hit > 0) drawSpriteGlow(*sp, p.x, p.y, cam.zoom, std::min(1.f, hit / 0.16f) * 0.75f * alpha);
  } else drawUnitFig(gx, gy, kind < 0 ? 0xf0e4c8 : udef(kind).color, 3.4f, frame >= 7, red);
}
static void drawNode(const Node& n) {
  Pt p = iso(n.x + .5f, n.y + .5f); float z = cam.zoom, f = n.amt / n.max;
  if (spritesReady()) {
    char nm[32]; bool tree = false;
    if (n.type == N_TREE) { int k = (n.x * 7 + n.y * 13) % 12; tree = true; if (k < 5) std::snprintf(nm, sizeof nm, "oak_%d", k % 4); else if (k < 10) std::snprintf(nm, sizeof nm, "pine_%d", k % 3); else std::snprintf(nm, sizeof nm, "palm_%d", k % 2); }
    else std::snprintf(nm, sizeof nm, n.type == N_GOLD ? "node_gold" : "node_stone");
    float sc = 0.6f + 0.4f * f;
    if (const SpriteDef* sp = findSprite(nm)) {
      drawSpriteShadow(*sp, p.x, p.y, z, 0.28f, sc);
      if (tree) drawSpriteSway(*sp, p.x, p.y, z, 0.02f, nowSec() * 1.6f + n.x * 0.7f + n.y * 0.45f, sc); else drawSprite(*sp, p.x, p.y, z, 1.f, sc);
      return;
    }
  }
  if (n.type == N_TREE) {
    float s = (7 + 6 * f) * z;
    G.rect(p.x - 2 * z, p.y - 8 * z, 4 * z, 8 * z, 0x4a2f18);
    polyFill({{p.x, p.y - 8 * z - s * 2}, {p.x - s, p.y - 6 * z}, {p.x + s, p.y - 6 * z}}, 0x2f6a2f);
    polyFill({{p.x, p.y - 8 * z - s * 2}, {p.x + s, p.y - 6 * z}, {p.x, p.y - 6 * z}}, 0x3b8a3b);
  } else {
    uint32_t col = n.type == N_GOLD ? 0xf2c230 : 0x9aa0a8; float s = (9 + 6 * f) * z;
    polyFill({{p.x, p.y - s * 1.3f}, {p.x + s, p.y - s * .3f}, {p.x, p.y + 2 * z}, {p.x - s, p.y - s * .3f}}, col);
    polyFill({{p.x, p.y - s * 1.3f}, {p.x + s, p.y - s * .3f}, {p.x, p.y - s * .5f}}, shadec(col, .3f));
  }
}
static void drawRubble(const SimB& b, float alpha = 1.f) {
  Pt c = iso(b.x + b.w / 2, b.y + b.h / 2);
  if (spritesReady()) { if (const SpriteDef* sp = findSprite(b.w >= 4 ? "rubble_4" : b.w >= 3 ? "rubble_3" : "rubble_2")) drawSprite(*sp, c.x, c.y, cam.zoom, alpha); }
  else polyFill({iso(b.x, b.y), iso(b.x + b.w, b.y), iso(b.x + b.w, b.y + b.h), iso(b.x, b.y + b.h)}, 0x1e140a, 0.55f * alpha);
}
struct BView { BType type; int lvl; float x, y; int w, h; float hpf; bool simMode; bool building; float prog; bool sel; int status; };
static const SpriteDef* spriteByAge(const char* base, int age) {
  char nm[64];
  for (int d = 0; d < 4; d++) for (int sgn = -1; sgn <= 1; sgn += 2) {
    int a = age + sgn * d; if (a < 0 || a > 3) continue;
    std::snprintf(nm, sizeof nm, "%s_%d", base, a); if (const SpriteDef* sp = findSprite(nm)) return sp;
  }
  return nullptr;
}
static const SpriteDef* buildingSprite(BType t, int lvl, int age) {
  age = clampi(age, 0, 3); lvl = clampi(lvl, 1, MAXLVL); char nm[48];
  switch (t) {
    case B_TOWNHALL: std::snprintf(nm, sizeof nm, "townhall_%d", lvl); return findSprite(nm);
    case B_TOWER: std::snprintf(nm, sizeof nm, "tower_%d", lvl <= 1 ? 1 : lvl <= 3 ? 2 : lvl <= 5 ? 3 : 4); return findSprite(nm);
    case B_HOUSE: return spriteByAge("house", age);
    case B_LUMBER: return spriteByAge("lumber", age);
    case B_GOLDCAMP: return spriteByAge("goldcamp", age);
    case B_STONECAMP: return spriteByAge("stonecamp", age);
    case B_FARM: return spriteByAge("farm", age);
    case B_BARRACKS: return spriteByAge("barracks", age);
    case B_ARMYCAMP: return spriteByAge("armycamp", age);
    case B_ARCHERY: return spriteByAge("archery", age);
    case B_STABLE: return spriteByAge("stable", age);
    case B_SIEGE: return spriteByAge("siege", age);
    case B_MONASTERY: return spriteByAge("monastery", age);
    case B_KEEP: return spriteByAge("keep", age);
    case B_BLACKSMITH: return spriteByAge("blacksmith", age);
    case B_MARKET: return spriteByAge("market", age);
    case B_CASTLE: return spriteByAge("castle", age);
    case B_UNIVERSITY: return spriteByAge("university", age);
    case B_BOMBARD: return spriteByAge("bombard", age);
    case B_SCORPION: return spriteByAge("scorpion", age);
    case B_STOREHOUSE: return findSprite("storehouse_0");
    case B_TENT: return findSprite("tent_0");
    default: return findSprite("chief_0");
  }
}
static bool wallGrid[MAP * MAP];
static bool wallAt(int x, int y) { return x >= 0 && y >= 0 && x < MAP && y < MAP && wallGrid[y * MAP + x]; }
static const SpriteDef* pieceSprite(BType t, int lvl, int x, int y) {
  int mask = (wallAt(x + 1, y) ? 1 : 0) | (wallAt(x - 1, y) ? 2 : 0) | (wallAt(x, y + 1) ? 4 : 0) | (wallAt(x, y - 1) ? 8 : 0);
  int st = lvl <= 2 ? 0 : lvl <= 5 ? 1 : lvl <= 7 ? 2 : 3; char nm[40];
  if (t == B_WALL) { std::snprintf(nm, sizeof nm, "wall_%d_%d", st, mask); return findSprite(nm); }
  if (t == B_GATE) { int axis = (mask & 3) ? 0 : ((mask & 12) ? 1 : 0); std::snprintf(nm, sizeof nm, "gate_%d_%c", st, axis ? 'y' : 'x'); return findSprite(nm); }
  return findSprite(t == B_SPIKE ? "spike_0" : "oil_0");
}
static const SpriteDef* scaffoldFor(int w) { return findSprite(w >= 4 ? "scaffold_4" : w >= 3 ? "scaffold_3" : "scaffold_2"); }
static void drawBuilding(const BView& b, int age) {
  const BDef& d = bdef(b.type);
  const SpriteDef* sp = !spritesReady() ? nullptr : isPiece(b.type) ? pieceSprite(b.type, b.lvl, (int)b.x, (int)b.y) : buildingSprite(b.type, b.lvl, age);
  if (sp) {
    Pt c = iso(b.x + b.w / 2.f, b.y + b.h / 2.f); float z = cam.zoom;
    if (b.lvl == 0) {
      float pop = b.prog < 0.02f ? easeBack(b.prog / 0.02f) : 1.f;                         // pop-in when construction starts
      if (const SpriteDef* sc = scaffoldFor(b.w)) { drawSpriteShadow(*sc, c.x, c.y, z, 0.25f, pop); drawSprite(*sc, c.x, c.y, z, 1.f, pop); }
      drawSprite(*sp, c.x, c.y, z, 0.1f + 0.8f * b.prog);
    } else {
      drawSpriteShadow(*sp, c.x, c.y, z, 0.30f); drawSprite(*sp, c.x, c.y, z);
      if (b.building) if (const SpriteDef* sc = scaffoldFor(b.w)) drawSprite(*sc, c.x, c.y, z, 0.65f);
    }
    if (b.sel) {
      float pulse = 0.65f + 0.35f * std::sin(nowSec() * 5.f);
      polyLine({iso(b.x - .12f, b.y - .12f), iso(b.x + b.w + .12f, b.y - .12f), iso(b.x + b.w + .12f, b.y + b.h + .12f), iso(b.x - .12f, b.y + b.h + .12f)}, 0xffe27a, pulse);
      polyLine({iso(b.x - .2f, b.y - .2f), iso(b.x + b.w + .2f, b.y - .2f), iso(b.x + b.w + .2f, b.y + b.h + .2f), iso(b.x - .2f, b.y + b.h + .2f)}, 0xffe27a, 0.25f * pulse);
    }
    Pt top{c.x, c.y - sp->ay / sp->scale * z};
    if (!b.simMode && b.lvl >= 1 && !isPiece(b.type) && z > 0.55f) {
      std::string lv = std::to_string(b.lvl); float w = G.textSize(lv, F_S).x + U(14);
      G.rrect(top.x - w / 2, top.y - U(8), w, U(16), U(8), 0x0b0704, 0.7f); G.rrect(top.x - w / 2 + U(1), top.y - U(7), w - U(2), U(14), U(7), 0x7a5a24, 0.9f);
      G.text(lv, top.x, top.y - U(6.5f), F_S, 0xfff0c0, A_CENTER);
    }
    if (b.building) { bar(top.x, top.y - U(22), U(46) * std::max(0.6f, z), b.prog, 0xffcf4a); }
    if (b.simMode && b.hpf < 1) bar(top.x, top.y - U(14), U(38) * std::max(0.6f, z), b.hpf, b.hpf > 0.5f ? 0x6fe06f : b.hpf > 0.25f ? 0xf0c040 : 0xe25555);
    if (!b.simMode && b.status == 3) G.textShadow("FULL", top.x, top.y - U(26), F_S, C_BAD, A_CENTER);
    if (!b.simMode && b.status == 4) G.textShadow("no nodes", top.x, top.y - U(26), F_S, C_BAD, A_CENTER);
    return;
  }
  uint32_t col = mixc(d.color, 0xa8a8a8, age * 0.18f);
  float hgt = d.hgt * (b.lvl >= 2 ? 1.12f : 1.f) * (b.lvl >= 3 ? 1.1f : 1.f); if (b.lvl == 0) hgt *= 0.4f;
  Pt top = boxDraw(b.x, b.y, b.w, b.h, hgt, col, b.lvl == 0 ? 0.6f : 1.f);
  if (b.sel) polyLine({iso(b.x - .1f, b.y - .1f), iso(b.x + b.w + .1f, b.y - .1f), iso(b.x + b.w + .1f, b.y + b.h + .1f), iso(b.x - .1f, b.y + b.h + .1f)}, 0xffe27a);
  float z = cam.zoom;
  if (b.lvl >= 1) {
    G.text(d.tag, top.x, top.y + U(1), F_S, 0xffffff, A_CENTER);
    if (!b.simMode) G.text("Lv" + std::to_string(b.lvl), top.x, top.y - U(15), F_S, 0xffe9a8, A_CENTER);
  }
  if (b.building) { bar(top.x, top.y - U(28), U(44) * std::max(0.6f, z), b.prog, 0xffcf4a); G.text("building", top.x, top.y - U(16), F_S, 0xffe9a8, A_CENTER); }
  if (b.simMode && b.hpf < 1) bar(top.x, top.y - U(20), U(36) * std::max(0.6f, z), b.hpf, 0xe25555);
  if (!b.simMode && b.status == 3) G.text("FULL", top.x, top.y - U(28), F_S, C_BAD, A_CENTER);
  if (!b.simMode && b.status == 4) G.text("no nodes", top.x, top.y - U(28), F_S, C_BAD, A_CENTER);
}

struct Item { float d; std::function<void()> f; };
static void drawGround(int tc, bool grid) {
  Pt A = iso(0, 0), Bp = iso(MAP, 0), C = iso(MAP, MAP), D = iso(0, MAP);
  bool any = false;
  if (spritesReady()) { Pt c = iso(MAP / 2.f, MAP / 2.f); for (int i = 0; i < 2; i++) for (int j = 0; j < 2; j++) { char nm[24]; std::snprintf(nm, sizeof nm, "ground_%d_%d", i, j); if (const SpriteDef* gs = findSprite(nm)) { drawSprite(*gs, c.x, c.y, cam.zoom); any = true; } } }
  if (!any) {
    polyFill({D, C, {C.x, C.y + 18 * cam.zoom}, {D.x, D.y + 18 * cam.zoom}}, 0x5a3f26);
    polyFill({A, Bp, C, D}, 0x4f7a3a);
  }
  if (grid) {
    G.setColor(0xffffff, 0.12f);
    for (int i = 0; i <= MAP; i++) { Pt a = iso(i, 0), b = iso(i, MAP), c = iso(0, i), d = iso(MAP, i); SDL_RenderDrawLineF(G.r, a.x, a.y, b.x, b.y); SDL_RenderDrawLineF(G.r, c.x, c.y, d.x, d.y); }
  }
  (void)tc;
}
static void drawProj(const Proj& p) {
  float k = 1 - p.life / p.max; Pt a = iso(p.x0, p.y0), b = iso(p.x1, p.y1); float z = cam.zoom; float dist = std::hypot(b.x - a.x, b.y - a.y);
  float x = a.x + (b.x - a.x) * k, y = a.y + (b.y - a.y) * k;
  if (p.col == 0xffa040) {                                  // siege stone: high arc with a ground shadow
    float arc = std::sin(k * 3.14159f) * (dist * 0.35f + 30 * z) , gy = y; y -= 14 * z + arc;
    G.ellipse(x, gy, 5 * z, 2.5f * z, 0x000000, 0.3f); G.circle(x, y, 4.2f * z, 0x6b6a64); G.circle(x - z, y - z, 2.2f * z, 0xa6a49a); return;
  }
  if (p.col == 0xff7a5a) { G.ring(a.x, a.y - 3 * z, (6 + k * 26) * z, 3 * z, 0xff9a60, 1 - k); return; }
  if (p.col == 0x7bffa0) { G.line(a.x, a.y - 22 * z, b.x, b.y - 22 * z, 0x7bffa0, 0.35f * (1 - k)); return; }
  float arc = std::sin(k * 3.14159f) * dist * 0.12f; y -= 16 * z + arc;
  float dx = b.x - a.x, dy = b.y - a.y, l = std::max(1.f, std::hypot(dx, dy)); dx /= l; dy /= l;
  if (p.col == 0xffe9b0) {                                  // scorpion bolt: long, bright, fading trail
    float tl = 36 * z; G.line(x - dx * tl, y - dy * tl, x, y, 0xffe9b0, 0.8f); G.line(x - dx * tl * 0.6f, y - dy * tl * 0.6f, x, y, 0xffffff, 1.f); G.circle(x, y, 2.2f * z, 0xffffff); return;
  }
  float tl = 11 * z; uint32_t c = p.col == 0xffd9a0 ? 0xf0d8a0 : 0xe8e4d8;
  G.line(x - dx * tl, y - dy * tl, x, y, 0x4a3a28, 1.f); G.line(x - dx * tl * 0.2f, y - dy * tl * 0.2f, x + dx * 2 * z, y + dy * 2 * z, c, 1.f);
  G.circle(x + dx * 2 * z, y + dy * 2 * z, 1.5f * z, 0xffffff);
}
static void drawParticles() {
  float z = cam.zoom;
  for (auto& p : FX_P) {
    Pt s = iso(p.x, p.y); float life = p.life / p.max, age = 1 - life, r = std::max(0.5f, p.size * (1 + p.grow * age)) * z * G.u * 0.75f, y = s.y - p.z * z;
    float a;
    switch (p.kind) {
      case FX_SMOKE: a = 0.34f * std::pow(life, 0.8f) * std::min(1.f, age * 6); G.circle(s.x, y, r, shadec(p.col, -0.1f * age), a); break;
      case FX_DUST: a = 0.2f * life; G.circle(s.x, y, r, p.col, a); break;
      case FX_SPARK: a = std::min(1.f, life * 2.5f); G.circle(s.x, y, r, p.col, a); G.circle(s.x, y, r * 2.2f, p.col, a * 0.18f); break;
      case FX_FLAME: a = 0.75f * life; G.circle(s.x, y, r * 1.5f, 0xff5a10, a * 0.35f); G.circle(s.x, y, r, p.col, a); break;
      case FX_HEAL: a = std::sin(life * 3.14159f) * 0.9f; G.circle(s.x, y, r * 2.4f, p.col, a * 0.2f); G.circle(s.x, y, r, 0xe8ffe8, a); break;
      case FX_DEBRIS: a = std::min(1.f, life * 2.f); G.rect(s.x - r / 2, y - r / 2, r, r, p.col, a); break;
      case FX_RING: G.ring(s.x, s.y - 4 * z, r * 2.5f, std::max(1.f, 3 * z * life), p.col, life * 0.8f); break;
    }
  }
}
static std::vector<Item> items;
static std::unordered_map<int, int> workFrameMem;
// small helper: a worker's 8-way facing from a direction vector
static void pushVillagerItem(float depth, std::function<void()> f) { items.push_back({depth, std::move(f)}); }
static void drawHome() {
  int tc = tcLvl(), age = clampi(ageOf(tc), 0, 3);
  drawGround(tc, buildMode);
  items.clear(); float t = nowSec();
  for (auto& n : S.nodes) if (n.amt > 0) { Node* np = &n; items.push_back({(float)(n.x + n.y + 1), [np] { drawNode(*np); }}); }
  std::fill(wallGrid, wallGrid + MAP * MAP, false);
  for (auto& b : S.b) if (b.type == B_WALL || b.type == B_GATE) wallGrid[b.y * MAP + b.x] = true;
  for (auto& b : S.b) {
    int vl = (b.type == B_WALL || b.type == B_GATE) ? S.wallLvl : (b.type == B_SPIKE || b.type == B_OIL) ? S.trapLvl : b.lvl;
    BView v{b.type, vl, (float)b.x, (float)b.y, b.w, b.h, 1, false, b.building, b.building ? clampf((float)((S.t - b.bStart) / (b.bEnd - b.bStart)), 0, 1) : 0, b.id == selId, b.status};
    items.push_back({(float)(b.x + b.y) + (b.w + b.h) / 2.f, [v, age] { drawBuilding(v, age); }});
  }
  // villagers: walk to the work site, swing their tool for a while, walk back (all analytic -> deterministic and cheap)
  int nIdle = std::max(0, idleV()); Bld& tcb = tcB();
  auto walkFrame = [](float cycles) { float f = cycles - std::floor(cycles); return 1 + (int)(f * 6) % 6; };
  auto addWalker = [&](Pt p0, Pt p1, float cycles) {
    int dir = dirIndex(p1.x - p0.x, p1.y - p0.y), fr = walkFrame(cycles);
    items.push_back({p0.x + p0.y, [=] { drawUnitSpr(p0.x, p0.y, -1, false, dir, fr); }});
  };
  auto addWorker = [&](Pt p, float fx, float fy, int tool, int fr, int key, float chipZ, int chip) {
    int dir = dirIndex(fx, fy);
    int& last = workFrameMem[key];
    if (fr == 2 && last != 2) { Pt q = {p.x + fx * 0.5f, p.y + fy * 0.5f}; fxSpawn(chip, q.x, q.y, chipZ, chip == FX_SPARK ? 3 : 4); }
    last = fr;
    items.push_back({p.x + p.y, [=] { drawUnitSpr(p.x, p.y, -1, false, dir, fr, 1.f, 0.f, tool); }});
  };
  for (auto& b : S.b) {
    const BDef& d = bdef(b.type); float cx = b.x + b.w / 2.f, cy = b.y + b.h / 2.f;
    if (b.building) { float bx = b.x + b.w + .35f, by = cy + std::sin(t * 1.7f + b.id) * 0.25f; addWorker({bx, by}, -1.f, 0.f, 0, (int)(t * 5 + b.id) % 4, 5000 + b.id, 18, FX_DUST); }
    if (d.res >= 0 && isDone(b) && b.workers && b.status == 2) {
      const Node* tgt = nullptr; float bd = 99;
      if (d.node >= 0) for (auto& n : S.nodes) if (n.type == d.node && n.amt > 0) { float dd = std::hypot(n.x + .5f - cx, n.y + .5f - cy); if (dd < bd && dd <= 7) { bd = dd; tgt = &n; } }
      int tool = d.res == WOOD ? 1 : d.res == FOOD ? 3 : 2; int chip = d.res == WOOD ? FX_DEBRIS : d.res == FOOD ? FX_DUST : FX_SPARK;
      for (int k = 0; k < b.workers; k++) {
        if (d.res == FOOD) {                                            // farmers hoe their own field
          Pt p{cx + (k ? 0.5f : 0.f) - 0.3f, cy + 0.2f}; addWorker(p, 0.f, -1.f, 3, (int)(t * 3.5f + b.id + k) % 4, 6000 + b.id * 8 + k, 10, FX_DUST); continue;
        }
        if (!tgt) { Pt p{cx + 1.4f + std::sin(t + k) * .3f, cy + std::cos(t) * .3f}; addWalker(p, {p.x + std::cos(t), p.y - std::sin(t)}, t * 1.2f); continue; }
        float tx = tgt->x + .5f, ty = tgt->y + .5f; float L = std::max(0.5f, std::hypot(tx - cx, ty - cy)); float ux = (tx - cx) / L, uy = (ty - cy) / L;
        Pt st{cx + ux * 1.1f + k * 0.12f, cy + uy * 1.1f}, en{tx - ux * 0.55f + k * 0.1f, ty - uy * 0.55f};
        float T = 8.f, p = std::fmod(t / T + k * 0.27f + b.id * 0.13f, 1.f), dist = std::hypot(en.x - st.x, en.y - st.y);
        if (p < 0.28f) { float f = p / 0.28f; Pt q{st.x + (en.x - st.x) * f, st.y + (en.y - st.y) * f}; addWalker(q, {en.x, en.y}, f * dist / 0.95f); }
        else if (p < 0.72f) addWorker(en, ux, uy, tool, (int)(t * 4.5f + k) % 4, 7000 + b.id * 8 + k, 22, chip);
        else { float f = (p - 0.72f) / 0.28f; Pt q{en.x + (st.x - en.x) * f, en.y + (st.y - en.y) * f}; addWalker(q, {st.x, st.y}, f * dist / 0.95f); }
      }
    }
  }
  for (int k = 0; k < std::min(nIdle, 8); k++) {
    float bx = (float)tcb.x, by = (float)tcb.y;
    auto pos = [&](float tt) { return Pt{bx + 1.5f + std::cos(tt * 0.5f + k * 1.7f) * 2.8f, by + 3.6f + std::sin(tt * 0.4f + k * 2.3f) * 0.8f}; };
    Pt p0 = pos(t), p1 = pos(t + 0.12f); addWalker(p0, p1, t * 0.8f + k * 0.37f);
  }
  if (buildMode) {
    BType ty = bmType; int x = bmx, y = bmy; bool ok = bmPaint ? (placePieceBlock(ty, x, y) == nullptr && canAfford(bdef(ty).cost)) : bmOk;
    items.push_back({999.f, [=] {
      const BDef& d = bdef(ty);
      if (bmPaint) { polyFill({iso(x, y), iso(x + 1, y), iso(x + 1, y + 1), iso(x, y + 1)}, ok ? 0x50ff78 : 0xff4646, 0.5f); return; }
      polyFill({iso(x, y), iso(x + d.w, y), iso(x + d.w, y + d.h), iso(x, y + d.h)}, ok ? 0x50ff78 : 0xff4646, 0.45f);
      if (const SpriteDef* sp = spriteNamed("") ; sp) {}
      if (const SpriteDef* sp = spritesReady() ? buildingSprite(ty, 1, std::min(ageOf(tcLvl()), 3)) : nullptr) { Pt c = iso(x + d.w / 2.f, y + d.h / 2.f); drawSprite(*sp, c.x, c.y, cam.zoom, ok ? 0.6f : 0.35f); }
      else boxDraw((float)x, (float)y, d.w, d.h, d.hgt, ok ? 0x6fd88a : 0xd86f6f, 0.55f);
    }});
  }
  std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.d < b.d; });
  for (auto& it : items) it.f();
  drawParticles();
}
static void drawBattle() {
  Sim& sim = B.sim; int tc = B.kind == K_DEFENCE ? tcLvl() : (B.kind == K_RIVAL ? scout.tc : 1), age = clampi(ageOf(tc), 0, 3);
  drawGround(tc, false); items.clear();
  std::fill(wallGrid, wallGrid + MAP * MAP, false);
  for (auto& b : sim.b) if (b.wall && !b.dead) wallGrid[(int)b.y * MAP + (int)b.x] = true;
  for (auto& b : sim.b) {
    if (b.dead && b.trap) continue;
    if (b.dead && b.wall) { const SimB* bp = &b; items.push_back({b.x + b.y + 0.9f, [bp] { Pt c = iso(bp->x + .5f, bp->y + .5f); if (const SpriteDef* sp = findSprite("wallrubble_0")) drawSprite(*sp, c.x, c.y, cam.zoom, std::min(1.f, bp->deadT * 3)); }}); continue; }
    if (b.dead) {
      const SimB* bp = &b; float fall = bp->deadT;
      items.push_back({b.x + b.y + b.w * .5f + b.h * .5f - .5f, [bp, age, fall] {
        if (fall < 0.9f && spritesReady()) {                       // collapse: the building sinks and fades while the rubble fades in
          if (const SpriteDef* sp = buildingSprite(bp->type, bp->lvl, age)) { Pt c = iso(bp->x + bp->w / 2.f, bp->y + bp->h / 2.f); float k = fall / 0.9f; drawSprite(*sp, c.x, c.y + k * 14 * cam.zoom, cam.zoom, 1.f - k, 1.f - 0.3f * k); }
        }
        drawRubble(*bp, easeOut((fall - 0.35f) / 0.6f));
      }}); continue;
    }
    BView v{b.type, b.lvl, b.x, b.y, b.w, b.h, b.hp / b.maxhp, true, false, 0, false, 0};
    items.push_back({b.x + b.y + (b.w + b.h) / 2.f, [v, age] { drawBuilding(v, age); }});
  }
  for (auto& c : sim.corpses) {
    const Corpse* cp = &c;
    items.push_back({c.x + c.y - 0.01f, [cp] { int dir = dirIndex(cp->dx, cp->dy); float a = std::max(0.f, 1.f - cp->age / 1.0f); drawUnitSpr(cp->x, cp->y, cp->type, cp->def != (B.kind == K_DEFENCE), dir, 0, a * 0.85f, 0, -1, cp->age * 5.f); }});
  }
  for (auto& u : sim.units) {
    const SimUnit* up = &u;
    items.push_back({u.x + u.y, [up] {
      drawUnitSpr(up->x, up->y, up->type, up->def != (B.kind == K_DEFENCE), simFace(*up), simFrame(*up), 1.f, up->hitT);
      Pt p = iso(up->x, up->y); float hgt = (up->type == U_KNIGHT || up->type == U_PALADIN ? 58.f : 50.f) * cam.zoom;
      if (up->hp < up->maxhp) bar(p.x, p.y - hgt, U(24), up->hp / up->maxhp, up->def != (B.kind == K_DEFENCE) ? 0xe25555 : 0x6fe06f);
    }});
  }
  std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.d < b.d; });
  for (auto& it : items) it.f();
  for (auto& tr : sim.b) if (tr.trap && !tr.dead && tr.aux > 0) {
    Pt c = iso(tr.x + .5f, tr.y + .5f); float fl = std::sin(SDL_GetTicks() / 70.f + tr.x);
    G.circle(c.x, c.y - 6 * cam.zoom, (9 + fl * 2) * cam.zoom, 0xff7a1a, 0.55f); G.circle(c.x + 3 * cam.zoom, c.y - 12 * cam.zoom, (6 + fl) * cam.zoom, 0xffd060, 0.6f);
  }
  for (auto& p : sim.proj) drawProj(p);
  drawParticles();
  if (B.kind != K_DEFENCE && !sim.started) {                       // deployment edge, pulsing
    float pulse = 0.25f + 0.2f * std::sin(nowSec() * 3.f); polyLine({iso(1, 1), iso(MAP - 1, 1), iso(MAP - 1, MAP - 1), iso(1, MAP - 1)}, 0xffe678, pulse + 0.15f);
  }
}

// ============================================================ raid flow
static void goHome() {
  modal = M_NONE; view = V_HOME; centerCamera(); saveGame(); incoming = false;
}
static void beginBattle() {
  modal = M_NONE; B.sim = Sim(); B.sim.b = scout.base.buildings; B.sim.reserve = [] { return armyCount() - S.army[U_MONK]; };
  B.sim.mul[0] = techMul(); B.sim.mul[1] = enemyMul(scout.rival ? scout.tc : 1);
  B.kind = scout.rival ? K_RIVAL : K_BARB; B.done = false; B.sel = U_MILITIA;
  for (int i = 0; i < U_COUNT; i++) if (S.army[i] > 0) { B.sel = (UType)i; break; }
  B.sim.spawnGarrisons();
  view = V_BATTLE; selId = -1; buildMode = false; centerCamera();
}
static void startBarb(int i) {
  scout = Scout(); scout.rival = false; scout.i = i; scout.base = genBase(1000 + i, 1, true, i); scout.title = scout.base.name; scout.win = 4 + 2 * i; scout.lose = 0; modal = M_SCOUT;
}
static void findRival(bool first) {
  if (!first) { if (S.res[GOLD] < 30) { toast("Need 30 gold to search again"); return; } S.res[GOLD] -= 30; }
  S.rivalSeq++; const auto& L = lords(); std::vector<int> pool;
  for (int i = 0; i < (int)L.size(); i++) if (std::abs(lordRenown(L[i]) - S.renown) <= 250) pool.push_back(i);
  if (pool.empty()) { std::vector<int> all; for (int i = 0; i < (int)L.size(); i++) all.push_back(i); std::sort(all.begin(), all.end(), [&](int a, int b) { return std::abs(lordRenown(L[a]) - S.renown) < std::abs(lordRenown(L[b]) - S.renown); }); pool.assign(all.begin(), all.begin() + 8); }
  Rng r((uint32_t)(S.rivalSeq * 7919) + S.seed); int li = pool[(int)(r.next() * pool.size())];
  double a = r.next(), b2 = r.next(); int tc = clampi(tcLvl() + (a < 0.3 ? 1 : b2 < 0.3 ? -1 : 0), 1, MAXTC);
  scout = Scout(); scout.rival = true; scout.base = genBase((uint32_t)(li * 131 + S.rivalSeq), tc, false, 0);
  scout.title = L[li].name; scout.opp = lordRenown(L[li]); scout.tc = tc;
  double E = expectedScore(S.renown, scout.opp); scout.win = roundi(40 * (1 - E)); scout.lose = roundi(40 * E); modal = M_SCOUT;
}
static void finishAttack() {
  B.done = true; Sim& sim = B.sim; int stars = sim.stars(), pct = sim.pct();
  AtkResult r{}; r.kind = B.kind; r.stars = stars; r.pct = pct;
  for (auto& u : sim.units) if (!u.def) { S.army[u.type]++; r.surv[u.type]++; }
  for (int k = 0; k < NRES; k++) {
    float full = scout.base.loot.v[k] * (scout.rival ? 0.2f : 1.f); int v = (int)std::floor(full * pct / 100.f);
    int got = std::max(0, std::min(v, (int)(cap() - S.res[k]))); S.res[k] += got; r.got.v[k] = got;
  }
  r.win = stars >= 1; static const int sb[4] = {0, 0, 2, 4};
  r.dr = scout.rival ? (r.win ? scout.win + sb[stars] : -scout.lose) : (r.win ? scout.win : 0);
  int before = S.renown; S.renown = std::max(0, S.renown + r.dr); r.dr = S.renown - before;
  if (!scout.rival && r.win) S.camp[scout.i] = S.t + 180;
  atk = r; modal = M_ATK_RESULT; audioPlay(r.win ? SFX_VICTORY : SFX_DEFEAT);
}
static void startDefence() {
  incoming = false; modal = M_NONE; selId = -1; buildMode = false;
  B.sim = makeDefenceSim(); B.kind = K_DEFENCE; B.done = false; view = V_BATTLE; centerCamera();
}
static void finishDefence() { B.done = true; defRep = applyDefence(B.sim); hasDef = true; modal = M_DEF_RESULT; audioPlay(defRep.held ? SFX_VICTORY : SFX_DEFEAT); }
static void raidCheck() {
  if (tcLvl() < 2 || S.nextRaid <= 0 || modal != M_NONE || buildMode) return;
  if (!incoming && S.t >= S.nextRaid && S.t >= S.shieldUntil) { incoming = true; incomingAt = S.t + 20; audioPlay(SFX_HORN); }
  if (incoming && S.t >= incomingAt) startDefence();
}

// ============================================================ hud
static const char* hint() {
  if (buildMode && bmPaint) return "Tap or drag to place pieces. Tap a wall piece with the Gate tool to make a gate.";
  if (buildMode) return "Tap the map to move the building, then press Place";
  auto has = [](BType t) { for (auto& b : S.b) if (b.type == t) return true; return false; };
  if (!has(B_LUMBER)) return "1. Build a Lumber Camp (Build menu) next to the trees";
  if (!has(B_FARM)) return "2. Build a Farm for food";
  if (!has(B_BARRACKS)) return "3. Build a Barracks to train Militia";
  bool brDone = false; for (auto& b : S.b) if (b.type == B_BARRACKS && isDone(b)) brDone = true;
  if (armyCount() + queuedPop() < 3 && brDone) return "4. Select the Barracks and train some Militia";
  if (S.renown == 0 && armyCount() >= 3) return "5. Press Raid and attack Barbarian Camp 1";
  bool anyBuild = false; for (auto& b : S.b) if (b.building) anyBuild = true;
  if (tcLvl() < MAXTC && !anyBuild) return tcLvl() < 3 ? "Select the Town Center and upgrade it (Lv3 = Feudal Age)" : tcLvl() < 5 ? "Keep upgrading: Town Center Lv5 = Castle Age" : tcLvl() < 8 ? "Build a Castle (Lv6); Town Center Lv8 = Imperial Age" : "Build Bombard Towers and research Chemistry at the University";
  return "";
}
static float chip(float x, float y, const std::string& s, uint32_t dot = 0, const std::string& small = "") {
  float tw = G.textSize(s, F_M).x + (small.empty() ? 0 : G.textSize(small, F_S).x + U(4)), w = tw + U(20) + (dot ? U(16) : 0), h = U(32);
  panelBox(x, y, w, h, 2, 1.f);
  float tx = x + U(10); if (dot) { G.circle(tx + U(5), y + h / 2, U(5.5f), dot); tx += U(16); }
  G.textShadow(s, tx, y + U(5), F_M, C_TXT); if (!small.empty()) G.text(small, tx + G.textSize(s, F_M).x + U(4), y + U(10), F_S, C_DIM);
  return w;
}
static float chipIcon(float x, float y, const char* ic, const std::string& s, const std::string& small = "", float fill = -1.f, uint32_t fillCol = 0xffd060) {
  float iw = U(28), tw = G.textSize(s, F_M).x + (small.empty() ? 0 : G.textSize(small, F_S).x + U(5)), w = iw + tw + U(22), h = U(36);
  panelBox(x, y, w, h, 2, 1.f);
  icon(ic, x + U(17), y + h / 2 - U(1), U(30));
  G.textShadow(s, x + iw + U(9), y + U(5), F_M, C_TXT);
  if (!small.empty()) G.text(small, x + iw + U(9) + G.textSize(s, F_M).x + U(5), y + U(10), F_S, C_DIM);
  if (fill >= 0) { float bx = x + iw + U(8), bw = w - iw - U(16); G.rrect(bx, y + h - U(9), bw, U(3.5f), U(2), 0x000000, 0.55f); if (fill > 0.01f) G.rrect(bx, y + h - U(9), bw * std::min(1.f, fill), U(3.5f), U(2), fillCol, 0.95f); }
  return w;
}
static void modalFrame(float wFrac, float maxW, float& bx, float& by, float& bw, float& bh) {
  float a = easeOut(modalAnim);
  G.rect(0, 0, (float)G.W, (float)G.H, 0x000000, 0.6f * a); block(0, 0, (float)G.W, (float)G.H);
  bw = std::min(U(maxW), G.W - U(24)); bh = std::min(modalContentH + U(44), G.H - U(24));
  bx = (G.W - bw) / 2; by = (G.H - bh) / 2 + (1 - a) * U(28); (void)wFrac;
  panelBox(bx, by, bw, bh, 1, a);
  SDL_Rect clip{(int)(bx + U(5)), (int)(by + U(5)), (int)(bw - U(10)), (int)(bh - U(10))}; SDL_RenderSetClipRect(G.r, &clip);
}
static void modalEnd(float by, float bh, float endY, float startY) {
  SDL_RenderSetClipRect(G.r, nullptr);
  modalContentH = endY - startY + modalScroll;
  float maxScroll = std::max(0.f, modalContentH + U(44) - bh); modalScroll = clampf(modalScroll, 0, maxScroll);
  (void)by;
}
static void flowStars(Flow& f, int n) {                         // three big stars that pop in one after another
  if (f.x > f.x0) flowNl(f);
  float r = U(24), cx = f.x0 + f.maxW / 2, cy = f.y + r + U(2);
  for (int i = 0; i < 3; i++) {
    float t = easeBack((modalAge - 0.25f - i * 0.28f) / 0.35f); float sc = i < n ? t : 1.f; float x = cx + (i - 1) * r * 2.5f, y = cy + (i == 1 ? -r * 0.18f : 0);
    G.star(x, y + U(2), r * 1.05f * sc, 0x000000, 0.45f);
    if (i < n) { G.star(x, y, r * sc, 0xb07a14, 1.f); G.star(x, y, r * 0.86f * sc, 0xffd24a, 1.f); G.star(x, y - r * 0.08f, r * 0.5f * sc, 0xfff2b0, 0.55f); }
    else { G.star(x, y, r, 0x1a130d, 1.f); G.star(x, y, r * 0.84f, 0x3a2e22, 1.f); }
  }
  f.y += r * 2 + U(10);
}
// ---- minimap (diamond, like the classic RTS corner map)
static SDL_FRect miniR{0, 0, 0, 0};
static bool inDiamond(float px, float py, const SDL_FRect& r) { float cx = r.x + r.w / 2, cy = r.y + r.h / 2; return std::fabs(px - cx) / (r.w / 2) + std::fabs(py - cy) / (r.h / 2) <= 1.f; }
static void drawMinimap(float x, float y, float w, float h, bool battle) {
  miniR = {x, y, w, h}; block(x - U(6), y - U(6), w + U(12), h + U(12));
  float cx = x + w / 2, cy = y + h / 2;
  auto mp = [&](float gx, float gy) { return Pt{cx + (gx - gy) / MAP * (w / 2), cy + ((gx + gy) / MAP - 1) * (h / 2)}; };
  auto dia = [&](float grow) { return std::vector<Pt>{{cx, y - grow}, {x + w + grow * 2, cy}, {cx, y + h + grow}, {x - grow * 2, cy}}; };
  { auto sh = dia(U(9)); for (auto& p : sh) p.y += U(4); polyFill(sh, 0x000000, 0.35f); }
  polyFill(dia(U(5)), 0x0b0704); polyFill(dia(U(3.6f)), 0xcfa650); polyFill(dia(U(2.2f)), 0x2a1d12);
  polyFill(dia(0), 0x4d7b37);
  { Pt q[4] = {mp(0, 0), mp(MAP, 0), mp(MAP, MAP), mp(0, MAP)}; G.fillPoly(q, 4, 0x5f9444, 0.5f); }
  if (!battle) {
    for (auto& n : S.nodes) if (n.amt > 0) { Pt p = mp(n.x + .5f, n.y + .5f); G.circle(p.x, p.y, U(n.type == N_TREE ? 1.8f : 2.2f), n.type == N_TREE ? 0x1f4a1d : n.type == N_GOLD ? 0xf2c230 : 0xb0b4ba); }
    for (auto& b : S.b) { Pt p = mp(b.x + b.w / 2.f, b.y + b.h / 2.f); float r = U(b.w == 1 ? 1.2f : 1.2f + b.w * 0.9f); uint32_t c = b.type == B_TOWNHALL ? 0xffd34a : b.id == selId ? 0xffffff : (b.type == B_WALL || b.type == B_GATE) ? 0xb8b8b0 : 0x5a8ee8; Pt d[4] = {{p.x, p.y - r * 0.7f}, {p.x + r, p.y}, {p.x, p.y + r * 0.7f}, {p.x - r, p.y}}; G.fillPoly(d, 4, c, 1); }
  } else {
    for (auto& b : B.sim.b) { if (b.trap) continue; Pt p = mp(b.x + b.w / 2.f, b.y + b.h / 2.f); float r = U(b.w == 1 ? 1.1f : 1.2f + b.w * 0.9f); uint32_t c = b.dead ? 0x2a2420 : b.wall ? 0xa8a8a0 : (B.kind == K_DEFENCE ? 0x5a8ee8 : 0xd8503e); Pt d[4] = {{p.x, p.y - r * 0.7f}, {p.x + r, p.y}, {p.x, p.y + r * 0.7f}, {p.x - r, p.y}}; G.fillPoly(d, 4, c, 1); }
    for (auto& u : B.sim.units) { Pt p = mp(u.x, u.y); G.circle(p.x, p.y, U(2.2f), (u.def != (B.kind == K_DEFENCE)) ? 0xff6a50 : 0x7fb4ff); }
  }
  Pt vp[4]; Pt corners[4] = {toGrid(0, 0), toGrid((float)G.W, 0), toGrid((float)G.W, (float)G.H), toGrid(0, (float)G.H)};
  for (int i = 0; i < 4; i++) vp[i] = mp(clampf(corners[i].x, -2, MAP + 2), clampf(corners[i].y, -2, MAP + 2));
  G.strokePoly(vp, 4, 0xffffff, 0.9f);
  if (in.down && inDiamond(in.px, in.py, miniR) && modal == M_NONE) {
    float mx = clampf(in.mx, x, x + w), my = clampf(in.my, y, y + h);
    float a = (mx - cx) / (w / 2) * MAP, bsum = ((my - cy) / (h / 2) + 1) * MAP; float gx = clampf((a + bsum) / 2, 0, MAP), gy = clampf((bsum - a) / 2, 0, MAP);
    cam.x = G.W / 2.f - (gx - gy) * TW / 2 * cam.zoom; cam.y = G.H / 2.f - (gx + gy) * TH / 2 * cam.zoom; cam.vx = cam.vy = 0;
  }
}

static void openRaidChecks() {}

static void drawModal() {
  if (modal == M_NONE) return;
  float bx, by, bw, bh; modalFrame(0, 480, bx, by, bw, bh);
  float startY = by + U(20) - modalScroll; Flow f = flowAt(bx + U(20), startY, bw - U(40));
  switch (modal) {
    case M_INTRO: {
      flowTitle(f, "Dawn of Kings", C_GOLD);
      flowPara(f, "Your Town Center, a House and a Watch Tower are all you have. Gather resources, train troops, raid barbarians and rival lords, and climb the ranks from Peasant to Emperor.", C_TXT);
      flowPara(f, "Drag to pan. Mouse wheel / pinch to zoom. Tap buildings to select them. The speed button (top right) fast-forwards timers while you explore. Everything runs offline.");
      if (flowBtn(f, "Begin", 1)) { modal = M_NONE; S.seenIntro = true; }
      break;
    }
    case M_WELCOME: {
      flowTitle(f, "Welcome back", C_GOLD); flowPara(f, "You were away " + fmtTime(welcome.away) + ".");
      flowCost(f, "Gathered:", welcome.gain, false);
      if (welcome.raided) {
        flowTitle(f, welcome.rep.held ? "Raid repelled!" : "Village raided!", welcome.rep.held ? C_OK : C_BAD, F_M);
        flowPara(f, welcome.rep.who + " attacked while you were away: " + std::to_string(welcome.rep.stars) + " star(s), " + std::to_string(welcome.rep.pct) + "% damaged (rebuilt).", C_TXT);
        flowCost(f, "Stolen:", welcome.rep.stolen, false); flowPara(f, "Renown " + std::string(welcome.rep.dr >= 0 ? "+" : "") + std::to_string(welcome.rep.dr) + " -> " + std::to_string(S.renown), C_TXT);
      }
      if (flowBtn(f, "Continue", 1)) modal = M_NONE; break;
    }
    case M_BUILD: {
      flowTitle(f, "Build", C_TXT); flowPara(f, "Idle villagers: " + std::to_string(idleV()) + ". Each construction needs one villager for its duration. Walls, gates and traps are built instantly.");
      const char* cats[3] = {"Economy", "Military", "Defence"};
      for (int c = 0; c < 3; c++) {
        flowTitle(f, cats[c], C_GOLD, F_M);
        for (int t = 0; t < B_COUNT; t++) {
          const BDef& d = bdef(t); if (d.cat != c || d.unlock > tcLvl() + 1) continue;
          const char* err = canBuildErr((BType)t); float rh = U(60);
          if (uiButton(f.x0, f.y, f.maxW, rh, "", 0, err == nullptr)) {
            if (isPiece(t)) { if (canAfford(d.cost)) { modal = M_NONE; buildMode = true; bmPaint = true; paintOn = true; bmType = (BType)t; selId = -1; bmx = bmy = -1; } else toast("Not enough resources"); }
            else if (canAfford(d.cost) && (idleV() >= 1 || assigned() >= 1)) { modal = M_NONE; buildMode = true; bmPaint = false; bmType = (BType)t; selId = -1; Pt c0 = toGrid(G.W / 2.f, G.H / 2.f); bmx = roundi(c0.x - d.w / 2.0); bmy = roundi(c0.y - d.h / 2.0); }
            else toast(idleV() < 1 && assigned() < 1 ? "No villager available" : "Not enough resources");
          }
          float tx0 = f.x0 + U(66);
          if (spritesReady()) { const SpriteDef* sp = isPiece(t) ? pieceSprite((BType)t, 1, 0, 0) : buildingSprite((BType)t, 1, std::min(ageOf(tcLvl()), 3)); if (sp) drawIcon(*sp, f.x0 + U(33), f.y + rh / 2, U(54), err ? 0.4f : 1.f); }
          G.text(d.name, tx0, f.y + U(7), F_M, err ? 0x8a8478 : C_TXT);
          G.text(err ? err : d.desc, tx0, f.y + U(32), F_S, err ? C_BAD : C_DIM);
          drawCost(f.x0 + f.maxW - U(10), f.y + U(7), d.cost, A_RIGHT);
          G.text((isPiece(t) ? std::string("instant") : "time " + fmtTime(d.time)) + "  " + std::to_string(buildCount((BType)t)) + "/" + std::to_string(bmaxn(t, tcLvl())), f.x0 + f.maxW - U(10), f.y + U(32), F_S, C_DIM, A_RIGHT);
          f.y += rh + U(6);
        }
      }
      if (flowBtn(f, "Close")) modal = M_NONE; break;
    }
    case M_ARMY: {
      flowTitle(f, "Army", C_TXT); flowPara(f, "Housing " + std::to_string(armyCount()) + " + " + std::to_string(queuedPop()) + " queued / " + std::to_string(housing()) + ". Train units at the Barracks and Archery Range.");
      for (int i = 0; i < U_COUNT; i++) {
        const UDef& u = udef(i); bool locked = u.unlock > tcLvl(); float rh = U(52);
        G.gradRRect(f.x0, f.y, f.maxW, rh, U(8), 0x3a2c20, 0x261a12, 0.9f); G.rrect(f.x0, f.y, f.maxW, U(1.2f), U(1), 0xcfa650, 0.35f);
        if (const SpriteDef* sp = unitSpriteFor(i, false, 3, 0)) drawIcon(*sp, f.x0 + U(30), f.y + rh / 2 + U(2), U(54), locked ? 0.35f : 1.f);
        G.text(u.name, f.x0 + U(64), f.y + U(6), F_M, locked ? 0x8a8478 : C_TXT);
        std::string det = "HP " + std::to_string(u.hp) + "   DMG " + std::to_string(u.dmg) + "   " + (u.ranged ? "ranged" : "melee") + "   pop " + std::to_string(u.pop) + (locked ? "   needs TC Lv" + std::to_string(u.unlock) : "");
        G.text(det, f.x0 + U(64), f.y + U(30), F_S, locked ? C_BAD : C_DIM);
        G.textShadow("x" + std::to_string(S.army[i]), f.x0 + f.maxW - U(12), f.y + U(12), F_L, S.army[i] ? C_GOLD : 0x6a6258, A_RIGHT);
        f.y += rh + U(5);
      }
      if (flowBtn(f, "Close")) modal = M_NONE; break;
    }
    case M_LADDER: {
      struct E { std::string n; int r; bool me; }; std::vector<E> all; for (auto& l : lords()) all.push_back({l.name, lordRenown(l), false}); all.push_back({"You", S.renown, true});
      std::stable_sort(all.begin(), all.end(), [](const E& a, const E& b) { return a.r != b.r ? a.r > b.r : a.me && !b.me; });
      int me = 0; for (int i = 0; i < (int)all.size(); i++) if (all[i].me) me = i;
      flowTitle(f, "Ladder", C_TXT);
      flowPara(f, "Season " + std::to_string(S.season) + "  -  ends in " + fmtTime(seasonLeft()) + "  -  raids won " + std::to_string(S.sWins) + ", defences held " + std::to_string(S.sDef), C_DIM, F_S);
      flowPara(f, std::string("Rank: ") + rankOf(S.renown) + "   Position #" + std::to_string(me + 1) + " of " + std::to_string(all.size()) + "   Renown " + std::to_string(S.renown), C_GOLD);
      int from = clampi(me - 5, 0, (int)all.size() - 11);
      for (int i = from; i < from + 11; i++) {
        uint32_t c = all[i].me ? C_GOLD : C_TXT;
        G.text("#" + std::to_string(i + 1), f.x0, f.y, F_M, c); G.text(all[i].n, f.x0 + U(56), f.y, F_M, c);
        G.text(rankOf(all[i].r), f.x0 + f.maxW - U(70), f.y, F_S, C_DIM, A_RIGHT); G.text(std::to_string(all[i].r), f.x0 + f.maxW, f.y, F_M, c, A_RIGHT); f.y += U(24);
      }
      f.y += U(8); if (flowBtn(f, "Close")) modal = M_NONE; break;
    }
    case M_RAID: {
      flowTitle(f, "Raid", C_TXT);
      std::string a; for (int i = 0; i < U_COUNT; i++) if (S.army[i]) a += std::string(udef(i).name) + " x" + std::to_string(S.army[i]) + "   ";
      flowPara(f, "Army: " + (a.empty() ? std::string("none - train troops at the Barracks") : a));
      flowTitle(f, "Barbarian Camps", C_GOLD, F_M);
      for (int i = 1; i <= 5; i++) {
        Base g = genBase(1000 + i, 1, true, i); bool locked = i > tcLvl() + 1; double cd = std::max(0.0, S.camp[i] - S.t); float rh = U(52);
        bool en = !locked && cd <= 0 && armyCount() > 0;
        if (uiButton(f.x0, f.y, f.maxW, rh, "", 0, en)) startBarb(i);
        G.text("Camp " + std::to_string(i), f.x0 + U(10), f.y + U(5), F_M, en ? C_TXT : 0x7a8696);
        std::string sub = locked ? "Needs Town Center Lv" + std::to_string(i - 1) : cd > 0 ? "Recovering " + fmtTime(cd) : std::to_string(g.buildings.size()) + " buildings";
        G.text(sub, f.x0 + U(10), f.y + U(28), F_S, C_DIM); drawCost(f.x0 + f.maxW - U(10), f.y + U(16), g.loot, A_RIGHT, false); f.y += rh + U(6);
      }
      flowTitle(f, "Rival Lords", C_GOLD, F_M);
      if (uiButton(f.x0, f.y, f.maxW, U(52), "", 0, armyCount() > 0)) findRival(true);
      G.text("Find a Rival", f.x0 + U(10), f.y + U(5), F_M, C_TXT); G.text("Ranked: wins raise Renown, losses lower it", f.x0 + U(10), f.y + U(28), F_S, C_DIM); f.y += U(58);
      if (flowBtn(f, "Close")) modal = M_NONE; break;
    }
    case M_SCOUT: {
      flowTitle(f, scout.title, C_GOLD);
      if (scout.rival) flowPara(f, std::string(PERSONALITY[scout.base.pers]) + "  -  " + rankOf(scout.opp) + "  -  " + std::to_string(scout.opp) + " Renown  -  Town Center Lv" + std::to_string(scout.tc));
      Cost shown; for (int i = 0; i < NRES; i++) shown.v[i] = (int)(scout.base.loot.v[i] * (scout.rival ? 0.2 : 1.0));
      flowCost(f, "Available loot:", shown, false);
      int towers = 0; for (auto& b : scout.base.buildings) if (b.dps > 0) towers++;
      flowPara(f, std::to_string(towers) + " towers, " + std::to_string(scout.base.buildings.size()) + " buildings.");
      flowPara(f, "Renown: +" + std::to_string(scout.win) + " for a win, -" + std::to_string(scout.lose) + " for a loss.", C_TXT);
      if (flowBtn(f, "Attack", 1)) beginBattle();
      if (scout.rival && flowBtn(f, "Next (30 gold)")) findRival(false);
      if (flowBtn(f, "Cancel")) modal = M_NONE; break;
    }
    case M_ATK_RESULT: {
      flowTitle(f, atk.win ? "Victory!" : "Defeat", atk.win ? C_OK : C_BAD);
      flowStars(f, atk.stars); flowPara(f, std::to_string(atk.pct) + "% of the base destroyed", C_GOLD, F_M);
      flowCost(f, "Loot:", atk.got, false);
      flowPara(f, "Renown " + std::string(atk.dr >= 0 ? "+" : "") + std::to_string(atk.dr) + " -> " + std::to_string(S.renown) + " (" + rankOf(S.renown) + ")", C_TXT);
      std::string sv; for (int i = 0; i < U_COUNT; i++) if (atk.surv[i]) sv += std::string(udef(i).name) + " x" + std::to_string(atk.surv[i]) + "  ";
      flowPara(f, "Surviving troops return home: " + (sv.empty() ? std::string("none") : sv));
      if (flowBtn(f, "Return Home", 1)) goHome(); break;
    }
    case M_DEF_RESULT: {
      flowTitle(f, defRep.held ? "Raid repelled!" : "Village raided!", defRep.held ? C_OK : C_BAD);
      flowStars(f, defRep.stars); flowPara(f, defRep.who + " attacked your village. " + std::to_string(defRep.pct) + "% damaged (rebuilt for free).", C_TXT);
      flowCost(f, "Stolen:", defRep.stolen, false);
      flowPara(f, "Renown " + std::string(defRep.dr >= 0 ? "+" : "") + std::to_string(defRep.dr) + " -> " + std::to_string(S.renown), C_TXT);
      flowPara(f, "Tip: more towers and upgrades reduce losses. A shield protects you for a short while.");
      if (flowBtn(f, "Continue", 1)) goHome(); break;
    }
    case M_SEASON: {
      flowTitle(f, "Season " + std::to_string(seasonRes.season) + " has ended", C_GOLD);
      flowPara(f, "You finished #" + std::to_string(seasonRes.pos) + " of 301 as " + rankOf(seasonRes.finalRenown) + " with " + std::to_string(seasonRes.finalRenown) + " Renown. Raids won: " + std::to_string(seasonRes.wins) + ", defences held: " + std::to_string(seasonRes.defs) + ".", C_TXT);
      flowCost(f, "Season chest:", seasonRes.reward, false);
      flowPara(f, "A new season begins. Renown above 3000 is halved towards 3000: you start at " + std::to_string(seasonRes.newRenown) + " (" + rankOf(seasonRes.newRenown) + "). The lords' ladder was reshuffled too.", C_DIM);
      if (flowBtn(f, "Begin the next season", 1)) modal = M_NONE; break;
    }
    case M_SETTINGS: {
      flowTitle(f, "Options", C_TXT);
      if (flowBtn(f, std::string("Music: ") + (S.musicOn ? "ON" : "OFF"), S.musicOn ? 1 : 0)) { S.musicOn = !S.musicOn; audioSetEnabled(S.musicOn, S.sfxOn); saveGame(); }
      if (flowBtn(f, std::string("Sound effects: ") + (S.sfxOn ? "ON" : "OFF"), S.sfxOn ? 1 : 0)) { S.sfxOn = !S.sfxOn; audioSetEnabled(S.musicOn, S.sfxOn); saveGame(); }
      flowNl(f);
      if (!audioDeviceOpen()) flowPara(f, "No audio output device was found, so the game is silent.", C_BAD, F_S);
      flowPara(f, "Season " + std::to_string(S.season) + " ends in " + fmtTime(seasonLeft()) + ". All music and sound is generated on your device; the game never goes online.", C_DIM, F_S);
      flowPara(f, "Controls: drag = pan, wheel / pinch = zoom, tap = select / place / deploy. Dev keys: F1 resources, F2 raid, F3 town center level, F4 end the season.", C_DIM, F_S);
      if (flowBtn(f, "Close", 1)) modal = M_NONE; break;
    }
    default: break;
  }
  modalEnd(by, bh, f.y + f.rowH, startY);
  if (modal != M_INTRO && modal != M_WELCOME && modal != M_SEASON && modal != M_ATK_RESULT && modal != M_DEF_RESULT) {
    float xs = U(30); clickGate = true;
    if (uiButton(bx + bw - xs - U(10), by + U(10), xs, xs, "X", 2, true, F_M)) modal = M_NONE;
  }
  // scrolling by drag inside the modal
  if (in.down && in.dragged && inRect({bx, by, bw, bh}, in.px, in.py)) modalScroll -= in.my - in.lasty;
}

static void drawPanel() {
  Bld* b = findB(selId); if (!b || modal != M_NONE || buildMode || view != V_HOME) { return; }
  const BDef& d = bdef(b->type);
  float w = std::min(U(440), G.W - U(16)), x = (G.W - w) / 2, hh = panelContentH + U(24), y = G.H - U(94) - hh;
  panelBox(x, y, w, hh, 0, 0.97f); block(x, y - U(40), w, hh + U(40));
  if (spritesReady()) { const SpriteDef* psp = isPiece(b->type) ? pieceSprite(b->type, isPiece(b->type) ? (b->type == B_SPIKE || b->type == B_OIL ? S.trapLvl : S.wallLvl) : b->lvl, b->x, b->y) : buildingSprite(b->type, std::max(1, b->lvl), std::min(ageOf(tcLvl()), 3)); if (psp) drawIcon(*psp, x + w - U(78), y + U(34), U(76)); }
  { float xs = U(28); if (uiButton(x + w - xs - U(8), y + U(8), xs, xs, "X", 2, true, F_M)) { selId = -1; return; } }
  Flow f = flowAt(x + U(16), y + U(14), w - U(32));
  if (isPiece(b->type)) {
    bool trap = b->type == B_SPIKE || b->type == B_OIL; int lv = trap ? S.trapLvl : S.wallLvl;
    flowTitle(f, std::string(d.name) + "  Lv" + std::to_string(lv), C_TXT, F_M); f.y -= U(2);
    flowPara(f, d.desc, C_DIM, F_S);
    if (trap) flowPara(f, std::to_string(bdps(b->type, lv)) + (b->type == B_SPIKE ? " damage on contact" : " damage/s for 3s, slows") + ", radius " + std::to_string((int)(d.range * 10) / 10.0).substr(0, 3), C_DIM, F_S);
    else flowPara(f, std::to_string(bhp(b->type, lv)) + " health. " + std::to_string(pieceCount(false)) + " wall points placed (gate = 5).", C_DIM, F_S);
    const char* blk = trap ? trapUpgradeBlock() : wallUpgradeBlock();
    Cost uc = trap ? trapUpgradeCost() : wallUpgradeCost();
    if (blk) flowBtn(f, std::string(trap ? "All traps: " : "All walls: ") + blk, 0, false);
    else if (flowBtn(f, std::string(trap ? "Upgrade all traps to Lv" : "Upgrade all walls to Lv") + std::to_string(lv + 1) + "  " + costStr(uc), 1, canAfford(uc))) { if (trap) upgradeTraps(); else upgradeWalls(); }
    flowNl(f);
    if (flowBtn(f, "Remove (refund half)", 2)) { int id = b->id; selId = -1; removePiece(id); return; }
    flowNl(f); panelContentH = f.y - (y + U(10)); return;
  }
  int shownLvl = b->building ? b->bTo : b->lvl;
  flowTitle(f, std::string(d.name) + "  Lv" + std::to_string(shownLvl), C_TXT, F_M); f.y -= U(2);
  if (d.desc[0]) flowPara(f, d.desc, C_DIM, F_S);
  if (b->building) {
    double left = b->bEnd - S.t; float frac = clampf(1 - (float)(left / (b->bEnd - b->bStart)), 0, 1);
    flowPara(f, std::string(b->lvl == 0 ? "Constructing" : "Upgrading to Lv" + std::to_string(b->bTo)) + "... " + fmtTime(left), C_TXT);
    G.rect(f.x0, f.y - U(2), f.maxW, U(8), 0x000000, 0.4f); G.rect(f.x0, f.y - U(2), f.maxW * frac, U(8), C_GOLD); f.y += U(12);
  }
  if (d.res >= 0 && isDone(*b)) {
    int slots = bslots(b->type, b->lvl); float mult = 1 + 0.15f * (b->lvl - 1);
    flowText(f, "Workers", C_TXT);
    if (flowBtn(f, "-", 0, true, U(40))) setWorkers(*b, -1);
    flowText(f, std::to_string(b->workers) + "/" + std::to_string(slots), C_GOLD);
    if (flowBtn(f, "+", 0, true, U(40))) setWorkers(*b, 1);
    flowText(f, "~" + std::to_string(roundi(b->workers * d.rate * mult)) + " " + RES_NAME[d.res] + "/min", C_DIM, F_S); flowNl(f);
    if (b->status == 4) flowPara(f, "No resources left nearby (they regrow in 5 min).", C_BAD, F_S);
    if (b->status == 3) flowPara(f, "Storage full - spend some!", C_BAD, F_S);
  }
  if (b->type == B_TOWNHALL) {
    if (flowBtn(f, "Train Villager (25F)")) trainVillager();
    flowText(f, std::to_string(S.villagers) + "/" + std::to_string(vCap()) + " queue " + std::to_string(S.vq.size()), C_DIM, F_S); flowNl(f);
  }
  bool anyTrain = false;
  for (int i = 0; i < U_COUNT; i++) { const UDef& u = udef(i); if (u.bld != b->type || u.unlock > tcLvl() || !isDone(*b)) continue; anyTrain = true; if (flowBtn(f, std::string(u.name) + "  " + costStr(u.cost))) trainUnit(*b, (UType)i); }
  if (anyTrain) {
    flowNl(f); flowPara(f, "Army " + std::to_string(armyCount()) + "+" + std::to_string(queuedPop()) + " / " + std::to_string(housing()) + " housing", C_DIM, F_S);
    if (!b->q.empty()) { std::string q = "Training: "; for (auto& it : b->q) q += std::string(udef(it.type).tag) + " "; flowPara(f, q + "(" + std::to_string((int)std::ceil(b->q[0].left)) + "s)", C_DIM, F_S); }
  }
  if (isDone(*b) && d.dps[0] > 0) flowPara(f, std::to_string(bdps(b->type, b->lvl)) + " damage/s, range " + std::to_string((int)d.range), C_DIM, F_S);
  if (isDone(*b) && b->type == B_CASTLE) flowPara(f, "Garrison: " + std::to_string(2 + b->lvl) + " defenders (Long Swordsmen and Knights) sally out when raiders attack.", C_DIM, F_S);
  if (isDone(*b) && b->type == B_MARKET) {
    for (int k = 0; k < 4; k++) {
      Cost g = tradeGive(k), r = tradeGet(k);
      if (flowBtn(f, std::string(tradeLabel(k)) + "  " + costStr(g) + " -> " + costStr(r), 0, canAfford(g))) trade(k);
      flowNl(f);
    }
  }
  if (isDone(*b) && (b->type == B_BLACKSMITH || b->type == B_UNIVERSITY)) {
    if (S.researchTech >= 0) flowPara(f, std::string("Researching ") + techdef(S.researchTech).name + "... " + fmtTime(S.researchEnd - S.t), C_GOLD, F_S);
    for (int k = 0; k < T_COUNT; k++) {
      const TechDef& td = techdef(k); if (td.bld != b->type) continue;
      const char* blk = techBlock(k);
      std::string lbl = std::string(td.name) + " " + std::to_string(S.tech[k]) + "/" + std::to_string(td.maxLvl);
      if (S.tech[k] < td.maxLvl) lbl += "  " + costStr(techCost(k)) + "  " + fmtTime(techTime(k));
      if (flowBtn(f, lbl, blk ? 0 : 1, blk == nullptr && canAfford(techCost(k)))) startResearch(k);
      flowNl(f); flowPara(f, td.desc, C_DIM, F_S);
    }
  }
  if (isDone(*b) && !b->building) {
    UpInfo u = upgradeInfo(*b);
    if (u.err) flowBtn(f, u.err, 0, false);
    else { if (flowBtn(f, u.label + "  " + costStr(u.cost) + "  " + fmtTime(u.time), 1)) startUpgrade(*b); }
  }
  flowNl(f); panelContentH = f.y - (y + U(10));
}

static float lastStarsShown = 0;
static void drawToasts() {
  if (toastT > 0) {
    float in_ = easeOut((2.0f - toastT) * 7.f), out = std::min(1.f, toastT * 2.5f), a = std::min(in_, out); float w = G.textSize(toastMsg, F_M).x + U(40), h = U(38);
    float x = (G.W - w) / 2, y = G.H - U(132) + (1 - in_) * U(16);
    G.shadowBox(x, y, w, h, U(19), 0.4f * a, U(8)); G.rrect(x - U(1.2f), y - U(1.2f), w + U(2.4f), h + U(2.4f), U(20), 0xcfa650, a);
    G.gradRRect(x, y, w, h, U(19), 0x3a2c20, 0x1c140e, a); G.text(toastMsg, G.W / 2.f, y + U(8), F_M, C_TXT, A_CENTER, a);
  }
  if (bannerT > 0) {
    float in_ = easeBack((3.0f - bannerT) * 4.f), out = std::min(1.f, bannerT * 1.5f), a = std::min(1.f, in_) * out; float bh = U(64) * std::min(1.f, in_);
    float y = U(150); G.gradRect(0, y - bh / 2, (float)G.W, bh, 0x000000, 0x000000, 0.55f * a);
    G.gradRect(G.W * 0.18f, y - bh / 2, G.W * 0.64f, U(2), 0xe9c466, 0xe9c466, 0.9f * a); G.gradRect(G.W * 0.18f, y + bh / 2 - U(2), G.W * 0.64f, U(2), 0xe9c466, 0xe9c466, 0.9f * a);
    G.textShadow(bannerMsg, G.W / 2.f, y - G.textSize("Ag", F_T).y / 2, F_T, 0xfff0c0, A_CENTER, a);
  }
}
static void drawHud() {
  float pad = U(8);
  if (!dispInit) { for (int i = 0; i < NRES; i++) dispRes[i] = S.res[i]; dispInit = true; }
  if (view == V_HOME) {
    float x = pad;
    float rw[NRES], total = 0; std::string rs[NRES], cs[NRES];
    for (int k = 0; k < NRES; k++) { rs[k] = std::to_string((int)std::round(dispRes[k])); cs[k] = "/" + std::to_string(cap()); rw[k] = U(28) + G.textSize(rs[k], F_M).x + G.textSize(cs[k], F_S).x + U(27); total += rw[k] + U(6); }
    for (int k = 0; k < NRES; k++) x += chipIcon(x, pad, RES_ICON[k], rs[k], cs[k], S.res[k] / (float)cap(), RES_COL[k] == 0x8a5a2b ? 0xc89050 : RES_COL[k]) + U(6);
    std::string rk = std::string(rankOf(S.renown)); std::string sh = S.shieldUntil > S.t ? "  shield " + fmtTime(S.shieldUntil - S.t) : "";
    std::string pop = std::to_string(idleV()) + " idle", popS = "/ " + std::to_string(S.villagers) + "/" + std::to_string(vCap()), ageS = ageName(tcLvl()), ageT = "TC" + std::to_string(tcLvl()), renS = std::to_string(S.renown);
    float widths[3] = {U(28) + G.textSize(pop, F_M).x + G.textSize(popS, F_S).x + U(27), U(28) + G.textSize(ageS, F_M).x + G.textSize(ageT, F_S).x + U(27), U(28) + G.textSize(renS, F_M).x + G.textSize(rk + sh, F_S).x + U(27)};
    float btnW = U(70), totalR = widths[0] + widths[1] + widths[2] + btnW + U(24), rx = G.W - totalR - pad; bool wrapRow = rx < x; float ry = wrapRow ? pad + U(44) : pad; if (wrapRow) rx = pad;
    chipIcon(rx, ry, "icon_pop", pop, popS); rx += widths[0] + U(6);
    chipIcon(rx, ry, "icon_shield", ageS, ageT); rx += widths[1] + U(6);
    chipIcon(rx, ry, "icon_trophy", renS, rk + sh); rx += widths[2] + U(6);
    if (uiButton(rx, ry + U(2), btnW, U(32), "x" + std::to_string((int)speedMul), 0, true, F_S)) speedMul = speedMul == 1 ? 5 : speedMul == 5 ? 20 : 1;
    // hint ribbon
    const char* h = hint();
    if (h[0] && modal == M_NONE) {
      float w = G.textSize(h, F_M).x + U(40), hy = pad + U(wrapRow ? 92 : 50), hx = (G.W - w) / 2;
      G.shadowBox(hx, hy, w, U(34), U(17), 0.35f, U(8)); G.rrect(hx - U(1.2f), hy - U(1.2f), w + U(2.4f), U(36.4f), U(18), 0x3a2208, 1.f); G.gradRRect(hx, hy, w, U(34), U(17), 0xf6cc62, 0xb07a1c, 1.f);
      G.text(h, G.W / 2.f, hy + U(6), F_M, 0x2b1806, A_CENTER);
    }
    if (incoming) {
      std::string a = "Raiders approach!  " + std::to_string((int)std::ceil(incomingAt - S.t)) + "s"; float w = G.textSize(a, F_M).x + U(40), pulse = 0.75f + 0.25f * std::sin(nowSec() * 7.f), hy = pad + U(wrapRow ? 134 : 92);
      G.shadowBox((G.W - w) / 2, hy, w, U(34), U(17), 0.4f, U(8)); G.gradRRect((G.W - w) / 2, hy, w, U(34), U(17), shadec(0xd2523e, 0.1f * pulse), 0x7e241a, 1.f); G.textShadow(a, G.W / 2.f, hy + U(6), F_M, 0xfff0e8, A_CENTER);
    }
    // bottom command bar
    float mw = U(206), mh = U(106), mx = G.W - pad - mw - U(6), my = G.H - pad - mh - U(2);
    drawMinimap(mx, my, mw, mh, false);
    if (!buildMode) {
      float bw = U(80), bh = U(70), by = G.H - pad - bh, bx = pad;
      if (iconButton(bx, by, bw, bh, "icon_hammer", "Build", 1)) { modal = M_BUILD; modalScroll = 0; } bx += bw + U(8);
      if (iconButton(bx, by, bw, bh, "icon_shield", "Army", 0)) { modal = M_ARMY; modalScroll = 0; } bx += bw + U(8);
      if (iconButton(bx, by, bw, bh, "icon_trophy", "Ladder", 0)) { modal = M_LADDER; modalScroll = 0; } bx += bw + U(8);
      if (iconButton(bx, by, bw, bh, "icon_gear", "Options", 0)) { modal = M_SETTINGS; modalScroll = 0; }
      float rw2 = U(120);
      if (iconButton(mx - rw2 - U(12), by, rw2, bh, "icon_swords", "RAID", 2)) { modal = M_RAID; modalScroll = 0; }
    } else if (bmPaint) {
      float by = G.H - U(52) - pad, w = U(150);
      std::string msg = std::string(bdef(bmType).name) + " " + std::to_string(buildCount(bmType)) + "/" + std::to_string(bmaxn(bmType, tcLvl())) + "  cost each " + costStr(bdef(bmType).cost) + (paintOn ? "  -  tap or drag to place" : "  -  drag to pan");
      float mw2 = G.textSize(msg, F_M).x + U(28); panelBox((G.W - mw2) / 2, by - U(46), mw2, U(36), 2, 1.f); G.text(msg, G.W / 2.f, by - U(40), F_M, C_TXT, A_CENTER);
      if (uiButton(G.W / 2 - w - pad / 2, by, w, U(48), "Done", 1)) { buildMode = false; bmPaint = false; saveGame(); }
      if (uiButton(G.W / 2 + pad / 2, by, w, U(48), paintOn ? "Switch to Pan" : "Switch to Paint")) paintOn = !paintOn;
    } else {
      float by = G.H - U(52) - pad, w = U(130);
      if (uiButton(G.W / 2 - w - pad / 2, by, w, U(48), "Place", 1, bmOk)) {
        const BDef& d = bdef(bmType);
        if (canAfford(d.cost) && takeVillager()) { pay(d.cost); audioPlay(SFX_PLACE); Bld& nb = addB(bmType, bmx, bmy, 0); nb.building = true; nb.bStart = (float)S.t; nb.bEnd = (float)S.t + d.time; nb.bTo = 1; buildMode = false; fxSpawn(FX_DUST, bmx + d.w / 2.f, bmy + d.h / 2.f, 6, 14); saveGame(); }
        else { buildMode = false; toast("Not enough resources / villagers"); }
      }
      if (uiButton(G.W / 2 + pad / 2, by, w, U(48), "Cancel", 2)) buildMode = false;
    }
  } else {
    Sim& sim = B.sim; int stars = sim.stars();
    std::string tm = fmtTime(sim.timeLeft), pc = std::to_string(sim.pct()) + "%";
    float w1 = U(30) + G.textSize(tm, F_M).x, w2 = U(30) + G.textSize(pc, F_M).x, w3 = U(88), total = w1 + w2 + w3 + U(40), x = (G.W - total) / 2;
    panelBox(x, pad, total, U(40), 2, 1.f);
    icon("icon_clock", x + U(20), pad + U(21), U(28)); G.textShadow(tm, x + U(36), pad + U(8), F_M, C_TXT);
    icon("icon_swords", x + w1 + U(34), pad + U(21), U(28)); G.textShadow(pc, x + w1 + U(50), pad + U(8), F_M, C_TXT);
    if (stars > lastStarsShown) lastStarsShown = (float)stars; if (sim.t < 0.2f) lastStarsShown = 0;
    for (int i = 0; i < 3; i++) { float sx = x + w1 + w2 + U(46) + i * U(26), sy = pad + U(21); if (i < stars) { G.star(sx, sy, U(11), 0xb07a14); G.star(sx, sy, U(9), 0xffd24a); } else { G.star(sx, sy, U(11), 0x1a130d); G.star(sx, sy, U(9), 0x3a2e22); } }
    std::string hh = B.kind == K_DEFENCE ? "Your village is under attack!" : (sim.started ? "" : "Tap the grass to deploy troops (outside the walls, away from buildings)");
    if (!hh.empty()) { float w = G.textSize(hh, F_M).x + U(40), hy = pad + U(48); G.shadowBox((G.W - w) / 2, hy, w, U(34), U(17), 0.35f, U(8)); G.gradRRect((G.W - w) / 2, hy, w, U(34), U(17), B.kind == K_DEFENCE ? 0xd2523e : 0xf6cc62, B.kind == K_DEFENCE ? 0x7e241a : 0xb07a1c, 1.f); G.text(hh, G.W / 2.f, hy + U(6), F_M, B.kind == K_DEFENCE ? 0xfff0e8 : 0x2b1806, A_CENTER); }
    float mw = U(206), mh = U(106), mx = G.W - pad - mw - U(6), my = G.H - pad - mh - U(2);
    drawMinimap(mx, my, mw, mh, true);
    if (B.kind != K_DEFENCE && modal == M_NONE) {
      float bw = U(76), bh = U(74), by = G.H - pad - bh, bx = pad;
      for (int i = 0; i < U_COUNT; i++) if (S.army[i] > 0) {
        bool sel = B.sel == i; clickGate = true;
        if (uiButton(bx, by, bw, bh, "", sel ? 1 : 0)) B.sel = (UType)i;
        if (const SpriteDef* sp = unitSpriteFor(i, false, 3, 0)) drawIcon(*sp, bx + bw / 2, by + bh * 0.42f, bh * 0.66f);
        std::string cnt = "x" + std::to_string(S.army[i]); G.textShadow(cnt, bx + bw - U(7), by + bh - U(20), F_M, sel ? 0xffffff : 0xffe9a8, A_RIGHT);
        bx += bw + U(6);
      }
      if (uiButton(mx - U(126) - U(10), by, U(126), U(48), "End Battle", 2)) B.sim.over = true;
    }
  }
  drawToasts();
}

// ============================================================ world taps
static void paintAt(float sx, float sy) {
  Pt g = toGrid(sx, sy); int x = (int)std::floor(g.x), y = (int)std::floor(g.y); bmx = x; bmy = y;
  const char* e = placePieceBlock(bmType, x, y);
  if (e) { if (std::strcmp(e, "Tile is occupied") != 0) toast(e); return; }
  placePiece(bmType, x, y);
}
// tiles a raider can reach from outside the walls (flood fill from the map edge); used to stop deploying inside a fortress
static bool reachableFromEdge(int tx, int ty) {
  static bool seen[MAP * MAP]; std::fill(seen, seen + MAP * MAP, false); std::vector<int> st;
  auto push = [&](int x, int y) { if (x < 0 || y < 0 || x >= MAP || y >= MAP) return; int i = y * MAP + x; if (seen[i] || B.sim.wallIdx[i] >= 0) return; seen[i] = true; st.push_back(i); };
  for (int i = 0; i < MAP; i++) { push(i, 0); push(i, MAP - 1); push(0, i); push(MAP - 1, i); }
  while (!st.empty()) { int c = st.back(); st.pop_back(); int x = c % MAP, y = c / MAP; push(x + 1, y); push(x - 1, y); push(x, y + 1); push(x, y - 1); }
  return tx >= 0 && ty >= 0 && tx < MAP && ty < MAP && seen[ty * MAP + tx];
}
static void onTap(float sx, float sy) {
  Pt g = toGrid(sx, sy);
  if (view == V_BATTLE) {
    if (B.kind == K_DEFENCE || B.done) return;
    if (S.army[B.sel] <= 0) { toast("No troops selected"); return; }
    if (g.x < 0.3f || g.y < 0.3f || g.x > MAP - 0.3f || g.y > MAP - 0.3f) return;
    for (auto& b : B.sim.b) if (!b.dead && !b.wall && !b.trap && distRect(g.x, g.y, b) < 2.5f) { toast("Deploy farther from buildings"); return; }
    if (!reachableFromEdge((int)g.x, (int)g.y)) { toast("Deploy outside the walls"); return; }
    S.army[B.sel]--; B.sim.deploy(B.sel, g.x, g.y);
    if (S.army[B.sel] <= 0) for (int i = 0; i < U_COUNT; i++) if (S.army[i] > 0) { B.sel = (UType)i; break; }
    return;
  }
  if (buildMode && bmPaint) return;
  if (buildMode) { const BDef& d = bdef(bmType); bmx = roundi(g.x - d.w / 2.0); bmy = roundi(g.y - d.h / 2.0); return; }
  for (auto& b : S.b) if (g.x >= b.x && g.x < b.x + b.w && g.y >= b.y && g.y < b.y + b.h) { selId = b.id; return; }
  for (auto& n : S.nodes) if (n.amt > 0 && g.x >= n.x && g.x < n.x + 1.2f && g.y >= n.y && g.y < n.y + 1.2f) {
    selId = -1; toast(std::string(n.type == N_TREE ? "Tree" : n.type == N_GOLD ? "Gold" : "Stone") + ": " + std::to_string((int)n.amt) + " left"); return;
  }
  selId = -1;
}

// ============================================================ main
static void boot() {
  if (!loadGame()) newGame();
  centerCamera();
  double away = std::max(0.0, std::min(28800.0, (double)((long long)std::time(nullptr) - S.lastSeen)));
  if (away > 10) {
    float before[NRES]; for (int i = 0; i < NRES; i++) before[i] = S.res[i];
    for (double left = away; left > 0; left -= 10) { float dt = (float)std::min(10.0, left); S.t += dt; econTick(dt); }
    welcome.away = away; for (int i = 0; i < NRES; i++) welcome.gain.v[i] = (int)(S.res[i] - before[i]);
    welcome.raided = false;
    if (tcLvl() >= 2 && S.nextRaid > 0 && S.t >= S.nextRaid && S.t >= S.shieldUntil) { welcome.rep = headlessRaid(); welcome.raided = true; }
    modal = M_WELCOME;
  } else if (!S.seenIntro) modal = M_INTRO;
}

// ============================================================ frame update / render (shared by the game loop and --smoke)
static const char* shotPath = nullptr;
static float saveT = 0;
static float shakeAmp = 0;
static float frand() { return (float)std::rand() / (float)RAND_MAX; }
static void spawnEventFx(const SimEvent& e) {
  switch (e.type) {
    case EV_HIT: fxSpawn(FX_SPARK, e.x, e.y, 22, 3); break;
    case EV_ARROW: fxSpawn(FX_DUST, e.x, e.y, 10, 1); break;
    case EV_BOLT: fxSpawn(FX_SPARK, e.x, e.y, 24, 5); fxSpawn(FX_RING, e.x, e.y, 10, 1); break;
    case EV_CANNON: fxSpawn(FX_RING, e.x, e.y, 6, 1); fxSpawn(FX_FLAME, e.x, e.y, 14, 4); fxSpawn(FX_SMOKE, e.x, e.y, 14, 4); fxSpawn(FX_DEBRIS, e.x, e.y, 16, 5); fxSpawn(FX_DUST, e.x, e.y, 4, 5); shakeAmp = std::max(shakeAmp, 2.5f); break;
    case EV_COLLAPSE: fxSpawn(FX_DUST, e.x, e.y, 10, 18); fxSpawn(FX_DEBRIS, e.x, e.y, 20, 12); fxSpawn(FX_SMOKE, e.x, e.y, 22, 8); fxSpawn(FX_RING, e.x, e.y, 6, 1); shakeAmp = std::max(shakeAmp, 5.f); break;
    case EV_TRAP: fxSpawn(FX_SPARK, e.x, e.y, 10, 10); fxSpawn(FX_FLAME, e.x, e.y, 6, 4); break;
    case EV_HEAL: fxSpawn(FX_HEAL, e.x, e.y, 14, 4); break;
    case EV_DEPLOY: fxSpawn(FX_DUST, e.x, e.y, 3, 5); break;
    case EV_DEATH: fxSpawn(FX_DUST, e.x, e.y, 4, 6); break;
  }
}
static void updateWorld(float dt) {
  frameDt = dt;
  if (toastT > 0) toastT -= dt;
  if (bannerT > 0) bannerT -= dt;
  { static Modal pm = M_NONE; if (modal != pm) { if (modal != M_NONE) { modalAnim = 0; modalAge = 0; } pm = modal; } if (modal != M_NONE) { modalAnim = std::min(1.f, modalAnim + dt * 7.f); modalAge += dt; } }
  { static View pv = V_HOME; if (view != pv) { fxClear(); pv = view; } }
  for (int i = 0; i < NRES; i++) { float d = S.res[i] - dispRes[i]; dispRes[i] += d * std::min(1.f, dt * 7.f); if (std::fabs(d) < 0.7f) dispRes[i] = S.res[i]; }
  if (std::fabs(cam.zt - cam.zoom) > 0.0008f) { float nz = cam.zoom + (cam.zt - cam.zoom) * std::min(1.f, dt * 11.f); zoomAt(cam.zax, cam.zay, nz); }
  if (!in.down && (std::fabs(cam.vx) > 4 || std::fabs(cam.vy) > 4)) { cam.x += cam.vx * dt; cam.y += cam.vy * dt; float k = std::exp(-dt * 4.2f); cam.vx *= k; cam.vy *= k; } else if (!in.down) cam.vx = cam.vy = 0;
  clampCam();
  fxUpdate(dt); if (shakeAmp > 0) shakeAmp = std::max(0.f, shakeAmp - dt * 14.f);
  { static Track cur = MUS_NONE; Track want = view == V_BATTLE ? MUS_BATTLE : MUS_VILLAGE; if (want != cur) { cur = want; audioMusic(want); } }
  if (view == V_HOME) {
    S.t += dt * speedMul; econTick(dt * speedMul); raidCheck();
    if (modal == M_NONE && !incoming) { SeasonResult sr; if (seasonCheck(sr)) { seasonRes = sr; modal = M_SEASON; audioPlay(SFX_FANFARE); saveGame(); } }
    saveT += dt; if (saveT > 5) { saveT = 0; saveGame(); }
    if (buildMode) bmOk = placeOk(bmType, bmx, bmy);
    {   // ambient life: chimney smoke, camp fires, construction dust
      static const struct { BType t; float ox, oy, z, rate; int kind; int maxLvl; } CH[] = {{B_HOUSE, -0.42f, -0.45f, 62, 1.1f, FX_SMOKE, 99}, {B_TOWNHALL, -0.5f, -0.5f, 100, 1.3f, FX_SMOKE, 3}, {B_BARRACKS, -0.8f, -0.45f, 84, 1.0f, FX_SMOKE, 99},
                                                                                       {B_BLACKSMITH, -0.45f, -0.3f, 92, 1.8f, FX_SMOKE, 99}, {B_ARMYCAMP, 0.2f, 0.05f, 12, 7.f, FX_FLAME, 99}};
      float sp = speedMul > 1 ? 0.35f : 1.f;
      for (auto& b : S.b) {
        float cx = b.x + b.w / 2.f, cy = b.y + b.h / 2.f;
        if (b.building) { if (frand() < dt * 5.f * sp) fxSpawn(FX_DUST, b.x + frand() * b.w, b.y + frand() * b.h, 6, 1); continue; }
        if (!isDone(b)) continue;
        for (auto& c : CH) if (b.type == c.t && b.lvl <= c.maxLvl && frand() < dt * c.rate * sp) fxSpawn(c.kind, cx + c.ox, cy + c.oy, c.z, 1);
      }
    }
  } else if (!B.done) {
    B.sim.step(dt * (B.kind == K_DEFENCE ? std::min(speedMul, 5.f) : 1.f));
    for (auto& ev : B.sim.events) {
      static const Sfx map[EV_COUNT] = {SFX_HIT, SFX_ARROW, SFX_BOLT, SFX_CANNON, SFX_COLLAPSE, SFX_TRAP, SFX_HEAL, SFX_DEPLOY, SFX_DEPLOY};
      if (ev.type != EV_DEATH) audioPlay(map[ev.type]);
      spawnEventFx(ev);
    }
    B.sim.events.clear();
    for (auto& u : B.sim.units) if (u.moving && frand() < dt * (u.type == U_KNIGHT || u.type == U_PALADIN ? 7.f : 3.f)) fxSpawn(FX_DUST, u.x, u.y, 1, 1);
    for (auto& b : B.sim.b) if (b.dead && !b.wall && !b.trap && b.deadT > 0.7f) {
      float cx = b.x + b.w / 2.f, cy = b.y + b.h / 2.f;
      if (frand() < dt * 5.f) fxSpawn(FX_FLAME, cx + (frand() - .5f) * b.w * 0.7f, cy + (frand() - .5f) * b.h * 0.7f, 4, 1);
      if (frand() < dt * 2.5f) fxSpawn(FX_SMOKE, cx + (frand() - .5f) * b.w * 0.5f, cy + (frand() - .5f) * b.h * 0.5f, 10, 1);
    }
    for (auto& p : B.sim.proj) if (p.col == 0xffa040 && frand() < dt * 18.f) { float k = 1 - p.life / p.max; fxSpawn(FX_SMOKE, p.x0 + (p.x1 - p.x0) * k, p.y0 + (p.y1 - p.y0) * k, 18 + std::sin(k * 3.14159f) * 40, 1); }
    if (B.sim.over) { if (B.kind == K_DEFENCE) finishDefence(); else finishAttack(); }
  }
}
static void renderFrame() {
  G.begin(0x14202e);
  float shx = shakeAmp > 0 ? (frand() - .5f) * shakeAmp * 2 : 0, shy = shakeAmp > 0 ? (frand() - .5f) * shakeAmp * 2 : 0; cam.x += shx; cam.y += shy;
  if (view == V_HOME) drawHome(); else drawBattle();
  cam.x -= shx; cam.y -= shy;
  blocks.clear();
  clickGate = modal == M_NONE; drawHud(); drawPanel();
  clickGate = true; drawModal();
  if (in.released && !in.dragged && !in.uiGesture && modal == M_NONE && !inBlocks(blocks, in.mx, in.my)) onTap(in.mx, in.my);
  prevBlocks = blocks;
  if (shotPath) {
    SDL_Surface* sf = SDL_CreateRGBSurfaceWithFormat(0, G.W, G.H, 32, SDL_PIXELFORMAT_ARGB8888);
    if (sf) { SDL_RenderReadPixels(G.r, nullptr, SDL_PIXELFORMAT_ARGB8888, sf->pixels, sf->pitch); SDL_SaveBMP(sf, shotPath); SDL_FreeSurface(sf); }
    shotPath = nullptr;
  }
  G.present();
  if (view == V_HOME && selId >= 0 && !findB(selId)) selId = -1;
}

// ============================================================ --smoke : scripted self-test (writes smoke.log + smoke_*.bmp, never touches the real save)
static FILE* smokeLog = nullptr;
static void LOG(const char* fmt, ...) { va_list a; va_start(a, fmt); if (smokeLog) { vfprintf(smokeLog, fmt, a); fputc('\n', smokeLog); fflush(smokeLog); } va_end(a); }
static bool nearSpot(BType t, int cx, int cy, int& ox, int& oy) {
  for (int r = 0; r < 14; r++) for (int dx = -r; dx <= r; dx++) for (int dy = -r; dy <= r; dy++)
    if (std::max(std::abs(dx), std::abs(dy)) == r && placeOk(t, cx + dx, cy + dy)) { ox = cx + dx; oy = cy + dy; return true; }
  return false;
}
static bool smokePlace(BType t, int cx, int cy) {
  const BDef& d = bdef(t); int x, y;
  if (!nearSpot(t, cx, cy, x, y)) { LOG("FAIL no spot for %s", d.name); return false; }
  if (!canAfford(d.cost) || idleV() < 1) { LOG("FAIL cannot afford / no villager for %s (idle=%d)", d.name, idleV()); return false; }
  pay(d.cost); Bld& b = addB(t, x, y, 0); b.building = true; b.bStart = (float)S.t; b.bEnd = (float)S.t + d.time; b.bTo = 1;
  LOG("placed %s at %d,%d", d.name, x, y); return true;
}
static bool smokePlaceAt(BType t, int x, int y) {
  const BDef& d = bdef(t);
  if (!placeOk(t, x, y) || !canAfford(d.cost) || idleV() < 1) return false;
  pay(d.cost); Bld& b = addB(t, x, y, 0); b.building = true; b.bStart = (float)S.t; b.bEnd = (float)S.t + d.time; b.bTo = 1; return true;
}
static int smoke() {
  smokeLog = std::fopen("smoke.log", "w"); g_savePath = "smoke_save.txt"; newGame(); centerCamera(); modal = M_NONE; S.seenIntro = true;
  static std::string keep;
  auto frames = [&](int n, float dt = 0.05f) { for (int i = 0; i < n; i++) { updateWorld(dt); renderFrame(); } };
  auto shot = [&](const char* name) { keep = std::string("smoke_") + name + ".bmp"; shotPath = keep.c_str(); renderFrame(); LOG("shot %s", name); };
  int fails = 0;
  auto CHECK = [&](bool ok, const char* what) { LOG("%s: %s", ok ? "ok  " : "FAIL", what); if (!ok) fails++; };
  speedMul = 20;
  CHECK(smokePlace(B_LUMBER, 11, 6), "place lumber camp"); CHECK(smokePlace(B_FARM, 18, 17), "place farm"); CHECK(smokePlace(B_HOUSE, 18, 9), "place house");
  auto focus = [&](float gx, float gy, float zm) { cam.zoom = cam.zt = zm; cam.x = G.W / 2.f - (gx - gy) * TW / 2 * zm; cam.y = G.H / 2.f - (gx + gy) * TH / 2 * zm; cam.vx = cam.vy = 0; };
  frames(60); shot("1_economy");
  focus(11, 8, 2.8f); frames(30); shot("24_closeup_home"); frames(7); shot("24b_closeup_home"); centerCamera();
  CHECK(idleV() == 3 - assigned(), "villager accounting"); LOG("wood=%.0f food=%.0f", S.res[WOOD], S.res[FOOD]);
  for (int i = 0; i < 200 && S.res[WOOD] < 260; i++) frames(5);
  S.res[STONE] = 100; CHECK(smokePlace(B_BARRACKS, 18, 21), "place barracks"); frames(80);
  Bld* br = nullptr; for (auto& b : S.b) if (b.type == B_BARRACKS) br = &b;
  CHECK(br && isDone(*br), "barracks completed"); S.res[FOOD] = 600; S.res[GOLD] = 400;
  if (br) for (int i = 0; i < 6; i++) trainUnit(*br, U_MILITIA);
  frames(300); CHECK(S.army[U_MILITIA] == 5, "militia trained up to the housing cap (5)"); LOG("army=%d", armyCount());
  startBarb(1); shot("2_scout"); beginBattle(); int army0 = armyCount();
  float spots[5][2] = {{3, 3}, {3.5f, 3}, {3, 3.5f}, {3.5f, 3.5f}, {4, 3}};
  for (auto& sp : spots) { B.sim.deploy(B.sel, sp[0], sp[1]); S.army[B.sel]--; }
  frames(100); shot("3_battle");
  for (int i = 0; i < 4000 && view == V_BATTLE && modal != M_ATK_RESULT; i++) frames(1);
  frames(14); shot("4_result"); CHECK(modal == M_ATK_RESULT, "attack finished"); CHECK(atk.win, "barbarian camp 1 beaten");
  LOG("stars=%d pct=%d loot wood=%d renown=%d army0=%d", atk.stars, atk.pct, atk.got.v[0], S.renown, army0); CHECK(S.renown > 0, "renown increased"); goHome();
  findRival(true); shot("5_rival"); CHECK(modal == M_SCOUT && scout.rival, "rival scouted"); modal = M_NONE;
  tcB().lvl = 2; S.nextRaid = S.t; S.shieldUntil = 0; speedMul = 5;
  for (int i = 0; i < 400 && view == V_HOME; i++) frames(1);
  CHECK(view == V_BATTLE && B.kind == K_DEFENCE, "NPC raid started"); frames(60); shot("6_defence");
  for (int i = 0; i < 6000 && modal != M_DEF_RESULT; i++) frames(1);
  shot("7_defence_result"); CHECK(modal == M_DEF_RESULT, "NPC raid resolved"); goHome();
  // ---------------- Castle Age scenario
  S.army[U_MILITIA] = 0; speedMul = 20; tcB().lvl = 6; S.villagers = 24; S.nextRaid = S.t + 1e9; S.shieldUntil = S.t + 1e9;
  for (int i = 0; i < NRES; i++) S.res[i] = (float)cap();
  CHECK(bmaxn(B_CASTLE, tcLvl()) == 1 && bmaxn(B_UNIVERSITY, tcLvl()) == 0 && bmaxn(B_UNIVERSITY, 7) == 1, "castle-age building limits by TC level");
  CHECK(std::string(ageName(tcLvl())) == "Castle Age", "TC level 6 is Castle Age");
  bool placed = true;
  for (BType t : {B_STABLE, B_SIEGE, B_MONASTERY, B_BLACKSMITH, B_MARKET, B_KEEP, B_CASTLE, B_ARMYCAMP, B_ARMYCAMP, B_ARMYCAMP, B_BARRACKS, B_ARCHERY}) placed &= smokePlace(t, 15, 21);
  CHECK(placed, "placed all Castle Age buildings"); frames(160);
  Bld *stable = nullptr, *siege = nullptr, *monk = nullptr, *smith = nullptr, *archery = nullptr, *castle = nullptr;
  for (auto& b : S.b) { if (b.type == B_STABLE) stable = &b; if (b.type == B_SIEGE) siege = &b; if (b.type == B_MONASTERY) monk = &b; if (b.type == B_BLACKSMITH) smith = &b; if (b.type == B_ARCHERY) archery = &b; if (b.type == B_CASTLE) castle = &b; }
  CHECK(stable && siege && monk && smith && archery && castle && isDone(*stable) && isDone(*siege) && isDone(*monk) && isDone(*smith) && isDone(*castle), "Castle Age buildings completed");
  if (smith) { startResearch(T_FORGING); frames(120); CHECK(S.tech[T_FORGING] == 1, "Forging researched at the Blacksmith"); LOG("melee mul %.2f", techMul().melee); }
  { float g0 = S.res[GOLD]; trade(0); CHECK(S.res[GOLD] >= g0 || S.res[GOLD] == (float)cap(), "market trade works"); }
  for (int i = 0; i < NRES; i++) S.res[i] = (float)cap();
  if (stable) { trainUnit(*stable, U_KNIGHT); trainUnit(*stable, U_KNIGHT); }
  if (siege) { trainUnit(*siege, U_RAM); trainUnit(*siege, U_MANGONEL); }
  if (monk) trainUnit(*monk, U_MONK);
  if (archery) trainUnit(*archery, U_CROSSBOW);
  for (auto& b : S.b) if (b.type == B_BARRACKS) { trainUnit(b, U_LONGSWORD); trainUnit(b, U_LONGSWORD); trainUnit(b, U_LONGSWORD); break; }
  frames(260);
  LOG("army: kn=%d ram=%d mg=%d monk=%d xb=%d ls=%d", S.army[U_KNIGHT], S.army[U_RAM], S.army[U_MANGONEL], S.army[U_MONK], S.army[U_CROSSBOW], S.army[U_LONGSWORD]);
  CHECK(S.army[U_KNIGHT] == 2 && S.army[U_RAM] == 1 && S.army[U_MANGONEL] == 1 && S.army[U_MONK] == 1 && S.army[U_CROSSBOW] == 1 && S.army[U_LONGSWORD] == 3, "Castle Age units trained");
  shot("10_castle_village");
  findRival(true); CHECK(scout.rival && scout.tc >= 5, "rival is Castle Age"); int rivalBuildings = (int)scout.base.buildings.size();
  bool hasCastleOrKeep = false; for (auto& sb : scout.base.buildings) if (sb.type == B_KEEP || sb.type == B_CASTLE) hasCastleOrKeep = true;
  CHECK(hasCastleOrKeep, "rival base has keeps / castle"); LOG("rival tc=%d buildings=%d", scout.tc, rivalBuildings);
  beginBattle();
  {
    float px = 2.5f, py = 2.5f; int n = 0;
    for (int ty = 0; ty < U_COUNT; ty++) while (S.army[ty] > 0) { B.sim.deploy((UType)ty, px + 0.5f * (n % 4), py + 0.5f * (n / 4), false); S.army[ty]--; n++; }
    LOG("deployed %d units against %d buildings (garrison units: %d)", n, rivalBuildings, (int)std::count_if(B.sim.units.begin(), B.sim.units.end(), [](const SimUnit& u) { return u.def; }));
  }
  frames(300); shot("11_castle_battle");
  { float sx = 0, sy = 0; int n = 0; for (auto& u : B.sim.units) if (!u.def) { sx += u.x; sy += u.y; n++; } if (n) { focus(sx / n, sy / n, 2.8f); frames(4); shot("25_closeup_battle"); frames(12); shot("25b_closeup_battle"); centerCamera(); } }
  for (int i = 0; i < 6000 && view == V_BATTLE && modal != M_ATK_RESULT; i++) frames(1);
  CHECK(modal == M_ATK_RESULT, "castle-age raid finished"); LOG("castle raid: stars=%d pct=%d renown=%d", atk.stars, atk.pct, S.renown); frames(14); shot("12_castle_result"); goHome();
  // NPC raid with a castle garrison defending the village
  S.nextRaid = S.t; S.shieldUntil = 0; speedMul = 5; S.army[U_MILITIA] = 0;
  for (int i = 0; i < 600 && view == V_HOME; i++) frames(1);
  CHECK(view == V_BATTLE && B.kind == K_DEFENCE, "NPC raid started on a Castle Age village");
  { int garrison = (int)std::count_if(B.sim.units.begin(), B.sim.units.end(), [](const SimUnit& u) { return u.def; }); CHECK(garrison >= 3, "castle garrison present"); LOG("garrison units: %d", garrison); }
  frames(120); shot("13_castle_defence");
  for (int i = 0; i < 8000 && modal != M_DEF_RESULT; i++) frames(1);
  CHECK(modal == M_DEF_RESULT, "castle-age NPC raid resolved"); LOG("defence: stars=%d pct=%d", defRep.stars, defRep.pct); goHome();
  // ---------------- Walls, gates and traps
  speedMul = 20; S.nextRaid = S.t + 1e9; S.shieldUntil = S.t + 1e9; for (int i = 0; i < NRES; i++) S.res[i] = (float)cap();
  {
    // A. standalone pathing tests on a tiny base (chieftain hall inside a palisade ring)
    auto ringSim = [&](int gapX, int gapY, bool trap, bool oil) {
      Sim sm; sm.b.push_back(makeSimB(B_CHIEF, 1, 14, 14));
      for (int a = 9; a <= 19; a++) for (int e = 0; e < 4; e++) {
        int x = e == 0 ? a : e == 1 ? a : e == 2 ? 9 : 19, y = e == 0 ? 9 : e == 1 ? 19 : a;
        if ((e == 2 || e == 3) && (a == 9 || a == 19)) continue;
        if (x == gapX && y == gapY) continue;
        sm.b.push_back(makeSimB(B_WALL, 1, x, y));
      }
      if (trap) sm.b.push_back(makeSimB(B_SPIKE, 2, 7, 14));
      if (oil) sm.b.push_back(makeSimB(B_OIL, 2, 7, 14));
      sm.reserve = [] { return 0; };
      return sm;
    };
    auto run = [&](Sim& sm, int units, float sy) { for (int i = 0; i < units; i++) sm.deploy(U_MILITIA, 3.0f, sy + 0.4f * i, false); for (int i = 0; i < 60 * 40 && !sm.over; i++) sm.step(1.0f / 20); };
    auto deadWalls = [](const Sim& sm) { int n = 0; for (auto& q : sm.b) if (q.wall && q.dead) n++; return n; };
    { Sim sm = ringSim(-1, -1, false, false); run(sm, 4, 13.0f); CHECK(sm.hqDead(), "raiders breach a closed wall ring and destroy the hall"); CHECK(deadWalls(sm) >= 1, "walls were damaged while breaching"); LOG("closed ring: walls destroyed=%d time=%.0fs", deadWalls(sm), sm.t); }
    { Sim sm = ringSim(9, 14, false, false); run(sm, 4, 13.5f); CHECK(sm.hqDead(), "raiders walk through a gap in the wall"); CHECK(deadWalls(sm) == 0, "no walls attacked when a gap exists"); LOG("gap ring: walls destroyed=%d time=%.0fs", deadWalls(sm), sm.t); }
    { Sim sm = ringSim(9, 14, true, false); float hp0 = 0; run(sm, 3, 13.8f); bool fired = false; for (auto& q : sm.b) if (q.type == B_SPIKE && q.dead) fired = true; CHECK(fired, "spike trap triggers on contact"); (void)hp0; }
    { Sim sm = ringSim(9, 14, false, true); float t0 = 0; run(sm, 2, 13.8f); bool burned = false; for (auto& q : sm.b) if (q.type == B_OIL && (q.dead || q.aux > 0)) burned = true; CHECK(burned, "oil trap triggers"); (void)t0; }
    { Sim sm = ringSim(-1, -1, false, false); int cnt = sm.pct(); CHECK(cnt == 0 && sm.alive() == 1, "walls and traps do not count towards destruction %"); }
    // trap damage really hurts and slows
    { Sim sm = ringSim(9, 14, true, false); sm.deploy(U_MILITIA, 7.0f, 14.0f, false); float hp = sm.units[0].hp; for (int i = 0; i < 4; i++) sm.step(0.05f); CHECK(sm.units.empty() || sm.units[0].hp < hp - 50, "spike trap deals heavy damage"); }
    { Sim sm = ringSim(9, 14, false, true); sm.deploy(U_MILITIA, 7.0f, 14.0f, false); sm.step(0.05f); sm.step(0.05f); bool slowed = !sm.units.empty() && sm.units[0].slow > 0; CHECK(slowed, "oil trap slows raiders"); }
  }
  {
    // B. building walls in the village
    int placedW = 0, placedG = 0;
    for (int a = 4; a <= 26; a++) { for (int e = 0; e < 2; e++) { int x = a, y = e ? 26 : 4; if (placePiece(B_WALL, x, y)) placedW++; } for (int e = 0; e < 2; e++) { int x = e ? 26 : 4, y = a; if (a != 4 && a != 26 && placePiece(B_WALL, x, y)) placedW++; } }
    CHECK(placedW > 60, "painted a wall ring around the village"); LOG("wall pieces placed=%d (limit %d)", placedW, bmaxn(B_WALL, tcLvl()));
    CHECK(placePiece(B_GATE, 26, 15) && placePiece(B_GATE, 15, 4), "gates replace wall pieces"); CHECK(buildCount(B_GATE) == 2, "two gates placed");
    bool tr = true; { int tx, ty; for (BType tt : {B_SPIKE, B_SPIKE, B_OIL}) { if (nearSpot(tt, 12, 23, tx, ty)) tr &= placePiece(tt, tx, ty); else tr = false; } } CHECK(tr, "spike and oil traps placed");
    CHECK(!placePiece(B_WALL, 4, 4), "cannot stack a wall on a wall"); CHECK(S.wallLvl == 1, "walls start at level 1");
    int before = S.wallLvl; upgradeWalls(); CHECK(S.wallLvl == before + 1, "upgrade all walls"); int tl = S.trapLvl; upgradeTraps(); CHECK(S.trapLvl == tl + 1, "upgrade all traps");
    int id = -1; for (auto& b : S.b) if (b.type == B_SPIKE) { id = b.id; break; } int nb = buildCount(B_SPIKE); removePiece(id); CHECK(buildCount(B_SPIKE) == nb - 1, "remove a trap");
    speedMul = 1; frames(10); shot("17_walled_village");
  }
  // NPC raid against the walled village
  S.nextRaid = S.t; S.shieldUntil = 0; speedMul = 5;
  for (int i = 0; i < 600 && view == V_HOME; i++) frames(1);
  CHECK(view == V_BATTLE && B.kind == K_DEFENCE, "NPC raid on a walled village");
  frames(200); shot("18_wall_defence");
  for (int i = 0; i < 9000 && modal != M_DEF_RESULT; i++) frames(1);
  CHECK(modal == M_DEF_RESULT, "walled-village raid resolved"); LOG("walled defence: stars=%d pct=%d", defRep.stars, defRep.pct); goHome();
  // rival bases get walls, gates and traps
  { bool wall = false, gate = false, trap = false; int tries = 0;
    for (; tries < 40 && !(wall && gate); tries++) { findRival(true); wall = gate = trap = false; for (auto& sb : scout.base.buildings) { if (sb.type == B_WALL) wall = true; if (sb.type == B_GATE) gate = true; if (sb.trap) trap = true; } }
    CHECK(wall && gate, "rival bases have walls and gates"); LOG("rival with walls found after %d searches (tc=%d, traps=%d)", tries, scout.tc, (int)trap); modal = M_NONE; shot("19_walled_rival_scout"); }
  { buildMode = true; bmPaint = true; paintOn = true; bmType = B_WALL; bmx = 10; bmy = 8; S.res[WOOD] = 5000; frames(2); shot("20_paint_mode"); buildMode = false; bmPaint = false; }
  // ---------------- Imperial Age scenario
  speedMul = 20; tcB().lvl = 9; S.nextRaid = S.t + 1e9; S.shieldUntil = S.t + 1e9;
  for (int i = 0; i < NRES; i++) S.res[i] = (float)cap();
  S.villagers = 30;
  CHECK(std::string(ageName(tcLvl())) == "Imperial Age", "TC level 9 is Imperial Age");
  CHECK(bmaxn(B_BOMBARD, 9) == 2 && bmaxn(B_BOMBARD, 7) == 0, "bombard tower limits by TC level");
  CHECK(tcup(10).cost.v[WOOD] > tcup(9).cost.v[WOOD] && tcup(8).label != nullptr, "TC 8-10 upgrade table");
  { Bld& tc = tcB(); UpInfo u = upgradeInfo(tc); CHECK(!u.err && u.cost.v[WOOD] == tcup(10).cost.v[WOOD], "TC 9 -> 10 upgrade offered"); }
  bool placedI = true;
  for (BType t : {B_BOMBARD, B_BOMBARD, B_UNIVERSITY, B_ARCHERY, B_STABLE, B_SIEGE, B_BARRACKS, B_ARMYCAMP, B_ARMYCAMP, B_ARMYCAMP, B_ARMYCAMP}) placedI &= smokePlace(t, 15, 22);
  CHECK(placedI, "placed Imperial Age buildings"); frames(220);
  { Bld *uni = nullptr, *arch = nullptr, *stb = nullptr, *sg = nullptr, *bar = nullptr;
    for (auto& b : S.b) { if (b.type == B_UNIVERSITY) uni = &b; if (b.type == B_ARCHERY) arch = &b; if (b.type == B_STABLE) stb = &b; if (b.type == B_SIEGE) sg = &b; if (b.type == B_BARRACKS) bar = &b; }
    CHECK(uni && isDone(*uni), "University built");
    if (uni) { startResearch(T_CHEMISTRY); frames(200); CHECK(S.tech[T_CHEMISTRY] == 1, "Chemistry researched"); LOG("ranged mul %.2f siege mul %.2f", techMul().ranged, techMul().siege); }
    for (int i = 0; i < NRES; i++) S.res[i] = (float)cap();
    for (int i = 0; i < U_COUNT; i++) S.army[i] = 0;
    if (bar) { trainUnit(*bar, U_CHAMPION); trainUnit(*bar, U_CHAMPION); }
    if (arch) { trainUnit(*arch, U_ARBALEST); trainUnit(*arch, U_CANNONEER); }
    if (stb) trainUnit(*stb, U_PALADIN);
    if (sg) { trainUnit(*sg, U_TREBUCHET); trainUnit(*sg, U_BOMBARD); }
    frames(300);
    CHECK(S.army[U_CHAMPION] == 2 && S.army[U_ARBALEST] == 1 && S.army[U_CANNONEER] == 1 && S.army[U_PALADIN] == 1 && S.army[U_TREBUCHET] == 1 && S.army[U_BOMBARD] == 1, "Imperial Age units trained");
    LOG("imperial army: ch=%d ab=%d hc=%d pl=%d tb=%d bc=%d", S.army[U_CHAMPION], S.army[U_ARBALEST], S.army[U_CANNONEER], S.army[U_PALADIN], S.army[U_TREBUCHET], S.army[U_BOMBARD]); }
  shot("14_imperial_village");
  findRival(true); CHECK(scout.rival && scout.tc >= 8, "rival is Imperial Age"); bool hasBomb = false; for (auto& sb : scout.base.buildings) if (sb.type == B_BOMBARD) hasBomb = true;
  CHECK(hasBomb, "Imperial rival has bombard towers"); LOG("imperial rival tc=%d buildings=%d", scout.tc, (int)scout.base.buildings.size());
  beginBattle();
  { float px = 2.5f, py = 2.5f; int n = 0; for (int ty = 0; ty < U_COUNT; ty++) while (S.army[ty] > 0) { B.sim.deploy((UType)ty, px + 0.5f * (n % 4), py + 0.5f * (n / 4), false); S.army[ty]--; n++; } LOG("deployed %d imperial units", n); }
  frames(400); shot("15_imperial_battle");
  for (int i = 0; i < 8000 && view == V_BATTLE && modal != M_ATK_RESULT; i++) frames(1);
  CHECK(modal == M_ATK_RESULT, "imperial raid finished"); LOG("imperial raid: stars=%d pct=%d", atk.stars, atk.pct); goHome();
  S.nextRaid = S.t; S.shieldUntil = 0; speedMul = 5;
  for (int i = 0; i < 600 && view == V_HOME; i++) frames(1);
  CHECK(view == V_BATTLE && B.kind == K_DEFENCE, "NPC raid on an Imperial Age village");
  { bool treb = false; for (auto& sq : B.sim.sq) if (sq.type == U_TREBUCHET || sq.type == U_PALADIN || sq.type == U_CHAMPION) treb = true; CHECK(treb, "imperial raiders (champions/paladins/trebuchets)"); }
  frames(150); shot("16_imperial_defence");
  for (int i = 0; i < 9000 && modal != M_DEF_RESULT; i++) frames(1);
  CHECK(modal == M_DEF_RESULT, "imperial NPC raid resolved"); LOG("imperial defence: stars=%d pct=%d", defRep.stars, defRep.pct); goHome();
  // ---------------- Scorpion, seasons, audio
  {
    CHECK(bmaxn(B_SCORPION, 6) == 0 && bmaxn(B_SCORPION, 7) == 1 && bmaxn(B_SCORPION, 10) == 4, "scorpion limits by TC level");
    Sim sm; sm.b.push_back(makeSimB(B_SCORPION, 1, 10, 10)); sm.b.push_back(makeSimB(B_CHIEF, 1, 24, 24)); sm.reserve = [] { return 0; };
    for (int i = 0; i < 4; i++) sm.deploy(U_MILITIA, 3.0f + 1.0f * i, 11.0f, false);       // four raiders lined up with the scorpion
    float hp0 = sm.units[0].hp; sm.step(0.05f); int hurt = 0; for (auto& u : sm.units) if (u.hp < u.maxhp) hurt++;
    CHECK(hurt >= 3, "one scorpion bolt pierces several raiders in a line"); LOG("scorpion bolt hit %d of 4 raiders (hp %.0f -> %.0f)", hurt, hp0, sm.units[0].hp);
    bool bolt = false; for (auto& ev : sm.events) if (ev.type == EV_BOLT) bolt = true; CHECK(bolt, "scorpion raises a bolt event for audio");
    Sim sp = sm; sp.units.clear(); sp.deploy(U_MILITIA, 3.0f, 3.0f, false); sp.deploy(U_MILITIA, 3.0f, 17.0f, false); sp.step(0.05f);
    int h2 = 0; for (auto& u : sp.units) if (u.hp < u.maxhp) h2++; CHECK(h2 == 0, "a lone raider outside the line is not hit");
    bool scBase = false; for (int i = 0; i < 80 && !scBase; i++) { findRival(true); if (scout.tc >= 7) for (auto& sb : scout.base.buildings) if (sb.type == B_SCORPION) scBase = true; }
    CHECK(scBase, "Castle/Imperial rival bases have scorpions"); modal = M_NONE;
    tcB().lvl = 9; { int sx, sy; bool ok = nearSpot(B_SCORPION, 15, 22, sx, sy); CHECK(ok, "free spot for a scorpion"); if (ok) { CHECK(smokePlaceAt(B_SCORPION, sx, sy), "scorpion placed in the village"); speedMul = 20; frames(120); speedMul = 1; shot("21_scorpion_village"); } }
  }
  {
    int oldSeason = S.season; S.renown = 4000; S.seasonStart = S.t - seasonLen() - 5; SeasonResult sr; int posBefore = ladderPosition();
    bool ended = seasonCheck(sr);
    CHECK(ended && sr.season == oldSeason && S.season == oldSeason + 1, "season ends and the next begins");
    CHECK(S.renown == 3500 && sr.newRenown == 3500, "soft reset compresses Renown above 3000 towards 3000"); CHECK(sr.reward.v[GOLD] > 0, "season chest granted");
    SeasonResult sr2; CHECK(!seasonCheck(sr2), "season does not end twice"); CHECK(seasonLeft() > seasonLen() - 100, "new season timer started");
    LOG("season %d ended: pos before %d, rank %s, reward gold %d, renown 4000 -> %d", oldSeason, posBefore, rankOf(sr.finalRenown), sr.reward.v[GOLD], S.renown);
    seasonRes = sr; modal = M_SEASON; frames(14); shot("22_season_modal"); modal = M_SETTINGS; frames(14); shot("23_options"); modal = M_NONE;
  }
  { std::string rep; int bad = audioSelfTest(rep); LOG("audio device open: %d\n%s", (int)audioDeviceOpen(), rep.c_str()); CHECK(bad == 0, "all synthesised sounds and both music loops are audible and not empty"); }
  // ---------------- unit line-up close-up (animation / team colour check)
  {
    view = V_BATTLE; modal = M_NONE; B.sim = Sim(); B.sim.b.push_back(makeSimB(B_CHIEF, 1, 21, 21)); B.sim.reserve = [] { return 1; }; B.kind = K_RIVAL; B.done = false; scout.tc = 3;
    UType ts[] = {U_MILITIA, U_MANATARMS, U_LONGSWORD, U_ARCHER, U_CROSSBOW, U_CHAMPION, U_KNIGHT, U_PALADIN, U_MONK, U_RAM, U_MANGONEL, U_CANNONEER, U_TREBUCHET, U_BOMBARD, U_ARBALEST};
    for (int i = 0; i < 15; i++) B.sim.deploy(ts[i], 7.0f + (i % 5) * 1.5f, 8.5f + (i / 5) * 1.7f, false);
    for (int i = 0; i < 3; i++) B.sim.deploy(i == 1 ? U_CHAMPION : U_MILITIA, 15.5f, 10.5f + i * 1.6f, true);
    auto focus2 = [&](float gx, float gy, float zm) { cam.zoom = cam.zt = zm; cam.x = G.W / 2.f - (gx - gy) * TW / 2 * zm; cam.y = G.H / 2.f - (gx + gy) * TH / 2 * zm; cam.vx = cam.vy = 0; };
    focus2(11.2f, 11.5f, 2.3f); frames(6); shot("26a_lineup"); frames(18); shot("26b_lineup"); frames(20); shot("26c_lineup");
    goHome();
  }
  modal = M_LADDER; frames(14); shot("8_ladder"); modal = M_BUILD; frames(14); shot("9_build"); modal = M_NONE;
  int nb = (int)S.b.size(), nn = (int)S.nodes.size(), rn = S.renown; float w = S.res[WOOD]; saveGame(); S.b.clear(); S.nodes.clear(); S.renown = -5;
  CHECK(loadGame(), "load succeeded"); CHECK((int)S.b.size() == nb && (int)S.nodes.size() == nn && S.renown == rn && std::fabs(S.res[WOOD] - w) < 1, "save/load roundtrip");
  S.res[WOOD] = 1500; Report r = headlessRaid(); LOG("headless raid: stars=%d pct=%d stolen wood=%d", r.stars, r.pct, r.stolen.v[0]); CHECK(r.pct >= 0, "headless raid resolves");
  LOG("SMOKE %s (%d failures)", fails ? "FAILED" : "PASSED", fails); if (smokeLog) std::fclose(smokeLog); return fails ? 2 : 0;
}

int main(int argc, char** argv) {
  std::srand((unsigned)std::time(nullptr));
  if (!G.init()) return 1;
  loadSprites();
  audioInit();
  for (int i = 1; i + 1 < argc; i++) if (std::string(argv[i]) == "--export-audio") { int n = audioExportWav(argv[i + 1]); std::fprintf(stderr, "exported %d audio files\n", n); audioShutdown(); G.shutdown(); return n > 0 ? 0 : 1; }
  for (int i = 1; i < argc; i++) if (std::string(argv[i]) == "--smoke") { audioSetEnabled(false, false); int rc = smoke(); audioShutdown(); G.shutdown(); return rc; }
  audioSetEnabled(false, false); boot(); audioSetEnabled(S.musicOn, S.sfxOn);
  Uint64 last = SDL_GetPerformanceCounter();
  while (running) {
    in.pressed = in.released = false; in.wheel = 0; in.lastx = in.mx; in.lasty = in.my;
    SDL_Event e; int winW = 1, winH = 1; SDL_GetWindowSize(G.win, &winW, &winH); float sxk = (float)G.W / winW, syk = (float)G.H / winH;
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_QUIT: running = false; break;
        case SDL_APP_WILLENTERBACKGROUND: case SDL_APP_TERMINATING: saveGame(); break;
        case SDL_MOUSEBUTTONDOWN:
          if (e.button.button == SDL_BUTTON_LEFT) { in.mx = e.button.x * sxk; in.my = e.button.y * syk; in.px = in.mx; in.py = in.my; in.down = true; in.pressed = true; in.dragged = false; in.uiGesture = inBlocks(prevBlocks, in.mx, in.my); in.lastx = in.mx; in.lasty = in.my; cam.vx = cam.vy = 0;
            if (buildMode && bmPaint && paintOn && !in.uiGesture && modal == M_NONE && view == V_HOME) paintAt(in.mx, in.my); }
          break;
        case SDL_MOUSEBUTTONUP:
          if (e.button.button == SDL_BUTTON_LEFT) { in.mx = e.button.x * sxk; in.my = e.button.y * syk; in.down = false; in.released = true; }
          break;
        case SDL_MOUSEMOTION:
          in.mx = e.motion.x * sxk; in.my = e.motion.y * syk;
          if (in.down && std::hypot(in.mx - in.px, in.my - in.py) > U(8)) in.dragged = true;
          if (buildMode && bmPaint && modal == M_NONE) { Pt g = toGrid(in.mx, in.my); bmx = (int)std::floor(g.x); bmy = (int)std::floor(g.y); }
          if (buildMode && bmPaint && paintOn && in.down && !in.uiGesture && modal == M_NONE) paintAt(in.mx, in.my);
          else if (in.down && in.dragged && !in.uiGesture && modal == M_NONE) {
            cam.x += e.motion.xrel * sxk; cam.y += e.motion.yrel * syk;
            static Uint32 lastT = 0; Uint32 nowT = SDL_GetTicks(); float edt = std::max(0.004f, (nowT - lastT) / 1000.f); lastT = nowT;
            cam.vx = cam.vx * 0.55f + 0.45f * (e.motion.xrel * sxk / edt); cam.vy = cam.vy * 0.55f + 0.45f * (e.motion.yrel * syk / edt);
          }
          break;
        case SDL_KEYDOWN:
          // developer shortcuts: F1 = resources + troops, F2 = trigger a raid now, F3 = max Town Center
          if (e.key.keysym.sym == SDLK_F1) { for (int i = 0; i < NRES; i++) S.res[i] = std::min((float)cap(), S.res[i] + 1500); S.army[U_MILITIA] += 6; if (tcLvl() >= 5) { S.army[U_LONGSWORD] += 4; S.army[U_KNIGHT] += 2; S.army[U_MONK] += 1; S.army[U_MANGONEL] += 1; S.army[U_RAM] += 1; }
          if (tcLvl() >= 8) { S.army[U_CHAMPION] += 3; S.army[U_PALADIN] += 2; S.army[U_CANNONEER] += 2; S.army[U_ARBALEST] += 2; S.army[U_TREBUCHET] += 1; } toast("Dev: +resources, +troops"); }
          if (e.key.keysym.sym == SDLK_F3 && view == V_HOME && tcLvl() < MAXTC) { tcB().lvl++; toast("Dev: Town Center level " + std::to_string(tcLvl())); }
          if (e.key.keysym.sym == SDLK_F2 && view == V_HOME) { S.nextRaid = S.t; S.shieldUntil = 0; if (tcLvl() < 2) { tcB().lvl = 2; } }
          if (e.key.keysym.sym == SDLK_F4 && view == V_HOME) { S.seasonStart = S.t - seasonLen() - 1; toast("Dev: ending the season"); }
          if (e.key.keysym.sym == SDLK_ESCAPE) { if (buildMode) buildMode = false; else if (modal != M_NONE && view == V_HOME) modal = M_NONE; else selId = -1; }
          break;
        case SDL_MOUSEWHEEL:
          if (modal != M_NONE) modalScroll -= e.wheel.y * U(40); else zoomSmooth(in.mx, in.my, cam.zt * (e.wheel.y > 0 ? 1.16f : 0.86f));
          break;
        case SDL_MULTIGESTURE:
          if (std::fabs(e.mgesture.dDist) > 0.0005f && modal == M_NONE) { zoomAt(e.mgesture.x * G.W, e.mgesture.y * G.H, cam.zoom * (1 + e.mgesture.dDist * 6)); cam.zt = cam.zoom; cam.vx = cam.vy = 0; in.dragged = true; }
          break;
        case SDL_WINDOWEVENT: if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) { SDL_GetRendererOutputSize(G.r, &G.W, &G.H); } break;
        default: break;
      }
    }
    Uint64 now = SDL_GetPerformanceCounter(); float dt = std::min(0.1f, (float)(now - last) / SDL_GetPerformanceFrequency()); last = now;
    updateWorld(dt); renderFrame();
  }
  saveGame(); audioShutdown(); G.shutdown(); return 0;
}
