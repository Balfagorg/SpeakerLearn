# SpeakEasy — macOS Build

This directory contains the macOS build configuration for the SpeakEasy (SpeakerLearn) C++ backend.

## Quick Build

```bash
cd "Mac OS"
chmod +x build-macos.sh   # first time only
./build-macos.sh
```

The executable will be at `Mac OS/build/speakerlearn_serverd`.

## Requirements

- **macOS** 12 (Monterey) or later
- **Xcode Command Line Tools** — provides `clang` with C++20 support
- **CMake** 3.20 or later

See [docs/INSTALL.md](../../docs/INSTALL.md) for full dependency installation instructions.

## Run the Full Application

From the project root:

```bash
npm install
./run.sh
```

This builds the backend (if needed), serves the frontend on port 8080, and runs the C++ API on port 8001.
