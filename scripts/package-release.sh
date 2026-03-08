#!/bin/bash
# SpeakEasy — Create release package for macOS or Linux
# Run on the target platform (macOS or Linux). Produces a .tar.gz archive.
#
# Usage: ./scripts/package-release.sh [macos|linux]
# Default: auto-detect from uname

set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# Detect platform
DETECTED=$(uname -s | tr '[:upper:]' '[:lower:]')
PLATFORM="${1:-$DETECTED}"
if [[ "$PLATFORM" == "darwin" ]]; then PLATFORM="macos"; fi

VERSION=$(node -p "require('./package.json').version" 2>/dev/null || echo "0.1.0")
DIST_DIR="$ROOT/dist"
PKG_NAME="SpeakEasy-v${VERSION}-${PLATFORM}"
PKG_DIR="$DIST_DIR/$PKG_NAME"

echo "SpeakEasy — Release Package ($PLATFORM)"
echo "========================================"
echo "Version:  $VERSION"
echo "Output:   $PKG_DIR"
echo ""

# 1. Build C++ backend
echo "[1/4] Building C++ backend..."
if [[ "$PLATFORM" == "macos" ]]; then
  if [[ -d "$ROOT/backend/Mac OS" ]]; then
    (cd "$ROOT/backend/Mac OS" && ./build-macos.sh)
    BACKEND_EXE="$ROOT/backend/Mac OS/build/speakerlearn_serverd"
  else
    mkdir -p "$ROOT/backend/build"
    (cd "$ROOT/backend/build" && cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build . --config Release)
    BACKEND_EXE="$ROOT/backend/build/speakerlearn_serverd"
  fi
else
  mkdir -p "$ROOT/backend/build"
  (cd "$ROOT/backend/build" && cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build . --config Release)
  BACKEND_EXE="$ROOT/backend/build/speakerlearn_serverd"
fi

if [[ ! -f "$BACKEND_EXE" ]]; then
  echo "Error: Backend build failed — executable not found"
  exit 1
fi
echo "  Backend: $BACKEND_EXE"

# 2. npm install (production deps, builds naudiodon for this platform)
echo "[2/4] Installing npm dependencies..."
npm install --omit=dev

# 3. Assemble package
echo "[3/4] Assembling package..."
rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR"

cp -r frontend "$PKG_DIR/"
cp -r server "$PKG_DIR/"
cp package.json "$PKG_DIR/"
cp -r node_modules "$PKG_DIR/"

# Backend: pre-built executable only (run.js looks in backend/build/)
mkdir -p "$PKG_DIR/backend/build"
cp "$BACKEND_EXE" "$PKG_DIR/backend/build/speakerlearn_serverd"

# Run script
cp run.sh "$PKG_DIR/"
chmod +x "$PKG_DIR/run.sh"

# 4. Create archive
echo "[4/4] Creating archive..."
mkdir -p "$DIST_DIR"
ARCHIVE="$DIST_DIR/${PKG_NAME}.tar.gz"
(cd "$DIST_DIR" && tar czf "${PKG_NAME}.tar.gz" "$PKG_NAME")
rm -rf "$PKG_DIR"

echo ""
echo "Done: $ARCHIVE"
echo ""
echo "User instructions:"
echo "  1. Extract the archive"
echo "  2. Ensure Node.js is installed (nodejs.org)"
echo "  3. Run: ./run.sh"
echo "  4. Open http://localhost:8080/homePage.html"
echo ""
