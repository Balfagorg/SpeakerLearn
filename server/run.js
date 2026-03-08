#!/usr/bin/env node
/**
 * SpeakEasy local runner — builds backend, serves frontend, runs both.
 * No Python required. Uses Node.js built-in modules only.
 *
 * Backend: http://localhost:8001
 * Frontend: http://localhost:8080
 */

const http = require('http');
const fs = require('fs');
const path = require('path');
const { spawn, execSync } = require('child_process');
const os = require('os');

const ROOT = path.resolve(__dirname, '..');
const FRONTEND_DIR = path.join(ROOT, 'frontend');
const BACKEND_DIR = path.join(ROOT, 'backend');
const BUILD_DIR = path.join(BACKEND_DIR, 'build');
const FRONTEND_PORT = 8080;
const BACKEND_PORT = 8001;

const isWindows = os.platform() === 'win32';

function findBackendExe() {
  const macOsBuildDir = path.join(BACKEND_DIR, 'Mac OS', 'build');
  const candidates = isWindows
    ? [
        path.join(BUILD_DIR, 'Release', 'speakerlearn_serverd.exe'),
        path.join(BUILD_DIR, 'Debug', 'speakerlearn_serverd.exe'),
        path.join(BUILD_DIR, 'speakerlearn_serverd.exe'),
      ]
    : [
        path.join(macOsBuildDir, 'speakerlearn_serverd'),
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
  console.log('[run] Configuring CMake...');
  execSync('cmake ..', { cwd: BUILD_DIR, stdio: 'inherit' });
  console.log('[run] Building...');
  const config = isWindows ? 'Release' : 'Release';
  execSync(`cmake --build . --config ${config}`, { cwd: BUILD_DIR, stdio: 'inherit' });
}

function startBackend() {
  const exe = findBackendExe();
  if (!exe) {
    console.log('[run] Backend not built. Building now...');
    buildBackend();
    const built = findBackendExe();
    if (!built) {
      console.error('[run] Build succeeded but executable not found.');
      process.exit(1);
    }
    return spawn(built, [], {
      cwd: BACKEND_DIR,
      stdio: 'pipe',
      detached: !isWindows,
    });
  }
  console.log(`[run] Backend executable: ${exe}`);
  return spawn(exe, [], {
    cwd: BACKEND_DIR,
    stdio: 'pipe',
    detached: !isWindows,
  });
}

const MIME = {
  '.html': 'text/html',
  '.css': 'text/css',
  '.js': 'application/javascript',
  '.json': 'application/json',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.jpeg': 'image/jpeg',
  '.gif': 'image/gif',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon',
  '.woff': 'font/woff',
  '.woff2': 'font/woff2',
};

function serveFile(filePath) {
  const ext = path.extname(filePath);
  return {
    'Content-Type': MIME[ext] || 'application/octet-stream',
  };
}

let bridge;
try {
  bridge = require('./bridge/audioBridge');
} catch (e) {
  console.warn('[run] Bridge unavailable:', e.message, '— Run "npm install" to enable SpeakEasy Bridge.');
  bridge = null;
}

function sendJson(res, status, obj) {
  res.writeHead(status, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify(obj));
}

function parseBody(req) {
  return new Promise((resolve) => {
    let body = '';
    req.on('data', (c) => { body += c; });
    req.on('end', () => {
      try {
        resolve(body ? JSON.parse(body) : {});
      } catch {
        resolve({});
      }
    });
  });
}

const server = http.createServer(async (req, res) => {
  const urlPath = (req.url || '/').split('?')[0];

  if (urlPath.startsWith('/bridge/')) {
    res.setHeader('Access-Control-Allow-Origin', '*');
    if (req.method === 'OPTIONS') {
      res.writeHead(204);
      res.end();
      return;
    }
    if (!bridge) {
      return sendJson(res, 503, {
        ok: false,
        error: 'Bridge not available. Run "npm install" in the project folder, then restart the app.'
      });
    }
    try {
      if (urlPath === '/bridge/devices' && req.method === 'GET') {
        const { inputs, outputs } = bridge.getDevices();
        return sendJson(res, 200, { inputs, outputs });
      }
      if (urlPath === '/bridge/status' && req.method === 'GET') {
        return sendJson(res, 200, bridge.getStatus());
      }
      if (urlPath === '/bridge/start' && req.method === 'POST') {
        const body = await parseBody(req);
        const result = bridge.startBridge(body);
        return sendJson(res, 200, result);
      }
      if (urlPath === '/bridge/stop' && req.method === 'POST') {
        const result = await bridge.stopBridge();
        return sendJson(res, 200, result);
      }
      if (urlPath === '/bridge/eq' && req.method === 'POST') {
        const body = await parseBody(req);
        const result = bridge.updateEq(body.eqGains);
        return sendJson(res, 200, result);
      }
      sendJson(res, 404, { error: 'Not found' });
    } catch (err) {
      sendJson(res, 500, { error: err.message || String(err) });
    }
    return;
  }

  const filePath = path.join(FRONTEND_DIR, urlPath === '/' ? 'homePage.html' : urlPath);
  if (!filePath.startsWith(FRONTEND_DIR)) {
    res.writeHead(403);
    res.end();
    return;
  }

  fs.readFile(filePath, (err, data) => {
    if (err) {
      if (err.code === 'ENOENT') {
        res.writeHead(404);
        res.end('Not found');
      } else {
        res.writeHead(500);
        res.end('Server error');
      }
      return;
    }
    res.writeHead(200, serveFile(filePath));
    res.end(data);
  });
});

let backendProc;

function main() {
  process.chdir(ROOT);

  backendProc = startBackend();
  backendProc.on('error', (err) => {
    console.error('[run] Backend failed to start:', err.message);
    process.exit(1);
  });
  backendProc.stderr?.on('data', (d) => process.stderr.write(d));
  backendProc.stdout?.on('data', (d) => process.stdout.write(d));

  // Give backend time to bind
  setTimeout(() => {
    server.listen(FRONTEND_PORT, () => {
      console.log();
      console.log('='.repeat(50));
      console.log('  SpeakEasy — Local Test Runner');
      console.log('='.repeat(50));
      console.log(`  Frontend:  http://localhost:${FRONTEND_PORT}/homePage.html`);
      console.log(`  Backend:   http://localhost:${BACKEND_PORT}/`);
      console.log(`  Bridge:    ${bridge ? 'enabled' : 'disabled (run npm install)'}`);
      console.log('='.repeat(50));
      console.log('  Press Ctrl+C to stop both servers');
      console.log();
    });
  }, 800);
}

function shutdown() {
  console.log('\n[run] Shutting down...');
  if (backendProc && !backendProc.killed) {
    backendProc.kill();
  }
  server.close();
  process.exit(0);
}

process.on('SIGINT', shutdown);
process.on('SIGTERM', shutdown);

main();
