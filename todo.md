# Beneath Uncounted Heavens — Interactive Tactical Timeline

The user has clarified the intended design of the timeline.

It is NOT simply a way to display final combat outcomes.

It should let the player examine the future Instant by Instant and introduce hypothetical opponent actions at selected times to explore possible reactions.

Current baseline: commit 8f27344.

**Stage 1: COMPLETE** — verified (graphical screenshot `Instant 6 / 13`, `Walk active (6), X -320` + `BUH.TimelinePreviewTests`), documented in notes.md §9.53, committed.

## Stage 1 — Verify movement scrubbing

The timeline slider now appears near the bottom of graphical PIE.

The screenshot shows "Instant 6 / 13" and "Player: Walk active (6), X -320".

Verify that dragging the slider through a Walk prediction:

- Changes the selected Instant.
- Moves the cyan predicted player through each simulated position.
- Shows intermediate positions, not only the destination.
- Updates the displayed movement phase and coordinates.
- Allows scrubbing backward.
- Leaves the actual fighter stationary.

Use the existing `GenerateTimeline`, snapshot getters, and ghost actors.

The prediction should use actual combat snapshots, not visual interpolation invented independently of the simulation.

If this already works, document the graphical checks and do not rewrite it.

Commit and push Stage 1 independently.

## Stage 2 — Schedule hypothetical opponent actions at selected Instants

The user wants this sequence:

1. Select Walk to a destination.
2. Scrub forward and watch the predicted Walk.
3. Stop at a chosen Instant, such as Instant 6.
4. Select hypothetical opponent Quick Strike.
5. See a new hypothetical timeline in which the opponent begins Quick Strike at Instant 6.
6. See the player's predicted movement advance until a valid reaction opportunity.
7. At that point, preview possible player responses or Continue.

**Important:** The current GenerateTimeline applies both decisions at the beginning. Changing the slider before choosing an opponent action must not be treated as merely changing the display index.

Implement a true hypothetical branch at the selected Instant.

### Simulation rules

- The opponent can begin a new action only when legally able to act at that Instant.
- If the opponent is in startup, active, or recovery, show that the action cannot begin yet.
- Preserve all simulation history leading to the selected Instant.
- Continue simulation deterministically from the correct state at the branch.
- Preserve existing preparation times, movement rules, and reaction restrictions.
- A hypothetical attack does not automatically land or cause a reaction merely because its button was clicked.
- Reuse the existing combat reaction detection.
- Stop at a legitimate reaction opportunity and offer legal responses.
- Do not automatically choose a response for the player.
- Do not reveal or modify the real opponent's hidden committed action.

### Technical requirements

The existing FBuhTimelineSnapshot primarily contains presentation data. It is not necessarily sufficient to resume combat with all internal action commitments and bookkeeping intact.

Do not reconstruct a complete combat state using only visible snapshot fields.

Instead, maintain or deterministically reconstruct a full simulated combat state at each potential branch point.

Use copies of authoritative combat state. Never mutate CurrentState while exploring predictions.

Do not make arbitrary snapshots into legal decision points if the fighter is still committed.

### UI behavior

The slider controls which Instant the player is inspecting.

When the player selects a hypothetical opponent action, show clearly:

"Opponent Quick Strike begins at Instant 6"

The preview timeline then branches from that point.

Keep the branch point identifiable when scrubbing backward.

When the predicted sequence reaches a player reaction opportunity, show:

"Reaction available at Instant N"

Allow the player to examine the legal responses and Continue without executing an actual move.

A reaction choice may generate another hypothetical branch, but should not modify the real encounter.

### Testing

Test:

- Walk with no opponent action, showing all movement Instants.
- Opponent Quick selected at Instant 0.
- Opponent Quick selected at Instant 6.
- The two scenarios produce appropriately different timelines.
- Attempting a new action while the opponent is busy is rejected.
- A valid attack produces the expected startup and reaction opportunities.
- Continue preserves the remaining Walk.
- Scrubbing before and after a branch shows the correct positions.
- The actual fighters never move during preview.
- Existing Walk, Dash, Jump, Execute, and Continue remain working.

Complete Stage 1 and establish its actual graphical result before implementing Stage 2.

Update notes.md and todo.md, run all BUH tests, and commit/push each completed stage to origin/master.

**Success: The player can watch a hypothetical Walk unfold Instant by Instant, introduce an opponent action at a legal chosen Instant, and explore how the combat situation changes before committing any real action.**