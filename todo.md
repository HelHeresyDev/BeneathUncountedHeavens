# BUH — Restore Ordinary Movement Prediction

The user confirmed that reaction → Continue previews work, but ordinary Walk planning does not visibly show the prediction ghost.

I investigated commit `20cd80e` and found two issues to address.

## 1. Fix planning ghost visibility

In `WBP_CombatUI.RefreshUI`, the existing ghost visibility rule requires:

`HypotheticalOpponentMove >= 0 OR (State == 3 AND bHasTimeline())`

This excludes ordinary planning (State 0) with no hypothetical opponent selection.

Correct the rule so prediction ghosts are visible whenever there is a valid preview for the currently selected player action, including ordinary Walk, Dash, and Jump in planning state.

Do not require a hypothetical opponent action just to display the baseline movement prediction.

Keep ghosts hidden when no valid player action/preview exists, after Reset, and during actual execution.

## 2. Prevent stale planning baselines

Inspect `RegeneratePredictionPreview` and the movement-selection and destination-change handlers.

The existing `GetBaseTimelineLength == 0` condition only detects a missing baseline, not an outdated one.

Ensure a fresh planning baseline is generated when the selected action, destination, or authoritative starting state changes.

Preserve the existing baseline while scrubbing or selecting hypothetical opponent branches. Do not regenerate the timeline simply because the slider moves.

## 3. Preserve working functionality

Do not change:
- The combat resolver or movement speeds.
- The reaction → Continue preview.
- The opponent decision-boundary behavior.
- The slider geometry or ghost materials.
- Authoritative combat execution.

## 4. Verification

Test both planning and reaction pathways.

Planning: select Walk, choose a destination, and scrub the prediction without selecting a hypothetical opponent action. The cyan ghost must move to the predicted intermediate positions.

Change the Walk destination and confirm the timeline is rebuilt using the new destination, rather than showing stale snapshots.

Confirm equivalent preview visibility for Dash and Jump.

Reaction: Walk → Execute → Heavy reaction → Continue must still generate the working preview, stopping at the opponent's next decision boundary.

Read back the actual saved Blueprint graphs, run the BUH automated tests, save the assets, and report what still requires graphical PIE verification.

Update notes.md and todo.md, then commit and push.

## Completion record (headless + automated)

- [x] `RefreshUI` ghost visibility now uses `(State == 0 OR State == 3) AND bHasTimeline`, so ordinary Walk/Dash/Jump planning shows ghosts without a hypothetical opponent selection.
- [x] `RegeneratePredictionPreview` detects stale baselines with a 13-value signature (action, positions, health, seed, PlayerY, Walk/Dash/Jump target X/Y) and regenerates only when it changes or the timeline is empty.
- [x] `RegeneratePredictionPreview` sets the C++ Walk preview target from manager variables and wraps planning states with `WithPlayerWalk/Dash/JumpTargetFromPreview`.
- [x] `ClearPendingMovementPreview` calls `CombatSim|ClearPreview` and `SetBasePlayerMove -1` to invalidate the stale-baseline signature.
- [x] All seven `BP_CombatManager` select functions call `RegeneratePredictionPreview` before `RefreshUI` (Jump via full rewrite; the other six via surgical `create_node`/`connect_pins` insertion because `write_graph_dsl` cannot re-emit `b`-prefixed bool setters).
- [x] `WBP_CombatUI` and `BP_CombatManager` compiled clean, read back, and saved.
- [x] All 26 `BUH.*` automation tests pass (0 failures).
- [ ] Graphical PIE verification: Walk ghost moves to predicted positions, changing the Walk destination rebuilds the timeline, Dash/Jump show equivalent previews, and reaction → Continue still stops at the opponent's decision boundary.