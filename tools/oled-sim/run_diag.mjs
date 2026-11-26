// Run the diagnostic-screen renderer (sim_diag.wasm). Same WASI shim as run.mjs.
import { WASI } from 'node:wasi';
import fs from 'node:fs';
import { argv } from 'node:process';

const wasi = new WASI({
  version: 'preview1',
  args: ['sim_diag.wasm', ...argv.slice(2)],
  env: {},
  returnOnExit: true,
});

const wasm = await WebAssembly.compile(fs.readFileSync(new URL('./sim_diag.wasm', import.meta.url)));
const instance = await WebAssembly.instantiate(wasm, wasi.getImportObject());
const code = wasi.start(instance);
process.exitCode = typeof code === 'number' ? code : 0;
