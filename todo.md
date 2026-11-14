# BUH — Invisible Slider: Diagnose RootCanvas Geometry, Stop Guessing

The user tested commit fcb577e in graphical PIE.

**Still no timeline slider, track, thumb, or Instant readout.**

Previous fixes have all failed visually:
1. SizeBox width/height override.
2. Replacing zero-weight Fill with Auto.
3. Moving BottomTimelineContent directly under RootCanvas.
4. Correcting CanvasPanelSlot offsets to positive width/height.

All compiled and passed automation. None produced a visible slider.

Do not make another speculative layout adjustment.

## 1. Investigate the historical RootCanvas problem

Read `notes.md` §4.2.

RootCanvas is inside RootBox (VerticalBox), and its VerticalBoxSlot previously had:

`SizeRule=Fill, Value=0`

This historically caused zero-height layout.

The notes later assumed that a single Fill child receives full height regardless of its coefficient.

That assumption is NOT verified for the current graphical layout.

Inspect the actual saved RootCanvas slot and determine whether it still has Fill coefficient 0.

Do not infer the root's size from the fact that top-anchored buttons are visible.

## 2. Perform a controlled A/B layout test

Temporarily move the EXISTING BottomTimelineContent to an unambiguous fixed location inside RootCanvas:

- Anchors min=max=(0,0)
- Alignment=(0,0)
- Position=(300,400)
- Size=(900,70)
- ZOrder=100

Do not modify the slider or its inner layout.

This is a DIAGNOSTIC placement, not the final layout.

Now test in graphical PIE.

If the timeline appears at (300,400) but not bottom-center, investigate RootCanvas's actual height and bottom-anchor geometry.

If it remains invisible, the problem is not simply bottom anchoring.

Do not keep adjusting offsets without answering this test.

## 3. Add an independent visible control

For additional diagnosis, create a temporary brightly colored TextBlock directly under RootCanvas at:

- Position=(300,350)
- Size=(500,50)
- Text="TIMELINE LAYOUT TEST"
- ZOrder=101
- Visibility=Visible
- RenderOpacity=1

This must be a direct child, with no SizeBox or HorizontalBox intermediary.

Also place a second diagnostic label inside an EXISTING VISIBLE container, such as the parent of the working opponent buttons.

This gives us a known-good reference.

Interpret the result:

- Both labels visible, timeline missing: inspect BottomTimelineContent and its children.
- Only the known-good-container label visible: investigate RootCanvas arrangement, clipping, or the new direct-child slots.
- Both labels and timeline visible at fixed coordinates: investigate bottom anchoring and RootCanvas height.
- Neither label visible: verify the running WBP class is the saved class with the changes and diagnose why the reference label failed.

## 4. Measure actual geometry in graphical PIE

Stop treating serialized WidgetTree properties as proof of on-screen layout.

Use a temporary diagnostic C++ function, Blueprint instrumentation, or another supported runtime method to log the following AFTER Slate performs layout:

- RootBox actual size
- RootCanvas actual size
- BottomTimelineContent actual size
- SizeBox_3 actual size
- TimelineSlider actual size
- Each widget's absolute screen position
- Effective visibility and render opacity
- Whether each widget is inside the viewport

Use `GetCachedGeometry()` in a real graphical PIE world after the first layout pass.

Do not measure in `-nullrhi`, where cached geometry is unreliable.

If a graphical test cannot be automated, make the measurements visible through a temporary on-screen debug readout so the user can provide one screenshot.

## 5. Find the actual cause before repairing

Use the measurements and diagnostic labels to determine whether the failure is:

- RootCanvas having zero or incorrect height.
- BottomTimelineContent being positioned outside the viewport.
- An ancestor or child receiving zero arranged size.
- Clipping or another widget obscuring the timeline.
- A slider style/rendering problem.
- An incorrect or stale runtime widget instance.

Fix only the confirmed cause.

If RootCanvas has zero height because of the Fill coefficient, correct its VerticalBoxSlot to Fill=1 and verify the runtime geometry.

If RootCanvas has normal height, do not touch that slot.

Once the issue is fixed, position the existing slider at bottom center and remove all diagnostic labels.

## 6. Protect gameplay

Do not modify BP_CombatManager, ExecuteMove, movement, reactions, Continue, simulation, or prediction logic.

Do not rebuild WBP_CombatUI or rewrite its EventGraph.

Preserve the existing TimelineSlider and OnValueChanged binding.

Compile and save, run the BUH suites, and read back the actual modified widgets.

Update notes.md to distinguish confirmed causes from failed hypotheses.

Commit and push to origin/master.

Report the commit SHA and the runtime geometry findings.

**Success: We identify WHY the timeline is invisible, fix that exact cause, and display the existing draggable slider without affecting combat.**

## Status (diagnostic setup complete — awaiting graphical PIE)

- **§1 Done:** `RootCanvas` `VerticalBoxSlot_0` confirmed still `SizeRule=Fill, Value=0`. The "single Fill child gets full height" assumption is treated as unverified; top-anchored buttons are not taken as proof of non-zero height.
- **§2 Done:** `BottomTimelineContent` `CanvasPanelSlot_1` set to the diagnostic fixed position — anchors `minimum`=`maximum`=`(0,0)`, alignment `(0,0)`, offsets `left=300, top=400, right=900, bottom=70` (900×70 at `(300,400)`), `bAutoSize=false`, `zOrder=100`.
- **§3 Done:** `DiagTimelineTest` TextBlock directly under `RootCanvas` (`CanvasPanelSlot_2`, `(300,350)`, 500×50, `zOrder=101`, yellow, Visible, opacity 1, "TIMELINE LAYOUT TEST"); `DiagKnownGood` TextBlock inside the known-good `RightPanelContent` container (`VerticalBoxSlot_6`, cyan, Visible, opacity 1, "KNOWN-GOOD CONTAINER").
- **§4 Done:** C++ `UBUHCombatUI::GetTimelineGeometryDebug()` (BlueprintCallable, Diagnostic) reports `GetCachedGeometry()` size + absolute position + visibility(int) + RenderOpacity for `RootBox`, `RootCanvas`, `BottomTimelineContent`, `SizeBox_3`, `TimelineSlider`. `UpdateGeometryDebug` function graph (CastToBUHCombatUI → GetTimelineGeometryDebug → SetText(Text) on the `DiagGeometryReadout` variable) is called from `EventTick` (full pre-existing body restored intact). `DiagGeometryReadout` is an amber AutoWrap TextBlock at `(10,10)`, 700×220, `zOrder=200`, exposed as a variable.
- **§6 Done:** No combat/manager/ExecuteMove changes; `TimelineSlider` + `OnValueChanged → ScrubTimeline` binding untouched. WBP compiled clean, saved, 22/22 `BUH` tests pass, all five widgets read back present. `notes.md` §9.50 records confirmed findings vs failed hypotheses + the new DSL/type gotchas.
- **§5 PENDING (needs the user's fresh graphical PIE screenshot):** read the three on-screen elements + the `DiagGeometryReadout` values, apply the §3 outcome table, identify the confirmed cause, fix only that cause (set `VerticalBoxSlot_0` to `Fill Value=1` only if zero-height is confirmed), restore the slider to bottom-center, remove all diagnostics, re-verify, then commit/push and report the SHA + runtime geometry.