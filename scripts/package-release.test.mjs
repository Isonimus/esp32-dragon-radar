import assert from 'node:assert/strict';
import { test } from 'node:test';
import {
  appOffset,
  containsVersion,
  flashModeByte,
  mergeBinArgs,
  partitionEntries,
} from './package-release.mjs';

const APP = 0x00;
const DATA = 0x01;

/** A binary partition table as gen_esp32part writes it: 32-byte entries, then an MD5
 *  record and 0xFF padding, which the parser must stop at. */
function partitionTable(entries) {
  const records = entries.map(({ type, subtype, offset, label }) => {
    const record = Buffer.alloc(32);
    record.writeUInt16LE(0x50aa, 0);
    record[2] = type;
    record[3] = subtype;
    record.writeUInt32LE(offset, 4);
    record.writeUInt32LE(0x10000, 8);
    record.write(label, 12, 'latin1');
    return record;
  });
  const md5Record = Buffer.alloc(32, 0x00);
  md5Record.writeUInt16LE(0xebeb, 0);
  return Buffer.concat([...records, md5Record, Buffer.alloc(64, 0xff)]);
}

// The layout every board builds today (ADR-0008).
const OTA_LAYOUT = [
  { type: DATA, subtype: 0x02, offset: 0x9000, label: 'nvs' },
  { type: DATA, subtype: 0x00, offset: 0xe000, label: 'otadata' },
  { type: APP, subtype: 0x10, offset: 0x10000, label: 'app0' },
  { type: APP, subtype: 0x11, offset: 0x150000, label: 'app1' },
];

test('reads every entry and stops at the MD5 record', () => {
  const entries = partitionEntries(partitionTable(OTA_LAYOUT));
  assert.deepEqual(
    entries.map((e) => [e.label, e.offset]),
    [['nvs', 0x9000], ['otadata', 0xe000], ['app0', 0x10000], ['app1', 0x150000]],
  );
});

test('the app goes to ota_0 when there is no factory partition', () => {
  assert.equal(appOffset(partitionTable(OTA_LAYOUT)), 0x10000);
});

test('the app goes to the factory partition when there is one, even after an ota slot', () => {
  const table = partitionTable([
    { type: APP, subtype: 0x10, offset: 0x110000, label: 'ota_0' },
    { type: APP, subtype: 0x00, offset: 0x10000, label: 'factory' },
  ]);
  assert.equal(appOffset(table), 0x10000);
});

test('a table with no factory or ota_0 app partition is refused, not guessed', () => {
  const table = partitionTable([
    { type: DATA, subtype: 0x02, offset: 0x9000, label: 'nvs' },
    { type: APP, subtype: 0x11, offset: 0x150000, label: 'app1' },
  ]);
  assert.throws(() => appOffset(table), /neither a factory nor an ota_0/);
});

test('an empty or erased table is refused', () => {
  assert.throws(() => appOffset(Buffer.alloc(3072, 0xff)), /neither a factory nor an ota_0/);
});

test('reads the flash-mode byte from a bootloader header at 0x0 (ESP32-C3, -S3)', () => {
  assert.equal(flashModeByte(Buffer.from([0xe9, 0x03, 0x02, 0x2f]), 0x0), 0x02);
  assert.equal(flashModeByte(Buffer.from([0xe9, 0x03, 0x00, 0x2f]), 0x0), 0x00);
});

test('reads the header at the bootloader offset, not the start of the image (ESP32 at 0x1000)', () => {
  // The factory image is padded with 0xFF from 0x0 up to the bootloader.
  const image = Buffer.concat([Buffer.alloc(0x1000, 0xff), Buffer.from([0xe9, 0x03, 0x02, 0x2f])]);
  assert.equal(flashModeByte(image, 0x1000), 0x02);
});

test('an image too short to hold the header is refused', () => {
  assert.throws(() => flashModeByte(Buffer.from([0xe9, 0x03]), 0x0), /too short/);
  assert.throws(() => flashModeByte(Buffer.alloc(0x800, 0xff), 0x1000), /too short/);
});

test('finds the version string anywhere in the firmware', () => {
  const firmware = Buffer.concat([Buffer.alloc(100, 0xaa), Buffer.from('v0.1.0\0'), Buffer.alloc(50)]);
  assert.equal(containsVersion(firmware, 'v0.1.0'), true);
});

test('a development build does not pass as a release', () => {
  const firmware = Buffer.concat([Buffer.alloc(100, 0xaa), Buffer.from('dev\0'), Buffer.alloc(50)]);
  assert.equal(containsVersion(firmware, 'v0.1.0'), false);
});

test('merge-bin gets the images in flash order, written dio, for flashing at 0x0', () => {
  const args = mergeBinArgs({
    chip: 'esp32',
    output: 'out.bin',
    images: [
      { offset: 0x10000, path: 'firmware.bin' },
      { offset: 0x1000, path: 'bootloader.bin' },
      { offset: 0xe000, path: 'boot_app0.bin' },
      { offset: 0x8000, path: 'partitions.bin' },
    ],
  });
  assert.deepEqual(args, [
    '--chip', 'esp32', 'merge-bin', '-o', 'out.bin', '--flash-mode', 'dio', '--target-offset', '0x0',
    '0x1000', 'bootloader.bin',
    '0x8000', 'partitions.bin',
    '0xe000', 'boot_app0.bin',
    '0x10000', 'firmware.bin',
  ]);
});
