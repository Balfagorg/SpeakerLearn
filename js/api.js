/**
 * FreqSync API — Backend integration with localStorage fallback
 * Preserves full functionality when backend is unavailable
 */
const API = {
  baseUrl: 'http://localhost:8001',
  userId: 'default-user',
  SPEAKERS_STORAGE_KEY: 'freqsync_speakers',
  ACTIVE_SPEAKER_KEY: 'freqsync_active_speaker',

  /** Returns unified speaker list: localStorage + API sync. Use for all dropdowns. */
  async getAllSpeakers() {
    let speakers = [];
    try {
      const raw = localStorage.getItem(this.SPEAKERS_STORAGE_KEY);
      speakers = raw ? JSON.parse(raw) : [];
    } catch (_) {}
    try {
      const list = await this.speakerSystemsList();
      if (Array.isArray(list) && list.length > 0) {
        for (const s of list) {
          if (speakers.some(sp => sp.id === s.id)) continue;
          let readings = [];
          try {
            const cal = await this.speakerSystemsCalibration(s.id);
            if (Array.isArray(cal)) readings = cal;
          } catch (_) {}
          speakers.push({
            id: s.id,
            name: s.name || 'Unnamed',
            addedAt: s.created_at ? new Date(s.created_at).toLocaleDateString() : new Date().toLocaleDateString(),
            source: 'learn',
            readings
          });
        }
      }
    } catch (_) {}
    for (let i = 0; i < localStorage.length; i++) {
      const key = localStorage.key(i);
      if (key && key.startsWith('eq_')) {
        const name = key.replace('eq_', '');
        if (!speakers.some(s => s.source === 'know' && (s.eqPresetKey === key || s.id === key))) {
          speakers.push({
            id: key,
            name,
            addedAt: new Date().toLocaleDateString(),
            source: 'know',
            eqPresetKey: key
          });
        }
      }
    }
    return speakers;
  },

  /** Persist speakers to localStorage. Call after add/edit/delete. */
  saveSpeakers(speakers) {
    try {
      localStorage.setItem(this.SPEAKERS_STORAGE_KEY, JSON.stringify(speakers));
    } catch (_) {}
  },

  /** Get/set active speaker ID for preferences page */
  getActiveSpeaker() {
    return localStorage.getItem(this.ACTIVE_SPEAKER_KEY) || '';
  },
  setActiveSpeaker(id) {
    try {
      localStorage.setItem(this.ACTIVE_SPEAKER_KEY, id || '');
    } catch (_) {}
  },

  async getActiveSpeakerFromBackend() {
    try {
      const r = await fetch(this.baseUrl + '/speaker-systems/active?user_id=' + encodeURIComponent(this.userId));
      if (!r.ok) throw new Error('Get active speaker failed');
      const data = await r.json();
      const id = data.speaker_system_id || '';
      this.setActiveSpeaker(id);
      return id;
    } catch (_) {
      return this.getActiveSpeaker();
    }
  },

  async check() {
    try {
      const r = await fetch(this.baseUrl + '/', { method: 'GET' });
      return r.ok;
    } catch { return false; }
  },

  async speakerSystemsCreate(data) {
    try {
      const r = await fetch(this.baseUrl + '/speaker-systems/create', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ user_id: this.userId, ...data })
      });
      if (!r.ok) throw new Error((await r.json()).error || 'Create failed');
      return await r.json();
    } catch (e) { throw e; }
  },

  async speakerSystemsList() {
    try {
      const r = await fetch(this.baseUrl + '/speaker-systems/list?user_id=' + encodeURIComponent(this.userId));
      if (!r.ok) throw new Error('List failed');
      return await r.json();
    } catch (e) { throw e; }
  },

  async speakerSystemsCalibration(speakerSystemId) {
    try {
      const r = await fetch(this.baseUrl + '/speaker-systems/calibration?speaker_system_id=' + encodeURIComponent(speakerSystemId));
      if (!r.ok) throw new Error('Calibration fetch failed');
      return await r.json();
    } catch (e) { throw e; }
  },

  async speakerSystemsCalibrate(speakerSystemId, readings) {
    try {
      const r = await fetch(this.baseUrl + '/speaker-systems/calibrate', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ speaker_system_id: speakerSystemId, readings })
      });
      if (!r.ok) throw new Error('Calibrate failed');
      return await r.json();
    } catch (e) { throw e; }
  },

  async speakerSystemsUpdate(id, name) {
    try {
      const r = await fetch(this.baseUrl + '/speaker-systems/update', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ id, name })
      });
      if (!r.ok) throw new Error((await r.json()).error || 'Update failed');
      return await r.json();
    } catch (e) { throw e; }
  },

  async speakerSystemsCalibrateReplace(speakerSystemId, readings) {
    try {
      const r = await fetch(this.baseUrl + '/speaker-systems/calibrate-replace', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ speaker_system_id: speakerSystemId, readings })
      });
      if (!r.ok) throw new Error('Calibrate replace failed');
      return await r.json();
    } catch (e) { throw e; }
  },

  async speakerSystemsSwitch(speakerSystemId) {
    try {
      const r = await fetch(this.baseUrl + '/speaker-systems/switch', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ user_id: this.userId, speaker_system_id: speakerSystemId || '' })
      });
      if (!r.ok) throw new Error('Switch failed');
      return await r.json();
    } catch (e) { throw e; }
  },

  async speakerSystemsDelete(id) {
    try {
      const r = await fetch(this.baseUrl + '/speaker-systems/delete', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ id })
      });
      if (!r.ok) throw new Error('Delete failed');
      return await r.json();
    } catch (e) { throw e; }
  },

  async eqPresetsList() {
    try {
      const r = await fetch(this.baseUrl + '/eq-presets/list?user_id=' + encodeURIComponent(this.userId));
      if (!r.ok) throw new Error('List presets failed');
      return await r.json();
    } catch (e) { throw e; }
  },

  async eqPresetsSave(data) {
    try {
      const r = await fetch(this.baseUrl + '/eq-presets/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ user_id: this.userId, ...data })
      });
      if (!r.ok) throw new Error((await r.json()).error || 'Save preset failed');
      return await r.json();
    } catch (e) { throw e; }
  },

  async eqPresetsDelete(id) {
    try {
      const r = await fetch(this.baseUrl + '/eq-presets/delete', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ id })
      });
      if (!r.ok) throw new Error('Delete preset failed');
      return await r.json();
    } catch (e) { throw e; }
  },

  async getAudioOutputDevices() {
    if (!navigator.mediaDevices?.enumerateDevices) return [];
    const devices = await navigator.mediaDevices.enumerateDevices();
    const outputs = devices.filter(d => d.kind === 'audiooutput');
    return outputs.map((d, i) => ({
      deviceId: d.deviceId || '',
      label: d.label || (d.deviceId ? 'Output ' + (i + 1) : 'Default output')
    }));
  },

  async requestDeviceLabels() {
    if (!navigator.mediaDevices?.getUserMedia) return;
    try {
      const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
      stream.getTracks().forEach(t => t.stop());
    } catch (_) {}
  },

  /** POST /audio/optimize — degradation compensation (source + issues + spectrum + prefs) */
  async audioOptimize(samples, preferences, platformHint, speakerCalibration, speakerSystemId) {
    try {
      const r = await fetch(this.baseUrl + '/audio/optimize', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          samples: samples || [],
          preferences: preferences || {},
          platform_hint: platformHint || 'local',
          speaker_calibration: speakerCalibration || {},
          speaker_system_id: speakerSystemId || ''
        })
      });
      if (!r.ok) throw new Error((await r.json()).error || 'Optimize failed');
      return await r.json();
    } catch (e) { throw e; }
  }
};
