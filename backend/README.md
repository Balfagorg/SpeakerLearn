# Adaptive Multi-System Audio Balancing Platform

A modular backend framework for **dynamically optimising audio playback** across different songs, streaming platforms, and speaker systems — while protecting hardware from distortion or overload.

> **Status:** Initial framework / prototype. Designed for expansion into a production-grade DSP system.

---

## Quick Start

```bash
# 1. Create a virtual environment
python -m venv .venv
.venv\Scripts\activate        # Windows
# source .venv/bin/activate   # macOS / Linux

# 2. Install dependencies
pip install -r requirements.txt

# 3. Start the API server
uvicorn api.main:app --reload --port 8000
```

Open **http://localhost:8000/docs** for interactive API documentation (Swagger UI).

---

## Project Structure

```
adaptive_audio_platform/
│
├── api/                          # FastAPI application
│   ├── main.py                   # App entry point, CORS, lifespan
│   └── routers/
│       ├── speakers.py           # Speaker system CRUD + calibration
│       ├── preferences.py        # User preference management
│       ├── audio.py              # Audio optimization endpoint
│       └── devices.py            # Device detection endpoint
│
├── core/                         # Processing pipeline modules
│   ├── pipeline.py               # End-to-end orchestrator
│   ├── source_detector.py        # Stage 1 — Source platform detection
│   ├── spectrum_analyzer.py      # Stage 2 — FFT spectral analysis
│   ├── track_profiler.py         # Stage 3 — Track classification
│   ├── preference_engine.py      # Stage 4 — User preference → EQ curve
│   ├── speaker_model.py          # Stage 5 — Speaker capability constraints
│   ├── eq_optimizer.py           # Stage 6 — Adaptive EQ computation
│   ├── dsp_processor.py          # Stage 7 — DSP engine (stub)
│   ├── limiter.py                # Stage 8 — Safety limiter
│   ├── calibration.py            # Speaker calibration system
│   └── device_detector.py        # Audio device discovery
│
├── db/                           # Database layer
│   ├── database.py               # Engine, sessions, init & seeding
│   ├── models.py                 # SQLAlchemy ORM models
│   └── schemas.py                # Pydantic request/response schemas
│
├── pyproject.toml                # Project metadata & dependencies
├── requirements.txt              # Pinned dependencies
└── README.md                     # ← you are here
```

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

The pipeline runs continuously, re-analysing and adjusting every 200–500 ms during playback.

---

## API Reference

| Endpoint | Method | Description |
|---|---|---|
| `/speaker-systems/create` | POST | Create a named speaker system |
| `/speaker-systems/list` | GET | List all systems for a user |
| `/speaker-systems/calibrate` | POST | Submit calibration ratings |
| `/speaker-systems/switch` | POST | Set the active speaker system |
| `/preferences/update` | POST | Update listening preferences |
| `/preferences` | GET | Get current preferences |
| `/audio/optimize` | POST | Run the full optimisation pipeline |
| `/devices/detect` | GET | Detect connected audio devices |

Full interactive documentation at `/docs` when the server is running.

---

## Database Schema (SQLite)

| Table | Purpose |
|---|---|
| `users` | Application users |
| `speaker_systems` | Named speaker configurations with device mapping |
| `speaker_capabilities` | Per-band frequency handling limits |
| `calibration_results` | Raw calibration test ratings |
| `user_preferences` | Listening preference weights |
| `source_profiles` | Per-platform audio correction offsets |

Tables are auto-created on first startup. Default source profiles (Spotify, Apple Music, YouTube, Local FLAC) are seeded automatically.

---

## Speaker Calibration Workflow

SpeakerLearn uses a unique **two-step playback comparison** to map capabilities:

1. **Test Tones:** The system triggers five specific audio profiles:
   - Pure Bass
   - Pure Mids
   - Pure Treble
   - Full Spectrum
   - Troublesome Frequency Mix
2. **Comparison:** Each tone is played first on the local device, then on the target speaker.
3. **Rating:** The user rates *Volume* (1-5 scale relative to the local device) and *Quality* (Good/Bad).
4. **EQ Generation:** The backend calculates a neutralizing EQ curve to flatten the response and cut troublesome distortions.

---

## Architecture: Python & C++ Tandem

SpeakerLearn uses a hybrid architecture to balance API flexibility with real-time DSP performance:

- **Python (FastAPI):** Orchestrates calibration, manages the local SQLite database of offline profiles, and calculates EQ parameters.
- **C++ DSP Engine (`dsp_engine/`):** A compiled shared library containing the raw audio processing logic (multiband EQ, dynamic compression, limiting). The Python backend loads this via `ctypes` and passes the optimized speaker configurations to it for real-time processing.

### Offline & Background Operations

SpeakerLearn runs continuously in the background, intercepting device audio output and applying the target EQ real-time. The local SQLite database ensures the application functions completely offline without internet connectivity.

---

## Frontend Integration Guide

This framework is **backend-only** by design. To build a frontend:

### Key Endpoints for UI

- **Speaker Setup Flow:** `POST /speaker-systems/create` → `POST /speaker-systems/calibrate`
- **Preferences Panel:** `GET /preferences` → `POST /preferences/update`

### Recommended Frontend Stack

- **Mobile/Desktop:** A cross-platform UI (React Native, Electron, Tauri) that communicates with this local Python/C++ tandem backend.

---

## Tech Stack

| Component | Technology |
|---|---|
| Backend API | Python / FastAPI |
| Database | SQLite / SQLAlchemy |
| DSP Engine | C++ (`dsp_engine/` loaded via ctypes) |
| Validation | Pydantic v2 |
