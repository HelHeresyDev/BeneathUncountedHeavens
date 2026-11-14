# BUH — Enlarge Working Timeline Slider and Clean Up UI

The user has confirmed that the timeline slider WORKS. They can drag it left and right.

The problem is purely visual: the slider is tiny and difficult to see.

**Do not troubleshoot or rewrite the slider's working functionality.**

## Required changes

- [x] 1. Preserve the existing TimelineSlider, OnValueChanged binding, and prediction logic. (binding + prediction logic untouched; `BUH.TimelinePreviewTests` passes)
- [x] 2. Move the timeline interface to the bottom center of the gameplay viewport. (`BottomTimeline` Border slot = bottom-center 640×90)
- [x] 3. Give the slider approximately:
  - [x] 600 px width. (`SizeBox_3` WidthOverride=600)
  - [x] 30–40 px interactive height. (HeightOverride=40)
  - [x] A clearly visible track. (engine-default bar, now in a sized container on a dark background)
  - [x] A large, easily grabbed thumb. (bright warm `Box` thumb brush, 40 px-tall container)
  - [x] Sufficient contrast against the background. (dark `BottomTimeline` background)
- [x] 4. Put the Instant readout directly above the track. (`TimelineInstantText` is the first child of `TimelineContent`, above the slider)
- [x] 5. Use a simple layout with the Instant text above the slider. (`TimelineContent` VerticalBox stacks Instant text / slider / distance texts)
- [x] 6. Add a subtle dark translucent background behind the timeline controls. (`BottomTimeline` Border dark background, now bottom-center)
- [x] 7. Preserve the currently working slider behavior; adjust bar/thumb styling only. (value logic untouched; only the thumb brush was restyled)

## Cleanup

Remove the temporary diagnostic elements:

- [x] TIMELINE LAYOUT TEST (`DiagTimelineTest` removed)
- [x] KNOWN-GOOD CONTAINER (`DiagKnownGood` removed)
- [x] GEOMETRY DEBUG (`DiagGeometryReadout` removed)
- [x] Temporary geometry-update calls (`UpdateGeometryDebug` graph + `EventTick` call removed; C++ `GetTimelineGeometryDebug()` left as dead code — C++ edits out of scope for this pass)

Remove the diagnostic UI without deleting the functional timeline. [x] Done — functional `TimelineSlider` + binding preserved.

## Restrictions

Do not modify:
- Combat simulation.
- BP_CombatManager.
- ExecuteMove.
- Walk, Dash, Jump, or Continue.
- Reaction behavior.
- Prediction generation or snapshot indexing.

Make targeted WidgetTree, slot geometry, and styling changes only.

## Verification

- [x] Slider is visibly wide and centered near the bottom. (geometry read back: bottom-center 640×90, `SizeBox_3` 600×40)
- [x] Dragging left/right still changes the selected Instant. (`BUH.TimelinePreviewTests` + combat-flow tests pass)
- [x] Prediction ghosts still respond correctly. (prediction tests pass)
- [x] Real fighters remain unaffected. (combat tests pass)
- [x] Initial encounter remains clean. (fresh-encounter tests pass)
- [x] Existing combat controls still work. (22/22 `BUH` tests pass)
- [ ] Graphical PIE drag check (user): confirm the enlarged bottom-center slider is clearly visible and still scrubs the Instant, and that the `Instant N / N` readout + dark background render as intended.

Compile and save WBP_CombatUI [x], run the BUH tests [x], and commit/push to origin/master [x].

Report the commit SHA and any outstanding graphical checks.

**Success: A large, clearly visible timeline slider that retains all existing functionality, with the temporary diagnostics removed.**