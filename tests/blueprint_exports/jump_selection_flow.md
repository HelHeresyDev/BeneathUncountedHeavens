# Jump Selection Flow — Blueprint Export

Exported via MCP `read_graph_dsl` after the Jump implementation. These are the
authoritative saved graph states for the Jump movement action.

## BP_CombatManager — Jump functions

### BeginJumpTargeting
```
(fn BeginJumpTargeting ()
  (Variables|Default|SetJumpAwaitingExecute false)
  (Variables|Default|SetTargetingAction 9))
```

### SelectJumpDestination
```
(fn SelectJumpDestination (TargetX TargetY)
  (CombatSim|SetPreviewJumpTarget TargetX TargetY)
  (CombatSim|SetExecJumpTarget TargetX TargetY)
  (Variables|Default|SetPlayerMove 9)
  (Variables|Default|SetJumpAwaitingExecute true)
  (CallFunction|BuildPredictions)
  (Class|WBPCombatUI|RefreshUI (Variables|Default|GetUI)))
```

### ClearJumpAwaitingExecute
```
(fn ClearJumpAwaitingExecute ()
  (Variables|Default|SetJumpAwaitingExecute false))
```

### IsJumpAwaitingExecute
```
(fn IsJumpAwaitingExecute ()
  (return (Variables|Default|GetJumpAwaitingExecute)))
```

### ExecuteMove (airborne Jump playback) — LIVE graph after the JumpTarget state-handoff fix
```
(fn ExecuteMove ()
  (Variables|Default|SetOpponentMove 1)
  (CallFunction|ClearDashAwaitingExecute)
  (if (== (Variables|Default|GetPlayerMove) -1)
    (Variables|Default|SetPredictedOutcome "")
    (else
      (Variables|Default|SetState 1)
      (bind _returnvalue (CombatSim|MakeState (Variables|Default|GetPlayerX) (Variables|Default|GetOpponentX) (Variables|Default|GetPlayerHealth) (Variables|Default|GetOpponentHealth) (Variables|Default|GetSeed) (Variables|Default|GetPlayerY)))
      (if (== (Variables|Default|GetPlayerMove) 9)
        (bind _returnvalue_1 (CombatSim|WithPlayerJumpTargetFromPreview _returnvalue))
        (Variables|Default|SetPreExecutionState _returnvalue_1)
        (else
          (Variables|Default|SetPreExecutionState _returnvalue)))
      (bind _returnvalue_1 (CombatSim|ExecuteMoveWithVars false _returnvalue (Variables|Default|GetPlayerX) (Variables|Default|GetOpponentX) (Variables|Default|GetPlayerHealth) (Variables|Default|GetOpponentHealth) (Variables|Default|GetSeed) (Variables|Default|GetPlayerMove) 1 (Variables|Default|GetPlayerWalkTarget) (Variables|Default|GetPlayerWalkTargetY) (Variables|Default|GetPlayerY)))
      (bind _returnvalue_2 (CombatSim|GetFinalState _returnvalue_1))
      (CombatSim|LogCombatState _returnvalue_2 "PostResolve")
      (bind _returnvalue_3 (CombatSim|GetFinalPlayerX _returnvalue_1))
      (bind _returnvalue_4 (CombatSim|GetFinalOpponentX _returnvalue_1))
      (bind _returnvalue_5 (CombatSim|GetFinalPlayerHealth _returnvalue_1))
      (bind _returnvalue_6 (CombatSim|GetFinalOpponentHealth _returnvalue_1))
      (Variables|Default|SetPlayerX _returnvalue_3)
      (Variables|Default|SetOpponentX _returnvalue_4)
      (Variables|Default|SetPlayerHealth _returnvalue_5)
      (Variables|Default|SetOpponentHealth _returnvalue_6)
      (bind _returnvalue_7 (CombatSim|GetFinalPlayerY _returnvalue_1))
      (if (or (== (Variables|Default|GetPlayerMove) 9) (> (CombatSim|GetJumpElapsedFlightInstants _returnvalue) 0))
        (Development|PrintString "JumpPlayback: skipping immediate player teleport")
        (else
          (Transformation|SetActorLocation (Variables|Default|GetPlayerFighter) (Math|Vector|MakeVector (Math|Conversions|ToFloat(Integer) _returnvalue_3) _returnvalue_7 (CombatSim|GetFinalPlayerWorldZ _returnvalue_1)))))
      (bind _returnvalue_8 (CombatSim|GetFinalOpponentY _returnvalue_1))
      (Transformation|SetActorLocation (Variables|Default|GetOpponentFighter) (Math|Vector|MakeVector (Math|Conversions|ToFloat(Integer) _returnvalue_4) _returnvalue_8 (CombatSim|GetFinalOpponentWorldZ _returnvalue_1)))
      (Variables|Default|SetPlayerY _returnvalue_7)
      (Variables|Default|SetLastPlayerMove (Variables|Default|GetPlayerMove))
      (bind _returnvalue_9 (CombatSim|IsReactionOpportunity _returnvalue_1))
      (if _returnvalue_9
        (bind _returnvalue_10 (CombatSim|GetReactionInstant _returnvalue_1))
        (Variables|Default|SetReactionInstant _returnvalue_10)
        (bind _returnvalue_11 (CombatSim|GetReactionFighter _returnvalue_1))
        (Variables|Default|SetReactionFighter _returnvalue_11)
        (bind _returnvalue_12 (CombatSim|GetObservedThreatAction _returnvalue_1))
        (Variables|Default|SetObservedThreatAction _returnvalue_12)
        (Variables|Default|SetState 3)
        (elif (<= _returnvalue_5 0)
          (Variables|Default|SetState 2)
          (elif (<= _returnvalue_6 0)
            (Variables|Default|SetState 2)
            (else
              (Variables|Default|SetState 0)))))
      (Class|WBPCombatUI|RefreshUI (Variables|Default|GetUI))
      (CombatSim|ClearWalkPath (Game|GetPlayerController 0))
      (CombatSim|SetExecDashTarget -9999)
      (CombatSim|SetExecJumpTarget -9999)
      (Variables|Default|SetJumpAwaitingExecute false)
      (if (or (== (Variables|Default|GetPlayerMove) 9) (> (CombatSim|GetJumpElapsedFlightInstants _returnvalue) 0))
        (CombatSim|BeginJumpPlayback (Game|GetPlayerController 0) (Variables|Default|GetPlayerFighter) (Variables|Default|GetOpponentFighter) (Variables|Default|GetPreExecutionState) (Variables|Default|GetPlayerMove) 1 _returnvalue_1)))))
```

