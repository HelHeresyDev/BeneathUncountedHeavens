# BUH — Hypothetical Attack Does Not Interrupt Walk Preview

The user has confirmed that ordinary Walk prediction ghosts now appear, but selecting a hypothetical opponent attack at a chosen Instant does not stop the Walk prediction.

I inspected repository commit `efdca6c`.

## Findings

`UBuhCombatSim::GenerateBranchFromBase` should already generate a reaction interruption when the opponent starts Quick or Heavy while the player is in a reactable Walk.

`CheckReactionPause` records the reaction, and `GenerateTimeline` stops with `StoppedReason=5`.

However, the branch can fail without replacing the existing timeline, and existing automation tests do not verify the complete Blueprint-to-ghost pathway.

## Investigate first

Read the actual saved Blueprint graphs:

- `WBP_CombatUI.EventGraph` — all four hypothetical opponent buttons
- `WBP_CombatUI.RegeneratePredictionPreview`
- `WBP_CombatUI.ScrubTimeline`
- `WBP_CombatUI.RefreshUI`

Trace this exact sequence:

1. Start a fresh encounter.
2. Select Walk and a sufficiently distant destination.
3. Scrub to Instant 5, while Walk is active.
4. Select hypothetical Heavy.
5. Check whether `GenerateBranchFromBase` executes.

Record:

- Selected slider snapshot index
- Selected hypothetical opponent action
- Base timeline length
- Opponent phase at the selected base snapshot
- Whether the branch is legal
- `OutReason` on rejection
- Returned branch timeline length
- Final snapshot's `StoppedReason`
- Final installed `GetTimelineLength`
- Slider maximum after regeneration

Determine whether the branch fails, is never called, or is overwritten by a later refresh/regeneration.

Do not guess which one is happening.

## Required behavior

When the opponent is legally ready and the player is actively Walking:

- Choosing hypothetical Quick or Heavy starts that action at the selected branch point.
- A valid reaction opportunity interrupts the simulated Walk.
- The shortened branch replaces the current displayed timeline.
- Earlier Walk snapshots remain available for scrubbing.
- The slider cannot move past the reaction boundary.
- The cyan ghost stops at the predicted reaction position.
- The real fighter remains stationary.

If the opponent is not legally ready, reject the hypothetical action and display the actual reason instead of silently retaining the original prediction.

Do not force an interruption when the player has already finished walking or when the movement is not legally reactable.

## Critical verification

Add a regression test using an ordinary planning Walk baseline, not a manually pre-committed Walk fixture.

Generate the baseline, branch at an active Walk snapshot (for example, index 5), and assert:

- `bLegal == true`
- The last snapshot has `bReactionOpportunity == true`
- The last snapshot has `StoppedReason == 5`
- The installed timeline ends at the reaction boundary
- No original baseline snapshots remain after that boundary
- The final ghost coordinates match the interrupted Walk position
- The original stored baseline remains intact for further branching
- The authoritative combat state is unchanged

Then verify the UI path invokes that successful branch, updates the slider range, and scrubs the final installed timeline rather than the old baseline.

## Constraints

Preserve:
- Ordinary Walk/Dash/Jump previews
- Reaction → Continue previews
- Opponent decision-boundary stopping
- Current slider geometry
- Existing ghost rendering and materials
- Authoritative combat execution

Make the smallest necessary changes. Do not introduce a new preview architecture.

Build, compile and save modified Blueprints, read back their graphs, run all BUH tests, and update notes.md and todo.md.

Commit and push the fix.

Report the exact root cause and distinguish automated verification from graphical PIE verification.

## Resolution

**Root cause:** The four hypothetical-opponent button handlers in `WBP_CombatUI:EventGraph` called `RefreshUI` before `RegeneratePredictionPreview`. `RefreshUI` scrubbed the old Walk baseline and left the ghosts/readout showing it; the later `RegeneratePredictionPreview` installed the reaction branch but did not update the UI, so the Walk prediction appeared to continue.

**Fix:** Reordered all four opponent handlers to call `RegeneratePredictionPreview` first and `RefreshUI` second. `WBP_CombatUI` compiled clean and the saved graph was read back to confirm the order.

**Regression test:** Added `BUH.PlanningWalkHypotheticalBranch`, which generates an ordinary planning Walk baseline, branches at Instant 5 with a hypothetical Heavy, and asserts legality, branch instant, shortened timeline, preserved earlier Walk snapshots, reaction opportunity, final `StoppedReason == 5`, and an unchanged authoritative combat state.

**Automated verification:** All 27 `BUH.*` automation tests pass (0 failures), including the new regression test and the existing reaction/decision-boundary suites.

**Graphical PIE verification:** Still pending. The user must confirm in a fresh PIE that selecting Walk, scrubbing to an active Walk instant, then choosing a hypothetical Heavy/Quick stops the Walk ghost at the reaction position, shortens the slider range, and leaves the real fighter stationary.