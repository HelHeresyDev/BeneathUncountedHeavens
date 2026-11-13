# Beneath Uncounted Heavens — Tactical Timeline Preview

## Objective

Make the existing hypothetical combat timeline fully usable in graphical PIE.

The user has confirmed that Walk, Dash, Jump, airborne reactions, Continue, and clean startup now work. Preserve those systems.

**Do not create a new simulation system. Connect and repair the existing prediction infrastructure.**

Read the latest `AGENTS.md`, `todo.md`, relevant `notes.md` sections, and the current GitHub source before editing.

## 1. Inspect the existing prediction implementation

Relevant existing C++ functions include:

- `GenerateTimeline`
- `SetCurrentTimeline`
- `GetSnapshotAt`
- `GetTimelineLength`
- `GetSnapshotInstantText`
- `GetSnapshotInfoText`
- `MoveGhostsToInstant`
- `GetSnapshotPlayerHeight`
- `GetSnapshotOpponentHeight`

`FBuhTimelineSnapshot` already stores positions, height, actions, phases, health, reaction information, and combat events.

Inspect the SAVED Blueprint graphs for:

- `BP_CombatManager.BuildPredictions`
- `WBP_CombatUI.RefreshUI`
- `WBP_CombatUI.TimelineSlider.OnValueChanged`
- All four hypothetical opponent action buttons
- Player action selection and movement confirmation handlers

Determine which connections already work and which are missing. Reuse existing controls and ghosts.

## 2. Make timeline scrubbing functional

When the player selects an action and hypothetical opponent response, generate a deterministic prediction timeline.

The timeline slider must navigate actual snapshot indices.

At every selected Instant:

- Cyan ghost shows the predicted player's position.
- Orange ghost shows the hypothetical opponent's position.
- Both use correct X, Y, and Z.
- Both use the snapshot's facing direction.
- Actual red and blue fighters remain stationary.
- The displayed Instant corresponds to the selected snapshot.
- Scrubbing backward correctly restores earlier predicted positions.

Use:

`WorldZ = GroundZ + Snapshot.Height`

An airborne Jump must place the cyan ghost in the air.

Do not confuse the snapshot array index with `Snapshot.Instant`. Display the actual Instant value.

The existing GenerateTimeline implementation records snapshots after simulation steps, so inspect the initial-snapshot behavior before labeling the first slider position.

Clamp all slider inputs to valid indices. Do not read beyond the timeline.

If the timeline has zero or one snapshots, handle that gracefully without division by zero.

## 3. Display useful tactical information

For the selected Instant, show:

**Player**
- Selected action.
- Startup, active, recovery, or ready phase.
- Current phase progress.
- Health.

**Hypothetical opponent**
- Selected hypothetical action.
- Current phase and progress.
- Health.

**Combat events**
- Attack connects and damage dealt.
- Attack misses.
- Guard blocks.
- Dodge avoids an attack.
- Reaction opportunity occurs.
- A fighter becomes ready.
- KO or encounter-ending event.

Use existing snapshot events and simulation outcomes.

Do not invent hit results based on cylinder overlap or distance alone.

When an Instant contains no combat event, a short "No event" message is sufficient.

Keep the display concise and readable. Avoid the current barely visible prediction text beneath the opponent buttons.

The goal is functionality, not a wholesale UI redesign.

## 4. Highlight important Instants

Make important timeline moments discoverable.

At minimum, show the next significant event and allow jumping directly to it.

Useful event types:

- First attack activation.
- Hit, miss, guard, or dodge.
- Reaction opportunity.
- Recovery completion.
- End of the predicted sequence.

Prefer using existing `FBuhTimelineSnapshot.Events` and `StoppedReason`.

Do not implement an elaborate interactive timeline visualization yet. A small "Next Event" control and readable Instant/event text are enough.

If snapshot data cannot reliably identify a particular event type, report the limitation rather than guessing.

## 5. Preserve hypothetical opponent selection

The four existing opponent buttons are:

Quick, Heavy, Guard, Dodge.

Selecting one must regenerate the hypothetical timeline without changing the opponent's real committed action.

Switching hypothetical choices should update:

- Timeline length.
- Snapshot positions.
- Player and opponent ghosts.
- Event text.
- Highlighted important Instants.

Use one consistent regeneration pathway rather than four competing implementations.

Preserve opponent-only hypothetical previews when the player has not yet chosen an action, if supported by the current combat system.

Label the opponent action clearly as **Hypothetical**, not a revealed enemy action.

Do not use the hidden actual opponent move as a fallback when regenerating a hypothetical prediction.

## 6. Use the correct starting combat state

For a new encounter, predictions start from the clean initial state.

After a real action or reaction pause, predictions must start from the authoritative saved `CurrentState`, where valid.

This is essential for airborne Jump reactions.

A hypothetical preview during a midair reaction must:

