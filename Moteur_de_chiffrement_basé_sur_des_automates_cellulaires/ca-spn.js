'use strict';

const DEFAULT_SBOX = new Uint8Array([
  0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
  0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
  0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
  0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
  0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
  0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
  0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
  0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
  0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
  0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
  0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
  0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
  0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
  0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
  0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
  0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
]);

const INV_SBOX = (() => {
  const t = new Uint8Array(256);
  for (let i = 0; i < 256; i++) t[DEFAULT_SBOX[i]] = i;
  return t;
})();

function buildCustomSBox(seed) {
  const s = new Uint8Array(256);
  for (let i = 0; i < 256; i++) s[i] = i;
  let x = seed >>> 0;
  for (let i = 255; i > 0; i--) {
    x = (Math.imul(x, 1664525) + 1013904223) >>> 0;
    const j = x % (i + 1);
    const t = s[i]; s[i] = s[j]; s[j] = t;
  }
  const inv = new Uint8Array(256);
  for (let i = 0; i < 256; i++) inv[s[i]] = i;
  return { sbox: s, inv };
}

function xmul(a, b) {
  let p = 0;
  for (let i = 0; i < 8; i++) {
    if (b & 1) p ^= a;
    const hb = a & 0x80;
    a = (a << 1) & 0xff;
    if (hb) a ^= 0x1b;
    b >>= 1;
  }
  return p;
}

function mixColumns(block) {
  const o = new Uint8Array(16);
  for (let c = 0; c < 4; c++) {
    const a=block[c], b=block[4+c], cc=block[8+c], d=block[12+c];
    o[c]    = xmul(a,2)^xmul(b,3)^cc^d;
    o[4+c]  = a^xmul(b,2)^xmul(cc,3)^d;
    o[8+c]  = a^b^xmul(cc,2)^xmul(d,3);
    o[12+c] = xmul(a,3)^b^cc^xmul(d,2);
  }
  return o;
}

function invMixColumns(block) {
  const o = new Uint8Array(16);
  for (let c = 0; c < 4; c++) {
    const a=block[c], b=block[4+c], cc=block[8+c], d=block[12+c];
    o[c]    = xmul(a,0x0e)^xmul(b,0x0b)^xmul(cc,0x0d)^xmul(d,0x09);
    o[4+c]  = xmul(a,0x09)^xmul(b,0x0e)^xmul(cc,0x0b)^xmul(d,0x0d);
    o[8+c]  = xmul(a,0x0d)^xmul(b,0x09)^xmul(cc,0x0e)^xmul(d,0x0b);
    o[12+c] = xmul(a,0x0b)^xmul(b,0x0d)^xmul(cc,0x09)^xmul(d,0x0e);
  }
  return o;
}

function shiftRows(b) {
  return new Uint8Array([
    b[0], b[5], b[10], b[15],
    b[4], b[9], b[14], b[3],
    b[8], b[13], b[2], b[7],
    b[12], b[1], b[6], b[11]
  ]);
}

function invShiftRows(b) {
  return new Uint8Array([
    b[0], b[13], b[10], b[7],
    b[4], b[1], b[14], b[11],
    b[8], b[5], b[2], b[15],
    b[12], b[9], b[6], b[3]
  ]);
}

function xorBlock(a, b) {
  const o = new Uint8Array(16);
  for (let i = 0; i < 16; i++) o[i] = a[i] ^ b[i];
  return o;
}

class CellularAutomaton {
  constructor(rows, cols, rule, moore, toroidal) {
    this.rows = rows; this.cols = cols;
    this.rule = rule >>> 0;
    this.moore = moore; this.toroidal = toroidal;
    this.grid = new Uint8Array(rows * cols);
  }

  seed(bytes) {
    this.grid.fill(0);
    for (let i = 0; i < Math.min(bytes.length, 16); i++)
      for (let b = 0; b < 8; b++) {
        const idx = i * 8 + b;
        if (idx < this.grid.length) this.grid[idx] = (bytes[i] >> (7 - b)) & 1;
      }
  }

