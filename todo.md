# Beneath Uncounted Heavens — Phase 9A: Movement and Reaction Windows

The design bible has been updated to Version 1.2.

Read `designbible.md` before beginning. Pay particular attention to the revised tactical combat rules.

We are changing the fundamental combat action system.

## Design decisions — mandatory

1. **One action at a time.** Characters choose one action, not an attack and movement simultaneously.
2. **Movement is its own category of abilities.** Walk, Dash, Dodge, Jump, and Qinggong will eventually be separate selectable actions.
3. **Ordinary attacks do not automatically move fighters.** Remove the current automatic approach/lunge behavior from Quick Strike and Heavy Strike. Movement belongs to explicit movement actions. Do not prevent specialized future techniques from defining their own movement effects.
4. **Actions can have reaction windows.** A character may abandon or transition from an ongoing action when its rules permit.
5. **Combat pauses at meaningful reaction opportunities.** The player has unlimited thinking time whenever a valid reaction decision becomes available.
6. **Reaction opportunities depend on observable information.** Never reveal or react to a secretly selected enemy action before the character can perceive it.
7. **Reactions are not free defenses.** Selecting a reaction begins a new action with its own timing and consequences.

## Objective

Implement the first complete, deterministic, reaction-aware movement scenario in the existing Unreal combat prototype.

**The immediate goal is Walking and reacting to an incoming attack. Do not implement the entire movement system at once.**

## Step 1 — Audit the existing combat architecture

Inspect:

- `CombatSim.h`
- `CombatSim.cpp`
- `StepCombatInstant`
- `ResolveCombat`
- `GenerateTimeline`
- `FBuhCombatState`
- `BP_CombatManager`
- `WBP_CombatUI`

Identify how the current system handles movement, readiness, action commitment, and decision scheduling.

Document which assumptions must change.

Make a source-control checkpoint before modifying the combat engine.

Preserve the current authoritative C++ simulation architecture.

## Step 2 — Separate movement from attacks

Remove the hardcoded automatic approach movement from ordinary Quick Strike and Heavy Strike.

An attack should only connect when its target is within the appropriate range at the relevant Instant.

Attacks should not automatically close the distance simply because the fighter selected them.

Preserve Guard and the existing movement-based Dodge behavior.

Implement the necessary changes in the shared authoritative simulator, not as Blueprint-only adjustments.

Update existing tests whose expectations legitimately change, documenting why. Do not weaken unrelated assertions to obtain passing results.

## Step 3 — Add a Walk action

Implement Walk as an explicit movement action.

Walk must support:

- A selected movement direction.
- A configurable movement speed.
- Deterministic movement each Instant.
- Collision and minimum-separation rules.
- An intended duration or destination.
- Persistent action state across decision boundaries.
- The ability to stop or transition to another action at permitted reaction opportunities.

For the first prototype, a simple horizontal approach or retreat is sufficient.

Preserve compatibility with eventual three-dimensional movement, but do not implement a complete navigation system yet.

Walking should be flexible: it does not require an extended initial commitment period.

## Step 4 — Introduce action commitment and reaction eligibility

Extend the action definitions so each action can describe when a character is allowed to change actions.

Avoid hardcoding this exclusively around Walk.

The underlying system must eventually support:

- Walk: reactable immediately or nearly immediately.
- Dash: initially committed, reactable later.
- Jump: initially committed during takeoff, reactable later.
- Other actions: individual commitment and cancellation rules.

For this milestone, implement and fully test Walk's reaction behavior.

Do not implement Dash or Jump merely as copies of Walk.

The exact timings for future movement abilities remain design parameters, not established balance values.

## Step 5 — Implement event-triggered reaction opportunities

This is the most important requirement.

When the player is Walking and an opponent begins an observable attack:

