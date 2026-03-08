# SpeakEasy — Installation Guide

Installation instructions for **Windows** and **macOS**. SpeakEasy requires Node.js, CMake, a C++20 compiler, and npm dependencies.

---

## Overview

| Component | Purpose |
|-----------|---------|
| **Node.js** | Runs the frontend server and SpeakEasy Bridge |
| **npm** | Installs `naudiodon` (audio bridge) and other dependencies |
| **CMake** | Configures the C++ backend build |
| **C++20 compiler** | Builds `speakerlearn_serverd` (HTTP API) |

---

## Windows

### 1. Node.js

1. Download the **LTS** version from [nodejs.org](https://nodejs.org/)
2. Run the installer; ensure **"Add to PATH"** is checked
3. Verify:
   ```cmd
   node --version
   npm --version
   ```

### 2. CMake

1. Download the **Windows x64 Installer** from [cmake.org/download](https://cmake.org/download/)
2. During install, choose **"Add CMake to the system PATH for all users"** (or current user)
3. Verify:
   ```cmd
   cmake --version
   ```
   Must be **3.20 or later**.

### 3. C++ Compiler (Visual Studio)

SpeakEasy needs a C++20-capable compiler. Use **Visual Studio 2022** (or 2019) with the C++ workload:

1. Download [Visual Studio 2022 Community](https://visualstudio.microsoft.com/downloads/) (free)
2. In the installer, select **"Desktop development with C++"**
3. Ensure **MSVC** and **Windows 10/11 SDK** are included
4. After install, open a **new** Command Prompt or PowerShell (so PATH is updated)

**Alternative:** If you use **Build Tools for Visual Studio** instead of the full IDE, install the "Desktop development with C++" workload.

### 4. Build and Run

```cmd
cd path\to\Brennan\Backend
npm install
run.bat
```

- **Frontend:** http://localhost:8080  
- **Backend:** http://localhost:8001  

The first run will build the C++ backend automatically.

### 5. SpeakEasy Bridge (Optional)

The Bridge routes system audio through SpeakEasy's EQ:

1. Install [VB-Audio Virtual Cable](https://vb-audio.com/Cable/) (free)
2. Set Windows **Output** to *CABLE Input*
3. In Play Music → SpeakEasy Bridge tab, select Input (CABLE Output) and Output (speakers)
4. Click **Start Bridge**

---

## macOS

**Requirements:** macOS 12 (Monterey) or later.

### 1. Node.js

**Option A — Official installer**

1. Download the **LTS** version from [nodejs.org](https://nodejs.org/)
2. Run the `.pkg` installer
3. Verify:
   ```bash
   node --version
   npm --version
   ```

**Option B — Homebrew**

```bash
brew install node
```

### 2. Xcode Command Line Tools (C++ compiler)

Required for building the C++ backend and native npm modules (naudiodon):

```bash
xcode-select --install
```

If prompted, accept the license. This installs `clang` with C++20 support.

**Optional:** For the full Xcode IDE:

```bash
# From App Store, or:
xcode-select --install
```

### 3. CMake

**Option A — Homebrew (recommended)**

```bash
brew install cmake
```

**Option B — Official installer**

1. Download the **macOS universal** `.dmg` from [cmake.org/download](https://cmake.org/download/)
2. Open the package and drag CMake to Applications
3. Run **CMake** once and choose **"Install Command Line Tools"**

Verify:

```bash
cmake --version
```

Must be **3.20 or later**.

### 4. Build and Run

**Full application (recommended):**

```bash
cd path/to/Brennan/Backend
npm install
./run.sh
```

- **Frontend:** http://localhost:8080  
- **Backend:** http://localhost:8001  

The first run will build the C++ backend in `backend/build/`.

**macOS-specific build (optional):**

To build the backend in `backend/Mac OS/build/`:

```bash
cd backend/"Mac OS"
./build-macos.sh
```

Then from the project root:

```bash
./run.sh
```

`run.js` will detect the executable in `Mac OS/build/` when present.

### 5. SpeakEasy Bridge (Optional)

On macOS, the Bridge uses Core Audio. No virtual cable is required:

1. In Play Music → SpeakEasy Bridge tab
2. Select **Input** (e.g. system output or BlackHole if installed) and **Output** (speakers/headphones)
3. Click **Start Bridge**

For system-wide routing, consider [BlackHole](https://existential.audio/blackhole/) as a virtual audio device.

---

## Backend-Only Build

If you only want the C++ API server:

**Windows:**

```cmd
cd backend
mkdir build
cd build
cmake ..
cmake --build . --config Release
Release\speakerlearn_serverd.exe
```

**macOS / Linux:**

```bash
cd backend
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
./speakerlearn_serverd
```

**macOS (Mac OS directory):**

```bash
cd backend/"Mac OS"
./build-macos.sh
./build/speakerlearn_serverd
```

Server: **http://localhost:8001**

---

## Troubleshooting

### "node: command not found" / "npm: command not found"

- **Windows:** Restart the terminal after installing Node.js; ensure it was added to PATH
- **macOS:** If using Homebrew, ensure `/opt/homebrew/bin` or `/usr/local/bin` is in your PATH

### "cmake: command not found"

- **Windows:** Re-run the CMake installer and select "Add to PATH"
- **macOS:** Run `brew install cmake` or install the command line tools from the CMake app

### "C++20" or compiler errors

- **Windows:** Install Visual Studio with the C++ workload; use a **Developer Command Prompt** or a new terminal after install
- **macOS:** Run `xcode-select --install` and accept the license

### naudiodon build fails (npm install)

- **Windows:** Ensure Visual Studio Build Tools are installed; `npm install` uses node-gyp to compile native code
- **macOS:** Install Xcode Command Line Tools: `xcode-select --install`

### Backend executable not found after build

- **Windows:** Check `backend/build/Release/speakerlearn_serverd.exe`
- **macOS:** Check `backend/build/speakerlearn_serverd` or `backend/Mac OS/build/speakerlearn_serverd`

---

## Version Requirements

| Tool | Minimum Version |
|------|-----------------|
| Node.js | 18.x LTS (or 20.x) |
| npm | 9.x (bundled with Node) |
| CMake | 3.20 |
| C++ compiler | C++20 support (MSVC 2019+, Clang 10+) |
