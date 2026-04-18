---
goal: Add WiFi web interface to ESP Buzzwire for config, debug, sound playback, scoreboard, OTA updates and remote game control
version: 1.0
date_created: 2026-04-18
last_updated: 2026-04-18
owner: tamaygz
status: 'Planned'
tags: [feature, web, wifi, ui]
---

![Status: Planned](https://img.shields.io/badge/status-Planned-blue)

# Feature — Web Interface for ESP Buzzwire

## Section 1 — Requirements & Constraints

### Functional Requirements

- **REQ-001**: WiFi operates in STA mode (connects to home WiFi). Falls back to AP mode (`Buzzwire-Game`) after 3 failed connection attempts or when no credentials are stored.
- **REQ-002**: On first boot (no stored credentials), ESP starts a captive portal in AP mode for the user to enter WiFi SSID + password.
- **REQ-003**: All `config.h` values are editable from the web UI: pro mode timings (green/red min/max), sensor threshold, debounce, fail/win display durations, countdown step, LED brightness, strip count, scroll speeds, sensor selection, pro mode enable/disable.
- **REQ-004**: Changed config values persist across reboots (stored in LittleFS as JSON).
- **REQ-005**: Real-time debug view shows game state transitions and pro mode phase changes via WebSocket.
- **REQ-006**: Sound playback via browser using remote URLs configured per event in the web UI. No local file storage.
- **REQ-007**: Sound events: fail (wire touch), win (finish reached), countdown beeps (3, 2, 1, GO), pro mode red start, pro mode green start, game start (touch start pad).
- **REQ-008**: Anonymous scoreboard storing top 10 scores (time + fail count), persisted in LittleFS.
- **REQ-009**: No authentication required — anyone on the network can access all pages.
- **REQ-010**: OTA firmware update from browser (no physical button confirmation required).
- **REQ-011**: WiFi settings page to change SSID/password from the UI. Changes saved to LittleFS, reconnect on save.
- **REQ-012**: Reset-to-defaults button restores all config values to compile-time defaults.
- **REQ-013**: Game remote control: start game, reset/stop current game, toggle pro mode on/off, simulate wire touch, simulate finish (for testing).

### Technical Constraints

- **CON-001**: ESP8266 has ~80 KB RAM. Web server + WebSocket must stay under ~15 KB additional heap usage.
- **CON-002**: LittleFS partition must fit: config JSON (~1 KB), scoreboard JSON (~1 KB), WiFi credentials (~128 bytes), sound URL config (~512 bytes), UI files (gzipped HTML/JS/CSS).
- **CON-003**: Max 2 concurrent WebSocket clients to avoid heap exhaustion.
- **CON-004**: UI built with vanilla HTML/CSS/JS — no frameworks. All assets gzipped and served from LittleFS.
- **CON-005**: Desktop/laptop is primary target device for UI layout.
- **CON-006**: Game loop must remain non-blocking. All web server operations are async.
- **CON-007**: `FastLED.show()` disables interrupts for ~3ms (94 LEDs). WiFi stack tolerates this but WebSocket messages should not be sent during LED refresh.

### Guidelines

- **GUD-001**: Use `ESPAsyncWebServer` + `ESPAsyncTCP` for non-blocking HTTP + WebSocket.
- **GUD-002**: Use `ArduinoJson` for config/scoreboard serialization.
- **GUD-003**: Keep firmware-side WebSocket messages as compact JSON events: `{"event":"state","data":"PLAYING"}`.
- **GUD-004**: Sound config endpoint validates URLs are non-empty strings before saving.
- **GUD-005**: WiFi module should be fully decoupled from game logic — game.cpp emits events, web module subscribes.

### Patterns

- **PAT-001**: Event callback pattern — game.cpp calls a registered callback on state change; web module registers its WebSocket broadcast as the callback.
- **PAT-002**: Config struct pattern — single `GameConfig` struct loaded from JSON at boot, written back on save. Runtime code reads from struct, not `#define` constants (for web-editable values).
- **PAT-003**: Captive portal pattern — DNS server redirects all domains to ESP IP when in AP mode.

## Section 2 — Implementation Steps

### Implementation Phase 1 — WiFi & Captive Portal

- **GOAL-001**: ESP connects to stored WiFi or launches captive portal AP for credential entry.

| Task     | Description                                                                 | Completed | Date |
|----------|-----------------------------------------------------------------------------|-----------|------|
| TASK-001 | Create `src/wifi_manager.h/.cpp` — STA connect with 3-retry fallback to AP mode | | |
| TASK-002 | Add LittleFS init in `main.cpp` setup; store/load WiFi credentials as `/wifi.json` | | |
| TASK-003 | Implement captive portal with `DNSServer` — redirect all DNS to ESP IP in AP mode | | |
| TASK-004 | Create `data/wifi.html` — minimal page: SSID dropdown (scan), password field, save button | | |
| TASK-005 | Add `platformio.ini` changes: `lib_deps` for `ESPAsyncWebServer`, `ESPAsyncTCP`, `ArduinoJson`; `board_build.filesystem = littlefs` | | |
| TASK-006 | Serial log: print assigned IP (STA) or AP IP (192.168.4.1) at boot | | |

### Implementation Phase 2 — Runtime Config System

- **GOAL-002**: All game parameters are editable at runtime via a `GameConfig` struct, persisted as JSON.

| Task     | Description                                                                 | Completed | Date |
|----------|-----------------------------------------------------------------------------|-----------|------|
| TASK-007 | Create `src/game_config.h/.cpp` — `GameConfig` struct with all editable fields, defaults from `config.h` | | |
| TASK-008 | Implement `configLoad()` (LittleFS JSON → struct) and `configSave()` (struct → JSON) | | |
| TASK-009 | Implement `configReset()` — restore compile-time defaults and save | | |
| TASK-010 | Refactor `game.cpp`, `promode.cpp`, `leds.cpp`, `matrix.cpp`, `sensors.cpp` to read from `GameConfig` struct instead of `#define` constants for web-editable values | | |
| TASK-011 | Keep `config.h` `#define`s as compile-time defaults; `GameConfig` uses them for initialization only | | |

### Implementation Phase 3 — Web Server & API

- **GOAL-003**: HTTP REST API serves config CRUD, scoreboard, sound management, OTA, and game remote control.

| Task     | Description                                                                 | Completed | Date |
|----------|-----------------------------------------------------------------------------|-----------|------|
| TASK-012 | Create `src/webserver.h/.cpp` — init `AsyncWebServer` on port 80, serve static files from LittleFS | | |
| TASK-013 | `GET /api/config` — return current `GameConfig` as JSON | | |
| TASK-014 | `POST /api/config` — update `GameConfig` fields from JSON body, validate ranges, save to LittleFS | | |
| TASK-015 | `POST /api/config/reset` — call `configReset()`, return new defaults | | |
| TASK-016 | `GET /api/scores` — return top 10 scoreboard as JSON array | | |
| TASK-017 | `POST /api/scores/clear` — wipe scoreboard file | | |
| TASK-018 | `GET /api/sounds` — return sound config JSON (remote URL per event) | | |
| TASK-019 | `POST /api/sounds` — save remote URLs per sound event to LittleFS | | |
| TASK-024 | `POST /api/game/start` — trigger game start (simulate start pad touch) | | |
| TASK-025 | `POST /api/game/reset` — reset/stop current game → IDLE | | |
| TASK-026 | `POST /api/game/promode` — toggle pro mode on/off at runtime | | |
| TASK-027 | `POST /api/game/simulate/wire` — simulate wire touch (testing) | | |
| TASK-028 | `POST /api/game/simulate/finish` — simulate finish touch (testing) | | |
| TASK-029 | `POST /api/wifi` — save new SSID/password, trigger reconnect | | |
| TASK-030 | `GET /api/wifi` — return current SSID (no password), connection status, IP, signal strength | | |
| TASK-031 | OTA endpoint via `AsyncElegantOTA` or manual `Update` class on `POST /api/ota` | | |
| TASK-032 | `GET /api/status` — return free heap, uptime, firmware version, LittleFS usage | | |

### Implementation Phase 4 — WebSocket Events

- **GOAL-004**: Browser receives real-time game state and pro mode phase transitions via WebSocket.

| Task     | Description                                                                 | Completed | Date |
|----------|-----------------------------------------------------------------------------|-----------|------|
| TASK-033 | Add WebSocket endpoint at `/ws` on the async server | | |
| TASK-034 | Create event callback system in `game.cpp` — `gameOnStateChange(callback)` fires on every `enterState()` | | |
| TASK-035 | Create event callback in `promode.cpp` — `promodeOnPhaseChange(callback)` fires on green↔red | | |
| TASK-036 | Register WebSocket broadcast as the callback in `webserver.cpp` | | |
| TASK-037 | WebSocket message format: `{"event":"state","state":"PLAYING","elapsed":1234,"fails":2}` | | |
| TASK-038 | WebSocket message for pro mode: `{"event":"phase","phase":"RED","duration":2500}` | | |
| TASK-039 | WebSocket message for sound trigger: `{"event":"sound","trigger":"fail"}` | | |
| TASK-040 | Limit to 2 concurrent WebSocket clients; reject additional connections with reason | | |

### Implementation Phase 5 — Scoreboard

- **GOAL-005**: Game wins are automatically recorded and retrievable via API.

| Task     | Description                                                                 | Completed | Date |
|----------|-----------------------------------------------------------------------------|-----------|------|
| TASK-041 | Create `src/scoreboard.h/.cpp` — `scoreboardAdd(elapsed, failCount)`, `scoreboardGet()`, `scoreboardClear()` | | |
| TASK-042 | Store as `/scores.json` in LittleFS — array of `{time, fails, date}` sorted by time ASC, max 10 entries | | |
| TASK-043 | Call `scoreboardAdd()` from `enterState(STATE_WIN)` in `game.cpp` | | |

### Implementation Phase 6 — Sound System

- **GOAL-006**: Browser plays sounds on game events using remote URLs configured in the web UI.

| Task     | Description                                                                 | Completed | Date |
|----------|-----------------------------------------------------------------------------|-----------|------|
| TASK-044 | Create `/sounds.json` in LittleFS — per-event URL map: `{"fail":"https://...","win":"https://...",...}` | | |
| TASK-045 | WebSocket `sound` events (TASK-039) trigger browser-side `Audio()` playback using configured URL | | |
| TASK-046 | Browser JS: on `sound` event, look up URL from cached config; skip playback if URL is empty | | |

### Implementation Phase 7 — Web UI (Multi-Page HTML/CSS/JS)

- **GOAL-007**: Multi-page web UI with shared header navbar, served gzipped from LittleFS, desktop-optimized.

Pages: **Game** (default) · **Config** · **Sounds** · **WiFi** · **System**

| Task     | Description                                                                 | Completed | Date |
|----------|-----------------------------------------------------------------------------|-----------|------|
| TASK-048 | Create shared `data/header.js` — reusable navbar injected into every page (Game, Config, Sounds, WiFi, System links); highlights active page | | |
| TASK-049 | Create `data/index.html` (Game page) — **Top section**: live game state badge (IDLE/COUNTDOWN/PLAYING/FAIL/WIN), elapsed timer, fail counter, remote control buttons (start, reset, toggle pro mode, simulate wire, simulate finish). **Pro mode section** (visible when pro mode active): virtual red/green LED indicators mirroring hardware LEDs, current phase label + duration countdown. **Bottom section**: leaderboard table (rank, time, fails, date) with clear button | | |
| TASK-050 | Create `data/config.html` — form with all `GameConfig` fields grouped by category (Timings, LEDs, Sensors, Pro Mode), save button, reset-to-defaults button, input validation (min/max ranges) | | |
| TASK-051 | Create `data/sounds.html` — per-event row with remote URL input and play/preview button; save button persists all URLs | | |
| TASK-052 | Create `data/wifi.html` — current connection info (SSID, IP, signal strength), form to change SSID/password | | |
| TASK-053 | Create `data/system.html` — firmware version, uptime, free heap, LittleFS usage, OTA firmware upload form | | |
| TASK-054 | Create shared `data/ws.js` — WebSocket client (auto-reconnect, parse events); on Game page: update state/timer/fails/LEDs and trigger sounds; on other pages: passive connection for sound triggers only | | |
| TASK-055 | Create shared `data/style.css` — common layout, navbar styling, game state badges, virtual LED circles (green/red glow), leaderboard table, form styling | | |
| TASK-056 | Gzip all static files for LittleFS deployment. Add PlatformIO `extra_scripts` or manual gzip step | | |

### Implementation Phase 8 — Integration & Testing

- **GOAL-008**: End-to-end verified: config changes apply to game, remote sounds play, scores record, OTA works.

| Task     | Description                                                                 | Completed | Date |
|----------|-----------------------------------------------------------------------------|-----------|------|
| TASK-057 | Verify captive portal flow: first boot → AP → enter credentials → connects to WiFi | | |
| TASK-058 | Verify config round-trip: change value in UI → save → reboot → value persists | | |
| TASK-059 | Verify WebSocket: open Game page → play game → state badge, timer, fails, virtual LEDs update in real-time | | |
| TASK-060 | Verify sounds: configure remote URL → trigger event → browser plays sound | | |
| TASK-061 | Verify leaderboard: win game → score appears in Game page leaderboard section | | |
| TASK-062 | Verify OTA: upload firmware binary → ESP reboots with new firmware | | |
| TASK-063 | Verify remote control: start/reset/simulate from dashboard → game responds correctly | | |
| TASK-064 | Heap check: run game with web UI open, confirm `ESP.getFreeHeap()` stays above 20 KB | | |
| TASK-065 | Build size check: flash usage under 80% with all UI files in LittleFS | | |

## Section 3 — Alternatives

- **ALT-001**: Use `ESP8266WebServer` (sync) instead of `ESPAsyncWebServer` — Rejected because synchronous server blocks game loop, causing LED flicker and missed sensor reads.
- **ALT-002**: Use EEPROM instead of LittleFS for config — Rejected because EEPROM has limited write cycles and no filesystem for sounds/UI files.
- **ALT-003**: Use MQTT for events instead of WebSocket — Rejected because it requires an external broker; WebSocket is self-contained.
- **ALT-004**: Use Preact/React for UI — Rejected per user preference; vanilla HTML/CSS/JS is smallest and has zero dependencies.
- **ALT-005**: Stream audio from ESP (I2S/DAC) — Rejected because ESP8266 has no DAC; browser-side playback is simpler and higher quality.

## Section 4 — Dependencies

- **DEP-001**: `ESPAsyncWebServer` — async HTTP server library for ESP8266
- **DEP-002**: `ESPAsyncTCP` — async TCP required by ESPAsyncWebServer
- **DEP-003**: `ArduinoJson` (v7) — JSON serialization/deserialization
- **DEP-004**: `LittleFS` — filesystem for config, scores, sounds, and UI files (built into ESP8266 Arduino core)
- **DEP-005**: `DNSServer` — captive portal DNS redirection (built into ESP8266 Arduino core)
- **DEP-006**: `ESP8266WiFi` — WiFi STA/AP management (built into ESP8266 Arduino core)
- **DEP-007**: `Update` — OTA firmware upload (built into ESP8266 Arduino core)
- **DEP-008**: `FastLED` ^3.6.0 — existing dependency (unchanged)

## Section 5 — Files

| ID       | Path                          | Action   | Description |
|----------|-------------------------------|----------|-------------|
| FILE-001 | `src/wifi_manager.h`          | created  | WiFi STA/AP management + captive portal header |
| FILE-002 | `src/wifi_manager.cpp`        | created  | WiFi connect, fallback, DNS, credentials load/save |
| FILE-003 | `src/game_config.h`           | created  | `GameConfig` struct definition + API |
| FILE-004 | `src/game_config.cpp`         | created  | Config load/save/reset from LittleFS JSON |
| FILE-005 | `src/webserver.h`             | created  | Web server + WebSocket init header |
| FILE-006 | `src/webserver.cpp`           | created  | All HTTP endpoints, WebSocket handler, event callbacks |
| FILE-007 | `src/scoreboard.h`            | created  | Scoreboard add/get/clear header |
| FILE-008 | `src/scoreboard.cpp`          | created  | Scoreboard LittleFS persistence |
| FILE-009 | `data/index.html`             | created  | Game page — state, virtual LEDs, controls, leaderboard |
| FILE-010 | `data/config.html`            | created  | Config page — all editable game settings |
| FILE-011a | `data/sounds.html`           | created  | Sounds page — remote URL config per event |
| FILE-012a | `data/wifi.html`             | created  | WiFi page — connection info + credential form (also captive portal) |
| FILE-013a | `data/system.html`           | created  | System page — status, OTA upload |
| FILE-014a | `data/header.js`             | created  | Shared navbar component injected into all pages |
| FILE-015a | `data/ws.js`                 | created  | Shared WebSocket client with auto-reconnect |
| FILE-016a | `data/style.css`             | created  | Shared styles — navbar, badges, virtual LEDs, tables, forms |
| FILE-011 | `src/config.h`                | modified | Defaults remain; add `#define` for AP SSID, firmware version, max scores |
| FILE-012 | `src/main.cpp`                | modified | Add LittleFS init, WiFi init, web server init in `setup()` |
| FILE-013 | `src/game.cpp`                | modified | Add event callback hooks, scoreboard call on WIN, remote control handlers |
| FILE-014 | `src/promode.cpp`             | modified | Add phase change callback hook; read durations from `GameConfig` |
| FILE-015 | `src/leds.cpp`                | modified | Read brightness/strip count from `GameConfig` |
| FILE-016 | `src/matrix.cpp`              | modified | Read scroll speeds from `GameConfig` |
| FILE-017 | `src/sensors.cpp`             | modified | Read threshold from `GameConfig` |
| FILE-018 | `platformio.ini`              | modified | Add lib_deps, filesystem config, LittleFS upload target |

## Section 6 — Testing

- **TEST-001**: First boot with no `/wifi.json` → ESP starts AP `Buzzwire-Game` → captive portal page loads at 192.168.4.1
- **TEST-002**: Enter valid WiFi credentials → ESP saves and reboots → connects to home WiFi → serial prints assigned IP
- **TEST-003**: 3 failed STA connect attempts → falls back to AP mode → captive portal accessible
- **TEST-004**: `GET /api/config` returns JSON with all `GameConfig` fields and current values
- **TEST-005**: `POST /api/config` with partial JSON → only specified fields update, others unchanged → values persist after reboot
- **TEST-006**: `POST /api/config/reset` → all values return to compile-time defaults
- **TEST-007**: WebSocket connection at `/ws` → receive `{"event":"state","state":"IDLE"}` immediately
- **TEST-008**: Play and win game → score appears in `GET /api/scores` → persists after reboot
- **TEST-009**: Set remote URL for fail sound via `POST /api/sounds` → URL persists after reboot → `GET /api/sounds` returns it
- **TEST-010**: Trigger win event with URL configured → browser plays remote sound; trigger with empty URL → no playback, no error
- **TEST-011**: `POST /api/game/start` from browser → game transitions from IDLE → COUNTDOWN
- **TEST-012**: `POST /api/game/simulate/wire` during PLAYING → game transitions to FAIL
- **TEST-013**: Upload firmware .bin via OTA → ESP reboots → new version reported in `/api/status`
- **TEST-014**: `ESP.getFreeHeap()` > 20,000 bytes with web UI open and game running
- **TEST-015**: Total flash usage (firmware + LittleFS) under 80% of 4MB flash

## Section 7 — Risks & Assumptions

- **RISK-001**: FastLED interrupt disable may cause occasional WebSocket frame drops — Mitigation: browser JS auto-reconnects; events are stateless snapshots, not deltas.
- **RISK-002**: LittleFS write during game could cause a brief stall (~5-10ms) — Mitigation: only write on explicit user actions (save config, upload sound), never during gameplay.
- **RISK-003**: Remote sound URLs may be slow or unavailable — Mitigation: browser `Audio()` handles network errors gracefully; playback is best-effort, does not block game.
- **RISK-004**: Captive portal may not trigger on all devices — Mitigation: always show ESP IP on serial output as fallback.
- **RISK-005**: OTA update without confirmation could brick if upload fails — Mitigation: ESP8266 `Update` class validates firmware before flashing; failed upload leaves old firmware intact.
- **ASSUMPTION-001**: User has a 4MB flash variant of D1 Mini (standard). 1MB variants would need reduced LittleFS partition.
- **ASSUMPTION-002**: Browser supports Web Audio API and WebSocket (all modern browsers do).
- **ASSUMPTION-003**: Home WiFi is 2.4 GHz (ESP8266 does not support 5 GHz).

## Section 8 — Related Specifications / Further Reading

- [`prd.md`](../prd.md) — Original product requirements document
- [`docs/wiring.md`](../docs/wiring.md) — Hardware pin assignments
- [`docs/software.md`](../docs/software.md) — Current software architecture
- [`docs/pro-mode.md`](../docs/pro-mode.md) — Pro Mode feature documentation
- [ESPAsyncWebServer docs](https://github.com/me-no-dev/ESPAsyncWebServer) — Library documentation
- [ArduinoJson v7](https://arduinojson.org/v7/) — JSON library documentation
- [LittleFS for ESP8266](https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html) — Filesystem documentation
