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

### ExecuteMove (corrected Jump execution)
```
(fn ExecuteMove ()
  (bind _bhascurrentstate (|GetbHasCurrentState))
  (bind _opponentmove (Variables|Default|GetOpponentMove))
  (bind _returnvalue (select _bhascurrentstate _opponentmove (CombatSim|ChooseOpponentMove (select _bhascurrentstate (Variables|Default|GetCurrentState) (CombatSim|MakeState (Variables|Default|GetPlayerX) (Variables|Default|GetOpponentX) (Variables|Default|GetPlayerHealth) (Variables|Default|GetOpponentHealth) (Variables|Default|GetSeed) (Variables|Default|GetPlayerY))) (Variables|Default|GetLastPlayerMove) (Variables|Default|GetSeed))))
  (bind _playery (Variables|Default|GetPlayerY))
  (bind _returnvalue_1 (CombatSim|MakeState (Variables|Default|GetPlayerX) (Variables|Default|GetOpponentX) (Variables|Default|GetPlayerHealth) (Variables|Default|GetOpponentHealth) (Variables|Default|GetSeed) (Variables|Default|GetPlayerY)))
  (bind _returnvalue_2 (CombatSim|ChooseOpponentMove (select _bhascurrentstate (Variables|Default|GetCurrentState) _returnvalue_1) (Variables|Default|GetLastPlayerMove) (Variables|Default|GetSeed)))
  (|SetbDashAwaitingExecute false)
  (if (== (Variables|Default|GetPlayerMove) -1)
    (Variables|Default|SetPredictedOutcome "")
    (else
      (Variables|Default|SetState 1)
      (if (not _bhascurrentstate)
        (Variables|Default|SetOpponentMove _returnvalue))
      (bind _returnvalue_3 (CombatSim|MakeState (Variables|Default|GetPlayerX) (Variables|Default|GetOpponentX) (Variables|Default|GetPlayerHealth) (Variables|Default|GetOpponentHealth) (Variables|Default|GetSeed) _playery))
      (bind _returnvalue_4 (CombatSim|ExecuteMoveWithVars _bhascurrentstate (select _bhascurrentstate (Variables|Default|GetCurrentState) _returnvalue_3) (Variables|Default|GetPlayerX) (Variables|Default|GetOpponentX) (Variables|Default|GetPlayerHealth) (Variables|Default|GetOpponentHealth) (Variables|Default|GetSeed) (Variables|Default|GetPlayerMove) _returnvalue (Variables|Default|GetPlayerWalkTarget) (Variables|Default|GetPlayerWalkTargetY) _playery 0))
      (bind _returnvalue_5 (CombatSim|GetFinalState _returnvalue_4))
      (CombatSim|LogCombatState _returnvalue_5 "PostResolve")
      (Variables|Default|SetCurrentState _returnvalue_5)
      (|SetbHasCurrentState true)
      (bind _returnvalue_6 (CombatSim|GetFinalPlayerX _returnvalue_4))
      (bind _returnvalue_7 (CombatSim|GetFinalOpponentX _returnvalue_4))
      (bind _returnvalue_8 (CombatSim|GetFinalPlayerHealth _returnvalue_4))
      (bind _returnvalue_9 (CombatSim|GetFinalOpponentHealth _returnvalue_4))
      (Variables|Default|SetPlayerX _returnvalue_6)
      (Variables|Default|SetOpponentX _returnvalue_7)
      (Variables|Default|SetPlayerHealth _returnvalue_8)
      (Variables|Default|SetOpponentHealth _returnvalue_9)
      (bind _returnvalue_10 (CombatSim|GetFinalPlayerY _returnvalue_4))
      (Transformation|SetActorLocation (Variables|Default|GetPlayerFighter) (Math|Vector|MakeVector (Math|Conversions|ToFloat(Integer) _returnvalue_6) _returnvalue_10 90))
      (bind _returnvalue_11 (CombatSim|GetFinalOpponentY _returnvalue_4))
      (Transformation|SetActorLocation (Variables|Default|GetOpponentFighter) (Math|Vector|MakeVector (Math|Conversions|ToFloat(Integer) _returnvalue_7) _returnvalue_11 90))
      (bind _returnvalue_12 (CombatSim|GetFinalPlayerY _returnvalue_4))
      (Variables|Default|SetPlayerY _returnvalue_12)
      (Variables|Default|SetLastPlayerMove (Variables|Default|GetPlayerMove))
      (bind _returnvalue_13 (CombatSim|IsReactionOpportunity _returnvalue_4))
      (if _returnvalue_13
        (|SetbReactionOpportunity true)
        (bind _returnvalue_14 (CombatSim|GetReactionInstant _returnvalue_4))
        (Variables|Default|SetReactionInstant _returnvalue_14)
        (bind _returnvalue_15 (CombatSim|GetReactionFighter _returnvalue_4))
        (Variables|Default|SetReactionFighter _returnvalue_15)
        (bind _returnvalue_16 (CombatSim|GetObservedThreatAction _returnvalue_4))
        (Variables|Default|SetObservedThreatAction _returnvalue_16)
        (Variables|Default|SetState 3)
        (else
          (|SetbReactionOpportunity false)
          (if (<= _returnvalue_8 0)
            (Variables|Default|SetState 2)
            (elif (<= _returnvalue_9 0)
              (Variables|Default|SetState 2)
              (else
                (Variables|Default|SetState 0))))))
      (Class|WBPCombatUI|RefreshUI (Variables|Default|GetUI))
      (CombatSim|ClearWalkPath (Game|GetPlayerController 0))
      (CombatSim|SetExecDashTarget -9999 0)
      (CombatSim|SetExecJumpTarget -9999 0)
      (Variables|Default|SetJumpAwaitingExecute false))))
```

The corrected `ExecuteMove` calls the fully implemented `CombatSim|ExecuteMoveWithVars` helper instead of the broken `CombatSim|ExecuteMove` UFUNCTION. `ExecuteMoveWithVars` reads the live `GExecJumpTarget` / `GExecJumpTargetY` globals set by `SelectJumpDestination`, applies the Jump destination when `PlayerMove == 9`, then resolves combat. Stale exec targets are cleared after execution.

## BP_CombatManager — variables

- `JumpAwaitingExecute` (bool): true while a confirmed Jump is awaiting Execute.
  Set true by `SelectJumpDestination`, cleared by `ClearJumpAwaitingExecute`,
  `ExecuteMove`, and `ResetEncounter`.

## WBP_CombatUI — EventTick (Jump-relevant sections)

The full EventTick body after the Jump rewrite:

```
(event UserInterface|EventTick (MyGeometry InDeltaTime)
  (bind _timelineslider (Variables|WBP_CombatUI|GetTimelineSlider))
  (bind _returnvalue (Behavior|GetValue _timelineslider))
  (bind _bzero (Variables|Default|GetBZero))
  (bind _manager (Variables|Default|GetManager))
  (bind _state (StateMachine|GetState _manager))
  (bind _returnvalue_1 (== _state _bzero))
  (bind _targetingaction (Class|BPCombatManager|GetTargetingAction _manager))
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
