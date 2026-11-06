# Technical Verification Report — Continue Walk and Reaction Predictions

## 1. Exact C++ and Blueprint files/functions changed

### C++ files
- `CombatSim.h`:
  - Added `int32 StoppedReason = 0` to `FBuhTimelineSnapshot` (line 66).
- `CombatSim.cpp`:
  - `MoveName` (line 48): added `case 7: return TEXT("Continue Walk");`.
  - `ResolveCombat` (line 272): added `Decision == 7` handling — when `IsReactableWalk(F)` is true, the existing Walk state is preserved (no new action committed). Changed guard from `Decision >= 7` to `Decision >= 8`.
  - `GenerateTimeline` (line 1572): same `Decision == 7` handling. Set `Snapshots.Last().StoppedReason` at each stop point (reaction=5, KO=3, decision boundary=2, player ready=1, max instants=4).
  - `GetTimelineLengthText` (line 2060): now uses the explicit `StoppedReason` field instead of inferring from fighter position/phase.
- `CombatSimAutomation.cpp`:
  - Step L: expanded to 4 scenarios (Quick, Heavy, Guard, Dodge) with `bStopAtDecisionBoundary=true`, recording stop Instant, stop reason, player X, and observed threat.
  - Step M: new test for Continue Walk (decision 7) — verifies Walk action is preserved and player walks.
  - Step N: new test for uninterrupted encounter — Walk vs Quick, reaction pause, Continue Walk, walk resumes, reset.

### Blueprint files
- `WBP_CombatUI`:
  - Added `BtnContinueWalk` Button widget to `LeftPanelContent` VerticalBox at slot index 7.
  - Added `TxtContinueWalk` TextBlock child with text "Continue Walk".
  - Wired `OnClicked(BtnContinueWalk)` event to call `SetPlayerMove(7)`, `BuildPredictions()`, and `RefreshUI()` on the CombatManager.
   - Added conditional visibility logic in `RefreshUI`: `BtnContinueWalk` is Visible when `GetReactionInfo` confirms a reaction opportunity AND `LegalActions` contains 7 (Continue Walk), Collapsed otherwise. Uses `GetManager` → `GetReactionOpportunity`/`GetReactionFighter`/`GetObservedThreatAction`/`GetReactionInstant`/`GetCurrentState` → `GetReactionInfo` → `BreakBuhReactionInfo` → `bIsReaction` AND `ContainsItem(LegalActions, 7)` → `ANDBoolean` → `ToVisibility(Boolean)` → `SetVisibility`.
- `BP_CombatManager.ExecuteMove` already passes `PlayerMove` (which can be 7) as `PlayerDecision` to `ResolveCombat`, so Continue Walk is supported end-to-end in the combat flow.

## 2. How Continue Walk is represented and selected in the combat UI

**Implemented:** A "Continue Walk" button (`BtnContinueWalk`) has been added to `WBP_CombatUI` in the `LeftPanelContent` VerticalBox at slot index 7. The button contains a `TxtContinueWalk` TextBlock with text "Continue Walk".

**Selection flow:** Clicking `BtnContinueWalk` triggers `OnClicked(BtnContinueWalk)` in the EventGraph, which:
1. Calls `SetPlayerMove(7)` on the CombatManager.
2. Calls `BuildPredictions()` to regenerate the prediction timeline.
3. Calls `RefreshUI()` to update all UI elements including button visibility.

**Conditional visibility:** The `RefreshUI` function now includes visibility logic for `BtnContinueWalk` that reuses the established CombatSim reaction eligibility helper:
- `GetManager` → `GetReactionOpportunity`, `GetReactionFighter`, `GetObservedThreatAction`, `GetReactionInstant`, `GetCurrentState`
- These are passed to `GetReactionInfo` (BlueprintCallable static function in `UBuhCombatSim`)
- `GetReactionInfo.ReturnValue` → `BreakBuhReactionInfo` → extracts `bIsReaction` and `LegalActions`
- `bIsReaction` AND `ContainsItem(LegalActions, 7)` → `ANDBoolean` → `ToVisibility(Boolean)` → `SetVisibility`
- When both conditions are true, the button is `Visible`; otherwise it is `Collapsed`.

