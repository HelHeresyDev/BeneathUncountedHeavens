# Dash Selection Flow — Blueprint Export

Evidence for the Dash-lifecycle fix. All graphs below were read back from the saved
assets with `BlueprintTools.read_graph_dsl` after the final compile.

Assets:
- `/Game/BUH/Prototypes/TacticalDuel/Blueprints/BP_CombatManager`
- `/Game/BUH/Prototypes/TacticalDuel/Blueprints/WBP_CombatUI`

C++ resolver (unchanged, pure):
- `UBuhCombatSim::GetDashElapsedMovementInstants(const FBuhCombatState&)` returns
  `F.Action == 8 && F.Phase == 2 ? F.PhaseInstants : 0`. No global flag.

## BP_CombatManager function graphs

### BeginDashTargeting
```lisp
(fn BeginDashTargeting ()
  (|SetbDashAwaitingExecute false)
  (Variables|Default|SetTargetingAction 8))
```
Writes: `bDashAwaitingExecute = false`, `TargetingAction = 8`.

### SelectDashDestination
```lisp
(fn SelectDashDestination (TargetX TargetY)
  (CombatSim|SetPreviewDashTarget TargetX TargetY)
  (CombatSim|SetExecDashTarget TargetX TargetY)
  (Variables|Default|SetPlayerMove 8)
  (|SetbDashAwaitingExecute true)
  (CallFunction|BuildPredictions)
  (Class|WBPCombatUI|RefreshUI (Variables|Default|GetUI)))
```
Writes: preview/exec dash targets, `PlayerMove = 8`, `bDashAwaitingExecute = true`.
Then rebuilds predictions and refreshes the UI.

### CancelTargeting
```lisp
(fn CancelTargeting ()
  (Variables|Default|SetTargetingAction -1))
```
Writes: `TargetingAction = -1`. The WBP right-click/Escape branch additionally calls
`ClearDashAwaitingExecute` to drop the pending-execute flag.

### ClearDashAwaitingExecute
```lisp
(fn ClearDashAwaitingExecute ()
  (|SetbDashAwaitingExecute false))
```
Writes: `bDashAwaitingExecute = false`.

### IsDashAwaitingExecute
```lisp
(fn IsDashAwaitingExecute ()
  (return (|GetbDashAwaitingExecute)))
```
Returns the instance flag. This is the only source the UI uses to decide whether a
confirmed-but-not-yet-executed Dash should draw red.

### ExecuteMove
```lisp
(fn ExecuteMove ()
  (bind _bhascurrentstate (|GetbHasCurrentState))
  (bind _opponentmove (Variables|Default|GetOpponentMove))
  (bind _returnvalue (select _bhascurrentstate _opponentmove (CombatSim|ChooseOpponentMove ...)))
  (bind _playery (Variables|Default|GetPlayerY))
  (|SetbDashAwaitingExecute false)
  ...
  (Variables|Default|SetState 1)
  (CombatSim|WithPlayerWalkTarget ...)
  (CombatSim|ResolveCombat ... 200)
  ...
  (|SetbHasCurrentState true)
  ...
  (if (not _bhascurrentstate) (Variables|Default|SetOpponentMove _returnvalue))
  ...
  (Variables|Default|SetState 0/2/3)
  (Class|WBPCombatUI|RefreshUI (Variables|Default|GetUI))
  (CombatSim|ClearWalkPath (Game|GetPlayerController 0)))
```
Key write: `bDashAwaitingExecute = false` before resolution. The commit period then
starts fresh from the resolved `FBuhCombatState`; the UI reads elapsed movement
instantly from that state.

### ResetEncounter
```lisp
(fn ResetEncounter ()
  ...
  (|SetbHasCurrentState false)
  (|SetbReactionOpportunity false)
  ...
  (CallFunction|CancelTargeting)
  (|SetbDashAwaitingExecute false)
  ...)
```
Writes: all combat state to defaults, `bDashAwaitingExecute = false`.

### EventTick
```lisp
(event EventTick (DeltaSeconds)
  (Variables|Default|SetInstantAccumulator (+ (Variables|Default|GetInstantAccumulator) DeltaSeconds))
  (if (>= (Variables|Default|GetInstantAccumulator) 0.05)
    (Variables|Default|SetInstantAccumulator (- (Variables|Default|GetInstantAccumulator) 0.05))
    (if (== (Variables|Default|GetPlayerPhase) 2)
      (if (> (Variables|Default|GetPlayerInstantsinPhase) 0)
        (Variables|Default|SetPlayerInstantsinPhase (- (Variables|Default|GetPlayerInstantsinPhase) 1))
        (else (Variables|Default|SetPlayerPhase 0))))
    (if (== (Variables|Default|GetOpponentPhase) 2)
      ...)))
```
This is the instant clock. While a Dash is in Phase 2 it decrements
`PlayerInstantsinPhase` once per 0.05 s tick. `GetDashElapsedMovementInstants` reads
that counter, so the green portion of a confirmed Dash grows over time without any
extra UI state.

### BeginPlay
```lisp
(event EventBeginPlay
  ...
  (Variables|Default|SetState 0)
  (Variables|Default|SetOpponentMove 1)
  ...
  (CallFunction|ResetEncounter))
```
Initialises the encounter and calls `ResetEncounter`, which clears
`bDashAwaitingExecute`.

## WBP_CombatUI EventGraph — Dash-relevant branches

