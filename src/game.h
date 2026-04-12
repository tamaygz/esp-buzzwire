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

// ── Accessors (for debug / display) ─────────────────────────────────────────
GameState gameGetState();
unsigned long gameGetElapsed();
int gameGetFailCount();

#endif // GAME_H
