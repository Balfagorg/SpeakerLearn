# SpeakerLearn Backend — Adaptive Multi-System Audio Balancing Platform

A **C++20 native backend** for dynamically optimizing audio playback across different songs, streaming platforms, and speaker systems — while protecting hardware from distortion or overload.

> **Status:** Initial framework / prototype. Designed for expansion into a production-grade DSP system.

---

## Tech Stack

| Component | Technology |
|-----------|------------|
| **Language** | C++20 |
| **Build System** | CMake 3.20+ |
| **HTTP Server** | cpp-httplib v0.15.3 |
| **JSON** | nlohmann/json v3.11.3 |
| **Database** | SQLite3 (amalgamation) |
| **DSP Engine** | C++ shared library (`dsp_engine/`) — C API for Python ctypes |

---

## Quick Start

### Prerequisites

- CMake 3.20 or later
- C++20-capable compiler (MSVC 2019+, GCC 10+, Clang 10+)

### Build

```bash
cd backend
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Run

```bash
# From build directory
./speakerlearn_serverd   # Linux/macOS
# or
Release\speakerlearn_serverd.exe   # Windows
```

The server starts on **http://0.0.0.0:8001** (port 8001 to avoid conflicts with Python services on 8000).

---

## Project Structure

```
backend/
│
├── src/
│   ├── main.cpp                 # Entry point — DB init, HTTP server
│   │
│   ├── api/                     # HTTP API layer
│   │   ├── HttpServer.cpp       # Route registration, CORS, handlers
│   │   └── HttpServer.h
│   │
│   ├── db/                      # Database layer
│   │   ├── DatabaseManager.cpp  # SQLite connection, migrations, CRUD
│   │   ├── DatabaseManager.h
│   │   └── Models.h            # ORM-like structs (User, SpeakerSystem, etc.)
│   │
│   ├── core/                    # Processing pipeline
│   │   ├── AudioPipeline.cpp    # 8-stage orchestrator
│   │   ├── AudioPipeline.h
│   │   └── stages/
│   │       ├── Stages.cpp       # SourceDetector, SpectrumAnalyzer, etc.
│   │       └── Stages.h
│   │
│   └── platform/                # Platform-specific code (optional)
│       ├── windows/
│       ├── macos/
│       └── linux/
│
├── dsp_engine/                  # Standalone DSP library (C API)
│   └── engine.cpp              # Biquad EQ, Exciter, Compressor, Limiter
│
├── docs/
│   ├── API.md                # Full API reference
│   └── ARCHITECTURE.md        # Framework & component diagram
├── CMakeLists.txt
├── README.md
└── speakerlearn_framework_plan.md
```

---

## API Reference

Base URL: `http://localhost:8001`

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Health check — service info and status |
| `/audio/optimize` | POST | Run the full 8-stage optimization pipeline |
| `/speaker-systems/create` | POST | Create a speaker system (persists to DB) |
| `/speaker-systems/list` | GET | List speaker systems for a user |
| `/preferences` | GET | Get user preferences |
| `/preferences/update` | POST | Update user preferences |
| `/devices/detect` | GET | Detect audio devices (stub) |
| `/speaker-systems/calibrate` | POST | Submit calibration ratings (Learn flow) |
| `/speaker-systems/switch` | POST | Set active speaker system |
| `/eq-presets/list` | GET | List EQ presets for user |
| `/eq-presets/save` | POST | Save EQ preset |
| `/eq-presets/delete` | POST | Delete EQ preset (body: `{ "id": "..." }`) |

### `GET /`

**Response:**
```json
{
  "service": "Adaptive Audio Balancing Platform (C++ Native)",
  "version": "0.1.0",
  "status": "online"
}
```

### `POST /audio/optimize`

**Request body (JSON):**
```json
{
  "samples": [0.1, -0.1, 0.2, -0.2],
  "preferences": { "bass_boost": 0.5 },
  "platform_hint": "spotify"
}
```

**Response:**
```json
{
  "final_eq": { "sub_bass": 0.0, "bass": 0.0, "low_mid": 0.0, "mid": 0.0, "high": 0.0, "air": 0.0 },
  "headroom_db": 3.0,
  "peak_limit_db": -1.0,
  "track_profile": { "avg_loudness": -14.0, "dynamic_range": 8.0 },
  "warnings": []
}
```

