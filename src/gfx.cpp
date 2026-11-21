#include "gfx.h"
#include <cstdio>

Gfx G;

uint32_t mixc(uint32_t a, uint32_t b, float t) {
  auto ch = [&](int sh) { float x = (float)((a >> sh) & 255), y = (float)((b >> sh) & 255); return (uint32_t)clampf(x + (y - x) * t, 0, 255); };
  return (ch(16) << 16) | (ch(8) << 8) | ch(0);
}
uint32_t shadec(uint32_t c, float f) { return f < 0 ? mixc(c, 0x000000, -f) : mixc(c, 0xffffff, f); }

static SDL_Color col4(uint32_t c, float a) { return {(Uint8)((c >> 16) & 255), (Uint8)((c >> 8) & 255), (Uint8)(c & 255), (Uint8)(clampf(a, 0, 1) * 255)}; }

bool Gfx::init() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) { std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return false; }
  if (TTF_Init() != 0) { std::fprintf(stderr, "TTF_Init: %s\n", TTF_GetError()); return false; }
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
  SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
  SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");
  SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
  Uint32 flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
#ifdef __ANDROID__
  flags |= SDL_WINDOW_FULLSCREEN;
#endif
  win = SDL_CreateWindow("Dawn of Kings", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, flags);
  if (!win) { std::fprintf(stderr, "window: %s\n", SDL_GetError()); return false; }
  r = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!r) r = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
  if (!r) { std::fprintf(stderr, "renderer: %s\n", SDL_GetError()); return false; }
  SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
  SDL_GetRendererOutputSize(r, &W, &H);
  u = clampf(std::min(W, H) / 560.f, 1.f, 3.f);
  // font: bundled asset first, then system fallbacks
  std::vector<std::string> paths;
#ifdef __ANDROID__
  paths = {"font.ttf", "/system/fonts/Roboto-Regular.ttf", "/system/fonts/DroidSans.ttf"};
#else
  char* bp = SDL_GetBasePath(); std::string base = bp ? bp : ""; if (bp) SDL_free(bp);
  paths = {base + "assets/font.ttf", "assets/font.ttf", "C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/arial.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"};
#endif
  const int sizes[3] = {13, 16, 26};
  for (auto& p : paths) {
    SDL_RWops* rw = SDL_RWFromFile(p.c_str(), "rb"); if (!rw) continue;
    bool ok = true;
    for (int i = 0; i < 3; i++) {
      if (i > 0) rw = SDL_RWFromFile(p.c_str(), "rb");
      fonts[i] = rw ? TTF_OpenFontRW(rw, 1, (int)(sizes[i] * u)) : nullptr;
      if (!fonts[i]) { ok = false; break; }
    }
    if (ok) break;
    for (auto& f : fonts) { if (f) TTF_CloseFont(f); f = nullptr; }
  }
  if (!fonts[0]) { std::fprintf(stderr, "No usable font found\n"); return false; }
  {   // title font (serif); falls back to the large UI font
    std::vector<std::string> tp;
#ifdef __ANDROID__
    tp = {"title.ttf"};
#else
    char* bp2 = SDL_GetBasePath(); std::string base2 = bp2 ? bp2 : ""; if (bp2) SDL_free(bp2);
    tp = {base2 + "assets/title.ttf", "assets/title.ttf"};
#endif
    for (auto& p : tp) { SDL_RWops* rw = SDL_RWFromFile(p.c_str(), "rb"); if (!rw) continue; fonts[3] = TTF_OpenFontRW(rw, 1, (int)(27 * u)); if (fonts[3]) { TTF_SetFontStyle(fonts[3], TTF_STYLE_BOLD); break; } }
    if (!fonts[3]) fonts[3] = fonts[2];
  }
  {   // tileable grain texture for panels
    const int N = 128; std::vector<unsigned char> px(N * N * 4); Rng rn(77); std::vector<float> g(N * N);
    for (auto& v : g) v = (float)rn.next();
    auto at = [&](int x, int y) { return g[((y + N) % N) * N + ((x + N) % N)]; };
    for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
      float s = 0.5f * at(x, y) + 0.25f * (at(x + 1, y) + at(x, y + 1)) * 0.5f + 0.25f * (at(x - 1, y - 1) + at(x + 2, y + 1)) * 0.5f;
      unsigned char a = (unsigned char)(clampf(s, 0, 1) * 255); unsigned char* p = &px[(y * N + x) * 4]; p[0] = p[1] = p[2] = 255; p[3] = a;
    }
    noiseTex = SDL_CreateTexture(r, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, N, N);
    if (noiseTex) { SDL_UpdateTexture(noiseTex, nullptr, px.data(), N * 4); SDL_SetTextureBlendMode(noiseTex, SDL_BLENDMODE_BLEND); }
  }
  return true;
}
void Gfx::shutdown() {
  for (auto& kv : cache) SDL_DestroyTexture(kv.second.t);
  cache.clear();
  if (fonts[3] && fonts[3] != fonts[2]) TTF_CloseFont(fonts[3]);
  for (int i = 0; i < 3; i++) if (fonts[i]) TTF_CloseFont(fonts[i]);
  if (noiseTex) SDL_DestroyTexture(noiseTex);
  if (r) SDL_DestroyRenderer(r); if (win) SDL_DestroyWindow(win); TTF_Quit(); SDL_Quit();
}
void Gfx::begin(uint32_t col) { SDL_GetRendererOutputSize(r, &W, &H); setColor(col); SDL_RenderClear(r); }
void Gfx::setColor(uint32_t c, float a) { SDL_Color k = col4(c, a); SDL_SetRenderDrawColor(r, k.r, k.g, k.b, k.a); }

