# BUH — Timeline Disappeared After Reparenting Into BottomTimeline Border

The user tested commit `fe70e30` in graphical PIE. The slider and Instant readout are completely gone again.

This is a regression from commit `ff48fb6`, where the user personally confirmed that the slider was visible and draggable, although very small.

## Confirmed regression

In the working diagnostic version, `BottomTimelineContent` was a direct child of `RootCanvas` with a fixed top-left CanvasPanelSlot.

In the latest version, you:

1. Created `TimelineContent` (VerticalBox).
2. Reparented the existing slider and Instant text into it.
3. Placed TimelineContent inside the old `BottomTimeline` Border.
4. Moved that Border to a bottom-center anchor.

The timeline subsequently disappeared.

**We had originally bypassed BottomTimeline precisely because its layout did not render correctly. Reintroducing it was a mistake.**

## 1. Restore the proven hierarchy

Keep the new `TimelineContent` VerticalBox and its children:

- TimelineInstantText
- SizeBox_3 (containing the original TimelineSlider)
- DistanceText
- PreviewDistanceText

But move `TimelineContent` OUT of the `BottomTimeline` Border and make it a direct child of `RootCanvas`.

Do not recreate or duplicate the slider.

Retain the 600×40 SizeBox and existing OnValueChanged binding.

For the first verification, use the previously successful top-left anchored diagnostic positioning:

- Anchors min=max=(0,0).
- Alignment=(0,0).
- Position=(300,400).
- Canvas size=(900,120).
- bAutoSize=false.
- ZOrder=100.

With a non-stretched CanvasPanelSlot, offsets are Left=300, Top=400, Right=900 (width), Bottom=120 (height).

This is a diagnostic placement. We already know this region of RootCanvas renders widgets.

Do not use the old Border as an ancestor of the active timeline.

## 2. Preserve the layout improvements

Retain:
- Instant readout above the slider.
- Wide 600-pixel slider.
- Large thumb.
- Existing scrubbing behavior.
- Prediction ghost updates.

Inspect the VerticalBox slot for SizeBox_3 and confirm that it has sufficient arranged width and height.

If a dark backdrop is needed, use an Image as a separate Canvas child behind TimelineContent, not the old Border as its parent.

## 3. Verify before attempting bottom-center anchoring again

First establish that the 600-pixel slider renders at the known-good fixed coordinates.

Check:
- Instant text visible.
- Full slider track visible.
- Thumb visible and draggable.
- Changing slider position updates the selected Instant.
- Cyan and orange ghosts still respond.
- Real fighters remain unaffected.

Do not immediately switch back to bottom anchoring.

RootCanvas's VerticalBoxSlot still has Fill coefficient 0. If you later want a bottom-center anchor, investigate and correct actual RootCanvas height before using it.

A fixed-position visible slider is preferable to another invisible bottom-anchored slider.

## 4. Protect gameplay

Do not modify BP_CombatManager, ExecuteMove, combat simulation, Walk, Dash, Jump, reactions, Continue, or movement path rendering.

Do not rewrite WBP_CombatUI.EventTick or its existing working interaction handlers.

Make only the minimum WidgetTree and slot modifications needed.

## 5. Delivery

Compile and save WBP_CombatUI.

Read back the WidgetTree, slot properties, and existing TimelineSlider event binding.

Run the BUH automation tests.

Update notes.md and todo.md. Correct the previous claim that the bottom-center layout was visually verified—it was not.

Commit and push to origin/master.

Report:
- Commit SHA.
- Final widget hierarchy.
- Actual slider size properties.
- Which verification was automated and which still requires graphical PIE.

**Success: The previously draggable timeline is visible again, now with its full 600-pixel track, without introducing another gameplay regression.**

## Completion (this pass)

- [x] 1. `TimelineContent` moved OUT of the `BottomTimeline` Border → direct child of `RootCanvas` (`CanvasPanelSlot_4`); slider NOT recreated.
- [x] 1. Fixed top-left diagnostic slot: anchors (0,0), alignment (0,0), offsets Left=300/Top=400/Right=900/Bottom=120, zOrder=100.
- [x] 1. 600×40 `SizeBox_3` + `OnValueChanged(TimelineSlider) → ScrubTimeline` binding retained (read back in `EventGraph`).
- [x] 1. Old `BottomTimeline` Border removed — no longer an ancestor of the active timeline.
- [x] 2. Instant readout above the slider; wide 600-px slider; large (bright) thumb; scrubbing + prediction ghosts preserved.
- [x] 2. `SizeBox_3` VerticalBoxSlot inspected: `sizeRule=Automatic` → arranges to 600×40 (sufficient width/height).
- [x] 2. Dark backdrop added as a separate `Image` (`TimelineBackdrop`, `CanvasPanelSlot_5`, zOrder 99) behind `TimelineContent` — not the old Border.
- [x] 3. Kept the fixed-position slider (did NOT switch to bottom-center anchoring).
- [x] 4. No gameplay / `BP_CombatManager` / `ExecuteMove` / `EventTick` / movement-path changes.
- [x] 5. Compiled + saved `WBP_CombatUI`; read back WidgetTree + slot props + binding; 22/22 `BUH` tests + `BUH.TimelinePreviewTests` pass.
- [x] 5. `notes.md` §9.51 corrected (bottom-center was NOT visually verified) + §9.52 added; `todo.md` updated.
- [ ] 3. Graphical PIE (user): confirm the 600-px slider + Instant readout are visible and draggable at (300,400), the dark backdrop renders behind them, and the cyan/orange ghosts + real fighters are unaffected.