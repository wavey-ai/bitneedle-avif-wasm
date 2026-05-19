# bitneedle-avif-wasm

Bitneedle-specific AVIF encoder WASM for embedding photographs in picture-record sidecar data.

This is intentionally not a general-purpose image codec package. It exposes only the encoding surface Bitneedle needs:

- encode already-prepared RGBA photo frames to AVIF
- monochrome AVIF via YUV400 for greyscale record-sleeve photos
- libavif + libaom quality path compiled to WASM
- exact target-byte fitting by quantizer search
- small ABI suitable for browser workers and build tools

## Requirements

- Emscripten SDK available on `PATH` (`emcc`, `emcmake`)
- CMake 3.24+
- Git

The build fetches libavif and builds only the AOM encoder path.

## Build

```sh
npm run build
```

Optional libavif ref override:

```sh
BITNEEDLE_LIBAVIF_REF=v1.2.1 npm run build
```

Build output is written to `dist/`:

- `dist/bitneedle_avif.js`
- `dist/bitneedle_avif.wasm`
- `dist/index.js`

## JavaScript API

```js
import { createBitneedleAvifEncoder } from "@wavey-ai/bitneedle-avif-wasm";

const encoder = await createBitneedleAvifEncoder();

const result = encoder.encodeForTarget(rgbaBytes, {
  width: 576,
  height: 576,
  targetBytes: 6800,
  monochrome: true,
  speed: 4
});

console.log(result.bytes.length, result.quantizer, result.fits);
```

The caller supplies RGBA bytes. Cropping, borders, scaling, and photo layout remain application-level decisions so the encoder stays small and deterministic.

## Quantizer model

AVIF quantizers follow libavif semantics:

- `0` is highest quality and largest output
- `63` is lowest quality and smallest output

`encodeForTarget` binary-searches for the best quality that fits the requested byte budget. If even quantizer `63` is too large, it returns the smallest candidate with `fits: false` so the caller can reduce dimensions or sidecar allocation.

## Scope

Included:

- AVIF encode only
- 8-bit RGBA input
- YUV400 monochrome or YUV444 color output
- target-byte fitting

Not included:

- AVIF decode
- JPEG, PNG, WebP, JPEG XL
- animated AVIF
- EXIF/XMP metadata
- filesystem APIs
- command-line app binaries
