# BUH — Fix Hypothetical Timeline Branching and One-Snapshot Preview

**Status: Fixed and verified.** All 25 BUH tests pass.

## Root Cause

`RegeneratePredictionPreview` called `GenerateAndStoreBaseTimeline` unconditionally on every invocation, overwriting the stored base timeline. During a reaction (State=1), the base was regenerated from the current non-planning combat state, producing a one-snapshot base. Branching at Instant 0 of that base yielded a one-snapshot combined timeline. Additionally, `GetBaseTimelineLength` was called before the generation, so the slider max was set to the stale/zero length.

## Fix Applied

Rewrote `RegeneratePredictionPreview` DSL in `WBP_CombatUI`:
- Only call `GenerateAndStoreBaseTimeline` when `GetBaseTimelineLength == 0` (no base exists yet).
- Call `GetBaseTimelineLength` after the conditional generation.
- Set slider max to the base length, then branch if a hypothetical opponent move is selected and the opponent snapshot phase is 0.
- Finally set slider max/value to the final `GetTimelineLength` and `ClampInstantFromFloat`.

The baseline Walk history is preserved across hypothetical branch selections.

## 1. Diagnose the one-snapshot timeline

Trace the exact sequence that generated it.

Log:
- Authoritative combat state and current Instant.
- Selected preview branch Instant.
- Player's active action and phase.
- Opponent's active action and phase.
- Hypothetical opponent action.
- Whether the opponent is legally able to start it.
- Generated prefix snapshot count.
- Generated branch snapshot count.
- First reaction Instant.
- StoppedReason.
- Final merged preview length.

A hypothetical attack that creates an immediate reaction may legitimately produce only one branch snapshot.

However, branching at Instant 6 must not discard the six earlier Instants of movement history.

## 2. Separate the baseline timeline from the branch

The intended workflow is:

1. Select Walk and a destination.
2. Generate a baseline timeline with no hypothetical opponent attack.
3. Scrub through the Walk's actual simulated positions.
4. Stop at an Instant, such as Instant 6.
5. Select hypothetical opponent Heavy.
6. Preserve the preview history leading up to Instant 6.
7. Start Heavy only if the opponent is legally ready.
8. Simulate forward to the next reaction or decision boundary.

Do not automatically replace the whole timeline with a fresh simulation beginning at Instant 0.

Preserve the original combat state at every branch point through deterministic replay or complete simulated state checkpoints. Do not reconstruct authoritative internal state from display-only snapshot fields.

## 3. Handle existing reaction states correctly

This screenshot was captured during an actual reaction to Heavy.

The opponent may already be committed to Heavy. Do not allow a hypothetical Heavy selection to overwrite that commitment unless the combat rules explicitly permit a new action.

Separate two contexts:

- Planning: investigate hypothetical future opponent actions.
- Reaction: investigate legal player responses to an already-observed threat.

At a reaction, preview Continue, Guard, Dodge, Quick, or Heavy as permitted by GetReactionInfo.

Do not restart the opponent's already committed attack merely because its hypothetical button was selected.

## 4. Fix the slider when timeline length changes

For N snapshots:
- Clamp selected index to 0 through N-1.
- Update the actual TimelineSlider.Value, not merely the displayed Instant text.
- If N=1, show Instant 0 and disable dragging.
- If N=0, show No preview.
- Preserve the selected branch point appropriately when the preview is regenerated.

The slider thumb, displayed Instant, and ghost positions must correspond to the same snapshot.

## 5. Verify using a controlled scenario

Start a fresh planning encounter, not an already triggered reaction.

Select Walk toward a destination.

Use a hypothetical opponent who remains idle during the baseline preview.

Verify:

- Slider moves cyan ghost progressively through Walk.
- Instant 0 and Instant 6 show different predicted positions.
- Select hypothetical Heavy at Instant 6.
- The prefix up to Instant 6 is unchanged.
- Heavy starts at Instant 6 if legal.
- Reaction detection uses the existing combat rules.
- Scrubbing backward still shows the original Walk history.
- The new branch displays its actual reaction event.
- Real fighters remain stationary throughout prediction.

Then separately test an existing real reaction with Continue.

## 6. Protect the working movement system

Do not modify BP_CombatManager.ExecuteMove, Walk/Dash/Jump movement, reaction timing, or Continue playback.

Limit changes to hypothetical preview state, branching, timeline selection, and their UI integration.

Add regression tests covering actual branch preservation and legal decision timing, not just GenerateTimeline returning a nonempty array.

Compile, save, read back relevant Blueprints, run all BUH tests, and update notes.md and todo.md.

Commit and push the changes to origin/master.

Report the commit SHA, one-snapshot cause, branching behavior, and any graphical checks requiring user confirmation.

**Success: The player can watch a Walk unfold, choose a hypothetical opponent action at a later legal Instant, and inspect the resulting reaction without losing the earlier movement history or altering real combat.**