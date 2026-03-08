# SpeakEasy — Create release package for Windows
# Run on Windows. Produces a .zip archive.
#
# Usage: .\scripts\package-release.ps1

$ErrorActionPreference = "Stop"

$ROOT = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $ROOT

$pkg = Get-Content package.json | ConvertFrom-Json
$VERSION = $pkg.version
if (-not $VERSION) { $VERSION = "0.1.0" }

$DIST_DIR = Join-Path $ROOT "dist"
$PKG_NAME = "SpeakEasy-v$VERSION-win64"
$PKG_DIR = Join-Path $DIST_DIR $PKG_NAME

Write-Host "SpeakEasy — Release Package (Windows)" -ForegroundColor Cyan
Write-Host "======================================"
Write-Host "Version:  $VERSION"
Write-Host "Output:   $PKG_DIR"
Write-Host ""

# 1. Build C++ backend
Write-Host "[1/4] Building C++ backend..." -ForegroundColor Yellow
$BUILD_DIR = Join-Path $ROOT "backend\build"
if (-not (Test-Path $BUILD_DIR)) {
    New-Item -ItemType Directory -Path $BUILD_DIR -Force | Out-Null
}
Push-Location $BUILD_DIR
try {
    cmake ..
    cmake --build . --config Release
} finally {
    Pop-Location
}

$EXE = Join-Path $BUILD_DIR "Release\speakerlearn_serverd.exe"
if (-not (Test-Path $EXE)) {
    Write-Host "Error: Backend build failed — executable not found at $EXE" -ForegroundColor Red
    exit 1
}
Write-Host "  Backend: $EXE"

# 2. npm install
Write-Host "[2/4] Installing npm dependencies..." -ForegroundColor Yellow
npm install --omit=dev

# 3. Assemble package
Write-Host "[3/4] Assembling package..." -ForegroundColor Yellow
if (Test-Path $PKG_DIR) { Remove-Item -Recurse -Force $PKG_DIR }
New-Item -ItemType Directory -Path $PKG_DIR -Force | Out-Null

Copy-Item -Recurse -Path (Join-Path $ROOT "frontend") -Destination (Join-Path $PKG_DIR "frontend")
Copy-Item -Recurse -Path (Join-Path $ROOT "server") -Destination (Join-Path $PKG_DIR "server")
Copy-Item -Path (Join-Path $ROOT "package.json") -Destination $PKG_DIR
Copy-Item -Recurse -Path (Join-Path $ROOT "node_modules") -Destination (Join-Path $PKG_DIR "node_modules")

$BACKEND_PKG = Join-Path $PKG_DIR "backend"
New-Item -ItemType Directory -Path (Join-Path $BACKEND_PKG "build\Release") -Force | Out-Null
Copy-Item -Path $EXE -Destination (Join-Path $BACKEND_PKG "build\Release\speakerlearn_serverd.exe")

Copy-Item -Path (Join-Path $ROOT "run.bat") -Destination $PKG_DIR

# 4. Create zip
Write-Host "[4/4] Creating archive..." -ForegroundColor Yellow
if (-not (Test-Path $DIST_DIR)) { New-Item -ItemType Directory -Path $DIST_DIR -Force | Out-Null }
$ARCHIVE = Join-Path $DIST_DIR "$PKG_NAME.zip"
if (Test-Path $ARCHIVE) { Remove-Item $ARCHIVE }
Compress-Archive -Path $PKG_DIR -DestinationPath $ARCHIVE
Remove-Item -Recurse -Force $PKG_DIR

Write-Host ""
Write-Host "Done: $ARCHIVE" -ForegroundColor Green
Write-Host ""
Write-Host "User instructions:"
Write-Host "  1. Extract the zip"
Write-Host "  2. Ensure Node.js is installed (nodejs.org)"
Write-Host "  3. Double-click run.bat (or run it from a terminal)"
Write-Host "  4. Open http://localhost:8080/homePage.html"
Write-Host ""
