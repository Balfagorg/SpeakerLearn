/**
 * SpeakEasy API — Backend integration with localStorage fallback
 * Preserves full functionality when backend is unavailable
 */
const API = {
  baseUrl: 'http://localhost:8001',
  userId: 'default-user',

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
  }
};
