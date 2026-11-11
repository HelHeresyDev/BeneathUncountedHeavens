# Beneath Uncounted Heavens — Implement Jump Movement

## Objective

Implement Jump as the third playable movement action, after Walk and Dash.

The user has confirmed that the previous Dash targeting, confirmation, red/green path, reaction, and Continue behavior now works in graphical PIE.

Read the latest source, `AGENTS.md`, `notes.md`, and `tests/blueprint_exports/dash_selection_flow.md` before editing.

**Jump must be a real simulated 3D movement action, not merely a curved preview line.**

Do not implement Qinggong or aerial attacks in this milestone.

## 1. First-playable Jump rules

Use these provisional values:

- Action ID: 9 (verify that it remains unused).
- Startup: 1 Instant.
- Flight: 8 Instants.
- Recovery: 3 Instants.
- Maximum horizontal range: 480 uu.
- Maximum height above ground: 220 uu.
- First 3 flight Instants: reactions locked (red).
- Remaining 5 flight Instants: reactions permitted against qualifying observable threats (green).

Unlike Dash, Jump's flight duration is always 8 Instants, even for short horizontal distances.

A short Jump must still rise, reach its apex, descend, and land. Do not shorten the flight merely because the destination is nearby.

A vertical jump in place (zero horizontal distance) should also be valid.

Jump performs no automatic attack or damage.

## 2. Parabolic trajectory

The player chooses an XY ground destination through the existing mouse-targeting system.

The airborne trajectory follows:

`P(t) = StartXY + t * (TargetXY - StartXY)`

`Height(t) = 4 * MaxHeight * t * (1 - t)`

Where:
- `t` ranges from 0 to 1 over the 8 flight Instants.
- MaxHeight is 220 uu.
- At takeoff, height is zero.
- At t=0.5, height reaches 220 uu.
- At landing, height returns to zero.

This describes a true three-dimensional parabola: horizontal travel along one straight XY segment, with vertical ascent and descent.

Use horizontal distance clamping at 480 uu.

Distinguish height above the floor from the fighter mesh's existing visual Z offset. The character should land at exactly its original ground-relative height, not sink into or float above the arena.

Use one deterministic trajectory calculation for authoritative motion and previews. Avoid calculating a visually appealing curve separately from the actual simulation.

## 3. Add authoritative Jump state

Extend the existing combat state conservatively.

Jump needs sufficient authoritative information to reconstruct its position at any Instant:

- Original takeoff XY.
- Selected landing XY.
- Current XY.
- Current height above ground.
- Flight Instants completed.
- Existing action phase and reaction bookkeeping.

Reuse Phase and PhaseInstants where possible rather than maintaining another competing flight clock.

Ensure the state survives a reaction pause and Continue.

The existing FBuhFighterState already contains XY movement and Dash targeting. Extend it without breaking serialization, current encounters, or existing tests.

Update timeline snapshots to expose the player's and opponent's relevant vertical position.

If a fighter is not jumping, its height should be zero.

Do not rely on actor animation or visual interpolation as the authoritative source of height.

## 4. Simulate the complete jump

During startup, the fighter prepares without beginning horizontal flight.

During each flight Instant:

1. Advance flight progress by one Instant.
2. Calculate the XY position from the saved takeoff and landing coordinates.
3. Calculate vertical height from the parabola.
4. Update authoritative position and height.
5. Apply existing phase and reaction logic.
6. At the eighth flight Instant, land exactly on the selected destination.
7. Enter recovery.

There must be no sudden teleport to the landing point.

Handle:
- Short jumps.
- Maximum-range jumps.
- Diagonal XY jumps.
- Jumps away from the opponent.
- Jumps perpendicular to the opponent.
- Vertical jumps in place.

Preserve current grounded fighter separation behavior wherever possible. Document any provisional collision limitations rather than silently changing core combat physics.

For this first milestone, do not invent aerial invulnerability or new attack hit-detection mechanics. Keep the existing damage rules unless a narrowly scoped adjustment is essential for a functional airborne state; document that limitation for later design work.

## 5. Jump reaction rules

Jump uses the same general reaction system as Walk and Dash.

During the first three active flight Instants, the fighter cannot voluntarily interrupt Jump through a threat-triggered reaction.

Beginning with the fourth flight Instant, a qualifying observable opponent threat can cause a reaction opportunity.

The red/green coloring must match those actual rules.

At a reaction pause:
- The fighter remains at the correct airborne XY and Z position.
- The original takeoff and landing coordinates remain saved.
- The elapsed flight progress remains saved.
- Continue is legal only where the existing reaction rules permit it.

