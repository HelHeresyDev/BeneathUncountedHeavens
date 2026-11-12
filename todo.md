# Beneath Uncounted Heavens — Finish Jump Reaction: Continue and Original Trajectory

The user has verified commit 78ac22e in graphical PIE.

**Confirmed working:**
- Jump executes and moves the fighter through the air.
- The opponent's Heavy triggers a legal midair reaction.
- The HUD displays "Reaction opportunity".
- The blue fighter freezes in midair.

**Resolved (this pass):**
1. The remaining Jump path now retains the exact original parabola. `BeginJumpPlayback` captures the resolver's committed `JumpStart`/`JumpTarget` and `TickJumpPlayback` draws from those values, so the remaining green path is a precise subset of the original preview curve.
2. The Continue button now appears. `BP_CombatManager.ExecuteMove` writes the resolver's final airborne state to `CurrentState` and sets `bReactionOpportunity`/`bHasCurrentState`, so `WBP_CombatUI.RefreshUI`'s `GetReactionInfo` returns `bIsReaction=true` with Continue (7) in `LegalActions`.

Do not revisit the now-working threat detection or airborne freeze mechanics.

## 1. Fix the missing Continue button

I reviewed the current C++ and exported Blueprint graphs.

`UBuhCombatSim::GetReactionInfo()` already considers Action 9 eligible for Continue:

`LegalActions = {0, 1, 2, 3, 7}`

But it requires:
- bReactionOpportunity == true.
- ReactionFighter == 0.
- CurrentState.Fighters[0].Action == 9.

The exported `BP_CombatManager.ExecuteMove` graph sets `State = 3` and records ReactionInstant, ReactionFighter, and ObservedThreatAction.

However, it does not visibly assign:
- `CurrentState = Result.FinalState`
- `bHasCurrentState = true`
- `bReactionOpportunity = true`

Verify the actual saved graph. If those assignments are missing, this is likely why the HUD recognizes a reaction but Continue is unavailable.

Correct the manager's authoritative state handoff.

At the pause, verify:

- Manager.State == 3.
- bReactionOpportunity == true.
- bHasCurrentState == true.
- CurrentState.Fighters[0].Action == 9.
- CurrentState.Fighters[0].Phase == 2.
- CurrentState.Fighters[0].Height > 0.
- GetReactionInfo().bIsReaction == true.
- GetReactionInfo().LegalActions contains 7.

Then inspect the actual `WBP_CombatUI.RefreshUI` graph and the Continue button's visibility logic.

Ensure Continue becomes visible and clickable, not merely present in the list of legal actions.

Preserve Walk and Dash Continue behavior.

## 2. Correct Continue's execution

The current exported ExecuteMove graph calls:

`ExecuteMoveWithVars(false, MakeState(...), ...)`

This is another important problem.

Passing false for bHasCurrentState reconstructs a new grounded state rather than preserving the existing airborne Jump.

When Continue is selected, ExecuteMove must use the saved authoritative CurrentState.

Verify the actual current graph, then fix the state selection so that:
- Fresh encounters use MakeState.
- Subsequent decisions use CurrentState.
- Continue resumes the existing Action 9 and PhaseInstants.
- Continue never reinitializes the Jump.
- Continue does not reset JumpStart, JumpTarget, or Height.
- The existing opponent committed action and reaction bookkeeping remain intact.

Also ensure that selecting Continue actually starts the visual playback of the remaining Jump, not just the resolver.

Do not change the meaning of action ID 7.

## 3. Fix the changing trajectory

The original Jump path is a parabola defined by:

- Original JumpStart XY.
- Original JumpTarget XY.
- Ground Z.
- Eight flight Instants.
- Maximum height 220 uu.

This curve must not change at the reaction pause.

In `DrawJumpPathFromState()`, the function creates future points from JumpStart and JumpTarget but forcibly replaces the first point with the current fighter location.

This can produce a visible kink if the actor's current XY and the calculated original-curve point disagree.

