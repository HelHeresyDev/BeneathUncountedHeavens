# BUH — Reaction Preview Is Empty Despite Continue Being Selected (RESOLVED)

The user tested commit `4839cc0` in graphical PIE.

The screenshot shows:
- State: Reaction opportunity.
- Observed opponent attack: Heavy.
- Continue button visible.
- Player selection: Continue.
- Remaining movement path visible.
- Timeline: "No preview".
- Opponent prediction panel: "No preview".

The user wants to scrub through the remaining movement and see what happens if they Continue against the already-observed Heavy.

**Do not implement more branching features until this works.**

## 1. Trace exactly why "No preview" appears

Inspect the actual SAVED Blueprint graphs, particularly:

- WBP_CombatUI Continue.OnClicked
- WBP_CombatUI.RefreshUI
- WBP_CombatUI.RegeneratePredictionPreview
- WBP_CombatUI.ScrubTimeline
- BP_CombatManager.GeneratePreviewTimeline
- BP_CombatManager.GenerateAndStoreBaseTimeline
- Any ClearPreview and SetCurrentTimeline calls

During the exact sequence:

Walk → Execute → reaction to Heavy → select Continue

Record:

- Manager.State
- Manager.CurrentState.Fighters[0].Action/Phase/PhaseInstants
- Manager.CurrentState.Fighters[1].Action/Phase/PhaseInstants
- Manager.PlayerMove
- Selected hypothetical opponent action
- GetBaseTimelineLength()
- GetTimelineLength()
- bHasTimeline()
- Whether GenerateAndStoreBaseTimeline executes
- Whether it returns snapshots
- Whether ClearPreview or another function subsequently erases them

Identify the exact function that prevents the preview from existing.

Do not infer success from passing C++ tests.

## 2. Correct the planning/reaction distinction

**Planning state**

- The player selects Walk, Dash, or Jump.
- Generate a baseline timeline.
- The player can scrub through their movement.
- Hypothetical opponent actions may be introduced at legal future Instants.

**Reaction state**

- The actual opponent has already committed to an observed action.
- The player chooses a legal response, including Continue.
- Generate the prediction from the authoritative frozen CurrentState.
- Preserve the opponent's existing action, phase, and preparation progress.
- Do not introduce a new hypothetical opponent action.
- Do not restart the opponent's Heavy.
- Do not reset the player's Walk to its original starting position.

For Continue, pass player decision 7 and no new opponent decision to the existing simulator.

Confirm that GenerateTimeline can advance this state while respecting the existing ReactedThreatFighter bookkeeping so the same Heavy does not trigger an identical reaction repeatedly.

For another legal response, simulate the appropriate replacement action from the same frozen combat state.

## 3. Generate a reaction preview when a legal response is selected

Selecting Continue should:

1. Read a COPY of the authoritative frozen CurrentState.
2. Generate the hypothetical continuation against the already committed opponent action.
3. Store a valid nonempty preview timeline and its associated full states.
4. Configure the slider range from the resulting snapshot count.
5. Set the slider to a valid initial snapshot index.
6. Move cyan/orange prediction ghosts to that snapshot.
7. Display the current Instant and expected action phases.

It must not require an opponent hypothetical button click.

Do not allow RegeneratePredictionPreview to clear a valid reaction preview merely because no hypothetical opponent move is selected.

Keep the reaction preview separate from the stored baseline used for planning branches.

## 4. Correct empty timeline UI behavior

If no valid preview exists:

- Display "No preview".
- Disable the slider.
- Reset its value to zero.
- Hide prediction ghosts.

When a preview exists:

- Enable the slider when more than one snapshot exists.
- Set its maximum to the last valid snapshot index.
- Ensure the thumb position, Instant text, and ghost locations match.
- Scrubbing must not regenerate or clear the preview.

If simulation legitimately stops immediately, display the actual reaction/stop reason rather than silently presenting an unexplained empty timeline.

## 5. Add a real reaction-preview regression test

A direct C++ test that GenerateTimeline works is not sufficient.

Test the actual connected UI/manager pathway:

1. Start a valid Walk.
2. Trigger an actual Heavy reaction.
3. Verify authoritative CurrentState preserves the active Walk and committed Heavy.
4. Select Continue in the real WBP_CombatUI instance.
5. Assert GetTimelineLength() > 1 for a scenario where remaining Walk movement exists.
6. Assert the selected snapshot's player positions advance.
7. Assert the opponent's Heavy phase progresses correctly.
8. Assert scrubbing moves only the prediction ghosts.
9. Assert the real blue/red fighters and CurrentState remain unchanged.
10. Click Execute and confirm existing authoritative Continue still works.

If actual UMG clicks cannot be automated, report that limitation clearly and provide diagnostic evidence from a valid PIE-world widget instance.

Also verify fresh-planning Walk preview still works, including the stored baseline and hypothetical branch behavior.

## 6. Strict scope

Do not modify:
- Authoritative ExecuteMove.
- Movement simulation or timing.
- Walk/Dash/Jump destinations.
- Jump playback.
- Reaction detection or legality.
- Movement path rendering.
- Timeline widget hierarchy or visual styling.

Fix only the prediction generation and UI state routing needed to make Continue preview work.

Read back the actual saved Blueprint graphs after making changes.

Run all BUH tests.

Update notes.md and todo.md with the real cause and verification evidence.

Commit and push to origin/master.

**Success: At an actual Heavy reaction during Walk, selecting Continue creates a scrubbable preview of the remaining Walk and the opponent's committed Heavy. The real battle remains frozen until Execute is clicked.**