  step() {
    const D = this.moore
      ? [[-1,-1],[-1,0],[-1,1],[0,-1],[0,1],[1,-1],[1,0],[1,1]]
      : [[-1,0],[1,0],[0,-1],[0,1]];
    const next = new Uint8Array(this.rows * this.cols);
    for (let r = 0; r < this.rows; r++) {
      for (let c = 0; c < this.cols; c++) {
        let n = 0;
        for (const [dr, dc] of D) {
          const nr = r + dr, nc = c + dc;
          if (this.toroidal) {
            n += this.grid[((nr + this.rows) % this.rows) * this.cols + (nc + this.cols) % this.cols];
          } else if (nr >= 0 && nr < this.rows && nc >= 0 && nc < this.cols) {
            n += this.grid[nr * this.cols + nc];
          }
        }
        const cell = this.grid[r * this.cols + c];
        next[r * this.cols + c] = (this.rule >> ((cell * (D.length + 1) + n) % 32)) & 1;
      }
    }
    this.grid = next;
  }

  run(steps) { for (let i = 0; i < steps; i++) this.step(); }

  get16() {
    const o = new Uint8Array(16);
    for (let i = 0; i < Math.min(128, this.grid.length); i++)
      o[i >> 3] |= this.grid[i] << (7 - (i & 7));
    return o;
  }
}

function caKeystream(seed16, rule, rows, cols, steps, moore, toroidal) {
  const ca = new CellularAutomaton(rows, cols, rule, moore, toroidal);
  ca.seed(seed16);
  ca.run(steps);
  return ca.get16();
}

function prng32(arr, salt) {
  let h = (salt ^ 0xdeadcafe) >>> 0;
  for (const b of arr) { h ^= b; h = (Math.imul(h, 0x9e3779b9) + (h << 6) + (h >> 2)) >>> 0; }
  return h;
}

function deriveRoundKeys(keyBytes, rounds) {
  const norm = new Uint8Array(32);
  for (let i = 0; i < 32; i++) norm[i] = keyBytes[i % keyBytes.length] ^ (i * 0x36 & 0xff);
  const keys = [];
  let state = norm.slice();
  for (let r = 0; r <= rounds; r++) {
    const k = new Uint8Array(16);
    for (let i = 0; i < 16; i++) k[i] = (state[(i * 7 + r * 3) % 32] ^ prng32(state, r * 19 + i)) & 0xff;
    keys.push(k);
    const next = new Uint8Array(32);
    for (let i = 0; i < 32; i++) next[i] = (state[(i + 7) % 32] ^ prng32(state, r * 37 + i)) & 0xff;
    state = next;
  }
  return keys;
}

function deriveCaRule(keyBytes, seed) {
  if (seed > 0) return seed >>> 0;
  let h = 0x811c9dc5 >>> 0;
  for (const b of keyBytes) { h ^= b; h = Math.imul(h, 0x01000193) >>> 0; }
  return h;
}

function normalizeKey(key) {
  if (typeof key === 'string') return new TextEncoder().encode(key);
  if (key instanceof Uint8Array) return key;
  if (Array.isArray(key)) return new Uint8Array(key);
  throw new Error('key: string, Uint8Array ou Array');
}

function padPKCS7(data) {
  const pad = 16 - (data.length % 16);
  const out = new Uint8Array(data.length + pad);
  out.set(data); out.fill(pad, data.length);
  return out;
}

function unpadPKCS7(data) {
  const pad = data[data.length - 1];
  if (pad < 1 || pad > 16) throw new Error('Padding invalide');
  for (let i = data.length - pad; i < data.length; i++)
    if (data[i] !== pad) throw new Error('Padding invalide');
  return data.slice(0, data.length - pad);
}

function generateIV() {
  const iv = new Uint8Array(16);
  if (typeof crypto !== 'undefined' && crypto.getRandomValues) crypto.getRandomValues(iv);
  else { let s = Date.now()>>>0; for(let i=0;i<16;i++){s=(Math.imul(s,1664525)+1013904223)>>>0;iv[i]=s&0xff;} }
  return iv;
}

