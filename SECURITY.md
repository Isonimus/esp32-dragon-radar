# Security Policy

## Scope

Dragon Radar is an offline game. The firmware uses no WiFi or Bluetooth; its only
inputs are the buttons and the GPS module's serial feed, and its only stored data is
the saved game and settings in the board's flash.

That stored data includes coordinates near where you started each game, so a reportable
issue is anything that puts the player's location or device at risk, for example:

- Location data leaking somewhere it should not, such as the serial log.
- A crash, hang or memory-safety bug triggered by malformed NMEA from the GPS module.
- A release image that does not match its `SHA256SUMS.txt`, or that bricks a board.

## Supported versions

The project is pre-1.0. Only the latest release and `main` are supported; fixes land
on `main`.

| Version | Supported |
| --- | --- |
| latest release and `main` | ✅ |
| older releases | ❌ |

## Reporting a vulnerability

Please report privately, not in a public issue:

1. **Preferred:** GitHub private vulnerability reporting, under this repository's
   **Security → Advisories → Report a vulnerability** tab.
2. Otherwise, contact the maintainer [@Isonimus](https://github.com/Isonimus).

Include the release or commit, the board (Cardputer ADV, M5StickC Plus 2 or the custom
board), steps to reproduce and the impact. There is no bug bounty, as this is a hobby
project, but reports are welcome and will be credited if you would like.
