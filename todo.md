# Beneath Uncounted Heavens — Finish and Verify Walk Ghost Preview

Review the latest `todo.md` and the graphical PIE screenshot showing the cyan ghost between the red opponent and blue player.

The ghost is now visible. Do not rebuild the system or introduce new movement mechanics.

The goal is to make Walk planning clear, reliable, and consistent with the combat simulation.

## Status (2026-11-02)
- [x] **1. Layout** — `DestSlider` moved from `RootCanvas` (was overlapping the TopBar) into `LeftPanelContent` (`VerticalBoxSlot_7`). Persistent readout uses `TechniqueDetailsText`, which `RefreshUI` does not touch, so it survives UI refreshes. Graphical initial-state capture (`20261102_124102`) confirms no slider over the TopBar.
- [x] **2. Destination vs prediction** — `DestSlider.OnValueChanged` now moves only the `DestPlayer` destination ghost (`BP_DestGhost_C_0`, yellow `MI_GhostDest`). `TimelineSlider`/Opponent buttons still move the `GhostPlayer`/`GhostOpponent` prediction ghosts. The two no longer share an actor. `BP_DestGhost` is a child of `BP_Fighter` whose ConstructionScript hardcodes `MI_GhostDest` (the parent's `bIsGhost`/`bIsDestGhost` bool getters are read-only in `write_graph_dsl`).
- [x] **3. Slider mapping** — `WorldX = Value*1000 - 500` applied in `DestSlider.OnValueChanged` and `BtnWalkMove` (the latter previously passed the raw 0..1 slider value — fixed). Distance is computed from the authoritative `PlayerX`. `BUH.WalkPreviewTests` passes.
- [x] **4. Stale ghosts** — the `DestPlayer` destination ghost is hidden in `EventConstruct`, `BtnBack`, `BtnDodge`, `BtnExecute`, and `BtnReset` (shown in `BtnWalkMove`). Verified in PIE: `BP_DestGhost_C_0` `bHidden=true` at encounter start.
- [x] **5. C++** — module compiles; `MoveGhostsToInstant` is tag-based (`GhostPlayer`/`GhostOpponent`). All four suites pass: `BUH.CombatFlowTests`, `BUH.CombatManagerIntegrationTests`, `BUH.CombatSimTests`, `BUH.WalkPreviewTests`.
- [ ] **6. Real interaction** — initial-state graphical capture PASSES (layout + fighters render). The slider-drag steps (left/center/right, distance readout, timeline scrub, cancel cleanup, execute, second Walk from the new position) **require manual verification**: MCP cannot click/drag UMG sliders and cannot unhide/move the destination ghost in a live PIE world (`ObjectTools.set_properties` silently fails on `SetActorHiddenInGame`). See manual steps below.

### Manual verification steps (graphical PIE on `Lvl_TacticalDuel`)
1. Click `Move` → `Walk`. The destination slider appears in the left panel and the yellow destination ghost shows at the slider's WorldX.
2. Drag the slider left, center, right. Confirm the yellow destination ghost moves each time and the `TechniqueDetailsText` readout (`Walk: X <PlayerX> → <WorldX> | Distance: <dist> uu`) updates.
3. Confirm the real blue fighter never moves before Execute.
4. Select a hypothetical opponent response and scrub the tactical timeline; confirm the cyan/orange prediction ghosts move independently of the yellow destination ghost.
5. Click `Back` (or Attack/Defend/Execute/Reset) and confirm the yellow destination ghost is hidden (no stale ghost).
6. Re-select Walk and Execute; then select Walk a second time and confirm the distance is computed from the fighter's new position.
7. Capture screenshots of at least two clearly different destination values.

## 1. Fix the Walk control layout

The destination slider appears to overlap the TopBar health text.

Move the Walk destination slider into the Move → Walk submenu, directly beneath the Walk option or its details.

Display a persistent, readable label:

`Walk: X -500 → 0 | Distance: 500 uu`

Use actual values.

Do not write the readout once and then have `RefreshUI` overwrite or hide it. Ensure it survives UI refreshes.

Do not reorganize the rest of the HUD.

## 2. Distinguish intended destination from predicted result

The current `DestSlider.OnValueChanged` directly moves GhostPlayer to the requested WorldX.

That is a useful destination preview, but it is not necessarily the final position produced by combat resolution.

Keep the two concepts separate:

- Destination marker/ghost: where the player intends to walk.
- Predicted combat ghost: where the simulation says the player will be at the selected Instant.

If the existing system uses the same ghost actor for both, establish an explicit display mode so the destination preview and timeline scrub cannot overwrite each other unpredictably.

Do not falsely present an interrupted Walk as guaranteed to reach the target.

Use the existing CombatSim resolver for hypothetical outcomes.

## 3. Verify the slider mapping

The slider range is 0 to 1, currently mapped to:

`WorldX = Value * 1000 - 500`

Verify that the mapping is applied consistently during selection, prediction, and execution.

Confirm that the displayed walking distance is calculated from the player's current authoritative position, not always from -500.

Keep the existing 1D movement prototype. Do not implement full 3D targeting yet.

## 4. Fix stale ghosts

When the player:

- Presses Back.
- Switches to Attack or Defend.
- Executes an action.
- Resets the encounter.
- Starts a new encounter.

The planning destination ghost should be hidden, reset, or repurposed according to the current UI mode.

Do not leave a translucent fighter at an obsolete destination.

Preserve the separate timeline-preview functionality.

## 5. Finish the C++ change

The latest report says `MoveGhostsToInstant` was changed from hardcoded actor-name matching to tag-based lookup, but the C++ rebuild was pending.

Compile the module.

Verify that `GhostPlayer` and `GhostOpponent` are found by tags in actual PIE.

Ensure the implementation does not depend on generated actor names such as `BP_Fighter_C_2`.

Run the existing four automation test suites.

## 6. Verify real interaction

In graphical PIE:

1. Select Move → Walk.
2. Move the destination slider left, center, and right.
3. Observe the player destination ghost changing position each time.
4. Confirm the real fighter never moves before Execute.
5. Verify the distance readout updates correctly.
6. Select a hypothetical opponent response.
7. Scrub the tactical timeline and check predicted ghost positions.
8. Cancel Walk and verify stale preview cleanup.
9. Select Walk again and Execute.
10. Select Walk a second time and verify distances are calculated from the new player position.

Capture screenshots of at least two clearly different destination values.

If MCP cannot perform actual slider interaction, label those checks as requiring manual verification. Do not claim success based solely on the presence of a ghost in a screenshot.

## Constraints

- Preserve blue and red real fighters.
- Preserve translucent team-colored ghost materials.
- No Dash or Jump.
- No new combat mechanics.
- No unrelated HUD redesign.
- No Git commit without authorization.

**Success criterion:** While planning Walk, the player can immediately see and read their selected destination and distance, compare it with predicted combat movement, and cancel or execute without leaving stale ghosts or modifying live combat state prematurely.