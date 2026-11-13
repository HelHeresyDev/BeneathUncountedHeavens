# BUH — Fix Continue Playback: Two Execution-Order Bugs

**Status: RESOLVED (headless-verified).** Both execution-order bugs are fixed in `BP_CombatManager.ExecuteMove` (Continue branch now sets `PreExecutionState` to the airborne `CurrentState` before the resolver; the resolver and both playback gates consume `PreExecutionState`, not the post-resolution `CurrentState`). The yellow "BUH DASH v3" banner is removed from `WBP_CombatUI`. A new C++ regression test `BUH.JumpContinueExecuteMoveRegressionTest` exercises the actual state-selection and playback-gating logic. All 22 `BUH.*` automation tests pass. See `notes.md` §9.39. The remaining manual check is the graphical-PIE button click-through (headless `-nullrhi` cannot click WBP buttons).

The user tested commit `109d310` in graphical PIE.

**Working:**
- Jump distance is now correctly restricted to 480 uu.
- Jump executes.
- Heavy triggers a midair reaction.
- Fighter freezes in midair.
- Continue button appears.

**Still broken:** Selecting Continue and clicking Execute returns the HUD to "Select a move / Ready", but the blue fighter remains suspended. The remaining Jump playback never visibly finishes.

I inspected the latest saved `BP_CombatManager.ExecuteMove` graph exported to `tests/blueprint_exports/jump_selection_flow.md`.

There are TWO concrete execution-order problems.

## Bug 1 — PreExecutionState is read before it is prepared

The graph begins:

`(bind _preexecutionstate (GetPreExecutionState))`

Later, for Continue:

`SetPreExecutionState(GetCurrentState)`

But both `ExecuteMoveWithVars` and `BeginJumpPlayback` receive the earlier `_preexecutionstate` binding.

The latest commit moved the correct airborne state into the manager variable but did not ensure that the value consumed by the resolver/playback is the newly selected state.

**Fix:**

Establish the pre-execution state in this order:

1. Determine PlayerMove.
2. For Continue (7), select the saved airborne CurrentState.
3. For a fresh Jump (9), select the correctly prepared state with the confirmed Jump target.
4. For other actions, retain the appropriate existing combat state.
5. Only AFTER this selection, capture the definitive PreExecutionState.
6. Pass that SAME captured state to both ResolveCombat/ExecuteMoveWithVars and BeginJumpPlayback.

Do not use a prestate snapshot taken before the Continue branch.

Avoid extra global variables or another combat state machine.

## Bug 2 — Playback eligibility checks POST-resolution state

At the end of ExecuteMove, the graph currently checks:

`PlayerMove == 9 OR GetJumpElapsedFlightInstants(CurrentState) > 0`

But `CurrentState` has already been overwritten with `Result.FinalState`.

For Continue:
- PlayerMove == 7, not 9.
- When Continue finishes the Jump, FinalState is grounded.
- GetJumpElapsedFlightInstants(FinalState) returns zero.
- BeginJumpPlayback is therefore never called.

The authoritative resolver may finish the Jump correctly, but the real fighter is left suspended at the previous visual position.

**Fix:**

Determine whether Jump playback is required using the PRE-EXECUTION state and selected action.

Examples:

- New Jump: PlayerMove == 9.
- Continue an airborne Jump: PlayerMove == 7 AND PreExecutionState.Fighters[0].Action == 9 AND Phase == Active.

Capture that decision before resolution, or derive it afterward only from the preserved pre-execution state.

Do not use Result.FinalState or the newly assigned CurrentState to determine whether the move WAS a Jump.

Playback must start even when the authoritative result has already landed. That is precisely when visual playback is needed to animate the remaining movement.

## Required execution sequence

For Continue from a midair reaction:

1. Capture the authoritative frozen airborne CurrentState.
2. Preserve its JumpStart, JumpTarget, PhaseInstants, XY, Height, and opponent commitment.
3. Confirm that the chosen action is Continue (7).
4. Record that Jump playback is required.
5. Resolve Continue from that frozen state.
6. Store the resulting FinalState as authoritative.
7. BeginJumpPlayback with the original frozen pre-execution state and the resolution result.
8. Play the remaining snapshots from frozen XYZ through landing.
9. On playback completion, ensure the real actor is at the authoritative final XYZ.
10. Clear the remaining Jump path and return to normal planning.

Do not teleport to the final position before playback.

Do not restart the full parabola.

Do not start a new reaction for the same observed Heavy.

## Regression tests

The current C++ playback acceptance test is insufficient because it directly passes a valid airborne prestate to BeginJumpPlayback.

Add a test that checks the actual BP_CombatManager ExecuteMove state-selection and playback-gating logic.

Verify:

- PreExecutionState is the frozen airborne state when PlayerMove=7.
- The resolver receives that exact state.
- Playback receives that exact same prestate.
- Playback is requested based on the PRE-resolution Jump action.
- Playback is still requested when FinalState is grounded.
- BeginJumpPlayback accepts PlayerDecision=7 with PreState.Action=9 and Phase=2.
- The fighter visually advances from its frozen midair location to the original landing destination.
- The final real actor XYZ matches FinalState.
- No duplicate reaction is generated.
- Walk and Dash Continue remain functional.

Log the exact prestate Action/Phase/Height, poststate Action/Phase/Height, playback-gate result, and BeginJumpPlayback acceptance.

Ensure the test would FAIL with the currently exported graph.

## Blueprint verification

Read the actual saved ExecuteMove graph through MCP before editing.

Make targeted wiring changes.

Pay special attention to evaluation order and Blueprint pins. Do not assume that setting a manager variable later updates an earlier cached/bound value.

After editing:
- Compile and save.
- Read back the graph again.
- Verify the definitive prestate is selected before being consumed.
- Verify the playback gate uses that prestate, not the resolved final state.
- Run all BUH automation suites.

Do not change Jump range, parabola, reaction detection, or the working airborne freeze.

Remove the old yellow "BUH DASH v3" diagnostic banner once this is verified.

## GitHub

Update notes.md, todo.md, and the actual Blueprint export.

Commit and push to `origin/master`.

Report the SHA, the exact corrected execution wiring, and the test results.

**Success:** Clicking Continue at a midair Jump reaction visibly finishes the remaining parabola, lands the blue fighter at the original destination, and returns to normal planning without another jump or a teleport.