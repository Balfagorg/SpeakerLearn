@echo off
REM SpeakEasy local runner — backend + frontend (no Python required)
REM Requires: Node.js, CMake 3.20+, C++20 compiler

cd /d "%~dp0"

where node >nul 2>nul
if errorlevel 1 (
  echo Error: Node.js is required. Install from https://nodejs.org/
  pause
  exit /b 1
)

node server/run.js
if errorlevel 1 pause
