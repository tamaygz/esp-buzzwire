#ifndef GAME_H
#define GAME_H

#include <Arduino.h>

// ── Game States ─────────────────────────────────────────────────────────────
enum GameState {
    STATE_IDLE,
    STATE_COUNTDOWN,
    STATE_PLAYING,
    STATE_FAIL,
    STATE_WIN
};

// ── Public Interface ────────────────────────────────────────────────────────
void gameSetup();
void gameLoop();

// ── Accessors ───────────────────────────────────────────────────────────────
GameState    gameGetState();
unsigned long gameGetElapsed();
int          gameGetFailCount();

// Returns a short human-readable name for the given state (e.g. "PLAYING").
// Useful for Serial debug output and diagnostics.
const char*  gameGetStateName(GameState s);

#endif // GAME_H
