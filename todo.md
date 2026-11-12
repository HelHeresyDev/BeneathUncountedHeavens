# Beneath Uncounted Heavens — Fix Missed Reaction During Jump's Locked Instants

## Status — RESOLVED and verified (headless)

The missed-reaction bug is fixed in `CheckReactionPause` (see `notes.md` §9.36). A committed attack that is still in Phase 1 startup when the Jump becomes reactable now triggers the pause; the defender's `bCommitted` flag prevents stale pauses on already-moving fighters. The fresh-encounter regression `BUH.JumpFreshEncounterReactionTest` passes (reaction at instant 4, height 206.25, `StoppedReason==5`). All 16 BUH automation suites pass (passed=16, failed=0) and `CombatTestReport.txt` shows 69 passed / 0 failed. Remaining: the full UI-driven graphical-PIE click-through (click Jump → destination → Execute → Reaction → Continue) still needs a graphical PIE run, because headless `-nullrhi` PIE cannot click WBP buttons.

The user tested the latest commit `dd81464` in graphical PIE. Jump moves through the air but does not pause midair as expected.

The screenshots show the HUD remaining at "Select a move / Ready", not "Reaction opportunity".

I inspected `CombatSim.cpp` and found a likely root cause in `CheckReactionPause()`.

## 1. The specific bug

`CheckReactionPause()` contains:

`if (AttackStart[Threat] != Threat) continue;`

It only detects a threat on the Instant the attack begins.

But Jump's first three flight Instants are reaction-locked.

In an ordinary encounter:
- Player commits Jump.
- Opponent commits Heavy simultaneously.
- The opponent starts Heavy while Jump is still reaction-locked.
- CheckReactionPause skips the threat.
- When Jump becomes reactable, the opponent's Heavy is still preparing, but AttackStart is no longer set.
- The reaction opportunity is lost.
- Jump continues to landing, and the manager returns to Planning.

This explains why the specially constructed `BUH.JumpGroundedReactionTest` passes but the normal user experience fails.

**Confirm this with a test reproducing a fresh encounter, not an opponent artificially placed in recovery.**

## 2. Required reaction behavior

An opponent's attack that becomes observable during the locked portion of Jump must remain eligible for a reaction when Jump becomes reactable, provided the attack is still an active, observable threat.

For the current prototype:

- The first three active Jump Instants are locked.
- From the fourth active Instant onward, a qualifying observable threat can trigger a pause.
- If Heavy began during the lock and is still preparing when the lock ends, trigger a reaction at the first legal opportunity.
- If the threat has already ended, do not trigger a stale reaction.
- If the opponent is idle or no qualifying threat exists, Jump should complete normally and land.
- A fighter must not receive multiple reaction pauses for the same attack event.
- A genuinely new opponent attack can create a new reaction opportunity.

Do not expose uncommitted or hidden opponent decisions.

Reuse the existing reaction-bookkeeping mechanism where practical, but distinguish a newly observed threat from an already reacted-to threat.

This should eventually work for Dash as well, but preserve its verified behavior during this repair.

## 3. Regression test — reproduce actual gameplay

Create a test using the normal initial combat state:

- Player at X=500, Y=0.
- Opponent at X=-500, Y=0.
- Both grounded and ready.
- Player chooses Jump (9), targeting a valid displaced XY destination.
- Opponent chooses Heavy (1) on the same decision.
- Resolve normally with reaction pauses enabled.

The test must demonstrate that the OLD code misses the reaction.

After the fix, verify:

- Heavy becomes observable while Jump is locked.
- No reaction occurs during the locked Instants.
- At the first legal reactable Instant, Heavy remains a qualifying threat.
- `Result.bReactionOpportunity == true`.
- `Result.StoppedReason == 5`.
- `Result.ReactionFighter == 0`.
- Player Action == 9.
- Player is still in the active flight phase.
- Player Height is positive (expected approximately 206.25 uu at the three-flight-Instant boundary).
- Original Jump start and landing coordinates are preserved.

Then verify that the actual timeline also stops at the same reaction state.

## 4. Verify manager reaction and Continue

Use the actual BP_CombatManager execution path.

After ResolveCombat reports the reaction:
- Manager.State must become Reaction.
- WBP_CombatUI must show the reaction controls, including Continue.
- Fighter should be at WorldZ = 90 + authoritative Height.
- The remaining Jump parabola must persist.
- It must not move while waiting for input.

On Continue:
- Reuse the stored CurrentState, not a new grounded MakeState.
- Preserve original Jump flight progress and destination.
- Do not restart Jump or its locked Instants.
- Do not repeat the same reaction.
- Complete the remaining trajectory and land normally.

IMPORTANT: Inspect the current ExecuteMove graph carefully. Its exported `ExecuteMoveWithVars` call contains a literal `false` for `bHasCurrentState`. Verify that this does not cause Continue to discard the saved airborne state.

Fix the state handoff if necessary, using the existing authoritative state. Do not create a parallel Jump state machine.

## 5. Distinguish actual reaction from ordinary landing

Record these values after the user executes Jump:

`StoppedReason, bReactionOpportunity, ReactionInstant, CurrentState.Action, CurrentState.Phase, CurrentState.Height, Manager.State`

If no reaction was generated, the fighter landing is expected. Fix the missing reaction instead of modifying actor gravity.

If a reaction WAS generated and the actor nevertheless falls, identify which actor-transform writer caused that separately.

Do not confuse the special `BUH.TriggerReactionJump` diagnostic with a real UI-driven Jump.

## 6. Verification and GitHub

Keep the existing movement parameters and parabolic trajectory.

Run all BUH automation suites, including the new fresh-encounter simultaneous Jump-versus-Heavy regression.

Compile, save, and read back the actual Blueprints.

Update `tests/blueprint_exports/`, `notes.md`, and `todo.md`.

Commit and push to `origin/master`, reporting the SHA and test results.

Do not claim graphical success until the actual game shows Reaction/Continue while the blue fighter remains suspended.

**Success: Starting Jump against an opponent Heavy that begins during Jump's locked Instants creates a reaction at the first legal opportunity, freezes the fighter midair, and allows Continue to finish the same Jump.**