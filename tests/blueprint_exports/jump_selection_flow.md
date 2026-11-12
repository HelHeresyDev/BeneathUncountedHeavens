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

### ExecuteMove (airborne Jump playback) — LIVE graph as of 745fe06
```
(fn ExecuteMove ()
  (bind _bhascurrentstate (|GetbHasCurrentState))
  (bind _opponentmove (Variables|Default|GetOpponentMove))
  (bind _returnvalue (select _bhascurrentstate _opponentmove (CombatSim|ChooseOpponentMove ...)))
  (bind _currentstate (Variables|Default|GetCurrentState))
  (bind _returnvalue_1 (select _bhascurrentstate _currentstate (CombatSim|MakeState ...)))
  (CallFunction|ClearDashAwaitingExecute)
  (if (== (Variables|Default|GetPlayerMove) -1)
    (Variables|Default|SetPredictedOutcome "")
    (else
      (Variables|Default|SetState 1)
      (if (not _bhascurrentstate)
        (Variables|Default|SetOpponentMove _returnvalue))
      (if (== (Variables|Default|GetPlayerMove) 9)
        (bind _returnvalue_6 (CombatSim|WithPlayerJumpTargetFromPreview _returnvalue_1))
        (Variables|Default|SetPreExecutionState _returnvalue_6)
        (else
          (Variables|Default|SetPreExecutionState _returnvalue_1)))
      (bind _returnvalue_6 (CombatSim|ExecuteMoveWithVars ...))
      (bind _returnvalue_7 (CombatSim|GetFinalState _returnvalue_6))
      (CombatSim|LogCombatState _returnvalue_7 "PostResolve")
      (Variables|Default|SetCurrentState _returnvalue_7)
      (|SetbHasCurrentState true)
      (Variables|Default|SetPlayerX ...)
      (Variables|Default|SetOpponentX ...)
      (Variables|Default|SetPlayerHealth ...)
      (Variables|Default|SetOpponentHealth ...)
      (if (or (== (Variables|Default|GetPlayerMove) 9) (> (CombatSim|GetJumpElapsedFlightInstants _returnvalue_1) 0))
        (Development|PrintString "JumpPlayback: skipping immediate player teleport")
        (else
          (Transformation|SetActorLocation (Variables|Default|GetPlayerFighter) ...)))
      (Transformation|SetActorLocation (Variables|Default|GetOpponentFighter) ...)
      (Variables|Default|SetPlayerY ...)
      (Variables|Default|SetLastPlayerMove ...)
      (bind _returnvalue_15 (CombatSim|IsReactionOpportunity _returnvalue_6))
      (if _returnvalue_15
        (|SetbReactionOpportunity true)
        (Variables|Default|SetReactionInstant ...)
        (Variables|Default|SetReactionFighter ...)
        (Variables|Default|SetObservedThreatAction ...)
        (Variables|Default|SetState 3)
        (else
          (|SetbReactionOpportunity false)
          ...state 0/2...))
      (Class|WBPCombatUI|RefreshUI (Variables|Default|GetUI))
      (CombatSim|ClearWalkPath (Game|GetPlayerController 0))
      (CombatSim|SetExecDashTarget -9999)
      (CombatSim|SetExecJumpTarget -9999)
      (Variables|Default|SetJumpAwaitingExecute false)
      (if (or (== (Variables|Default|GetPlayerMove) 9) (> (CombatSim|GetJumpElapsedFlightInstants _returnvalue_1) 0))
        (CombatSim|BeginJumpPlayback (Game|GetPlayerController 0) (Variables|Default|GetPlayerFighter) (Variables|Default|GetOpponentFighter) _returnvalue_1 (Variables|Default|GetPlayerMove) _returnvalue _returnvalue_6)))))
```

