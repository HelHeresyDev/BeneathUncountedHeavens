# Beneath Uncounted Heavens — Fix the Confirmed Dash Lifecycle, Not the Timing Helper

The user has tested commit `55b302c` in graphical PIE. The bug persists.

**Observed behavior:** During an interrupted Dash, choosing a new Dash displays the proper red commitment section while moving the targeting cursor. Clicking the ground to confirm the new Dash turns the path green again, before Execute.

## What went wrong with the previous fix

The latest commit introduced `UBuhCombatSim::GbDashAwaitingExecute` and modified `GetDashElapsedMovementInstants()` to return zero when the flag is true.

This approach is insufficient.

`GetDashElapsedMovementInstants()` should remain a pure calculation based on the supplied authoritative combat state. It should not depend on global UI selection state.

The core problem is likely the Blueprint lifecycle that changes from targeting to confirmed selection.

Do not add another global flag or alter the Dash movement rules.

## 1. Inspect the exact Blueprint sequence

Use MCP to read back the actual saved Blueprint graphs for:

- `BP_CombatManager.BeginDashTargeting`
- `BP_CombatManager.SelectDashDestination`
- `BP_CombatManager.CancelTargeting`
- `BP_CombatManager` action confirmation and execution functions
- `WBP_CombatUI.EventTick`
- The left-click confirmation handler
- `DrawDashPath` call sites
- Back, Continue, and Execute handlers

Determine exactly when `GbDashAwaitingExecute` is set to true and false.

At each transition, record:

`TargetingAction`, `PlayerMove`, `State`, `CurrentState.Action`, `CurrentState.PhaseInstants`, pending destination, pending decision status, and `GbDashAwaitingExecute`.

The crucial test is the state **immediately after clicking the ground but before Execute**, followed by the state after the next EventTick.

Find the specific operation that changes the renderer back to green.

## 2. Represent pending movement decisions explicitly

Prefer keeping pending UI decisions on the actual `BP_CombatManager` instance rather than in a global static variable.

The system must distinguish:

| Situation | Expected Dash rendering |
|---|---|
| Existing Dash paused at reactable Instant | Remaining path green |
| Targeting replacement Dash | Fresh red commitment section |
| Replacement Dash confirmed, awaiting Execute | Fresh red commitment section |
| Continue selected for existing Dash | Resume old Dash; no new red lock |
| Replacement Dash executed | New commitment timing starts |

The targeting state and the confirmed-but-unexecuted state must be distinct.

When confirmation ends mouse targeting, do not clear the pending decision.

Do not treat `PlayerMove == 8` as sufficient to identify a new Dash, since the existing Dash may also have action ID 8.

Canceling a replacement should restore the existing continuation state without modifying authoritative combat progress.

## 3. Give the path renderer one owner

Rework only the relevant rendering control flow, not the renderer geometry or simulation.

The effective priority should be:

1. New movement currently being targeted.
2. New movement confirmed and awaiting Execute.
3. Existing movement awaiting Continue.
4. No movement path.

These states must be mutually exclusive.

For a new Dash, pass elapsed=0 to `DrawDashPath`.

For an existing continuing Dash, pass the value from authoritative `GetDashElapsedMovementInstants(CurrentState)`.

Restore `GetDashElapsedMovementInstants()` to a deterministic read-only function. Remove `GbDashAwaitingExecute` once no longer needed.

Do not manipulate CurrentState to change path colors.

## 4. Add a test that actually fails on this bug

The previous commit added no dedicated regression test for confirmed replacement Dash selection.

Create an integration test exercising the actual pending-decision transitions and renderer update path.

Test this sequence:

Existing Dash reaction (green) → Begin new Dash targeting (red) → Confirm ground destination (STILL RED) → one or more UI refreshes (STILL RED) → Cancel (old green continuation restored).

Repeat using a short new Dash of no more than 270 uu, which should have an entirely red path, and a longer Dash, which should have both red and green sections.

