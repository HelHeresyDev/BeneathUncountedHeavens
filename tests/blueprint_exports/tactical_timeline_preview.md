# Tactical Timeline Preview — Graph Connections Export

Exported via MCP `read_graph_dsl` after wiring the hypothetical combat timeline into
graphical PIE. The prediction infrastructure is the existing C++ `UBuhCombatSim`
(`GenerateTimeline`, snapshot getters, `GetNextSignificantEventIndex`); this export
documents the Blueprint connections that drive it.

## 1. WBP_CombatUI — TimelineSlider.OnValueChanged (scrubbing)

The slider value is clamped to a valid snapshot index, then the cyan/orange ghosts are
moved to that snapshot's X/Y with `Z = 90.0 + Snapshot.Height`, rotated to the
snapshot's facing, and the instant + event text is updated. Real fighters are untouched.

```
(event OnValueChanged(TimelineSlider) (Value)
  (bind _returnvalue (Math|Float|Truncate Value))
  ; clamp to [0, TimelineLength-1]
  (bind _returnvalue_13 (CombatSim|GetTimelineLength))
  (bind _returnvalue_15 (CombatSim|GetSnapshotPlayerX (Math|Float|Truncate _returnvalue_13)))
  (bind _returnvalue_16 (CombatSim|GetSnapshotOpponentX (Math|Float|Truncate _returnvalue_13)))
  (bind _returnvalue_17 (CombatSim|GetSnapshotPlayerY (Math|Float|Truncate _returnvalue_13)))
  (bind _returnvalue_18 (CombatSim|GetSnapshotOpponentY (Math|Float|Truncate _returnvalue_13)))
  (bind _returnvalue_19 (CombatSim|GetSnapshotPlayerFacing (Math|Float|Truncate _returnvalue_13)))
  (bind _returnvalue_20 (CombatSim|GetSnapshotOpponentFacing (Math|Float|Truncate _returnvalue_13)))
  (bind _returnvalue_21 (CombatSim|GetSnapshotInstantText (Math|Float|Truncate _returnvalue_13)))
  (bind _returnvalue_22 (CombatSim|GetSnapshotInfoText (Math|Float|Truncate _returnvalue_13)))
  (bind _outactors (Actor|GetAllActorsWithTag "GhostPlayer"))
  (bind _outactors_1 (Actor|GetAllActorsWithTag "GhostOpponent"))
  (Transformation|SetActorLocation (Utilities|Array|Get(acopy) _outactors)
      (Math|Vector|MakeVector _returnvalue_15 _returnvalue_17
          (+ 90.0 (CombatSim|GetSnapshotPlayerHeight (Math|Float|Truncate _returnvalue_13)))))
  (Transformation|SetActorLocation (Utilities|Array|Get(acopy) _outactors_1)
      (Math|Vector|MakeVector _returnvalue_16 _returnvalue_18
          (+ 90.0 (CombatSim|GetSnapshotOpponentHeight (Math|Float|Truncate _returnvalue_13)))))
  (Transformation|SetActorRotation (Utilities|Array|Get(acopy) _outactors)
      (Math|Rotator|MakeRotator 0.0 (select (== _returnvalue_19 1) 0.0 180.0)))
  (Transformation|SetActorRotation (Utilities|Array|Get(acopy) _outactors_1)
      (Math|Rotator|MakeRotator 0.0 (select (== _returnvalue_20 1) 0.0 180.0)))
  (Widget|SetText(Text) (Variables|WBP_CombatUI|GetTimelineInstantText)
      (Utilities|Text|ToText(String) _returnvalue_21))
  (Widget|SetText(Text) (Variables|WBP_CombatUI|GetPredictedOutcomeText)
      (Utilities|Text|ToText(String) _returnvalue_22)))
```

## 2. WBP_CombatUI — BtnNextEvent.OnClicked (jump to next significant event)

Reads the current slider index, asks C++ for the next significant event index, and, if
one exists (>= 0), moves the slider there (which re-runs the scrubbing handler above).

```
(event OnClicked(BtnNextEvent)
  (bind _timelineslider (Variables|WBP_CombatUI|GetTimelineSlider))
  (bind _returnvalue (Behavior|GetValue _timelineslider))
  (bind _returnvalue_1 (Math|Float|Truncate _returnvalue))
  (bind _returnvalue_2 (CombatSim|GetNextSignificantEventIndex _returnvalue_1))
  (if (>= _returnvalue_2 0)
    (Behavior|SetValue (Variables|WBP_CombatUI|GetTimelineSlider)
        (Math|Conversions|ToFloat(Integer) _returnvalue_2))))
```

