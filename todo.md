# Beneath Uncounted Heavens — Fix Stale Ghost Cylinders and Combat Position Synchronization

The latest graphical PIE screenshot shows unwanted cylinders remaining at earlier fighter positions after movement and hypothetical prediction.

The player describes each fighter as leaving remnants behind.

We currently have five intended actors: two real fighters, two hypothetical prediction ghosts, and one yellow Walk destination ghost.

The goal is to make their visibility and positions consistent with the current encounter, without adding or duplicating actors.

## 1. Identify every visible cylinder

Inspect the live PIE world and enumerate all BP_Fighter-derived actors.

For each, record:

- Actor name and class.
- Tags and Team.
- Whether it is real, prediction, or destination.
- World location.
- HiddenInGame status.
- Visible mesh components and their visibility.
- Assigned material.

Compare this information against the screenshot.

Determine whether the unwanted cylinders are legitimate ghosts left visible, duplicate spawned actors, or real fighters whose transforms no longer match the combat state.

Do not assume the problem is only cosmetic.

## 2. Define consistent ghost visibility rules

In normal planning without an active preview:

- Real player: visible.
- Real opponent: visible.
- Yellow destination ghost: hidden.
- Cyan prediction ghost: hidden.
- Orange prediction ghost: hidden.

While planning Walk:

- Real fighters remain visible.
- Yellow ghost shows the intended destination.
- Cyan/orange ghosts appear only when an actual hypothetical prediction is being displayed.

While scrubbing a hypothetical timeline:

- Cyan and orange ghosts show the simulated positions at the selected Instant.
- Yellow ghost remains at the requested Walk destination, if Walk planning is active.
- Real fighters remain at their authoritative positions.

After Execute, Cancel, Back, or Reset:

- Invalidate stale preview data.
- Hide ghosts no longer relevant to the current planning state.
- Restore only the previews appropriate for the new state.

## 3. Fix ghost synchronization after combat advances

When an action executes:

1. Commit the combat result to the authoritative state.
2. Synchronize both real fighter transforms to that state.
3. Clear the previous hypothetical timeline and selection as appropriate.
4. Hide obsolete prediction ghosts.
5. Hide the yellow Walk destination ghost.
6. Begin the next planning/reaction state using the saved combat state.

When a new hypothetical prediction is generated, initialize its ghosts using that prediction's first snapshot derived from the current authoritative state.

Never initialize a new prediction from the original ±500 spawn positions after combat has already advanced.

Do not hide or reset ghosts mid-preview in ways that break timeline scrubbing.

## 4. Investigate state versus actor position

The screenshot shows `Current X: 392`, while several cylinders appear in earlier positions.

Log and compare:

- Authoritative PlayerX and OpponentX.
- CurrentState fighter positions.
- Actual real fighter actor transforms.
- Ghost actor transforms.
- Predicted snapshot positions.
- Timeline scrub Instant.

Verify these values before and after Execute and after selecting a new hypothetical response.

If CurrentState exists, ensure predictions preserve its complete action phases, Instant, health, and opponent commitment. Do not silently replace ongoing combat state with a fresh MakeState containing only positions and health.

## 5. Centralize ghost visibility management

Prefer one clearly defined function or small set of functions controlling preview visibility and positioning.

Avoid scattering contradictory SetActorHiddenInGame and SetActorLocation operations across many widget handlers.

Where possible, keep prediction actor management separate from authoritative combat execution.

Use actor tags or explicit actor references, not generated names such as BP_Fighter_C_2.

Do not add another ghost actor to solve this.

## 6. Preserve correct preview behavior

After the fix:

- Moving the Walk slider updates only the yellow destination target and regenerates prediction data when appropriate.
- Selecting hypothetical Quick/Heavy/Guard/Dodge updates the cyan/orange predicted positions.
- Scrubbing the timeline displays those predicted positions.
- The real fighters never move during planning.
- Executing an action advances real fighters using the authoritative resolver.
- Starting another prediction uses the new positions.
- Reset returns real fighters to their starting positions and clears obsolete ghosts.

## 7. Test an uninterrupted encounter

In graphical PIE:

1. Start the encounter: only two real fighters visible.
2. Select Walk: yellow destination ghost appears.
3. Select hypothetical Heavy: cyan/orange prediction ghosts appear.
4. Adjust destination: yellow ghost updates and prediction regenerates.
5. Execute Walk: stale preview ghosts disappear.
6. Observe the real fighters' new positions.
7. Select Walk again: destination and prediction ghosts must be based on the new positions.
8. Select another hypothetical response and scrub the timeline.
9. Cancel: all irrelevant ghosts disappear.
10. Reset: exactly two real fighters remain visible.

Capture screenshots before and after execution.

If MCP cannot click the controls, report the automated checks separately from manual visual verification.

## 8. Regression and documentation

Preserve all existing material colors, menu functionality, Walk distance calculation, timeline, and reaction rules.

Run the four existing automation suites.

Compile and save modified assets.

Update notes.md only with a confirmed, reusable lesson.

No Dash, Jump, new movement mechanics, or unrelated HUD redesign.

No Git commit without authorization.

**Success criterion:** No cylinders remain at obsolete starting positions after combat advances. Every visible ghost represents the currently active Walk destination or an explicitly selected hypothetical prediction, and all previews are based on the current authoritative combat state.