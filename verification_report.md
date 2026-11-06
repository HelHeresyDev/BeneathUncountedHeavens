# Technical Verification Report — General Continue Decision and Reaction Predictions

## 1. Exact C++ and Blueprint files/functions changed

### C++ files
- `CombatSim.h`:
  - Added `int32 StoppedReason = 0` to `FBuhTimelineSnapshot` (line 66).
- `CombatSim.cpp`:
  - `MoveName` (line 59): `case 7` returns `TEXT("Continue")` (renamed from "Continue Walk").
  - `GetReactionInfo` (line 1573): eligibility for decision 7 changed from `PlayerCurrentAction == 6` to `PlayerCurrentAction == 4 || PlayerCurrentAction == 5 || PlayerCurrentAction == 6`, matching `IsReactableWalk`.
  - `ResolveCombat` (line 272): `Decision == 7` handling — when `IsReactableWalk(F)` is true, the existing Walk state is preserved (no new action committed). Guard is `Decision >= 8`.
  - `GenerateTimeline` (line 1572): same `Decision == 7` handling. Set `Snapshots.Last().StoppedReason` at each stop point (reaction=5, KO=3, decision boundary=2, player ready=1, max instants=4).
  - `GetTimelineLengthText` (line 2060): uses the explicit `StoppedReason` field.
- `CombatSimAutomation.cpp`:
  - Step L: 4 prediction scenarios (Quick, Heavy, Guard, Dodge) with `bStopAtDecisionBoundary=true`.
  - Step M: Continue (decision 7) preserves Walk action and player walks.
  - Step N: Uninterrupted encounter — Walk vs Quick, reaction pause, Continue, walk resumes, reset.

### Blueprint files
- `WBP_CombatUI`:
  - `BtnContinueWalk` Button widget in `LeftPanelContent` VerticalBox at slot index 7.
  - `TxtContinueWalk` TextBlock child with text "Continue" (renamed from "Continue Walk").
  - `OnClicked(BtnContinueWalk)` calls `SetPlayerMove(7)` → `RefreshUI()` only.
  - `RefreshUI` visibility: `GetReactionInfo` → `BreakBuhReactionInfo` → `AND(bIsReaction, ContainsItem(LegalActions, 7))` → `ToVisibility(Boolean)` → `SetVisibility`.
- `BP_CombatManager.ExecuteMove`: passes `PlayerMove` (7) to `ResolveCombat`. Uses `GetCurrentState` (not fresh `MakeState`) when `bHasCurrentState` is true.

## 2. How Continue is represented and selected in the combat UI

**Implemented:** A "Continue" button (`BtnContinueWalk`) in `WBP_CombatUI` at `LeftPanelContent` slot index 7. TextBlock `TxtContinueWalk` displays "Continue".

**Selection flow:** Clicking `BtnContinueWalk` calls `SetPlayerMove(7)` → `RefreshUI()` only. No immediate execution. `BuildPredictions()` is omitted because it creates a fresh `MakeState` that does not preserve ongoing Walk phases or opponent commitment.

**Conditional visibility:** `RefreshUI` uses `GetReactionInfo` (BlueprintCallable static in `UBuhCombatSim`):
- `GetManager` → reaction state variables → `GetReactionInfo` → `BreakBuhReactionInfo` → `AND(bIsReaction, ContainsItem(LegalActions, 7))` → `ToVisibility(Boolean)` → `SetVisibility`.
- Visible only during a genuine reaction pause when the player has an ongoing reactable Walk action (4, 5, or 6). Does NOT rely on `PlayerMove` or `State`.

## 2b. Root cause of the missing button and the fix

**Symptom:** After selecting Walk and clicking Execute, the Continue button did not appear despite "Reaction opportunity" being displayed.

**Root cause:** The UI Walk buttons commit Actions 4 (Walk Toward) and 5 (Walk Away). There is no Walk destination button in the UI. `GetReactionInfo` only included decision 7 in `LegalActions` when `PlayerCurrentAction == 6`, so Actions 4 and 5 produced an empty `LegalActions` list and the button stayed Collapsed.

**Evidence:** Diagnostic `PrintString` nodes in `ExecuteMove` (added, used, then removed) confirmed in the graphical PIE log (`feedback.md`): `bReactionOpportunity=1`, `ReactionFighter=0`, `ObservedThreatAction=1`, `PlayerMove=6`. The `RefreshUI` visibility chain and button parent chain were structurally correct.

