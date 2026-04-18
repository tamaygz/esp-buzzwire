#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#include <Arduino.h>

#define SCORES_MAX 10

struct ScoreEntry {
    unsigned long timeMs;
    int           fails;
    unsigned long timestamp;  // millis() at recording time (used as relative ordering)
};

void scoreboardInit();
void scoreboardAdd(unsigned long elapsedMs, int failCount);
int  scoreboardCount();
bool scoreboardGet(int index, ScoreEntry &out);
void scoreboardClear();

#endif // SCOREBOARD_H
