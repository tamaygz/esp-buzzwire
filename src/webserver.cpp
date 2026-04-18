#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "webserver.h"
#include "game_config.h"
#include "game.h"
#include "scoreboard.h"
#include "wifi_manager.h"

static AsyncWebServer  sServer(80);
static AsyncWebSocket  sWs("/ws");

// ── Deferred action flags ─────────────────────────────────────────────────────
// Reboot/format operations are deferred to webServerLoop() to keep handlers short.
enum PendingAction { PA_NONE, PA_REBOOT, PA_WIFI_RESET, PA_FORMAT };
static volatile uint8_t sPendingAction = PA_NONE;
static unsigned long    sLastSysInfoBroadcast = 0;

static void setPendingAction(PendingAction action) {
    noInterrupts();
    sPendingAction = (uint8_t)action;
    interrupts();
}

// ── JSON Builders ─────────────────────────────────────────────────────────────
static constexpr size_t kMaxSoundValueLength = 256;

static bool isAllowedSoundKey(const char* key) {
    return strcmp(key, "start") == 0 ||
           strcmp(key, "countdown") == 0 ||
           strcmp(key, "fail") == 0 ||
           strcmp(key, "win") == 0;
}

static bool validateAndCopySounds(const JsonDocument& src, JsonDocument& dst) {
    if (!src.is<JsonObjectConst>()) {
        return false;
    }

    JsonObjectConst obj = src.as<JsonObjectConst>();
    for (JsonPairConst kv : obj) {
        const char* key = kv.key().c_str();
        if (!isAllowedSoundKey(key) || !kv.value().is<const char*>()) {
            return false;
        }

        const char* value = kv.value().as<const char*>();
        if (value == nullptr || strlen(value) > kMaxSoundValueLength) {
            return false;
        }

        dst[key] = value;
    }

    return true;
}

static String buildStateJson() {
    JsonDocument doc;
    GameState gs = gameGetState();
    doc["state"]   = gameGetStateName(gs);
    doc["elapsed"] = gameGetElapsed();
    doc["fails"]   = gameGetFailCount();
    String out;
    serializeJson(doc, out);
    return out;
}

static String buildConfigJson() {
    JsonDocument doc;
    doc["proModeEnabled"]    = cfg.proModeEnabled;
    doc["proModeSensor"]     = cfg.proModeSensor;
    doc["proGreenMin"]       = cfg.proGreenMin;
    doc["proGreenMax"]       = cfg.proGreenMax;
    doc["proRedMin"]         = cfg.proRedMin;
    doc["proRedMax"]         = cfg.proRedMax;
    doc["irMoveThreshold"]   = cfg.irMoveThreshold;
    doc["debounceMs"]        = cfg.debounceMs;
    doc["stripBrightness"]   = cfg.stripBrightness;
    doc["matrixBrightness"]  = cfg.matrixBrightness;
    doc["failDisplayMs"]     = cfg.failDisplayMs;
    doc["winDisplayMs"]      = cfg.winDisplayMs;
    doc["countdownStepMs"]   = cfg.countdownStepMs;
    doc["strobeIntervalMs"]  = cfg.strobeIntervalMs;
    doc["strobeFlashCount"]  = cfg.strobeFlashCount;
    doc["scrollIdleMs"]      = cfg.scrollIdleMs;
    doc["scrollCountdownMs"] = cfg.scrollCountdownMs;
    doc["scrollFailMs"]      = cfg.scrollFailMs;
    doc["scrollWinMs"]       = cfg.scrollWinMs;
    String out;
    serializeJson(doc, out);
    return out;
}

static String buildScoresJson() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < scoreboardCount(); i++) {
        ScoreEntry e;
        if (!scoreboardGet(i, e)) continue;
        JsonObject o    = arr.add<JsonObject>();
        o["rank"]       = i + 1;
        o["timeMs"]     = e.timeMs;
        o["timeSec"]    = e.timeMs / 1000.0f;
        o["fails"]      = e.fails;
    }
    String out;
    serializeJson(doc, out);
    return out;
}

static String buildSoundsJson() {
    if (!LittleFS.exists("/sounds.json")) return "{}";
    File f = LittleFS.open("/sounds.json", "r");
    if (!f) return "{}";
    String s = f.readString();
    f.close();
    return s;
}

