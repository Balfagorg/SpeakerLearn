#!/bin/bash
# SpeakEasy local runner — backend + frontend (no Python required)
# Requires: Node.js, CMake 3.20+, C++20 compiler

cd "$(dirname "$0")"

if ! command -v node &> /dev/null; then
  echo "Error: Node.js is required. Install from https://nodejs.org/"
  exit 1
fi

node server/run.js
exit $?
