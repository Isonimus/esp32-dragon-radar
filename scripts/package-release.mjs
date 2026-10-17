#!/usr/bin/env node
// Packages one board's built firmware into its release images (ADR-0008).
//
//   node scripts/package-release.mjs <env> <version> <out-dir>
//
// Run after `pio run -e <env>` with FIRMWARE_VERSION set to <version>. Writes
// dragon-radar-<env>-<version>-app.bin (firmware.bin unchanged) and -factory.bin (the
// whole flash from 0x0). Every offset and the chip come from PlatformIO, never from a
// table here: the three chips put their bootloader at different offsets, and a table
// would go stale the first time a board or partition layout changed.

import { execFileSync } from 'node:child_process';
import { copyFileSync, existsSync, mkdirSync, readFileSync, realpathSync } from 'node:fs';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';

const PARTITION_ENTRY_BYTES = 32;
const PARTITION_MAGIC = 0x50aa;
const PARTITION_TYPE_APP = 0x00;
const APP_SUBTYPE_FACTORY = 0x00;
const APP_SUBTYPE_OTA_0 = 0x10;

// The image header's flash-mode byte (offset 2). The ROM loads the second-stage
// bootloader in this mode before quad mode is enabled, so anything but dio reset-loops
// the device; esptool image_info does not catch it (ADR-0008).
const FLASH_MODE_BYTE_OFFSET = 2;
const FLASH_MODE_DIO = 0x02;

const VERSION_PATTERN = /^v\d+\.\d+\.\d+$/;

/** The entries of a binary ESP partition table, up to its first non-entry record (the
 *  MD5 record or the 0xFF padding). */
export function partitionEntries(table) {
  const entries = [];
  for (let at = 0; at + PARTITION_ENTRY_BYTES <= table.length; at += PARTITION_ENTRY_BYTES) {
    if (table.readUInt16LE(at) !== PARTITION_MAGIC) break;
    entries.push({
      type: table[at + 2],
      subtype: table[at + 3],
      offset: table.readUInt32LE(at + 4),
      label: table.subarray(at + 12, at + 28).toString('latin1').replace(/\0+$/, ''),
    });
  }
  return entries;
}

/** Where firmware.bin goes: the factory app partition, else ota_0, which is the slot
 *  the bootloader starts from on a fresh flash with empty otadata. */
export function appOffset(table) {
  const apps = partitionEntries(table).filter((e) => e.type === PARTITION_TYPE_APP);
  const slot =
    apps.find((e) => e.subtype === APP_SUBTYPE_FACTORY) ??
    apps.find((e) => e.subtype === APP_SUBTYPE_OTA_0);
  if (!slot) {
    throw new Error('partition table has neither a factory nor an ota_0 app partition');
  }
  return slot.offset;
}

/** The bootloader header's flash-mode byte in a factory image that starts at 0x0. The
 *  header is at the bootloader's offset, which is 0x1000 on the classic ESP32, with
 *  0xFF padding before it. */
export function flashModeByte(image, bootloaderOffset) {
  const at = bootloaderOffset + FLASH_MODE_BYTE_OFFSET;
  if (image.length <= at) {
    throw new Error(`image is ${image.length} bytes, too short to hold a header at 0x${bootloaderOffset.toString(16)}`);
  }
  return image[at];
}

export function containsVersion(firmware, version) {
  return firmware.includes(Buffer.from(version, 'latin1'));
}

/** esptool merge-bin arguments: the images in flash order, in an image that is flashed
 *  at 0x0 on every chip. */
export function mergeBinArgs({ chip, output, images }) {
  const ordered = [...images].sort((a, b) => a.offset - b.offset);
  return [
    '--chip', chip,
    'merge-bin',
    '-o', output,
    '--flash-mode', 'dio',
    '--target-offset', '0x0',
    ...ordered.flatMap((image) => [`0x${image.offset.toString(16)}`, image.path]),
  ];
}

function parseOffset(text) {
  if (!/^0x[0-9a-f]+$/i.test(text)) throw new Error(`unreadable flash offset "${text}"`);
  return Number.parseInt(text, 16);
}

const pio = (...args) => JSON.parse(execFileSync('pio', args, { encoding: 'utf8' }));

function boardOf(env) {
  const section = pio('project', 'config', '--json-output').find(([name]) => name === `env:${env}`);
  if (!section) throw new Error(`no [env:${env}] in platformio.ini`);
  const board = section[1].find(([key]) => key === 'board');
  if (!board) throw new Error(`[env:${env}] names no board`);
  return board[1];
}

/** esptool's chip name, from the board manifest. `pio boards` matches loosely, so the
 *  exact id is picked out of its results. */
function chipOf(board) {
  const manifest = pio('boards', '--json-output', board).find((b) => b.id === board);
  if (!manifest) throw new Error(`no board manifest for "${board}"`);
  return manifest.mcu.toLowerCase();
}

function packageRelease(env, version, outDir) {
  if (!VERSION_PATTERN.test(version)) {
    throw new Error(`version "${version}" is not vMAJOR.MINOR.PATCH`);
  }
  const metadata = pio('project', 'metadata', '-e', env, '--json-output')[env];
  const firmwarePath = metadata.prog_path.replace(/\.elf$/, '.bin');
  if (!existsSync(firmwarePath)) {
    // PlatformIO clears an env's build directory when the build flags differ from the
    // last build's, and the metadata call above counts, so a mismatched
    // PLATFORMIO_BUILD_FLAGS lands here too.
    throw new Error(
      `${firmwarePath} not found; build ${env} first, with the same PLATFORMIO_BUILD_FLAGS as this run`,
    );
  }

  const firmware = readFileSync(firmwarePath);
  if (!containsVersion(firmware, version)) {
    throw new Error(`${firmwarePath} does not contain "${version}"; was FIRMWARE_VERSION passed to the build?`);
  }

  const extraImages = metadata.extra.flash_images.map((image) => ({
    offset: parseOffset(image.offset),
    path: image.path,
  }));
  const imageNamed = (name) => {
    const image = extraImages.find((candidate) => candidate.path.endsWith(`/${name}`));
    if (!image) throw new Error(`no ${name} among ${env}'s flash images`);
    return image;
  };
  const bootloader = imageNamed('bootloader.bin');
  const partitions = imageNamed('partitions.bin');
  const images = [...extraImages, { offset: appOffset(readFileSync(partitions.path)), path: firmwarePath }];

  mkdirSync(outDir, { recursive: true });
  const stem = join(outDir, `dragon-radar-${env}-${version}`);
  const factoryPath = `${stem}-factory.bin`;
  execFileSync('esptool', mergeBinArgs({ chip: chipOf(boardOf(env)), output: factoryPath, images }), {
    stdio: 'inherit',
  });

  const mode = flashModeByte(readFileSync(factoryPath), bootloader.offset);
  if (mode !== FLASH_MODE_DIO) {
    throw new Error(`${factoryPath} has flash mode 0x${mode.toString(16)}, not dio; refusing to publish it`);
  }
  copyFileSync(firmwarePath, `${stem}-app.bin`);
  for (const image of images) console.log(`0x${image.offset.toString(16).padStart(5, '0')}  ${image.path}`);
}

if (process.argv[1] && realpathSync(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const [env, version, outDir] = process.argv.slice(2);
  if (!env || !version || !outDir) {
    console.error('usage: node scripts/package-release.mjs <env> <version> <out-dir>');
    process.exit(2);
  }
  packageRelease(env, version, outDir);
}