Selecting Continue resumes the same parabola from the saved progress.

It must not:
- Restart Jump at takeoff.
- Restart the three locked Instants.
- Give an additional eight flight Instants.
- Teleport to the landing point.
- Change the original landing destination.

A replacement action selected at a legal reaction opportunity follows the existing interruption behavior. Do not invent a complete aerial maneuver system in this task.

## 6. Render the actual 3D arc

Reuse ABUHPathRenderer.

It already accepts world-space sample points and red/green path segments.

Create a Jump-specific path-building helper that samples the authoritative trajectory at flight-Instant boundaries.

For a new Jump:
- The complete path is a visible parabola.
- The initial locked portion is red.
- The later reactable portion is green.
- The red/green boundary occurs after the first 3 flight Instants.
- The line ends at the yellow landing marker.

Do not use a straight green ground line for Jump.

Do not color based on arbitrary percentages of rendered length. Color by flight progress and real reaction eligibility.

During a legal reaction pause:
- Render only the remaining portion of the original parabola.
- Begin at the fighter's current airborne location.
- Preserve the original apex and landing trajectory.
- Use the saved elapsed Instants to determine remaining red/green sections.

IMPORTANT: Do not generate a fresh 8-Instant parabola starting from the fighter's midair location. That would create a second jump and a false second apex.

## 7. Add Jump to the Move submenu

Use the existing hierarchy:

Move
- Walk
- Dash
- Jump

Place Jump alongside Walk and Dash, not below Execute.

Use the established UMG event-binding tool:

`UMGToolSet.UMGToolSet.BindToEventProperty`

when needed to create the new button's OnClicked event automatically.

Do not require manual Blueprint wiring.

Reuse the existing ground cursor targeting system:
- Mouse moves the proposed landing point.
- Left click confirms.
- Right click/Escape cancels.
- Cursor remains visible.
- Yellow marker shows landing destination.
- The path follows the prospective landing point while targeting.

Keep the confirmed movement path visible until Execute.

Do not accidentally confirm a ground location from the same click that selected Jump.

## 8. Preserve the fixed Dash lifecycle

This is a critical regression risk.

The latest Dash fix distinguishes:
1. Active targeting.
2. Confirmed movement awaiting Execute.
3. Existing movement awaiting Continue.

Extend that architecture to Jump.

Avoid adding another global static variable to distinguish selected versus committed movement.

Prefer using the existing BP_CombatManager instance and shared pending-action state.

A newly selected Jump must use fresh elapsed progress zero, even if it replaces an already-running Jump.

After confirmation, its red opening section must stay red until Execute.

At an interrupted existing Jump, the remaining green reactable section must remain green when Continue is chosen.

Canceling a replacement must restore the existing action's continuation state.

Path rendering should have one effective owner per UI update.

Do not reintroduce the repeated Dash problem where an old continuation renderer overwrites the new movement preview.

## 9. Update fighters and predictions

The real fighter must visibly leave the ground, rise, descend, and land.

Update actor placement using authoritative XY plus height above ground, with the correct existing mesh Z offset.

The cyan predicted-player ghost should appear at the actual predicted airborne position for the chosen timeline Instant.

If an opponent Jump is represented in a timeline, its ghost must also use the correct height.

Do not leave ghost positions fixed to Z=90 during Jump.

The hypothetical simulation and actual execution must use the same trajectory calculations.

The user should be able to inspect the predicted position at Instants near:
- Takeoff.
- Early ascent.
- Apex.
- Descent.
- Landing.

Preserve opponent hidden-action separation.

## 10. Automated regression tests

Add Jump-specific tests covering at least:

- Correct parabola: zero height at takeoff and landing.
- Exact 220 uu apex at the midpoint.
- Correct XY target after 8 flight Instants.
- Short jump still lasts the full flight.
- Maximum range clamps to 480 uu.
- Diagonal distance calculation.
- Zero-distance vertical jump.
- No unwanted horizontal movement toward the opponent.
- No reaction during the first 3 locked flight Instants.
- Legal reaction after the lock window.
- Continue preserves progress and height.
- Continue does not create another apex or restart the lock.
- Initial Jump preview has the correct red/green boundary.
- Midair reaction displays the correct remaining arc.
- Replacement Jump retains its fresh red section after confirmation.
- Cancel restores the existing movement continuation.
- Timeline ghost Z matches the simulated snapshot.
- Landing restores exact ground-relative height.
- Reset removes the curve and restores grounded fighters.

