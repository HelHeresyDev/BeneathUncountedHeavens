# BUH — URGENT REGRESSION: Restore Movement After Timeline Preview Update

The user tested commit `2b155ac` in graphical PIE and reports that movement is broken.

The previous commit `f53bfc6` had user-confirmed working Walk, Dash, Jump, Continue, and clean startup.

Prioritize restoring movement functionality. Pause all new timeline-preview features.

## Status (updated after regression fix)

- Known-good commit `f53bfc6`; broken commit `2b155ac`.
- Root cause: `BP_CombatManager.ExecuteMove` set `State=3`, `ReactionInstant`, `ReactionFighter`, and `ObservedThreatAction` on a reaction, but never set `bReactionOpportunity`. `WBP_CombatUI.RefreshUI` hides `BtnContinueWalk` unless `GetReactionInfo(...).bIsReaction` is true, which requires `bReactionOpportunity==true`, `ReactionFighter==0`, and a valid `CurrentState`.
- Fix: `ExecuteMove` now calls `SetReactionOpportunity true` in the reaction branch and `false` in the no-reaction, player-KO, opponent-KO, and `PlayerMove==-1` branches. The Blueprint was compiled and saved.
- `WBP_CombatUI.TimelineSlider.OnValueChanged` was already correct; the export file `tests/blueprint_exports/tactical_timeline_preview.md` was inaccurate and has been corrected to match the actual saved graph.
- `TimelineSlider` defaults: `MinValue = 0`, `MaxValue = 30`, `StepSize = 0.01`. The handler clamps the selected index to `[0, GetTimelineLength()-1]`.
- Added PIE-world integration test `BUH.PIE.WalkContinueReaction`. It invokes `ResetEncounter`, `SelectWalkToward`, and `ExecuteMove`, then verifies the C++ resolver produces a reaction opportunity and that `GetReactionInfo` includes Continue (action 7). It passes under headless `-nullrhi`; it does not verify Blueprint variable writes or widget visibility because `ProcessEvent` variable writes are not observable in editor context.
- All 24 `BUH` automation tests pass.
- Graphical PIE A–J verification is still pending and must be performed in a real PIE session.

## Evidence from the screenshot

The HUD shows:
- "Reaction opportunity / Ready / 0: Heavy"
- "Your: Walk"
- A green Walk trajectory.
- The real fighters remain at their original positions.
- No visible Continue button.

A reaction at Instant 0 may legitimately occur before Walk moves. Do not change the reaction timing merely to make the fighter move.

But a legitimate Walk reaction should allow Continue, and the UI does not display it.

Determine whether the manager's authoritative reaction state is incorrect or the widget is failing to display the legal action.

## 1. Compare against the known-good commit

Inspect the Git diff between:

`f53bfc6` — last user-confirmed working movement

`2b155ac` — timeline-preview changes

Focus on changed binary Blueprint assets:

- `BP_CombatManager.uasset`
- `WBP_CombatUI.uasset`

Read their CURRENT saved graphs through MCP.

Specifically inspect:
- EventBeginPlay and InitCurrentState
- ExecuteMove
- ResetEncounter
- BuildPredictions / GeneratePreviewTimeline
- RefreshUI
- Walk/Dash/Jump selection and destination confirmation
- Continue button visibility and OnClicked
- TimelineSlider.OnValueChanged
- EventTick

The new timeline implementation must not initialize, overwrite, or reset real combat state during hypothetical prediction.

## 2. Reproduce Walk → Heavy reaction

From a fresh encounter:

1. Choose Walk.
2. Confirm an XY destination.
3. Execute against the normally committed opponent Heavy.
4. Inspect the reaction state at Instant 0.

Record:
- Manager.State
- PlayerMove
- bHasCurrentState
- bReactionOpportunity
- ReactionFighter
- CurrentState.Fighters[0].Action
- CurrentState.Fighters[0].Phase and PhaseInstants
- CurrentState.Fighters[0].WalkTarget XY
- GetReactionInfo().bIsReaction
- GetReactionInfo().LegalActions
- Continue button visibility
- Whether Continue's OnClicked delegate is bound

C++ GetReactionInfo already includes action ID 7 (Continue) when the current player action is Walk (ID 4, 5, or 6), Dash (8), or Jump (9).

If CurrentState indicates Walk and GetReactionInfo returns Continue as legal, fix the UI visibility/binding.

