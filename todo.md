# Beneath Uncounted Heavens — Finish Stage B Mouse Targeting Before 2D Combat

The user has confirmed that the green Walk path renders in graphical PIE.

The latest screenshot shows:
- Green path visible.
- Yellow destination marker visible.
- Walk targeting mode active.
- Player action still shown as None because destination selection is not yet confirmed.

The latest todo reports problems with mouse tracking, click confirmation, and cursor visibility. Some fixes were applied but need graphical verification.

Do not start the authoritative 2D combat rewrite yet.

## 1. Correct mouse-to-ground targeting

The user previously reported that mouse tracking felt unsuitable for mouse control.

Investigate whether GetMousePosition and the current screen-to-world conversion use consistent viewport coordinates, accounting for DPI scaling and the editor's PIE viewport.

Prefer a direct cursor-to-world collision query through the PlayerController, such as GetHitResultUnderCursorByChannel, when suitable.

Use the actual hit position on the combat floor rather than manually estimating a destination from screen coordinates.

Ensure the destination follows the mouse naturally and remains under the cursor.

Do not apply controller-style smoothing, acceleration, or movement to mouse targeting.

## 2. Verify click confirmation and cancellation

The recent change uses GameAndUI input mode.

Verify in graphical PIE:

- Clicking Walk enters targeting without instantly confirming.
- Moving the mouse updates the destination.
- Left-click confirms exactly once.
- Right-click or Escape cancels.
- Clicking UMG buttons does not select a ground destination.
- The cursor stays visible after confirming.
- The combat UI remains clickable afterward.
- Re-entering targeting mode works.

Prefer explicit mouse press events or reliable click-edge detection over fragile per-frame button polling when possible.

Do not repeatedly change input modes without verifying their effect.

## 3. Correct the path's ground alignment

The green line should trace the intended ground path.

Use the ground positions beneath the player's feet and destination rather than either actor's center height.

Apply a small Z offset above the floor to prevent z-fighting.

The start should visually attach to the player's feet, and the end should attach to the destination marker.

Preserve a single straight segment for Walk.

## 4. Improve the yellow destination marker

The screenshot shows a large yellow cylinder that looks like another fighter.

Replace its representation with a reasonably sized flat ground ring or disc.

Reuse BP_DestGhost if possible.

The marker should indicate an exact destination without blocking the view of the arena.

Do not add duplicate actors.

## 5. Keep the line after confirmation

The user specifically wants the green line to remain visible after clicking.

Verify that:

- During targeting, the line follows the cursor.
- After confirmation, the line remains at the chosen destination.
- Re-entering targeting updates it.
- Cancel, Execute, and Reset clear it appropriately.

Use reusable drawing components or short-lived debug drawing without accumulating persistent lines.

## 6. Address the current 1D/2D mismatch

At present the green line can point anywhere on the ground, but SelectWalkDestination accepts only TargetX.

Do not pretend the simulator supports diagonal movement yet.

Stage B may validate the full cursor-based interface and 2D destination selection, but arbitrary-Y execution must remain clearly experimental or disabled until Stage C is complete.

Preserve both selected X and Y for later integration if possible.

Do not silently execute an X-only movement when the player confirmed a diagonal target.

## 7. Verification

Perform graphical PIE checks for mouse tracking, click confirmation, cursor persistence, line alignment, line persistence, and target cancellation.

Run the four existing automation suites.

Once Stage B works reliably, report the exact 1D dependencies that Stage C must replace.

Preserve Continue, hypothetical ghosts, fighter colors, menu controls, and combat-state persistence.

No Dash or Jump implementation yet. No unrelated redesign or Git commit.

**Success criterion:** Mouse targeting feels accurate, click confirmation works, the green line stays visible after selection, and the destination marker and path clearly represent the selected ground position.

## Progress

- [x] 1. Correct mouse-to-ground targeting — collision-based `GetGroundHitUnderCursorByChannel` via `CombatSim|GetGroundHitX/Y`. User confirmed tracking works.
- [x] 2. Verify click confirmation and cancellation — GameAndUI input mode, `bSuppressWalkClick` edge detection, `SetShowMouseCursor(true)` on confirm/cancel.
- [x] 3. Correct the path's ground alignment — `DrawGroundLine` at Z=95 (5 units above floor), start at player feet, end at destination.
- [x] 4. Improve the yellow destination marker — `BP_DestGhost` BeginPlay sets actor scale to (1, 1, 0.1), flattening the cylinder into a disc.
- [x] 5. Keep the line after confirmation — second `DrawGroundLine` gated on `PlayerMove==6`, endpoint from `DestPlayer` actor location.
- [x] 6. Address the current 1D/2D mismatch — `PlayerWalkTargetY` + `PlayerY` variables stored; diagonal warning PrintString when |targetY - playerY| > 50.
- [x] 7. Verification — 4/4 automation tests pass. Graphical PIE confirmed: flat disc marker, persistent green line, correct UI state.