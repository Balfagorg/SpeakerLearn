#!/usr/bin/env node
/**
 * Prepares the C++ backend for Electron packaging.
 * Builds the backend and copies the executable to electron-resources/bin/
 * Run before electron-builder (e.g. in "beforeBuild" or manually).
 */

const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

const ROOT = path.resolve(__dirname, '..');
const BACKEND_DIR = path.join(ROOT, 'backend');
const BUILD_DIR = path.join(BACKEND_DIR, 'build');
const RESOURCES_BIN = path.join(ROOT, 'electron-resources', 'bin');
const isWindows = process.platform === 'win32';

function findBackendExe() {
  const macBuild = path.join(BACKEND_DIR, 'Mac OS', 'build');
  const candidates = isWindows
    ? [
        path.join(BUILD_DIR, 'Release', 'speakerlearn_serverd.exe'),
        path.join(BUILD_DIR, 'Debug', 'speakerlearn_serverd.exe'),
        path.join(BUILD_DIR, 'speakerlearn_serverd.exe'),
      ]
    : [
        path.join(macBuild, 'speakerlearn_serverd'),
        path.join(BUILD_DIR, 'speakerlearn_serverd'),
        path.join(BUILD_DIR, 'Release', 'speakerlearn_serverd'),
      ];
  for (const p of candidates) {
    if (fs.existsSync(p)) return p;
  }
  return null;
}

function buildBackend() {
  if (!fs.existsSync(BUILD_DIR)) {
    fs.mkdirSync(BUILD_DIR, { recursive: true });
  }
  console.log('[prepare] Configuring CMake...');
  execSync('cmake ..', { cwd: BUILD_DIR, stdio: 'inherit' });
  console.log('[prepare] Building...');
  execSync(`cmake --build . --config Release`, { cwd: BUILD_DIR, stdio: 'inherit' });
}

let exe = findBackendExe();
if (!exe) {
  console.log('[prepare] Backend not built. Building now...');
  buildBackend();
  exe = findBackendExe();
}
if (!exe) {
  console.error('[prepare] Build failed — executable not found.');
  process.exit(1);
}

fs.mkdirSync(RESOURCES_BIN, { recursive: true });
const dest = path.join(RESOURCES_BIN, isWindows ? 'speakerlearn_serverd.exe' : 'speakerlearn_serverd');
fs.copyFileSync(exe, dest);
console.log(`[prepare] Copied to ${dest}`);