### Active targeting (mouse-down aiming)
```lisp
(if (>= _targetingaction 0)
  ...
  (if (== _targetingaction 8)
    (CombatSim|DrawDashPath (Game|GetPlayerController 0)
      (Transformation|GetActorLocation (Class|BPCombatManager|GetPlayerFighter _manager))
      (Math|Vector|MakeVector _returnvalue_3 _returnvalue_4 95.0))
    (else (CombatSim|DrawGroundLine ...)))
  (if (Game|Player|WasInputKeyJustPressed ... "LeftMouseButton")
    (if (== _targetingaction 8)
      (Class|BPCombatManager|SelectDashDestination _manager _returnvalue_3 _returnvalue_4)
      (else (Class|BPCombatManager|SelectWalkDestination ...)))
    (Class|BPCombatManager|CancelTargeting _manager)
    ...)
  (if (or RightMouseButton Escape)
    (Class|BPCombatManager|CancelTargeting _manager)
    (Class|BPCombatManager|ClearDashAwaitingExecute (Variables|Default|GetManager))
    ...))
```
- While `TargetingAction == 8`, the preview line is drawn with
  `DrawDashPath(..., Start, End)` — the elapsed-movement pin is left unconnected, so
  it defaults to `0`. `DrawDashPath` then locks the first 3 movement instants red and
  colours the rest green. A short Dash (≤ 270 uu) is therefore entirely red.
- Left-click calls `SelectDashDestination`, which sets `PlayerMove = 8` and
  `bDashAwaitingExecute = true`.
- Right-click / Escape calls `CancelTargeting` **and** `ClearDashAwaitingExecute`, so
  the pending-execute flag cannot leak into the next encounter.

### Confirmed Dash, before Execute (the lifecycle fix)
```lisp
(Math|Integer|CompareInt _targetingaction 0
  (:<
    (if (== (Class|BPCombatManager|GetPlayerMove _manager) 8)
      (bind _outactors (Actor|GetAllActorsWithTag "DestPlayer"))
      (bind _returnvalue_3 (Class|BPCombatManager|IsDashAwaitingExecute _manager))
      (CombatSim|DrawDashPath (Game|GetPlayerController 0)
        (Transformation|GetActorLocation (Class|BPCombatManager|GetPlayerFighter _manager))
        (Transformation|GetActorLocation (Utilities|Array|Get(acopy) _outactors))
        (Math|Integer|SelectInt 0
          (CombatSim|GetDashElapsedMovementInstants (CustomizableObjectInstance|GetCurrentState _manager))
          _returnvalue_3)))
    ...))
```
- After `SelectDashDestination` returns, `TargetingAction` is `-1`, so this branch
  runs every tick while `PlayerMove == 8`.
- Elapsed movement is `SelectInt(0, GetDashElapsedMovementInstants(CurrentState),
  IsDashAwaitingExecute)`:
  - `IsDashAwaitingExecute == true` (confirmed, not yet executed) → `0` → the whole
    path draws red, even if the old Dash had already moved some instants.
  - `IsDashAwaitingExecute == false` (executed, or a reaction-pause continuation) →
    the authoritative `GetDashElapsedMovementInstants` value → the already-travelled
    prefix draws green and the locked suffix draws red.
- This is the exact rule the C++ lifecycle test mirrors with `EffectiveDashElapsed`.

### Execute / Reset button handlers
`OnClicked(BtnExecute)` calls `ExecuteMove`, which clears `bDashAwaitingExecute` and
resolves the commit. `OnClicked(BtnReset)` calls `ResetEncounter`, which does the
same. After either, the next tick's `DrawDashPath` uses the fresh resolved state.

## DrawDashPath argument sources (summary)

| Context | Start | End | Elapsed movement | Result |
|---|---|---|---|---|
| Active Dash targeting | `PlayerFighter` location | mouse ground hit (Z 95) | unconnected → `0` | red locked prefix, green remainder (all red if ≤270 uu) |
| Confirmed Dash, awaiting Execute | `PlayerFighter` location | `DestPlayer` actor location | `SelectInt → 0` | entirely red |
| Executed Dash / reaction-pause continuation | `PlayerFighter` location | `DestPlayer` actor location | `GetDashElapsedMovementInstants(CurrentState)` | green travelled prefix, red locked suffix, all green once elapsed ≥ 3 |

## Test evidence

Run through `AutomationTestToolset.RunTests` inside a headless MCP editor
(`UnrealEditor-Cmd -nullrhi -unattended -ModelContextProtocolStartServer`):

```
BUH.PathRendererTests              Success
BUH.DashPathColorTests             Success
BUH.DashReactionIntegrationTest    Success
BUH.DashGateRegressionTest         Success
BUH.DashLifecycleRegressionTest    Success
BUH.CombatSimTests                 Success
BUH.CombatFlowTests                Success
BUH.WalkPreviewTests               Success
BUH.CombatManagerIntegrationTests  Success
passed=9 failed=0 skipped=0 total=9
```

`BUH.DashLifecycleRegressionTest` is the new C++-only regression test. It asserts:
- `BP_CombatManager` exposes `bDashAwaitingExecute` and `IsDashAwaitingExecute`.
- A reaction-pause continuation Dash draws all green (elapsed = authoritative 3).
- A replacement Dash while awaiting execute draws red (and red+green for long paths).
- The replacement Dash stays red across simulated UI refreshes.
- Cancel restores the old green continuation.
- Continue preserves the old green progress.
- A short replacement Dash (≤270 uu) is entirely red.
- Execute starts a fresh commitment period (new elapsed < 3).
