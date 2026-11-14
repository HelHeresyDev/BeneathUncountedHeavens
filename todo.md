# BUH — Stop Guessing: Move Timeline Into Visible RootCanvas

## Status (this pass)
- **§1 done:** `BottomTimelineContent` moved out of the `BottomTimeline` Border into `RootCanvas` as a direct child (fresh `CanvasPanelSlot_1`). No second slider created.
- **§1 done:** `CanvasPanelSlot_1` configured via whole-object `layoutData` — anchors min/max `(0.5,1.0)`, offsets `left=-450, top=-95, right=450, bottom=-25`, alignment `(0.5,1.0)` → bottom-center 900×70, 25 px above the viewport bottom; `bAutoSize=false`, `zOrder=10`.
- **§1 done:** existing `TimelineSlider` stays inside `BottomTimelineContent` → `SizeBox_3`; `OnValueChanged(TimelineSlider) → ScrubTimeline` binding intact (same widget instance).
- **§4 done:** no gameplay/`BP_CombatManager`/`ExecuteMove`/`EventTick`/`RefreshUI` changes; walk/dash/jump/continue untouched.
- **§5 done:** `compile_blueprint` clean; WidgetTree + `CanvasPanelSlot_1` read back with the exact values; 22/22 `BUH` automation tests pass; `save_assets` succeeded.
- **§5 pending (limitation, not resolved):** graphical PIE on-screen visibility + slider drag. Headless `-nullrhi` cannot capture the viewport or drive a drag, so this must be confirmed by the user's fresh graphical PIE. Per §5, this is reported as a limitation, **not** called resolved.
- Root cause recorded in `notes.md` §9.48: the absent area was the `BottomTimeline` Border ancestor chain, not the slider slot; the fix bypasses it by reparenting `BottomTimelineContent` directly under `RootCanvas`. `RootBox` has exactly one child (`RootCanvas`), so `RootCanvas` is the sole `Fill` child and gets full height, making the bottom anchor valid.

The user tested commit b91b499 in graphical PIE.

**The timeline is STILL completely invisible.**

Two attempted fixes have failed:
- SizeBox_2 with WidthOverride=550.
- SizeBox_3 with a fresh Auto HorizontalBoxSlot and WidthOverride=550.

Both compiled and passed automation, but neither made the timeline appear.

The screenshot shows that the entire bottom timeline area is absent, while normal combat UI controls render correctly.

Do not make a third change to the slider's width or Fill coefficient.

## 1. Bypass the broken ancestor layout

Inspect the actual saved WBP_CombatUI WidgetTree.

Locate:
- RootCanvas
- BottomTimelinePanel
- BottomTimelineContent
- SizeBox_3
- TimelineSlider
- TimelineInstantText

**Move the existing BottomTimelineContent into RootCanvas as a direct child**, bypassing its previous parent containers.

Do not create a second slider.

Give BottomTimelineContent an explicit CanvasPanelSlot:

- Anchored to bottom center of RootCanvas.
- Approximately 900 px wide and 70 px tall.
- Alignment (0.5, 1.0).
- Positioned approximately 25 px above the bottom of the gameplay viewport.
- Suitable positive Z-order so it displays above the arena and any background panels.

Ensure the existing slider remains inside BottomTimelineContent and keeps its OnValueChanged binding.

Make the timeline readable and clickable.

If this specific reparenting would break another essential child relationship, position the existing timeline widgets directly in RootCanvas instead. Preserve widget identities and event bindings.

## 2. Prove the layout is actually on screen

The saved widget hierarchy alone has repeatedly failed to predict graphical visibility.

After Slate layout is performed, inspect in a GRAPHICAL PIE world:

- RootCanvas cached size.
- BottomTimelineContent cached size.
- SizeBox_3 cached size.
- TimelineSlider cached size.
- Slider screen-space bounds.
- Effective visibility.
- Hit-testability.

The slider must have positive width and height, and its bounds must intersect the actual gameplay viewport.

If the layout is still invisible, add one temporary, high-contrast diagnostic TextBlock reading "TIMELINE HERE" directly to RootCanvas at the intended position.

If that diagnostic appears but the timeline does not, isolate the faulty widget or container.

If the diagnostic also fails, investigate RootCanvas sizing, viewport coordinates, clipping, and Z-order.

Remove the diagnostic after fixing the actual cause.

## 3. Keep the control visible without a prediction

At clean startup:

- Slider is visible but disabled.
- Text reads "No preview".
- No prediction ghosts are visible.

With a valid hypothetical prediction:

- Slider becomes enabled.
- Text displays the correct Instant.
- The thumb can be dragged.
- Cyan/orange ghosts update to the selected snapshot.

Do not hide the entire timeline panel when the prediction is empty.

## 4. Do not disturb gameplay

Preserve the working:
- Walk.
- Dash.
- Jump.
- Continue.
- Reaction pauses.
- Movement path rendering.
- Clean startup.

Do not modify BP_CombatManager, ExecuteMove, movement simulation, or the reaction rules.

Do not rewrite WBP_CombatUI.EventTick or RefreshUI as part of this layout fix.

## 5. Verification

Compile and save WBP_CombatUI.

Read back the actual CanvasPanelSlot and WidgetTree hierarchy.

Verify event bindings still reference the same TimelineSlider.

Run all BUH automation tests.

If the graphical test runner is available, launch graphical PIE and capture a screenshot demonstrating the visible timeline.

If graphical verification cannot be performed automatically, explicitly report that limitation rather than calling the visual issue resolved.

Update notes.md and todo.md with the actual measured layout cause.

Commit and push to origin/master.

Report the SHA, exact final hierarchy, and what evidence confirms the widget is on screen.

**Success: A clearly visible slider and Instant readout near the bottom center of the gameplay viewport. No more changes to width overrides without measured graphical evidence.**