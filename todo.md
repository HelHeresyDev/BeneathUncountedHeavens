# Beneath Uncounted Heavens — Fix Airborne Jump Movement and Interrupted Path

The user tested the latest pushed Jump implementation (`48dd02b`) in graphical PIE.

## Confirmed failures

1. The blue fighter moves horizontally after executing Jump but never visibly leaves the ground.
2. When Jump is interrupted, the remaining parabolic path does not reliably persist.
3. The reaction screen displays Continue, but the fighter appears grounded even when Jump should still be airborne.

The Jump targeting preview already renders a parabola. Do not redesign the targeting system.

I inspected the current GitHub source and Blueprint export and found specific problems.

## 1. Fix the real fighter's Z coordinate

In the exported `BP_CombatManager.ExecuteMove`, the real fighter is positioned using:

`MakeVector(FinalPlayerX, FinalPlayerY, 90)`

This ignores `FBuhFighterState.Height`.

The simulation already calculates Jump height, but the real fighter never receives that vertical position.

Fix the actor transform application so that:

`WorldZ = GroundZ + AuthoritativeHeight`

Use the proper height from the resolved authoritative state.

The opponent should also use authoritative height when appropriate.

Verify:
- Grounded fighter: Height=0.
- During Jump: Height is positive.
- At Jump apex: Height reaches 220 uu.
- At landing: Height returns to zero.
- At a midair reaction pause: fighter remains visibly airborne.

Avoid adding another independent height calculation to the Blueprint.

## 2. Make the Jump visibly travel through the air

Important: `ResolveCombat` currently processes multiple Instants synchronously.

Simply applying FinalState after execution may only display the final landing position. That would still make Jump look like a teleport.

Implement a minimal, deterministic visual playback of the actual resolved movement Instants.

Prefer recording authoritative per-Instant snapshots as combat resolves.

During playback:
- Show the real fighter moving along the saved XY trajectory.
- Apply saved Jump height at each Instant.
- Display the ascent, apex, and descent.
- Stop playback at the actual decision or reaction boundary.
- At a reaction pause, leave the fighter at that authoritative position.
- At completed landing, restore exact ground-relative height.

This is visual playback, not another combat simulation.

Keep authoritative combat outcomes unchanged.

Do not introduce physics-based jumping, animation montages, skeletal animation, or cinematic camera work.

Make the simplest playback implementation that reliably shows the cylinder following its simulated parabola.

Keep the existing one-action-at-a-time decision system. Do not allow a new decision during unresolved visual playback.

## 3. Reconstruct the interrupted parabola correctly

There is a second specific code issue in `UBuhCombatSim::DrawJumpPath()`.

The function currently takes the current actor location as Start, resets its Z to 90, and interpolates toward End using the original flight-time fractions.

This is incorrect for an interrupted Jump.

An interrupted Jump must follow its ORIGINAL parabola, not a new parabola between the current position and landing point.

Use authoritative state:

- JumpStartX/Y.
- JumpTargetX/Y.
- Flight Instants completed.
- Current fighter XY.
- Current Height.
- Original flight duration of 8 Instants.

At an interruption, construct the remaining points using the original trajectory for `t = elapsed/8` through `t = 1`.

The first point must coincide with the actual airborne fighter location.

Do not reset the parabola's origin to the current fighter location.

The remaining trajectory should not produce a second apex.

## 4. Preserve the path during reaction

Inspect the exact execution sequence in `BP_CombatManager.ExecuteMove`.

The current exported graph calls `CombatSim.ClearWalkPath` after resolving combat.

Check whether that clearing call removes the Jump path after the combat state has entered Reaction.

At a legitimate Jump reaction opportunity:
- Preserve or regenerate the remaining Jump path.
- Use CurrentState rather than the pending landing marker.
- Render green for the remaining reactable flight Instants.
- Keep the fighter at its authoritative airborne position.
- Leave the original landing destination unchanged.

The path should remain visible until the player selects Continue or a replacement action.

When Continue is selected:
- Resume the existing Jump trajectory.
- Preserve elapsed flight progress.
- Do not restart the three locked Instants.
- Do not generate a second arc.
- Do not teleport to the landing point.

When the Jump finishes, clear the path.

When a replacement action is selected, clear the abandoned Jump path and show the replacement's proper preview.

