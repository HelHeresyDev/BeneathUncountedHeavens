# Beneath Uncounted Heavens — Fix Two Confirmed Jump Bugs

## Status (HEADLESS VERIFIED)

- [x] BUG 1: Continue now passes the airborne `CurrentState` as `PreExecutionState` (surgical node edits in `ExecuteMove`).
- [x] BUG 2: `DrawJumpPath` clamps `End` to 480 uu and forces `GroundZ=90`; `GetClampedJumpDestination` UFUNCTION added.
- [x] WBP `DestPlayer` marker placed at `GetClampedJumpDestination` (preview and marker agree at any cursor distance).
- [x] C++ rebuilt clean; `BP_CombatManager` and `WBP_CombatUI` compiled and saved.
- [x] Regression tests added: `BUH.JumpContinuePlaybackAcceptanceTest`, `BUH.JumpPreviewClampTest`.
- [x] All 10 `BUH.Jump*` tests pass; all 21 `BUH.*` tests pass.
- [x] Blueprint exports updated in `tests/blueprint_exports/jump_selection_flow.md`.
- [x] `notes.md` §9.38 documents both fixes.
- [ ] Graphical PIE click-through: click Jump → destination → Execute → reaction pause → Continue → Jump resumes through landing; preview and marker clamped to 480 uu. (Requires a graphical PIE run; headless `-nullrhi` cannot click WBP buttons or capture the path/marker.)

The user tested commit 3568add in graphical PIE.

Current behavior:
- The player genuinely freezes midair at a reaction pause. WORKING.
- The Continue button is now visible. WORKING.
- Clicking Continue does not resume the airborne Jump. BROKEN.
- The Jump trajectory becomes shorter/different after Execute. BROKEN.

I reviewed the latest GitHub C++ and Blueprint export. There are two concrete implementation problems.

## BUG 1: Continue passes a grounded prestate to playback

In `BP_CombatManager.ExecuteMove`, when `PlayerMove == 7`:

- The manager preserves `CurrentState`, which correctly contains the interrupted airborne Jump.
- However, `PreExecutionState` is still constructed from fresh `MakeState(...)`, because the Jump-specific adjustment only runs for `PlayerMove == 9`.
- `ExecuteMoveWithVars(true, CurrentState, ..., 7, ...)` correctly resolves Continue from the airborne state.
- But `BeginJumpPlayback(..., PreExecutionState, PlayerMove=7, ...)` receives the fresh grounded state.

In C++, `BeginJumpPlayback()` requires either:

`PlayerDecision == 9`

OR

`PreState.Fighters[0].Action == 9 && PreState.Fighters[0].Phase == 2`

For Continue, both are FALSE when the wrong PreExecutionState is supplied. The function returns without starting playback.

### Fix

Build one correct `PreExecutionState` BEFORE resolving:

For Continue (7):
- Set PreExecutionState to the existing authoritative airborne CurrentState.
- Do not call MakeState to replace the airborne state.
- Preserve Action=9, Phase=2, PhaseInstants, Height, JumpStart, JumpTarget, and threat bookkeeping.

For a new Jump (9):
- Build or reuse the appropriate combat state.
- Apply the newly confirmed Jump target.
- Set PreExecutionState to that destination-adjusted state.

For other actions:
- Preserve existing working state-selection behavior.

Pass the SAME PreExecutionState to:
1. The authoritative combat resolver.
2. BeginJumpPlayback / GenerateTimeline.

Ensure the manager stores the resulting FinalState after resolution, not only when another reaction occurs.

Do not introduce another global flag or secondary simulation.

### Verify

At the moment Continue is clicked, log:

- PlayerMove = 7.
- PreExecutionState.Action = 9.
- PreExecutionState.Phase = 2.
- PreExecutionState.Height > 0.
- PreExecutionState.JumpStart/Target.
- BeginJumpPlayback acceptance.
- Generated playback snapshot count.

Continue must resume from the existing midair XYZ, finish the original trajectory, and land.

The same Heavy must not cause a duplicate reaction.

## BUG 2: The preview ignores Jump's 480-unit range

I found the likely cause of the changing trajectory.

`UBuhCombatSim::DrawJumpPath()` (CombatSim.cpp) draws the preview directly from Start to the raw End supplied by the cursor.

It DOES NOT clamp horizontal range.

But `PrepareJumpCommit()` clamps the committed destination to `JumpMaxRange = 480`.

Consequently:
- The preview can show a long parabola toward a destination 800–1000 uu away.
- The executed Jump clamps that destination to 480 uu.
- The airborne remaining trajectory uses the real clamped destination.
- The trajectory therefore appears to change dramatically when combat starts.

This matches the screenshots.

### Fix

Use one common deterministic Jump destination-clamping helper for:

1. Cursor targeting.
2. Yellow landing marker.
3. Confirmed Jump preview.
4. Committed destination.
5. Actual simulated movement.
6. Playback and remaining path.

Range is based on XY distance from the takeoff location, not 3D arc length.

If the cursor is farther than 480 uu, place the landing marker at the reachable point along that direction.

The visible parabola must end at that clamped marker.

Reuse the same target-clamping math already implemented by PrepareJumpCommit. Avoid creating a second version with different rounding or coordinate rules.

Do not modify the 480-uu range or Jump's eight-Instant flight duration.

### Verify

Test a cursor destination 1000 uu from the player.

Expected:
- Marker is 480 uu from the takeoff point.
- Preview ends at exactly that marker.
- Committed JumpTarget equals preview endpoint.
- Actual Jump lands at that endpoint.
- At an airborne reaction, every remaining trajectory point is a subset of the original preview curve.

Also test a destination within 480 uu. It must not be shortened.

## Regression tests

Add tests for:

- Initial Jump into a valid midair reaction.
- Continue preserving the airborne PreExecutionState.
- BeginJumpPlayback accepting Continue with an airborne state.
- Playback starting at the frozen midair XYZ.
- No extra jump apex or commitment restart.
- Landing at the original destination.
- Preview range clamped to 480 uu.
- Yellow marker matching the clamped target.
- Original preview and interrupted remaining path matching point-for-point for a long-distance cursor selection.
- Existing Walk and Dash behavior remaining unchanged.

Do not treat a standalone ResolveCombat(airborneState, 7, ...) test as sufficient. That bypasses the broken Blueprint PreExecutionState handoff.

Run all BUH automation suites.

## Delivery

Read back the actual current BP_CombatManager.ExecuteMove graph before editing.

Make targeted changes. Preserve the working frozen-time reaction, mouse cursor, and Dash behavior.

Compile and save the changed Blueprints.

Export actual saved graph readback to tests/blueprint_exports/.

Update todo.md and notes.md.

Commit and push the changes to origin/master on HelHeresyDev/BeneathUncountedHeavens so ChatGPT can inspect the results.

Report the commit SHA, tests, and outstanding graphical checks.

**Success: A Jump preview never exceeds its actual 480-uu range. When the fighter freezes midair, its remaining path matches the original preview, and Continue resumes the same Jump through landing.**