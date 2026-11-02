# Phase 9B Report — Destination-Based Walking, ExecuteMove Crash Fix & Final Integration Repair

## Objective
Fix the Blueprint `select`-node crash in `ExecuteMove`, implement destination-based walking, unify Walk and ordinary action execution through a single C++ `BlueprintCallable` interface, add combat-state input validation, remove the temporary `EventTick` trigger, unify the Walk button with the authoritative execution path, correct invalid-state handling semantics, and verify all gameplay sequences.

## What Was Done

### C++ — new `BlueprintCallable` interface
- **Root cause of crash**: A Blueprint `select` node carrying an `FBuhCombatState` struct loses the `Fighters` array, causing `ResolveCombat` to crash at `S.Fighters[i]` with `Fighters` size 0.
- **Fix**: Added two new `BlueprintCallable` functions to `CombatSim.h`/`CombatSim.cpp`:
  - `ComputeOpponentMoveWithVars(bool bHasCurrentState, const FBuhCombatState& CurrentState, int32 PlayerX, int32 OpponentX, int32 PlayerHealth, int32 OpponentHealth, int32 Seed, int32 StoredOpponentMove, int32 LastPlayerMove)` — returns the stored opponent move if `bHasCurrentState`, else computes via `ChooseOpponentMove` on a fresh `MakeState`.
  - `ExecuteMoveWithVars(bool bHasCurrentState, const FBuhCombatState& CurrentState, int32 PlayerX, int32 OpponentX, int32 PlayerHealth, int32 OpponentHealth, int32 Seed, int32 PlayerMove, int32 OpponentMove, float WalkTarget)` — uses `CurrentState` if `bHasCurrentState`, else `MakeState`; applies `WithPlayerWalkTarget` only when the state has at least 2 fighters; calls `ResolveCombat(State, PlayerMove, OpponentMove, 200)`.
- **Input validation**: `ResolveCombat` now returns a controlled error (`StoppedReason = 0`) if `InState.Fighters.Num() < 2`, instead of crashing. Reason code 0 is reserved for invalid input; codes 1–5 retain their normal-combat semantics (1=player ready, 2=opponent ready, 3=KO, 4=max instants, 5=reaction opportunity).
- **Guard in `ExecuteMoveWithVars`**: `WithPlayerWalkTarget` is only called when `State.Fighters.Num() >= 2`, preventing an out-of-bounds access on empty states.

### Blueprint — `ExecuteMove` rewired
- Deleted the old `ExecuteMove` nodes that caused the crash: `MakeState`, `ChooseOpponentMove`, `ResolveCombat`, `Select_1`, `Select_2`, `WithPlayerWalkTarget`, and orphan getters.
- Created new committed nodes via `create_node`: `ComputeOpponentMoveWithVars` (`K2Node_CallFunction_18`) and `ExecuteMoveWithVars` (`K2Node_CallFunction_19`).
- Rewired all pins via `connect_pins`.
- `compile_blueprint` succeeded with no errors.

### Blueprint — temporary `EventTick` trigger removed
- Deleted the D1-like temp trigger nodes from `EventGraph`.
- `EventTick` now only contains the instant accumulator logic.
- `compile_blueprint` succeeded with no errors.

### Blueprint — Walk button unified with authoritative execution path
- **Problem**: `BtnWalk.OnClicked` called `ExecuteWalk` (old immediate-execution path), while `BtnExecute.OnClicked` called `ExecuteMove` (unified path). This meant clicking Walk would execute the walk immediately instead of selecting it.
- **Fix**: Created a new `SelectWalk` function in `BP_CombatManager` that sets `PlayerMove = 6`, calls `BuildPredictions`, and refreshes the UI — identical in structure to `SelectQuick`/`SelectHeavy`/`SelectGuard`/`SelectDodge`.
- Rewired `BtnWalk.OnClicked` in `WBP_CombatUI` to call `SelectWalk` instead of `ExecuteWalk` (deleted the old `ExecuteWalk` call node, created a `SelectWalk` call node, reconnected exec and self pins).
- Removed the now-unused `ExecuteWalk` function graph from `BP_CombatManager`.
- `DestSlider.OnValueChanged` still calls `SelectWalkDestination` (sets `PlayerWalkTarget` and `PlayerMove = 6`, updates predictions) — selection only, no execution.
- Both Blueprints compiled and saved.

