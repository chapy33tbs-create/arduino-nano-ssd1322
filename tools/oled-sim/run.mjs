// Run the WASI build of the OLED simulator (Smart App Control blocks new .exe,
// so the renderer is compiled to wasm32-wasi and run here instead).
import { WASI } from 'node:wasi';
import fs from 'node:fs';
import { argv } from 'node:process';

const wasi = new WASI({
  version: 'preview1',
  args: ['sim.wasm', ...argv.slice(2)],
  env: {},
  returnOnExit: true,
});

const wasm = await WebAssembly.compile(fs.readFileSync(new URL('./sim.wasm', import.meta.url)));
const instance = await WebAssembly.instantiate(wasm, wasi.getImportObject());
const code = wasi.start(instance);
process.exitCode = typeof code === 'number' ? code : 0;