static String buildSysJson() {
    JsonDocument doc;
    doc["freeHeap"]   = ESP.getFreeHeap();
    doc["uptime"]     = millis() / 1000UL;
    doc["ssid"]       = wifiGetSSID();
    doc["ip"]         = wifiGetIP();
    doc["apMode"]     = wifiGetApMode();
    doc["rssi"]       = WiFi.RSSI();
    doc["wsClients"]  = sWs.count();
    String out;
    serializeJson(doc, out);
    return out;
}

// ── WebSocket broadcasts ──────────────────────────────────────────────────────
static String wrapEvent(const char* event, const String& data) {
    String out;
    out.reserve(strlen(event) + data.length() + 24);
    out = F("{\"event\":\"");
    out += event;
    out += F("\",\"data\":");
    out += data;
    out += '}';
    return out;
}

void wsBroadcastState()   { sWs.textAll(wrapEvent("state",   buildStateJson()));   }
void wsBroadcastConfig()  { sWs.textAll(wrapEvent("config",  buildConfigJson()));  }
void wsBroadcastScores()  { sWs.textAll(wrapEvent("scores",  buildScoresJson()));  }
void wsBroadcastSysInfo() { sWs.textAll(wrapEvent("sysinfo", buildSysJson()));     }

// ── WebSocket handler ─────────────────────────────────────────────────────────
static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        if (sWs.count() > WS_MAX_CLIENTS) { client->close(); return; }
        // Send full state snapshot on connect
        client->text(wrapEvent("state",   buildStateJson()));
        client->text(wrapEvent("config",  buildConfigJson()));
        client->text(wrapEvent("scores",  buildScoresJson()));
        client->text(wrapEvent("sysinfo", buildSysJson()));
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        if (!info->final || info->opcode != WS_TEXT) return;
        // Use (data, len) overload — avoids writing past the end of the receive buffer
        JsonDocument doc;
        if (deserializeJson(doc, data, len) != DeserializationError::Ok) return;
        const char* cmd = doc["cmd"] | "";
        if      (strcmp(cmd, "start")      == 0) gameRemoteStart();
        else if (strcmp(cmd, "reset")      == 0) gameRemoteReset();
        else if (strcmp(cmd, "simWire")    == 0) gameRemoteSimWire();
        else if (strcmp(cmd, "simFinish")  == 0) gameRemoteSimFinish();
    } else if (type == WS_EVT_DISCONNECT) {
        sWs.cleanupClients();
    }
}

// ── REST helpers ──────────────────────────────────────────────────────────────
static void sendJson(AsyncWebServerRequest* req, int code, const String& body) {
    req->send(code, "application/json", body);
}

static bool applyConfigValidation(const GameConfig& in, String& error) {
    if (in.proModeSensor < SENSOR_IR || in.proModeSensor > SENSOR_BOTH) {
        error = "invalid proModeSensor";
        return false;
    }
    if (in.proGreenMin == 0 || in.proGreenMax == 0 || in.proGreenMin > in.proGreenMax) {
        error = "invalid proGreen range";
        return false;
    }
    if (in.proRedMin == 0 || in.proRedMax == 0 || in.proRedMin > in.proRedMax) {
        error = "invalid proRed range";
        return false;
    }
    if (in.irMoveThreshold > 1023) { error = "invalid irMoveThreshold"; return false; }
    if (in.countdownStepMs == 0) { error = "countdownStepMs must be > 0"; return false; }
    if (in.failDisplayMs == 0 || in.winDisplayMs == 0) { error = "display durations must be > 0"; return false; }
    if (in.strobeIntervalMs == 0) { error = "strobeIntervalMs must be > 0"; return false; }
    if (in.strobeFlashCount == 0) { error = "strobeFlashCount must be > 0"; return false; }
    if (in.scrollIdleMs == 0 || in.scrollCountdownMs == 0 || in.scrollFailMs == 0 || in.scrollWinMs == 0) {
        error = "scroll timings must be > 0";
        return false;
    }
    return true;
}

