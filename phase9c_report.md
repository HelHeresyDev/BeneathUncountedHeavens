# Phase 9C — Final HUD Cleanup and Verification Report

## Summary
Phase 9C is functionally complete. The two blocking visual defects (overlapping TopBar text, invisible bottom timeline) are fixed and verified in graphical PIE. Combat logic is untouched by the HUD cleanup, so all menu interactions are preserved. Regressions pass. A small set of click-through interactions still needs manual confirmation (see below).

## 1. Implemented functionality (what changed)
- **TopBar restructured** (`WBP_CombatUI`): health group (`PlayerHealthText`, `OpponentHealthText`) sits on the left; a `TopBarSpacer` (`UMG.Spacer`, slot size `Fill` value 1) creates a clear gap; the status group (`StateText`, `RecoveryText`, `OpponentMoveText`) sits on the right. Padding on all TopBar slots set to `left=12 top=4 right=12 bottom=4`. `RecoveryText` is retained (it is driven by `RefreshUI` to show `Ready`/`Recovering`, not obsolete) and is a variable.
- **RootCanvas Fill coefficient restored to 1**: `RootBox.VerticalBoxSlot_0.size = {value:1, sizeRule:Fill}`. This was the root cause of the invisible timeline — a zero-height canvas clipped all bottom-anchored content. (A prior working-state fix had been lost when the WBP was reverted to resolve an `ExecuteWalk` desync.)
- **Obsolete widget removed**: `LeftPanelBG` (`Image`, `CanvasPanelSlot_0`) was referenced by no graph and was redundant with the `LeftPanel` Border's own background. Removed. Widget count 51 → 50. All other widgets (`BtnWalk`, `DestSlider`, `TimelineSlider`, `TimelineInstantText`, `DistanceText`, `PreviewDistanceText`, `BtnExecute`, `BtnReset`, all TopBar/panel texts) are confirmed used in `EventGraph`/`RefreshUI` and were preserved.

## 2. Graphical verification (PIE screenshots)
- TopBar no overlap: `tmp_opencode/visual_test/20261031_205404/fighters.png` — `P 100  O 100` left, clear gap, `Select a move  Ready` right.
- Timeline + background visible: `tmp_opencode/visual_test/20261031_211157/fighters.png` — bottom band now shows the dark `BottomTimeline` background and the distance/preview text (`D: 1000.0`, `P: preview D: ---`). The Fill=1 fix restored both the timeline content and the background band; the earlier "Border brush does not paint" diagnosis was a misread of the zero-height-canvas clipping.
- Both runs `RESULT=PASS` (no crash, both fighters visible).

## 3. Regressions
- `BUH.CombatSimTests` suite: **Success**, 0 errors, 0 failures (run via MCP `AutomationTestToolset.RunTestsByFilter`). Combat logic unchanged by the HUD edits.

## 4. Manual verification still required (click-through in graphical PIE)
Interaction automation is not available in this environment, so the following are provided as exact manual tests rather than claimed-passed. Combat logic is unchanged, so these are expected to pass, but each needs a human click to confirm:
1. **Categories**: click Attack / Defend / Move / Special — the ability list updates without executing a move.
2. **Ability selection**: click an ability (e.g. Quick Strike) — `SelectedMoveText` updates, no immediate execution.
3. **Move → Walk**: select Walk Toward / Walk Away, drag `DestSlider` — destination control updates.
4. **Execute**: click `Execute` — `ExecuteMove` resolves, fighters lunge, hit/block flash.
5. **Timeline scrubbing**: drag `TimelineSlider` — ghost movement / preview distance updates.
6. **Reaction pause + selection**: trigger an opponent threat — reaction window pauses, select a reaction.
7. **Reset**: click `Reset` (visible when `State == 2`) — encounter resets to `(±500,0,90)`.

## 5. Known limitations
- `fontSize` is not readable via `ObjectTools.get_properties` on TextBlocks (the `Font` property is readable; TopBar font is Roboto Bold size 16).
- Some `Border`/`Image` backgrounds in this WBP render unreliably under certain slot configurations; the `BottomTimeline` background now renders correctly with the Fill=1 canvas sizing.

## Constraints honored
- No Dash / Jump / new mechanics added.
- No Git commit created (working-tree changes only).
- All execution still through `ExecuteMove` / `ExecuteMoveWithVars`.
- Original xianxia styling preserved.

## Key files
- `Content/BUH/Prototypes/TacticalDuel/Blueprints/WBP_CombatUI.uasset`
- `tmp_opencode/visual_test/20261031_211157/fighters.png` (latest screenshot)
- `tmp_opencode/mcp_do.py` (MCP helper)
- `todo.md` (phase spec), `AGENTS.md`, `notes.md`
