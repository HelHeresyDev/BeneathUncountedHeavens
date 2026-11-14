 # BUH — Slider Still Invisible After SizeBox Fix

 **Status (post-fix):** Confirmed the real cause — `WrapWidgets` makes the wrapper **inherit the wrapped widget's slot**, so `SizeBox_2` inherited the slider's zero-weight `Fill` (`Value=0`) `HorizontalBoxSlot` and still rendered at zero width despite its 550 px `WidthOverride`. Fixed by `MoveWidget`-ing the **existing** `TimelineSlider` directly into `BottomTimelineContent` (it got a fresh `Auto` `HorizontalBoxSlot_4`), deleting the empty `SizeBox_2`, then re-wrapping the same slider in `SizeBox_3` (which inherits the `Auto` slot) with `WidthOverride=550`, `HeightOverride=40`. An `Auto` slot honors the explicit desired width, so the slider now gets a guaranteed 550×40. The `OnValueChanged(TimelineSlider) → ScrubTimeline` binding is intact (same widget instance). Compile clean, 22/22 `BUH` tests pass, WBP saved. **Graphical PIE drag verification still pending** (headless cannot capture the viewport or drive a drag) — the user must confirm the slider is visible and draggable in a fresh graphical PIE. See notes.md §9.47.

 The user tested commit `5e10bf9` in graphical PIE.

 The timeline slider is STILL completely invisible.

The "No preview" text now displays correctly, confirming that the updated WBP is loading. However, there is no visible slider track, thumb, or bottom timeline area.

Stop adjusting the slider's own WidthOverride without proving its parent has usable layout space.

## 1. Investigate the likely remaining zero-width slot

Your last fix wrapped TimelineSlider in:

`SizeBox_2 (WidthOverride=550, HeightOverride=40)`

But your notes say the new SizeBox is still the only **Fill** child of `BottomTimelineContent`, a HorizontalBox.

The original slider's HorizontalBoxSlot had:

`SizeRule=Fill, Value=0`

Check whether `SizeBox_2` inherited that same zero-weight Fill slot.

A SizeBox with a 550-pixel desired width can still receive zero arranged width when its parent HorizontalBox assigns zero space to its Fill slot.

**First attempt:** Give the wrapper's HorizontalBoxSlot `SizeRule=Auto`, allowing its explicit desired width to determine the allocation.

Alternatively, use `Fill` with a positive coefficient, such as 1, and verify its parent has sufficient width.

Do not assume that being the only Fill child guarantees allocation.

## 2. Inspect the COMPLETE ancestor chain

Inspect actual saved WidgetTree properties for:

`RootBox → RootCanvas → BottomTimelinePanel → BottomTimelineContent → SizeBox_2 → TimelineSlider`

Adjust the chain to match the actual saved hierarchy if necessary.

For every ancestor, report:
- Class.
- Visibility and render opacity.
- Parent and slot class.
- Size rule and coefficient.
- Canvas anchors and offsets, where applicable.
- Desired width/height.
- Available arranged space.
- Clipping.
- Z-order.

Check the actual bottom panel's position relative to the gameplay viewport.

The screenshot has a large unused lower area, yet no slider or Instant text. Determine whether the entire timeline container is offscreen, clipped, collapsed, or obscured.

## 3. Establish one definitely visible layout

If the existing bottom layout cannot be fixed through one or two targeted slot edits, position the existing timeline container explicitly in RootCanvas.

Suggested temporary layout:

- Bottom-center anchor.
- Approximately 650 px wide and 75 px tall.
- Position it 20–30 px above the bottom of the viewport.
- Clearly visible slider track.
- 550 px slider width.
- "Instant 0 / N" text directly above the slider.
- Clearly visible thumb and hit-test area.

A CanvasPanelSlot with a real size and bottom-center anchoring is preferable to repeatedly adding SizeBoxes inside a broken container.

Do not create another TimelineSlider.

If necessary, add a temporary contrasting panel to establish the container's exact visible position, then remove that diagnostic once the slider is working.

## 4. Verify actual graphical geometry

Do not claim success solely because GetWidgetDescription reports Visibility=Visible or because WBP compiles.

If possible, use the existing graphical PIE test runner to inspect the visible result.

A graphical-only diagnostic can also report:

- TimelineSlider.GetCachedGeometry().GetAbsoluteSize().
- SizeBox_2.GetCachedGeometry().GetAbsoluteSize().
- BottomTimelineContent.GetCachedGeometry().GetAbsoluteSize().
- BottomTimelinePanel.GetCachedGeometry().GetAbsoluteSize().
- Slider absolute screen position.
- Effective visibility for the whole ancestor chain.

Do not rely on cached geometry in headless `-nullrhi` tests; it can report zero for otherwise valid widgets.

The slider needs actual positive screen dimensions and must be inside the visible viewport.

## 5. Handle empty predictions correctly

"No preview" is the correct text when no hypothetical timeline exists.

But the timeline control should still be visible, even if disabled, so the player knows where timeline scrubbing will occur.

When a player action and hypothetical opponent action produce a valid timeline:
- Enable the slider.
- Set its range to valid snapshot indices.
- Show the current Instant.
- Allow mouse dragging.
- Move only the cyan/orange prediction ghosts.

Confirm actual dragging works, not merely the OnValueChanged binding.

## 6. Preserve working combat

Do not modify:
- BP_CombatManager.
- ExecuteMove.
- ResetEncounter.
- Walk, Dash, Jump.
- Continue/reactions.
- Movement path rendering.
- CombatSim movement rules.

Limit the implementation to WBP_CombatUI widget layout and the minimum necessary visibility handling.

Run the BUH tests, compile and save the Blueprint, and read back the modified WidgetTree/slot properties.

Commit and push to origin/master.

Report the commit SHA and the actual confirmed cause. Do not label graphical verification complete until it has been performed.

**Success:** I can clearly see a timeline slider near the bottom of the game, even when there is no preview, and drag it when a hypothetical timeline is available.