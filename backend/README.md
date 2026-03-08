# SpeakerLearn Backend

C++20 native HTTP API for audio optimization and speaker management.

---

## Build

```bash
cd backend
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

**Run:** `./speakerlearn_serverd` (or `Release\speakerlearn_serverd.exe`)

Server: **http://localhost:8001**

---

## Stack

| Component | Tech |
|-----------|------|
| HTTP | cpp-httplib |
| JSON | nlohmann/json |
| DB | SQLite3 |
| DSP | Custom 8-stage pipeline |

---

## Structure

```
backend/
├── src/          # api, db, core (pipeline, stages)
├── dsp_engine/   # C API for real-time DSP
└── docs/
    ├── API.md
    └── ARCHITECTURE.md
```

---

## Pipeline

```
SourceDetector → SpectrumAnalyzer → TrackProfiler → PreferenceEngine
→ SpeakerModel → EQOptimizer → DSP → SafetyLimiter
```

---

## Docs

- [API.md](docs/API.md) — Endpoint reference
- [ARCHITECTURE.md](docs/ARCHITECTURE.md) — Component diagram, data flow
