# Beneath Uncounted Heavens — Opponent Decision Boundary

## Objective

Fix the tactical combat prediction so it stops when the opponent becomes ready to choose another action, instead of assuming the opponent remains idle for the rest of the 30-Instant prediction window.

**Preserve the newly working reaction-preview system.**

## Current verified behavior

The user has confirmed in graphical Unreal PIE:

- Walk → Execute → Heavy reaction → Continue generates a timeline.
- The slider scrubs forward and backward successfully.
- The cyan prediction ghost moves smoothly along the simulated Walk trajectory.
- The real fighter remains stationary while previewing.
- Predicted distance updates correctly.
- The opponent's Heavy progresses through its action phases.

This is a working milestone. Do not regress it.

## Problem

In the user's latest screenshot, the timeline is at Instant 20/30.

The opponent has finished Heavy and is now Ready, while the player is still walking.

However, the timeline continues predicting movement beyond that point as if the opponent will take no further action.

This is misleading because the opponent could initiate another attack or defensive action.

## Code investigation

Repository: `HelHeresyDev/BeneathUncountedHeavens`

Current baseline: `5bb0bca`

In `Source/BeneathUncountedHeav/CombatSim.cpp`, inspect:

- `GenerateTimeline`
- `GenerateAndStoreReactionPreview`
- `GenerateReactionPreviewFromManager`
- `GenerateBranchFromBase`
- `GetTimelineLengthText`
- Timeline snapshot stop reasons

`GenerateAndStoreReactionPreview` currently calls:

```cpp
GenerateTimeline(
    InState,
    PlayerDecision,
    -1,
    MaxInstants,
    true,
    false,
    true
);
```

The arguments after `MaxInstants` are:

1. `bSuppressPlayerReadyBreak = true`
2. `bStopAtDecisionBoundary = false`
3. `bOpponentNoAction = true`

`GenerateTimeline` already contains logic to stop when `bOpponentReady && bStopAtDecisionBoundary`.

Investigate and use this existing mechanism before creating any new simulation logic.

## Required behavior

### 1. Stop at the next opponent decision

During a reaction preview, continue simulating the player's chosen response and the opponent's already-committed action.

Stop when the opponent completes that action and becomes ready to choose another.

The final snapshot should represent the exact state at that decision boundary.

Do not automatically select another opponent action.

If combat reaches a KO or another legitimate earlier termination, preserve that termination instead.

### 2. Preserve all committed actions

Continue must retain:

- The player's original Walk/Dash/Jump trajectory.
- The opponent's existing Heavy action and remaining phases.
- Current health and positions.
- Reaction bookkeeping, including `ReactedThreatFighter`.

Do not restart the opponent's Heavy or reconstruct a new combat state.

Use the existing safe `CurrentState` copying implementation. Never reintroduce raw `FMemory::Memcpy` for `FBuhCombatState`.

### 3. Timeline behavior

The slider maximum must reflect the actual number of generated snapshots, not a fixed 30 Instants.

For example, if the opponent becomes ready at Instant 20, the preview must end there rather than extrapolating to Instant 30.

The displayed Instant, ghost positions, predicted distance, and opponent phase should all correspond to the same snapshot.

At the end, communicate that the opponent is ready to decide and that subsequent actions are unknown. Reuse existing stop-reason UI where possible rather than redesigning the interface.

### 4. Preserve the planning system

This change applies specifically to reaction previews.

Do not change ordinary planning previews, the stored planning baseline, or existing hypothetical branching behavior.

Do not implement additional opponent branching UI yet. That will be our next separate task.

## Verification requirements

Add a regression test that:

1. Creates a genuine Walk-versus-Heavy reaction state using the existing combat resolver.
2. Copies the actual paused combat state.
3. Generates a Continue reaction preview.
4. Confirms the opponent's Heavy progresses through its remaining phases.
5. Confirms the preview stops at the first opponent-ready decision boundary.
6. Confirms the final snapshot's opponent phase is Ready (0).
7. Confirms no additional idle snapshots occur after that boundary.
8. Confirms the player's original movement and reaction bookkeeping are preserved.
9. Confirms previewing does not mutate the authoritative combat state.
10. Confirms the stored snapshots and full-state arrays remain synchronized.

Also verify that planning previews and hypothetical branches continue to pass their existing regression tests.

## Scope restrictions

Do not:

- Rewrite `ScrubTimeline`.
- Change slider geometry or layout.
- Change prediction ghost materials or positioning code.
- Modify combat action durations or movement speeds.
- Change authoritative `ExecuteMove` behavior.
- Introduce new opponent AI logic.
- Implement new hypothetical branching controls.
- Perform unrelated UI cleanup.

## Completion

- Build the Unreal Editor target.
- Run all BUH automation tests.
- Compile and save any modified Blueprint assets.
- Read back modified Blueprint graphs.
- Update `notes.md` and `todo.md`.
- Commit and push the changes.

Report the exact code changes, regression tests, and results.

**Do not claim graphical PIE verification.** The user will perform that test manually.

### Success criteria

In graphical PIE:

Walk → Execute → reaction to Heavy → Continue → scrub timeline.

The prediction stops when Heavy finishes and the opponent becomes ready. The slider ends at that decision boundary, and the cyan ghost still moves correctly through every preceding Instant.

The real fighter remains frozen until Execute is clicked.

## Completed (2026-11-15)

- [x] Built the Unreal Editor target successfully.
- [x] Ran all BUH automation tests: 26/26 passed, 0 failed.
- [x] No Blueprint assets modified; no Blueprint compile/save/read-back required.
- [x] Updated `notes.md` and `todo.md`.
- [x] Committed and pushed the changes.
- [ ] Graphical PIE verification: user performs manually (do not claim).