// Apply a JSON body to cfg fields (only keys present are updated)
static bool applyConfigJson(const String& body, String& error) {
    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) { error = "bad JSON"; return false; }

    GameConfig next = cfg;
    next.proModeEnabled    = doc["proModeEnabled"] | next.proModeEnabled;

    if (!doc["proModeSensor"].isNull()) {
        int v = doc["proModeSensor"].as<int>();
        if (v < SENSOR_IR || v > SENSOR_BOTH) { error = "invalid proModeSensor"; return false; }
        next.proModeSensor = (uint8_t)v;
    }
    if (!doc["proGreenMin"].isNull()) {
        long v = doc["proGreenMin"].as<long>();
        if (v <= 0) { error = "invalid proGreenMin"; return false; }
        next.proGreenMin = (uint32_t)v;
    }
    if (!doc["proGreenMax"].isNull()) {
        long v = doc["proGreenMax"].as<long>();
        if (v <= 0) { error = "invalid proGreenMax"; return false; }
        next.proGreenMax = (uint32_t)v;
    }
    if (!doc["proRedMin"].isNull()) {
        long v = doc["proRedMin"].as<long>();
        if (v <= 0) { error = "invalid proRedMin"; return false; }
        next.proRedMin = (uint32_t)v;
    }
    if (!doc["proRedMax"].isNull()) {
        long v = doc["proRedMax"].as<long>();
        if (v <= 0) { error = "invalid proRedMax"; return false; }
        next.proRedMax = (uint32_t)v;
    }
    if (!doc["irMoveThreshold"].isNull()) {
        int v = doc["irMoveThreshold"].as<int>();
        if (v < 0 || v > 1023) { error = "invalid irMoveThreshold"; return false; }
        next.irMoveThreshold = (uint16_t)v;
    }
    if (!doc["debounceMs"].isNull()) {
        long v = doc["debounceMs"].as<long>();
        if (v < 0) { error = "invalid debounceMs"; return false; }
        next.debounceMs = (uint32_t)v;
    }
    if (!doc["stripBrightness"].isNull()) {
        int v = doc["stripBrightness"].as<int>();
        if (v < 0 || v > 255) { error = "invalid stripBrightness"; return false; }
        next.stripBrightness = (uint8_t)v;
    }
    if (!doc["matrixBrightness"].isNull()) {
        int v = doc["matrixBrightness"].as<int>();
        if (v < 0 || v > 255) { error = "invalid matrixBrightness"; return false; }
        next.matrixBrightness = (uint8_t)v;
    }
    if (!doc["failDisplayMs"].isNull()) {
        long v = doc["failDisplayMs"].as<long>();
        if (v <= 0) { error = "invalid failDisplayMs"; return false; }
        next.failDisplayMs = (uint32_t)v;
    }
    if (!doc["winDisplayMs"].isNull()) {
        long v = doc["winDisplayMs"].as<long>();
        if (v <= 0) { error = "invalid winDisplayMs"; return false; }
        next.winDisplayMs = (uint32_t)v;
    }
    if (!doc["countdownStepMs"].isNull()) {
        long v = doc["countdownStepMs"].as<long>();
        if (v <= 0) { error = "invalid countdownStepMs"; return false; }
        next.countdownStepMs = (uint32_t)v;
    }
    if (!doc["strobeIntervalMs"].isNull()) {
        long v = doc["strobeIntervalMs"].as<long>();
        if (v <= 0) { error = "invalid strobeIntervalMs"; return false; }
        next.strobeIntervalMs = (uint32_t)v;
    }
    if (!doc["strobeFlashCount"].isNull()) {
        int v = doc["strobeFlashCount"].as<int>();
        if (v <= 0 || v > 255) { error = "invalid strobeFlashCount"; return false; }
        next.strobeFlashCount = (uint8_t)v;
    }
    if (!doc["scrollIdleMs"].isNull()) {
        long v = doc["scrollIdleMs"].as<long>();
        if (v <= 0) { error = "invalid scrollIdleMs"; return false; }
        next.scrollIdleMs = (uint32_t)v;
    }
    if (!doc["scrollCountdownMs"].isNull()) {
        long v = doc["scrollCountdownMs"].as<long>();
        if (v <= 0) { error = "invalid scrollCountdownMs"; return false; }
        next.scrollCountdownMs = (uint32_t)v;
    }
    if (!doc["scrollFailMs"].isNull()) {
        long v = doc["scrollFailMs"].as<long>();
        if (v <= 0) { error = "invalid scrollFailMs"; return false; }
        next.scrollFailMs = (uint32_t)v;
    }
    if (!doc["scrollWinMs"].isNull()) {
        long v = doc["scrollWinMs"].as<long>();
        if (v <= 0) { error = "invalid scrollWinMs"; return false; }
        next.scrollWinMs = (uint32_t)v;
    }
    if (!applyConfigValidation(next, error)) return false;
    cfg = next;
    return true;
}

