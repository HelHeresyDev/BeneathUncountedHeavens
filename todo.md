 # BUH — Timeline Slider Is Invisible in Graphical PIE

 **Status (post-fix):** Inspected the full UMG hierarchy — the slider's own state was fine (`Visible`, opacity 1, `Value 0 / Min 0 / Max 30`, horizontal) and `RootCanvas` was **not** zero-height (panels render), so the §4.2 bug is not active. The slider's `HorizontalBoxSlot` was `Fill coeff 0` with no explicit size, so it rendered at zero width. Wrapped `TimelineSlider` in a `SizeBox` (`SizeBox_2`, `WidthOverride=550`, `HeightOverride=40`) via `UMGToolSet.WrapWidgets` + `ObjectTools.set_properties`, matching the existing Left/Right panel convention. Rewrote `ScrubTimeline` to guard on `CombatSim|bHasTimeline` (not `GetTimelineLength<=0`), so an empty timeline shows "No preview" instead of the garbage `0/134743044`. Compile clean, 22/22 `BUH` tests pass, WBP saved. **Graphical PIE drag verification still pending** (headless cannot capture the viewport or drive a drag) — the user must confirm the slider is visible and draggable in a fresh graphical PIE. See notes.md §9.46.

 The user tested commit 3ae8de0. The slider is NOT visible anywhere in graphical PIE.

The latest commit changed ScrubTimeline, OnValueChanged, RefreshUI, and RegeneratePredictionPreview, but it did not verify the widget's visual layout.

The screenshot shows player/opponent controls and faint prediction text, but no visible slider track or draggable thumb.

## 1. Inspect the actual UMG widget hierarchy

Open WBP_CombatUI and inspect:

- TimelineSlider
- TimelineInstantText
- BottomTimelineContent
- BottomTimelinePanel and its ancestors, if present
- RootCanvas
- RootBox

Use UMGToolSet.GetWidgetDescription and available ObjectTools properties.

For the slider and each ancestor, report:
- Widget class and parent
- Visibility
- Render opacity
- IsEnabled
- Slot type
- Anchors, offsets, alignment
- Desired dimensions
- SizeBox or container constraints
- Clipping
- Z-order where relevant

Review notes.md §4.2: this project previously had a zero-height RootCanvas issue that hid the entire bottom timeline.

Also check whether the bottom timeline is positioned outside the viewport or behind another widget.

A widget existing in the WidgetTree does not prove that it is visible or usable.

## 2. Repair the layout, not the simulation

Make the existing TimelineSlider clearly visible near the bottom center of the gameplay viewport.

Suggested first-playable presentation:
- Slider track approximately 500–650 px wide.
- Adequate clickable height.
- Clearly visible thumb.
- Distinct contrast against the arena.
- Readable "Instant N / N" display beside or above it.
- No overlap with the Execute button.
- No overlap with the opponent-action buttons.

Use correct slot geometry and anchors so the slider remains visible when the PIE viewport is resized.

Ensure the widget is enabled and hit-testable.

Preserve the existing player and opponent menu layouts as much as possible.

Do not create another slider if TimelineSlider already exists.

## 3. Check the displayed Instant value

The screenshot contains a suspicious prediction readout resembling "0/134743044".

Verify:
- Actual GetTimelineLength.
- Actual selected snapshot index.
- TimelineSlider MinValue and MaxValue.
- TimelineInstantText's final displayed value.
- Whether another Blueprint handler overwrites that text.

For N snapshots, valid indices are 0 through N-1.

Do not display an uninitialized or invalid timeline length.

An empty timeline should display "No preview".

## 4. Verify real scrubbing

Once visible:

1. Select Quick.
2. Select hypothetical opponent Heavy.
3. Confirm the timeline has multiple snapshots.
4. Drag the slider to the first, middle, and final snapshots.
5. Confirm the cyan and orange ghosts change positions appropriately.
6. Confirm the real blue and red fighters remain stationary.
7. Confirm the displayed Instant matches the snapshot.
8. Confirm the thumb is actually draggable and not obstructed by an overlapping widget.

For Jump predictions, verify the cyan ghost's airborne Z.

If the available headless MCP environment cannot verify widget geometry or mouse dragging, explicitly report that limitation. Do not call the feature visually verified until graphical PIE confirms it.

## 5. Preserve the stable combat system

Do not modify:
- BP_CombatManager.ExecuteMove
- Walk, Dash, or Jump simulation
- Reaction/Continue logic
- ResetEncounter
- Combat balance
- Movement path renderers

Do not rewrite WBP_CombatUI.EventTick or RefreshUI merely to solve widget geometry.

Prefer a small, targeted WidgetTree or slot-property correction.

Compile and save WBP_CombatUI, then read back the corrected widget properties.

Run all BUH automation tests.

Update notes.md and todo.md with the actual visibility cause.

Commit and push to origin/master and report the SHA.

**Success:** The slider is unmistakably visible and draggable at the bottom of graphical PIE, shows a valid Instant number, and moves prediction ghosts without disturbing real combat.