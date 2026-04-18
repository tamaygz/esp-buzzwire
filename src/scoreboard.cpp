#include "scoreboard.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

static const char SCORES_PATH[] = "/scores.json";
static ScoreEntry sScores[SCORES_MAX];
static int        sCount = 0;

// ── Helpers ──────────────────────────────────────────────────────────────────
static void sortScores() {
    // Sort ascending by time (faster = better rank)
    for (int i = 0; i < sCount - 1; i++) {
        for (int j = i + 1; j < sCount; j++) {
            if (sScores[j].timeMs < sScores[i].timeMs) {
                ScoreEntry tmp = sScores[i];
                sScores[i]     = sScores[j];
                sScores[j]     = tmp;
            }
        }
    }
}

static void saveToDisk() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < sCount; i++) {
        JsonObject o    = arr.add<JsonObject>();
        o["timeMs"]     = sScores[i].timeMs;
        o["fails"]      = sScores[i].fails;
        o["timestamp"]  = sScores[i].timestamp;
    }
    File f = LittleFS.open(SCORES_PATH, "w");
    if (!f) return;
    serializeJson(doc, f);
    f.close();
}

static void loadFromDisk() {
    if (!LittleFS.exists(SCORES_PATH)) return;
    File f = LittleFS.open(SCORES_PATH, "r");
    if (!f) return;

    JsonDocument doc;
    if (deserializeJson(doc, f) != DeserializationError::Ok) { f.close(); return; }
    f.close();

    sCount = 0;
    for (JsonObject o : doc.as<JsonArray>()) {
        if (sCount >= SCORES_MAX) break;
        sScores[sCount].timeMs    = o["timeMs"]    | 0UL;
        sScores[sCount].fails     = o["fails"]     | 0;
        sScores[sCount].timestamp = o["timestamp"] | 0UL;
        sCount++;
    }
    sortScores();
}

// ── Public API ───────────────────────────────────────────────────────────────
void scoreboardInit() {
    sCount = 0;
    loadFromDisk();
    Serial.print(F("[SCORES] Loaded "));
    Serial.print(sCount);
    Serial.println(F(" entries"));
}

void scoreboardAdd(unsigned long elapsedMs, int failCount) {
    if (sCount < SCORES_MAX) {
        sScores[sCount] = { elapsedMs, failCount, millis() };
        sCount++;
    } else {
        // Replace worst only when new entry is better (lower time)
        if (elapsedMs >= sScores[sCount - 1].timeMs) return;
        sScores[sCount - 1] = { elapsedMs, failCount, millis() };
    }
    sortScores();
    saveToDisk();
    Serial.print(F("[SCORES] Added entry time="));
    Serial.print(elapsedMs / 1000UL);
    Serial.print(F("s fails="));
    Serial.println(failCount);
}

int scoreboardCount() { return sCount; }

bool scoreboardGet(int index, ScoreEntry &out) {
    if (index < 0 || index >= sCount) return false;
    out = sScores[index];
    return true;
}

void scoreboardClear() {
    sCount = 0;
    LittleFS.remove(SCORES_PATH);
    Serial.println(F("[SCORES] Cleared"));
}