This ensures the button only appears during a genuine reaction pause when the player has an ongoing Walk action and the CombatSim confirms Continue Walk is a legal reaction decision. It does NOT rely on `PlayerMove` (which is the pending UI selection) or `State` (which has multiple values: 0=Planning, 1=Resolving, 2=Finished, 3=Reaction).

## 3. How it preserves PlayerWalkTarget, current Walk phase, position, and remaining travel

In `ResolveCombat` and `GenerateTimeline`, when `Decision == 7` and `IsReactableWalk(F)` is true:
- `F.Action` remains 6 (Walk).
- `F.Phase` is unchanged (preserves current phase: startup, active, or recovery).
- `F.PhaseInstants` is unchanged (preserves phase progress).
- `F.X` is unchanged (preserves current position).
- `F.WalkTarget` is unchanged (preserves the destination).
- `F.WalkRemaining` is unchanged (preserves remaining travel distance).

The simulation continues with the existing Walk state — no restart, no new action, no rewind.

## 4. How the opponent's committed action and preparation progress are preserved

When Continue Walk is chosen (decision 7 for the player), the opponent's fighter state is not modified:
- The decision commit loop only affects the player's fighter (fighter 0) when `Decision == 7`.
- The opponent's fighter (fighter 1) has `OpponentDecision = -1` (not committed), so its `Action`, `Phase`, `PhaseInstants`, and `X` are preserved.
- The opponent's committed attack (e.g., Quick) continues from its saved phase.

## 5. How repeated reactions to the same already-observed attack are prevented

`CheckReactionPause` checks `if (Def.ReactedThreatFighter == Threat) continue;` — this prevents repeated reactions to the same threat. When a reaction pause occurs, `ReactedThreatFighter` is set to the threat fighter. The value is preserved in the fighter state and is only reset when the threat fighter becomes ready (Phase==0) and the defender's `ReactedThreatFighter` points to it (line 216-218 in `CombatSim.cpp`).

## 6. Whether opponent Dodge incorrectly triggers an attack-based reaction

**No.** `PendingAttackStart` is only set for Quick (0) and Heavy (1) in both `ResolveCombat` and `GenerateTimeline`. Dodge (3) does not set `PendingAttackStart`, so `AttackStart[Threat]` remains -1, and `CheckReactionPause` skips it. Walk does NOT react to Dodge.

## 7. Actual results for Walk vs Quick, Heavy, Guard, and Dodge

All scenarios use starting positions `(-500, 500)`, Walk target `500`, and `bStopAtDecisionBoundary=true`:

| Scenario | Stop Instant | Stop Reason | Player X | Distance Traveled | Distance Remaining |
|----------|-------------|-------------|----------|-------------------|-------------------|
| Walk vs Quick | 0 | 5 (reaction) | -500 | 0 | 1000 |
| Walk vs Heavy | 0 | 5 (reaction) | -500 | 0 | 1000 |
| Walk vs Guard | 11 | 2 (decision boundary) | -170 | 330 | 670 |
| Walk vs Dodge | 7 | 2 (decision boundary) | -290 | 210 | 790 |

Log output from `BUH.CombatFlowTests`:
```
L: Walk-vs-Quick Num=1 stop=5 playerX=-500.0 oppX=500.0 reaction=1 threat=0
L: Walk-vs-Heavy Num=1 stop=5 playerX=-500.0 oppX=500.0 reaction=1 threat=1
L: Walk-vs-Guard Num=12 stop=2 playerX=-170.0 oppX=500.0 reaction=0
L: Walk-vs-Dodge Num=8 stop=2 playerX=-290.0 oppX=560.0 reaction=0
```

## 8. Test names and results