Run all existing BUH suites, especially the Dash lifecycle regression tests.

Do not replace existing Dash tests with Jump tests.

## 11. Graphical PIE verification

Verify in the actual graphical game view:

1. Start a clean encounter.
2. Open Move → Jump.
3. Select a landing point perpendicular to the opponent.
4. Observe a parabolic red/green preview.
5. Move the targeting cursor and verify the arc follows the target.
6. Confirm the destination and verify that the new red portion remains.
7. Execute Jump.
8. Confirm the blue fighter rises and follows the selected XY trajectory.
9. Verify it lands exactly at the selected destination.
10. Repeat with a very short Jump.
11. Trigger a qualifying reaction in the reactable flight portion.
12. Confirm that the remaining arc is green where reactions are permitted.
13. Select Continue and verify that the original parabola resumes.
14. Select a replacement movement action and confirm there is no stale curve.
15. Reset and verify all path geometry clears.

Headless automation cannot prove that the actual 3D arc or UMG interaction looks correct. Clearly distinguish automated verification from graphical PIE confirmation.

## 12. Documentation and GitHub

Read `AGENTS.md` before modifying assets, particularly the warnings about `write_graph_dsl` rewriting EventTick.

Prefer targeted node edits.

Export important saved Blueprint graph changes using actual MCP readback into:

`tests/blueprint_exports/jump_selection_flow.md`

Record the Jump movement rules and intentional prototype limitations in `designbible.md`, update `notes.md` with technical findings, and update `todo.md` with completion status.

After completing the implementation and verifying all available automated checks, **commit and push changes to `origin/master` under `HelHeresyDev/BeneathUncountedHeavens`** so ChatGPT can inspect them.

Do not claim manual graphical PIE success if it has not been performed. Report any remaining graphical checks separately.

Report the commit SHA and automation results.

## Scope

Implement Jump fully enough to playtest its movement, reactions, visualization, and Continue behavior.

Preserve Walk, Dash, Quick, Heavy, Guard, Dodge, cursor behavior, and hypothetical predictions.

No Qinggong, double jumps, stamina, wall jumping, aerial attacks, or broader combat-system refactoring.

**Success criterion:** The player can select a ground destination, preview a genuine parabolic Jump with accurate red/green reaction sections, execute the jump with visible airborne movement, react at legal Instants, and Continue along the same saved trajectory without restarting the action.

## Completion Status

- **C++ Jump simulation:** Complete. `FBuhFighterState` extended, `MoveData[9]` added, parabolic movement, snapshots, ghosts, `DrawJumpPath`, and `GetJumpElapsedFlightInstants` implemented.
- **Jump reaction rules:** Complete. First 3 flight Instants locked, remaining 5 reactable, Continue preserves progress and height.
- **Jump path rendering:** Complete. `DrawJumpPath` samples the authoritative parabola at flight-Instant boundaries and colors by reaction eligibility, not rendered length.
- **Blueprint lifecycle:** Complete. `JumpAwaitingExecute`, `BeginJumpTargeting`, `SelectJumpDestination`, `ClearJumpAwaitingExecute`, `IsJumpAwaitingExecute` added to `BP_CombatManager`. `ExecuteMove` and `ResetEncounter` clear the flag.
- **UMG Move submenu:** Complete. `BtnJumpMove` and `TxtJumpMove` added, `OnClicked(BtnJumpMove)` bound via `UMGToolSet.BindToEventProperty`, visibility mirrored in all relevant events.
- **EventTick Jump branches:** Complete. Targeting preview, confirmation, cancel, and confirmed-Jump execution with `SelectInt(0, GetJumpElapsedFlightInstants, IsJumpAwaitingExecute)`.
- **Automated tests:** Complete. `BUH.JumpSimulationTests` and `BUH.JumpPathColorTests` added. All 12 `BUH.*` suites pass, including the updated `BUH.DashGateRegressionTest`.
- **Documentation:** Complete. `designbible.md` §30.1.1 records the Jump rules and prototype limitations. `notes.md` §9.29 records the technical findings. `tests/blueprint_exports/jump_selection_flow.md` exports the actual Blueprint readback.
- **Graphical PIE verification:** Pending. Headless `-nullrhi` PIE cannot click UMG buttons or capture the 3D arc. The user must perform the §11 graphical checks in a fresh graphical PIE session.
- **Commit and push:** Complete. Committed as `22b9baa` and pushed to `origin/master` under `HelHeresyDev/BeneathUncountedHeavens`.