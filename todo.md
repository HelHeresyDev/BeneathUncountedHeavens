# Beneath Uncounted Heavens — Tactical Duel Prototype

## Current phase: Phase 5 — Combat UI + prediction preview

### Status: Continue/reaction prediction preview — logic fixes complete, graphical PIE verification pending

The ChatGPT review identified three concrete gaps behind the invisible prediction ghosts and the stale distance readout. All three are now fixed on disk, compiled, saved, and covered by the 25-test BUH automation suite.

- [x] **Fix ghost visibility** — `RefreshUI` now shows `GhostPlayer`/`GhostOpponent` when `HypotheticalOpponentMove >= 0` OR (`State == 3` AND `bHasTimeline()`). The old rule hid the ghosts during a reaction because `HypotheticalOpponentMove` stays `-1`.
- [x] **Fix world selection** — `MoveGhostsToInstant`, `MoveGhostsToFinalInstant`, and `SetupTimelineAndMoveGhosts` take `UObject* WorldContextObject`; `ScrubTimeline` passes `self` so the ghosts are moved in the same PIE world as `WBP_CombatUI`.
- [x] **Fix distance synchronization** — `ScrubTimeline` updates `PreviewDistanceText` from `GetPreviewDistanceText(currentInstant)`; `RefreshUI` reads the current slider instant instead of `0`.
- [x] Compile `WBP_CombatUI` clean, read back the graphs, and save the asset.
- [x] Run all 25 `BUH` automation tests — 25 passed, 0 failed.
- [ ] **Verify in graphical PIE** — scrub to Instant 17 and confirm the cyan prediction ghost moves near `(-2, -111)` while the real fighter stays stationary, and that `PreviewDistanceText` matches the snapshot.

## Verification evidence

- `compile_blueprint` clean on `/Game/BUH/Prototypes/TacticalDuel/Blueprints/WBP_CombatUI.WBP_CombatUI`.
- `read_graph_dsl` read-back of `RefreshUI` and `ScrubTimeline` confirms the new visibility condition, the `self` world-context argument, and the `PreviewDistanceText` update.
- `AutomationTestToolset.RunTests` over the explicit 25-name `BUH` array: **25 passed, 0 failed**.
- `save_assets` on the WBP package succeeded.
- `notes.md` §9.59 records the problem, cause, fix, and lesson.

## Next step

Ask the user for a fresh graphical PIE on `Lvl_TacticalDuel`: select Heavy, wait for the reaction opportunity, click Continue, then scrub the timeline to Instant 17 and confirm the prediction ghosts and distance readout are visible and correct.