### `POST /speaker-systems/create`

**Request body:**
```json
{
  "name": "Living Room Speakers",
  "user_id": "default-user",
  "device_identifier": "optional-device-id",
  "channel_config": "stereo"
}
```

**Response:** Persists to SQLite and returns created ID.

### `GET /speaker-systems/list?user_id=default-user`

**Response:** Array of speaker systems.

### `GET /preferences?user_id=default-user`

**Response:** Object of preference keys and weights.

### `POST /preferences/update`

**Request body:**
```json
{
  "user_id": "default-user",
  "preferences": { "bass_boost": 0.5, "treble": 0.3 }
}
```

### `GET /devices/detect`

**Response:** Array of detected audio devices (stub).

---

## Processing Pipeline

Audio flows through 8 independent stages:

```
Audio Input → Source Detection → Spectrum Analysis → Track Profiling
    → User Preference Model → Speaker Model → EQ Optimizer
    → DSP Processing → Limiter / Safety → Speaker Output
```

**EQ Formula:**
```
Final EQ = (User Preference + Source Correction − Track Bias) × Speaker Capability
```

| Stage | Class | Purpose |
|-------|-------|---------|
| 1 | `SourceDetector` | Platform hint → source profile (Spotify, Apple Music, etc.) |
| 2 | `SpectrumAnalyzer` | FFT spectral analysis → band energies |
| 3 | `TrackProfiler` | Track classification (loudness, dynamic range) |
| 4 | `PreferenceEngine` | User preferences → target EQ curve |
| 5 | `SpeakerModel` | Speaker capability constraints |
| 6 | `EQOptimizer` | Compute final EQ from curve + constraints |
| 7 | DSP Engine | Multiband EQ, compression (stub / pass-through) |
| 8 | `SafetyLimiter` | Brickwall limiting, headroom check |

---

## Database Schema (SQLite)

Database file: `adaptive_audio_cpp.db` (created on first run)

| Table | Purpose |
|-------|---------|
| `users` | Application users |
| `speaker_systems` | Named speaker configurations with device mapping |
| `speaker_capabilities` | Per-band frequency handling limits |
| `calibration_results` | Raw calibration test ratings |
| `user_preferences` | Listening preference weights |
| `source_profiles` | Per-platform audio correction offsets |

Tables are auto-created on startup. Default source profiles (Spotify, Apple Music, YouTube, Local FLAC) are seeded automatically.

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                     speakerlearn_serverd                         │
├─────────────────────────────────────────────────────────────────┤
│  HTTP Layer (cpp-httplib)                                        │
│  ├── CORS pre-routing                                           │
│  └── JSON request/response                                      │
├─────────────────────────────────────────────────────────────────┤
│  API Handlers → AudioPipeline → Stages (SourceDetector, etc.)     │
├─────────────────────────────────────────────────────────────────┤
│  DatabaseManager (Singleton)                                     │
│  └── SQLite3 — speaker_systems, user_preferences, etc.          │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│  dsp_engine (separate shared library)                            │
│  C API: init_dsp, configure_eq, process_audio_interleaved         │
│  Used by Python via ctypes for real-time audio processing        │
└─────────────────────────────────────────────────────────────────┘
```

---

## DSP Engine (`dsp_engine/`)

The `dsp_engine` is a standalone C++ library exposing a C API for Python (ctypes) integration:

- **Biquad** — Parametric peaking EQ
- **Exciter** — Harmonic saturation for sub-bass
- **Compressor** — Soft-knee RMS dynamics
- **LookaheadLimiter** — Brickwall output protection

The main C++ backend uses in-process stubs for the pipeline; full DSP integration can link this library directly or via FFI.

---

## Frontend Integration

### Key Endpoints for UI

- **Speaker Setup:** `POST /speaker-systems/create` (stub — extend for full CRUD)
- **Audio Optimization:** `POST /audio/optimize` — pass samples, preferences, platform

### CORS

All origins, methods, and headers are allowed for development. Restrict in production.

---

## Frontend Integration

The HTML pages (`homePage.html`, `addSpeakerPage.html`, `preferencesPage.html`) use `js/api.js` to sync with the backend. All operations dual-write to localStorage and the API when available, preserving full functionality offline.