// ── Body accumulator for POST requests ────────────────────────────────────────
struct BodyState {
    String body;
    bool   rejected = false;
};

static constexpr size_t MAX_BODY_BYTES = 2048;

static BodyState* ensureBodyState(AsyncWebServerRequest* req) {
    // ESPAsyncWebServer exposes _tempObject as request-scoped scratch storage.
    BodyState* state = reinterpret_cast<BodyState*>(req->_tempObject);
    if (state == nullptr) {
        state = new BodyState();
        req->_tempObject = state;
    }
    return state;
}

static void clearBodyState(AsyncWebServerRequest* req) {
    BodyState* state = reinterpret_cast<BodyState*>(req->_tempObject);
    if (state != nullptr) {
        delete state;
        req->_tempObject = nullptr;
    }
}

static BodyState* takeBodyState(AsyncWebServerRequest* req) {
    BodyState* state = reinterpret_cast<BodyState*>(req->_tempObject);
    req->_tempObject = nullptr;
    return state;
}

static void collectBody(AsyncWebServerRequest* req,
                        uint8_t* data, size_t len, size_t index, size_t total) {
    BodyState* state = ensureBodyState(req);
    if (state == nullptr) {
        req->send(500, "application/json", "{\"error\":\"body allocation failed\"}");
        return;
    }
    if (state->rejected) return;
    if (total > MAX_BODY_BYTES) {
        state->rejected = true;
        clearBodyState(req);
        req->send(413, "application/json", "{\"error\":\"payload too large\"}");
        return;
    }
    if (index == 0) {
        state->body = "";
        state->body.reserve(total);
    }
    state->body.concat((char*)data, len);
}