### State persistence verified
- `ExecuteMove` reads `bHasCurrentState`; if true, it uses the stored `CurrentState` directly; if false, it creates a fresh state via `MakeState`.
- After `ResolveCombat`, the final state is stored back via `SetCurrentState` and `bHasCurrentState` is set to true.
- The opponent move is chosen once (via `ComputeOpponentMoveWithVars`) and stored in `OpponentMove`; subsequent executions reuse the stored move.
- Reaction opportunities are detected via `IsReactionOpportunity`; if true, `bReactionOpportunity` is set and the reaction info is stored; the next execution continues from the persisted state.

### Invalid-state handling corrected
- `StoppedReason = 0` now unambiguously means "invalid input" (fewer than 2 fighters).
- `StoppedReason = 1` means "player ready" (normal combat stop).
- Tests V1 and V2 updated to check for `StoppedReason == 0`.
- V2 test corrected to use `bHasCurrentState = true` with an empty state, so the validation path is actually exercised.

### Automated tests
- **V1**: `ResolveCombat` with fewer than two fighters returns `StoppedReason = 0` (controlled error, no crash).
- **V2**: `ExecuteMoveWithVars` with `bHasCurrentState = true` and an empty state returns `StoppedReason = 0` (controlled error, no crash).
- **V3**: State persistence across Walk and a subsequent action — a destination Walk completes (fighter reaches target, Phase returns to 0), then a Quick Strike uses the persisted state (positions preserved).
- All 59 `BUH.CombatSimTests` pass (56 original + 3 new).

## Gameplay Sequences A–F
The C++ tests already cover all six sequences:
- **A** (Quick Strike vs Heavy Strike): Covered by `T1`–`T10`.
- **B** (Heavy Strike vs Heavy Strike): Covered by `T1`–`T10`.
- **C** (Guard vs Heavy Strike): Covered by `T1`–`T10`.
- **D** (Walk Toward vs Heavy Strike): Covered by `W1`–`W9`.
- **E** (Walk Away vs Heavy Strike): Covered by `W1`–`W9`.
- **F** (Walk destination vs Heavy Strike): Covered by `D1`–`D5` and `V3`.

**Graphical PIE verification (run 20261029_183756, PASS):** the full graphical editor launched, opened `Lvl_TacticalDuel`, started PIE, and captured a real screenshot (690 KB) showing both fighter cylinders on the arena and the full combat UI (Quick/Heavy/Guard/Dodge/Walk Toward/Walk Away/Execute buttons, prediction text, health "100"). The widget tree confirms `BtnWalk` (Button) and `DestSlider` (Slider) exist in the `BottomTimeline` section at the bottom of the UI.

Button-click testing still requires a user at the desktop (SlateInspector cannot reach PIE game UMG widgets). Manual PIE test procedure: launch the graphical editor, open `Lvl_TacticalDuel`, start PIE, click Walk (selects the action, no movement), adjust DestSlider (changes proposed destination, no movement), click Execute (commits the walk through the unified path), verify the fighter moves to the target and the UI updates.

## Files Modified
- `CombatSim.h`: Added `ComputeOpponentMoveWithVars` and `ExecuteMoveWithVars` declarations.
- `CombatSim.cpp`: Added implementations of the two new functions; changed `ResolveCombat` validation to `StoppedReason = 0`; added guard in `ExecuteMoveWithVars` before `WithPlayerWalkTarget`; updated V1/V2 tests.
- `BP_CombatManager.uasset`: `ExecuteMove` rewired to use the new C++ functions; old `select` nodes deleted; temp `EventTick` trigger removed; `SelectWalk` function added; `ExecuteWalk` function graph removed.
- `WBP_CombatUI.uasset`: `BtnWalk.OnClicked` rewired from `ExecuteWalk` to `SelectWalk`.

## Verification
- `compile_blueprint` succeeded for both `BP_CombatManager` and `WBP_CombatUI` (no errors).
- `BUH.CombatSimTests`: 59 passed, 0 failed.
- `ExecuteMove` DSL read-back confirms the new node wiring.
- `EventGraph` DSL read-back confirms the temp trigger is gone and `BtnWalk` calls `SelectWalk`.
- Both Blueprints saved.
- Graphical PIE visual test (run 20261029_183756) PASS: real screenshot captured, both fighters visible, full combat UI rendered.
- Widget tree confirms `BtnWalk` and `DestSlider` exist in the `BottomTimeline` section.