`ExecuteMove` builds the original state via `MakeState`, applies `WithPlayerJumpTargetFromPreview` when the player committed a Jump (storing the committed destination in `PreExecutionState`), resolves through `ExecuteMoveWithVars false` (which applies `GExecJumpTarget` for the real destination), skips the player teleport for Jump/airborne states (the C++ playback system owns the fighter's position), moves the opponent, sets reaction state if applicable, and starts `BeginJumpPlayback` when `PlayerMove == 9` or the player was already airborne. The `BeginJumpPlayback` call receives `(Variables|Default|GetPreExecutionState)` — the committed state with the real JumpTarget — so the resolver and the visual playback start from the SAME state. Previously it received the original `_returnvalue` (JumpTarget = -9999), which made `PrepareJumpCommit` substitute the takeoff and jump in place. `SetJumpAwaitingExecute false` fires BEFORE the `BeginJumpPlayback` gate, but the gate checks `PlayerMove == 9` (not `JumpAwaitingExecute`), so the early clear does not block playback.

## BP_CombatManager — variables

- `JumpAwaitingExecute` (bool): true while a confirmed Jump is awaiting Execute.
  Set true by `SelectJumpDestination`, cleared by `ClearJumpAwaitingExecute`,
  `ExecuteMove`, and `ResetEncounter`.
- `PreExecutionState` (struct `BuhCombatState`): the authoritative combat state
  immediately before `ExecuteMove` resolves. Used to reconstruct the committed
  Jump parabola during playback.

## WBP_CombatUI — EventTick (Jump-relevant sections)

The full EventTick body after the playback-ownership fix. The WBP no longer calls
`TickJumpPlayback` (the manager EventTick is the sole playback driver), and every
movement-path draw is gated on `State == 0` (`_returnvalue_1`) so the WBP never
overwrites the C++ playback's authoritative remaining path during execution or a
reaction pause. The manager `State`/`CurrentState` getters use the `Class|` prefix
(`StateMachine|GetState` is ambiguous and `Variables|BPCombatManager|GetState` does
not exist).

```
(event UserInterface|EventTick (MyGeometry InDeltaTime)
  (bind _timelineslider (Variables|WBP_CombatUI|GetTimelineSlider))
  (bind _returnvalue (Behavior|GetValue _timelineslider))
  (bind _bzero (Variables|Default|GetBZero))
  (bind _manager (Variables|Default|GetManager))
  (bind _state (Class|BPCombatManager|GetState _manager))
  (bind _returnvalue_1 (== _state _bzero))
  (bind _targetingaction (Class|BPCombatManager|GetTargetingAction _manager))
  (if (and _returnvalue_1 (>= _targetingaction 0))
    (bind _returnvalue_2 (CombatSim|GetGroundHitX (Game|GetPlayerController 0)))
    (bind _returnvalue_3 (CombatSim|GetGroundHitY (Game|GetPlayerController 0)))
    (bind _outactors (Actor|GetAllActorsWithTag "DestPlayer"))
    (Rendering|SetActorHiddenInGame (Utilities|Array|Get(acopy) _outactors))
    (Transformation|SetActorLocation (Utilities|Array|Get(acopy) _outactors) (Math|Vector|MakeVector _returnvalue_2 _returnvalue_3 90.0))
    (if (== _targetingaction 8)
      (CombatSim|DrawDashPath (Game|GetPlayerController 0) (Transformation|GetActorLocation (Class|BPCombatManager|GetPlayerFighter _manager)) (Math|Vector|MakeVector _returnvalue_2 _returnvalue_3 95.0))
      (elif (== _targetingaction 9)
        (CombatSim|DrawJumpPath (Game|GetPlayerController 0) (Transformation|GetActorLocation (Class|BPCombatManager|GetPlayerFighter _manager)) (Math|Vector|MakeVector _returnvalue_2 _returnvalue_3 95.0))
      (else
        (CombatSim|DrawGroundLine (Game|GetPlayerController 0) (Transformation|GetActorLocation (Class|BPCombatManager|GetPlayerFighter _manager)) (Math|Vector|MakeVector _returnvalue_2 _returnvalue_3 95.0) 5.0 0.1))))
    (if (Game|Player|WasInputKeyJustPressed (Game|GetPlayerController 0) "LeftMouseButton")
      (if (== _targetingaction 8)
        (Class|BPCombatManager|SelectDashDestination _manager _returnvalue_2 _returnvalue_3)
        (elif (== _targetingaction 9)
          (Class|BPCombatManager|SelectJumpDestination _manager _returnvalue_2 _returnvalue_3)
        (else
          (Class|BPCombatManager|SelectWalkDestination _manager _returnvalue_2 _returnvalue_3))))
      (Class|BPCombatManager|CancelTargeting _manager)
      (Widget|SetVisibility (Variables|WBP_CombatUI|GetWalkDistanceText) "Hidden"))
    (if (or (Game|Player|WasInputKeyJustPressed (Game|GetPlayerController 0) "RightMouseButton") (Game|Player|WasInputKeyJustPressed (Game|GetPlayerController 0) "Escape"))
      (Class|BPCombatManager|CancelTargeting _manager)
      (Class|BPCombatManager|ClearDashAwaitingExecute (Variables|Default|GetManager))
      (Class|BPCombatManager|ClearJumpAwaitingExecute (Variables|Default|GetManager))
      (Widget|SetVisibility (Variables|WBP_CombatUI|GetWalkDistanceText) "Hidden")
      (bind _outactors_1 (Actor|GetAllActorsWithTag "DestPlayer"))
      (Rendering|SetActorHiddenInGame (Utilities|Array|Get(acopy) _outactors_1) true)
      (CombatSim|ClearWalkPath (Game|GetPlayerController 0))))
  (if (and _returnvalue_1 (== (Class|BPCombatManager|GetPlayerMove _manager) 6))
    (bind _outactors (Actor|GetAllActorsWithTag "DestPlayer"))
    (CombatSim|DrawGroundLine (Game|GetPlayerController 0) (Transformation|GetActorLocation (Class|BPCombatManager|GetPlayerFighter _manager)) (Transformation|GetActorLocation (Utilities|Array|Get(acopy) _outactors)) 5.0 0.1))
  (bind _returnvalue_2 (Class|BPCombatManager|IsJumpAwaitingExecute _manager))
  (if (and _returnvalue_1 (== (Class|BPCombatManager|GetPlayerMove _manager) 9))
    (bind _outactors (Actor|GetAllActorsWithTag "DestPlayer"))
    (Class|BPCombatManager|IsJumpAwaitingExecute _manager)
    (CombatSim|DrawJumpPath (Game|GetPlayerController 0) (Transformation|GetActorLocation (Class|BPCombatManager|GetPlayerFighter _manager)) (Transformation|GetActorLocation (Utilities|Array|Get(acopy) _outactors)) (Math|Integer|SelectInt 0 (CombatSim|GetJumpElapsedFlightInstants (Class|BPCombatManager|GetCurrentState _manager)) _returnvalue_2)))
  (bind _returnvalue_3 (Class|BPCombatManager|IsDashAwaitingExecute _manager))
  (if (and _returnvalue_1 (== (Class|BPCombatManager|GetPlayerMove _manager) 8))
    (bind _outactors (Actor|GetAllActorsWithTag "DestPlayer"))
    (Class|BPCombatManager|IsDashAwaitingExecute _manager)
    (CombatSim|DrawDashPath (Game|GetPlayerController 0) (Transformation|GetActorLocation (Class|BPCombatManager|GetPlayerFighter _manager)) (Transformation|GetActorLocation (Utilities|Array|Get(acopy) _outactors)) (Math|Integer|SelectInt 0 (CombatSim|GetDashElapsedMovementInstants (Class|BPCombatManager|GetCurrentState _manager)) _returnvalue_3)))
  (if (and _returnvalue_1 (== (Class|BPCombatManager|GetPlayerPhase _manager) 0))
    (if (Game|Player|WasInputKeyJustPressed (Game|GetPlayerController 0) "One")
      (Class|BPCombatManager|SelectQuick _manager))
    (if (Game|Player|WasInputKeyJustPressed (Game|GetPlayerController 0) "Two")
      (Class|BPCombatManager|SelectHeavy _manager))
    (if (Game|Player|WasInputKeyJustPressed (Game|GetPlayerController 0) "Three")
      (Class|BPCombatManager|SelectGuard _manager))
    (if (Game|Player|WasInputKeyJustPressed (Game|GetPlayerController 0) "Four")
      (Class|BPCombatManager|SelectDodge _manager)))
  (if (and _returnvalue_1 (Game|Player|WasInputKeyJustPressed (Game|GetPlayerController 0) "Enter"))
    (Class|BPCombatManager|ExecuteMove _manager))
  (if (and (== _state 2) (Game|Player|WasInputKeyJustPressed (Game|GetPlayerController 0) "R"))
    (Class|BPCombatManager|ResetEncounter _manager))
  (if (Game|Player|WasInputKeyJustPressed (Game|GetPlayerController 0) "Left")
    (Behavior|SetValue (Variables|WBP_CombatUI|GetTimelineSlider) (- _returnvalue 1.0)))
  (if (Game|Player|WasInputKeyJustPressed (Game|GetPlayerController 0) "Right")
    (Behavior|SetValue (Variables|WBP_CombatUI|GetTimelineSlider) (+ _returnvalue 1.0))))
```

### Jump-specific behavior in EventTick

- **No playback tick**: the WBP no longer calls `CombatSim|TickJumpPlayback`.
  `BP_CombatManager.EventTick` is the sole playback driver (one
  `TickJumpPlayback(DeltaSeconds)` per frame). While a Jump playback is active it
  advances the deterministic timeline, moves the fighter to the authoritative
  `WorldZ = 90 + Height`, redraws the remaining parabola via
  `DrawJumpPathFromState`, and stops at a reaction pause.
- **Exclusive path rendering**: every WBP movement-path draw is gated on
  `State == 0` (Planning). While playback owns the path (State 1/Resolving) or a
  reaction is pending (State 3/Reaction), the WBP draws no movement path, so it
  cannot overwrite the C++ playback's authoritative remaining trajectory with
  stale `DestPlayer`/targeting coordinates.
- **Targeting preview**: while `State == 0` and `TargetingAction == 9`,
  `DrawJumpPath` renders the prospective parabola from the player fighter to the
  cursor landing point.
- **Confirmation**: left-click while `TargetingAction == 9` calls
  `SelectJumpDestination`, which sets `PlayerMove = 9` and
  `JumpAwaitingExecute = true`. This is the single authoritative
  targeting-confirmation path (the manager's `HandleTargetingTick` debug handler
  was removed).
- **Cancel**: right-click/Escape calls `CancelTargeting`,
  `ClearDashAwaitingExecute`, and `ClearJumpAwaitingExecute`.
- **Confirmed Jump path**: while `State == 0` and `PlayerMove == 9`,
  `DrawJumpPath` renders the committed parabola. The elapsed flight instants
  argument is `SelectInt(0, GetJumpElapsedFlightInstants, IsJumpAwaitingExecute)`,
  so the path stays fully red (elapsed 0) until Execute, then shows the real
  red/green split during flight.
- **Confirmed Dash path**: same pattern with `State == 0` and `PlayerMove == 8`
  and `SelectInt(0, GetDashElapsedMovementInstants, IsDashAwaitingExecute)`.

### OnClicked(BtnJumpMove) (corrected cursor)
```
(event OnClicked(BtnJumpMove)
  (bind _returnvalue (Game|GetPlayerController 0))
  (Input|SetInputModeGameAndUI _returnvalue 0 "DoNotLock" false)
  (|SetbShowMouseCursor true _returnvalue)
  (CombatSim|LogCursorState (Game|GetPlayerController 0) "JumpAfterInput")
  (Class|BPCombatManager|BeginJumpTargeting (Variables|Default|GetManager))
  (Widget|SetVisibility (Variables|WBP_CombatUI|GetDestSlider) "Hidden")
  (Widget|SetVisibility (Variables|WBP_CombatUI|GetWalkDistanceText))
  (Widget|SetText(Text) (Variables|WBP_CombatUI|GetWalkDistanceText) "NSLOCTEXT(\"\", \"01A1BDB42AD07E348286B94D6ADA6096\", \"Jump Targeting: Click to set destination, Right-click to cancel\")")
  (Widget|SetVisibility (Variables|WBP_CombatUI|GetTechniqueDetailsText) "Hidden"))
```

## WBP_CombatUI — widgets

- `BtnJumpMove` (Button, variable): child index 10 of `LeftPanelContent`,
  background color `{r:0.18, g:0.35, b:0.65, a:1.0}`.
- `TxtJumpMove` (TextBlock, variable): inside `BtnJumpMove`, text `Jump`.
- `OnClicked(BtnJumpMove)` event: sets input mode to GameAndUI, explicitly
  calls `SetShowMouseCursor true`, logs cursor state, calls
  `BeginJumpTargeting`, hides `DestSlider`, shows `WalkDistanceText`, sets
  targeting text, hides `TechniqueDetailsText`.
- `EventConstruct` sets `TxtJumpMove` text to `Jump` and hides `BtnJumpMove`.
- `OnClicked(BtnMove)` shows `BtnJumpMove`.
- Nine events that hide `BtnDashMove` also hide `BtnJumpMove`.
