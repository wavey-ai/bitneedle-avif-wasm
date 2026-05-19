import { mkdir, copyFile } from "node:fs/promises";
import { spawnSync } from "node:child_process";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const buildDir = resolve(root, "build", "wasm");
const distDir = resolve(root, "dist");
const libavifRef = process.env.BITNEEDLE_LIBAVIF_REF || "v1.2.1";

function run(command, args, options = {}) {
  const result = spawnSync(command, args, {
    cwd: root,
    stdio: "inherit",
    env: process.env,
    ...options
  });
  if (result.status !== 0) {
    throw new Error(`${command} ${args.join(" ")} failed`);
  }
}

await mkdir(buildDir, { recursive: true });
await mkdir(distDir, { recursive: true });

run("emcmake", [
  "cmake",
  "-S",
  root,
  "-B",
  buildDir,
  "-DCMAKE_BUILD_TYPE=Release",
  `-DBITNEEDLE_LIBAVIF_REF=${libavifRef}`
]);

run("cmake", ["--build", buildDir, "--config", "Release", "--parallel"]);

await copyFile(resolve(buildDir, "bitneedle_avif.js"), resolve(distDir, "bitneedle_avif.js"));
await copyFile(resolve(buildDir, "bitneedle_avif.wasm"), resolve(distDir, "bitneedle_avif.wasm"));
await copyFile(resolve(root, "src", "index.js"), resolve(distDir, "index.js"));

console.log(`built bitneedle-avif-wasm with libavif ${libavifRef}`);
