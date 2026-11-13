# Beneath Uncounted Heavens — Restore Known-Good Movement First

The user tested commit `314b27c`. Movement is STILL broken.

Stop making speculative fixes to individual reaction flags.

We have a known-good baseline:

`f53bfc6`

The user personally confirmed that this version had working Walk, Dash, Jump, Continue, and clean startup.

The regression began after the Tactical Timeline Preview implementation at:

`2b155ac`

## Status (completed)

- [x] §1 Inspected `BUH.PIE.WalkContinueReaction`; confirmed it built a separate `MakeState` and did not read the manager's authoritative fields.
- [x] §2 Restored ONLY `BP_CombatManager.uasset` and `WBP_CombatUI.uasset` from `f53bfc6` (backup branch `backup/timeline-preview-314b27c` preserves the pre-restoration state).
- [x] §3 Restarted the editor, compiled both Blueprints, ran all 24 `BUH` tests (24 passed, 0 failed), and verified the saved graphs match the known-good implementations.
- [x] §4 Redesigned the test to read the real manager properties after `ExecuteMove`. Evidence showed a headless limitation: the automation controller ends PIE at test start and Blueprint variable writes do not land in the editor world, so PIE-world manager state and `BtnContinueWalk` visibility are not observable from a headless automation test. The test now proves the three UI functions are callable, logs the observed editor-world values, and reports the limitation explicitly. Walk-vs-Heavy reaction-at-instant-0 is proven by `BUH.CombatFlowTests` / `CombatSimAutomation` Step L; the Continue button is verified by the graphical PIE playtest.
- [x] §5 Committed and pushed the targeted restoration plus the limitation-reporting test.
- [ ] §6 Reintroduce timeline features incrementally — pending user confirmation that movement works again in graphical PIE.

## 1. The current integration test is insufficient

Inspect `BUH.PIE.WalkContinueReaction` in TimelinePreviewAutomation.cpp.

It invokes ResetEncounter, SelectWalkToward, and ExecuteMove, but then builds a separate MakeState and tests ResolveCombat and GetReactionInfo directly.

It does not verify that the actual BP_CombatManager's authoritative fields contain the expected result.

It also explicitly sets bHasCurrentState=false before execution, possibly bypassing an initialization problem introduced by InitCurrentState.

Therefore, its passing result does NOT establish that movement or Continue works in graphical PIE.

Do not describe this as a successful end-to-end gameplay test.

## 2. Restore the known-good Blueprints

First, ensure the current changes are safely committed or otherwise preserved. Do not discard uncommitted work.

Create a backup branch pointing at the latest committed implementation.

Close Unreal Editor completely before restoring binary Blueprint assets.

Restore ONLY these two assets from f53bfc6:

```bash
git restore --source=f53bfc6 -- \
  Content/BUH/Prototypes/TacticalDuel/Blueprints/BP_CombatManager.uasset \
  Content/BUH/Prototypes/TacticalDuel/Blueprints/WBP_CombatUI.uasset
```

Do not revert the entire repository.

Preserve:
- New C++ prediction functions.
- Timeline tests.
- Existing movement simulation.
- Jump physics and reaction detection.
- notes.md and design documentation.

It is acceptable for the newly added Next Event widget to disappear temporarily. Restoring gameplay takes priority.

## 3. Verify the restored baseline

Restart Unreal Editor so the restored assets are loaded from disk.

Compile both Blueprints.

Run all existing BUH automation tests.

Verify that the saved Blueprint graphs correspond to the restored known-good implementations.

The new C++ timeline helper added by commit 2b155ac should remain available, but it must not alter authoritative gameplay.

If the restored assets have compatibility issues with the current C++ module, investigate those directly rather than rebuilding the UI.

## 4. Add meaningful regression checks

Do not construct a separate MakeState after invoking the Blueprint and treat that as verification.

For an actual PIE-world combat manager, inspect its properties after ExecuteMove:

- State
- PlayerMove
- bHasCurrentState
- bReactionOpportunity
- ReactionFighter
- CurrentState
- CurrentState.Fighters[0].Action
- CurrentState.Fighters[0].WalkTarget
- Actual player actor location

Where possible, inspect the real WBP_CombatUI instance and BtnContinueWalk visibility.

If headless testing cannot reliably observe Blueprint state changes, report that limitation rather than replacing the assertions with unrelated direct C++ tests.

The user's graphical PIE testing remains the decisive verification.

## 5. Commit and push the restoration

Commit the targeted Blueprint restoration and push to origin/master.

Report:
- Commit SHA.
- Exact assets restored.
- Tests passed.
- Any compatibility issues.
- Which timeline-preview features are temporarily unavailable.

Do not reintroduce the timeline Blueprint changes in the same commit.

## 6. Reintroduce timeline features incrementally afterward

Only after the user confirms movement works again:

1. Preserve the functioning BP_CombatManager execution and reaction graphs.
2. Add timeline UI behavior to WBP_CombatUI without changing movement selection or Continue.
3. Test Walk, Dash, Jump, and Continue after each meaningful Blueprint change.
4. Reintroduce Next Event only after the slider itself is verified.
5. Keep hypothetical predictions strictly read-only relative to authoritative combat state.

Do not run another broad write_graph_dsl rewrite of working EventGraphs.

## Success criteria

First milestone: The restored project behaves exactly as it did at f53bfc6, with Walk, Dash, Jump, Continue, and startup working.

Second milestone, undertaken separately: The timeline controls work without changing that behavior.

**Priority: restore the known-good combat system instead of layering another unverified repair onto the broken version.**