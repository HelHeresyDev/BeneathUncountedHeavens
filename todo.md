# Beneath Uncounted Heavens — Fix Dirty Initial Encounter State

## STATUS — RESOLVED (headless-verified; interactive graphical checks remain)

Root cause: `TargetingAction` was never reset to `-1` at startup, so its default (`>= 0`)
kept the `WBP_CombatUI.EventTick` targeting-preview block active every frame, showing the
`DestPlayer` marker and redrawing the green `DrawGroundLine` path even after `ResetEncounter`
cleared it. Fix: `SetTargetingAction -1` inserted into `EventBeginPlay` (before `CreateWidget`)
and `ResetEncounter` (before `RefreshUI`); the `WBP_CombatUI CONSTRUCTED` `PrintString` removed
from `EventConstruct`. Verified headless: fresh PIE launch reads `TargetingAction = -1`,
`State = 0`, `PlayerMove = -1`, `PathRenderer.segmentPool = []`; both Blueprints compiled and
saved; all 22 `BUH.*` automation tests pass. Remaining manual graphical-PIE checks: interactive
Walk/Dash/Jump → cancel (B–E), switch-to-Attack (E), and reset-after-movement (G) — headless
`-nullrhi` PIE cannot click WBP buttons or move the cursor. See notes.md §9.40.

The user has confirmed in graphical PIE that Jump, airborne interruption, and Continue now work.

**Do not change those mechanics.**

The remaining problem is the initial presentation.

At fresh PIE launch:
- HUD says "Select a move / Ready".
- HUD says "Your: None".
- Both real fighters are present.
- A yellow DestPlayer cylinder is visible.
- A green movement line is already visible between the player and the destination marker.
- No movement action has been selected.

This should be a completely clean planning state.

## 1. Investigate initialization and reset

Read the actual saved graphs for:

- `BP_CombatManager.EventBeginPlay`
- `BP_CombatManager.ResetEncounter`
- `BP_CombatManager.CancelTargeting`
- `WBP_CombatUI.EventConstruct`
- `WBP_CombatUI.RefreshUI`
- `WBP_CombatUI.EventTick`

Also inspect `ABUHPathRenderer.BeginPlay`, `SetPathVisible`, `SetPath`, and `ClearPath`.

Important findings from GitHub:

- EventBeginPlay creates and adds WBP_CombatUI to the viewport, waits 0.5 seconds, then calls ResetEncounter.
- PathRenderer.BeginPlay hides its path.
- notes.md §9.24 documents an earlier fix where ResetEncounter needed to call CancelTargeting and hide DestPlayer.
- PathRenderer.SetPath is persistent until explicitly cleared.

Verify whether those earlier ResetEncounter fixes remain intact and correctly connected in the saved Blueprint.

## 2. Identify the last startup path writer

Do not assume ResetEncounter isn't running.

During one fresh PIE launch, log:

1. PathRenderer.BeginPlay.
2. WBP EventConstruct.
3. CombatManager.BeginPlay.
4. ResetEncounter entry and completion.
5. Every SetPath, SetLine, DrawGroundLine, DrawDashPath, DrawJumpPath, and ClearPath until the first clean planning frame.

For each path write, identify its actual caller and the relevant PlayerMove, TargetingAction, and manager State.

Determine whether:
- ResetEncounter fails to clear the path.
- The path gets drawn after ResetEncounter.
- The marker isn't hidden.
- A default targeting variable is briefly active during widget construction.
- A saved actor or widget default is exposing stale preview geometry.

Find the precise cause before editing.

## 3. Establish a clean initial-state invariant

After initial setup, and after ResetEncounter:

- State = Planning (0).
- PlayerMove = -1.
- TargetingAction = -1.
- No pending Walk, Dash, or Jump selection.
- No pending movement destination.
- bHasCurrentState and reaction flags reset appropriately.
- DestPlayer hidden.
- GhostPlayer hidden.
- GhostOpponent hidden.
- PathRenderer hidden, with zero visible segments.
- No stale predicted timeline or destination.
- No movement selected in the HUD.
- Only the two real fighters visible in the arena.

A clean state should be established before the player is allowed to interact.

Preserve the correct fighter positions and health.

## 4. Fix startup ordering and preview cleanup

Make the smallest reliable correction.

If the UI is ticking before the encounter is initialized, correct the ordering or guard presentation until initialization finishes.

If ResetEncounter is incomplete, repair its existing cleanup execution chain.

If a later function redraws a preview when PlayerMove == -1, fix that function's eligibility condition.

Prefer centralized, idempotent cleanup shared by fresh initialization and ResetEncounter.

**Do not clear paths every EventTick.**

That would break the already-working confirmed Dash previews and midair Jump reaction paths.

Do not hide legitimate movement paths during targeting, execution, or reactions.

## 5. Remove remaining temporary startup diagnostics

The large yellow Dash version banner is already removed.

The screenshot still shows "WBP_CombatUI CONSTRUCTED" at the upper left.

Remove that temporary on-screen construction diagnostic if no longer needed.

Keep useful error logging, but eliminate unnecessary per-frame diagnostics.

## 6. Regression verification

Check in actual PIE or with a valid PIE-world integration test:

A. Fresh launch: clean encounter with only two fighters.

B. Move → Walk → cancel: no leftover line or destination marker.

C. Move → Dash → cancel: no leftover line or marker.

D. Move → Jump → cancel: no leftover arc or marker.

E. Select any movement, then Back and switch to Attack: no stale movement geometry.

F. Execute Jump → freeze in midair → Continue: existing working trajectory and playback unaffected.

G. Reset after completing movement: same clean state as fresh launch.

H. Start another PIE session: still clean.

Assert actual renderer visibility and ghost actor hidden flags, not just PlayerMove == -1.

Compile and save the affected Blueprints, then read back the saved graph.

Run all BUH automation suites.

## 7. GitHub

Update `notes.md`, `todo.md`, and relevant Blueprint exports with the proven startup cause and fix.

Commit and push to `origin/master`.

Report the commit SHA, root cause, automated tests, and any graphical PIE checks still needed.

**Success: Fresh PIE opens to a clean battlefield with only the red and blue real fighters, no yellow marker, no green line, and no debug banner. All existing movement and reaction behavior remains working.**