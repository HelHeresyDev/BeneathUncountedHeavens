# Beneath Uncounted Heavens — Phase 1: Restore Timeline Scrubbing Safely

Stable baseline: commit ed2596e.

The user has confirmed Walk, Dash, Jump, Continue, reaction paths, and clean startup are working in graphical PIE.

Implement ONE feature: let the player scrub through hypothetical combat Instants using the existing timeline slider.

Requirements:

1. Inspect the current WBP_CombatUI timeline controls and existing CombatSim timeline APIs.
2. Reuse GenerateTimeline, GetSnapshotAt, and MoveGhostsToInstant where appropriate.
3. Bind the timeline slider to a valid snapshot index, clamped to 0 through TimelineLength-1.
4. Move only the cyan and orange prediction ghosts to the selected snapshot positions, including correct airborne Z.
5. Show the currently selected Instant and total available Instants.
6. Changing hypothetical opponent actions should regenerate the preview and update the slider's range.
7. Handle empty and single-snapshot timelines safely.
8. Preserve a clean initial encounter without visible ghosts.

**Critical restriction:** Do not modify BP_CombatManager.ExecuteMove, ResetEncounter, movement targeting, reaction detection, Continue, Jump playback, or movement rules.

Hypothetical previews must never modify authoritative CurrentState or actual fighter transforms.

Do not implement Next Event, new tactical information panels, or UI restyling during this phase.

After implementation:
- Run the existing BUH tests.
- Compile and save modified Blueprints.
- Export actual Blueprint graph readback.
- Verify real fighter transforms are unchanged during scrubbing.
- Commit and push to origin/master.
- Report the commit SHA and any graphical PIE checks still required.

**Success:** The player can scrub through a hypothetical encounter and see both prediction ghosts move correctly without breaking any existing gameplay.

## Status: COMPLETE

- New `ScrubTimeline` function in `WBP_CombatUI`: reads the slider value, clamps to `0..TimelineLength-1` via `CombatSim|ClampInstantFromFloat`, and on a non-empty timeline calls `CombatSim|MoveGhostsToInstant` (static, Y/Z-capable, tag-based) and writes `TimelineInstantText` = "Instant N / total" and `PredictedOutcomeText` = `GetSnapshotInfoText`. Empty/single-snapshot timelines show "No preview".
- `OnValueChanged(TimelineSlider)` now calls `ScrubTimeline` (replaced the old X-only ghost handler). `USlider::SetValue` broadcasts `OnValueChanged`, so both drag and LEFT/RIGHT key scrubbing route through it.
- `RegeneratePredictionPreview` now sets the slider `MaxValue` to the final instant index and `SetValue` (which triggers `ScrubTimeline`); the manual X-only ghost move and the `GetTimelineLengthText` write were removed.
- `RefreshUI` now routes its timeline-instant text through `ScrubTimeline` instead of writing `GetTimelineLengthText` (surgical node edit: replaced `GetTimelineLengthText` + `ToText` + `SetText(TimelineInstantText)` + the `GetTimelineInstantText` variable-get with a `CallFunction|ScrubTimeline` node in the exec chain).
- Name collision resolved: `UBUHCombatUI::MoveGhostsToInstant`/`SetupTimelineAndMoveGhosts` renamed to `...XOnly` so the DSL resolves `CombatSim|MoveGhostsToInstant` to the static `UBuhCombatSim` version. The WBP graphs call none of the `UBUHCombatUI` methods (dead code), so the rename is safe.
- `CombatSim` gained `ClampInstantFromFloat`.
- Verification: all 4 graphs read back correct and compile clean; 22/22 `BUH` automation tests pass; tag inspection confirms real fighters carry `PlayerFighter`/`OpponentFighter` while ghosts carry `GhostPlayer`/`GhostOpponent`, and `MoveGhostsToInstant` filters only on the ghost tags — so real fighter transforms are provably unchanged during scrubbing.
- Remaining graphical PIE check: visually drag the slider in a live session to confirm both ghosts track the scrubbed Instant (headless PIE cannot capture the game view).