class CASPNCipher {
  constructor(opts = {}) {
    this.rounds       = opts.rounds       ?? 10;
    this.neighborhood = opts.neighborhood ?? 'moore';
    this.toroidal     = opts.toroidal     ?? true;
    this.caSteps      = opts.caSteps      ?? 3;
    this.mode         = opts.mode         ?? 'cbc';
    this.sboxMode     = opts.sboxMode     ?? 'aes';
    this.sboxSeed     = opts.sboxSeed     ?? 0;
    this.caRows       = opts.caRows       ?? 4;
    this.caCols       = opts.caCols       ?? 32;
    this._validate();
    if (this.sboxMode === 'custom' && this.sboxSeed > 0) {
      const b = buildCustomSBox(this.sboxSeed);
      this.sbox = b.sbox; this.invSbox = b.inv;
    } else { this.sbox = DEFAULT_SBOX; this.invSbox = INV_SBOX; }
    this._moore = this.neighborhood === 'moore';
  }

  _validate() {
    if (this.rounds < 1 || this.rounds > 64) throw new Error('rounds: 1–64');
    if (!['moore','vonneumann'].includes(this.neighborhood)) throw new Error('neighborhood: moore|vonneumann');
    if (this.caSteps < 1 || this.caSteps > 32) throw new Error('caSteps: 1–32');
    if (!['cbc','ctr','ecb'].includes(this.mode)) throw new Error('mode: cbc|ctr|ecb');
    if (!['aes','custom'].includes(this.sboxMode)) throw new Error('sboxMode: aes|custom');
  }

  _ctx(key) {
    const kb = normalizeKey(key);
    return { rk: deriveRoundKeys(kb, this.rounds), rule: deriveCaRule(kb, this.sboxSeed) };
  }

  _caKS(seed16, rule, round) {
    return caKeystream(seed16, (rule ^ round) >>> 0, this.caRows, this.caCols, this.caSteps, this._moore, this.toroidal);
  }

  _encBlock(blk, rk, rule) {
    let s = xorBlock(blk, rk[0]);
    for (let r = 1; r <= this.rounds; r++) {
      s = new Uint8Array(s.map(b => this.sbox[b]));
      s = shiftRows(s);
      if (r < this.rounds) s = mixColumns(s);
      const ks = this._caKS(rk[r], rule, r);
      s = xorBlock(s, ks);
      s = xorBlock(s, rk[r]);
    }
    return s;
  }

  _decBlock(blk, rk, rule) {
    let s = blk;
    for (let r = this.rounds; r >= 1; r--) {
      s = xorBlock(s, rk[r]);
      const ks = this._caKS(rk[r], rule, r);
      s = xorBlock(s, ks);
      if (r < this.rounds) s = invMixColumns(s);
      s = invShiftRows(s);
      s = new Uint8Array(s.map(b => this.invSbox[b]));
    }
    s = xorBlock(s, rk[0]);
    return s;
  }

  _ctrInc(iv, n) {
    const c = new Uint8Array(iv); let carry = n;
    for (let i = 15; i >= 0 && carry > 0; i--) { const v = c[i]+(carry&0xff); c[i]=v&0xff; carry=(carry>>8)+(v>>8); }
    return c;
  }

  encrypt(plaintext, key, iv) {
    const { rk, rule } = this._ctx(key);
    const useIV = (iv instanceof Uint8Array && iv.length === 16) ? iv : generateIV();
    let data;
    if (typeof plaintext === 'string') data = new TextEncoder().encode(plaintext);
    else if (plaintext instanceof Uint8Array) data = plaintext;
    else throw new Error('plaintext: string ou Uint8Array');

    if (this.mode === 'ctr') {
      const out = new Uint8Array(16 + data.length); out.set(useIV);
      for (let i = 0; i < data.length; i += 16) {
        const ks = this._encBlock(this._ctrInc(useIV, i/16|0), rk, rule);
        for (let j = 0; j < Math.min(16, data.length-i); j++) out[16+i+j] = data[i+j] ^ ks[j];
      }
      return out;
    }

    const padded = padPKCS7(data);
    if (this.mode === 'ecb') {
      const out = new Uint8Array(padded.length);
      for (let i = 0; i < padded.length; i += 16) out.set(this._encBlock(padded.slice(i,i+16), rk, rule), i);
      return out;
    }
    const out = new Uint8Array(16 + padded.length); out.set(useIV);
    let prev = useIV;
    for (let i = 0; i < padded.length; i += 16) {
      const enc = this._encBlock(xorBlock(padded.slice(i,i+16), prev), rk, rule);
      out.set(enc, 16+i); prev = enc;
    }
    return out;
  }

