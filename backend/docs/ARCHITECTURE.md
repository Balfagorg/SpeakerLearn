# SpeakerLearn Backend — Architecture

## Framework & Stack

| Layer | Technology |
|-------|------------|
| **Runtime** | C++20 native executable (`speakerlearn_serverd`) |
| **HTTP** | cpp-httplib (header-only, single-threaded) |
| **Serialization** | nlohmann/json |
| **Persistence** | SQLite3 (embedded, WAL mode) |
| **DSP** | Custom pipeline + optional `dsp_engine` shared library |

---

## Component Diagram

```
┌──────────────────────────────────────────────────────────────────────────┐
│                         speakerlearn_serverd                              │
│                              (main.cpp)                                   │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌─────────────────────────────────────────────────────────────────────┐ │
│  │  api::HttpServer                                                     │ │
│  │  • CORS pre-routing                                                  │ │
│  │  • Routes: /, /audio/optimize, /speaker-systems/*, /preferences/*,   │
│  │    /devices/detect                                                    │ │
│  │  • JSON request parsing / response serialization                     │ │
│  └───────────────────────────────┬─────────────────────────────────────┘ │
│                                  │                                        │
│                                  ▼                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐ │
│  │  core::AudioPipeline                                                 │ │
│  │  • 8-stage processing chain                                          │ │
│  │  • Orchestrates: SourceDetector → SpectrumAnalyzer → TrackProfiler   │ │
│  │    → PreferenceEngine → SpeakerModel → EQOptimizer → DSP → Limiter   │ │
│  └───────────────────────────────┬─────────────────────────────────────┘ │
│                                  │                                        │
│          ┌───────────────────────┼───────────────────────┐                │
│          ▼                       ▼                       ▼                │
│  ┌───────────────┐     ┌─────────────────┐     ┌─────────────────┐        │
│  │ core::stages  │     │ db::Database    │     │ dsp_engine      │        │
│  │ • SourceDet   │     │ Manager         │     │ (optional)      │        │
│  │ • Spectrum    │     │ • SQLite CRUD   │     │ Biquad, Comp,   │        │
│  │ • TrackProf   │     │ • Models        │     │ Limiter         │        │
│  │ • PrefEngine  │     │ • Singleton     │     │ C API for       │        │
│  │ • SpeakerMod  │     └─────────────────┘     │ Python ctypes   │        │
│  │ • EQOptimizer │                              └─────────────────┘        │
│  │ • SafetyLimit │                                                         │
│  └───────────────┘                                                         │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

---

## Data Flow

### Audio Optimization Request

```
Client POST /audio/optimize
    │
    ▼
HttpServer::setup_routes → lambda parses JSON
    │
    ▼
AudioPipeline::run(samples, preferences, speaker_capability, platform_hint)
    │
    ├─► SourceDetector::detect(platform_hint)     → SourceInfo
    ├─► SpectrumAnalyzer::analyze(samples)       → Spectrum
    ├─► TrackProfiler::profile(spectrum)          → TrackProfile
    ├─► PreferenceEngine::build_target_curve()     → TargetEQCurve
    ├─► SpeakerModel::load_constraints()         → SpeakerConstraints
    ├─► EQOptimizer::compute(...)                → final_eq map
    ├─► (DSP stub / dsp_engine)                  → processed_samples
    └─► SafetyLimiter::limit()                   → headroom, warnings
    │
    ▼
JSON response { final_eq, headroom_db, track_profile, warnings }
```

---

## Directory Layout

```
backend/
├── src/
│   ├── main.cpp              # Entry: DB init, HttpServer(8001), server.start()
│   ├── api/
│   │   ├── HttpServer.cpp    # Route handlers, CORS
│   │   └── HttpServer.h
│   ├── db/
│   │   ├── DatabaseManager.cpp
│   │   ├── DatabaseManager.h
│   │   └── Models.h          # User, SpeakerSystem, SpeakerCapability, etc.
│   └── core/
│       ├── AudioPipeline.cpp
│       ├── AudioPipeline.h
│       └── stages/
│           ├── Stages.cpp
│           └── Stages.h
├── dsp_engine/
│   └── engine.cpp            # C API: init_dsp, configure_eq, process_audio_interleaved
├── docs/
│   ├── API.md
│   └── ARCHITECTURE.md
├── CMakeLists.txt
└── README.md
```

---

## Build Outputs

| Target | Type | Description |
|--------|------|-------------|
| `speakerlearn_core` | Static library | Core logic (api, db, core) — linkable into apps/tests |
| `speakerlearn_serverd` | Executable | Desktop HTTP server |

---

## Concurrency Model

- **HTTP:** cpp-httplib uses a single-threaded blocking server. One request at a time.
- **Database:** `DatabaseManager` uses a `std::mutex` for thread-safe access (future multi-threaded use).
- **DSP:** Pipeline stages are stateless per call; no shared mutable state.

---

## Extensibility

1. **New API routes:** Add handlers in `HttpServer::setup_routes()`.
2. **New pipeline stages:** Implement in `core/stages/`, wire in `AudioPipeline::run()`.
3. **Platform-specific code:** Add `src/platform/{windows,macos,linux}/*.cpp` — CMake auto-includes.
4. **DSP integration:** Link `dsp_engine` and replace pass-through in `AudioPipeline` with `process_audio_interleaved()`.
