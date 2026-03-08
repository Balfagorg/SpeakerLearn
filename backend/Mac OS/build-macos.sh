#!/bin/bash
# SpeakEasy (SpeakerLearn) — macOS build script
# Builds the C++ backend (speakerlearn_serverd) for macOS.
# Requires: macOS 12 (Monterey) or later, CMake 3.20+, Xcode Command Line Tools (clang with C++20)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BACKEND_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$SCRIPT_DIR/build"

echo "SpeakEasy — macOS Build"
echo "======================="
echo "Backend: $BACKEND_DIR"
echo "Build:   $BUILD_DIR"
echo ""

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo "[1/2] Configuring CMake..."
cmake "$BACKEND_DIR" -DCMAKE_BUILD_TYPE=Release

# Build
echo "[2/2] Building..."
cmake --build . --config Release

# Result
EXE="$BUILD_DIR/speakerlearn_serverd"
if [[ -f "$EXE" ]]; then
  echo ""
  echo "Build successful: $EXE"
  echo "Run: $EXE"
else
  echo "Error: Executable not found at $EXE"
  exit 1
fi