1. The authoritative simulation detects the observable threat.
2. It checks whether the player's ongoing action permits a reaction.
3. If eligible, the simulation stops at that Instant.
4. The game enters a reaction-planning state.
5. The player can examine the situation without a time limit.
6. The player selects a new legal action.
7. The Walking action ends or transitions according to its cancellation rules.
8. The new action begins from the current combat state.
9. Simulation resumes normally.

Do not rewind time or reset the opponent's attack.

The opponent must continue from its actual preparation, active, or recovery state.

The reaction window must not instantly cancel damage or guarantee successful defense.

A Guard that becomes effective too late should still fail.

## Step 6 — Define observable threats

Initially, implement a simple explicit perception rule:

An opponent's attack preparation becomes observable when the attack visibly begins.

This should create an observable combat event rather than expose the opponent's secret input selection.

Do not trigger a reaction merely because the opponent committed an action internally.

Keep the detection mechanism extensible for future cultivation perception, Qi sensing, concealed techniques, and deceptive attacks.

Prevent the same unchanged threat from causing an infinite loop of repeated pauses.

A reaction opportunity should occur only when there is a meaningful new decision to make.

NPCs must follow the same action-eligibility and perception rules. Their reaction decisions can use a simple deterministic AI for now.

## Step 7 — Preserve complete combat state

Reaction decisions must preserve:

- Current Instant.
- Both fighters' positions and facing.
- Current actions.
- Current action phases.
- Elapsed and remaining action timing.
- Recovery and commitment status.
- Observed threats.
- Health.
- Pending action consequences.
- Relevant AI and random-seed state.

Prefer storing the complete authoritative `FBuhCombatState` rather than reconstructing it from a small collection of Blueprint variables.

The original attack must not restart after a reaction pause.

No fighter may receive a free Guard or Dodge merely because the simulation stopped.

## Step 8 — Integrate with tactical predictions

The existing Phase 8 tactical timeline must use the same simulation and reaction rules.

When previewing Walking against an opponent's hypothetical attack:

- Display movement up to the reaction opportunity.
- Identify the Instant when a reaction becomes available.
- Stop the hypothetical branch at that decision point unless a specific continuation is provided.
- Allow the actual player to choose a response before real combat advances.
- Keep the real encounter unchanged while previewing.

Do not fabricate the player's future reaction inside the prediction engine.

Continue to protect hidden opponent information.

## Step 9 — Minimal UI integration

Do not undertake another major cosmetic UI redesign during this milestone.

Add a minimally usable Move category containing Walk and the existing Dodge action.

For now:

- Attack: Quick Strike, Heavy Strike.
- Defend: Guard.
- Move: Walk, Dodge.
- Special: disabled or empty.

Use the hierarchical Attack / Defend / Move / Special concept from the design bible, but only implement enough to support this milestone.

When a reaction becomes available, clearly display:

- Reaction opportunity.
- Current Instant.
- Observed threat.
- Current action.
- Available responses.

Keep Execute separate from action selection.

Preserve the timeline, opponent-response interface, and existing combat HUD. Prevent new UI elements from obscuring the fighters.

A fully polished Persona-style or radial menu will be a separate task.

## Step 10 — Mandatory automated tests

Preserve the existing combat and timeline tests wherever their assumptions remain valid.

Add tests proving:

**A — Walking moves the fighter**

The fighter changes position according to movement speed and elapsed Instants.

**B — Attacks no longer move automatically**

Quick and Heavy Strike do not close distance without an explicit movement effect.

**C — Observable attack triggers a reaction**

The player is Walking. The opponent visibly begins preparing a strike.

Expected: simulation pauses at a valid reaction Instant.

**D — No secret-information reaction**

An opponent secretly selects an attack, but it has not become observable.

Expected: no reaction is triggered by that hidden selection alone.

**E — Reaction preserves enemy progress**

The player reacts while the opponent is preparing an attack.

Expected: the opponent retains its current preparation progress, rather than restarting.

**F — Guard may be too slow**

