# SpeakerLearn

**HackCU Speaker Learning + Optimization Program**

Adaptive audio balancing platform that optimizes playback across different songs, streaming platforms, and speaker systems — while protecting hardware from distortion or overload.

---

## Project Overview

| Component | Description |
|-----------|-------------|
| **Backend** | C++20 native HTTP API — audio optimization pipeline, speaker system management |
| **Frontend** | HTML pages (homePage, addSpeakerPage, preferencesPage) |
| **DSP Engine** | C++ shared library for real-time EQ, compression, limiting |

---

## Quick Start

### Local Runner (Backend + Frontend)

For quick local testing, run both backend and frontend together:

```bash
python run_local.py
# or on Windows: run.bat
```

- **Frontend:** http://localhost:8080/homePage.html  
- **Backend:** http://localhost:8001  

Requires: Python 3, CMake 3.20+, C++20 compiler. Builds the backend automatically if needed.

### Backend Only (C++)

```bash
cd backend
mkdir build && cd build
cmake ..
cmake --build . --config Release
./speakerlearn_serverd   # or Release\speakerlearn_serverd.exe on Windows
```

Server runs at **http://localhost:8001**.

### Documentation

| Document | Description |
|----------|-------------|
| [backend/README.md](backend/README.md) | Backend overview, build, API summary |
| [backend/docs/API.md](backend/docs/API.md) | Full API reference |
| [backend/docs/ARCHITECTURE.md](backend/docs/ARCHITECTURE.md) | Framework & component architecture |
| [backend/speakerlearn_framework_plan.md](backend/speakerlearn_framework_plan.md) | Product vision & calibration workflow |

---

## Repository Structure

```
testingforrealsies/
├── backend/           # C++ native API server
│   ├── src/           # api, db, core (pipeline, stages)
│   ├── dsp_engine/    # DSP library (C API)
│   └── docs/          # API & architecture docs
├── homePage.html
├── addSpeakerPage.html
├── preferencesPage.html
└── README.md
```
