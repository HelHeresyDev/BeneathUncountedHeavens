# BUH — Fix Hypothetical Branching in the Actual Planning UI

Current repository baseline: `9c230bc`.

The user reports that selecting a hypothetical opponent action does not stop the prediction ghost. The latest screenshot shows State 3 (Reaction opportunity), Continue selected, and an already-committed Heavy attack.

We need to distinguish two cases and verify the actual UI behavior.

## 1. Planning mode (State 0)

Reproduce:

1. Reset the encounter.
2. Select Walk with a destination far enough to remain active beyond Instant 5.
3. Scrub to Instant 5.
4. Without executing, select hypothetical Heavy.
5. Inspect the installed timeline.

Expected:

- `GenerateBranchFromBase(5, 1, ...)` succeeds.
- A reaction opportunity occurs when Heavy starts.
- The timeline ends at that reaction opportunity.
- The slider maximum becomes the final branch index.
- The cyan ghost stops at the interruption position.
- Scrubbing past that boundary is impossible.
- The authoritative fighter remains stationary.

If this fails, trace the actual saved `WBP_CombatUI` execution graph, including baseline-signature invalidation, selected slider index, branch legality, result length, slider maximum, and subsequent `RefreshUI` calls.

Do not assume the previous button-order fix solved it.

## 2. Reaction mode (State 3)

During an actual reaction, the opponent's observed Heavy is already committed.

- Do not allow the hypothetical opponent buttons to create another attack.
- Clearly identify the committed attack separately from hypothetical planning selections.
- Continue must simulate the already-committed action until the next valid decision boundary.
- Preserve the working reaction slider and ghost movement.

Do not artificially truncate Continue at the original reaction Instant: Continue means declining the interruption.

## 3. Strengthen regression coverage

The current `BUH.PlanningWalkHypotheticalBranch` test starts with a manually pre-committed Walk. Add coverage starting from a fresh planning state with normal Walk selection and a destination.

Verify the actual Blueprint event path or explicitly report that it requires graphical PIE.

Add assertions for:

- Selected branch Instant.
- Branch legality and rejection reason.
- Final `StoppedReason`.
- Installed timeline length.
- Slider maximum matching the installed timeline.
- Ghost coordinates at the final snapshot.
- Preservation of the baseline and authoritative state.

## 4. Restrictions

Do not rewrite the combat resolver, slider, movement system, or preview architecture.

Preserve working ordinary movement previews and reaction → Continue behavior.

Compile, save, and read back modified Blueprints; run all BUH tests; update notes.md and todo.md; commit and push.

Report the specific failure found, not merely that the automated tests passed.