import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const CONFIG_PATH = path.join(__dirname, '..', 'config.json');

const DEFAULT_CONFIG = {
  handy: {
    connectionKey: '',
    appKey: '',
    zone: {
      enabled: false,
      outerMin: 0, // 0-100% of full travel - overall allowed range
      outerMax: 100,
      subZoneWidth: 40, // 0-100% - width of the actively-used band within the outer range
      intervalSeconds: 15, // how often to pick a new random sub-zone
      pauseSeconds: 1, // how long to pause movement before applying each shift
    },
  },
};

export function loadConfig() {
  try {
    const raw = fs.readFileSync(CONFIG_PATH, 'utf-8');
    const parsed = JSON.parse(raw);
    return {
      ...DEFAULT_CONFIG,
      ...parsed,
      handy: {
        ...DEFAULT_CONFIG.handy,
        ...(parsed.handy || {}),
        zone: { ...DEFAULT_CONFIG.handy.zone, ...(parsed.handy?.zone || {}) },
      },
    };
  } catch {
    return structuredClone(DEFAULT_CONFIG);
  }
}

export function saveConfig(config) {
  fs.writeFileSync(CONFIG_PATH, JSON.stringify(config, null, 2));
}
