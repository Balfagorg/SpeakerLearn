# SpeakEasy (SpeakerLearn)

**Adaptive audio balancing platform** — calibrates speakers, flattens frequency response, and lets users add personal EQ on a neutral base. Works offline with localStorage fallback.

---

## Quick Start

```bash
# Install dependencies (required for SpeakEasy Bridge)
npm install

# macOS / Linux
./run.sh

# Windows
run.bat
```

- **Frontend:** http://localhost:8080  
- **Backend:** http://localhost:8001  

Requires: **Node.js**, CMake 3.20+, C++20 compiler. Builds backend automatically. (Python no longer required.)

### Build .exe / Installer (Electron)

```bash
npm install
npm run build:win   # → dist-electron/SpeakEasy Setup 0.1.0.exe
```

Produces a double-clickable installer and portable .exe. No Node.js needed for end users. See [docs/PACKAGING.md](docs/PACKAGING.md).

**Installation:** See [docs/INSTALL.md](docs/INSTALL.md) for detailed setup of CMake, npm, and dependencies on **Windows** and **macOS**.

### SpeakEasy Bridge — Use as system output device

To route **all system audio** (Spotify, YouTube, games) through SpeakEasy's EQ:

1. Install [VB-Audio Virtual Cable](https://vb-audio.com/Cable/) (free)
2. In Windows Sound settings, set **Output** to *CABLE Input*
3. Open Play Music → **SpeakEasy Bridge** tab
4. Select **Input** (CABLE Output) and **Output** (your speakers)
5. Choose a speaker/preset/genre for EQ, then click **Start Bridge**

Audio from any app will flow: app → CABLE Input → SpeakEasy (EQ) → your speakers.

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
├── frontend/               # UI (HTML, JS, assets)
│   ├── index.html         # Entry → redirects to homePage
│   ├── homePage.html      # Landing (hero, mission, 3D speaker)
│   ├── addSpeakerPage.html
│   ├── preferencesPage.html
│   ├── playMusic.html
│   └── js/
│       ├── api.js         # Backend API + localStorage fallback
│       ├── bands.js       # 7-band EQ constants, Learn↔7 conversion
│       └── eqExport.js    # EqualizerAPO config export
├── server/                # Node.js server + bridge
│   ├── run.js             # Main runner (builds backend, serves frontend)
│   └── bridge/
│       ├── audioBridge.js # Virtual cable capture → EQ → output
│       └── biquadEq.js    # Real-time biquad EQ for bridge
├── backend/               # C++20 HTTP API
│   ├── src/               # api, db, core (pipeline, stages)
│   ├── Mac OS/            # macOS build script (build-macos.sh)
│   ├── dsp_engine/        # DSP library (C API)
│   └── docs/              # API.md, ARCHITECTURE.md
├── docs/
│   ├── INSTALL.md
│   ├── BRIDGE.md
│   └── FRONTEND.md
├── run.bat                # Windows launcher
├── run.sh                 # macOS/Linux launcher
└── run_local.py           # Python launcher (alternative)
```

---

## User Flow

| Step | Page | Action |
|------|------|--------|
| 1 | homePage | Landing, mission, 3D speaker demo |
| 2 | addSpeakerPage | **Learn** (5 test tones) or **Know** (manual 7-band EQ) |
| 3 | preferencesPage | Select speaker, tweak EQ (±16 dB), save presets |
| 4 | playMusic | Choose speaker + preset, output device, play with correction |
| 5 | SpeakEasy Bridge | Route system audio through EQ (requires VB-Audio Virtual Cable) |

**SpeakEasy Bridge:** Set system output to a virtual cable; SpeakEasy captures it, applies EQ, and forwards to your speakers. See the Bridge tab in Play Music.

**EqualizerAPO export:** For system-wide EQ without the bridge, export config and use [EqualizerAPO](https://sourceforge.net/projects/equalizerapo/).

---

## Documentation

| Doc | Description |
|-----|-------------|
| [docs/OVERVIEW.md](docs/OVERVIEW.md) | **System overview** — How SpeakEasy works end-to-end |
| [docs/INSTALL.md](docs/INSTALL.md) | **Install guide** — CMake, npm, dependencies (Windows & macOS) |
| [docs/PACKAGING.md](docs/PACKAGING.md) | **Release packaging** — Create downloadable builds for Win/Mac/Linux |
| [docs/FRONTEND.md](docs/FRONTEND.md) | Design system, CSS, components, responsive, API usage |
| [backend/README.md](backend/README.md) | Backend build, pipeline, schema |
| [backend/Mac OS/README.md](backend/Mac%20OS/README.md) | macOS-specific build instructions |
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
