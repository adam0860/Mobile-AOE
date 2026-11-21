#include "audio.h"
#include <SDL.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>
#define DR_MP3_IMPLEMENTATION
#include "../third_party/dr/dr_mp3.h"

namespace {
constexpr int RATE = 22050;
constexpr float PI2 = 6.2831853f;
typedef std::vector<float> Buf;

uint32_t rngState = 0x1234567u;
float noise() { rngState ^= rngState << 13; rngState ^= rngState >> 17; rngState ^= rngState << 5; return (rngState & 0xffff) / 32768.f - 1.f; }
Buf mk(float sec) { return Buf((size_t)(sec * RATE), 0.f); }
float ex(float t, float k) { return std::exp(-t * k); }
float midiHz(int m) { return 440.f * std::pow(2.f, (m - 69) / 12.f); }

void tone(Buf& b, float t0, float dur, float f0, float f1, float amp, float decay, int harmonics = 1, float attack = 0.004f) {
  size_t s0 = (size_t)(t0 * RATE), n = (size_t)(dur * RATE); float ph = 0;
  for (size_t i = 0; i < n && s0 + i < b.size(); i++) {
    float t = (float)i / RATE, f = f0 + (f1 - f0) * (t / dur); ph += PI2 * f / RATE;
    float v = 0; for (int h = 1; h <= harmonics; h++) v += std::sin(ph * h) / (float)h;
    float a = std::min(1.f, t / attack) * ex(t, decay) * std::min(1.f, (dur - t) / 0.02f);
    b[s0 + i] += v * amp * a;
  }
}
void noiseBurst(Buf& b, float t0, float dur, float amp, float decay, float hp = 0.f, float lp = 0.f) {
  size_t s0 = (size_t)(t0 * RATE), n = (size_t)(dur * RATE); float prev = 0, lps = 0;
  for (size_t i = 0; i < n && s0 + i < b.size(); i++) {
    float t = (float)i / RATE, v = noise();
    if (hp > 0) { float y = v - prev * hp; prev = v; v = y; }
    if (lp > 0) { lps += (v - lps) * lp; v = lps; }
    b[s0 + i] += v * amp * ex(t, decay);
  }
}
// Karplus-Strong plucked string (lute / harp-like)
void pluck(Buf& b, float t0, float freq, float dur, float amp, float damp = 0.996f) {
  size_t s0 = (size_t)(t0 * RATE); int N = std::max(2, (int)(RATE / freq)); std::vector<float> d(N);
  for (int i = 0; i < N; i++) d[i] = noise() * 0.8f + (i > 0 ? d[i - 1] * 0.2f : 0.f);
  size_t len = (size_t)(dur * RATE); int idx = 0;
  for (size_t n = 0; n < len && s0 + n < b.size(); n++) {
    float out = d[idx], avg = (d[idx] + d[(idx + 1) % N]) * 0.5f * damp; d[idx] = avg; idx = (idx + 1) % N;
    float fade = std::min(1.f, (float)(len - n) / (0.03f * RATE));
    b[s0 + n] += out * amp * fade;
  }
}
void brass(Buf& b, float t0, float dur, float freq, float amp, float attack = 0.1f) {
  size_t s0 = (size_t)(t0 * RATE), n = (size_t)(dur * RATE); float ph = 0, lp = 0;
  for (size_t i = 0; i < n && s0 + i < b.size(); i++) {
    float t = (float)i / RATE; ph += PI2 * freq * (1.f + 0.004f * std::sin(t * 30.f)) / RATE;
    float v = 0; for (int h = 1; h <= 9; h++) v += std::sin(ph * h) / (float)h;
    lp += (v - lp) * (0.15f + 0.25f * std::min(1.f, t / attack));            // brass-like brightening
    float a = std::min(1.f, t / attack) * std::min(1.f, (dur - t) / 0.12f);
    b[s0 + i] += lp * amp * a;
  }
}
void normalize(Buf& b, float peak) { float m = 1e-6f; for (float v : b) m = std::max(m, std::fabs(v)); float k = peak / m; for (float& v : b) v *= k; }

// ------------------------------------------------------------------ effects
Buf genSfx(int id) {
  Buf b;
  switch (id) {
    case SFX_CLICK: b = mk(0.07f); tone(b, 0, 0.07f, 1100, 1000, 0.5f, 55); noiseBurst(b, 0, 0.01f, 0.12f, 200); break;
    case SFX_PLACE: b = mk(0.22f); tone(b, 0, 0.2f, 170, 65, 0.9f, 16); noiseBurst(b, 0, 0.05f, 0.35f, 90, 0, 0.5f); break;
    case SFX_BUILT: b = mk(0.9f); for (int i = 0; i < 3; i++) tone(b, 0, 0.85f, 784.f * (1 + i * 0.5f), 784.f * (1 + i * 0.5f), 0.32f / (i + 1), 4.f + i * 2); tone(b, 0.18f, 0.7f, 1046, 1046, 0.25f, 5); break;
    case SFX_COIN: b = mk(0.4f); tone(b, 0, 0.3f, 1760, 1760, 0.4f, 12, 2); tone(b, 0.07f, 0.32f, 2349, 2349, 0.4f, 11, 2); break;
    case SFX_HIT: b = mk(0.14f); tone(b, 0, 0.12f, 1800, 1800, 0.25f, 35); tone(b, 0, 0.12f, 2750, 2750, 0.2f, 45); noiseBurst(b, 0, 0.1f, 0.4f, 60, 0.6f); break;
    case SFX_ARROW: b = mk(0.16f); { float prev = 0; for (size_t i = 0; i < b.size(); i++) { float t = (float)i / RATE, v = noise(); float y = v - prev * 0.92f; prev = v; b[i] = y * 0.3f * std::sin(3.14159f * t / 0.16f); } } break;
    case SFX_BOLT: b = mk(0.4f); tone(b, 0, 0.3f, 190, 120, 0.7f, 10, 3); noiseBurst(b, 0.02f, 0.3f, 0.35f, 9, 0.8f); break;
    case SFX_CANNON: b = mk(0.9f); tone(b, 0, 0.8f, 95, 32, 1.0f, 5); noiseBurst(b, 0, 0.7f, 0.9f, 7, 0, 0.12f); break;
    case SFX_COLLAPSE: b = mk(1.0f); noiseBurst(b, 0, 0.95f, 0.9f, 3.5f, 0, 0.1f); for (int i = 0; i < 14; i++) noiseBurst(b, 0.05f + i * 0.05f * (0.4f + (noise() + 1) * 0.5f), 0.04f, 0.5f, 60, 0.5f); tone(b, 0, 0.5f, 70, 40, 0.5f, 6); break;
    case SFX_TRAP: b = mk(0.4f); noiseBurst(b, 0, 0.12f, 0.8f, 30, 0.5f); tone(b, 0, 0.3f, 120, 60, 0.8f, 12); noiseBurst(b, 0.1f, 0.25f, 0.3f, 12, 0, 0.3f); break;
    case SFX_HEAL: b = mk(0.55f); tone(b, 0, 0.5f, 600, 960, 0.35f, 3, 2, 0.05f); tone(b, 0.05f, 0.45f, 900, 1440, 0.2f, 4, 1, 0.05f); break;
    case SFX_DEPLOY: b = mk(0.12f); tone(b, 0, 0.1f, 420, 700, 0.45f, 22, 2); break;
    case SFX_HORN: b = mk(2.0f); brass(b, 0.0f, 0.85f, 146.8f, 0.5f, 0.18f); brass(b, 0.95f, 1.0f, 220.f, 0.5f, 0.15f); break;
    case SFX_FANFARE: b = mk(2.3f); { float n[4] = {261.6f, 329.6f, 392.f, 523.3f}; for (int i = 0; i < 4; i++) { brass(b, i * 0.22f, i < 3 ? 0.24f : 1.4f, n[i], 0.45f, 0.04f); brass(b, i * 0.22f, i < 3 ? 0.24f : 1.4f, n[i] * 0.5f, 0.25f, 0.04f); } } break;
    case SFX_VICTORY: b = mk(2.8f); { float n[5] = {392.f, 523.3f, 659.3f, 784.f, 1046.5f}; for (int i = 0; i < 5; i++) brass(b, i * 0.2f, i < 4 ? 0.22f : 1.6f, n[i], 0.4f, 0.04f); tone(b, 1.0f, 1.6f, 523.3f, 523.3f, 0.2f, 2.5, 3); tone(b, 1.0f, 1.6f, 784.f, 784.f, 0.15f, 2.5, 3); } break;
    case SFX_DEFEAT: b = mk(2.4f); { float n[4] = {220.f, 174.6f, 146.8f, 110.f}; for (int i = 0; i < 4; i++) brass(b, i * 0.45f, i < 3 ? 0.5f : 0.6f, n[i], 0.35f, 0.12f); tone(b, 1.4f, 1.0f, 73.4f, 73.4f, 0.4f, 2.f, 3, 0.1f); } break;
    case SFX_ERROR: b = mk(0.18f); { float ph = 0; for (size_t i = 0; i < b.size(); i++) { float t = (float)i / RATE; ph += PI2 * 190.f / RATE; b[i] = (std::sin(ph) > 0 ? 0.3f : -0.3f) * ex(t, 18); } } break;
  }
  return b;
}

// ------------------------------------------------------------------ music
void drum(Buf& b, float t, float amp) { tone(b, t, 0.22f, 130, 48, amp, 14); noiseBurst(b, t, 0.03f, amp * 0.5f, 120, 0, 0.4f); }
void snare(Buf& b, float t, float amp) { noiseBurst(b, t, 0.14f, amp, 22, 0.4f); tone(b, t, 0.08f, 190, 150, amp * 0.5f, 30); }
void hat(Buf& b, float t, float amp) { noiseBurst(b, t, 0.04f, amp, 90, 0.95f); }

Buf genVillage() {
  const float bpm = 78.f, beat = 60.f / bpm; const int bars = 16; Buf b = mk(bars * 4 * beat);
  // chords (D dorian): Dm, C, G, Dm  (root, third, fifth, octave as midi)
  int chords[4][4] = {{50, 53, 57, 62}, {48, 52, 55, 60}, {43, 47, 50, 55}, {50, 53, 57, 62}};
  for (int bar = 0; bar < bars; bar++) {
    int* c = chords[bar % 4]; float t0 = bar * 4 * beat;
    // soft arpeggio on eighth notes: root, 3rd, 5th, octave, 5th, 3rd, 5th, 3rd
    static const int pat[8] = {0, 1, 2, 3, 2, 1, 2, 1};
    for (int i = 0; i < 8; i++) pluck(b, t0 + i * beat * 0.5f, midiHz(c[pat[i]] + 12), beat * 1.6f, 0.07f, 0.9965f);
    pluck(b, t0, midiHz(c[0] - 12), beat * 3.5f, 0.13f, 0.998f);                  // bass note
    drum(b, t0, 0.12f); drum(b, t0 + 2 * beat, 0.07f);
    for (int h = 1; h < 8; h += 2) hat(b, t0 + h * beat * 0.5f, 0.012f);
  }
  // melody (midi, beats, rest -> -1), 8 bars repeated twice
  struct N { int m; float d; };
  static const N mel[] = {{74, 1}, {77, 1}, {81, 2},  {79, 1}, {76, 1}, {72, 2},  {74, 1}, {79, 1}, {83, 2},  {81, 2}, {74, 2},
                          {77, 2}, {76, 1}, {74, 1},  {76, 1}, {79, 1}, {76, 2},  {74, 1}, {71, 1}, {67, 2},  {69, 4}};
  float t = 0; for (int rep = 0; rep < 2; rep++) for (const N& n : mel) { if (n.m > 0) pluck(b, t * beat, midiHz(n.m), n.d * beat * 1.4f, 0.22f, 0.9985f); t += n.d; }
  // drone + pad
  for (size_t i = 0; i < b.size(); i++) { float tt = (float)i / RATE; b[i] += 0.035f * std::sin(PI2 * 73.4f * tt) * (0.7f + 0.3f * std::sin(tt * 0.35f)) + 0.02f * std::sin(PI2 * 110.f * tt); }
  normalize(b, 0.8f); return b;
}
Buf genBattle() {
  const float bpm = 128.f, beat = 60.f / bpm; const int bars = 16; Buf b = mk(bars * 4 * beat);
  int roots[4] = {38, 34, 41, 36};                         // Dm, Bb, F, C (bass)
  int thirds[4] = {41, 38, 45, 40}, fifths[4] = {45, 41, 48, 43};
  for (int bar = 0; bar < bars; bar++) {
    float t0 = bar * 4 * beat; int c = bar % 4;
    for (int i = 0; i < 16; i++) { int m = (i % 4 == 0 ? roots[c] : i % 4 == 2 ? fifths[c] : thirds[c]) + 24; pluck(b, t0 + i * beat * 0.25f, midiHz(m), beat * 0.55f, 0.07f, 0.994f); }
    for (int i = 0; i < 4; i++) { drum(b, t0 + i * beat, i % 2 == 0 ? 0.35f : 0.2f); if (i % 2 == 1) snare(b, t0 + i * beat, 0.28f); }
    for (int h = 0; h < 8; h++) hat(b, t0 + h * beat * 0.5f, 0.03f);
    pluck(b, t0, midiHz(roots[c] - 12), beat * 1.8f, 0.2f, 0.997f); pluck(b, t0 + 2 * beat, midiHz(roots[c] - 12), beat * 1.8f, 0.17f, 0.997f);
    brass(b, t0, beat * 1.6f, midiHz(roots[c] + 12), 0.1f, 0.02f);
    if (bar % 4 == 3) { snare(b, t0 + 3.5f * beat, 0.3f); snare(b, t0 + 3.75f * beat, 0.34f); }
  }
  static const int lead[8] = {74, 74, 77, 74, 81, 79, 77, 74};
  for (int bar = 8; bar < bars; bar++) for (int i = 0; i < 8; i++) if ((i + bar) % 3 != 2) pluck(b, bar * 4 * beat + i * beat * 0.5f, midiHz(lead[i] + (bar % 4 == 1 ? -2 : 0)), beat * 0.6f, 0.16f, 0.996f);
  normalize(b, 0.8f); return b;
}

// ------------------------------------------------------------------ engine
// Device: 44.1 kHz stereo.  Effects are synthesised mono @22.05 kHz (played with linear interpolation).
// Music: assets/music_village.mp3 and assets/music_raid.mp3 are streamed (decoded on the fly, looped); if they are missing the synthesised loops are used.
constexpr int DEV_RATE = 44100;
struct Voice { const int16_t* data = nullptr; size_t len = 0; float pos = 0, vol = 0; };
struct MStream {
  drmp3 dec; std::vector<unsigned char> file; bool ok = false; std::vector<int16_t> blk; int blkCount = 0, blkIdx = 0, ch = 2; double ratio = 1, frac = 0; float p[2] = {0, 0}, n[2] = {0, 0};
  bool fetch(float& l, float& r) {
    if (blkIdx >= blkCount) { blkCount = (int)drmp3_read_pcm_frames_s16(&dec, 2048, blk.data()); blkIdx = 0; if (blkCount <= 0) { drmp3_seek_to_pcm_frame(&dec, 0); blkCount = (int)drmp3_read_pcm_frames_s16(&dec, 2048, blk.data()); if (blkCount <= 0) return false; } }
    const int16_t* q = &blk[(size_t)blkIdx * ch]; blkIdx++; l = q[0] / 32768.f; r = (ch > 1 ? q[1] : q[0]) / 32768.f; return true;
  }
  bool open(const std::string& path) {
    size_t sz = 0; void* d = SDL_LoadFile(path.c_str(), &sz); if (!d) return false; file.assign((unsigned char*)d, (unsigned char*)d + sz); SDL_free(d);
    if (!drmp3_init_memory(&dec, file.data(), file.size(), nullptr)) return false;
    ch = (int)dec.channels; ratio = (double)dec.sampleRate / DEV_RATE; blk.assign(2048 * std::max(1, ch), 0); blkCount = blkIdx = 0; ok = fetch(n[0], n[1]); frac = 0; if (ok) { p[0] = n[0]; p[1] = n[1]; fetch(n[0], n[1]); } return ok;
  }
  void frame(float& l, float& r) {
    l = p[0] + (n[0] - p[0]) * (float)frac; r = p[1] + (n[1] - p[1]) * (float)frac; frac += ratio;
    while (frac >= 1.0) { p[0] = n[0]; p[1] = n[1]; if (!fetch(n[0], n[1])) { n[0] = n[1] = 0; } frac -= 1.0; }
  }
};
std::vector<int16_t> sfxData[SFX_COUNT];
std::vector<int16_t> musData[3];            // synthesised fallback loops (generated only when an mp3 is missing)
MStream streams[3]; bool streamLoaded = false;
Voice voices[24];
SDL_AudioDeviceID dev = 0;
bool musicOn = true, sfxOn = true;
Track wantTrack = MUS_NONE; float musVol[3] = {0, 0, 0}; size_t musPos[3] = {0, 0, 0};
uint32_t lastPlay[SFX_COUNT] = {};
const uint32_t minGap[SFX_COUNT] = {25, 40, 100, 100, 70, 55, 160, 130, 140, 90, 350, 50, 800, 800, 800, 800, 80};
std::vector<int16_t> toPcm(const Buf& b) { std::vector<int16_t> o(b.size()); for (size_t i = 0; i < b.size(); i++) o[i] = (int16_t)(std::max(-1.f, std::min(1.f, b[i])) * 32000.f); return o; }

void SDLCALL mixer(void*, Uint8* stream, int len) {
  int16_t* out = (int16_t*)stream; int n = len / 4;
  for (int i = 0; i < n; i++) {
    float L = 0, R = 0;
    for (int tr = 1; tr <= 2; tr++) {
      float target = (musicOn && wantTrack == tr) ? 1.f : 0.f; float& v = musVol[tr];
      const float step = 1.f / (DEV_RATE * 1.5f);                      // 1.5 s crossfade
      if (v < target) v = std::min(target, v + step); else if (v > target) v = std::max(target, v - step);
      if (v <= 0.001f) continue;
      if (streams[tr].ok) { float l, r; streams[tr].frame(l, r); L += l * v * 0.8f; R += r * v * 0.8f; }
      else if (!musData[tr].empty()) { float m = musData[tr][musPos[tr] / 2] / 32768.f * v * 0.38f; musPos[tr]++; if (musPos[tr] / 2 >= musData[tr].size()) musPos[tr] = 0; L += m; R += m; }
    }
    if (sfxOn) for (Voice& vo : voices) if (vo.data) {
      size_t i0 = (size_t)vo.pos; float fr = vo.pos - i0; float a = vo.data[i0] / 32768.f, b = i0 + 1 < vo.len ? vo.data[i0 + 1] / 32768.f : a; float m = (a + (b - a) * fr) * vo.vol; L += m; R += m;
      vo.pos += 0.5f; if (vo.pos >= vo.len - 1) vo.data = nullptr;
    }
    L = std::max(-1.f, std::min(1.f, L)); R = std::max(-1.f, std::min(1.f, R));
    out[i * 2] = (int16_t)(L * 30000.f); out[i * 2 + 1] = (int16_t)(R * 30000.f);
  }
}
std::string musicPath(const char* f) {
#ifdef __ANDROID__
  return f;
#else
  char* bp = SDL_GetBasePath(); std::string base = bp ? bp : ""; if (bp) SDL_free(bp); return base + "assets/" + f;
#endif
}
}  // namespace

