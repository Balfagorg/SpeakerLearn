/**
 * SpeakEasy — Electron main process
 * Spawns C++ backend, serves frontend + bridge API, opens app window.
 */

const { app, BrowserWindow } = require('electron');
const http = require('http');
const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');

const FRONTEND_PORT = 8080;
const BACKEND_PORT = 8001;
const isWindows = process.platform === 'win32';

// Paths: development vs packaged (app.getAppPath() works for both asar and unpacked)
const ROOT = app.isPackaged ? app.getAppPath() : path.join(__dirname, '..');
const FRONTEND_DIR = path.join(ROOT, 'frontend');
const BACKEND_BIN = app.isPackaged
  ? path.join(process.resourcesPath, 'bin', isWindows ? 'speakerlearn_serverd.exe' : 'speakerlearn_serverd')
  : getDevBackendExe();
const BACKEND_CWD = app.isPackaged ? app.getPath('userData') : path.join(ROOT, 'backend');

function getDevBackendExe() {
  const backendDir = path.join(__dirname, '..', 'backend');
  const buildDir = path.join(backendDir, 'build');
  const macBuild = path.join(backendDir, 'Mac OS', 'build');
  const candidates = isWindows
    ? [
        path.join(buildDir, 'Release', 'speakerlearn_serverd.exe'),
        path.join(buildDir, 'Debug', 'speakerlearn_serverd.exe'),
        path.join(buildDir, 'speakerlearn_serverd.exe'),
      ]
    : [
        path.join(macBuild, 'speakerlearn_serverd'),
        path.join(buildDir, 'speakerlearn_serverd'),
        path.join(buildDir, 'Release', 'speakerlearn_serverd'),
      ];
  for (const p of candidates) {
    if (fs.existsSync(p)) return p;
  }
  return null;
}

let backendProc;
let mainWindow;
let server;

const MIME = {
  '.html': 'text/html', '.css': 'text/css', '.js': 'application/javascript',
  '.json': 'application/json', '.png': 'image/png', '.jpg': 'image/jpeg',
  '.jpeg': 'image/jpeg', '.gif': 'image/gif', '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon', '.woff': 'font/woff', '.woff2': 'font/woff2',
};

let bridge;
try {
  bridge = require(path.join(ROOT, 'server', 'bridge', 'audioBridge'));
} catch (e) {
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
      try { resolve(body ? JSON.parse(body) : {}); } catch { resolve({}); }
    });
  });
}

function createServer() {
  return http.createServer(async (req, res) => {
    const urlPath = (req.url || '/').split('?')[0];

    if (urlPath.startsWith('/bridge/')) {
      res.setHeader('Access-Control-Allow-Origin', '*');
      if (req.method === 'OPTIONS') { res.writeHead(204); res.end(); return; }
      if (!bridge) {
        return sendJson(res, 503, { ok: false, error: 'Bridge not available.' });
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
          return sendJson(res, 200, bridge.startBridge(body));
        }
        if (urlPath === '/bridge/stop' && req.method === 'POST') {
          return sendJson(res, 200, await bridge.stopBridge());
        }
        if (urlPath === '/bridge/eq' && req.method === 'POST') {
          const body = await parseBody(req);
          return sendJson(res, 200, bridge.updateEq(body.eqGains));
        }
        sendJson(res, 404, { error: 'Not found' });
      } catch (err) {
        sendJson(res, 500, { error: err.message || String(err) });
      }
      return;
    }

    const filePath = path.join(FRONTEND_DIR, urlPath === '/' ? 'homePage.html' : urlPath);
    if (!filePath.startsWith(FRONTEND_DIR)) {
      res.writeHead(403); res.end(); return;
    }
    fs.readFile(filePath, (err, data) => {
      if (err) {
        res.writeHead(err.code === 'ENOENT' ? 404 : 500);
        res.end(err.code === 'ENOENT' ? 'Not found' : 'Server error');
        return;
      }
      const ext = path.extname(filePath);
      res.writeHead(200, { 'Content-Type': MIME[ext] || 'application/octet-stream' });
      res.end(data);
    });
  });
}

function startBackend() {
  if (!BACKEND_BIN || !fs.existsSync(BACKEND_BIN)) {
    console.error('[SpeakEasy] Backend not found. Run the build first.');
    return null;
  }
  return spawn(BACKEND_BIN, [], {
    cwd: BACKEND_CWD,
    stdio: 'pipe',
    detached: !isWindows,
  });
}

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1200,
    height: 800,
    webPreferences: { nodeIntegration: false, contextIsolation: true },
    // icon: optional — add frontend/favicon.ico for custom icon
  });
  mainWindow.loadURL(`http://localhost:${FRONTEND_PORT}/homePage.html`);
  mainWindow.on('closed', () => { mainWindow = null; });
}

app.whenReady().then(() => {
  backendProc = startBackend();
  if (!backendProc) {
    app.quit();
    return;
  }
  backendProc.on('error', (err) => {
    console.error('[SpeakEasy] Backend failed:', err.message);
    app.quit();
  });
  backendProc.stderr?.on('data', (d) => process.stderr.write(d));
  backendProc.stdout?.on('data', (d) => process.stdout.write(d));

  server = createServer();
  server.listen(FRONTEND_PORT, () => {
    createWindow();
  });
});

app.on('window-all-closed', () => {
  if (backendProc && !backendProc.killed) backendProc.kill();
  if (server) server.close();
  app.quit();
});

app.on('before-quit', () => {
  if (backendProc && !backendProc.killed) backendProc.kill();
  if (server) server.close();
});
