# SpeakerLearn Backend — API Reference

Base URL: `http://localhost:8001` (configurable via `main.cpp`)

---

## Overview

The SpeakerLearn backend exposes a REST-style HTTP API for audio optimization and speaker system management. All responses are JSON. CORS is enabled for all origins in development.

---

## Endpoints

### Health & Status

#### `GET /`

Returns service metadata and health status.

**Response:** `200 OK`

```json
{
  "service": "Adaptive Audio Balancing Platform (C++ Native)",
  "version": "0.1.0",
  "status": "online"
}
```

---

### Audio Optimization

#### `POST /audio/optimize`

Runs the full 8-stage audio optimization pipeline. Accepts audio samples, user preferences, and an optional platform hint.

**Request Headers:**
- `Content-Type: application/json`

**Request Body:**
| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `samples` | `number[]` | No | Audio samples (float). Default: stub data |
| `preferences` | `object` | No | Key-value preference weights (e.g. `bass_boost`) |
| `platform_hint` | `string` | No | Source platform: `spotify`, `apple_music`, `youtube`, `local_flac` |

**Example:**
```json
{
  "samples": [0.1, -0.1, 0.2, -0.2],
  "preferences": { "bass_boost": 0.5 },
  "platform_hint": "spotify"
}
```

**Response:** `200 OK`

| Field | Type | Description |
|-------|------|-------------|
| `final_eq` | `object` | Per-band EQ gains (sub_bass, bass, low_mid, mid, high, air) |
| `headroom_db` | `number` | Headroom in dB before limiter ceiling |
| `peak_limit_db` | `number` | Limiter ceiling in dB |
| `track_profile` | `object` | Track classification (avg_loudness, dynamic_range) |
| `warnings` | `string[]` | Pipeline warnings (e.g. signal exceeded ceiling) |

**Example:**
```json
{
  "final_eq": {
    "sub_bass": 0.0,
    "bass": 0.0,
    "low_mid": 0.0,
    "mid": 0.0,
    "high": 0.0,
    "air": 0.0
  },
  "headroom_db": 3.0,
  "peak_limit_db": -1.0,
  "track_profile": {
    "avg_loudness": -14.0,
    "dynamic_range": 8.0
  },
  "warnings": []
}
```

**Error Response:** `400 Bad Request` (e.g. invalid JSON)

```json
{
  "error": "parse error message"
}
```

---

### Speaker Systems

#### `POST /speaker-systems/create`

Creates a new speaker system and persists to SQLite.

**Request Body:**
| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | `string` | No | Speaker system name (default: "Unnamed Speaker") |
| `user_id` | `string` | No | User ID (default: "default-user") |
| `device_identifier` | `string` | No | Device ID for mapping |
| `channel_config` | `string` | No | "stereo" (default) |

**Response:** `200 OK`

```json
{
  "status": "created",
  "id": "550e8400-e29b-41d4-a716-446655440000"
}
```

#### `GET /speaker-systems/list`

Lists speaker systems for a user.

**Query Parameters:** `user_id` (optional, default: "default-user")

**Response:** `200 OK` — Array of `{ id, name, device_identifier, channel_config, is_active }`

---

### Preferences

#### `GET /preferences`

Returns user preferences as key-value weights.

**Query Parameters:** `user_id` (optional, default: "default-user")

**Response:** `200 OK` — Object `{ "bass_boost": 0.5, "treble": 0.3, ... }`

#### `POST /preferences/update`

Updates user preferences.

**Request Body:**
| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `user_id` | `string` | No | User ID (default: "default-user") |
| `preferences` | `object` | Yes | Key-value preference weights |

**Response:** `200 OK` — `{ "status": "updated" }`

---

### Devices

#### `GET /devices/detect`

Returns detected audio devices. **Currently a stub** — returns a default output device.

**Response:** `200 OK` — Array of `{ id, name, type }`

---

## Planned Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/speaker-systems/calibrate` | POST | Submit calibration ratings |
| `/speaker-systems/switch` | POST | Set active speaker system |

---

## CORS

Pre-routing handler sets:
- `Access-Control-Allow-Origin: *`
- `Access-Control-Allow-Methods: POST, GET, OPTIONS`
- `Access-Control-Allow-Headers: *`

OPTIONS requests are handled and return immediately.
