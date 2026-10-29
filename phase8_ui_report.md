# Phase 8 — Tactical Combat UI Redesign Report

## Status: In Progress

The four-region layout is in place, dark semi-transparent panel backgrounds are rendering,
button colors and text hierarchy are applied, and all button click connections are verified
intact in the Blueprint event graph. Viewport-size testing revealed that the PIE viewport
is fixed at 1280×689 regardless of editor window size. The BottomTimeline slider and text
remain difficult to see with the default UE widget style.

## Layout Structure

The WBP_CombatUI widget tree uses a RootCanvas with four Border panels:

| Panel | Slot | Layout | Size |
|---|---|---|---|
| TopBar | CanvasPanelSlot_16 | Top-stretched | Full width × 60px |
| LeftPanel | CanvasPanelSlot_17 | Left-stretched | 300px × (height − 140px) |
| RightPanel | CanvasPanelSlot_18 | Right-stretched | 380px × (height − 140px) |
| BottomTimeline | CanvasPanelSlot_19 | Bottom-stretched | Full width × 80px |

All four panels use `drawAs=Box`, `imageType=FullColor`, `WhiteSquareTexture`, and
explicit tint alpha. The LeftPanelBG Image widget is hidden (`visibility=Collapsed`)
because the LeftPanel Border background is sufficient.

## Color Scheme

### Panel Backgrounds
- TopBar: `(0.07, 0.08, 0.11, 0.88)` dark blue-grey
- LeftPanel: `(0.07, 0.08, 0.11, 0.88)` dark blue-grey
- RightPanel: `(0.06, 0.07, 0.10, 0.82)` dark blue-grey
- BottomTimeline: `(0.05, 0.06, 0.09, 0.85)` dark blue-grey

### Button Colors
- Player move buttons (Quick, Heavy, Guard, Dodge): `(0.15, 0.17, 0.22, 1)` dark grey-blue
- Execute: `(0.65, 0.45, 0.15, 1)` bronze
- Reset: `(0.2, 0.2, 0.25, 1)` dark grey
- Opponent response buttons: `(0.25, 0.5, 0.4, 1)` jade

### Text Hierarchy
- Health, state, recovery, opponent move labels: size 18, warm white `(0.9, 0.9, 0.85, 1)`
- Selected move, hypothetical label: size 18/16, jade `(0.4, 0.75, 0.6, 1)`
- Predicted outcome: size 14, grey `(0.7, 0.7, 0.7, 1)`
- Distance text: size 16, grey `(0.8, 0.8, 0.8, 1)`
- Timeline instant text: size 20, warm white `(0.95, 0.95, 0.9, 1)`

## Verified Button Connections

The Blueprint EventGraph contains `OnClicked` handlers for all buttons:

- BtnQuick → `BPCombatManager.SelectQuick`
- BtnHeavy → `BPCombatManager.SelectHeavy`
- BtnGuard → `BPCombatManager.SelectGuard`
- BtnDodge → `BPCombatManager.SelectDodge`
- BtnExecute → `BPCombatManager.ExecuteMove` (guarded by `State == 0`)
- BtnReset → `BPCombatManager.ResetEncounter`
- OpponentQuickButton → timeline generation with opponent move 0
- OpponentHeavyButton → timeline generation with opponent move 1
- OpponentGuardButton → timeline generation with opponent move 2
- OpponentDodgeButton → timeline generation with opponent move 3

Keyboard shortcuts are wired in EventTick:
- One/Two/Three/Four → SelectQuick/Heavy/Guard/Dodge
- Enter → ExecuteMove (when State == 0)
- R → ResetEncounter (when State == 2)
- Left/Right → TimelineSlider adjust

## Screenshot Evidence

- `tmp_opencode/ui_test/20261029_103920_1280x720/ui_screenshot.png` — final state with
  dark panels, button colors, text hierarchy, and increased timeline text sizes.
- `tmp_opencode/ui_test/20261029_103017_1280x720/ui_screenshot.png` — layout at 1280×720
  (PIE viewport is fixed at 1280×689 regardless of editor window size).
- `tmp_opencode/ui_test/20261029_103351_1000x570/ui_screenshot.png` — layout at 1000×570
  (identical to 1280×720 because the PIE viewport does not change).
- `tmp_opencode/leftpanel_diag/20261029_101741/leftpanel_screenshot.png` — dark panels,
  button colors, and text hierarchy visible in graphical PIE.
- `tmp_opencode/leftpanel_diag/20261029_101028/leftpanel_screenshot.png` — dark panel
  backgrounds replacing red diagnostic backgrounds.
- `tmp_opencode/leftpanel_diag/20261029_100041/leftpanel_screenshot.png` — red diagnostic
  backgrounds rendering (pre-redesign baseline).

## Remaining Work

1. **Viewport-size testing limitation**: The `-ResX`/`-ResY` editor flags do not change
   the PIE viewport size. All captured screenshots are 1280×689. The UI layout uses fixed
   pixel offsets (300px left panel, 380px right panel, 60px top bar, 80px bottom bar), so
   at smaller viewports the panels would cover a larger percentage of the screen. At
   1280×689, the panels cover ~53% of the width, leaving ~47% for the 3D view. At a
   hypothetical 1000×570 viewport, the panels would cover ~68% of the width. Testing at
   true smaller viewports requires a different approach (e.g., resizing the PIE viewport
   via console command or creating a custom resolution setting).
2. **Slider visual styling**: The TimelineSlider uses the default UE widget style (thin
   grey line). The timeline text was increased to size 20 (TimelineInstantText) and size 16
   (DistanceText, PreviewDistanceText) with brighter colors, but the slider and text remain
   difficult to see in screenshots. Making the slider more visible requires a custom
   `UWidgetStyle` with thicker/brighter brushes for the track, fill, and thumb. This is a
   known limitation.
3. **BottomTimeline background**: The `brush` property on the Border widget could not be
   set via the MCP `set_properties` tool (both `brush` and `Brush` property names failed).
   The BottomTimeline background remains at the original dark tint `(0.05, 0.06, 0.09, 0.85)`.
   Making it more visible would require setting the brush through a different mechanism
   (e.g., Blueprint variable or material).
4. **Ghost preview visibility**: The 3D ghost preview fighters are visible in the center of
   the viewport, not obscured by the panel backgrounds. The panels are semi-transparent
   (alpha 0.82–0.88) and positioned at the edges, leaving the center clear for the 3D view.

## Preserved Tests

All 42 C++ combat and timeline tests remain passing. No combat mechanics were modified
during the UI redesign. The UI changes are purely visual (colors, sizes, layout) and do
not affect the combat resolver, move prediction, or timeline simulation logic.