If CurrentState has lost the active Walk, identify the write that overwrote it.

If the player is in a legitimate reaction at Instant 0, clicking Continue should advance the original Walk toward the selected destination.

## 3. Check the new initialization and preview pathways

The new timeline export shows:

`ResetEncounter → InitCurrentState`

and a GeneratePreviewTimeline function that reads CurrentState.

Check whether InitCurrentState changes bHasCurrentState, phase, active action, or any execution-state variable in a way that alters the original ExecuteMove behavior.

Hypothetical preview must NEVER write:
- CurrentState
- bHasCurrentState
- Manager.State
- Reaction flags
- PlayerMove
- OpponentMove
- Real fighter actor transforms

unless that write is part of the existing authoritative execution flow, not preview generation.

Use a copy of the authoritative combat state for prediction calculations.

Do not assume that because a C++ function takes its state by value, the surrounding Blueprint is incapable of changing manager variables.

## 4. Fix the confirmed timeline slider wiring error

I inspected `tests/blueprint_exports/tactical_timeline_preview.md`.

The saved TimelineSlider.OnValueChanged export shows:

`_returnvalue = Truncate(Value)`

but the subsequent snapshot getters use:

`Truncate(GetTimelineLength())`

That is wrong.

For a timeline of N snapshots, indices are 0 through N-1, not N.

Correct the actual saved Blueprint so that every snapshot getter uses the SAME correctly clamped selected index.

Check the actual slider's configured MinValue, MaxValue, and StepSize.

Choose one consistent representation:
- Integer-style slider with range 0..N-1, or
- Normalized 0..1 converted to an index.

Do not mix both.

Handle N=0 and N=1 safely.

Verify scrubbing genuinely moves between DIFFERENT snapshots and that Next Event selects the exact intended index.

Keep ghost-only transforms separate from real actor transforms.

## 5. Movement regression checklist

Test the actual gameplay pathway:

A. Fresh PIE: clean arena, no path or ghosts.

B. Walk to a destination away from the opponent: follows the selected XY path, with correct reaction and Continue behavior.

C. Dash to a destination: correct red/green commitment, execution, interruption, Continue.

D. Jump: parabolic execution, midair reaction freeze, preserved remaining trajectory, Continue to landing.

E. Quick, Heavy, Guard, Dodge still execute.

F. Switching hypothetical opponent actions never changes the real committed opponent action.

G. Timeline scrubbing moves only cyan/orange ghosts, never the real fighters.

H. Back/cancel/reset clears stale preview geometry but not an active authoritative movement.

I. Reopening the editor and starting a fresh PIE session preserves these results.

For each movement, compare selected destination, authoritative resolved position, and actual real fighter XYZ.

## 6. Regression tests must target the actual integration

The new `BUH.TimelinePreviewTests` mostly tests C++ prediction functions.

Add a focused PIE-world integration test or verifiable Blueprint runtime test for:

- Walk confirmation → ExecuteMove → Reaction → Continue.
- Expected CurrentState and legal Continue actions.
- The actual Continue widget visible and clickable.
- TimelineSlider selection using an in-range snapshot index.
- Prediction regeneration leaving authoritative combat state unchanged.

Tests should fail with the current broken behavior.

Do not claim that all graphical interaction tests passed merely because the widget functions exist or the C++ simulation tests pass.

The current timeline export incorrectly claims manual user confirmation of the A–J graphical tests. The user has instead reported a regression. Correct the documentation.

## 7. Repair approach

Make the smallest necessary changes.

If the new timeline Blueprint rewiring damaged the existing movement flow, restore the affected nodes from the known-good implementation while retaining only independently verified timeline additions.

Do not change movement balance, Jump physics, reaction detection, or the established Continue rules.

Do not reset or hard-revert the Git working tree without preserving any newer intended changes.

Compile, save, and read back the corrected Blueprint graphs.

Run all BUH automation tests.

Update `todo.md`, `notes.md`, and `tests/blueprint_exports/tactical_timeline_preview.md` with actual findings.

Commit and push the fix to `origin/master` on `HelHeresyDev/BeneathUncountedHeavens`.

Report the commit SHA and distinguish automated verification from graphical PIE verification.

**Success: Walk, Dash, Jump, and Continue behave exactly as they did before the timeline-preview update. The timeline feature operates independently without corrupting or replacing authoritative combat state.**