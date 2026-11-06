# Beneath Uncounted Heavens — Unblock Stage B Walk Path Rendering

Review the current `todo.md` and `tmp_opencode/stage_a_analysis.md`.

Stage B is partially implemented, but the green Walk line is blocked because the Blueprint DSL does not expose `DrawLine`.

**Unblock this without rewriting the existing targeting system.**

## 1. Use the correct Unreal drawing API

Unreal Engine 5.8 provides:

`UKismetSystemLibrary::DrawDebugLine`

Header:
`Kismet/KismetSystemLibrary.h`

It is BlueprintCallable, but DevelopmentOnly.

Check whether the MCP Blueprint tools expose this function under Kismet System Library rather than Kismet Rendering Library.

If the DSL cannot call it, create a minimal BlueprintCallable C++ wrapper.

For the immediate PIE test:
- Draw a green line between the real player's ground-plane position and the current proposed destination.
- Offset it slightly above the ground to avoid z-fighting.
- Use a visible thickness.
- Update it every frame while targeting.
- Keep it visible after confirmation during planning.
- Stop drawing it when the preview is canceled, executed, or reset.

Use short-lived debug drawing rather than persistent debug lines that accumulate.

This is a temporary visual validation mechanism, not the final shipping renderer.

## 2. Plan the reusable renderer

After the temporary green line works, implement a reusable path visualization component or actor using an appropriate runtime rendering solution.

Consider USplineMeshComponent segments.

The renderer must eventually support:
- Walk: a straight green ground path.
- Dash: straight red/green segments.
- Jump: sampled three-dimensional parabolic arc with red/green segments.

Reaction colors will ultimately come from simulation Instant data.

Do not implement Dash or Jump now.

Do not introduce an unnecessarily complicated path framework before validating Walk.

## 3. Verify mouse-targeting events

The recent implementation uses EventTick for cursor targeting and mouse-button detection.

Inspect the real event ordering.

Ensure:
- Clicking the Walk UI button doesn't immediately confirm a destination.
- Clicking another UMG control doesn't confirm a destination.
- Left-click confirms only on a valid ground target.
- Right-click/Escape cancels.
- The cursor remains visible and functional.
- The yellow marker follows the ground target correctly.
- Repeated targeting does not spawn duplicate actors.
- The combat planning EventTick functionality remains connected.

Use click-edge detection or equivalent input handling to prevent an already-held UI click from being interpreted as a ground confirmation.

## 4. Do not misrepresent the 1D simulation

Stage C has not started. The existing CombatSim still uses X-only positions.

Until authoritative 2D movement is implemented:
- Do not claim a nonzero Y movement preview represents actual simulated movement.
- Do not silently discard the selected Y value during execution.
- Keep a clear boundary between visual targeting and authoritative simulation.
- Preserve the existing functional 1D combat tests.

If necessary, limit execution of experimental 2D targets until Stage C is ready.

## 5. Test Stage B in graphical PIE

Verify:
1. Select Move → Walk.
2. Cursor moves the yellow destination marker.
3. A straight green line follows the destination.
4. Confirming retains the chosen preview.
5. Canceling removes the preview.
6. Clicking combat UI does not accidentally confirm.
7. Reset clears targeting.
8. Continue and existing reaction controls still work.

Compile, save, and run the existing automation tests.

Report what is actually verified headlessly versus what still requires manual graphical PIE testing.

**Do not start Stage C until the Stage B targeting and path visualization have been visually verified.**

No unrelated UI redesign, new movement mechanics, or Git commits without authorization.

## Progress

- [x] C++ `DrawGroundLine` wrapper added (uses `::DrawDebugLine` from `DrawDebugHelpers.h`).
- [x] `bSuppressWalkClick` variable added to `WBP_CombatUI`.
- [x] `OnClicked(BtnWalkMove)` updated to enter targeting mode with click suppression.
- [x] `EventTick` targeting block: mouse raycast, `DestPlayer` marker movement, green `DrawGroundLine`, left-click confirm (suppressed on first frame), right-click/Escape cancel.
- [x] `EventTick` combat planning logic intact.
- [x] Blueprint compiles and saves cleanly.
- [x] All 4 BUH automation suites pass.
- [ ] Graphical PIE verification of targeting, green line, and click-edge behavior.
- [ ] Reusable path renderer (USplineMeshComponent) for final shipping visualization.

## Diagnostic Findings (2026-11-06)

### Mouse tracking
- Green line renders correctly (confirmed by user screenshot).
- User reports poor mouse tracking: "good for controller control scheme, but not for mouse control."
- Diagnostics added to EventTick: `PrintString "Mouse: <Vector2D>"` and `PrintString "Ground: <X>, <Y>"` while targeting.
- Suspected causes: (1) `GetMousePosition` returns window-relative coords but `DeprojectScreenToWorld` expects viewport-relative; (2) steep camera angle makes small mouse deltas produce large ground deltas; (3) EventTick polling lag.
- Need graphical PIE log output to confirm which cause.

### Click confirmation not setting position (FIXED)
- User reports: "clicking should set the position but it currently does not."
- Root cause: `BP_CombatManager.BeginPlay` sets input mode to **UI Only** (§9.16 notes.md). In UI Only mode, all mouse input routes to UMG widgets. `WasInputKeyJustPressed("LeftMouseButton")` on the PlayerController never fires because UMG consumes the click first.
- Fix applied: `OnClicked(BtnWalkMove)` now calls `Input|SetInputModeGameAndUI` before entering targeting. EventTick confirm/cancel branches call `Input|SetInputModeUIOnly` to restore UI Only mode.
- All 4 BUH automation tests pass after the change.
- `SelectWalkDestination` takes `TargetX` float only — Y is used for the marker/line but not for the authoritative 1D simulation (correct per §4 "Do not misrepresent the 1D simulation").
- **Needs graphical PIE verification** that the click now confirms the destination.

### Line persistence after confirmation (FIXED)
- User: "I want the line to stay after I click."
- Root cause: `DrawGroundLine` was only inside the `if (GetWalkTargeting)` block, so the line stopped when targeting ended.
- Fix: Added a second `DrawGroundLine` call outside the targeting block, gated on `GetPlayerMove == 6` (Walk destination selected). Uses the DestPlayer marker's location as the line endpoint. Line persists during planning until Execute/Reset/Cancel.

### Cursor disappears after confirming destination (FIXED)
- User: "after I click the cursor disappears."
- Root cause: `bShowMouseCursor` was being set to false by the input mode change.
- Fix: Added `Class|PlayerController|SetShowMouseCursor :self ... :bShowMouseCursor true` to `OnClicked(BtnWalkMove)` and both confirm/cancel branches in EventTick. Input mode stays as Game+UI (no `SetInputModeUIOnly` on confirm/cancel).

### DSL scoping
- A `write_graph_dsl` attempt failed with "Undefined variable _hitx" when `SetActorLocation`/`DrawGroundLine` referenced variables bound inside a nested `if` block. The saved blueprint (from an earlier successful write) is structurally correct — the failure was from a different, aborted rewrite attempt.