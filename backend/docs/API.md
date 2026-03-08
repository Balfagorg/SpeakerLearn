# SpeakerLearn API Reference

**Base URL:** `http://localhost:8001`

---

## Endpoints

### Health

**GET /** — Service status

```json
{ "service": "Adaptive Audio Balancing Platform (C++ Native)", "version": "0.1.0", "status": "online" }
```

---

### Audio

**POST /audio/optimize**

| Field | Type | Description |
|-------|------|-------------|
| `samples` | number[] | Audio samples (float) |
| `preferences` | object | Preference weights (e.g. `bass_boost`) |
| `platform_hint` | string | `spotify`, `apple_music`, `youtube`, `local_flac` |
| `speaker_calibration` | object | 7-band EQ (optional) |
| `speaker_system_id` | string | Active speaker ID (optional) |

**Response:** `{ final_eq, headroom_db, peak_limit_db, track_profile, warnings }`

---

### Speaker Systems

| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/speaker-systems/create` | Create speaker. Body: `{ name, user_id, device_identifier, channel_config }` |
| GET | `/speaker-systems/list?user_id=` | List speakers |
| POST | `/speaker-systems/calibrate` | Submit Learn flow ratings. Body: `{ speaker_system_id, readings: [{ label, val, issue }] }` |
| POST | `/speaker-systems/switch` | Set active. Body: `{ user_id, speaker_system_id }` |
| PUT | `/speaker-systems/update` | Update speaker |
| DELETE | `/speaker-systems/delete` | Delete speaker |

---

### EQ Presets

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/eq-presets/list?user_id=` | List presets |
| POST | `/eq-presets/save` | Body: `{ id, name, speaker_system_id, bands }` |
| POST | `/eq-presets/delete` | Body: `{ id }` |

---

### Preferences

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/preferences?user_id=` | Get preference weights |
| POST | `/preferences/update` | Body: `{ user_id, preferences }` |

---

### Devices

**GET /devices/detect** — Returns `[{ id, name, type }]` (stub)

---

## CORS

- `Access-Control-Allow-Origin: *`
- `Access-Control-Allow-Methods: POST, GET, OPTIONS`
- `Access-Control-Allow-Headers: *`