The player reacts with Guard, but its effective defensive window begins after the incoming attack connects.

Expected: the player takes damage.

**G — Successful reaction**

The player selects an appropriately timed defensive or movement action.

Expected: the attack is avoided or blocked according to the authoritative rules.

**H — Prediction matches execution**

The same initial state and choices produce identical results in timeline prediction and real combat.

**I — No repeated pause loop**

The same observable attack does not create endless duplicate reaction pauses.

**J — Determinism and state persistence**

Identical inputs, events, and seeds produce identical reaction opportunities and outcomes across multiple decisions.

## Step 11 — Graphical verification

Use the established VRAM-safe graphical testing workflow.

Demonstrate this complete sequence:

1. Two fighters begin separated.
2. The player selects Walk toward the opponent.
3. The fighter visibly walks.
4. The opponent begins preparing an attack.
5. The simulation automatically pauses.
6. The UI identifies a reaction opportunity.
7. The player selects Guard or Dodge.
8. Combat resumes from the correct Instant.
9. The attack resolves based on actual timing and position.

Capture the relevant combat states.

If automated UI input is unavailable, provide a reproducible manual playtest and clearly distinguish verified functionality from untested interaction.

## Step 12 — Final report

Create `phase9a_report.md` documenting:

1. Combat architecture changes.
2. Separation of movement and attacks.
3. Walk implementation.
4. Action-commitment representation.
5. Observable-threat detection.
6. Reaction decision scheduling.
7. How complete combat state is preserved.
8. Prediction and execution consistency.
9. Automated test results.
10. Graphical verification.
11. Known limitations.

Do not add Dash, Jump, Qinggong, new attack techniques, cinematic animations, or complex movement physics during this milestone.

Do not mark the milestone complete until a walking fighter can react to an observable incoming attack in actual gameplay.

**Success criterion: Walking toward an opponent creates a genuine, timing-sensitive tactical reaction opportunity when that opponent attacks, with no loss of deterministic state or hidden-information fairness.**# Beneath Uncounted Heavens — Phase 9A: Movement and Reaction Windows

The design bible has been updated to Version 1.2.

Read `designbible.md` before beginning. Pay particular attention to the revised tactical combat rules.

We are changing the fundamental combat action system.

## Design decisions — mandatory

1. **One action at a time.** Characters choose one action, not an attack and movement simultaneously.
2. **Movement is its own category of abilities.** Walk, Dash, Dodge, Jump, and Qinggong will eventually be separate selectable actions.
3. **Ordinary attacks do not automatically move fighters.** Remove the current automatic approach/lunge behavior from Quick Strike and Heavy Strike. Movement belongs to explicit movement actions. Do not prevent specialized future techniques from defining their own movement effects.
4. **Actions can have reaction windows.** A character may abandon or transition from an ongoing action when its rules permit.
5. **Combat pauses at meaningful reaction opportunities.** The player has unlimited thinking time whenever a valid reaction decision becomes available.
6. **Reaction opportunities depend on observable information.** Never reveal or react to a secretly selected enemy action before the character can perceive it.
7. **Reactions are not free defenses.** Selecting a reaction begins a new action with its own timing and consequences.

## Objective

Implement the first complete, deterministic, reaction-aware movement scenario in the existing Unreal combat prototype.

**The immediate goal is Walking and reacting to an incoming attack. Do not implement the entire movement system at once.**

## Step 1 — Audit the existing combat architecture

Inspect:

- `CombatSim.h`
- `CombatSim.cpp`
- `StepCombatInstant`
- `ResolveCombat`
- `GenerateTimeline`
- `FBuhCombatState`
- `BP_CombatManager`
- `WBP_CombatUI`

Identify how the current system handles movement, readiness, action commitment, and decision scheduling.

Document which assumptions must change.

Make a source-control checkpoint before modifying the combat engine.

Preserve the current authoritative C++ simulation architecture.

## Step 2 — Separate movement from attacks

