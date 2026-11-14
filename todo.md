# BUH — Found Exact Cause of Invisible Timeline Slider

## Status (this pass)
- **Root cause accepted:** for a non-stretched `CanvasPanelSlot` (min=max anchors), `FMargin` offsets are `Left`=X, `Top`=Y, `Right`=WIDTH, `Bottom`=HEIGHT. The §9.48 `bottom=-25` assigned a **negative height** → invisible.
- **Fix applied:** `CanvasPanelSlot_1` re-set via whole-object `layoutData` — anchors min=max `(0.5,1.0)`, alignment `(0.5,1.0)`, offsets `left=0, top=-25, right=900, bottom=70`, `bAutoSize=false`, `zOrder=10` → 900×70 panel, centered, 25 px above the canvas bottom. No second slider, no hierarchy rebuild.
- **Verified:** slot readback returns the exact corrected `layoutData`; visibility chain clean (`BottomTimelineContent`/`SizeBox_3` = `SelfHitTestInvisible`, `TimelineSlider` = `Visible`, none `Collapsed`/`Hidden`); `SizeBox_3` inside `BottomTimelineContent`, `TimelineSlider` inside `SizeBox_3`; `OnValueChanged(TimelineSlider) → ScrubTimeline` binding intact; `compile_blueprint` clean; 22/22 `BUH` tests pass; WBP saved.
- **Limitation (reported, not resolved):** the native `GetPosition`/`GetSize`/`GetAutoSize`/`GetAnchors` getters are non-UFUNCTION C++ virtuals with no MCP method-invoker, so they were not called directly; geometry verified via the `layoutData` readback under the documented non-stretched semantics. Final on-screen proof requires the user's fresh graphical PIE.
- Recorded in `notes.md` §9.49 (with a correction note added to §9.48).

The user tested commit 512a9e3. The timeline remains completely invisible.

I've identified a concrete error in the CanvasPanelSlot configuration.

## Root cause

You configured `BottomTimelineContent` with:

- Anchors min=max=(0.5, 1.0)
- Left=-450
- Top=-95
- Right=450
- Bottom=-25
- Alignment=(0.5, 1.0)
- bAutoSize=false

You described these offsets as producing a 900×70 panel.

That is incorrect for a non-stretched CanvasPanelSlot.

When the min/max anchors are identical, FMargin offsets represent:

- Left = position X
- Top = position Y
- Right = WIDTH
- Bottom = HEIGHT

You accidentally assigned the panel a NEGATIVE height of -25.

This explains why it remains invisible even after bypassing the old parent container.

## Exact fix

Retain the existing `BottomTimelineContent` as a direct child of `RootCanvas`.

Configure `CanvasPanelSlot_1` as follows:

- Anchors min=(0.5, 1.0), max=(0.5, 1.0)
- Alignment=(0.5, 1.0)
- Offsets.Left=0
- Offsets.Top=-25
- Offsets.Right=900
- Offsets.Bottom=70
- bAutoSize=false
- ZOrder=10

This produces a 900×70 panel, centered horizontally and positioned 25 pixels above the canvas bottom.

Set the complete `layoutData` object using ObjectTools if individual offset fields cannot be modified.

## Verify immediately

After saving, use the actual CanvasPanelSlot getters:

- GetPosition should return (0, -25).
- GetSize should return (900, 70).
- GetAutoSize should return false.
- GetAnchors should return the bottom-center fixed anchor.

Do not infer dimensions by subtracting offsets.

Verify:
- `BottomTimelineContent` has positive dimensions.
- `SizeBox_3` remains inside it.
- The original `TimelineSlider` remains inside `SizeBox_3`.
- Slider OnValueChanged binding is intact.
- Visibility is not Collapsed or Hidden anywhere along the chain.

Compile and save WBP_CombatUI.

If the slider still cannot appear with this corrected layout, capture actual runtime Slate geometry and investigate effective visibility. Do not guess another set of offsets.

## Preserve gameplay

No modifications to:
- BP_CombatManager
- ExecuteMove
- Movement mechanics
- Reaction/Continue
- CombatSim
- The existing slider event logic

Do not add another slider or rebuild the widget hierarchy.

Run the BUH tests, update notes.md and todo.md with the corrected offset semantics, then commit and push to origin/master.

Report the commit SHA and actual CanvasPanelSlot.GetSize result.

**Success: The timeline control becomes visible near the bottom center of graphical PIE.**