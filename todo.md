# Beneath Uncounted Heavens — Stage C: Authoritative 2D Ground Movement

## Context

Stage B is complete. Graphical PIE confirms:

- Mouse-based ground targeting works.
- The green Walk line is visible and follows the ground.
- The destination marker is a flat yellow disc.
- Clicking confirms the Walk destination.
- The green path persists during planning.
- The cursor and combat UI remain functional.
- All four existing automation suites pass.

**The remaining problem:** CombatSim is still one-dimensional. The manager stores `PlayerWalkTargetY` and `PlayerY` separately, but authoritative combat execution and predictions primarily use X.

We now need to make ground-plane movement genuinely two-dimensional.

Read `todo.md`, `tmp_opencode/stage_a_analysis.md`, `AGENTS.md`, and relevant project notes before modifying code.

## 1. Establish one authoritative position representation

Inspect all existing position and target fields.

Use a consistent ground-plane coordinate representation, preferably `FVector2D`, for:

- Player and opponent positions.
- Movement destinations.
- Movement directions.
- Predicted fighter positions.
- Distances and attack range.

Preserve Unreal's world-space Z coordinate for rendering fighters above the ground.

Avoid maintaining independent X-only and XY representations that can disagree.

If legacy fields must be temporarily retained for Blueprint compatibility, provide explicit conversion and a clear migration plan. Do not allow legacy X values to overwrite authoritative XY positions.

Do not add Jump height or airborne physics in this phase.

## 2. Implement 2D Walk movement

For Walk destination action 6:

- Read the fighter's current XY position.
- Read the selected XY destination.
- Calculate the normalized direction toward that destination.
- Advance using the established Walk speed of 30 Unreal units per Instant, unless the existing simulation specifies otherwise.
- Clamp movement to the destination to prevent overshooting.
- Determine completion using remaining two-dimensional distance.
- Preserve the existing action phases and reaction timing.

A diagonal Walk must travel at the same total speed as a horizontal Walk.

For example, moving diagonally must not apply 30 uu independently to X and Y, which would incorrectly increase total speed.

Keep Walk as one straight-line movement action.

Also preserve the behavior of Walk Toward (4) and Walk Away (5), defining their directions using both fighters' current XY positions.

## 3. Update combat distance and range

Replace X-only distance checks where two-dimensional separation is relevant.

This includes:

- Quick Strike range.
- Heavy Strike range.
- Minimum-distance calculations.
- Fighter separation.
- Dodge displacement and direction.
- Any collision or overlap safeguards.
- Prediction summaries.

Distance should be based on Euclidean ground-plane separation.

For example, fighters separated by (300, 400) have a distance of 500 uu, not 300 or 400.

Preserve current combat rules and damage values.

Do not add automatic facing, navigation, pathfinding, or attack movement.

Handle coincident fighter positions deterministically rather than normalizing a zero-length vector.

## 4. Preserve the reaction and Continue system

The general Continue decision (ID 7) has already been verified in graphical PIE.

It must continue to work after the position migration.

At a reaction opportunity:

- Preserve current XY position.
- Preserve the original XY Walk target.
- Preserve phase and phase progress.
- Preserve remaining movement distance.
- Preserve the opponent's committed action and progress.
- Resume the ongoing Walk from the paused position.
- Do not restart movement or rewind time.
- Do not repeatedly react to the same observed attack.

Update `GetReactionInfo`, `IsReactableWalk`, `ResolveCombat`, and `GenerateTimeline` only as required by the 2D migration.

Do not alter established reaction windows.

## 5. Update BP_CombatManager integration

Inspect `BP_CombatManager` and all callers of `SelectWalkDestination`.

The manager must accept and retain both target coordinates.

Update:

- Walk destination selection.
- Combat-state creation.
- ExecuteMove.
- CurrentState persistence.
- Real fighter placement.
- Prediction generation.
- ResetEncounter.
- Any references to `PlayerX`, `OpponentX`, or `PlayerWalkTarget`.

The player's confirmed mouse destination must become the actual movement destination used by CombatSim.

