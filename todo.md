# Beneath Uncounted Heavens — WBP_CombatUI Cast Failure Resolved + Reaction-Pause Freeze Fixed

## Status: DONE (headless-verified)

The two blockers are resolved and verified in headless PIE.

## 1. The failing cast is eliminated (not worked around)

`BP_CombatManager:EventBeginPlay` no longer calls `CastToWBP_CombatUI`. The
`CreateWidget(WBP_CombatUI_C)` result is used directly for `SetUI`,
`SetWidgetManager`, `AddToViewport`, and `SetInputModeGameAndUI`. `SetWidgetManager`
writes the widget's `Manager` property via C++ `FObjectProperty::SetObjectPropertyValue`,
which is the only reflective write that compiles and runs without crashing in UE 5.8.4.

Verified in `mcp_author33.log`:
- `SetWidgetManager: Manager prop type=ObjectProperty name=Manager`
- `SetWidgetManager: FObjectProperty PropertyClass=.../BP_CombatManager.BP_CombatManager_C`
- `SetWidgetManager: DONE (SetObjectPropertyValue)`
- `WBP_CombatUI CONSTRUCTED`
- No `Signal 11` crash.

The `UI` property is the valid `WBP_CombatUI_C` instance and the widget's `Manager`
property references the exact live `BP_CombatManager` instance.

## 2. The reaction-pause freeze is fixed

Root cause of the "fighter falls/snaps to Z=90" bug: `TickJumpPlayback` was never
being called in the real flow (the WBP EventTick was not reliably driving it), so the
actor stayed at ground while the playback statics thought it was airborne.

Fix: `BP_CombatManager:EventTick` now calls `CombatSim|TickJumpPlayback DeltaSeconds`
every frame. `TickJumpPlayback` no-ops safely when no playback is active
(`if (!GJumpPlaybackActive || index >= timeline.Num()) return false;`).

Verified in `mcp_author33.log` (real PIE flow, `BUH.TriggerReactionJump` console
command driving the reaction prestate):
- Timeline ascends the parabola: inst=0 Z=90, inst=1 Z=186.2, inst=2 Z=255.0,
  inst=3 Z=296.2.
- At inst=4 the reaction triggers (`reaction=1`), freezing the fighter at
  `playerZ=296.2` (= 90 + 206.25 authoritative Height), XY frozen at -320.
- `JumpPlayback STOP: index=5 count=5 reaction=1`; the fighter then holds
  `PZ=296.25 active=0` for every subsequent frame. No fall, no snap to ground.

This satisfies the original constraint: freeze at the precise simulated XY/Height,
remain suspended indefinitely, no extra falling/interpolation, no gravity during
planning, authoritative simulation height, deterministic per-Instant playback.

## 3. Diagnosis scaffolding

`BUH.TriggerReactionJump` console command builds the reaction-pause prestate
(opponent mid-Heavy `Phase=3 Action=1`, player grounded), resolves with
`PlayerMove=9, OpponentMove=1`, and calls the real `BeginJumpPlayback`. It is
diagnosis-only; the normal gameplay path is UI → `ExecuteMove` → `BeginJumpPlayback`,
with the manager tick driving `TickJumpPlayback`. The auto-trigger that was
temporarily added to `EventBeginPlay` has been removed to restore the clean workflow.

## 4. What still awaits graphical confirmation

Headless PIE cannot click the WBP buttons (Slate snapshot shows no widget buttons
under `-nullrhi`), so the full UI-driven Jump (click Jump → click destination →
click Execute) has not been exercised end-to-end in this session. The C++ playback
path and the manager-driven tick are verified; the WBP button → manager → ExecuteMove
path is wired (manager reference restored) but needs a graphical PIE run to confirm
a real button click reaches `ExecuteMove` and starts playback.

Next graphical check (fresh PIE):
1. Click the Jump button.
2. Click a destination on the ground.
3. Click Execute.
4. Confirm `BeginJumpPlayback` is invoked and the fighter ascends.
5. Reach a reaction pause and confirm the fighter freezes at the authoritative Z.
6. Test Continue.

## 5. Verification and GitHub

- Compiled and saved `BP_CombatManager` (BeginPlay + EventTick).
- Exported the corrected `EventBeginPlay` and `EventTick` graphs to
  `tests/blueprint_exports/combat_manager_beginplay_tick.md`.
- `BUH.JumpGroundedReactionTest` and the other reaction/Jump suites pass in PIE
  (tight-loop playback holds the fighter at Z=296.25 through the reaction).
- Committed and pushed the C++ changes (`TriggerReactionJump`, `SetWidgetManager`,
  `HandleTargetingTick`, `LogFighterZ`), the Blueprint changes, the export, and the
  docs.
