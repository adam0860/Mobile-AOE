// Thin SDL2 2D drawing layer: polygons, text, camera
#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "defs.h"

struct Pt { float x, y; };
enum Align { A_LEFT, A_CENTER, A_RIGHT };
enum FontSize { F_S, F_M, F_L, F_T };      // small, medium, large, title (serif)

uint32_t mixc(uint32_t a, uint32_t b, float t);
uint32_t shadec(uint32_t c, float f);

struct Gfx {
  SDL_Window* win = nullptr; SDL_Renderer* r = nullptr; int W = 0, H = 0; float u = 1; // pixel size, ui scale
  TTF_Font* fonts[4] = {nullptr, nullptr, nullptr, nullptr}; SDL_Texture* noiseTex = nullptr;
  struct Tex { SDL_Texture* t; int w, h; };
  std::unordered_map<std::string, Tex> cache;

  bool init();
  void shutdown();
  void begin(uint32_t col);
  void present() { SDL_RenderPresent(r); }
  void setColor(uint32_t col, float a = 1);
  void fillPoly(const Pt* p, int n, uint32_t col, float a = 1);
  void strokePoly(const Pt* p, int n, uint32_t col, float a = 1, bool close = true);
  void line(float x0, float y0, float x1, float y1, uint32_t col, float a = 1);
  void rect(float x, float y, float w, float h, uint32_t col, float a = 1);
  void rectOutline(float x, float y, float w, float h, uint32_t col, float a = 1);
  void rrect(float x, float y, float w, float h, float rad, uint32_t col, float a = 1);
  void circle(float x, float y, float rad, uint32_t col, float a = 1);
  void ellipse(float x, float y, float rx, float ry, uint32_t col, float a = 1);
  void batchTris(const std::vector<SDL_Vertex>& v);
  void gradRect(float x, float y, float w, float h, uint32_t top, uint32_t bot, float a = 1);
  void gradRRect(float x, float y, float w, float h, float rad, uint32_t top, uint32_t bot, float a = 1);
  void shadowBox(float x, float y, float w, float h, float rad, float a = 0.35f, float spread = 10);
  void noiseOverlay(float x, float y, float w, float h, float a);
  void textShadow(const std::string& s, float x, float y, FontSize f, uint32_t col, Align al = A_LEFT, float a = 1, uint32_t sh = 0x000000);
  void star(float cx, float cy, float r, uint32_t col, float a = 1, float rot = 0);
  void ring(float x, float y, float rad, float thick, uint32_t col, float a = 1);
  Pt textSize(const std::string& s, FontSize f);
  void text(const std::string& s, float x, float y, FontSize f, uint32_t col, Align al = A_LEFT, float a = 1);
};
extern Gfx G;