Remove the hardcoded automatic approach movement from ordinary Quick Strike and Heavy Strike.

An attack should only connect when its target is within the appropriate range at the relevant Instant.

Attacks should not automatically close the distance simply because the fighter selected them.

Preserve Guard and the existing movement-based Dodge behavior.

Implement the necessary changes in the shared authoritative simulator, not as Blueprint-only adjustments.

Update existing tests whose expectations legitimately change, documenting why. Do not weaken unrelated assertions to obtain passing results.

## Step 3 — Add a Walk action

Implement Walk as an explicit movement action.

Walk must support:

- A selected movement direction.
- A configurable movement speed.
- Deterministic movement each Instant.
- Collision and minimum-separation rules.
- An intended duration or destination.
- Persistent action state across decision boundaries.
- The ability to stop or transition to another action at permitted reaction opportunities.

For the first prototype, a simple horizontal approach or retreat is sufficient.

Preserve compatibility with eventual three-dimensional movement, but do not implement a complete navigation system yet.

Walking should be flexible: it does not require an extended initial commitment period.

## Step 4 — Introduce action commitment and reaction eligibility

Extend the action definitions so each action can describe when a character is allowed to change actions.

Avoid hardcoding this exclusively around Walk.

The underlying system must eventually support:

- Walk: reactable immediately or nearly immediately.
- Dash: initially committed, reactable later.
- Jump: initially committed during takeoff, reactable later.
- Other actions: individual commitment and cancellation rules.

For this milestone, implement and fully test Walk's reaction behavior.

Do not implement Dash or Jump merely as copies of Walk.

The exact timings for future movement abilities remain design parameters, not established balance values.

## Step 5 — Implement event-triggered reaction opportunities

This is the most important requirement.

When the player is Walking and an opponent begins an observable attack:

1. The authoritative simulation detects the observable threat.
2. It checks whether the player's ongoing action permits a reaction.
3. If eligible, the simulation stops at that Instant.
4. The game enters a reaction-planning state.
5. The player can examine the situation without a time limit.
6. The player selects a new legal action.
7. The Walking action ends or transitions according to its cancellation rules.
8. The new action begins from the current combat state.
9. Simulation resumes normally.

Do not rewind time or reset the opponent's attack.

The opponent must continue from its actual preparation, active, or recovery state.

The reaction window must not instantly cancel damage or guarantee successful defense.

A Guard that becomes effective too late should still fail.

## Step 6 — Define observable threats

Initially, implement a simple explicit perception rule:

An opponent's attack preparation becomes observable when the attack visibly begins.

This should create an observable combat event rather than expose the opponent's secret input selection.

Do not trigger a reaction merely because the opponent committed an action internally.

Keep the detection mechanism extensible for future cultivation perception, Qi sensing, concealed techniques, and deceptive attacks.

Prevent the same unchanged threat from causing an infinite loop of repeated pauses.

A reaction opportunity should occur only when there is a meaningful new decision to make.

NPCs must follow the same action-eligibility and perception rules. Their reaction decisions can use a simple deterministic AI for now.

## Step 7 — Preserve complete combat state

Reaction decisions must preserve:

- Current Instant.
- Both fighters' positions and facing.
- Current actions.
- Current action phases.
- Elapsed and remaining action timing.
- Recovery and commitment status.
- Observed threats.
- Health.
- Pending action consequences.
- Relevant AI and random-seed state.

Prefer storing the complete authoritative `FBuhCombatState` rather than reconstructing it from a small collection of Blueprint variables.

The original attack must not restart after a reaction pause.

No fighter may receive a free Guard or Dodge merely because the simulation stopped.

## Step 8 — Integrate with tactical predictions

The existing Phase 8 tactical timeline must use the same simulation and reaction rules.

When previewing Walking against an opponent's hypothetical attack:

