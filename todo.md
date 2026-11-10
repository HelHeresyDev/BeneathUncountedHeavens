# Beneath Uncounted Heavens — Fix New Dash Turning Green After Confirmation

I have reviewed the GitHub source, including CombatSim.cpp, PathRenderer.cpp, and notes.md.

The user has reproduced the following sequence in graphical PIE:

1. An existing Dash is interrupted during its reactable phase.
2. Its remaining path is green — CORRECT.
3. The player chooses a NEW Dash.
4. During mouse targeting, the new path has a red initial section — CORRECT.
5. The player clicks the ground to confirm the new destination.
6. The path immediately becomes green again, even though the new Dash has not been executed — WRONG.

**This is a confirmed-selection state bug, not a Dash timing or material bug.**

## Root-cause hypothesis

The existing EventTick distinguishes targeting with `TargetingAction == 8`.

But confirmation calls CancelTargeting, setting `TargetingAction = -1`.

At that point:
- The new Dash is selected and confirmed.
- The old Dash is still in authoritative CurrentState.
- The combat manager is still handling a reaction opportunity.
- The execution-path renderer reads elapsed Instants from the OLD Dash.

Consequently, the old Dash's elapsed value turns the newly confirmed path green.

Inspect the actual Blueprint graph and confirm this transition.

## Required fix

Distinguish these three states:

**A. Targeting a new Dash**
- Cursor selects a destination.
- Render the new Dash with elapsed=0.

**B. New Dash confirmed but awaiting Execute**
- The cursor no longer changes the destination.
- Preserve a pending, confirmed Dash decision.
- Render its new commitment window with elapsed=0.
- Do NOT read elapsed Instants from the old Dash.
- Keep the original destination and preview visible.

**C. Existing Dash being continued**
- No pending replacement action.
- Read authoritative Dash progress from CurrentState.
- Render only its remaining path using the actual elapsed Instants.
- Once reactable, the remaining line is green.

Use the existing state variables if they are sufficient. Otherwise, introduce the smallest explicit pending-decision state needed to distinguish B from C.

Do not use `PlayerMove == 8` alone: both the old Dash and new Dash have action ID 8.

Do not use `TargetingAction == 8` alone: that becomes false once the new destination is confirmed.

## Lifecycle requirements

- Begin new Dash targeting: establish a pending replacement decision.
- Confirm destination: preserve the pending decision and its new target.
- Cancel before confirmation: restore the old action's continuation preview when applicable.
- Cancel/back after confirmation: discard the pending replacement and restore the old preview when applicable.
- Execute new Dash: commit a genuinely new Dash with its own startup and three locked movement Instants.
- Continue old Dash: resume the original action without restarting its lock window.
- Switch to Quick/Heavy/Guard/Dodge/Walk: clear or replace the pending Dash preview appropriately.
- Reset: clear both pending and targeting state.

Never mutate the authoritative CurrentState merely to make the preview red.

## Make path rendering mutually exclusive

Audit the actual EventTick branches and every caller of DrawDashPath.

Recommended effective priority:

1. Active movement targeting preview.
2. Confirmed new movement awaiting execution.
3. Ongoing movement continuation visualization.
4. No movement path.

Only one branch should update the renderer per refresh.

Ensure confirmation doesn't accidentally fall through to the previous Dash continuation branch.

Use targeted Blueprint node edits. Do not rewrite the entire EventGraph.

## Regression verification

Reproduce in an initialized PIE game world:

- Existing Dash reaction → green remaining path.
- Select new Dash → initial red section.
- Confirm destination → initial red section remains.
- Cancel confirmed Dash → old green continuation restored.
- Select and confirm new Dash again → red section.
- Execute new Dash → new action starts with its own locked window.
- Continue old Dash instead → old action progress preserved.
- Fresh encounter Dash → normal red/green preview.
- Switching actions and Reset → no stale path.

Inspect actual rendered segment colors after each UI transition, not just the CombatSim result.

Run all existing BUH automation suites.

Compile, save, and read back the affected Blueprint nodes.

Keep working cursor/input behavior, XY targeting, and Dash simulation unchanged.

Remove the large yellow diagnostic banner after graphical verification.

Do not implement Jump, make unrelated UI changes, or create a Git commit without authorization.

**Success: A newly selected Dash retains its initial red commitment section throughout targeting AND after confirmation, until the new action is executed. An interrupted Dash being continued still displays its green remaining path.**