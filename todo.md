This looks like another regression in the Walk-targeting controls. I checked the latest GitHub commit, `63fcbad`, and the project's previous fixes.

Your screenshot shows `Your: None` and `No preview`. Those are normal before choosing a Walk destination, but **the cursor should remain visible and a green ground path should follow it while targeting**.

### **What I found**

The project has encountered almost this exact cursor problem before. In `notes.md` §9.24, `SetInputModeGameAndUI` was hiding the cursor because `bHideCursorDuringCapture` was enabled. Section 9.30 also established that targeting handlers need to explicitly set `ShowMouseCursor = true`.

There is another possible issue: the click that selects Walk might also be interpreted as the click confirming its destination, immediately ending targeting.

The current targeting flow should be:

Click Walk

TargetingAction = 6

Cursor visible + green line follows mouse

Separate ground click confirms destination

I also notice **Continue is visible in the movement submenu during ordinary planning**. That should only be available during a genuine movement reaction.

## **Next Qwen prompt**

# BUH — Repair Walk Targeting Cursor and Missing Path

Repository: `HelHeresyDev/BeneathUncountedHeavens`\
Baseline: `63fcbad`

## Reproduction

1. Start a fresh graphical PIE session.
2. Open the Move submenu.
3. Click Walk.

Observed:

- Mouse cursor disappears.
- No green Walk targeting line appears.
- No destination marker appears.
- HUD remains `Your: None`.
- Timeline remains `No preview`.

The screenshot also shows Continue visible during ordinary planning.

## Investigation — identify the actual failure

Inspect the saved Blueprint graphs:

- `WBP_CombatUI.OnClicked(BtnWalkMove)`
- `WBP_CombatUI.EventTick`
- `WBP_CombatUI.RefreshUI`
- `BP_CombatManager.BeginWalkTargeting`
- `BP_CombatManager.CancelTargeting`
- `BP_CombatManager.SelectWalkDestination`

Read project notes §§9.23, 9.24, 9.30, 9.34 and 9.40 before modifying anything.

### Check 1: Cursor capture

Inspect `SetInputModeGameAndUI` in the Walk handler.

Verify:

- `bHideCursorDuringCapture = false`
- `SetShowMouseCursor(true)` is called on the correct PlayerController.
- The mouse is not being captured by the first-person controller.
- No later event immediately hides the cursor or changes input mode.

### Check 2: Targeting state

Verify clicking Walk calls `BeginWalkTargeting` and sets `TargetingAction = 6`.

Log the actual value immediately after clicking and on the next EventTick.

Check whether another function immediately calls `CancelTargeting`, returning the value to -1.

### Check 3: Same-click confirmation

Investigate whether the mouse click that activates the Walk button is also detected by `WasInputKeyJustPressed(LeftMouseButton)` in EventTick.

If so, targeting may start and cancel in the same frame.

Targeting confirmation must require a separate mouse click after the Walk button has been released.

Do not introduce arbitrary timing delays if a proper input-state transition can solve it.

### Check 4: Path generation

When `State == 0` and `TargetingAction == 6`:

- EventTick must update the ground cursor location.
- The ground hit test must return a valid location.
- `DrawGroundLine` must receive the correct start and end coordinates.
- `DestPlayer` must be positioned and visible.
- No other function should clear the path every frame.

Inspect the existing `BUHPathRenderer` rather than replacing it.

## Required behavior

- Clicking Walk enters targeting mode.
- Cursor remains visible.
- Green path follows the cursor across the ground.
- A separate left-click confirms the destination.
- Confirmation produces a normal Walk preview and cyan ghost.
- Right-click or Escape cancels targeting.
- Canceling restores normal input and removes temporary targeting geometry.
- Walk → Dash → Jump transitions remain functional.
- Continue is unavailable during ordinary planning and visible only when a genuine resumable movement reaction exists.

## Preserve existing systems

Do not modify the combat resolver, movement speed, hypothetical opponent branches, gold reaction ghost, or timeline scrubber.

Avoid full EventGraph rewrites. The project previously lost entire EventTick handlers from `write_graph_dsl`. Prefer surgical node edits and read back the actual connections afterward.

## Verification

Perform automated checks of targeting transitions, including the same-click scenario.

Inspect Blueprint compile warnings, not merely compile success.

Verify C++/Blueprint wiring and saved assets.

Report exactly:

- Why the cursor disappeared.
- Whether TargetingAction was set and remained active.
- Why DrawGroundLine was not visible.
- Which Blueprint nodes changed.
- Automated results.
- What still requires manual graphical PIE verification.

Update notes.md and todo.md, commit and push.

Do not claim that cursor visibility is verified from headless PIE.

**My priority would be fixing targeting before touching predictions again.** The movement-preview architecture is working well enough now that we should stabilize how actions are selected and confirmed, rather than risk another regression in the combat simulator.