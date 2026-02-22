# CA-SPN Cipher 

Moteur de chiffrement par blocs combinant un **Automate Cellulaire 2D** (couche de diffusion) et un réseau **Substitution-Permutation** (SPN). Compatible Node.js et navigateur.

---

## Installation

Copier `ca-spn.js` dans votre projet. Aucune dépendance externe.

```bash
cp ca-spn.js votre-projet/lib/
```

---

## Intégration dans un projet existant

### Node.js (CommonJS)

```js
const { CASPNCipher, createCipher, quickEncrypt, quickDecrypt } = require('./lib/ca-spn.js');
```

### Node.js (ES Module)

```js
// Renommer le fichier en ca-spn.mjs ou ajouter "type":"module" dans package.json
import { CASPNCipher, createCipher, quickEncrypt, quickDecrypt } from './lib/ca-spn.js';
```

### Navigateur (script tag)

```html
<script src="ca-spn.js"></script>
<script>
  const cipher = new CASPNCipher();
  // ou
  const { createCipher, quickEncrypt, quickDecrypt } = window.caSpn;
</script>
```

---

## Démarrage rapide

```js
const { quickEncrypt, quickDecrypt } = require('./ca-spn.js');

const cle = 'ma-cle-secrete';
const message = 'Bonjour le monde !';

const chiffre  = quickEncrypt(message, cle);
const dechiffre = quickDecrypt(chiffre, cle);

console.log(chiffre);   // base64
console.log(dechiffre); // "Bonjour le monde !"
```

---

## API complète

### `new CASPNCipher(options)`

Crée une instance du moteur de chiffrement avec les options souhaitées.

```js
const cipher = new CASPNCipher({
  rounds:       10,          // Nombre de rounds SPN (1–64)
  mode:         'cbc',       // Mode opératoire : 'cbc' | 'ctr' | 'ecb'
  neighborhood: 'moore',     // Voisinage CA : 'moore' | 'vonneumann'
  caSteps:      3,           // Générations CA par round (1–32)
  caRows:       4,           // Lignes de la grille CA
  caCols:       32,          // Colonnes de la grille CA
  toroidal:     true,        // Bords connectés (grille sans bords)
  sboxMode:     'aes',       // S-Box : 'aes' | 'custom'
  sboxSeed:     0            // Seed pour S-Box custom (entier > 0)
});
```

---

### Chiffrement

#### `.encrypt(plaintext, key, iv?)`
Retourne un `Uint8Array`. L'IV est généré aléatoirement si non fourni.
```js
const bytes = cipher.encrypt('message', 'cle');
const bytes = cipher.encrypt(new Uint8Array([1,2,3]), 'cle');
```

#### `.encryptToBase64(plaintext, key, iv?)`
Retourne une chaîne base64 (IV inclus dans le résultat).
```js
const b64 = cipher.encryptToBase64('message', 'cle');
```

#### `.encryptToHex(plaintext, key, iv?)`
Retourne une chaîne hexadécimale.
```js
const hex = cipher.encryptToHex('message', 'cle');
```

---

### Déchiffrement

#### `.decrypt(ciphertext, key)`
Accepte un `Uint8Array`, retourne un `Uint8Array`.
```js
const bytes = cipher.decrypt(cipherBytesArray, 'cle');
```

#### `.decryptToString(ciphertext, key)`
Accepte un `Uint8Array` ou base64, retourne une chaîne UTF-8.
```js
const texte = cipher.decryptToString(b64, 'cle');
```

#### `.decryptFromBase64(b64, key)`
Retourne un `Uint8Array`.
```js
const bytes = cipher.decryptFromBase64(b64, 'cle');
```

#### `.decryptFromHex(hex, key)`
Retourne un `Uint8Array`.
```js
const bytes = cipher.decryptFromHex(hex, 'cle');
```

---

### Utilitaires

#### `.getConfig()`
Retourne l'objet de configuration courante.
```js
const config = cipher.getConfig();
// { rounds: 10, mode: 'cbc', neighborhood: 'moore', ... }
```

#### `.clone(overrides)`
Crée une copie du cipher avec certains paramètres modifiés.
```js
const cipher2 = cipher.clone({ rounds: 20, mode: 'ctr' });
```

---

### Fonctions utilitaires standalone

#### `createCipher(options)`
Alias de `new CASPNCipher(options)`.
```js
const cipher = createCipher({ rounds: 16, mode: 'ctr' });
```

#### `quickEncrypt(plaintext, key, options?)`
Chiffrement en une ligne, retourne base64.
```js
const chiffre = quickEncrypt('message', 'cle', { rounds: 20 });
```

#### `quickDecrypt(ciphertext, key, options?)`
Déchiffrement en une ligne depuis base64, retourne string.
```js
const clair = quickDecrypt(chiffre, 'cle', { rounds: 20 });
```

#### `benchmark(options?)`
Mesure les performances pour une configuration donnée.
```js
const result = benchmark({ rounds: 10, caSteps: 3 });
// { encryptMs: '12.40', decryptMs: '13.10', throughputMBs: '0.08', config: {...} }
```

---

## Exemples d'intégration dans des architectures existantes

### Express.js — Middleware de chiffrement de cookies

