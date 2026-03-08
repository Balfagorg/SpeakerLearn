/**
 * SpeakEasy — Universal 7-band EQ constants and conversion utilities
 * Single source of truth for all pages (addSpeaker, preferences, playMusic)
 */
const BANDS_7 = [60, 170, 350, 1000, 3500, 8000, 16000];
const BANDS_7_LABELS = ['60 Hz', '170 Hz', '350 Hz', '1 kHz', '3.5 kHz', '8 kHz', '16 kHz'];

/** Learn flow 5 input steps: 60, 500, 1k, 3.5k, 3.5k+500 mix */
const LEARN_5_HZ = [60, 500, 1000, 3500, 3500];
const LEARN_5_LABELS = ['60 Hz', '500 Hz', '1,000 Hz', '3,500 Hz', '3.5k+500 Hz mix'];
const LEARN_5_NAMES = ['Sub-bass', 'Low midrange', 'Midrange', 'Upper midrange', 'Mix stress test'];

/** Convert Learn 5 readings (val 0–100, issue per band) to 7-band storage */
function learn5To7Band(readings) {
  if (!readings || readings.length < 5) return { bands: {}, issues: {} };
  const valToDb = (v) => ((50 - (v ?? 50)) / 50) * 8;
  const r = readings;
  const bands = {};
  const issues = {};
  bands[60] = valToDb(r[0]?.val);
  bands[170] = (valToDb(r[0]?.val) + valToDb(r[1]?.val)) / 2;
  bands[350] = (valToDb(r[0]?.val) + valToDb(r[1]?.val)) / 2;
  bands[500] = valToDb(r[1]?.val);
  bands[1000] = valToDb(r[2]?.val);
  bands[3500] = valToDb(r[3]?.val);
  const mixVal = valToDb(r[4]?.val ?? 50);
  bands[8000] = mixVal;
  bands[16000] = mixVal;
  issues[60] = r[0]?.issue || 'none';
  issues[170] = r[0]?.issue || 'none';
  issues[350] = r[1]?.issue || 'none';
  issues[500] = r[1]?.issue || 'none';
  issues[1000] = r[2]?.issue || 'none';
  issues[3500] = r[3]?.issue || 'none';
  issues[8000] = r[4]?.issue || 'none';
  issues[16000] = r[4]?.issue || 'none';
  return { bands, issues };
}

/** Convert 7-band stored data back to 5 readings for Learn edit modal */
function sevenBandToLearn5(bands, issues) {
  const dbToVal = (db) => Math.round(50 - (db * 50) / 8);
  const b = bands || {};
  const i = issues || {};
  return [
    { label: LEARN_5_LABELS[0], name: LEARN_5_NAMES[0], val: dbToVal(b[60] ?? 0), issue: i[60] || 'none' },
    { label: LEARN_5_LABELS[1], name: LEARN_5_NAMES[1], val: dbToVal(b[500] ?? 0), issue: i[500] || 'none' },
    { label: LEARN_5_LABELS[2], name: LEARN_5_NAMES[2], val: dbToVal(b[1000] ?? 0), issue: i[1000] || 'none' },
    { label: LEARN_5_LABELS[3], name: LEARN_5_NAMES[3], val: dbToVal(b[3500] ?? 0), issue: i[3500] || 'none' },
    { label: LEARN_5_LABELS[4], name: LEARN_5_NAMES[4], val: dbToVal(b[8000] ?? b[16000] ?? 0), issue: i[8000] || i[16000] || 'none' }
  ];
}

/** Migration: if Learn speaker has old 5 readings (no bands_7), convert to 7-band */
function migrateLearnTo7Band(readings) {
  if (!readings || readings.length < 5) return { bands: {}, issues: {} };
  return learn5To7Band(readings);
}

/** Platform options for source selector */
const PLATFORM_OPTIONS = [
  { value: 'spotify', label: 'Spotify' },
  { value: 'youtube', label: 'YouTube' },
  { value: 'apple_music', label: 'Apple Music' },
  { value: 'local', label: 'Local files' }
];
