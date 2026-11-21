#include "fx.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

std::vector<Particle> FX_P;
static float rnd() { return (float)std::rand() / (float)RAND_MAX; }
static float rr(float a, float b) { return a + (b - a) * rnd(); }

void fxSpawn(int kind, float gx, float gy, float zpx, int n) {
  for (int i = 0; i < n; i++) {
    Particle p{gx, gy, zpx, 0, 0, 0, 1, 1, 3, 0, 0xffffff, kind};
    float a = rr(0, 6.2831853f);
    switch (kind) {
      case FX_SMOKE: p.x += rr(-0.04f, 0.04f); p.y += rr(-0.04f, 0.04f); p.vx = 0.16f + rr(-0.05f, 0.05f); p.vy = -0.1f + rr(-0.05f, 0.05f); p.vz = rr(15, 24); p.size = rr(4, 6); p.grow = 1.8f; p.life = p.max = rr(1.8f, 2.8f); p.col = 0xd2d0cc; break;
      case FX_DUST: p.vx = std::cos(a) * rr(0.2f, 0.7f); p.vy = std::sin(a) * rr(0.2f, 0.7f); p.vz = rr(3, 9); p.size = rr(2.2f, 3.8f); p.grow = 1.1f; p.life = p.max = rr(0.4f, 0.8f); p.col = 0xb8a47a; break;
      case FX_SPARK: p.vx = std::cos(a) * rr(0.5f, 1.7f); p.vy = std::sin(a) * rr(0.5f, 1.7f); p.vz = rr(30, 95); p.size = rr(1.3f, 2.2f); p.life = p.max = rr(0.22f, 0.5f); p.col = rnd() < 0.5f ? 0xffd070 : 0xff9a40; break;
      case FX_FLAME: p.x += rr(-0.12f, 0.12f); p.y += rr(-0.12f, 0.12f); p.vz = rr(18, 34); p.vx = rr(-0.03f, 0.03f); p.size = rr(3.5f, 6); p.grow = -0.7f; p.life = p.max = rr(0.45f, 0.85f); p.col = rnd() < 0.5f ? 0xff9020 : 0xffc040; break;
      case FX_HEAL: p.x += rr(-0.15f, 0.15f); p.y += rr(-0.15f, 0.15f); p.vz = rr(18, 38); p.size = rr(1.8f, 2.8f); p.grow = -0.4f; p.life = p.max = rr(0.7f, 1.2f); p.col = 0x8cffb0; break;
      case FX_DEBRIS: p.vx = std::cos(a) * rr(0.4f, 1.2f); p.vy = std::sin(a) * rr(0.4f, 1.2f); p.vz = rr(40, 115); p.size = rr(1.6f, 3.2f); p.life = p.max = rr(0.5f, 0.95f); p.col = rnd() < 0.5f ? 0x7a5a3a : 0x8a8478; break;
      case FX_RING: p.size = 2; p.grow = 10.f; p.life = p.max = 0.45f; p.col = 0xffe9c0; break;
    }
    if (FX_P.size() > 1800) FX_P.erase(FX_P.begin(), FX_P.begin() + 200);
    FX_P.push_back(p);
  }
}
void fxUpdate(float dt) {
  for (auto& p : FX_P) {
    p.life -= dt; p.x += p.vx * dt; p.y += p.vy * dt; p.z += p.vz * dt;
    if (p.kind == FX_SPARK || p.kind == FX_DEBRIS) { p.vz -= 250.f * dt; if (p.z < 0) { p.z = 0; p.vz *= -0.3f; p.vx *= 0.6f; p.vy *= 0.6f; } }
    if (p.kind == FX_SMOKE) { p.vx *= 1.0f - 0.2f * dt; }
  }
  FX_P.erase(std::remove_if(FX_P.begin(), FX_P.end(), [](const Particle& p) { return p.life <= 0; }), FX_P.end());
}
void fxClear() { FX_P.clear(); }