- Preserve the fighter's actual airborne starting position.
- Preserve active-action progress when Continue is considered.
- Correctly represent legal replacement actions.
- Never reset the real fighter to the floor.
- Never overwrite CurrentState.

For Walk, Dash, and Jump predictions, use the actual confirmed movement destination and the established range limits.

The preview and authoritative execution must agree under the same starting state and selected decisions.

## 7. Keep previews separate from execution

This is a strict requirement.

Hypothetical preview:
- Moves only the cyan/orange ghosts.
- Does not change real fighter transforms.
- Does not change health or combat state.
- Does not advance the real Instant clock.
- Does not alter committed opponent actions.
- Does not trigger Jump playback.
- Does not replace the real reaction state.

Actual execution:
- Remains under BP_CombatManager.
- Uses authoritative combat results.
- Preserves the existing Jump playback and Continue behavior.

Audit the existing global `GCurrentTimeline` and separate Jump playback timeline to prevent accidental cross-contamination.

Also inspect `MoveGhostsToInstant()`: it currently obtains a world using `GEngine->GetWorldContexts()[0]`.

That can select the wrong world when both editor and PIE worlds exist. Prefer explicitly using the relevant PIE world or widget/manager world context.

Do not introduce another global gameplay state manager.

## 8. Ghost visibility and cleanup

Reuse the existing actors tagged:

- `GhostPlayer`
- `GhostOpponent`
- `DestPlayer`

Maintain their distinct roles.

Preview ghosts should appear only for a valid prediction and disappear when the preview is cleared.

The yellow destination marker must not become a third prediction fighter.

Verify cleanup after:

- Selecting Back.
- Switching from movement to Attack.
- Canceling a movement target.
- Changing hypothetical opponent action.
- Executing the selected action.
- Completing a reaction.
- Resetting the encounter.
- Starting a new PIE session.

Never leave stale ghosts at old predicted locations.

Do not clear a valid Walk, Dash, or Jump trajectory while the player is still planning that movement.

## 9. Automated tests

Add tests covering:

1. Timeline regeneration after selecting an opponent response.
2. Correct snapshot index-to-Instant mapping.
3. Ghost XY and Z matching the selected snapshot.
4. Correct Jump height when scrubbing through ascent, apex, and descent.
5. Walk and Dash predicted coordinates.
6. Correct health changes at hit Instants.
7. Guard, dodge, and miss events.
8. Reaction snapshots stopping at the correct decision boundary.
9. Next Event selecting the correct subsequent event.
10. Changing opponent choices replacing the previous hypothetical timeline.
11. No mutation of authoritative CurrentState or real fighter actors.
12. Valid prediction generation from an existing airborne reaction state.
13. Reset clearing all hypothetical ghosts and timeline UI.

Use a valid PIE game world for actor/UMG integration checks.

Tests should catch disconnected TimelineSlider handlers and incorrect ghost transforms—not merely confirm that GenerateTimeline returns snapshots.

Run all existing BUH automation tests.

## 10. Manual graphical PIE verification

Verify this sequence if graphical automation is available:

A. Start a clean encounter.

B. Select Quick Strike and hypothetical opponent Heavy.

C. Move the timeline slider from the beginning to the end. Confirm both ghosts update.

D. Select an Instant at which an attack connects or misses. Confirm the displayed event is correct.

E. Switch hypothetical opponent to Dodge. Confirm positions and event text change.

F. Select Walk or Dash and choose a destination. Confirm the predicted movement follows that destination.

G. Select Jump and scrub through its flight. Confirm cyan ghost rises and descends.

H. Execute a real Jump, stop at a legitimate midair reaction, and preview Continue. Confirm ghosts use the current airborne state without moving the real fighter.

I. Continue the real Jump and verify normal landing.

J. Reset and confirm no ghosts or stale timeline remain.

Distinguish headless checks from actual graphical UI verification. Do not claim that slider interaction was tested if only C++ functions were exercised.

## 11. Implementation constraints

- No new movement mechanics.
- No combat balance changes.
- No real-time combat or automatic playback redesign.
- No new opponent AI.
- No broad UMG restyling.
- No rebuilding working Blueprint assets.

Use targeted Blueprint edits. `write_graph_dsl` can destructively replace EventTick bodies; preserve existing handlers and connections.

Compile, save, and read back affected Blueprints.

Export relevant graph connections to:

`tests/blueprint_exports/tactical_timeline_preview.md`

Update `notes.md` and `todo.md`.

## 12. Commit and push

After implementation and available verification, commit and push to:

`origin/master`

Repository:
`HelHeresyDev/BeneathUncountedHeavens`

Report:
- Commit SHA.
- Actual implementation changes.
- Automation results.
- Graphical PIE checks completed.
- Any remaining limitations.

**Success criterion:** I can select a player action and hypothetical opponent action, scrub through Instants, see both prediction ghosts at their correct 3D positions, identify attacks and reactions, and change my planned action without affecting the actual battle.