Do not silently execute an X-only movement when the user selects a diagonal path.

Preserve the one-action-at-a-time Execute workflow.

## 6. Update predictions and ghosts

The existing hypothetical prediction system must use the same 2D simulation as authoritative execution.

Requirements:

- Cyan ghost shows the predicted player's XY position at the selected Instant.
- Orange ghost shows the predicted opponent's XY position.
- Yellow disc shows the intended Walk destination.
- The green path connects the player's actual starting ground position to the selected destination.
- Timeline scrubbing updates ghost positions correctly in both axes.
- Reaction pauses display the correct predicted position.
- Preview generation never changes authoritative fighter positions.
- Continue preserves progress after actual execution.

Update prediction text where necessary to display meaningful XY positions and traveled/remaining distance.

Do not reintroduce the previous duplicate prediction-generation blocks in WBP_CombatUI.

## 7. Preserve Stage B targeting

Do not rebuild the working mouse-targeting implementation.

Keep:

- Collision-based cursor targeting.
- The flat destination marker.
- The persistent green Walk path.
- Left-click confirmation.
- Right-click/Escape cancellation.
- Visible mouse cursor.
- Functional UMG controls.

Replace the temporary Y-only warning behavior with actual XY execution when Stage C is functional.

The visual path must agree with the actual simulated movement.

Keep using the existing green debug line until movement correctness is established. The final reusable renderer can be addressed separately.

## 8. Testing

Extend the existing tests instead of replacing them.

Test these scenarios:

**Movement**
1. Walk in positive X.
2. Walk in negative X.
3. Walk in positive Y.
4. Walk in negative Y.
5. Walk diagonally.
6. Walk a distance shorter than one movement Instant.
7. Walk toward and away from the opponent.

**Combat**
8. Quick and Heavy correctly calculate range with nonzero Y separation.
9. Dodge displaces the fighter correctly in the ground plane.
10. Repeated actions do not cause fighter overlap or position resets.

**Reactions**
11. Walk pauses on an observable attack.
12. Continue resumes a partially completed diagonal Walk.
13. The original XY target is preserved.
14. The opponent's committed attack continues.
15. The same observed threat does not trigger duplicate reactions.

**Predictions**
16. Hypothetical Dodge and Guard branches update both XY coordinates.
17. Cyan and orange ghost positions agree with stored timeline snapshots.
18. Changing the destination does not mutate authoritative state.
19. Timeline scrubbing works with nonzero Y positions.

**Gameplay**
20. Confirm, Execute, and Reset work.
21. No duplicate ghost or path actors appear.
22. The visible green line agrees with the selected and executed path.

Keep legacy X-axis tests working with Y=0, but add genuine nonzero-Y tests that would fail under the old implementation.

Run all four BUH automation suites.

## 9. Implementation discipline

Perform this in small, verifiable steps:

1. Update C++ combat data and movement calculations.
2. Run pure simulation tests.
3. Migrate the manager and Blueprint interfaces.
4. Update real fighter and prediction ghost transforms.
5. Integrate confirmed mouse destinations.
6. Run automation and PIE regression tests.

Do not attempt a broad, unverified Blueprint EventGraph rewrite.

Use targeted node edits where possible.

If a required Blueprint interface cannot be migrated safely, report the precise blocker rather than introducing a second competing combat state.

Preserve the project's GPU memory-management workflow.

Do not implement Dash, Jump, Qinggong, obstacles, or navigation in this phase.

No unrelated UI redesign or Git commits without authorization.

## Completion criteria

Stage C is complete when:

- A mouse-selected Walk destination executes correctly in both X and Y.
- Movement speed remains consistent in every direction.
- Attack ranges use actual 2D separation.
- Prediction ghosts show correct XY positions.
- The green path matches actual Walk movement.
- Reaction opportunities and Continue work during diagonal movement.
- Reset and existing combat actions still work.

Report exactly what passed in C++ tests, what was verified through Blueprint integration, and what requires manual graphical PIE verification.

**Priority: authoritative simulation correctness first, Blueprint integration second, visual polish last.**