Also verify that Execute starts a new commitment period and Continue preserves the original Dash progress.

A test that merely invokes `DrawDashPath(..., 0)` is insufficient. That function already works.

## 5. Provide readable evidence in GitHub

Export the relevant saved Blueprint node connections to a text file, for example:

`tests/blueprint_exports/dash_selection_flow.md`

Include the event execution order, variable writes, branches, and DrawDashPath argument sources.

Use actual MCP graph readback, not a manually reconstructed or hypothetical graph.

This will let ChatGPT review the Blueprint behavior through GitHub instead of relying on descriptions of binary `.uasset` changes.

## 6. Verify and push

Compile and save changed Blueprints. Run all existing BUH automation suites and the new lifecycle regression test.

If graphical PIE verification is not possible, state that clearly. Do not claim the visual bug is fixed from C++ unit tests alone.

Update `notes.md` with the confirmed cause and `todo.md` with the latest status.

**Commit and push the completed changes to `origin/master` on `HelHeresyDev/BeneathUncountedHeavens`.**

Include the Blueprint export, tests, and updated source. Report the commit SHA and test results.

Do not implement Jump or change Dash balance values.

**Success criterion:** A new Dash retains its red initial commitment section after destination confirmation, while Continue correctly preserves the green remaining path of an existing Dash.

## Status (completed 2026-11-10)

- [x] §1 Inspected the exact Blueprint sequence via `read_graph_dsl` readback of `BeginDashTargeting`, `SelectDashDestination`, `CancelTargeting`, `ExecuteMove`, `ResetEncounter`, `EventTick`, `BeginPlay`, and the `WBP_CombatUI:EventGraph` Dash branches. The green-before-Execute cause is the after-confirm `DrawDashPath` reading the old authoritative `CurrentState` elapsed value while `PlayerMove==8` and `TargetingAction==-1`, with no record that a new destination was confirmed but not executed.
- [x] §2 Pending movement decisions are represented explicitly on the `BP_CombatManager` instance: `bDashAwaitingExecute` + `IsDashAwaitingExecute` (BlueprintPure). Targeting (`BeginDashTargeting` → false), confirmed-awaiting-Execute (`SelectDashDestination` → true), cancel (`ClearDashAwaitingExecute` → false), execute (`ExecuteMove` → false), and reset (`ResetEncounter` → false) are all wired. `PlayerMove == 8` is no longer treated as "new Dash".
- [x] §3 The path renderer has one owner per context. Active targeting and confirmed-awaiting-Execute both draw with elapsed `0` (red); an executed/continuing Dash draws with authoritative `GetDashElapsedMovementInstants(CurrentState)`. `GetDashElapsedMovementInstants` is restored to a pure calculation; `GbDashAwaitingExecute` is removed. `CurrentState` is never manipulated to change colors.
- [x] §4 `BUH.DashLifecycleRegressionTest` exercises the real pending-decision transitions and renderer path: reaction-pause green → targeting red → confirm STILL RED → UI refreshes STILL RED → cancel restores old green; short (≤270 uu) and long replacement Dashes; Execute starts a new commitment period; Continue preserves the original progress.
- [x] §5 `tests/blueprint_exports/dash_selection_flow.md` contains the actual MCP graph readback (event order, variable writes, branches, `DrawDashPath` argument sources).
- [x] §6 Compiled and saved both Blueprints; rebuilt C++ clean. All 9 `BUH.*` automation suites pass via `AutomationTestToolset.RunTests` in a headless MCP editor (9 passed, 0 failed). Graphical PIE visual confirmation remains user-driven (headless `-nullrhi` cannot click UMG buttons or capture path color); the C++ lifecycle test + on-disk graph readback prove the rule and its wiring.
- [x] `notes.md` §9.28 records the confirmed cause, fix, test, and the command-line trace-server flake workaround.
- [x] Committed and pushed to `origin/master`.