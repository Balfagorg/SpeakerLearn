# SpeakerLearn Backend Architecture

## Stack

| Layer | Tech |
|-------|------|
| Runtime | C++20 (`speakerlearn_serverd`) |
| HTTP | cpp-httplib (single-threaded) |
| JSON | nlohmann/json |
| DB | SQLite3 (WAL mode) |
| DSP | Custom pipeline + optional `dsp_engine` |

---

## Component Diagram

```
speakerlearn_serverd (main.cpp)
├── HttpServer — CORS, routes, JSON parse/serialize
├── AudioPipeline — 8-stage chain
│   ├── SourceDetector → SpectrumAnalyzer → TrackProfiler
│   ├── PreferenceEngine → SpeakerModel → EQOptimizer
│   └── DSP → SafetyLimiter
├── DatabaseManager (Singleton) — SQLite CRUD
└── dsp_engine (optional) — Biquad, Compressor, Limiter (C API)
```

---

## Data Flow

```
POST /audio/optimize
  → HttpServer parses JSON
  → AudioPipeline::run(samples, preferences, speaker_calibration, platform_hint)
  → SourceDetector → Spectrum → TrackProfile → PreferenceEngine → SpeakerModel
  → EQOptimizer → DSP → SafetyLimiter
  → JSON { final_eq, headroom_db, track_profile, warnings }
```

---

## Directory Layout

```
backend/
├── src/
│   ├── main.cpp
│   ├── api/HttpServer.{cpp,h}
│   ├── db/DatabaseManager.{cpp,h}, Models.h
│   └── core/AudioPipeline.{cpp,h}, stages/{Stages.cpp, Stages.h}
├── dsp_engine/engine.cpp
├── docs/API.md, ARCHITECTURE.md
└── CMakeLists.txt
```

---

## Pipeline Stages

| Stage | Purpose |
|-------|---------|
| SourceDetector | Platform hint → source profile |
| SpectrumAnalyzer | FFT → band energies |
| TrackProfiler | Loudness, dynamic range |
| PreferenceEngine | User prefs → target curve |
| SpeakerModel | Speaker constraints |
| EQOptimizer | Final EQ from curve + constraints |
| DSP | Multiband EQ (stub/pass-through) |
| SafetyLimiter | Brickwall, headroom |

---

## Concurrency

- HTTP: single-threaded, one request at a time
- DB: `std::mutex` for thread-safe access
- Pipeline stages: stateless per call
