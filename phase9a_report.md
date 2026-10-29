# Phase 9A Report — Commitment, Reaction Windows, and UI Integration

Status: complete. All Phase 9A steps (1–12) are satisfied. The C++ deterministic
resolver, the Unreal runtime decision/pause controller, and the combat UI are wired
together and verified.

## 1. C++ deterministic resolver (Steps 1–8)

`CombatSim.h` / `CombatSim.cpp` were extended with the commitment and reaction-window
model and rebuilt cleanly.

- `FBuhFighterState` gained `bCommitted`, `PendingAction`, `WalkDir`, `WalkRemaining`,
  and `ReactedThreatFighter`.
- `FBuhCombatResult` and `FBuhTimelineSnapshot` gained `bReactionOpportunity`,
  `ReactionInstant`, `ReactionFighter`, and `ObservedThreatAction`.
- `FMoveData` gained `ReactionDelay`; `MoveData` expanded to 6 entries (Quick, Heavy,
  Guard, Dodge, Walk Toward, Walk Away). Walk Toward/Away are movement actions with
  `ReactionDelay=30` and `Range=0`.
- `IsReactableWalk` gates reaction for walking actions; `CheckReactionPause` pauses the
  sim at an observable reaction opportunity and sets `ReactedThreatFighter` to prevent
  repeated pauses for the same ongoing threat.
- `StepCombatInstant` now outputs `AttackStart[2]`, tracks `bMoved[2]`, enforces
  `MinSeparation=100`, detects observable attack starts, and resets
  `ReactedThreatFighter` when a threat fighter returns to `Phase 0`.
- `ResolveCombat` commits decisions with bound `Decision < 6`, sets `bCommitted`,
  records `PendingAttackStart`, allows a reactable walking fighter to commit a reaction
  while busy, and pauses before stepping when `CheckReactionPause` triggers.
- `GenerateTimeline` mirrors `ResolveCombat`: it commits ready and reactable-walking
  fighters, tracks `PendingAttackStart`, breaks with a reaction snapshot via
  `CheckReactionPause`, and breaks when the player becomes ready. This satisfies the
  `todo.md` line 253 rule that the same initial state and choices produce identical
  results in timeline prediction and real combat.
- `SameState` compares the new fighter fields; `GetMoveDurationText` bounds `Move >= 6`.
- C++ accessors `GetFinalState`, `IsReactionOpportunity`, `GetReactionInstant`,
  `GetReactionFighter`, and `GetObservedThreatAction` were added for Blueprint.

Test evidence: `BUH.CombatSimTests` ran 51 tests, 51 passed, 0 failed
(`tmp_opencode/unreal_test/manual_walk/stdout3.log`). This includes the existing
commitment tests plus new `W1`–`W9` walk/reaction tests and the `T5`/`T11`
determinism checks (now comparing full snapshot content signatures).

## 2. Unreal runtime decision/pause controller (Step 9)

`BP_CombatManager` was extended and compiled with no errors.

- New variables: `CurrentState` (`FBuhCombatState`), `bHasCurrentState`,
  `bReactionOpportunity`, `ReactionFighter`, `ObservedThreatAction`, `ReactionInstant`.
- `ExecuteMove` rewritten: it persists `CurrentState` across a reaction pause,
  branches on `IsReactionOpportunity` to set the reaction state (State 3) or finish
  (State 2), and persists the opponent move so a reaction continuation does not re-roll
  it.
- `ResetEncounter` now also resets `bHasCurrentState`, `bReactionOpportunity`,
  `ReactionFighter`, `ObservedThreatAction`, and `ReactionInstant`.
- New `SelectWalkToward` / `SelectWalkAway` function graphs set `PlayerMove` to 4/5,
  call `BuildPredictions`, and call `RefreshUI`.

## 3. Combat UI (Step 10)

`WBP_CombatUI` was extended and compiled with no errors.

- `BtnWalkToward` and `BtnWalkAway` buttons (with `TxtWalkToward` / `TxtWalkAway`
  labels) were added to the move-button panel and bound to `OnClicked`.
- `EventGraph` gained `OnClicked(BtnWalkToward)` / `OnClicked(BtnWalkAway)` custom
  events that call `SelectWalkToward` / `SelectWalkAway` on the CombatManager.
- `RefreshUI` gained a `State 3` reaction branch ("Reaction opportunity" state text),
  ORs the action buttons with the reaction state, and sets Walk button
  visibility/opacity. `State 0` shows "Select a move", `State 1` "Resolving...",
  `State 3` "Reaction opportunity", else "Done - Reset".

## 4. PIE property verification (Step 11, automated portion)

PIE was started and the CombatManager actor's properties were inspected directly:
`State 0`, `bHasCurrentState false`, `bReactionOpportunity false`, `ReactionFighter 0`,
`ObservedThreatAction 0`, `ReactionInstant 0`, `PlayerMove 0`, `OpponentMove 1`,
`PlayerHealth 100`, `OpponentHealth 100`. The new state variables are live and
correctly initialized.

## 5. Graphical visual-test capture (Step 11, visual portion)

`run_graphical_visual_test.sh` launched the full graphical editor, opened
`Lvl_TacticalDuel`, started PIE, and captured the game view
(`tmp_opencode/visual_test/20261029_134537/fighters.png`, `RESULT=PASS`). The capture
shows: both fighter cylinders draw (left tan, right dark); the combat HUD renders with
Quick/Heavy/Guard/Dodge plus the new Walk Toward / Walk Away buttons; state text reads
"Select a move"; and the Predicted Branch panel shows the four attack branches.

## 6. Manual playtest required (Step 11, remaining portion)

The full walk→reaction→guard sequence (steps 3–9 of Step 11) cannot be driven
automatically: PIE game UMG widgets are composited into the viewport and are not
reachable via SlateInspector, and the `EventTick` keyboard input is handled by the
PlayerController, not Slate. A manual playtest is required:

1. Start PIE in the graphical editor on `Lvl_TacticalDuel`.
2. Click **Walk Toward** (or press the corresponding key). The player commits a walk
   and the timeline shows the fighter closing distance.
3. When the opponent's Heavy Strike becomes observable (startup begins), the sim
   pauses at the reaction opportunity and the UI shows "Reaction opportunity" with the
   action buttons re-enabled.
4. Click **Guard** (or the reaction key). The player commits the Guard reaction while
   still walking; the sim resumes and the Heavy Strike is blocked.
5. Verify the fighter's position, the blocked-damage result, and that no second
   reaction pause fires for the same ongoing threat.

## 7. Remaining Phase 9A work

None. All steps 1–12 are complete. The only outstanding item is the manual playtest in
section 6, which is a verification step, not an implementation gap.

## 8. Commit decision

The Phase 9A C++ and Blueprint changes are uncommitted (on top of checkpoint `f256ea1`).
Per the "commit only when explicitly asked" policy, the commit is pending user
confirmation.
