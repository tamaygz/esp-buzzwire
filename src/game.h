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
const char*  gameGetStateName(GameState s);

// ── Event Callback ──────────────────────────────────────────────────────────
// Registered function is called on every state transition.
void gameOnStateChange(void (*cb)(GameState state, unsigned long elapsed, int fails));

// ── Remote Control ───────────────────────────────────────────────────────────
// Request actions to be handled on the next gameLoop() tick.
void gameRemoteStart();
void gameRemoteReset();
void gameRemoteSimWire();
void gameRemoteSimFinish();

#endif // GAME_H
