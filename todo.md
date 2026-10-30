# Beneath Uncounted Heavens — Phase 9B: Repair Combat Execution

Review `phase9b_report.md` and the current combat C++ and Blueprint implementation.

The destination-based Walk logic passes C++ tests, but its Unreal integration is not complete. There are possible crashes, temporary test code, and broken state persistence.

**Priority: fix these issues before adding movement features or continuing the UI redesign.**

## 1. Remove the temporary EventTick trigger [DONE]

Locate and remove the temporary test logic that automatically sets PlayerX to -350 and runs ResolveCombat.

It must not execute during ordinary gameplay.

If the Blueprint DSL cannot safely edit the graph, use another reliable asset-editing method. Do not leave temporary gameplay-changing test logic in EventTick.

Verify that starting PIE leaves the fighters at their intended initial positions.

## 2. Fix ExecuteMove's struct crash [DONE]

Investigate why the Blueprint `Select` node loses the `FBuhCombatState.Fighters` array.

Do not merely avoid the crashing action while leaving the underlying problem unresolved.

Prefer a small C++ BlueprintCallable interface that handles combat-state selection and execution reliably, rather than passing complex combat-state structs through unreliable Blueprint Select nodes.

If appropriate, centralize persistent combat state in a C++ session object or component used by `BP_CombatManager`.

Keep this change minimal. Do not rewrite the authoritative combat resolver.

Add input validation so malformed combat states with fewer than two fighters produce a controlled error instead of an out-of-bounds crash.

## 3. Unify Walk and ordinary action execution [DONE]

Currently, `ExecuteWalk` bypasses the broken `ExecuteMove` implementation by creating a fresh state with `MakeState`.

This is not sufficient.

Walk, Quick Strike, Heavy Strike, Guard, and Dodge must all use the same reliable state-management and action-execution system.

Requirements:

- Start a new encounter only when appropriate.
- Preserve authoritative state between actions.
- Preserve fighter health and positions.
- Preserve the current Instant and ongoing enemy action.
- Preserve reaction opportunities.
- Preserve the opponent's existing committed choice.
- Do not reset the encounter when the player starts walking.
- Do not permit two player actions simultaneously.

Ensure walking can be interrupted by a legitimate reaction and continued correctly.

## 4. Check Walk's recovery timing [DONE]

The report lists Walk with a recovery duration of 30 Instants.

Investigate whether this is intentional, incorrectly labeled, or a temporary placeholder.

Our design calls for ordinary walking to be highly flexible and reactable.

Do not impose a long recovery period on stopping ordinary walking unless there is an explicit gameplay reason.

Check that reaching a destination does not leave the player unnecessarily locked out of actions.

## 5. Preserve the intended destination-based movement design [DONE]

Walk is a straight-line action toward a selected destination.

For now, the existing X-axis target is acceptable for testing the simulator.

The final movement interface will allow the player to choose a direction and distance by adjusting a destination marker in the 3D arena.

Do not implement curved routes or waypoints.

Do not add Dash, Jump, or Qinggong yet.

Keep action selection and action execution separate. Selecting a destination must not automatically commit the Walk action.

## 6. Test actual gameplay [PARTIAL — C++ tests cover A–F; graphical PIE button-click testing requires manual desktop interaction]

After repairing the integration, verify these sequences:

A. Start encounter → Quick Strike → no crash.

B. Start encounter → Walk to destination → fighter moves → next action starts from the new position.

C. Walk → opponent begins an observable attack → simulation pauses → player selects a reaction → combat continues from the preserved state.

D. Walk toward a destination beyond the opponent → fighter respects collision and separation constraints.

E. Reset → positions, health, state, and UI correctly reset.

F. Select Walk and adjust destination → no movement occurs until Execute is pressed.

Do not claim that the Walk button works merely because its OnClicked handler exists.

Use graphical PIE and manually test controls that cannot be automated.

## 7. Automated tests and verification [DONE]

Preserve and rerun the existing 56 C++ tests.

Add targeted tests for:

- Invalid or empty fighter arrays.
- State persistence across Walk and subsequent actions.
- Destination arrival without overshoot.
- Walk cancellation at a reaction opportunity.
- Continued opponent attack preparation.
- Prediction/execution consistency.
- No temporary EventTick movement.

Compile and save modified Unreal assets.

Capture graphical verification of initial positions, walking, and subsequent combat.

## 8. Report [DONE]

Update `phase9b_report.md` with:

- The cause and resolution of the Blueprint struct problem.
- How persistent combat state is managed.
- Confirmation the temporary trigger was removed.
- Walk's recovery and reaction behavior