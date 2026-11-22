#include "audio.h"
#include <SDL.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <ctime>
#include <new>
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
// Music: two "decks" stream mp3 files and crossfade.  Village tracks and battle tracks each form a playlist; the next track is picked at random
// (weighted by Age / time of day for the village, by battle size / defence / Imperial for fights, never repeating the last two), and a track that is about to
// end fades into the next one.  If no mp3 can be found the synthesised loops are used.
constexpr int DEV_RATE = 44100;
struct Voice { const int16_t* data = nullptr; size_t len = 0; float pos = 0, vol = 0; };
struct MStream {
  drmp3 dec; std::vector<unsigned char> file; bool ok = false, eof = false; std::vector<int16_t> blk; int blkCount = 0, blkIdx = 0, ch = 2; double ratio = 1, frac = 0; float p[2] = {0, 0}, n[2] = {0, 0};
  bool fetch(float& l, float& r) {
    if (eof) return false;
    if (blkIdx >= blkCount) { blkCount = (int)drmp3_read_pcm_frames_s16(&dec, 2048, blk.data()); blkIdx = 0; if (blkCount <= 0) { eof = true; return false; } }
    const int16_t* q = &blk[(size_t)blkIdx * ch]; blkIdx++; l = q[0] / 32768.f; r = (ch > 1 ? q[1] : q[0]) / 32768.f; return true;
  }
  bool open(const std::string& path) {
    size_t sz = 0; void* d = SDL_LoadFile(path.c_str(), &sz); if (!d) return false; file.assign((unsigned char*)d, (unsigned char*)d + sz); SDL_free(d);
    if (!drmp3_init_memory(&dec, file.data(), file.size(), nullptr)) { file.clear(); return false; }
    ch = (int)dec.channels; ratio = (double)dec.sampleRate / DEV_RATE; blk.assign(2048 * std::max(1, ch), 0); blkCount = blkIdx = 0; eof = false; ok = fetch(n[0], n[1]); frac = 0; if (ok) { p[0] = n[0]; p[1] = n[1]; fetch(n[0], n[1]); } return ok;
  }
  void close() { if (ok) drmp3_uninit(&dec); ok = false; file.clear(); file.shrink_to_fit(); blk.clear(); blk.shrink_to_fit(); }
  void frame(float& l, float& r) {
    l = p[0] + (n[0] - p[0]) * (float)frac; r = p[1] + (n[1] - p[1]) * (float)frac; frac += ratio;
    while (frac >= 1.0) { p[0] = n[0]; p[1] = n[1]; if (!fetch(n[0], n[1])) { n[0] = n[1] = 0; } frac -= 1.0; }
  }
};
struct Deck { MStream ms; bool active = false, loaded = false, synth = false; float vol = 0, target = 0, step = 0; uint64_t played = 0, total = 0; size_t spos = 0; int list = 0, idx = -1; };
Deck decks[2]; int curDeck = -1; bool needNext = false, modeChanged = false;
Track wantMode = MUS_NONE;
int ctxAge = 0, ctxSize = 1; bool ctxDefence = false, ctxImperial = false;
std::vector<int16_t> sfxData[SFX_COUNT];
std::vector<int16_t> musData[3];            // synthesised fallback loops (generated only when no mp3 can be found)
Voice voices[24];
SDL_AudioDeviceID dev = 0;
bool musicOn = true, sfxOn = true; float musicVolume = 0.85f, sfxVolume = 1.f;
uint32_t lastPlay[SFX_COUNT] = {};
const uint32_t minGap[SFX_COUNT] = {25, 40, 100, 100, 70, 55, 160, 130, 140, 90, 350, 50, 800, 800, 800, 800, 80};
std::vector<int16_t> toPcm(const Buf& b) { std::vector<int16_t> o(b.size()); for (size_t i = 0; i < b.size(); i++) o[i] = (int16_t)(std::max(-1.f, std::min(1.f, b[i])) * 32000.f); return o; }

const char* PLAYLIST[2][6] = {
  {"music_village.mp3", "music_village_dark.mp3", "music_village_day.mp3", "music_village_evening.mp3", "music_village_feudal.mp3", "music_village_imperial.mp3"},
  {"music_raid.mp3", "music_battle_light.mp3", "music_battle_standard.mp3", "music_battle_big.mp3", "music_battle_defence.mp3", "music_battle_imperial.mp3"}};
int history[2][2] = {{-1, -1}, {-1, -1}}; bool missing[2][6] = {};

