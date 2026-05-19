import { readFile } from "node:fs/promises";
import createModule from "../dist/bitneedle_avif.js";
import { createBitneedleAvifEncoder } from "../dist/index.js";

const wasmBinary = await readFile(new URL("../dist/bitneedle_avif.wasm", import.meta.url));
const encoder = await createBitneedleAvifEncoder(createModule, { wasmBinary });

const width = 576;
const height = 576;
const rgba = new Uint8Array(width * height * 4);

for (let y = 0; y < height; y++) {
  for (let x = 0; x < width; x++) {
    const i = (y * width + x) * 4;
    const v = (x * 3 + y * 5) & 255;
    rgba[i] = v;
    rgba[i + 1] = 255 - v;
    rgba[i + 2] = (x ^ y) & 255;
    rgba[i + 3] = 255;
  }
}

const direct = encoder.encode(rgba, {
  width,
  height,
  quantizer: 40,
  speed: 5,
  monochrome: true
});

const brand = new TextDecoder().decode(direct.subarray(4, 12));
if (!brand.startsWith("ftypavif")) {
  throw new Error(`unexpected AVIF brand box: ${brand}`);
}

const targetBytes = direct.length + 50;
const fitted = encoder.encodeForTarget(rgba, {
  width,
  height,
  targetBytes,
  speed: 5,
  monochrome: true
});

if (!fitted.fits) {
  throw new Error("expected fitted encode to fit target");
}
if (fitted.bytes.length > targetBytes) {
  throw new Error(`fitted output ${fitted.bytes.length} exceeds target ${targetBytes}`);
}

console.log(JSON.stringify({
  directBytes: direct.length,
  fittedBytes: fitted.bytes.length,
  quantizer: fitted.quantizer,
  fits: fitted.fits
}));