- Display movement up to the reaction opportunity.
- Identify the Instant when a reaction becomes available.
- Stop the hypothetical branch at that decision point unless a specific continuation is provided.
- Allow the actual player to choose a response before real combat advances.
- Keep the real encounter unchanged while previewing.

Do not fabricate the player's future reaction inside the prediction engine.

Continue to protect hidden opponent information.

## Step 9 — Minimal UI integration

Do not undertake another major cosmetic UI redesign during this milestone.

Add a minimally usable Move category containing Walk and the existing Dodge action.

For now:

- Attack: Quick Strike, Heavy Strike.
- Defend: Guard.
- Move: Walk, Dodge.
- Special: disabled or empty.

Use the hierarchical Attack / Defend / Move / Special concept from the design bible, but only implement enough to support this milestone.

When a reaction becomes available, clearly display:

- Reaction opportunity.
- Current Instant.
- Observed threat.
- Current action.
- Available responses.

Keep Execute separate from action selection.

Preserve the timeline, opponent-response interface, and existing combat HUD. Prevent new UI elements from obscuring the fighters.

A fully polished Persona-style or radial menu will be a separate task.

## Step 10 — Mandatory automated tests

Preserve the existing combat and timeline tests wherever their assumptions remain valid.

Add tests proving:

**A — Walking moves the fighter**

The fighter changes position according to movement speed and elapsed Instants.

**B — Attacks no longer move automatically**

Quick and Heavy Strike do not close distance without an explicit movement effect.

**C — Observable attack triggers a reaction**

The player is Walking. The opponent visibly begins preparing a strike.

Expected: simulation pauses at a valid reaction Instant.

**D — No secret-information reaction**

An opponent secretly selects an attack, but it has not become observable.

Expected: no reaction is triggered by that hidden selection alone.

**E — Reaction preserves enemy progress**

The player reacts while the opponent is preparing an attack.

Expected: the opponent retains its current preparation progress, rather than restarting.

**F — Guard may be too slow**

The player reacts with Guard, but its effective defensive window begins after the incoming attack connects.

Expected: the player takes damage.

**G — Successful reaction**

The player selects an appropriately timed defensive or movement action.

Expected: the attack is avoided or blocked according to the authoritative rules.

**H — Prediction matches execution**

The same initial state and choices produce identical results in timeline prediction and real combat.

**I — No repeated pause loop**

The same observable attack does not create endless duplicate reaction pauses.

**J — Determinism and state persistence**

Identical inputs, events, and seeds produce identical reaction opportunities and outcomes across multiple decisions.

## Step 11 — Graphical verification

Use the established VRAM-safe graphical testing workflow.

Demonstrate this complete sequence:

1. Two fighters begin separated.
2. The player selects Walk toward the opponent.
3. The fighter visibly walks.
4. The opponent begins preparing an attack.
5. The simulation automatically pauses.
6. The UI identifies a reaction opportunity.
7. The player selects Guard or Dodge.
8. Combat resumes from the correct Instant.
9. The attack resolves based on actual timing and position.

Capture the relevant combat states.

If automated UI input is unavailable, provide a reproducible manual playtest and clearly distinguish verified functionality from untested interaction.

## Step 12 — Final report

Create `phase9a_report.md` documenting:

1. Combat architecture changes.
2. Separation of movement and attacks.
3. Walk implementation.
4. Action-commitment representation.
5. Observable-threat detection.
6. Reaction decision scheduling.
7. How complete combat state is preserved.
8. Prediction and execution consistency.
9. Automated test results.
10. Graphical verification.
11. Known limitations.

Do not add Dash, Jump, Qinggong, new attack techniques, cinematic animations, or complex movement physics during this milestone.

Do not mark the milestone complete until a walking fighter can react to an observable incoming attack in actual gameplay.

**Success criterion: Walking toward an opponent creates a genuine, timing-sensitive tactical reaction opportunity when that opponent attacks, with no loss of deterministic state or hidden-information fairness.**