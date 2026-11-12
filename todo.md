# Beneath Uncounted Heavens — Jump Playback Uses the Wrong Pre-Execution State (FIXED)

Root cause confirmed, fixed, regression-tested, and verified headless. The full UI-driven graphical-PIE click-through is still pending (headless cannot click WBP buttons).

## 1. Root cause (CONFIRMED)

Read the actual saved `BP_CombatManager.ExecuteMove` graph via MCP. It showed:

- `_returnvalue` = original `MakeState` (JumpTarget = -9999).
- When `PlayerMove == 9`: `_returnvalue_1 = WithPlayerJumpTargetFromPreview(_returnvalue)` and `PreExecutionState = _returnvalue_1` (JumpTarget set).
- `ExecuteMoveWithVars` resolves from `_returnvalue` but applies `GExecJumpTarget` internally, so the resolver used the real destination.
- `BeginJumpPlayback` received `_returnvalue` (JumpTarget = -9999), NOT `PreExecutionState`.

So `GenerateTimeline(PreState, ...)` started from a state with `JumpTarget = -9999`. `PrepareJumpCommit` substituted the takeoff XY, producing a Jump in place even though the resolver used the real destination. `DrawJumpPathFromState` read the same wrong `GJumpPlaybackPreState`, explaining the incorrect trajectory.

C++ verification:
- `BeginJumpPlayback` (CombatSim.cpp:3315) calls `GenerateTimeline(PreState, ...)` at line 3330 and stores `GJumpPlaybackPreState = PreState`.
- `PrepareJumpCommit` (CombatSim.cpp:369) sets `JumpTarget = F.X` when `F.JumpTarget < -9998.f` (Jump in place).
- `WithPlayerJumpTargetFromPreview` (CombatSim.cpp:150) uses `GPlayerJumpTarget`; `ExecuteMoveWithVars` (CombatSim.cpp:2840) uses `GExecJumpTarget`. Both are set to the same value by `SelectJumpDestination`.

## 2. Four critical states (one real Jump selection)

A. Confirmed pending selection:
- PlayerMove = 9
- Preview Jump target XY = (-20, 120)
- Execution Jump target XY = (-20, 120)

B. PreExecutionState (committed):
- Player X/Y = (500, 0)
- JumpStart X/Y = (500, 0)
- JumpTarget X/Y = (-20, 120)
- Action = -1, Phase = 0, PhaseInstants = 0 (grounded, not yet committed)

C. State passed to BeginJumpPlayback:
- BEFORE fix: JumpTarget = -9999 (the unadjusted `_returnvalue`) — differed from PreExecutionState.
- AFTER fix: JumpTarget = (-20, 120) — identical to PreExecutionState.

D. Result.FinalState:
- Final player X/Y = (324.6, 40.5)
- Player Height = 206.25
- Action = 9, Phase = 2, PhaseInstants = 3
- ReactionOpportunity = true, ReactionInstant = 4

First five playback snapshots (XY, Height):
- inst=0: (500.0, 0.0), Z=90.0
- inst=1: (441.5, 13.5), Z=186.2
- inst=2: (383.1, 27.0), Z=255.0
- inst=3: (324.6, 40.5), Z=296.2
- inst=4: (324.6, 40.5), Z=296.2, reaction=1

No Jump target was -9999, zero, or replaced with the takeoff coordinates after the fix. Before the fix the playback PreState target was -9999.

## 3. State handoff fix (DONE)

Smallest targeted Blueprint edit to `BP_CombatManager.ExecuteMove`: the final `BeginJumpPlayback` call now passes `(Variables|Default|GetPreExecutionState)` instead of `_returnvalue` as the `PreState` argument. No second targeting state invented; `ExecuteMoveWithVars` and `WithPlayerJumpTargetFromPreview` both apply the same destination (GExecJumpTarget == GPlayerJumpTarget). Jump's deterministic XY movement and parabolic height are preserved. The whole EventGraph was not rewritten.

## 4. Resolution vs playback (VERIFIED)

- First playback snapshot matches the real takeoff (500, 0).
- Every subsequent airborne XY moves toward the selected landing point.
- Playback destination (324.6, 40.5) matches the authoritative committed Jump destination.
- Final playback snapshot agrees with the resolved state at the reaction boundary (Height 206.25).
- Path renderer uses the same takeoff/landing pair (Last=(-20.0,120.0) in the path logs).
- At the airborne reaction the fighter freezes at the snapshot XYZ (Z=296.25 = 90 + 206.25); the remaining path uses the original parabola.

## 5. Diagnostic vs real combat

The `BUH.TriggerReactionJump` diagnostic was not used for this verification. The regression test exercises the same target-confirmation and execution-state-building functions as the manager (SetPreviewJumpTarget/SetExecJumpTarget, MakeStateWithPhases, WithPlayerJumpTargetFromPreview, ExecuteMoveWithVars, BeginJumpPlayback with the committed prestate). The manager enters State=Reaction when the resolver returns a reaction opportunity (Result.bReactionOpportunity = true). Continue/full-flow graphical click-through is still pending.

## 6. Regression test (ADDED, PASSING)

`BUH.JumpTargetHandoffTest` in PathRendererAutomation.cpp:
- Player on the right (X=500), opponent on the left (X=-500) and mid-Heavy (Phase=3).
- Jump landing point clearly to the left and offset on Y: (-20, 120).
- Exercises the same functions as the manager.
- Asserts: pending target valid; playback prestate contains the target; target not -9999; playback fighter changes XY during flight; snapshots agree with the resolver; remaining path ends at the original clamped destination; reaction pause freezes at the correct XYZ.
- Control: a timeline generated from the ORIGINAL state (JumpTarget=-9999) jumps in place (movedXY=0), proving the unfixed path is caught.
- Result: `Success`. Real fighter actor moved to (324.6, 40.5) and froze at Z=296.25.

All existing Walk, Dash, and Jump tests kept and passing.

## 7. Verification and push (DONE headless)

- Read back the corrected `BP_CombatManager.ExecuteMove` graph: `BeginJumpPlayback` now uses `(Variables|Default|GetPreExecutionState)`.
- Compiled and saved `BP_CombatManager` (`save_assets` returned true).
- Ran all 5 Jump tests: `BUH.JumpSimulationTests`, `BUH.JumpPathColorTests`, `BUH.JumpPlaybackIntegrationTests`, `BUH.JumpGroundedReactionTest`, `BUH.JumpTargetHandoffTest` — all `Success` (passed=5, failed=0).
- notes.md updated with the proven state mismatch and fix.
- Committed and pushed to origin/master.

Report:
- Mismatched prestate confirmed: YES (BeginJumpPlayback received `_returnvalue` with JumpTarget=-9999, not PreExecutionState).
- Before fix: playback PreState JumpTarget = -9999 → Jump in place (movedXY=0).
- After fix: playback PreState JumpTarget = (-20, 120) → fighter moves to (324.6, 40.5), freezes at Z=296.25.
- Tests passed: 5/5 Jump tests.
- Full UI-driven graphical-PIE click-through: NOT yet verified (headless cannot click WBP buttons; needs a graphical PIE session).

**Success criterion (met headless):** The combat resolver, actual airborne fighter, playback timeline, and remaining Jump path all use the same committed destination and stop at the same reaction state.
