#!/usr/bin/env node
/**
 * SpeakEasy Audio Bridge — captures from virtual cable, applies EQ, outputs to speakers.
 * Run as: node bridge/audioBridge.js
 * Or controlled via HTTP API from run.js
 *
 * Setup: Install VB-Audio Virtual Cable. Set system output to "CABLE Input".
 * This bridge captures from "CABLE Output" and forwards to your speakers with EQ.
 */
const { Transform } = require('stream');

let portAudio;
try {
  portAudio = require('naudiodon');
} catch (e) {
  console.warn('[bridge] naudiodon not installed. Run: npm install. Bridge will be unavailable.');
  throw e;
}

const { createStereoEq, processInterleavedStereo, updateStereoEqGains } = require('./biquadEq');

const SAMPLE_RATE = 48000;
const CHANNELS = 2;
const FRAMES_PER_BUFFER = 1024;

let bridgeState = {
  running: false,
  inputStream: null,
  outputStream: null,
  eqGains: {},
  inputDeviceId: -1,
  outputDeviceId: -1
};

function findDeviceByName(devices, nameSubstr) {
  const lower = (nameSubstr || '').toLowerCase();
  return devices.findIndex(d => (d.name || '').toLowerCase().includes(lower));
}

function getDevices() {
  const devices = portAudio.getDevices();
  const inputs = devices
    .filter(d => d.maxInputChannels > 0)
    .map((d, i) => ({ id: d.id, name: d.name, hostAPI: d.hostAPIName }));
  const outputs = devices
    .filter(d => d.maxOutputChannels > 0)
    .map((d, i) => ({ id: d.id, name: d.name, hostAPI: d.hostAPIName }));
  return { inputs, outputs, all: devices };
}

function createEqTransform(eqGains) {
  const stereoEq = createStereoEq(SAMPLE_RATE, eqGains);
  bridgeState._stereoEq = stereoEq;
  return new Transform({
    objectMode: false,
    transform(chunk, enc, cb) {
      const numSamples = chunk.length / 4;
      if (numSamples < CHANNELS) return cb(null, chunk);
      const buffer = new Float32Array(chunk.buffer, chunk.byteOffset, numSamples);
      const numFrames = Math.floor(numSamples / CHANNELS);
      processInterleavedStereo(stereoEq, buffer, numFrames);
      cb(null, Buffer.from(buffer.buffer, buffer.byteOffset, buffer.byteLength));
    }
  });
}

function startBridge(options = {}) {
  if (bridgeState.running) {
    return { ok: false, error: 'Bridge already running' };
  }

  const devices = portAudio.getDevices();
  const inputId = options.inputDeviceId ?? bridgeState.inputDeviceId ?? -1;
  const outputId = options.outputDeviceId ?? bridgeState.outputDeviceId ?? -1;
  const eqGains = options.eqGains ?? bridgeState.eqGains ?? {};

  bridgeState.eqGains = eqGains;
  bridgeState.inputDeviceId = inputId;
  bridgeState.outputDeviceId = outputId;

  try {
    const inOpts = {
      channelCount: CHANNELS,
      sampleFormat: portAudio.SampleFormatFloat32,
      sampleRate: SAMPLE_RATE,
      closeOnError: false,
      framesPerBuffer: FRAMES_PER_BUFFER
    };
    if (inputId >= 0) inOpts.deviceId = inputId;

    const outOpts = {
      channelCount: CHANNELS,
      sampleFormat: portAudio.SampleFormatFloat32,
      sampleRate: SAMPLE_RATE,
      closeOnError: false,
      framesPerBuffer: FRAMES_PER_BUFFER
    };
    if (outputId >= 0) outOpts.deviceId = outputId;

    const inputStream = portAudio.AudioIO({ inOptions: inOpts });
    const outputStream = portAudio.AudioIO({ outOptions: outOpts });

    const eqTransform = createEqTransform(eqGains);
    inputStream.pipe(eqTransform).pipe(outputStream);

    // Start output when first data arrives (avoids race; matches naudiodon restartTest pattern)
    inputStream.once('data', () => outputStream.start());
    inputStream.start();

    bridgeState.inputStream = inputStream;
    bridgeState.outputStream = outputStream;
    bridgeState.running = true;

    return { ok: true, message: 'Bridge started' };
  } catch (err) {
    return { ok: false, error: err.message || String(err) };
  }
}

/**
 * Stop the bridge. Returns a Promise that resolves when streams are fully closed.
 * Per naudiodon/PortAudio best practice: only quit the input stream (pipe propagates
 * to output). Quitting both causes double-quit and SIGSEGV in PaWasapi_ThreadPriorityRevert.
 */
function stopBridge() {
  if (!bridgeState.running) {
    return Promise.resolve({ ok: true, message: 'Bridge not running' });
  }
  const inputStream = bridgeState.inputStream;
  const outputStream = bridgeState.outputStream;
  bridgeState.inputStream = null;
  bridgeState.outputStream = null;
  bridgeState._stereoEq = null;
  bridgeState.running = false;

  if (!inputStream || !outputStream) {
    return Promise.resolve({ ok: true, message: 'Bridge stopped' });
  }

  return new Promise((resolve) => {
    let settled = false;
    const done = (err) => {
      if (settled) return;
      settled = true;
      clearTimeout(timer);
      if (err) resolve({ ok: false, error: err.message || String(err) });
      else resolve({ ok: true, message: 'Bridge stopped' });
    };
    const timer = setTimeout(() => done(), 5000);
    outputStream.once('finished', () => done());
    outputStream.once('error', (err) => done(err));
    try {
      inputStream.quit(() => {});
    } catch (err) {
      done(err);
    }
  });
}

function updateEq(eqGains) {
  bridgeState.eqGains = eqGains || {};
  if (bridgeState.running && bridgeState._stereoEq) {
    updateStereoEqGains(bridgeState._stereoEq, eqGains, SAMPLE_RATE);
  }
  return { ok: true };
}

function getStatus() {
  const { inputs, outputs } = getDevices();
  return {
    running: bridgeState.running,
    inputDeviceId: bridgeState.inputDeviceId,
    outputDeviceId: bridgeState.outputDeviceId,
    eqGains: bridgeState.eqGains,
    inputs,
    outputs
  };
}

if (require.main === module) {
  const args = process.argv.slice(2);
  if (args[0] === 'devices') {
    const { inputs, outputs } = getDevices();
    console.log('Input devices (use as "SpeakEasy" source):');
    inputs.forEach(d => console.log(`  ${d.id}: ${d.name}`));
    console.log('\nOutput devices:');
    outputs.forEach(d => console.log(`  ${d.id}: ${d.name}`));
    process.exit(0);
  }
  if (args[0] === 'start') {
    const result = startBridge({ inputDeviceId: -1, outputDeviceId: -1 });
    console.log(result.ok ? 'Bridge started' : 'Error: ' + result.error);
    if (result.ok) {
      process.on('SIGINT', () => {
        stopBridge().then(() => process.exit(0));
      });
    } else {
      process.exit(1);
    }
  }
}

module.exports = {
  getDevices,
  findDeviceByName,
  startBridge,
  stopBridge,
  updateEq,
  getStatus,
  bridgeState
};