**Fix:** `GetReactionInfo` line 1573 changed from `if (Info.PlayerCurrentAction == 6)` to `if (Info.PlayerCurrentAction == 4 || Info.PlayerCurrentAction == 5 || Info.PlayerCurrentAction == 6)`, matching `IsReactableWalk`. `MoveName(7)` renamed to "Continue". UI text changed to "Continue".

## 3. How Continue preserves PlayerWalkTarget, current Walk phase, position, and remaining travel

In `ResolveCombat` and `GenerateTimeline`, when `Decision == 7` and `IsReactableWalk(F)` is true:
- `F.Action` remains 4, 5, or 6 (the committed Walk variant).
- `F.Phase` is unchanged.
- `F.PhaseInstants` is unchanged.
- `F.X` is unchanged.
- `F.WalkTarget` is unchanged.
- `F.WalkRemaining` is unchanged.

The simulation continues with the existing Walk state — no restart, no new action, no rewind.

## 4. How the opponent's committed action and preparation progress are preserved

When Continue is chosen (decision 7 for the player), the opponent's fighter state is not modified:
- The decision commit loop only affects the player's fighter (fighter 0) when `Decision == 7`.
- The opponent's fighter (fighter 1) has `OpponentDecision = -1` (not committed), so its `Action`, `Phase`, `PhaseInstants`, and `X` are preserved.
- The opponent's committed attack continues from its saved phase.

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

## 9. Whether the complete Continue sequence has been exercised through the real BP_CombatManager and WBP_CombatUI

**Verified via DSL inspection, node-level tracing, diagnostic prints in the graphical PIE log, and C++ tests.**

- **C++ combat logic:** `ResolveCombat` correctly handles decision 7 — preserves Walk action, phase, position, target, and remaining distance. The opponent's committed action is preserved. Verified by automation tests (Steps M and N).
- **`ExecuteMove` Blueprint flow:** Uses `GetCurrentState` (not fresh `MakeState`) when `bHasCurrentState` is true. Passes `PlayerMove` (7) to `ResolveCombat`. Opponent's committed action preserved.
- **Button handler:** `OnClicked(BtnContinueWalk)` calls `SetPlayerMove(7)` → `RefreshUI()` only.
- **Visibility logic:** `RefreshUI` uses `GetReactionInfo` with the fixed eligibility check (Actions 4, 5, 6).
- **Diagnostic evidence:** `PrintString` nodes in `ExecuteMove` confirmed `bReactionOpportunity=1`, `ReactionFighter=0`, `ObservedThreatAction=1`, `PlayerMove=6` in the graphical PIE log. The button was absent because `GetReactionInfo` did not include decision 7 for Actions 4/5. After the fix, the eligibility check covers all three Walk variants.
- **RefreshUI execution flow:** Restored and intact. All original UI updates plus the `GetReactionInfo` → `SetVisibility(BtnContinueWalk)` chain at the end.

**Gap:** Graphical PIE verification of button visibility and click behavior is still required. Headless PIE cannot click buttons or capture the game view.

## 10. Which interactions still require manual graphical PIE verification

1. **Continue button visibility:** Verify that the Continue button appears when Walk Toward or Walk Away is selected and a reaction opportunity occurs. Verify it is hidden during normal planning and after action completion.
2. **Continue button click:** Verify that clicking sets `PlayerMove=7` and refreshes the UI without executing.
3. **Graphical ghost positioning:** Verify ghosts are positioned correctly during a Continue sequence.
4. **Reaction pause UI:** Verify the reaction pause UI appears correctly when the opponent starts an attack while the player is walking.
5. **Full gameplay flow:** Walk → opponent attack → reaction pause → Continue → walk resumes → opponent attack continues → reset.
6. **"O: Dodge" label:** Verify whether the opponent move label is stale during a reaction opportunity (it displays `GetOpponentMove`, not the reaction threat).

## Summary

- Continue (decision 7) is implemented in C++ and supported by the Blueprint. `MoveName(7)` returns "Continue".
- `GetReactionInfo` eligibility now covers all reactable Walk actions (4, 5, 6), fixing the missing button for Walk Toward and Walk Away.
- The UI button text is "Continue". Visibility is driven by `GetReactionInfo`, not `PlayerMove`.
- All 4 automation suites pass.
- `ExecuteMove` uses the saved `CurrentState` during a reaction pause.
- The button handler calls `SetPlayerMove(7)` → `RefreshUI()` only.
- `RefreshUI` execution flow is intact.
- Graphical PIE verification of button visibility, click behavior, ghost positioning, and full gameplay flow is the remaining step.
