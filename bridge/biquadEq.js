/**
 * Biquad peaking EQ — matches Brennan 10-band format
 * Used by the audio bridge for real-time EQ processing
 */
const FREQ_BANDS = [32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000];
const Q_VALUES = [1.0, 1.0, 1.2, 1.2, 1.4, 1.4, 1.4, 1.4, 1.2, 1.0];

class Biquad {
  constructor() {
    this.b0 = 1; this.b1 = 0; this.b2 = 0;
    this.a1 = 0; this.a2 = 0;
    this.z1 = 0; this.z2 = 0;
  }

  setPeaking(sampleRate, freq, Q, gainDB) {
    const A = Math.pow(10, gainDB / 40);
    const w0 = 2 * Math.PI * freq / sampleRate;
    const alpha = Math.sin(w0) / (2 * Q);

    const b0 = 1 + alpha * A;
    const b1 = -2 * Math.cos(w0);
    const b2 = 1 - alpha * A;
    const a0 = 1 + alpha / A;
    const a1 = -2 * Math.cos(w0);
    const a2 = 1 - alpha / A;

    this.b0 = b0 / a0;
    this.b1 = b1 / a0;
    this.b2 = b2 / a0;
    this.a1 = a1 / a0;
    this.a2 = a2 / a0;
  }

  process(inVal) {
    const out = inVal * this.b0 + this.z1;
    this.z1 = inVal * this.b1 - out * this.a1 + this.z2;
    this.z2 = inVal * this.b2 - out * this.a2;
    return out;
  }
}

function createEqChain(sampleRate, eqGains) {
  const gains = eqGains || {};
  return FREQ_BANDS.map((freq, i) => {
    const bq = new Biquad();
    const gain = parseFloat(gains[freq] ?? gains[String(freq)] ?? 0) || 0;
    bq.setPeaking(sampleRate, freq, Q_VALUES[i], gain);
    return bq;
  });
}

function createStereoEq(sampleRate, eqGains) {
  return {
    left: createEqChain(sampleRate, eqGains),
    right: createEqChain(sampleRate, eqGains)
  };
}

function processSample(filters, sample) {
  let s = sample;
  for (const f of filters) s = f.process(s);
  return s;
}

function processInterleavedStereo(stereoEq, buffer, numFrames) {
  for (let i = 0; i < numFrames; i++) {
    buffer[i * 2] = processSample(stereoEq.left, buffer[i * 2]);
    buffer[i * 2 + 1] = processSample(stereoEq.right, buffer[i * 2 + 1]);
  }
}

module.exports = {
  FREQ_BANDS,
  createEqChain,
  createStereoEq,
  processSample,
  processInterleavedStereo
};