Preserve the previously fixed Dash lifecycle.

## 5. Diagnose with actual runtime values

At the first legitimate Jump interruption, log:

- Player action.
- Player phase.
- PhaseInstants.
- JumpStart XY.
- JumpTarget XY.
- Current fighter XY.
- Current authoritative Height.
- Real actor world XYZ.
- Remaining flight Instants.
- Renderer path visibility and segment count.
- Last caller that cleared or updated the renderer.

Do not assume a simulation unit test proves the real fighter is airborne.

If Height is positive but ActorZ remains 90, fix actor transform application.

If Height is zero during an expected midair interruption, inspect the resolver and phase progression.

If CurrentState is correct but the path disappears, inspect renderer clearing and redraw ownership.

## 6. Tests

Add focused integration tests verifying:

1. Jump's authoritative Height is greater than zero during flight.
2. Real actor Z reflects Height at a midair interruption.
3. Flight playback visits ascent, apex, descent, and landing.
4. Horizontal movement follows the chosen XY direction.
5. An interrupted Jump retains its exact original takeoff and landing coordinates.
6. Remaining path begins at the actual midair position.
7. Remaining path does not create a second apex.
8. Remaining reactable segments are green.
9. Continue preserves flight progress.
10. A completed Jump clears the path.
11. Walk and Dash remain unchanged.

Run all existing BUH automation suites.

## 7. Correct documentation

The current `todo.md` opening incorrectly describes Jump as free, instantaneous, and uninterruptible, contradicting the actual implemented rules.

Restore the correct rules:
- Startup: 1 Instant.
- Flight: 8 Instants.
- Recovery: 3 Instants.
- First 3 flight Instants locked.
- Remaining 5 flight Instants reactable.

Keep design documentation consistent with the implemented combat rules.

## 8. Commit and push

Compile and save modified Blueprints.

Update the actual saved Blueprint graph export under `tests/blueprint_exports/`.

Update `notes.md` and `todo.md`.

After automated verification, commit and push to `origin/master` on `HelHeresyDev/BeneathUncountedHeavens`.

Report the commit SHA, tests, and any remaining graphical PIE checks.

Do not implement Qinggong or aerial combat during this fix.

**Success criteria:** Executing Jump visibly lifts and moves the blue cylinder through its parabola. An interruption freezes it in midair and preserves the correct remaining green trajectory. Continue finishes the same Jump, and landing restores the normal ground height.

## Completion status (2026-11-10)

Jump rules as implemented:
- Startup: 1 Instant.
- Flight: 8 Instants.
- Recovery: 3 Instants.
- First 3 flight Instants locked.
- Remaining 5 flight Instants reactable.

Completed:
- `ExecuteMove` now uses `PreExecutionState`, `WithPlayerJumpTargetFromPreview`, and `ExecuteMoveWithVars`; it skips the player teleport for Jump/airborne states and uses `GetFinalOpponentWorldZ` for the opponent.
- `BeginJumpPlayback` / `TickJumpPlayback` / `StopJumpPlayback` drive a deterministic per-Instant playback that applies `WorldZ = 90 + Height` every tick and redraws the remaining parabola from the original `JumpStartX/Y` and `JumpTarget/JumpTargetY`.
- `GenerateTimeline` reaction-pause and KO snapshots now carry `PlayerHeight` / `OpponentHeight`.
- `TickJumpPlayback` builds a corrected `DrawState` so the interrupted parabola is reconstructed instead of cleared.
- `BUH.JumpPlaybackIntegrationTests` covers completed Jump, airborne interrupted Jump, Continue from airborne, after-apex path reconstruction, and non-Jump path clearing.
- `BP_CombatManager` and `WBP_CombatUI` compiled clean and saved; `WBP_CombatUI.EventTick` calls `TickJumpPlayback` every tick.
- All 13 `BUH.*` automation suites pass via `AutomationTestToolset.RunTests` with the explicit test names.
- `tests/blueprint_exports/jump_selection_flow.md`, `notes.md` §9.31, and this `todo.md` updated with the verified evidence.

Remaining graphical-PIE checks (user-driven, headless `-nullrhi` cannot click UMG buttons or capture the path):
- The blue cylinder visibly lifts and follows the parabola in a real rendered viewport.
- The mouse cursor remains visible during Jump targeting.