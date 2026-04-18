#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "webserver.h"
#include "game_config.h"
#include "game.h"
#include "scoreboard.h"
#include "wifi_manager.h"
#include "promode.h"

static AsyncWebServer  sServer(80);
static AsyncWebSocket  sWs("/ws");

// ── Deferred action flags ─────────────────────────────────────────────────────
// Handlers run in the network interrupt context — blocking calls (delay, LittleFS.format,
// ESP.restart) must be deferred to webServerLoop() which runs in the main loop context.
enum PendingAction { PA_NONE, PA_REBOOT, PA_WIFI_RESET, PA_FORMAT };
static PendingAction sPendingAction = PA_NONE;

// ── JSON Builders ─────────────────────────────────────────────────────────────
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
    doc["rssi"]       = WiFi.RSSI();
    doc["wsClients"]  = sWs.count();
    String out;
    serializeJson(doc, out);
    return out;
}

// ── WebSocket broadcasts ──────────────────────────────────────────────────────
static String wrapEvent(const char* event, const String& data) {
    JsonDocument doc;
    doc["event"] = event;
    JsonDocument payload;
    deserializeJson(payload, data);
    doc["data"]  = payload;
    String out;
    serializeJson(doc, out);
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

// Apply a JSON body to cfg fields (only keys present are updated)
static bool applyConfigJson(const String& body) {
    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) return false;
    cfg.proModeEnabled    = doc["proModeEnabled"]    | cfg.proModeEnabled;
    cfg.proModeSensor     = doc["proModeSensor"]     | cfg.proModeSensor;
    cfg.proGreenMin       = doc["proGreenMin"]       | cfg.proGreenMin;
    cfg.proGreenMax       = doc["proGreenMax"]       | cfg.proGreenMax;
    cfg.proRedMin         = doc["proRedMin"]         | cfg.proRedMin;
    cfg.proRedMax         = doc["proRedMax"]         | cfg.proRedMax;
    cfg.irMoveThreshold   = doc["irMoveThreshold"]   | cfg.irMoveThreshold;
    cfg.debounceMs        = doc["debounceMs"]        | cfg.debounceMs;
    cfg.stripBrightness   = doc["stripBrightness"]   | cfg.stripBrightness;
    cfg.matrixBrightness  = doc["matrixBrightness"]  | cfg.matrixBrightness;
    cfg.failDisplayMs     = doc["failDisplayMs"]     | cfg.failDisplayMs;
    cfg.winDisplayMs      = doc["winDisplayMs"]      | cfg.winDisplayMs;
    cfg.countdownStepMs   = doc["countdownStepMs"]   | cfg.countdownStepMs;
    cfg.strobeIntervalMs  = doc["strobeIntervalMs"]  | cfg.strobeIntervalMs;
    cfg.strobeFlashCount  = doc["strobeFlashCount"]  | cfg.strobeFlashCount;
    cfg.scrollIdleMs      = doc["scrollIdleMs"]      | cfg.scrollIdleMs;
    cfg.scrollCountdownMs = doc["scrollCountdownMs"] | cfg.scrollCountdownMs;
    cfg.scrollFailMs      = doc["scrollFailMs"]      | cfg.scrollFailMs;
    cfg.scrollWinMs       = doc["scrollWinMs"]       | cfg.scrollWinMs;
    return true;
}

// ── Body accumulator for POST requests ───────────────────────────────────────
// ESPAsyncWebServer delivers body in chunks; we accumulate in a static String.
// This is safe for single-client REST usage on an ESP8266.
struct BodyRequest {
    AsyncWebServerRequest* req;
    String                 body;
};
static BodyRequest sPending = { nullptr, "" };

static constexpr size_t MAX_BODY_BYTES = 2048;

static void collectBody(AsyncWebServerRequest* req,
                        uint8_t* data, size_t len, size_t index, size_t total) {
    if (total > MAX_BODY_BYTES) {
        req->send(413, "application/json", "{\"error\":\"payload too large\"}");
        return;
    }
    if (index == 0) {
        sPending.req  = req;
        sPending.body = "";
        sPending.body.reserve(total);
    }
    sPending.body.concat((char*)data, len);
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
            if (!applyConfigJson(sPending.body)) {
                sendJson(req, 400, "{\"error\":\"bad JSON\"}");
                return;
            }
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
            JsonDocument doc;
            if (deserializeJson(doc, sPending.body) != DeserializationError::Ok) {
                sendJson(req, 400, "{\"error\":\"bad JSON\"}");
                return;
            }
            File f = LittleFS.open("/sounds.json", "w");
            if (!f) { sendJson(req, 500, "{\"error\":\"fs write failed\"}"); return; }
            serializeJson(doc, f);
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
        sPendingAction = PA_WIFI_RESET;  // deferred: execute in webServerLoop()
    });

    // --- GET /api/sysinfo ---
    sServer.on("/api/sysinfo", HTTP_GET, [](AsyncWebServerRequest* req) {
        sendJson(req, 200, buildSysJson());
    });

    // --- POST /api/system/reboot ---
    sServer.on("/api/system/reboot", HTTP_POST, [](AsyncWebServerRequest* req) {
        sendJson(req, 200, "{\"ok\":true}");
        sPendingAction = PA_REBOOT;  // deferred: execute in webServerLoop()
    });

    // --- POST /api/system/format ---
    sServer.on("/api/system/format", HTTP_POST, [](AsyncWebServerRequest* req) {
        sendJson(req, 200, "{\"ok\":true}");
        sPendingAction = PA_FORMAT;  // deferred: format + reboot in webServerLoop()
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
    // Execute deferred actions here — safe in main loop context, not interrupt context
    if (sPendingAction != PA_NONE) {
        PendingAction action = sPendingAction;
        sPendingAction = PA_NONE;
        if (action == PA_REBOOT)     { delay(100); ESP.restart(); }
        if (action == PA_WIFI_RESET) { wifiReset(); }
        if (action == PA_FORMAT)     { LittleFS.format(); delay(100); ESP.restart(); }
    }
}