void Gfx::fillPoly(const Pt* p, int n, uint32_t c, float a) {
  if (n < 3) return;
  std::vector<SDL_Vertex> v(n); SDL_Color k = col4(c, a);
  for (int i = 0; i < n; i++) { v[i].position = {p[i].x, p[i].y}; v[i].color = k; v[i].tex_coord = {0, 0}; }
  std::vector<int> idx; for (int i = 1; i + 1 < n; i++) { idx.push_back(0); idx.push_back(i); idx.push_back(i + 1); }
  SDL_RenderGeometry(r, nullptr, v.data(), n, idx.data(), (int)idx.size());
}
void Gfx::batchTris(const std::vector<SDL_Vertex>& v) { if (!v.empty()) SDL_RenderGeometry(r, nullptr, v.data(), (int)v.size(), nullptr, 0); }
void Gfx::strokePoly(const Pt* p, int n, uint32_t c, float a, bool close) {
  setColor(c, a);
  for (int i = 0; i + 1 < n; i++) SDL_RenderDrawLineF(r, p[i].x, p[i].y, p[i + 1].x, p[i + 1].y);
  if (close && n > 2) SDL_RenderDrawLineF(r, p[n - 1].x, p[n - 1].y, p[0].x, p[0].y);
}
void Gfx::line(float x0, float y0, float x1, float y1, uint32_t c, float a) { setColor(c, a); SDL_RenderDrawLineF(r, x0, y0, x1, y1); }
void Gfx::rect(float x, float y, float w, float h, uint32_t c, float a) { setColor(c, a); SDL_FRect f{x, y, w, h}; SDL_RenderFillRectF(r, &f); }
void Gfx::rectOutline(float x, float y, float w, float h, uint32_t c, float a) { setColor(c, a); SDL_FRect f{x, y, w, h}; SDL_RenderDrawRectF(r, &f); }
void Gfx::rrect(float x, float y, float w, float h, float rad, uint32_t c, float a) {
  rad = std::min(rad, std::min(w, h) / 2); std::vector<Pt> p; const int seg = 5;
  const float cx[4] = {x + w - rad, x + w - rad, x + rad, x + rad}, cy[4] = {y + rad, y + h - rad, y + h - rad, y + rad};
  for (int k = 0; k < 4; k++) for (int i = 0; i <= seg; i++) { float ang = (-90.f + 90.f * k + 90.f * i / seg) * 3.14159265f / 180.f; p.push_back({cx[k] + std::cos(ang) * rad, cy[k] + std::sin(ang) * rad}); }
  fillPoly(p.data(), (int)p.size(), c, a);
}
void Gfx::ellipse(float x, float y, float rx, float ry, uint32_t c, float a) {
  Pt p[16]; for (int i = 0; i < 16; i++) { float an = i * 6.2831853f / 16; p[i] = {x + std::cos(an) * rx, y + std::sin(an) * ry}; }
  fillPoly(p, 16, c, a);
}
void Gfx::circle(float x, float y, float rad, uint32_t c, float a) { ellipse(x, y, rad, rad, c, a); }

Pt Gfx::textSize(const std::string& s, FontSize f) { int w = 0, h = 0; if (!s.empty()) TTF_SizeUTF8(fonts[f], s.c_str(), &w, &h); return {(float)w, (float)h}; }
void Gfx::text(const std::string& s, float x, float y, FontSize f, uint32_t c, Align al, float a) {
  if (s.empty()) return;
  std::string key = std::to_string(f) + ":" + std::to_string(c) + ":" + s;
  auto it = cache.find(key);
  if (it == cache.end()) {
    if (cache.size() > 700) { for (auto& kv : cache) SDL_DestroyTexture(kv.second.t); cache.clear(); }
    SDL_Color k = col4(c, 1); SDL_Surface* sf = TTF_RenderUTF8_Blended(fonts[f], s.c_str(), k); if (!sf) return;
    SDL_Texture* t = SDL_CreateTextureFromSurface(r, sf); Tex tx{t, sf->w, sf->h}; SDL_FreeSurface(sf);
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    it = cache.emplace(key, tx).first;
  }
  Tex& tx = it->second; SDL_SetTextureAlphaMod(tx.t, (Uint8)(clampf(a, 0, 1) * 255));
  float dx = al == A_LEFT ? x : al == A_CENTER ? x - tx.w / 2.f : x - tx.w;
  SDL_FRect d{dx, y, (float)tx.w, (float)tx.h}; SDL_RenderCopyF(r, tx.t, nullptr, &d);
}