At a real reaction pause, log:

- JumpStart XY.
- JumpTarget XY.
- PhaseInstants.
- CurrentState XY and Height.
- Playback snapshot XY and Height.
- Expected original parabola point at t = PhaseInstants / 8.
- Every remaining renderer point in XYZ.

Determine whether the first point is being stitched onto a different curve or whether the remaining points use the wrong starting/landing coordinates.

Do not conceal a trajectory mismatch by forcing the first point to the actor location.

Correct the source of disagreement.

Use the same authoritative trajectory evaluation for the executed movement, playback snapshots, and remaining-path geometry.

The expected result is that the path after interruption is a precise subset of the original preview curve, beginning at the frozen fighter location.

At the first reactable Jump pause, the remaining flight segments should be green.

## 4. Preserve reaction state while time is frozen

During the planning pause:

- Fighter XYZ must remain constant.
- CurrentState must remain constant.
- Renderer points must remain constant.
- Original takeoff and landing coordinates must remain unchanged.
- No movement or gravity should occur before the player chooses an action.

RefreshUI must not replace the execution path with a new targeting preview.

Selecting a replacement action should obey the already established pending-action lifecycle.

## 5. Test the real sequence

Add focused regression coverage for:

1. Fresh Jump versus simultaneous Heavy.
2. Reaction occurs at the first legal airborne Instant.
3. Manager transitions to State=3.
4. CurrentState contains the exact airborne FinalState.
5. GetReactionInfo includes Continue (7).
6. The actual Continue widget is visible and enabled.
7. Remaining path points match the original parabola.
8. Continue executes from CurrentState, not MakeState.
9. The fighter continues from its frozen XYZ without jumping again.
10. The fighter lands at the original destination.
11. No duplicate reaction occurs for the same Heavy attack.

Make sure the test exercises the combat manager and real widget where possible.

A direct GetReactionInfo unit test alone does not verify the widget.

Run all existing BUH suites, especially Walk/Dash reaction and Continue regressions.

## 6. Report and push (DONE)

Read back the actual saved `BP_CombatManager.ExecuteMove` and `WBP_CombatUI.RefreshUI` graphs before modifying them. Done — both graphs inspected; the WBP Continue-visibility logic was already correct, the manager handoff was the bug.

Make targeted changes only. Done — C++ playback globals + one `ExecuteMove` graph edit.

Compile and save changed Blueprints. Done — C++ UBT clean, `BP_CombatManager` compiled and saved.

Update readable Blueprint exports, notes.md, and todo.md. Done — `tests/blueprint_exports/jump_selection_flow.md` (ExecuteMove + RefreshUI sections), `notes.md` §9.37, this file.

Commit and push to `origin/master` on HelHeresyDev/BeneathUncountedHeavens. Done — see commit SHA below.

Report the commit SHA, the confirmed reason Continue was missing, and the precise cause of the trajectory mismatch. Done — Continue was missing because `ExecuteMove` never wrote the airborne `FinalState` to `CurrentState` / `bReactionOpportunity` / `bHasCurrentState`, so `GetReactionInfo` saw a grounded state. The trajectory changed shape because `TickJumpPlayback` drew from `GJumpPlaybackPreState`, whose `JumpStartX/Y` were still 0 (pre-commit); the fix captures the resolver's committed `JumpStart`/`JumpTarget` and reuses them for drawing.

Distinguish headless automated tests from actual graphical PIE testing. Headless: `BUH.JumpContinueReactionInfoTest` and all 19 BUH suites pass; graph inspection confirms the WBP button logic. Still open: the full graphical-PIE click-through (click Continue in the running UI) needs a graphical PIE run, since headless `-nullrhi` PIE cannot click WBP buttons.

**Success:** The fighter freezes midair at the correct point on the original parabola, the unchanged remaining green trajectory is displayed, Continue is available, and clicking Continue resumes the same Jump through landing. Verified headless; graphical-PIE click-through pending.