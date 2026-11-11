# Beneath Uncounted Heavens — Fix Jump Falling to Ground at Reaction Pause

The user tested the latest implementation (`c31d7f7`) in graphical PIE.

**The fighter now visibly jumps through the air. That part works.**

But when combat pauses for an interruption/reaction opportunity, the fighter falls or snaps back to the ground instead of freezing at its current airborne position.

This violates the core frozen-time combat mechanic.

## Current debugging state (as of this update)

### What is verified working
- C++ `BeginJumpPlayback` / `TickJumpPlayback` / `StopJumpPlayback` are correct. A tight-loop automation test (`BUH.JumpGroundedReactionTest`) drives them directly in PIE and the fighter holds `Z=296.25` (90+206.25) through the reaction pause and 5 post-reaction ticks. No fall.
- `ResolveCombat` and `GenerateTimeline` agree on the reaction boundary: `reaction=1`, `reactionInstant=4`, `finalHeight=206.25`.
- `BP_CombatManager:EventTick` IS ticking in PIE. It now runs `TickJumpPlayback`, `LogFighterZ`, `HandleTargetingTick`, and the Instant/phase countdowns.
- `HandleTargetingTick` (new C++ UFUNCTION) detects left-mouse clicks when input mode is GameAndUI and calls `SetPreviewJumpTarget`/`SetExecJumpTarget`.

### What is broken
- **WBP_CombatUI EventTick is NOT ticking.** The `UI` variable on BP_CombatManager is `None` every PIE session. The `CastToWBP_CombatUI` in `BeginPlay` fails, so the widget is never stored, never gets its EventTick running, and the Jump/Execute buttons are dead.
- The WBP asset was restored from git (`c31d7f7`) and `tickFrequency` was set to `Auto` and saved, but the EventTick still does not fire. The root cause of the failed cast is unknown.
- Because the WBP EventTick is dead, `TargetingAction` is never set by the UI, `SelectJumpDestination` is never called, and `ExecuteMove` is never triggered by the Execute button.
- `ActorTools` has no `call_function` method, so Blueprint functions cannot be invoked via MCP.
- The full WBP EventTick DSL restoration fails on a `StateMachine|GetState` pin-type error when keyboard shortcuts are included.

### Workarounds attempted
- Set `TargetingAction=9`, `PlayerMove=9`, `JumpAwaitingExecute=true` manually via `ObjectTools.set_properties`. Jump target defaults to (0,0).
- Changed `BeginPlay` input mode from `SetInputModeUIOnly` to `SetInputModeGameAndUI` so the player controller receives mouse input.
- With these manual settings, the user reported "Jumped and fell" — but logs show `PZ=90.00 active=0` throughout, meaning `BeginJumpPlayback` was never actually called (no `ExecuteMove` trigger).

### What ChatGPT needs to figure out
1. Why `CastToWBP_CombatUI` fails in `BP_CombatManager:BeginPlay`, leaving `UI=None`.
2. How to get the WBP EventTick ticking (or move all UI input handling to BP_CombatManager EventTick / C++).
3. The actual per-frame writer that drops the fighter to Z=90 after the reaction pause in the real game flow (the tight-loop test proves C++ playback is clean, so the writer is external to the playback path).

## Expected behavior

When Jump is interrupted at a legal reaction opportunity:

- The fighter stops at the precise simulated XY position and height.
- The fighter remains suspended in midair indefinitely while the player considers the next decision.
- The remaining Jump parabola is displayed.
- Continue resumes the original Jump from that same position and flight progress.
- There is no additional falling or visual interpolation during the pause.

Do not implement gravity during the planning state. Time is frozen.

## 1. Investigate the actual live transition

Inspect the latest GitHub source and saved Blueprint graphs:

- `UBuhCombatSim::ResolveCombat`
- `UBuhCombatSim::GenerateTimeline`
- `UBuhCombatSim::BeginJumpPlayback`
- `UBuhCombatSim::TickJumpPlayback`
- `UBuhCombatSim::StopJumpPlayback`
- `BP_CombatManager.ExecuteMove`
- `BP_CombatManager.EventTick`
- `WBP_CombatUI.EventTick`

Focus on the transition from visual playback to reaction planning.

At the exact frame where the reaction menu becomes available, log:

- Authoritative CurrentState player Action, Phase, PhaseInstants, XY, Height.
- Result.FinalState player XY and Height.
- Last playback snapshot XY and Height.
- Real fighter actor XYZ immediately before playback stops.
- Real fighter actor XYZ immediately after playback stops.
- Real fighter actor XYZ on the following five graphical ticks.
- Whether playback remains active.
- Every function that writes the player's actor location during these ticks.

Find the FIRST operation that causes the actor's Z coordinate to decrease unexpectedly.

## 2. Check for simulation/playback divergence

The current `BeginJumpPlayback()` independently calls `GenerateTimeline(PreState, PlayerDecision, OpponentDecision, ...)`.

It does not directly play the same ordered snapshots used by the authoritative ResolveCombat result.

Compare:
- The actual resolver's reaction boundary.
- The playback timeline's reaction boundary.
- The final playback snapshot.
- The actual FinalState.

They must agree on the fighter's XY, Height, phase, and elapsed flight Instants.

If the playback timeline continues to landing after the authoritative resolver has stopped for a reaction, fix that divergence.

Prefer recording or deriving the visual snapshots from the same deterministic resolution sequence that produced FinalState. Avoid two independently diverging combat outcomes.

Do not modify Jump's balance values to hide the issue.

## 3. Make the reaction boundary authoritative

When playback reaches the actual reaction pause:

1. Stop advancing visual playback.
2. Apply the exact authoritative reaction-state transform to the real fighter.
3. Preserve its positive airborne Height.
4. Preserve the original Jump takeoff and landing points.
5. Preserve elapsed flight Instants.
6. Render the remaining parabolic trajectory.
7. Enter frozen planning state.

While waiting for the player's reaction decision, no other Tick, UI refresh, timeline update, actor transform, or movement handler should move the real fighter toward the floor.

Do not use `SetActorLocation(..., Z=90)` at a midair reaction pause.

Do not rely on the last displayed playback snapshot if it disagrees with authoritative CurrentState.

Do not force the actor's Z on every frame as a workaround. Fix the writer or incorrect state transition that causes the drop.

## 4. Check every actor-location writer

The previous code contained direct Blueprint actor placement and now also uses C++ playback.

Audit possible conflicting writers:
- BP_CombatManager's final transform application.
- BP_CombatManager.EventTick.
- WBP_CombatUI.EventTick.
- Playback completion and cleanup.
- Reaction UI refresh.
- Actor movement/components.
- Any remaining reset-to-ground logic.

`StopJumpPlayback()` should disable playback without resetting a fighter's height.

Check the actual saved execution wiring rather than inferring behavior from a function name.

## 5. Preserve Continue

During the pause:
- The fighter remains frozen midair.
- The path begins at the airborne location.
- The remaining reactable trajectory is green.

After Continue:
- Playback resumes the same Jump.
- The fighter moves from its saved airborne position.
- The trajectory does not restart at takeoff.
- The fighter does not teleport back upward after falling.
- Landing restores its normal ground height.

## 6. Add a genuinely end-to-end regression test

The existing JumpPlaybackIntegrationTest starts its reaction test from a manually constructed airborne state.

That test is useful but insufficient.

Add a new integration case that:

1. Starts from an ordinary grounded encounter.
2. Selects and commits a real Jump.
3. Produces a qualifying observable opponent threat after the Jump becomes reactable.
4. Advances to the genuine reaction boundary.
5. Runs the same playback path used by the game.
6. Confirms the authoritative fighter is airborne.
7. Confirms the real actor Z equals 90 + authoritative Height.
8. Advances several more UI/game ticks without issuing a decision.
9. Confirms the actor's XY and Z do not change at all.
10. Selects Continue and confirms the fighter completes the original parabola.

Use the actual PIE-world actor and manager where possible.

A test that only constructs a fighter already in midair must not count as passing this new requirement.

Keep all existing Walk/Dash/Jump regression suites.

## 7. Verification and GitHub

Rebuild the changed C++ code.

Compile and save the affected Blueprints.

Export their actual saved graph readback into `tests/blueprint_exports/jump_selection_flow.md`.

Update `notes.md` and `todo.md` with the confirmed cause and evidence.

After automated verification, commit and push to `origin/master` on `HelHeresyDev/BeneathUncountedHeavens`.

Report the commit SHA and test results. Distinguish automated checks from user-confirmed graphical PIE behavior.

No Qinggong, aerial attacks, new movement mechanics, or unrelated refactoring.

**Success criterion: At a Jump reaction opportunity, the fighter freezes at its exact airborne location and stays there until the player acts. Continue resumes from that exact position without a fall, reset, or second jump.**