```js
const { CASPNCipher } = require('./lib/ca-spn.js');
const cipher = new CASPNCipher({ mode: 'ctr', rounds: 12 });
const SECRET = process.env.COOKIE_SECRET;

app.use((req, res, next) => {
  const raw = req.cookies?.session;
  if (raw) {
    try {
      req.sessionData = JSON.parse(cipher.decryptToString(raw, SECRET));
    } catch {
      req.sessionData = null;
    }
  }
  res.encryptCookie = (name, data) => {
    const enc = cipher.encryptToBase64(JSON.stringify(data), SECRET);
    res.cookie(name, enc, { httpOnly: true });
  };
  next();
});
```

---

### React — Chiffrement de données locales

```js
import { createCipher } from './lib/ca-spn.js';

const cipher = createCipher({ mode: 'cbc', rounds: 10 });
const USER_KEY = 'user-derived-key-from-password';

function saveEncrypted(key, data) {
  const enc = cipher.encryptToBase64(JSON.stringify(data), USER_KEY);
  localStorage.setItem(key, enc);
}

function loadDecrypted(key) {
  const enc = localStorage.getItem(key);
  if (!enc) return null;
  return JSON.parse(cipher.decryptToString(enc, USER_KEY));
}
```

---

### API REST — Chiffrement de payload

```js
const { quickEncrypt, quickDecrypt } = require('./lib/ca-spn.js');
const API_KEY = process.env.API_CIPHER_KEY;

router.post('/secure-data', (req, res) => {
  const payload = quickEncrypt(JSON.stringify(req.body), API_KEY, { rounds: 14 });
  db.save({ data: payload });
  res.json({ ok: true });
});

router.get('/secure-data/:id', async (req, res) => {
  const row   = await db.find(req.params.id);
  const plain = quickDecrypt(row.data, API_KEY, { rounds: 14 });
  res.json(JSON.parse(plain));
});
```

---

### Chiffrement de fichier (Node.js)

```js
const fs = require('fs');
const { CASPNCipher } = require('./lib/ca-spn.js');

const cipher = new CASPNCipher({ mode: 'cbc', rounds: 16 });
const key    = 'cle-de-fichier-longue';

function encryptFile(inputPath, outputPath) {
  const data = fs.readFileSync(inputPath);
  const enc  = cipher.encrypt(new Uint8Array(data), key);
  fs.writeFileSync(outputPath, Buffer.from(enc));
}

function decryptFile(inputPath, outputPath) {
  const data = new Uint8Array(fs.readFileSync(inputPath));
  const dec  = cipher.decrypt(data, key);
  fs.writeFileSync(outputPath, Buffer.from(dec));
}
```

---

## CLI — Référence des commandes

```bash
# Chiffrer un texte
node ca-spn-cli.js encrypt --key "MaCle" --text "Mon message"

# Chiffrer en hex
node ca-spn-cli.js encrypt --key "MaCle" --text "Mon message" --format hex

# Chiffrer un fichier
node ca-spn-cli.js encrypt --key "MaCle" --input secret.txt --output secret.bin

# Déchiffrer un texte (base64)
node ca-spn-cli.js decrypt --key "MaCle" --text "<base64ici>"

# Déchiffrer un fichier
node ca-spn-cli.js decrypt --key "MaCle" --input secret.bin --output recupere.txt

# Paramètres avancés
node ca-spn-cli.js encrypt \
  --key "MaCle" \
  --text "Test" \
  --rounds 20 \
  --mode ctr \
  --ca-steps 5 \
  --neighborhood vonneumann \
  --sbox-mode custom \
  --sbox-seed 1337 \
  --verbose

# Benchmark
node ca-spn-cli.js benchmark --rounds 10 --ca-steps 3
node ca-spn-cli.js benchmark --rounds 20 --mode ctr

# Voir la config active
node ca-spn-cli.js info --rounds 16 --mode cbc --sbox-mode custom --sbox-seed 42

# Aide
node ca-spn-cli.js help
```

---

## Paramètres — Guide de choix

| Paramètre     | Valeur basse              | Valeur haute                   | Recommandé |
|---------------|---------------------------|--------------------------------|------------|
| `rounds`      | Rapide, moins sûr         | Lent, très sûr                 | 10–16      |
| `caSteps`     | Diffusion partielle       | Diffusion totale, lent         | 3–6        |
| `mode`        | ECB (déconseillé)         | CBC/CTR (recommandés)          | `cbc`      |
| `neighborhood`| vonneumann (4 voisins)    | moore (8 voisins, plus rapide) | `moore`    |
| `toroidal`    | false (bords fixes)       | true (diffusion uniforme)      | `true`     |
| `sboxMode`    | aes (standard)            | custom (seed secret)           | `aes`      |

---

## Notes de sécurité

- **Ne pas réutiliser la même clé et le même IV** en mode CBC.
- L'IV est automatiquement généré et inclus dans le chiffré. Pas besoin de le gérer manuellement.
- En mode ECB, chaque bloc identique produit un chiffré identique — à éviter pour des données structurées.
- Cette librairie est expérimentale. Pour des données de production critiques, préférer AES-256-GCM via les APIs crypto natives.
- La clé peut être n'importe quelle longueur (string ou Uint8Array) — elle est normalisée en interne sur 32 octets.

---

## Compatibilité

| Environnement       | Support |
|---------------------|---------|
| Node.js >= 14       | ✅      |
| Chrome / Firefox    | ✅      |
| Safari >= 13        | ✅      |
| Deno                | ✅      |
| Edge                | ✅      |
| IE 11               | ✗       |