  decrypt(ciphertext, key) {
    if (!(ciphertext instanceof Uint8Array)) throw new Error('ciphertext: Uint8Array');
    const { rk, rule } = this._ctx(key);

    if (this.mode === 'ctr') {
      const iv = ciphertext.slice(0,16), data = ciphertext.slice(16);
      const out = new Uint8Array(data.length);
      for (let i = 0; i < data.length; i += 16) {
        const ks = this._encBlock(this._ctrInc(iv, i/16|0), rk, rule);
        for (let j = 0; j < Math.min(16, data.length-i); j++) out[i+j] = data[i+j]^ks[j];
      }
      return out;
    }

    if (this.mode === 'ecb') {
      if (ciphertext.length % 16 !== 0) throw new Error('Longueur invalide');
      const out = new Uint8Array(ciphertext.length);
      for (let i = 0; i < ciphertext.length; i += 16) out.set(this._decBlock(ciphertext.slice(i,i+16), rk, rule), i);
      return unpadPKCS7(out);
    }

    if (ciphertext.length < 32 || (ciphertext.length-16) % 16 !== 0) throw new Error('Longueur invalide');
    const iv = ciphertext.slice(0,16), data = ciphertext.slice(16);
    const out = new Uint8Array(data.length); let prev = iv;
    for (let i = 0; i < data.length; i += 16) {
      const blk = data.slice(i,i+16);
      out.set(xorBlock(this._decBlock(blk, rk, rule), prev), i);
      prev = blk;
    }
    return unpadPKCS7(out);
  }

  encryptToBase64(p, key, iv) {
    const b = this.encrypt(p, key, iv);
    return typeof Buffer !== 'undefined' ? Buffer.from(b).toString('base64') : btoa(String.fromCharCode(...b));
  }

  decryptFromBase64(b64, key) {
    const b = typeof Buffer !== 'undefined' ? new Uint8Array(Buffer.from(b64,'base64')) : new Uint8Array(atob(b64).split('').map(c=>c.charCodeAt(0)));
    return this.decrypt(b, key);
  }

  encryptToHex(p, key, iv) {
    return Array.from(this.encrypt(p, key, iv)).map(b=>b.toString(16).padStart(2,'0')).join('');
  }

  decryptFromHex(hex, key) {
    return this.decrypt(new Uint8Array(hex.match(/.{2}/g).map(h=>parseInt(h,16))), key);
  }

  decryptToString(ct, key) {
    const b = typeof ct === 'string' ? this.decryptFromBase64(ct, key) : this.decrypt(ct, key);
    return new TextDecoder().decode(b);
  }

  getConfig() {
    return { rounds:this.rounds, mode:this.mode, neighborhood:this.neighborhood, caSteps:this.caSteps, caRows:this.caRows, caCols:this.caCols, toroidal:this.toroidal, sboxMode:this.sboxMode, sboxSeed:this.sboxSeed };
  }

  clone(ov={}) { return new CASPNCipher({ ...this.getConfig(), ...ov }); }
}

function createCipher(options) { return new CASPNCipher(options); }
function quickEncrypt(p, key, opts={}) { return new CASPNCipher(opts).encryptToBase64(p, key); }
function quickDecrypt(c, key, opts={}) { return new CASPNCipher(opts).decryptToString(c, key); }

function benchmark(options={}) {
  const cipher = new CASPNCipher(options);
  const key = 'benchmark-key-32bytes!!!!!!!!!!!';
  const data = new Uint8Array(1024).fill(0x41);
  const N = 5;
  const t0 = Date.now(); let enc;
  for (let i = 0; i < N; i++) enc = cipher.encrypt(data, key);
  const eT = (Date.now()-t0)/N;
  const t1 = Date.now();
  for (let i = 0; i < N; i++) cipher.decrypt(enc, key);
  const dT = (Date.now()-t1)/N;
  return { encryptMs: eT.toFixed(1), decryptMs: dT.toFixed(1), config: cipher.getConfig() };
}

if (typeof module !== 'undefined' && module.exports)
  module.exports = { CASPNCipher, createCipher, quickEncrypt, quickDecrypt, benchmark };
else if (typeof window !== 'undefined') {
  window.CASPNCipher = CASPNCipher;
  window.caSpn = { createCipher, quickEncrypt, quickDecrypt, benchmark };
}
