# Beneath Uncounted Heavens — Dead Opponent Buttons (RESOLVED 2026-11-04)

**Root cause confirmed:** `RightPanel`'s `CanvasPanelSlot` had coincident horizontal anchors (`Min.X=Max.X=1`) with `Right=0`, giving it **zero arranged width**. Children painted but the hit-test area was empty — visible button text with no hover/press/click.

**Fix applied:** slot offsets `Left=-380, Top=60, Right=300, Bottom=-80`; anchors `(1,0)->(1,1)` kept; `bAutoSize=false`, `Alignment=(0,0)`. Compiled + saved `WBP_CombatUI`.

**Verified (graphical PIE, `2026-11-04 14-32-42`):** hovering the Dodge button shows `DODGE HOVER` and the button highlights with a real background; all four opponent buttons now receive mouse events.

**Cleanup done:** removed the `TempDiagnosticText` widget and the 5 diagnostic event chains (OpponentDodgeButton OnHovered/OnPressed + the 3 dangling TempRightProbeButton events). Real opponent `OnClicked` handlers preserved. Compiled clean, saved. All 4 `BUH.*` regression tests pass.

See `notes.md` §9.15 (root cause + fix) and §4.3 (cross-reference). Original report below.

---

Read `chatgpt_report.md`.

The report reveals a specific likely CanvasPanel geometry bug.

`RightPanel` has:
- Horizontal anchors Min.X=1, Max.X=1
- Offsets Left=-380, Right=0
- A child SizeBox with desired width 300

Because the horizontal anchors coincide, the Right offset represents the slot's width, not its right margin.

Therefore RightPanel may have ZERO arranged width, while its children still paint outside that area.

This would explain visible button text with no hover, press, or click.

## Step 1 — Verify geometry

In graphical PIE, inspect the real RightPanel and LeftPanel geometry.

Record their arranged local sizes, especially widths, using a reliable post-layout geometry read or `UCanvasPanel::GetGeometryForSlot`.

Expected:
- LeftPanel approximately 300 units wide.
- RightPanel potentially 0 units wide.

Do not use headless -nullrhi geometry as visual proof.

## Step 2 — Apply one minimal fix

Change RightPanel's CanvasPanel slot offsets from:

Left=-380, Top=60, Right=0, Bottom=-80

to:

Left=-380, Top=60, Right=300, Bottom=-80

Keep horizontal anchors (1,0) to (1,1).

Set Auto Size=false and Alignment=(0,0).

Do not modify the right-side buttons or prediction code.

If MCP property editing fails, use an editor-side modification targeting the actual UCanvasPanelSlot:

- GetOffsets()
- Change the Right member to 300
- SetOffsets()
- SetAutoSize(false)

Compile and save the actual Widget Blueprint asset, then restart PIE.

Read back the saved slot properties.

## Step 3 — Test the button

Hover over the hypothetical Dodge button.

The existing diagnostic should display:

DODGE HOVER

Then click it.

Verify:

- DODGE PRESSED fires.
- OnClicked fires.
- HypotheticalOpponentMove becomes 3.
- The selected response label changes.

If the width fix works, apply the same verification to Quick, Heavy, and Guard.

## Step 4 — If the panel still doesn't respond

Do not immediately rewrite prediction code.

Check the actual arranged geometry and hit-test path again.

Only if RightPanel now has nonzero width and clicks still fail, investigate its z-order relative to the full-canvas LeftPanelBG Image.

Temporarily raise RightPanel's CanvasPanel ZOrder above LeftPanelBG and test again.

Change one variable at a time.

## Step 5 — Final verification

Keep the working persistent cursor.

Confirm all opponent response buttons receive mouse events.

Only after that, determine whether the existing hypothetical selection and prediction functions behave correctly.

Remove temporary diagnostics after successful verification.

Run regressions and save affected assets.

No broad graph rewrites, unrelated changes, new mechanics, or Git commits.

**Success criterion:** RightPanel has valid hit-testable geometry, and clicking the opponent buttons triggers their existing handlers.