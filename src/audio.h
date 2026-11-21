// Fully procedural audio: every sound effect and both music loops are synthesised at start-up (no audio files, no network).
#pragma once
#include <string>

enum Sfx { SFX_CLICK, SFX_PLACE, SFX_BUILT, SFX_COIN, SFX_HIT, SFX_ARROW, SFX_BOLT, SFX_CANNON, SFX_COLLAPSE, SFX_TRAP, SFX_HEAL, SFX_DEPLOY,
           SFX_HORN, SFX_FANFARE, SFX_VICTORY, SFX_DEFEAT, SFX_ERROR, SFX_COUNT };
enum Track { MUS_NONE, MUS_VILLAGE, MUS_BATTLE };

bool audioInit();                         // synthesises the buffers; returns false if no output device could be opened (game stays silent)
void audioShutdown();
bool audioDeviceOpen();
void audioPlay(Sfx s, float vol = 1.f);   // rate-limited per effect so battles don't turn into noise
void audioMusic(Track t);                 // crossfades to the requested loop
void audioSetEnabled(bool music, bool sfx);
// self-test: number of generated buffers that are silent or clipped (0 = all good); report lists levels
int audioSelfTest(std::string& report);
// writes every effect and both music loops as 16-bit mono WAV files so they can be auditioned outside the game
int audioExportWav(const std::string& dir);