The `BtnNextEvent` button and `TxtNextEvent` label were added to `BottomTimelineContent`
and the button's `OnClicked` delegate was bound via `UMGToolSet.BindToEventProperty`.

## 3. WBP_CombatUI — Hypothetical opponent buttons (single regeneration pathway)

All four opponent buttons share one pathway: set `HypotheticalOpponentMove`, refresh,
then call the single `RegeneratePredictionPreview` function. No hidden actual-opponent
fallback is used.

```
(event OnClicked(OpponentQuickButton)
  (Variables|Default|SetHypotheticalOpponentMove 0)
  (CallFunction|RefreshUI)
  ... opacity/text updates ...
  (CallFunction|RegeneratePredictionPreview))
; OpponentHeavyButton -> 1, OpponentGuardButton -> 2, OpponentDodgeButton -> 3
```

`RegeneratePredictionPreview` reads `HypotheticalOpponentMove` and the manager's
authoritative `CurrentState`, then calls the manager's `GeneratePreviewTimeline`, which
runs `UBuhCombatSim::GenerateTimeline` and stores the result in the global
`GCurrentTimeline` used by every snapshot getter.

## 4. BP_CombatManager — BeginPlay (clean-start preview state)

BeginPlay resets the encounter, then calls `InitCurrentState` so the first preview is
generated from the clean initial state.

```
(event EventBeginPlay
  ... create UI, set view target ...
  (CallFunction|ResetEncounter)
  (CallFunction|InitCurrentState))
```

`InitCurrentState` reads the real fighter positions/health/facing into the manager's
`CurrentState` struct without mutating the fighters. `GeneratePreviewTimeline` reads
`CurrentState` internally (working around the `CustomizableObjectInstance|GetCurrentState`
round-trip bug) and the hypothetical opponent move, then generates the timeline.

## 5. Preview isolation (no execution cross-contamination)

- Preview moves only the `GhostPlayer`/`GhostOpponent` actors; real `PlayerFighter` /
  `OpponentFighter` transforms, health, and the real Instant clock are untouched.
- `GCurrentTimeline` (preview) is separate from the Jump playback timeline.
- `MoveGhostsToInstant` uses the widget/manager world context, not
  `GEngine->GetWorldContexts()[0]`.

## 6. Automated test coverage (BUH.TimelinePreviewTests, 13 checks)

1. Regeneration changes with opponent response (quick vs heavy at in-range distance).
2. Snapshot index-to-Instant mapping.
3. Ghost XY/Z match the selected snapshot.
4. Jump height through ascent, apex, and descent (opponent Guarded to avoid the
   reaction pause cutting the flight short).
5. Walk and Dash predicted coordinates.
6. Health changes at hit Instants.
7. Guard, dodge (out-of-range evasion), and miss events.
8. Reaction snapshots stop at the correct decision boundary.
9. Next Event selects the correct subsequent event.
10. Changing opponent choice replaces the previous hypothetical timeline.
11. No mutation of authoritative CurrentState or real fighter actors.
12. Valid prediction from an existing airborne reaction state.
13. Reset clears all hypothetical ghosts and timeline UI.

All 13 pass; the full BUH suite is 23/23.

## 7. Graphical PIE verification (user-driven)

MCP cannot click UMG buttons or drag the slider, so the A–J sequence in `todo.md` §10
is confirmed manually in a fresh graphical PIE session:

A. Start a clean encounter.
B. Select Quick Strike and hypothetical opponent Heavy.
C. Move the timeline slider end-to-end; both ghosts update.
D. Select an Instant where an attack connects/misses; the event text is correct.
E. Switch the hypothetical opponent to Dodge; positions and event text change.
F. Select Walk or Dash and a destination; the predicted movement follows it.
G. Select Jump and scrub through its flight; the cyan ghost rises and descends.
H. Execute a real Jump, stop at a midair reaction, preview Continue; ghosts use the
   airborne state without moving the real fighter.
I. Continue the real Jump; normal landing.
J. Reset; no ghosts or stale timeline remain.
