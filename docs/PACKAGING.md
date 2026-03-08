# SpeakEasy — Release Packaging Guide

Two packaging options: **Electron (.exe / .dmg / .AppImage)** for a native app experience, or **zip/tar.gz** for a lighter, script-based package.

---

## Option A: Electron (.exe / Installer) — Recommended

Produces a **double-clickable app** — no Node.js required for users.

| Platform | Command | Output |
|----------|---------|--------|
| **Windows** | `npm run build:win` | `dist-electron/SpeakEasy Setup 0.1.0.exe` (installer) + `SpeakEasy 0.1.0.exe` (portable) |
| **macOS** | `npm run build:mac` | `dist-electron/SpeakEasy-0.1.0.dmg` |
| **Linux** | `npm run build:linux` | `dist-electron/SpeakEasy-0.1.0.AppImage` |

**Requirements:** Run on the target platform. Needs Node.js, CMake 3.20+, C++20 compiler, and (first time) `npm install` to pull Electron.

```bash
npm install
npm run build:win   # or build:mac / build:linux
```

The build script automatically compiles the C++ backend and bundles it. Outputs go to `dist-electron/`.

---

## Option B: Zip / Tar.gz (Script-based)

Lighter packages; users need Node.js installed.

| Platform | Command | Output |
|----------|---------|--------|
| **Windows** | `npm run package:win` | `dist/SpeakEasy-v0.1.0-win64.zip` |
| **macOS** | `npm run package:mac` | `dist/SpeakEasy-v0.1.0-macos.tar.gz` |
| **Linux** | `npm run package:linux` | `dist/SpeakEasy-v0.1.0-linux64.tar.gz` |

**Requirements:** Run each command on the target platform. Needs Node.js, CMake 3.20+, C++20 compiler.

---

## What Gets Packaged

Each release archive contains:

```
SpeakEasy-v0.1.0-{platform}/
├── frontend/           # HTML, JS, CSS
├── server/             # Node server + bridge
├── backend/
│   └── build/          # Pre-built speakerlearn_serverd[.exe]
├── node_modules/       # naudiodon + deps (built for target platform)
├── package.json
└── run.bat / run.sh    # Launcher
```

**No build step on first run** — the C++ backend is pre-built. Users extract and run.

---

## User Instructions (for your download page)

### Windows

1. Download `SpeakEasy-v0.1.0-win64.zip`
2. Extract to a folder
3. Install [Node.js](https://nodejs.org/) if not already installed
4. Double-click `run.bat` (or run it from a terminal)
5. Open **http://localhost:8080/homePage.html** in your browser

### macOS

1. Download `SpeakEasy-v0.1.0-macos.tar.gz`
2. Extract: `tar xzf SpeakEasy-v0.1.0-macos.tar.gz`
3. Install [Node.js](https://nodejs.org/) if not already installed
4. Run: `./run.sh`
5. Open **http://localhost:8080/homePage.html** in your browser

### Linux

1. Download `SpeakEasy-v0.1.0-linux64.tar.gz`
2. Extract: `tar xzf SpeakEasy-v0.1.0-linux64.tar.gz`
3. Install [Node.js](https://nodejs.org/) if not already installed
4. Run: `./run.sh`
5. Open **http://localhost:8080/homePage.html** in your browser

---

## CI / Automated Builds

Use GitHub Actions (or similar) to build all three platforms on each release:

```yaml
# .github/workflows/release.yml (example)
name: Release
on:
  push:
    tags: ['v*']
jobs:
  build:
    strategy:
      matrix:
        include:
          - os: windows-latest
            run: npm run package:win
          - os: macos-latest
            run: npm run package:mac
          - os: ubuntu-latest
            run: npm run package:linux
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-node@v4
        with:
          node-version: '20'
      - name: Install CMake (Linux)
        if: runner.os == 'Linux'
        run: sudo apt-get install -y cmake build-essential
      - name: Package
        run: npm ci && ${{ matrix.run }}
      - uses: actions/upload-artifact@v4
        with:
          name: SpeakEasy-${{ matrix.os }}
          path: dist/*.zip
```

---

## Optional: Bundle Node.js

To avoid requiring users to install Node.js, you can bundle a portable Node runtime:

1. Download the Node.js binary for each platform from [nodejs.org](https://nodejs.org/dist/)
2. Extract `node.exe` (Windows) or `bin/node` (macOS/Linux) into the package
3. Update `run.bat` / `run.sh` to use the bundled `./node` instead of `node`

This increases package size (~30–50 MB) but improves one-click experience.

---

## Optional: Native Installers

For a more polished experience, wrap the package in platform installers:

| Platform | Tool | Output |
|----------|------|--------|
| Windows | [Inno Setup](https://jrsoftware.org/isinfo.php) or NSIS | `.exe` installer |
| macOS | Create `.app` bundle or use [create-dmg](https://github.com/create-dmg/create-dmg) | `.dmg` |
| Linux | [AppImage](https://appimage.org/) or `.deb` / `.rpm` | Single-file app |

The backend `CMakeLists.txt` already includes CPack for the C++ binary; the full app packaging above handles the Node + frontend layer.

---

## Troubleshooting

### "Backend executable not found" after extract

- Ensure you extracted the full archive (including `backend/build/`)
- On Windows, the exe is at `backend/build/Release/speakerlearn_serverd.exe`
- On Mac/Linux, it's at `backend/build/speakerlearn_serverd`

### naudiodon build fails during packaging

- **Windows:** Install Visual Studio with C++ workload; run from Developer Command Prompt if needed
- **macOS:** Run `xcode-select --install`
- **Linux:** Install `build-essential` and `libasound2-dev` (for PortAudio)

### Package too large

- Exclude `node_modules` and run `npm install` on first launch (adds complexity)
- Or use `npm prune --production` before packaging to remove dev deps (already done with `--omit=dev`)
