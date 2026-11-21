// Lightweight particle system (smoke, dust, sparks, flames, healing motes)
#pragma once
#include <cstdint>
#include <vector>

enum FxKind { FX_SMOKE, FX_DUST, FX_SPARK, FX_FLAME, FX_HEAL, FX_DEBRIS, FX_RING };
struct Particle { float x, y, z, vx, vy, vz, life, max, size, grow; uint32_t col; int kind; };
extern std::vector<Particle> FX_P;
void fxSpawn(int kind, float gx, float gy, float zpx, int n = 1);   // grid position, height in px at zoom 1
void fxUpdate(float dt);
void fxClear();
