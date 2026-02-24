#!/usr/bin/env node
'use strict';

const { CASPNCipher, createCipher, benchmark } = require('./ca-spn.js');
const fs   = require('fs');
const path = require('path');

const COMMANDS = ['encrypt', 'decrypt', 'benchmark', 'info', 'help'];

function printBanner() {
  console.log(`
╔══════════════════════════════════════════════════════╗
║       CA-SPN CIPHER —                                ║
║       Automate Cellulaire + SPN                      ║
╚══════════════════════════════════════════════════════╝
`);
}

function printHelp() {
  printBanner();
  console.log(`USAGE:
  node ca-spn-cli.js <commande> [options]

COMMANDES:
  encrypt     Chiffrer un texte ou fichier
  decrypt     Déchiffrer un texte ou fichier
  benchmark   Mesurer les performances
  info        Afficher la configuration courante
  help        Afficher cette aide

OPTIONS GLOBALES:
  --rounds <n>          Nombre de rounds SPN           (défaut: 10, min: 1, max: 64)
  --mode <m>            Mode opératoire                (défaut: cbc)
                          cbc = Cipher Block Chaining
                          ctr = Counter Mode
                          ecb = Electronic Code Book
  --neighborhood <n>    Voisinage du CA                (défaut: moore)
                          moore      = 8 voisins
                          vonneumann = 4 voisins
  --ca-steps <n>        Générations CA par round       (défaut: 3, min: 1, max: 32)
  --ca-rows <n>         Lignes de la grille CA         (défaut: 4)
  --ca-cols <n>         Colonnes de la grille CA       (défaut: 32)
  --toroidal <b>        Grille toroïdale               (défaut: true)
  --sbox-mode <m>       Mode S-Box                     (défaut: aes)
                          aes    = S-Box standard AES
                          custom = S-Box dérivée du seed
  --sbox-seed <n>       Seed pour S-Box custom         (défaut: 0)
  --key <k>             Clé de chiffrement             (string)
  --key-file <f>        Fichier contenant la clé
  --input <f>           Fichier d'entrée
  --output <f>          Fichier de sortie
  --text <t>            Texte en ligne de commande
  --format <f>          Format de sortie               (défaut: base64)
                          base64 = encodage base64
                          hex    = encodage hexadécimal
                          raw    = binaire brut
  --verbose             Afficher les détails

EXEMPLES:
  node ca-spn-cli.js encrypt --key "MaCleSecrete" --text "Bonjour le monde"
  node ca-spn-cli.js decrypt --key "MaCleSecrete" --text "<base64>"
  node ca-spn-cli.js encrypt --key "MaCleSecrete" --input fichier.txt --output chiffre.bin
  node ca-spn-cli.js encrypt --rounds 20 --mode ctr --ca-steps 5 --key "Cle" --text "Test"
  node ca-spn-cli.js benchmark --rounds 10 --ca-steps 3
  node ca-spn-cli.js info --rounds 12 --mode cbc --sbox-mode custom --sbox-seed 42
`);
}

function parseArgs(argv) {
  const args = {};
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    if (a.startsWith('--')) {
      const key = a.slice(2);
      const val = argv[i + 1] && !argv[i + 1].startsWith('--') ? argv[++i] : true;
      args[key] = val;
    } else if (!args._cmd) {
      args._cmd = a;
    }
  }
  return args;
}

function buildCipherFromArgs(args) {
  const opts = {};
  if (args.rounds)       opts.rounds       = parseInt(args.rounds);
  if (args.mode)         opts.mode         = args.mode;
  if (args.neighborhood) opts.neighborhood = args.neighborhood;
  if (args['ca-steps'])  opts.caSteps      = parseInt(args['ca-steps']);
  if (args['ca-rows'])   opts.caRows       = parseInt(args['ca-rows']);
  if (args['ca-cols'])   opts.caCols       = parseInt(args['ca-cols']);
  if (args.toroidal !== undefined) opts.toroidal = args.toroidal !== 'false';
  if (args['sbox-mode']) opts.sboxMode     = args['sbox-mode'];
  if (args['sbox-seed']) opts.sboxSeed     = parseInt(args['sbox-seed']);
  return new CASPNCipher(opts);
}

function getKey(args) {
  if (args['key-file']) return fs.readFileSync(args['key-file'], 'utf8').trim();
  if (args.key) return args.key;
  throw new Error('Clé manquante. Utilisez --key ou --key-file');
}

function getInput(args) {
  if (args.input) return fs.readFileSync(args.input);
  if (args.text) return args.text;
  throw new Error('Données manquantes. Utilisez --text ou --input');
}

