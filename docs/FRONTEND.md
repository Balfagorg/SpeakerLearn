# SpeakEasy Frontend — Design System & Reference

## Design System

### Colors (`:root` variables)

| Variable | Hex | Use |
|----------|-----|-----|
| `--light-green` | #a2faa3 | CTAs, accents |
| `--muted-teal` | #92c9b1 | Labels |
| `--cerulean` | #4f759b | Speaker UI |
| `--grape` | #5d5179 | Team UI |
| `--deep-purple` | #571f4e | Header, footer |
| `--bg` | #0e0b14 | Background |
| `--surface` | #16101f | Cards |
| `--text` | #f0ede8 | Primary text |
| `--text-muted` | #a09ab0 | Secondary |

### Typography

- **Display:** DM Serif Display — headings, logo
- **Body:** DM Sans — body, buttons, nav
- **Monospace:** Space Mono — labels, technical UI

### Global Styles

```css
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
body { background: var(--bg); color: var(--text); font-family: 'DM Sans'; overflow-x: hidden; }
```

---

## Page Structure

### homePage.html

- **Header:** topbar (logo, tagline, nav) + hero (value prop, CTAs)
- **3D Canvas:** Three.js speaker model, scroll-driven wave rings
- **Mission:** stats, before/after waveform demo
- **Team:** speaker + team member cards

### Sub-pages (addSpeaker, preferences, playMusic)

- Compact header: `.page-title-strip` instead of hero
- Nav active state: `nav a.active` on current page
- Logo links to `homePage.html`

---

## Components

| Component | Classes | Use |
|-----------|---------|-----|
| Primary CTA | `.btn-primary` | Solid light-green |
| Secondary | `.btn-outline` | Border only |
| Stat card | `.stat-card` → `.stat-number`, `.stat-label` | Metrics |
| Photo card | `.photo-card` → `.photo-placeholder` + `.card-info` | Speakers, team |
| Section label | `.section-label` | Eyebrow with line |
| Waveform bars | `.wave-bars`, `.wave-bar` | Before/after visualization |

---

## JavaScript

### api.js

| Method | Purpose |
|--------|---------|
| `getAllSpeakers()` | Merge backend + localStorage |
| `getActiveSpeaker()` / `setActiveSpeaker(id)` | Active speaker |
| `speakerSystemsCreate/List/Calibrate/Update/Delete` | CRUD |
| `eqPresetsList/Save/Delete` | Preset management |
| `audioOptimize(samples, preferences, platformHint, speakerCalibration, speakerSystemId)` | POST /audio/optimize |

### bands.js

| Constant | Purpose |
|----------|---------|
| `BANDS_7` | [60, 170, 350, 1000, 3500, 8000, 16000] Hz |
| `learn5To7Band(readings)` | 5 Learn steps → 7-band storage |
| `sevenBandToLearn5(bands, issues)` | 7-band → 5 for edit |
| `PLATFORM_OPTIONS` | Spotify, YouTube, Apple Music, Local |

---

## Responsive Design

### Breakpoints

| Width | Changes |
|-------|---------|
| **768px** | Grids stack; photo grids `auto-fit minmax(140px, 1fr)`; section padding reduced |
| **480px** | Topbar/nav wrap; single column; touch targets 44px; footer column |

### Touch Targets

- Nav links, buttons, tabs: `min-height: 44px` (WCAG 2.5.5)
- Edit/delete: `min-width: 44px; min-height: 44px`

### Clamp

- `.logo`: `clamp(1.6rem, 3vw, 2.2rem)`
- `.hero h1`: `clamp(3rem, 7vw, 6rem)`

---

## File Quick Reference

| Need… | Look at |
|-------|---------|
| API calls, data shape | `js/api.js` |
| EQ bands, conversions | `js/bands.js` |
| Landing, 3D scene | `homePage.html` |
| Add/calibrate flows | `addSpeakerPage.html` |
| EQ, presets | `preferencesPage.html` |
| Playback, device select | `playMusic.html` |

---

## Elevator Pitch

> SpeakEasy flattens speaker frequency response so different speakers sound consistent. Users **Learn** (5 test tones) or **Know** (manual 7-band EQ) their speaker. The app applies correction in real time; users add personal EQ on a neutral base. Offline-first via localStorage + backend sync.
