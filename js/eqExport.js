/**
 * SpeakEasy — EqualizerAPO export for system-wide EQ
 * Exports Brennan's 10-band EQ curves to EqualizerAPO config format
 * so the same EQ can be applied to Spotify, YouTube, and all system audio.
 */
(function (global) {
  const FREQ_BANDS = [32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000];
  const Q_VALUES = [1.0, 1.0, 1.2, 1.2, 1.4, 1.4, 1.4, 1.4, 1.2, 1.0];

  /**
   * Normalize EQ object to 10-band format.
   * Accepts { 32: dB, 64: dB, ... } or 7-band { 60: dB, 170: dB, ... }.
   */
  function normalizeTo10Band(eq) {
    if (!eq || typeof eq !== 'object') return {};
    const result = {};
    const inputFreqs = [...new Set(
      Object.keys(eq)
        .map(k => parseInt(k, 10))
        .filter(n => !isNaN(n) && (eq[n] ?? eq[String(n)]) !== undefined && (eq[n] ?? eq[String(n)]) !== null)
    )].sort((a, b) => a - b);

    FREQ_BANDS.forEach(f => {
      const direct = eq[f] ?? eq[String(f)];
      if (direct !== undefined && direct !== null) {
        result[f] = parseFloat(direct);
        return;
      }
      if (inputFreqs.length === 0) {
        result[f] = 0;
        return;
      }
      let lo = null, hi = null;
      for (const freq of inputFreqs) {
        if (freq <= f) lo = freq;
        if (freq >= f && hi === null) {
          hi = freq;
          break;
        }
      }
      const vLo = lo !== null ? parseFloat(eq[lo] ?? eq[String(lo)] ?? 0) : 0;
      const vHi = hi !== null ? parseFloat(eq[hi] ?? eq[String(hi)] ?? 0) : 0;
      if (lo === null) result[f] = vHi;
      else if (hi === null) result[f] = vLo;
      else if (lo === hi) result[f] = vLo;
      else result[f] = vLo + (vHi - vLo) * (f - lo) / (hi - lo);
    });
    return result;
  }

  /**
   * Generate EqualizerAPO config text from Brennan EQ object.
   * @param {Object} activeEQ - EQ in { 32: dB, 64: dB, ... } format
   * @returns {string} EqualizerAPO config file content
   */
  function exportToEqualizerAPO(activeEQ) {
    const eq = normalizeTo10Band(activeEQ);
    const lines = [
      '# SpeakEasy EQ — exported for EqualizerAPO',
      '# Apply this curve to Spotify, YouTube, and all system audio',
      '# Save to: C:\\Program Files\\EqualizerAPO\\config\\config.txt',
      '# Then restart EqualizerAPO or your audio device.',
      ''
    ];

    let maxGain = 0;
    FREQ_BANDS.forEach((freq, i) => {
      const gain = parseFloat(eq[freq] ?? 0) || 0;
      if (gain > maxGain) maxGain = gain;
    });

    if (maxGain > 0) {
      const preampDb = Math.min(-maxGain, -0.5);
      lines.push(`Preamp: ${preampDb.toFixed(1)} dB`);
      lines.push('');
    }

    FREQ_BANDS.forEach((freq, i) => {
      const gain = parseFloat(eq[freq] ?? 0) || 0;
      const q = Q_VALUES[i] ?? 1.2;
      lines.push(`Filter: ON PK Fc ${freq} Hz Gain ${gain >= 0 ? ' ' : ''}${gain.toFixed(1)} dB Q ${q.toFixed(2)}`);
    });

    return lines.join('\n');
  }

  /**
   * Trigger download of EqualizerAPO config file.
   * @param {Object} activeEQ - EQ object
   * @param {string} [filename='config.txt'] - Download filename
   */
  function downloadEqualizerAPOConfig(activeEQ, filename) {
    const content = exportToEqualizerAPO(activeEQ);
    const blob = new Blob([content], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename || 'config.txt';
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  }

  /**
   * Copy EqualizerAPO config to clipboard.
   * @param {Object} activeEQ - EQ object
   * @returns {Promise<boolean>} true if copied successfully
   */
  async function copyEqualizerAPOConfig(activeEQ) {
    const content = exportToEqualizerAPO(activeEQ);
    try {
      await navigator.clipboard.writeText(content);
      return true;
    } catch (_) {
      return false;
    }
  }

  global.EQExport = {
    exportToEqualizerAPO,
    downloadEqualizerAPOConfig,
    copyEqualizerAPOConfig,
    normalizeTo10Band
  };
})(typeof window !== 'undefined' ? window : this);
