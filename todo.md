# Beneath Uncounted Heavens — Fix Jump Playback Ownership and Airborne Path

## Completed (headless-verified)

- **§1 Duplicate ticking eliminated.** Removed `(CombatSim|TickJumpPlayback InDeltaTime)` from `WBP_CombatUI.EventTick`. `BP_CombatManager.EventTick` is now the sole playback driver. Confirmed by reading back both live graphs: WBP EventTick has no `TickJumpPlayback`; manager EventTick has exactly one.
- **§2 Preview overwrite prevented.** All WBP movement-path drawing (targeting parabola, Walk ground line, confirmed Jump/Dash preview) is now gated on `State == 0` (Planning). While playback owns the path (State 1/Resolving) or a reaction is pending (State 3/Reaction), the WBP does not redraw using DestPlayer or stale targeting coordinates. The C++ `TickJumpPlayback` (`DrawJumpPathFromState`) is the sole path owner during execution.
- **§5 Duplicate mouse input removed.** Removed `(CombatSim|HandleTargetingTick ...)` from `BP_CombatManager.EventTick`. The WBP EventTick mouse-confirmation (`WasInputKeyJustPressed LeftMouseButton` → `SelectJumpDestination`/`SelectDashDestination`/`SelectWalkDestination`) is now the single authoritative targeting-confirmation path.
- **§4 Authoritative pause verified.** `BUH.JumpGroundedReactionTest` passes: fighter ascends 90 → 186.2 → 255.0 → 296.2, freezes at `Z=296.25` at the reaction boundary (`reaction=1`, `finalHeight=206.25`), and holds `active=0` with no fall through 5 post-reaction ticks.
- Both Blueprints compiled clean and saved.

## Remaining (graphical PIE)

- **§3 Real UI-driven Jump.** Headless PIE cannot click the WBP buttons (Slate snapshot shows no widget buttons under `-nullrhi`), and MCP has no console-command runner, so the full UI path (click Jump → click destination → click Execute → `ExecuteMove` → `BeginJumpPlayback`) has not been exercised end-to-end in a headless session. A graphical PIE run is needed to confirm a real button click reaches `ExecuteMove`, the manager State changes to Reaction, Continue appears, and the fighter stays airborne with the correct remaining arc.
- **§6 Graphical verification.** One playback tick per frame, one path owner per UI state, correct remaining parabola, no stale destination overwrite, correct Reaction/Continue HUD, Walk/Dash still work — all need a graphical PIE confirmation.

---

The user's latest graphical PIE screenshots show partial progress but incorrect behavior.

Observed:
- The blue cylinder can visibly become airborne.
- While airborne, a straight green line extends offscreen to the right instead of following the remaining parabola.
- The airborne screenshot displays "Select a move / Ready", not a reaction menu.
- A newly selected Jump still renders a curved path with red near the player and green toward the landing point. That color order is CORRECT for a right-to-left jump.

I've reviewed commit c0a7504 and the Blueprint exports in GitHub.

## 1. Critical: eliminate duplicate playback ticking

The saved exports show BOTH:

`BP_CombatManager.EventTick → TickJumpPlayback(DeltaSeconds)`

and

`WBP_CombatUI.EventTick → TickJumpPlayback(InDeltaTime)`

Now that the widget is running again, this may advance the same static playback timeline twice per frame.

Verify the actual saved graphs and runtime call counts.

Make **BP_CombatManager the sole playback driver**.

Remove the playback-advancement call from WBP_CombatUI.EventTick, leaving the widget responsible for UI interaction and presentation.

Use one game-world owner and one consistent Instant clock.

Verify exactly one TickJumpPlayback invocation per graphical game frame.

## 2. Prevent the targeting preview from overwriting execution

The WBP EventTick currently draws Jump when `PlayerMove == 9`, using the DestPlayer actor's location.

This is insufficient because PlayerMove can remain 9 after Execute.

Inspect whether WBP continues drawing the old destination preview over the C++ playback's authoritative remaining path.

Enforce exclusive rendering ownership:

- Actively targeting a NEW Jump: show the targeting parabola.
- Confirmed new Jump awaiting Execute: show the committed preview.
- Executing Jump: playback owns the remaining trajectory.
- Paused airborne Jump: authoritative CurrentState owns the remaining trajectory.
- Jump completed: clear the old path.
- Replacement action: show the new action's preview.

Do not use PlayerMove==9 alone to decide that a preview should be drawn.

While playback owns the path, WBP must not redraw using DestPlayer or stale targeting coordinates.

Audit all SetPath, DrawJumpPath, DrawGroundLine, and ClearWalkPath calls to identify the last writer.

## 3. Distinguish the diagnostic command from real gameplay

`BUH.TriggerReactionJump` starts playback directly but does not execute the full manager decision lifecycle.

Therefore it cannot prove the actual reaction UI works.

Test BOTH:

A. Diagnostic playback:
- Verify airborne position and path.
- Do not interpret its unchanged UI as a gameplay-state regression.

B. Real UI:
- Select Jump.
- Confirm destination.
- Press Execute.
- Reach the actual reaction boundary.
- Verify the manager's State changes to Reaction.
- Verify Continue appears.
- Verify the fighter remains airborne.
- Verify the correct remaining parabola persists.

If normal UI execution does not reach a reaction, debug that separately. Do not silently substitute the diagnostic command.

## 4. Verify the authoritative pause

At the reaction boundary, compare:

- CurrentState Action, Phase, PhaseInstants.
- CurrentState XY and Height.
- Playback's final snapshot XY and Height.
- Real fighter actor XYZ.
- Manager.State and ReactionOpportunity.
- Current renderer points and colors.

The real fighter must remain at exactly `GroundZ + CurrentState.Height` while awaiting a decision.

Continue must resume the same original trajectory.

## 5. Avoid duplicate mouse input handling

The current manager EventTick also calls `HandleTargetingTick`, while the WBP EventTick has its own mouse-confirmation code.

Check for two handlers processing the same click.

Keep one authoritative targeting-confirmation path and preserve the working Walk/Dash/Jump UI.

Do not let a debug input handler update target coordinates independently after the normal UI confirms a destination.

## 6. Verify and push

Run a genuine PIE-world integration test with the real combat manager and widget.

Check:
- One playback tick per game frame.
- One effective path owner per UI state.
- Correct parabolic remaining path.
- No stale destination-marker overwrite.
- Correct Reaction/Continue HUD.
- Fighter frozen in midair until a decision.
- Continue resumes the original Jump.
- Walk and Dash still work.

Compile and save all edited Blueprints.

Update the actual read-back exports under tests/blueprint_exports/.

Update todo.md and notes.md with the confirmed causes and results.

Commit and push to origin/master and report the commit SHA.

Keep the current Jump mechanics and reaction timing unchanged. Do not implement additional movement systems.

**Success: Real UI-driven Jump follows a parabola, pauses airborne with the correct remaining arc and Continue button, and resumes without any duplicate ticking or stale preview rendering.**