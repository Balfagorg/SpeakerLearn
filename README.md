# SpeakEasy (SpeakerLearn)

**Adaptive audio balancing platform** — calibrates speakers, flattens frequency response, and lets users add personal EQ on a neutral base. Works offline with localStorage fallback.

---

## Quick Start

```bash
# macOS / Linux
./run.sh

# Windows
run.bat
```

- **Frontend:** http://localhost:8080  
- **Backend:** http://localhost:8001  

Requires: **Node.js**, CMake 3.20+, C++20 compiler. Builds backend automatically. (Python no longer required.)

### Backend Only

```bash
cd backend && mkdir build && cd build
cmake .. && cmake --build . --config Release
./speakerlearn_serverd   # or Release\speakerlearn_serverd.exe
```

---

## Project Structure

```
Backend/
├── index.html              # Entry → redirects to homePage
├── homePage.html           # Landing (hero, mission, 3D speaker)
├── addSpeakerPage.html     # Add/calibrate (Learn or Know flow)
├── preferencesPage.html    # EQ preferences, active speaker, presets
├── playMusic.html          # Playback with EQ correction
├── js/
│   ├── api.js              # Backend API + localStorage fallback
│   └── bands.js            # 7-band EQ constants, Learn↔7 conversion
├── backend/                # C++20 HTTP API
│   ├── src/                # api, db, core (pipeline, stages)
│   ├── dsp_engine/         # DSP library (C API)
│   └── docs/               # API.md, ARCHITECTURE.md
└── docs/
    └── FRONTEND.md         # Design system, components, responsive
```

---

## User Flow

| Step | Page | Action |
|------|------|--------|
| 1 | homePage | Landing, mission, 3D speaker demo |
| 2 | addSpeakerPage | **Learn** (5 test tones) or **Know** (manual 7-band EQ) |
| 3 | preferencesPage | Select speaker, tweak EQ (±16 dB), save presets |
| 4 | playMusic | Choose speaker + preset, output device, play with correction |

---

## Documentation

| Doc | Description |
|-----|-------------|
| [docs/FRONTEND.md](docs/FRONTEND.md) | Design system, CSS, components, responsive, API usage |
| [backend/README.md](backend/README.md) | Backend build, pipeline, schema |
| [backend/docs/API.md](backend/docs/API.md) | Full API reference |
| [backend/docs/ARCHITECTURE.md](backend/docs/ARCHITECTURE.md) | Component diagram, data flow |

---

## Tech Stack

| Layer | Tech |
|-------|------|
| Frontend | Static HTML, inline CSS, no build step |
| Shared logic | api.js, bands.js |
| Backend | C++20, cpp-httplib, SQLite, nlohmann/json |
| DSP | 8-stage pipeline (SourceDetector → EQOptimizer → Limiter) |
