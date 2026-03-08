# SpeakEasy — System Overview

How the SpeakEasy (SpeakerLearn) adaptive audio platform works end-to-end.

---

## 1. Startup Flow

```
run.sh / run.bat  →  run.js
```

**run.js** orchestrates everything:

1. **Find or build C++ backend** — Looks for `speakerlearn_serverd` in `backend/build/` (or `backend/Mac OS/build/` on macOS). If missing, runs `cmake` configure + build.
2. **Spawn backend process** — Runs the C++ server on port 8001.
3. **Start HTTP server** — Listens on port 8080:
   - Serves static HTML/JS (homePage, addSpeaker, preferences, playMusic)
   - Proxies `/bridge/*` to the audioBridge module (if `npm install` was run)
4. **Shutdown** — On Ctrl+C, kills the backend and closes the server.

---

## 2. Two Servers

| Port | Role | Technology |
|------|------|------------|
| **8080** | Frontend + Bridge API | Node.js `http` (run.js) |
| **8001** | Audio API (optimize, speakers, presets) | C++ `speakerlearn_serverd` |

The frontend (`api.js`) calls both: backend on 8001, bridge on 8080.

---

## 3. C++ Backend (speakerlearn_serverd)

```
main.cpp
  → init SQLite DB (adaptive_audio_cpp.db)
  → HttpServer(8001).start()
```

**HttpServer** exposes:

- **POST /audio/optimize** — Core path: samples + preferences → EQ curve
- **Speaker systems** — create, list, calibrate, switch
- **EQ presets** — list, save, delete
- **Preferences** — user preference weights

See [backend/docs/API.md](backend/docs/API.md) for the full API.

---

## 4. Audio Optimization Pipeline

```
POST /audio/optimize
  Body: { samples, preferences, platform_hint, speaker_calibration, speaker_system_id }
    ↓
  SourceDetector      → platform_hint → source profile (e.g. Spotify vs local)
  SpectrumAnalyzer    → FFT → band energies
  TrackProfiler       → loudness, dynamic range
  PreferenceEngine    → user prefs → target curve
  SpeakerModel        → speaker constraints
  EQOptimizer         → final 7-band EQ
  DSP                 → multiband EQ (stub)
  SafetyLimiter       → brickwall, headroom
    ↓
  Response: { final_eq, headroom_db, track_profile, warnings }
```

See [backend/docs/ARCHITECTURE.md](backend/docs/ARCHITECTURE.md) for pipeline details.

---

## 5. SpeakEasy Bridge (System-Wide EQ)

**Purpose:** Route system audio through SpeakEasy's EQ so any app (Spotify, YouTube, games) uses your calibrated curve.

```
[Apps] → [Virtual Cable / BlackHole] → [Bridge Input]
                                          ↓
                                    biquadEq (real-time EQ)
                                          ↓
                                    [Bridge Output] → [Speakers]
```

**Flow:**

1. **naudiodon** (PortAudio) opens input (e.g. CABLE Output on Windows, BlackHole on macOS) and output (speakers).
2. Input stream → `biquadEq` transform → output stream.
3. `biquadEq.js` applies 7-band EQ in real time.
4. Frontend calls `/bridge/start`, `/bridge/stop`, `/bridge/eq` on port 8080.

See [BRIDGE.md](BRIDGE.md) for setup.

---

## 6. User Flow (Frontend Pages)

| Page | Purpose |
|------|---------|
| **homePage** | Landing, mission, 3D speaker demo |
| **addSpeakerPage** | **Learn** (5 test tones) or **Know** (manual 7-band) calibration |
| **preferencesPage** | Select speaker, tweak EQ (±16 dB), save presets |
| **playMusic** | Play with EQ correction, or start Bridge for system-wide EQ |

---

## 7. Data Flow Summary

```
Browser (homePage, playMusic, etc.)
    │
    ├──→ localhost:8001  (C++ backend)
    │         │
    │         ├── DatabaseManager (SQLite)
    │         └── AudioPipeline (8 stages)
    │
    └──→ localhost:8080  (Node.js)
              │
              ├── Static files (HTML, JS, CSS)
              └── /bridge/* → audioBridge (naudiodon + biquadEq)
```

---

## 8. Dependencies

| Component | Depends On |
|-----------|------------|
| **run.js** | Node.js (built-in modules only) |
| **Bridge** | `naudiodon` (PortAudio) via `npm install` |
| **Backend** | CMake 3.20+, C++20 compiler, FetchContent (cpp-httplib, nlohmann/json, sqlite-amalgamation) |
| **Frontend** | No build step; static HTML/JS |

See [docs/INSTALL.md](INSTALL.md) for installation on Windows and macOS.
