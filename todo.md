# BUH — Restore Opponent Prediction Controls After Reaction Ghost Update

Current baseline: commit `edca825`.

The user reports losing the ability to predict enemy actions following implementation of the separate gold reaction ghost.

## Observed regression

In graphical PIE:

- The right-side hypothetical opponent action controls have disappeared.
- The panel displays prediction text but no Quick, Heavy, Guard, or Dodge buttons.
- The actual-distance display reads `D: 0.0` despite separated fighters.
- The player can still scrub the reaction timeline.

Previous builds displayed the four opponent-action buttons.

## 1. Investigate the saved UI Blueprint

Inspect the actual saved `WBP_CombatUI`:

- Widget tree, including RightPanel and its children.
- `RefreshUI`
- `EventGraph`
- Four hypothetical-opponent button handlers.
- `RegeneratePredictionPreview`
- `ScrubTimeline`

Determine why the opponent-action controls are missing.

Check each button's visibility, enabled state, parent visibility, opacity, layout geometry, and execution paths.

Pay particular attention to the recent deletion of 10 nodes from `RefreshUI`, documented in `notes.md` §9.63.

Do not assume a successful Blueprint compilation proves the UI is working.

## 2. Restore opponent predictions

Planning mode (State 0):

- Show Quick, Heavy, Guard, and Dodge hypothetical actions.
- Selecting an action at the chosen timeline Instant must generate a separate hypothetical branch.
- Preserve the full baseline movement timeline.
- Preserve the moving cyan player ghost.
- Show the fixed gold reaction ghost at the calculated reaction opportunity.
- Display the selected hypothetical opponent action and its predicted consequences.
- Allow changing or clearing the hypothetical selection.

Reaction mode (State 3):

- Display the already-committed opponent action clearly.
- Do not allow a new opponent action while the opponent is busy.
- Show an explanatory unavailable state rather than silently removing prediction controls.
- Preserve the Continue preview and its decision-boundary stopping behavior.

## 3. Preserve separate timelines

The latest implementation correctly stores `GBranchTimeline` separately from the baseline.

Do not revert to replacing the main timeline with hypothetical branches.

Use `bHasBranchTimeline()` and `GetReactionSnapshot()` to display branch information independently.

Verify that the right panel reports the hypothetical branch rather than merely repeating baseline snapshot information.

## 4. Restore the actual-distance readout

The previous `RefreshUI` changes removed the actor-position nodes used to calculate actual distance.

Restore a valid distance calculation using the real fighter positions, with safe actor validation.

Do not substitute the predicted distance for the actual distance.

## 5. Verification

Test these scenarios:

A. Planning Walk, no hypothetical action: cyan ghost follows the full movement prediction.

B. Planning Walk, hypothetical Heavy at Instant 5: baseline continues, gold reaction ghost appears at the actual reaction opportunity, and the hypothetical branch information is displayed.

C. Change Heavy to Quick: reaction prediction updates without replacing the baseline.

D. Clear hypothetical selection: gold ghost disappears; baseline remains.

E. Actual Heavy reaction → Continue: existing reaction preview still works.

F. Reset: all hypothetical data and reaction ghosts are cleared.

G. Actual and predicted distance displays remain accurate.

Read back the actual Blueprint graphs and widget properties after editing.

Run all BUH tests, build, save, update notes.md and todo.md, and commit.

Do not call this resolved until the user verifies the controls and ghost behavior in graphical PIE.

**Critical restriction:** Do not remove existing UI nodes simply to make a headless test stop crashing. If the test cannot safely execute a gameplay-dependent widget in the editor world, fix the test environment or narrow its assertions instead.