bool audioInit() {
  for (int i = 0; i < SFX_COUNT; i++) { Buf b = genSfx(i); for (int pass = 0; pass < 2; pass++) { float lp = 0; for (float& v : b) { lp += (v - lp) * 0.62f; v = lp; } } normalize(b, i == SFX_CLICK || i == SFX_ARROW ? 0.5f : 0.75f); sfxData[i] = toPcm(b); }
  streams[MUS_VILLAGE].open(musicPath("music_village.mp3")); streams[MUS_BATTLE].open(musicPath("music_raid.mp3"));
  if (!streams[MUS_VILLAGE].ok) musData[MUS_VILLAGE] = toPcm(genVillage());
  if (!streams[MUS_BATTLE].ok) musData[MUS_BATTLE] = toPcm(genBattle());
  if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) { std::fprintf(stderr, "audio init: %s\n", SDL_GetError()); return false; }
  SDL_AudioSpec want{}, have{}; want.freq = DEV_RATE; want.format = AUDIO_S16SYS; want.channels = 2; want.samples = 1024; want.callback = mixer;
  dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
  if (!dev) { std::fprintf(stderr, "audio device: %s\n", SDL_GetError()); return false; }
  SDL_PauseAudioDevice(dev, 0); return true;
}
void audioShutdown() { if (dev) { SDL_CloseAudioDevice(dev); dev = 0; } }
bool audioDeviceOpen() { return dev != 0; }
void audioSetEnabled(bool music, bool sfx) { if (dev) SDL_LockAudioDevice(dev); musicOn = music; sfxOn = sfx; if (dev) SDL_UnlockAudioDevice(dev); }
void audioMusic(Track t) { if (dev) SDL_LockAudioDevice(dev); wantTrack = t; if (dev) SDL_UnlockAudioDevice(dev); }
void audioPlay(Sfx s, float vol) {
  if (!dev || !sfxOn || s < 0 || s >= SFX_COUNT || sfxData[s].empty()) return;
  uint32_t now = SDL_GetTicks(); if (now - lastPlay[s] < minGap[s]) return; lastPlay[s] = now;
  SDL_LockAudioDevice(dev);
  Voice* slot = nullptr; for (Voice& v : voices) if (!v.data) { slot = &v; break; }
  if (!slot) { slot = &voices[0]; for (Voice& v : voices) if (v.pos > slot->pos) slot = &v; }
  slot->data = sfxData[s].data(); slot->len = sfxData[s].size(); slot->pos = 0.f; slot->vol = 0.55f * vol;
  SDL_UnlockAudioDevice(dev);
}
int audioSelfTest(std::string& report) {
  static const char* names[SFX_COUNT] = {"click", "place", "built", "coin", "hit", "arrow", "bolt", "cannon", "collapse", "trap", "heal", "deploy", "horn", "fanfare", "victory", "defeat", "error"};
  int bad = 0; char line[96];
  for (int i = 0; i < SFX_COUNT; i++) {
    if (sfxData[i].empty()) { std::snprintf(line, sizeof line, "sfx %-9s EMPTY\n", names[i]); report += line; bad++; continue; }
    int peak = 0; double rms = 0; for (int16_t v : sfxData[i]) { peak = std::max(peak, std::abs((int)v)); rms += (double)v * v; } rms = std::sqrt(rms / sfxData[i].size());
    std::snprintf(line, sizeof line, "sfx %-9s %.2fs peak %.2f rms %.3f\n", names[i], sfxData[i].size() / (float)RATE, peak / 32768.0, rms / 32768.0); report += line;
    if (peak < 3000 || rms < 100) bad++;
  }
  for (int t = 1; t <= 2; t++) {
    if (streams[t].ok) { std::snprintf(line, sizeof line, "music %d streaming mp3 (%d Hz, %d ch)\n", t, (int)streams[t].dec.sampleRate, streams[t].ch); report += line; continue; }
    int peak = 0; double rms = 0; for (int16_t v : musData[t]) { peak = std::max(peak, std::abs((int)v)); rms += (double)v * v; } rms = std::sqrt(rms / std::max<size_t>(1, musData[t].size()));
    std::snprintf(line, sizeof line, "music %d (synth fallback) %.1fs peak %.2f rms %.3f\n", t, musData[t].size() / (float)RATE, peak / 32768.0, rms / 32768.0); report += line;
    if (musData[t].size() < (size_t)RATE * 10 || peak < 3000 || rms < 300) bad++;
  }
  return bad;
}