// ── Route registration ────────────────────────────────────────────────────────
void webServerSetup() {
    sWs.onEvent(onWsEvent);
    sServer.addHandler(&sWs);

    // --- Static files from LittleFS ---
    sServer.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    sServer.onNotFound([](AsyncWebServerRequest* req) {
        req->send(404, "text/plain", "Not found");
    });

    // --- GET /api/state ---
    sServer.on("/api/state", HTTP_GET, [](AsyncWebServerRequest* req) {
        sendJson(req, 200, buildStateJson());
    });

    // --- POST /api/config/reset  (MUST be registered before /api/config to avoid prefix match) ---
    sServer.on("/api/config/reset", HTTP_POST, [](AsyncWebServerRequest* req) {
        configReset();
        wsBroadcastConfig();
        sendJson(req, 200, buildConfigJson());
    });

    // --- GET /api/config ---
    sServer.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* req) {
        sendJson(req, 200, buildConfigJson());
    });

    // --- POST /api/config  (body: partial JSON) ---
    sServer.on("/api/config", HTTP_POST,
        [](AsyncWebServerRequest* req) {
            BodyState* bodyState = takeBodyState(req);
            if (bodyState == nullptr) {
                sendJson(req, 400, "{\"error\":\"missing body\"}");
                return;
            }
            if (bodyState->rejected) { delete bodyState; return; }
            String err;
            if (!applyConfigJson(bodyState->body, err)) {
                delete bodyState;
                sendJson(req, 400, String("{\"error\":\"") + err + "\"}");
                return;
            }
            delete bodyState;
            configSave();
            wsBroadcastConfig();
            sendJson(req, 200, buildConfigJson());
        },
        nullptr,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            collectBody(req, data, len, index, total);
        }
    );

    // --- GET /api/scores ---
    sServer.on("/api/scores", HTTP_GET, [](AsyncWebServerRequest* req) {
        sendJson(req, 200, buildScoresJson());
    });

    // --- DELETE /api/scores ---
    sServer.on("/api/scores", HTTP_DELETE, [](AsyncWebServerRequest* req) {
        scoreboardClear();
        wsBroadcastScores();
        sendJson(req, 200, "{\"ok\":true}");
    });

    // --- GET /api/sounds ---
    sServer.on("/api/sounds", HTTP_GET, [](AsyncWebServerRequest* req) {
        sendJson(req, 200, buildSoundsJson());
    });

    // --- POST /api/sounds  (body: {"win":"url","fail":"url","start":"url","countdown":"url"}) ---
    sServer.on("/api/sounds", HTTP_POST,
        [](AsyncWebServerRequest* req) {
            BodyState* bodyState = takeBodyState(req);
            if (bodyState == nullptr) {
                sendJson(req, 400, "{\"error\":\"missing body\"}");
                return;
            }
            if (bodyState->rejected) { delete bodyState; return; }
            JsonDocument doc;
            if (deserializeJson(doc, bodyState->body) != DeserializationError::Ok) {
                delete bodyState;
                sendJson(req, 400, "{\"error\":\"bad JSON\"}");
                return;
            }
            delete bodyState;

            JsonDocument sanitizedDoc;
            if (!validateAndCopySounds(doc, sanitizedDoc)) {
                sendJson(req, 400, "{\"error\":\"invalid sounds payload\"}");
                return;
            }

            File f = LittleFS.open("/sounds.json", "w");
            if (!f) { sendJson(req, 500, "{\"error\":\"fs write failed\"}"); return; }
            serializeJson(sanitizedDoc, f);
            f.close();
            sendJson(req, 200, buildSoundsJson());
        },
        nullptr,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            collectBody(req, data, len, index, total);
        }
    );

    // --- POST /api/wifi/reset  (forget saved credentials, reboot into config portal) ---
    sServer.on("/api/wifi/reset", HTTP_POST, [](AsyncWebServerRequest* req) {
        sendJson(req, 200, "{\"ok\":true,\"reboot\":true}");
        setPendingAction(PA_WIFI_RESET);  // deferred: execute in webServerLoop()
    });

    // --- GET /api/sysinfo ---
    sServer.on("/api/sysinfo", HTTP_GET, [](AsyncWebServerRequest* req) {
        sendJson(req, 200, buildSysJson());
    });

    // --- POST /api/system/reboot ---
    sServer.on("/api/system/reboot", HTTP_POST, [](AsyncWebServerRequest* req) {
        sendJson(req, 200, "{\"ok\":true}");
        setPendingAction(PA_REBOOT);  // deferred: execute in webServerLoop()
    });

    // --- POST /api/system/format ---
    sServer.on("/api/system/format", HTTP_POST, [](AsyncWebServerRequest* req) {
        sendJson(req, 200, "{\"ok\":true}");
        setPendingAction(PA_FORMAT);  // deferred: format + reboot in webServerLoop()
    });

    // --- Game control ---
    sServer.on("/api/game/start",     HTTP_POST, [](AsyncWebServerRequest* req) { gameRemoteStart();     sendJson(req, 200, "{\"ok\":true}"); });
    sServer.on("/api/game/reset",     HTTP_POST, [](AsyncWebServerRequest* req) { gameRemoteReset();     sendJson(req, 200, "{\"ok\":true}"); });
    sServer.on("/api/game/simwire",   HTTP_POST, [](AsyncWebServerRequest* req) { gameRemoteSimWire();   sendJson(req, 200, "{\"ok\":true}"); });
    sServer.on("/api/game/simfinish", HTTP_POST, [](AsyncWebServerRequest* req) { gameRemoteSimFinish(); sendJson(req, 200, "{\"ok\":true}"); });

    sServer.begin();
    Serial.println(F("[WEB] Server started on port 80"));
}

void webServerLoop() {
    sWs.cleanupClients();
    unsigned long now = millis();
    if (now - sLastSysInfoBroadcast >= 3000UL) {
        sLastSysInfoBroadcast = now;
        if (sWs.count() > 0) {
            wsBroadcastSysInfo();
        }
    }
    // Execute deferred actions here — safe in main loop context, not interrupt context
    noInterrupts();
    PendingAction action = (PendingAction)sPendingAction;
    sPendingAction = PA_NONE;
    interrupts();
    if (action == PA_REBOOT)     { delay(100); ESP.restart(); }
    if (action == PA_WIFI_RESET) { wifiReset(); }
    if (action == PA_FORMAT)     { LittleFS.format(); delay(100); ESP.restart(); }
}