void SDLCALL mixer(void*, Uint8* stream, int len) {
  int16_t* out = (int16_t*)stream; int n = len / 4;
  for (int i = 0; i < n; i++) {
    float L = 0, R = 0;
    for (Deck& d : decks) {
      if (!d.active) continue;
      if (d.vol < d.target) d.vol = std::min(d.target, d.vol + d.step); else if (d.vol > d.target) d.vol = std::max(d.target, d.vol - d.step);
      if (d.target == 0.f && d.vol <= 0.0005f) { d.active = false; continue; }
      float l = 0, r = 0;
      if (d.synth) { const std::vector<int16_t>& md = musData[d.list + 1]; if (!md.empty()) { float m = md[d.spos / 2] / 32768.f * 0.38f; d.spos++; if (d.spos / 2 >= md.size()) d.spos = 0; l = r = m; } }
      else { d.ms.frame(l, r); d.played++; if (d.ms.eof) { d.target = 0.f; d.step = std::max(d.step, 1.f / (DEV_RATE * 0.05f)); } }
      if (musicOn) { L += l * d.vol * 0.8f * musicVolume; R += r * d.vol * 0.8f * musicVolume; }
    }
    if (sfxOn) for (Voice& vo : voices) if (vo.data) {
      size_t i0 = (size_t)vo.pos; float fr = vo.pos - i0; float a = vo.data[i0] / 32768.f, b = i0 + 1 < vo.len ? vo.data[i0 + 1] / 32768.f : a; float m = (a + (b - a) * fr) * vo.vol * sfxVolume; L += m; R += m;
      vo.pos += 0.5f; if (vo.pos >= vo.len - 1) vo.data = nullptr;
    }
    L = std::max(-1.f, std::min(1.f, L)); R = std::max(-1.f, std::min(1.f, R));
    out[i * 2] = (int16_t)(L * 30000.f); out[i * 2 + 1] = (int16_t)(R * 30000.f);
  }
  if (curDeck >= 0 && decks[curDeck].active && !decks[curDeck].synth && !needNext && decks[curDeck].target > 0.f && decks[curDeck].total > 0 && decks[curDeck].played + (uint64_t)(DEV_RATE * 3.8) >= decks[curDeck].total) needNext = true;
}
std::string musicPath(const char* f) {
#ifdef __ANDROID__
  return f;
#else
  char* bp = SDL_GetBasePath(); std::string base = bp ? bp : ""; if (bp) SDL_free(bp); return base + "assets/" + f;
#endif
}
// weighted random pick that favours tracks matching the situation and avoids the last two played
int pickTrack(int list) {
  float w[6]; int hour = 12; { std::time_t tt = std::time(nullptr); std::tm* lt = std::localtime(&tt); if (lt) hour = lt->tm_hour; } bool night = hour >= 19 || hour < 5;
  for (int k = 0; k < 6; k++) {
    float v = 1.f;
    if (list == 0) {
      switch (k) {
        case 1: v = ctxAge == 0 ? 4.f : ctxAge == 1 ? 2.5f : 0.4f; break;
        case 2: v = night ? 0.8f : 2.4f; break;
        case 3: v = night ? 3.5f : 0.5f; break;
        case 4: v = (ctxAge == 1 || ctxAge == 2) ? 3.f : 0.5f; break;
        case 5: v = ctxAge >= 3 ? 4.f : 0.25f; break;
        default: v = 1.f;
      }
    } else {
      int s = ctxSize;
      switch (k) {
        case 1: v = s == 0 ? 3.f : s == 1 ? 1.f : 0.3f; break;
        case 2: v = s == 0 ? 1.f : s == 1 ? 3.f : 1.5f; break;
        case 3: v = s == 0 ? 0.3f : s == 1 ? 1.5f : 3.f; break;
        case 4: v = ctxDefence ? 4.5f : 0.15f; break;
        case 5: v = ctxImperial ? 3.5f : 0.3f; break;
        default: v = 1.f;
      }
    }
    if (missing[list][k]) v = 0.f;
    if (k == history[list][0]) v *= 0.02f; if (k == history[list][1]) v *= 0.25f;
    w[k] = v;
  }
  float tot = 0; for (float v : w) tot += v; if (tot <= 0) return -1;
  float r = (float)(std::rand() % 10000) / 10000.f * tot; for (int k = 0; k < 6; k++) { r -= w[k]; if (r <= 0) return k; }
  return 5;
}
}  // namespace

