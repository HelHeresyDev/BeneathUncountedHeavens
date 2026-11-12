# BP_CombatManager — Corrected BeginPlay + EventTick Export

Exported via MCP `read_graph_dsl` after eliminating the failing `CastToWBP_CombatUI`
and moving per-frame Jump playback into the reliably-firing manager tick.

## BP_CombatManager — EventBeginPlay (cast eliminated)

The failing `CastToWBP_CombatUI` is gone. `CreateWidget` returns a valid
`WBP_CombatUI_C` instance directly; that same result is stored in `UI`, passed to
`SetWidgetManager` (which writes the widget's `Manager` property via C++
`FObjectProperty::SetObjectPropertyValue`), added to the viewport, and used for
`SetInputModeGameAndUI`. No reflective cast is required.

```
(event EventBeginPlay
  (bind _returnvalue (Game|GetPlayerController 0))
  (bind _self self)
  (bind _outactors (Actor|GetAllActorsWithTag "PlayerFighter"))
  (bind _asbp_fighter (Utilities|Casting|CastToBP_Fighter (Utilities|Array|Get(acopy) _outactors)))
  (bind _outactors_1 (Actor|GetAllActorsWithTag "OpponentFighter"))
  (bind _asbp_fighter_1 (Utilities|Casting|CastToBP_Fighter (Utilities|Array|Get(acopy) _outactors_1)))
  (Variables|Default|SetPlayerFighter _asbp_fighter)
  (Variables|Default|SetOpponentFighter _asbp_fighter_1)
  (Variables|Default|SetPlayerHealth 100)
  (Variables|Default|SetOpponentHealth 100)
  (Variables|Default|SetState 0)
  (Variables|Default|SetOpponentMove 1)
  (bind _returnvalue_1 (UserInterface|CreateWidget "/Game/BUH/Prototypes/TacticalDuel/Blueprints/WBP_CombatUI.WBP_CombatUI_C" (Game|GetPlayerController 0)))
  (Variables|Default|SetUI _returnvalue_1)
  (CombatSim|SetWidgetManager _returnvalue_1 _self)
  (UserInterface|Viewport|AddToViewport _returnvalue_1)
  (Input|SetInputModeGameAndUI _returnvalue _returnvalue_1)
  (Utilities|FlowControl|Delay 0.5)
  (bind _returnvalue_2 (Actor|GetActorOfClass "/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter.BP_FirstPersonCharacter_C"))
  (Actor|DestroyActor _returnvalue_2)
  (bind _outactors_2 (Actor|GetAllActorsOfClass "/Script/Engine.CameraActor"))
  (Game|Player|SetViewTargetWithBlend _returnvalue (Utilities|Array|Get(acopy) _outactors_2))
  (CallFunction|ResetEncounter))
```

Key object-pin connections:
- `CreateWidget` Class pin → `WBP_CombatUI_C` (`/Game/BUH/Prototypes/TacticalDuel/Blueprints/WBP_CombatUI.WBP_CombatUI_C`).
- `CreateWidget` ReturnValue → `SetUI` value, `SetWidgetManager` widget arg, `AddToViewport` widget, and `SetInputModeGameAndUI` widget.
- `SetWidgetManager` second arg → `self` (the current `BP_CombatManager` instance), so the widget's `Manager` property points at the exact live manager.

## BP_CombatManager — EventTick (sole Jump playback driver)

Per-frame `TickJumpPlayback` is driven ONLY by the manager tick. The WBP EventTick
no longer calls `TickJumpPlayback` (it was a duplicate that advanced the same static
timeline twice per frame once the widget was running again). `TickJumpPlayback`
no-ops safely when no playback is active
(`if (!GJumpPlaybackActive || index >= timeline.Num()) return false;`), so calling
it every frame is safe.

The C++ debug-input workaround `HandleTargetingTick` is also gone: the WBP EventTick
mouse-confirmation (`WasInputKeyJustPressed LeftMouseButton` → `SelectJumpDestination`
/ `SelectDashDestination` / `SelectWalkDestination`) is the single authoritative
targeting-confirmation path, so the same ground click is no longer processed twice.

```
(event EventTick (DeltaSeconds)
  (CombatSim|TickJumpPlayback DeltaSeconds)
  (CombatSim|LogFighterZ (Variables|Default|GetPlayerFighter) (Variables|Default|GetOpponentFighter) "tick")
  (Variables|Default|SetInstantAccumulator (+ (Variables|Default|GetInstantAccumulator) DeltaSeconds))
  (if (>= (Variables|Default|GetInstantAccumulator) 0.05)
    (Variables|Default|SetInstantAccumulator (- (Variables|Default|GetInstantAccumulator) 0.05))
    (if (== (Variables|Default|GetPlayerPhase) 2)
      (if (> (Variables|Default|GetPlayerInstantsinPhase) 0)
        (Variables|Default|SetPlayerInstantsinPhase (- (Variables|Default|GetPlayerInstantsinPhase) 1))
        (else
          (Variables|Default|SetPlayerPhase 0))))
    (if (== (Variables|Default|GetOpponentPhase) 2)
      (if (> (Variables|Default|GetOpponentInstantsinPhase) 0)
        (Variables|Default|SetOpponentInstantsinPhase (- (Variables|Default|GetOpponentInstantsinPhase) 1))
        (else
          (Variables|Default|SetOpponentPhase 0))))))
```

## Reaction-pause freeze — verified behavior

With the manager tick driving `TickJumpPlayback`, a Jump interrupted by an
opponent reaction now freezes at the authoritative airborne position instead of
falling/snapping to ground Z=90:

- Timeline ascends the parabola: inst=0 Z=90, inst=1 Z=186.2, inst=2 Z=255.0,
  inst=3 Z=296.2.
- At inst=4 the reaction triggers (`reaction=1`), freezing the fighter at
  `playerZ=296.2` (= 90 + 206.25 authoritative Height), XY frozen at -320.
- `JumpPlayback STOP: index=5 count=5 reaction=1`; the fighter then holds
  `PZ=296.25 active=0` for every subsequent frame. No fall, no snap to ground.

## TriggerReactionJump console command

`BUH.TriggerReactionJump` builds the reaction-pause prestate (opponent mid-Heavy
`Phase=3 Action=1`, player grounded), resolves with `PlayerMove=9, OpponentMove=1`,
and calls the real `BeginJumpPlayback`. It is diagnosis scaffolding only; the
normal gameplay path is UI → `ExecuteMove` → `BeginJumpPlayback`, with the manager
tick driving `TickJumpPlayback`.
