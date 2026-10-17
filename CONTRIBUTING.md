# Contributing to Dragon Radar

Thanks for your interest. A few things help a contribution land smoothly.

## Development setup

Install [PlatformIO](https://platformio.org/) (the VS Code extension, or
`pip install platformio`), clone the repo, and use the environments defined in
`platformio.ini`:

```bash
pio test -e native            # host unit tests (Unity), no hardware needed
pio run  -e custom            # ESP32-C3 custom board
pio run  -e cardputer         # M5Stack Cardputer ADV (ESP32-S3)
pio run  -e m5stickc-plus2    # M5StickC Plus 2 (ESP32)
```

Build the envs one after another, not in parallel: they share PlatformIO's package
cache.

With Node.js 22 and Python 3, `npm test` runs every test suite: the release tooling, the
screen capture script (which needs `pip install pillow pyserial`) and the native tests. `npm run lint` checks the docs. CI runs all four, plus cppcheck, CodeQL and the doc checks, on every push and pull
request, and a pull request must be green to merge.

## Code standards

The engineering bar is in [docs/quality-bar.md](docs/quality-bar.md), and the rules
specific to this repo are in [CLAUDE.md](CLAUDE.md). Both are written for AI-assisted
work, but they apply to everyone. In short:

- Game code reaches the hardware only through the interfaces in `src/hal/`. Board
  `#ifdef`s and pin numbers live in `src/hal/boards/` and nowhere else.
- A change to a HAL interface or shared code is built in every env before it is done.
- Logic gets a regression test under `test/`, covering the error and edge paths too.
- Fail loud: no swallowed errors, no fallback that hides a failure.
- No magic numbers; name the constant and say why it has that value.
- Comments explain why, not what. Match the surrounding style.

## Commits and pull requests

- **Conventional commits**: `feat:`, `fix:`, `docs:`, `refactor:`, `test:`, `chore:`,
  with an optional scope (`fix(gps): …`). One logical change per commit.
- Explain **why** in the body; the diff already shows what.
- Fork, branch (`feature/…` or `fix/…`), and open a pull request against `main`.
- Fill in the pull request template, and run the native tests and the builds your change
  touches before you push.

## A note on `adr/` and `slices/`

Those directories are the project's decision records: architecture decisions (ADRs) and
feature work-units (slices), kept under a documentation method enforced by
`scripts/lint-docs.mjs`. Read them for context on why something is the way it is, but you
do **not** need to write them to contribute. Describe your change in the pull request and
link the issue it addresses; the maintainer records any decision it implies.

## Reporting bugs and requesting features

Use the issue templates. Questions, help with a build and photos of yours go to
[Discussions](https://github.com/Isonimus/esp32-dragon-radar/discussions). For anything
security-sensitive, follow [SECURITY.md](SECURITY.md) instead of opening a public issue.