// ---------------------------------------------------------------- theme helpers
static SDL_Vertex vtx(float x, float y, uint32_t col, float a) { SDL_Vertex v; v.position = {x, y}; v.color = col4(col, a); v.tex_coord = {0, 0}; return v; }
void Gfx::gradRect(float x, float y, float w, float h, uint32_t top, uint32_t bot, float a) {
  SDL_Vertex v[4] = {vtx(x, y, top, a), vtx(x + w, y, top, a), vtx(x + w, y + h, bot, a), vtx(x, y + h, bot, a)}; int idx[6] = {0, 1, 2, 0, 2, 3};
  SDL_RenderGeometry(r, nullptr, v, 4, idx, 6);
}
void Gfx::gradRRect(float x, float y, float w, float h, float rad, uint32_t top, uint32_t bot, float a) {
  rad = std::min(rad, std::min(w, h) / 2); const int seg = 5; std::vector<SDL_Vertex> v; v.push_back(vtx(x + w / 2, y + h / 2, mixc(top, bot, 0.5f), a));
  const float cx[4] = {x + w - rad, x + w - rad, x + rad, x + rad}, cy[4] = {y + rad, y + h - rad, y + h - rad, y + rad};
  for (int k = 0; k < 4; k++) for (int i = 0; i <= seg; i++) {
    float ang = (-90.f + 90.f * k + 90.f * i / seg) * 3.14159265f / 180.f; float px = cx[k] + std::cos(ang) * rad, py = cy[k] + std::sin(ang) * rad;
    v.push_back(vtx(px, py, mixc(top, bot, clampf((py - y) / std::max(1.f, h), 0, 1)), a));
  }
  std::vector<int> idx; int n = (int)v.size() - 1; for (int i = 0; i < n; i++) { idx.push_back(0); idx.push_back(1 + i); idx.push_back(1 + (i + 1) % n); }
  SDL_RenderGeometry(r, nullptr, v.data(), (int)v.size(), idx.data(), (int)idx.size());
}
void Gfx::shadowBox(float x, float y, float w, float h, float rad, float a, float spread) {
  for (int i = 5; i >= 1; i--) { float s = spread * i / 5.f; rrect(x - s, y - s + spread * 0.45f, w + 2 * s, h + 2 * s, rad + s, 0x000000, a / 7.f); }
}
void Gfx::noiseOverlay(float x, float y, float w, float h, float a) {
  if (!noiseTex) return; SDL_Rect clip{(int)x, (int)y, (int)w, (int)h}; SDL_RenderSetClipRect(r, &clip);
  float ts = 128.f * std::max(1.f, u * 0.8f); SDL_SetTextureAlphaMod(noiseTex, (Uint8)(clampf(a, 0, 1) * 255)); SDL_SetTextureColorMod(noiseTex, 255, 230, 190);
  for (float ty = y; ty < y + h; ty += ts) for (float tx = x; tx < x + w; tx += ts) { SDL_FRect d{tx, ty, ts, ts}; SDL_RenderCopyF(r, noiseTex, nullptr, &d); }
  SDL_RenderSetClipRect(r, nullptr);
}
void Gfx::textShadow(const std::string& s, float x, float y, FontSize f, uint32_t col, Align al, float a, uint32_t sh) {
  float o = std::max(1.f, u * 1.3f); text(s, x + o, y + o, f, sh, al, a * 0.85f); text(s, x, y, f, col, al, a);
}
void Gfx::star(float cx, float cy, float rad, uint32_t col, float a, float rot) {
  Pt p[10]; for (int i = 0; i < 10; i++) { float an = rot - 3.14159265f / 2 + i * 3.14159265f / 5, rr = i % 2 == 0 ? rad : rad * 0.42f; p[i] = {cx + std::cos(an) * rr, cy + std::sin(an) * rr}; }
  fillPoly(p, 10, col, a);
}
void Gfx::ring(float x, float y, float rad, float thick, uint32_t col, float a) {
  std::vector<SDL_Vertex> v; std::vector<int> idx; const int n = 28; SDL_Color k = col4(col, a);
  for (int i = 0; i < n; i++) { float an = i * 6.2831853f / n; for (int j = 0; j < 2; j++) { SDL_Vertex q; float rr = j ? rad : rad - thick; q.position = {x + std::cos(an) * rr, y + std::sin(an) * rr}; q.color = k; q.tex_coord = {0, 0}; v.push_back(q); } }
  for (int i = 0; i < n; i++) { int a0 = i * 2, a1 = i * 2 + 1, b0 = ((i + 1) % n) * 2, b1 = ((i + 1) % n) * 2 + 1; idx.insert(idx.end(), {a0, a1, b1, a0, b1, b0}); }
  SDL_RenderGeometry(r, nullptr, v.data(), (int)v.size(), idx.data(), (int)idx.size());
}