function outputResult(data, args) {
  const fmt = args.format || 'base64';
  let out;
  if (data instanceof Uint8Array) {
    if (fmt === 'base64') out = Buffer.from(data).toString('base64');
    else if (fmt === 'hex') out = Buffer.from(data).toString('hex');
    else out = data;
  } else {
    out = data;
  }
  if (args.output) {
    fs.writeFileSync(args.output, typeof out === 'string' ? out : Buffer.from(out));
    console.log(`✓ Résultat écrit dans : ${args.output}`);
  } else {
    console.log(typeof out === 'string' ? out : Buffer.from(out).toString('base64'));
  }
}

function cmdEncrypt(args) {
  const cipher = buildCipherFromArgs(args);
  const key    = getKey(args);
  const input  = getInput(args);
  const fmt    = args.format || 'base64';

  if (args.verbose) {
    console.log('⚙ Configuration :', cipher.getConfig());
    console.log('⚙ Format de sortie :', fmt);
  }

  let result;
  if (typeof input === 'string') {
    if (fmt === 'hex')    result = cipher.encryptToHex(input, key);
    else                  result = cipher.encryptToBase64(input, key);
  } else {
    const data  = new Uint8Array(input);
    const bytes = cipher.encrypt(data, key);
    if (fmt === 'hex')    result = Buffer.from(bytes).toString('hex');
    else if (fmt === 'base64') result = Buffer.from(bytes).toString('base64');
    else result = bytes;
  }

  outputResult(result, args);
}

function cmdDecrypt(args) {
  const cipher = buildCipherFromArgs(args);
  const key    = getKey(args);
  const input  = getInput(args);
  const fmt    = args.format || 'base64';

  if (args.verbose) {
    console.log('⚙ Configuration :', cipher.getConfig());
  }

  let result;
  if (typeof input === 'string') {
    if (fmt === 'hex') result = new TextDecoder().decode(cipher.decryptFromHex(input, key));
    else               result = cipher.decryptToString(input, key);
  } else {
    const data  = new Uint8Array(input);
    const bytes = cipher.decrypt(data, key);
    result = new TextDecoder().decode(bytes);
  }

  outputResult(result, args);
}

function cmdBenchmark(args) {
  const opts = {};
  if (args.rounds)       opts.rounds   = parseInt(args.rounds);
  if (args['ca-steps'])  opts.caSteps  = parseInt(args['ca-steps']);
  if (args.mode)         opts.mode     = args.mode;
  if (args.neighborhood) opts.neighborhood = args.neighborhood;

  console.log('⏱ Benchmark en cours...\n');
  const result = benchmark(opts);
  console.log('RÉSULTATS:');
  console.log(`  Chiffrement  : ${result.encryptMs} ms / ko`);
  console.log(`  Déchiffrement: ${result.decryptMs} ms / ko`);
  console.log(`  Débit        : ${result.throughputMBs} MB/s`);
  console.log('\nCONFIGURATION TESTÉE:');
  Object.entries(result.config).forEach(([k, v]) => console.log(`  ${k.padEnd(16)} : ${v}`));
}

function cmdInfo(args) {
  const cipher = buildCipherFromArgs(args);
  const config = cipher.getConfig();
  printBanner();
  console.log('CONFIGURATION ACTIVE:\n');
  const descs = {
    rounds:       'Nombre de rounds SPN',
    mode:         'Mode opératoire',
    neighborhood: 'Voisinage CA',
    caSteps:      'Générations CA / round',
    caRows:       'Lignes grille CA',
    caCols:       'Colonnes grille CA',
    toroidal:     'Grille toroïdale',
    sboxMode:     'Mode S-Box',
    sboxSeed:     'Seed S-Box custom'
  };
  Object.entries(config).forEach(([k, v]) => {
    const desc = descs[k] || k;
    console.log(`  ${desc.padEnd(24)} : ${String(v).padEnd(10)}`);
  });
  console.log('');
}

function main() {
  const argv = process.argv.slice(2);
  if (argv.length === 0) { printHelp(); process.exit(0); }

  const args = parseArgs(argv);
  const cmd  = args._cmd;

  if (!cmd || cmd === 'help' || args.help) { printHelp(); return; }
  if (!COMMANDS.includes(cmd)) {
    console.error(`Commande inconnue : "${cmd}". Utilisez "help" pour la liste.`);
    process.exit(1);
  }

  try {
    if (cmd === 'encrypt')   cmdEncrypt(args);
    if (cmd === 'decrypt')   cmdDecrypt(args);
    if (cmd === 'benchmark') cmdBenchmark(args);
    if (cmd === 'info')      cmdInfo(args);
  } catch (err) {
    console.error('✗ Erreur :', err.message);
    if (args.verbose) console.error(err.stack);
    process.exit(1);
  }
}

main();