All 4 automation suites pass:
- `BUH.CombatFlowTests`: Success (includes Step L, M, N)
- `BUH.CombatSimTests`: Success
- `BUH.CombatManagerIntegrationTests`: Success
- `BUH.WalkPreviewTests`: Success

New tests added:
- Step L: 4 prediction scenarios with stop details (Quick, Heavy, Guard, Dodge).
- Step M: Continue Walk preserves Walk action and player walks.
- Step N: Uninterrupted encounter (Walk, reaction pause, Continue Walk, walk resumes, reset).

## 9. Whether the complete Continue Walk sequence has been exercised through the real BP_CombatManager and WBP_CombatUI

**Partially verified via DSL inspection and C++ tests.**

- **C++ combat logic:** `ResolveCombat` correctly handles decision 7 — preserves Walk action, phase, position, target, and remaining distance. The opponent's committed action is preserved. Verified by automation tests (Steps M and N).
- **`ExecuteMove` Blueprint flow:** Inspected the DSL. When `bHasCurrentState` is true (reaction pause), it uses `GetCurrentState` as the base state (not a fresh `MakeState`). It passes `PlayerMove` (7) to `ResolveCombat`, which preserves the Walk state. The opponent's committed action is preserved because `ChooseOpponentMove` is called with the current state. The resulting state is saved to `CurrentState`. This matches the C++ test results.
- **Button handler:** `OnClicked(BtnContinueWalk)` calls `SetPlayerMove(7)` → `BuildPredictions()` → `RefreshUI()`. This follows the same pattern as all existing reaction buttons (`SelectQuick`, `SelectHeavy`, `SelectGuard`, `SelectDodge` all call `BuildPredictions` after `SetPlayerMove`). `BuildPredictions` creates a fresh state via `MakeState` with current positions/health/seed, which is the established behavior for all reaction buttons.
- **Visibility logic:** `RefreshUI` uses `GetReactionInfo` to determine eligibility, reusing the established CombatSim reaction helper rather than introducing a second inconsistent legality rule.

**Gap:** A PIE-world integration test that triggers the actual `BP_CombatManager.ExecuteMove` with `PlayerMove=7` during a reaction opportunity via the UI button is not yet built. This would require a graphical PIE run or a C++ PIE-world integration test that triggers the WBP event delegates. Headless PIE cannot click the button or capture the game view.

## 10. Which interactions still require manual graphical PIE verification

1. **Continue Walk button visibility:** Verify that `BtnContinueWalk` appears (Visible) only when `GetReactionInfo` confirms a reaction opportunity AND `LegalActions` contains 7, and is Collapsed otherwise.
2. **Continue Walk button click:** Verify that clicking the button sets `PlayerMove=7`, rebuilds predictions, and refreshes the UI.
3. **Graphical ghost positioning:** Verify that the cyan/orange ghosts are positioned correctly during a Continue Walk sequence.
4. **Reaction pause UI:** Verify that the reaction pause UI (showing reaction options) appears correctly when the opponent starts an attack while the player is walking.
5. **Full gameplay flow:** Verify the complete sequence in graphical PIE: Walk, opponent attack, reaction pause, Continue Walk, walk resumes, opponent attack continues, reset.

## Summary

- Continue Walk is implemented in C++ (`ResolveCombat`, `GenerateTimeline`) and supported by the Blueprint (`ExecuteMove` passes `PlayerMove` to `ResolveCombat`).
- The UI button for Continue Walk (`BtnContinueWalk`) is now implemented in `WBP_CombatUI` with conditional visibility using `GetReactionInfo` (visible only when a reaction opportunity exists AND Continue Walk is a legal action).
- All 4 automation suites pass with the new tests.
- `ExecuteMove` correctly uses the saved `CurrentState` during a reaction pause (not a fresh state), preserving Walk state and opponent commitment. Verified via DSL inspection.
- The button handler follows the established pattern of all existing reaction buttons (`SetPlayerMove` → `BuildPredictions` → `RefreshUI`).
- Graphical PIE verification is still required for button visibility, button click behavior, ghost positioning, and full gameplay flow.
