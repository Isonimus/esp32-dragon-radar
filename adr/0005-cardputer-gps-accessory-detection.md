---
id: '0005'
title: "Cardputer ADV GPS: listen on the cap and Grove ports, keep the first that speaks NMEA"
type: architecture
status: accepted
date: 2026-10-13
supersedes: []
superseded_by: []
---

## Context

M5Stack sells two official GPS accessories for the Cardputer ADV, and a player will
have one or the other:

- the **Grove GPS unit** (AT6668) on the Grove port, ESP32 RX G1 / TX G2;
- the **GPS/LoRa cap** on the expansion header, ESP32 RX G15 / TX G13, with an SX1262
  radio on the SD card's SPI bus.

Both were measured by the sibling `../adversary` firmware on the same ADV hardware: its
slice 0007 detected the cap's GPS as "AT6668 on Cap" at 115200 baud in 0.702 s, and the
Grove unit also runs at 115200. Only the pins differ. Adversary drives the cap radio's
chip select (G5) high, but only because it uses the SD card, which shares that SPI bus;
this firmware does not use SD, and none of the cap's pins collide with the ADV's display
(33–38), keyboard (8, 9, 11), speaker I2S (41–43) or battery ADC (10).

Before this decision `CardputerGps` read only the Grove pins, so a cap owner saw
`chars=0` forever.

Options considered:

- **Probe at boot** (adversary's approach): listen on each pin set in turn for ~1.2 s.
  Adds up to ~2.4 s of boot with no GPS attached, and needs a background re-probe for a
  GPS plugged in after boot.
- **A build flag per accessory**: puts the burden of matching firmware to hardware on
  the player, which is the wrong place for it in a game.
- **Listen on both at once**: the ESP32-S3 has UART1 and UART2 free (the console is USB
  CDC), so both ports can be read in parallel with no timeout to tune.

## Decision

- `CardputerGps` opens the cap port on `Serial1` and the Grove port on `Serial2`, both at
  115200, each with its own `GpsDriver`, and updates both until one is chosen.
- The port kept is the first, cap before Grove, to parse a **checksum-valid** NMEA
  sentence (`chooseGpsSource`, pure and native-tested). Received bytes are not enough: an
  unconnected RX pin can pick up noise, but noise does not pass the NMEA checksum.
- Once a port is chosen, the other UART is released with `end()` and the choice holds
  until reboot.
- This is accessory detection *within* a board. It does not conflict with ADR-0001,
  which forbids detecting the *board* at runtime; the board is still chosen at build time.

## Consequences

- Either official GPS works with the same firmware, and a GPS attached after boot is
  picked up as soon as it sends a valid sentence.
- Swapping accessories after one has been chosen needs a reboot.
- With both accessories attached, the cap wins only when both deliver their first valid
  sentence in the same update; otherwise the faster one wins. Either is a working GPS.
- Only 115200 is listened for, because both measured modules use it. A module at another
  baud shows `chars` rising with `parsed=0` in that port's diagnostics line.