bool audioInit() {
  for (int i = 0; i < SFX_COUNT; i++) { Buf b = genSfx(i); for (int pass = 0; pass < 2; pass++) { float lp = 0; for (float& v : b) { lp += (v - lp) * 0.62f; v = lp; } } normalize(b, i == SFX_CLICK || i == SFX_ARROW ? 0.5f : 0.75f); sfxData[i] = toPcm(b); }
  if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) { std::fprintf(stderr, "audio init: %s\n", SDL_GetError()); return false; }
  SDL_AudioSpec want{}, have{}; want.freq = DEV_RATE; want.format = AUDIO_S16SYS; want.channels = 2; want.samples = 1024; want.callback = mixer;
  dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
  if (!dev) { std::fprintf(stderr, "audio device: %s\n", SDL_GetError()); return false; }
  SDL_PauseAudioDevice(dev, 0); return true;
}
void audioShutdown() { if (dev) { SDL_CloseAudioDevice(dev); dev = 0; } for (Deck& d : decks) { d.ms.close(); d.loaded = false; d.active = false; } }
bool audioDeviceOpen() { return dev != 0; }
void audioSetEnabled(bool music, bool sfx) { if (dev) SDL_LockAudioDevice(dev); musicOn = music; sfxOn = sfx; if (dev) SDL_UnlockAudioDevice(dev); }
void audioSetVolumes(float music, float sfx) { if (dev) SDL_LockAudioDevice(dev); musicVolume = std::max(0.f, std::min(1.f, music)); sfxVolume = std::max(0.f, std::min(1.f, sfx)); if (dev) SDL_UnlockAudioDevice(dev); }
void audioMusic(Track t) { if (dev) SDL_LockAudioDevice(dev); if (t != wantMode) { wantMode = t; modeChanged = true; } if (dev) SDL_UnlockAudioDevice(dev); }
void audioMusicContext(int age, int battleSize, bool defence, bool imperial) { if (dev) SDL_LockAudioDevice(dev); ctxAge = age; ctxSize = battleSize; ctxDefence = defence; ctxImperial = imperial; if (dev) SDL_UnlockAudioDevice(dev); }
// called once per frame from the main thread: frees finished decks, and starts the next track (mode change or the current one is about to end)
void audioTick() {
  if (!dev) return;
  SDL_LockAudioDevice(dev);
  for (Deck& d : decks) if (!d.active && d.loaded) { d.ms.close(); d.loaded = false; d.synth = false; }
  bool nn = needNext, ch = modeChanged; Track wm = wantMode; int cur = curDeck;
  SDL_UnlockAudioDevice(dev);
  if (wm == MUS_NONE) return;
  if (!ch && !nn && cur >= 0 && decks[cur].active) return;
  int slot = -1; for (int k = 0; k < 2; k++) if (!decks[k].active && !decks[k].loaded) { slot = k; break; }
  if (slot < 0) return;                                              // both decks are still busy fading: try again next frame
  int list = wm == MUS_BATTLE ? 1 : 0, idx = -1; Deck& nd = decks[slot];      // opened in place: the decoder keeps a pointer to itself, so it must never be moved
  for (int tries = 0; tries < 6 && idx < 0; tries++) {
    int k = pickTrack(list); if (k < 0) break;
    if (nd.ms.open(musicPath(PLAYLIST[list][k]))) { idx = k; break; }
    missing[list][k] = true;
  }
  float fadeSec = ch ? 1.6f : 3.6f;
  if (idx >= 0) {
    uint64_t frames = drmp3_get_pcm_frame_count(&nd.ms.dec); nd.total = (uint64_t)((double)frames * DEV_RATE / std::max<drmp3_uint32>(1, nd.ms.dec.sampleRate));
    drmp3_seek_to_pcm_frame(&nd.ms.dec, 0); nd.ms.blkCount = nd.ms.blkIdx = 0; nd.ms.eof = false; nd.ms.fetch(nd.ms.n[0], nd.ms.n[1]); nd.ms.p[0] = nd.ms.n[0]; nd.ms.p[1] = nd.ms.n[1]; nd.ms.fetch(nd.ms.n[0], nd.ms.n[1]); nd.ms.frac = 0;
    history[list][1] = history[list][0]; history[list][0] = idx;
  } else {                                                           // no mp3 at all: fall back to the synthesised loop
    if (musData[list + 1].empty()) musData[list + 1] = toPcm(list == 0 ? genVillage() : genBattle());
    nd.synth = true;
  }
  SDL_LockAudioDevice(dev);
  Deck& dst = nd; dst.synth = idx < 0; dst.loaded = idx >= 0; if (idx < 0) dst.total = 0; dst.played = 0; dst.spos = 0; dst.list = list; dst.idx = idx;
  dst.vol = 0; dst.target = 1; dst.step = 1.f / (DEV_RATE * fadeSec); dst.active = true;
  if (cur >= 0 && decks[cur].active) { decks[cur].target = 0; decks[cur].step = 1.f / (DEV_RATE * fadeSec); }
  curDeck = slot; needNext = false; modeChanged = false;
  SDL_UnlockAudioDevice(dev);
}
void audioPlay(Sfx s, float vol) {
  if (!dev || !sfxOn || s < 0 || s >= SFX_COUNT || sfxData[s].empty()) return;
  uint32_t now = SDL_GetTicks(); if (now - lastPlay[s] < minGap[s]) return; lastPlay[s] = now;
  SDL_LockAudioDevice(dev);
  Voice* slot = nullptr; for (Voice& v : voices) if (!v.data) { slot = &v; break; }
  if (!slot) { slot = &voices[0]; for (Voice& v : voices) if (v.pos > slot->pos) slot = &v; }
  slot->data = sfxData[s].data(); slot->len = sfxData[s].size(); slot->pos = 0.f; slot->vol = 0.55f * vol;
  SDL_UnlockAudioDevice(dev);
}
int audioNowPlaying(int& list) { if (!dev || curDeck < 0 || !decks[curDeck].active) return -1; list = decks[curDeck].list; return decks[curDeck].synth ? -2 : decks[curDeck].idx; }
void audioForceNext() { if (dev) SDL_LockAudioDevice(dev); needNext = true; if (dev) SDL_UnlockAudioDevice(dev); }
int audioPickSample(int list, int age, int size, bool defence, bool imperial) { int a0 = ctxAge, s0 = ctxSize; bool d0 = ctxDefence, i0 = ctxImperial; int h0 = history[list][0], h1 = history[list][1]; history[list][0] = history[list][1] = -1; ctxAge = age; ctxSize = size; ctxDefence = defence; ctxImperial = imperial; int r = pickTrack(list); ctxAge = a0; ctxSize = s0; ctxDefence = d0; ctxImperial = i0; history[list][0] = h0; history[list][1] = h1; return r; }
int audioSelfTest(std::string& report) {
  static const char* names[SFX_COUNT] = {"click", "place", "built", "coin", "hit", "arrow", "bolt", "cannon", "collapse", "trap", "heal", "deploy", "horn", "fanfare", "victory", "defeat", "error"};
  int bad = 0; char line[96];
  for (int i = 0; i < SFX_COUNT; i++) {
    if (sfxData[i].empty()) { std::snprintf(line, sizeof line, "sfx %-9s EMPTY\n", names[i]); report += line; bad++; continue; }
    int peak = 0; double rms = 0; for (int16_t v : sfxData[i]) { peak = std::max(peak, std::abs((int)v)); rms += (double)v * v; } rms = std::sqrt(rms / sfxData[i].size());
    std::snprintf(line, sizeof line, "sfx %-9s %.2fs peak %.2f rms %.3f\n", names[i], sfxData[i].size() / (float)RATE, peak / 32768.0, rms / 32768.0); report += line;
    if (peak < 3000 || rms < 100) bad++;
  }
  for (int l = 0; l < 2; l++) {
    int found = 0; for (int k = 0; k < 6; k++) { MStream ms; if (ms.open(musicPath(PLAYLIST[l][k]))) { found++; ms.close(); } }
    std::snprintf(line, sizeof line, "%s playlist: %d of 6 mp3 tracks found\n", l == 0 ? "village" : "battle", found); report += line;
    if (found == 0) { if (musData[l + 1].empty()) musData[l + 1] = toPcm(l == 0 ? genVillage() : genBattle()); int peak = 0; for (int16_t v : musData[l + 1]) peak = std::max(peak, std::abs((int)v)); if (peak < 3000) bad++; }
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
  if (musData[MUS_VILLAGE].empty()) musData[MUS_VILLAGE] = toPcm(genVillage());
  if (musData[MUS_BATTLE].empty()) musData[MUS_BATTLE] = toPcm(genBattle());
  if (writeWav(dir + "/music_village.wav", musData[MUS_VILLAGE])) n++;
  if (writeWav(dir + "/music_battle.wav", musData[MUS_BATTLE])) n++;
  return n;
}
