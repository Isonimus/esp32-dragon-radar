---
id: '0016'
title: "Promote select ledger items to GitHub issues: an item lives in one tracker, never both"
type: architecture
status: accepted
date: 2026-10-17
supersedes: []
superseded_by: []
---

## Context

The repository went public with v0.1.0 on 2026-10-17. Outside contributors need a surface
they can find, discuss and claim work on, and GitHub issues is that surface. `LEDGER.md` is
the repo's single open-work tracker, on the ground that two trackers need manual sync and
manual sync does not happen (stele:ADR-0001).

Two ways were weighed:

- **Mirror:** the issue opens and the ledger line stays, carrying the issue's link; the PR
  that resolves it deletes the line and closes the issue. Every promoted item then lives in
  two places, and the two drift whenever an issue is edited, closed as won't-fix or
  re-scoped on GitHub.
- **Promote:** the item moves. Chosen. The sibling adversary repo has run this since
  2026-09-12 (adversary:ADR-0022) without the trackers drifting.

Unlike adversary's, this ledger is public, so a contributor can read it; it is still the
maintainer's worklist, written in this repo's own vocabulary, and not where an outsider
should have to look for work.

## Decision

- **Promote, don't mirror.** An item opened to outside contributors moves from
  `LEDGER.md` to a GitHub issue, and its ledger line is deleted in the same change. The
  issue is then its only home. An item lives in exactly one tracker.
- **Eligible only if all three hold:** an outsider can understand it without this repo's
  internal context; it is self-contained and bounded; and it genuinely wants an outside
  contributor. `good first issue` when it is also small and teaches the codebase,
  `help wanted` otherwise. A board port also takes `new board`.
- **Stays in the ledger:** audits that need the maintainer's own hardware or field tests,
  design and architecture deferrals, and bugs whose diagnosis needs deep project state.
  Work that starts as a contribution proposal, such as a new board, may open as an issue
  directly and never enter the ledger.
- **One way.** A closed or declined issue does not come back to the ledger; the issue and
  the git log are its record. A maintainer-only follow-up found while working an issue is a
  new ledger line, not a copy of the issue.
- Issues are written for outsiders: context, the files to touch, how to test it and when it
  is done, without this repo's document vocabulary.

## Consequences

- Open work has two surfaces, the ledger and the issues, but never the same item in both,
  so the no-sync guarantee of stele:ADR-0001 holds: what it protects is one owner per item,
  not one file.
- Grooming the ledger now includes a promote-or-keep call. Keep is the default.
- A Projects board or milestones are extensions of this decision, unless they mirror an
  item across surfaces, which needs a new ADR superseding this one.