int audioExportWav(const std::string& dir) {
  static const char* names[SFX_COUNT] = {"click", "place", "built", "coin", "hit", "arrow", "bolt", "cannon", "collapse", "trap", "heal", "deploy", "horn", "fanfare", "victory", "defeat", "error"};
  auto writeWav = [&](const std::string& path, const std::vector<int16_t>& d) {
    FILE* f = std::fopen(path.c_str(), "wb"); if (!f) return false;
    uint32_t dataLen = (uint32_t)(d.size() * 2), riff = 36 + dataLen, fmtLen = 16, rate = RATE, byteRate = RATE * 2; uint16_t pcm = 1, ch = 1, align = 2, bits = 16;
    std::fwrite("RIFF", 1, 4, f); std::fwrite(&riff, 4, 1, f); std::fwrite("WAVEfmt ", 1, 8, f); std::fwrite(&fmtLen, 4, 1, f); std::fwrite(&pcm, 2, 1, f); std::fwrite(&ch, 2, 1, f);
    std::fwrite(&rate, 4, 1, f); std::fwrite(&byteRate, 4, 1, f); std::fwrite(&align, 2, 1, f); std::fwrite(&bits, 2, 1, f); std::fwrite("data", 1, 4, f); std::fwrite(&dataLen, 4, 1, f);
    std::fwrite(d.data(), 2, d.size(), f); std::fclose(f); return true;
  };
  int n = 0;
  for (int i = 0; i < SFX_COUNT; i++) if (writeWav(dir + "/sfx_" + names[i] + ".wav", sfxData[i])) n++;
  if (!musData[MUS_VILLAGE].empty() && writeWav(dir + "/music_village.wav", musData[MUS_VILLAGE])) n++;
  if (!musData[MUS_BATTLE].empty() && writeWav(dir + "/music_battle.wav", musData[MUS_BATTLE])) n++;
  return n;
}