`ExecuteMove` stores the pre-execution state in `PreExecutionState`, applies `WithPlayerJumpTargetFromPreview` when the player committed a Jump, resolves through `ExecuteMoveWithVars`, sets `CurrentState` and `bHasCurrentState`, skips the player teleport for Jump/airborne states (the C++ playback system owns the fighter's position), moves the opponent, sets reaction state if applicable, and starts `BeginJumpPlayback` when `PlayerMove == 9` or the player was already airborne. Note: `SetJumpAwaitingExecute false` fires BEFORE the `BeginJumpPlayback` gate, but the gate checks `PlayerMove == 9` (not `JumpAwaitingExecute`), so the early clear does not block playback.

## BP_CombatManager — variables

- `JumpAwaitingExecute` (bool): true while a confirmed Jump is awaiting Execute.
  Set true by `SelectJumpDestination`, cleared by `ClearJumpAwaitingExecute`,
  `ExecuteMove`, and `ResetEncounter`.
- `PreExecutionState` (struct `BuhCombatState`): the authoritative combat state
  immediately before `ExecuteMove` resolves. Used to reconstruct the committed
  Jump parabola during playback.

## WBP_CombatUI — EventTick (Jump-relevant sections)

The full EventTick body after the airborne-Jump playback rewrite:

```
(event UserInterface|EventTick (MyGeometry InDeltaTime)
  (bind _timelineslider (Variables|WBP_CombatUI|GetTimelineSlider))
  (bind _returnvalue (Behavior|GetValue _timelineslider))
  (bind _bzero (Variables|Default|GetBZero))
  (bind _manager (Variables|Default|GetManager))
  (bind _state (StateMachine|GetState _manager))
  (bind _returnvalue_1 (== _state _bzero))
  (bind _targetingaction (Class|BPCombatManager|GetTargetingAction _manager))
  (bind _returnvalue_4 (CombatSim|TickJumpPlayback (Game|GetPlayerController 0) InDeltaTime))
  (IfThenElse_86 _returnvalue_4)
  (if (>= _targetingaction 0)
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
  (if (== (Class|BPCombatManager|GetPlayerMove _manager) 6)
    (bind _outactors (Actor|GetAllActorsWithTag "DestPlayer"))
    (CombatSim|DrawGroundLine (Game|GetPlayerController 0) (Transformation|GetActorLocation (Class|BPCombatManager|GetPlayerFighter _manager)) (Transformation|GetActorLocation (Utilities|Array|Get(acopy) _outactors)) 5.0 0.1))
  (bind _returnvalue_2 (Class|BPCombatManager|IsJumpAwaitingExecute _manager))
  (if (== (Class|BPCombatManager|GetPlayerMove _manager) 9)
    (bind _outactors (Actor|GetAllActorsWithTag "DestPlayer"))
    (Class|BPCombatManager|IsJumpAwaitingExecute _manager)
    (CombatSim|DrawJumpPath (Game|GetPlayerController 0) (Transformation|GetActorLocation (Class|BPCombatManager|GetPlayerFighter _manager)) (Transformation|GetActorLocation (Utilities|Array|Get(acopy) _outactors)) (Math|Integer|SelectInt 0 (CombatSim|GetJumpElapsedFlightInstants (CustomizableObjectInstance|GetCurrentState _manager)) _returnvalue_2)))
  (bind _returnvalue_3 (Class|BPCombatManager|IsDashAwaitingExecute _manager))
  (if (== (Class|BPCombatManager|GetPlayerMove _manager) 8)
    (bind _outactors (Actor|GetAllActorsWithTag "DestPlayer"))
    (Class|BPCombatManager|IsDashAwaitingExecute _manager)
    (CombatSim|DrawDashPath (Game|GetPlayerController 0) (Transformation|GetActorLocation (Class|BPCombatManager|GetPlayerFighter _manager)) (Transformation|GetActorLocation (Utilities|Array|Get(acopy) _outactors)) (Math|Integer|SelectInt 0 (CombatSim|GetDashElapsedMovementInstants (CustomizableObjectInstance|GetCurrentState _manager)) _returnvalue_3)))
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

- **Playback tick**: every tick calls `CombatSim|TickJumpPlayback` with the tick
  delta. While a Jump playback is active it advances the deterministic
  timeline, moves the fighter to the authoritative `WorldZ = 90 + Height`,
  redraws the remaining parabola, and stops at a reaction pause.
- **Targeting preview**: while `TargetingAction == 9`, `DrawJumpPath` renders the
  prospective parabola from the player fighter to the cursor landing point.
- **Confirmation**: left-click while `TargetingAction == 9` calls
  `SelectJumpDestination`, which sets `PlayerMove = 9` and
  `JumpAwaitingExecute = true`.
- **Cancel**: right-click/Escape calls `CancelTargeting`,
  `ClearDashAwaitingExecute`, and `ClearJumpAwaitingExecute`.
- **Confirmed Jump path**: while `PlayerMove == 9`, `DrawJumpPath` renders the
  committed parabola. The elapsed flight instants argument is
  `SelectInt(0, GetJumpElapsedFlightInstants, IsJumpAwaitingExecute)`, so the
  path stays fully red (elapsed 0) until Execute, then shows the real red/green
  split during flight.
- **Confirmed Dash path**: same pattern with `PlayerMove == 8` and
  `SelectInt(0, GetDashElapsedMovementInstants, IsDashAwaitingExecute)`.

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
