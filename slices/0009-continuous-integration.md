---
id: '0009'
title: "Continuous integration: docs, firmware builds, native tests and static analysis on every push"
type: slice
status: accepted
date: 2026-10-14
supersedes: []
superseded_by: []
---

## Goal

Check every push and pull request automatically before the repo goes public. Until now
every check ran only on the maintainer's machine. Invariant #2 (build every firmware env
and run the native tests before calling a shared change done) was enforced by review
alone. The pre-commit hook's header named `.github/workflows/docs.yml` as its backstop for
clones without the hook, but that file did not exist, so a `--no-verify` commit was caught
by nothing.

## Definition of Done

- **Given** a push or pull request
- **When** the `docs` workflow runs
- **Then** it fails if `lint-docs` reports an error, if `adr/INDEX.md` does not match the corpus, or if an immutable document's body lost or changed a line since the base revision

- **Given** a push or pull request
- **When** the `CI` workflow runs
- **Then** it builds `custom`, `cardputer` and `m5stickc-plus2`, runs the `native` tests, runs the release-tooling tests, and fails if any of them fails

- **Given** a firmware build in CI
- **When** it finishes
- **Then** its RAM and flash use appear in the run summary, and the step fails if PlatformIO's size line is missing

- **Given** a push or pull request that adds an error-class cppcheck finding in `src/`
- **When** the `cppcheck` job runs
- **Then** the job fails

- **Given** a push to `main`, a pull request to `main`, or the weekly schedule
- **When** the `CodeQL` workflow runs
- **Then** it analyses the C++ compiled for all three boards and uploads only results located outside `.pio/` and `test/`

## Design

Adapted from the adversary project's `ci.yml`, `codeql.yml` and `dependabot.yml`, and from
stele's own `docs.yml`. What was reused and what changed:

- **`docs.yml`** is stele's workflow minus its stele-specific steps (changelog, mutant
  checks, the linter's own test suite), which have no counterpart here. It keeps the name
  the vendored hook already cites.
- **`ci.yml`** keeps adversary's matrix of `env` → command, its Python 3.13 pin (the
  pioarduino platform rejects 3.14), the PlatformIO cache keyed on `platformio.ini`, and
  its build-size summary. The matrix lists our four envs. The build step sets
  `shell: bash`, which adversary's lacks: GitHub's default shell is `bash -e` without
  pipefail, so `pio ... | tee build.log` took tee's exit status and a failing test
  passed. Reproduced locally (`bash -e` exit 0, `bash -eo pipefail` exit 1) after the
  adversarial review at wrap-up found it. A `scripts` job runs
  `node --test scripts/*.test.mjs` for the release tooling (slice 0010).
- **cppcheck** is adversary's gate unchanged: error severity only, over `src/`. It passed
  on 2026-10-14 (cppcheck 2.13.0, exit 0), so it gates from the first run.
- **`codeql.yml`** keeps adversary's post-analysis SARIF filter. Path filters are inert
  for compiled languages, so the vendored libraries in `.pio/libdeps` are analysed and
  have to be removed from the results afterwards. Adversary builds one env; this repo
  builds all three between `init` and `analyze`, because each board's HAL is compiled
  only in its own env and one build would leave two boards' code unscanned.
- **`dependabot.yml`** tracks GitHub Actions only. Dependabot cannot read
  `platformio.ini`, and libraries are pinned by hand (ADR-0008).

## Verification

- The commands each job runs were run locally on 2026-10-14 and passed:
  `node scripts/lint-docs.mjs .`, `node scripts/build-index.mjs --check .`,
  `pio test -e native`, `pio run -e <env>` for the three boards, `node --test scripts/*.test.mjs`,
  and the cppcheck command line.
- That GitHub runs the workflows, and that each one is green, cannot be checked before the
  repo has a remote. A local `scripts/*-verify.mjs` would only re-run the same commands
  and could not show that GitHub runs them. The proof is the first push's run of
  `docs`, `CI` and `CodeQL`, and it stays an open `LEDGER.md` item until it has been
  seen green.
