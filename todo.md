# BUH — Fix Walk Path Disappearing at Reaction Pause

The user tested restored commit `5ea1e38` in graphical PIE.

**Confirmed working:**
- Walk execution.
- Reaction detection.
- Continue button.
- Continue movement.
- Clean startup.

**Remaining bug:** The green Walk path disappears when combat stops for a reaction.

The screenshot shows a valid reaction with Continue available, but no remaining green movement line.

## Suspected cause

The known-good `BP_CombatManager.ExecuteMove` graph contains a `ClearWalkPath` call after combat resolution.

Meanwhile, `WBP_CombatUI.EventTick` draws the Walk preview primarily in Planning (`State == 0`).

When a reaction changes the manager to State 3, the existing path is cleared and not restored.

Verify the actual saved graphs before editing.

## Required behavior

When Walk is interrupted:

- Preserve the fighter's current authoritative position.
- Preserve the original Walk destination.
- Display a green line from the current fighter position to that destination.
- Keep the line visible while the player considers the reaction.
- Continue remains available.
- Selecting Continue resumes the existing Walk, not a new movement action.
- If another reaction happens, update the remaining line again.
- Once the Walk finishes, remove the line.
- If the player abandons Walk for another action, remove the old line.

## Implementation

1. Inspect `BP_CombatManager.ExecuteMove`, especially `ClearWalkPath`, state assignment, and `RefreshUI`.

2. Inspect the current path-rendering branches in `WBP_CombatUI.EventTick`.

3. For an active Walk reaction, obtain the actual remaining destination from `CurrentState.Fighters[0].WalkTarget` and `WalkTargetY`.

4. Use `CurrentState.Fighters[0].X/Y` or the authoritative real fighter location for the beginning of the line.

5. Draw the remaining path using the existing `DrawWalkPath` or `DrawGroundLine` API.

6. Make sure this happens AFTER any unconditional clearing, or conditionally skip clearing only when preserving a valid interrupted movement.

7. Do not use the yellow `DestPlayer` actor as the authoritative endpoint during a reaction. Its location may be stale or the actor may be hidden.

8. Do not rely on `PlayerMove == 6` during a reaction. The player may have selected Continue (`PlayerMove == 7`), while the active movement remains Walk in `CurrentState`.

9. Avoid redrawing every Tick unless necessary. Prefer updating the persistent PathRenderer when the authoritative movement/reaction state changes.

10. Preserve Dash and Jump path behavior. Do not modify their coloring, movement timing, or interruption rules.

## Verification

Test in graphical PIE:

- Fresh startup has no path.
- Selecting Walk displays a green line.
- Executing Walk moves the fighter correctly.
- At a reaction pause, the remaining green line persists.
- Selecting Continue does not erase the remaining path prematurely.
- A second reaction displays the shorter remaining line.
- Completing Walk clears the path.
- Choosing a replacement action clears the abandoned Walk path.
- Dash and Jump still behave exactly as before.

Run existing BUH automation tests and add a focused renderer lifecycle test if possible.

Compile and save affected Blueprints, read back the actual changed graph, and update notes.md and todo.md.

Commit and push to `origin/master` on `HelHeresyDev/BeneathUncountedHeavens`.

Report the commit SHA, confirmed cause, and verification results.

**Success: Walk's green trajectory stays visible throughout an interrupted Walk and disappears only when the movement completes or is abandoned.**

## Status (implementation complete — graphical PIE verification pending)

- **Cause confirmed:** `ExecuteMove` ended with an unconditional `ClearWalkPath`; `EventTick` only drew the Walk line while `State == 0` and `PlayerMove == 6` from the `DestPlayer` actor, so a reaction (`State == 3`) cleared the line and nothing redrew it.
- **Fix implemented:** new C++ `UBuhCombatSim::UpdateWalkPath(PC, FinalState)` draws the remaining line from the fighter's current `(X, Y)` to the original `WalkTarget` when `FinalState.Fighters[0].Action == 6` and the fighter is not yet at the destination, else clears. `ExecuteMove` now calls `UpdateWalkPath(GetPlayerController 0, _returnvalue_2)` (the `GetFinalState` output) instead of the unconditional `ClearWalkPath`.
- **Headless-verified:** C++ rebuilt clean; graph edited surgically (node delete/create/wire — `write_graph_dsl` cannot re-emit the bool-set nodes); `compile_blueprint` clean; read-back shows `UpdateWalkPath` with bool-sets intact; `save_assets` succeeded; **22/22 `BUH` automation tests pass**.
- **Pending (user graphical PIE):** confirm the 8 verification scenarios above — especially that the remaining green line persists through a reaction pause, survives a Continue, shortens on a second reaction, and clears on completion/abandonment, with Dash/Jump unchanged.
- **Not yet committed:** held for the graphical PIE confirmation per the "no unverified behavior" policy.