# Tactical Duel — Jump Movement Action

## Objective

Add a Jump movement action to the Tactical Duel prototype. Jump is a free
movement action: it costs no instants, does not open a reaction window, and
cannot be interrupted. The player chooses a landing destination, a red
parabola previews the flight path, and Execute commits the move. The fighter
travels along the parabola over 6 instants, landing at the chosen destination.

## Completed

- [x] Jump move ID `9` added to `UBuhCombatSim::ResolveCombat`.
- [x] Jump is free: it advances the timeline by 0 instants and produces no
      reaction pause.
- [x] Jump cannot be interrupted: it completes even when the opponent has a
      faster startup.
- [x] Jump destination stored in `FCombatState.JumpTarget` / `JumpTargetY`.
- [x] `PrepareJumpCommit` replaces an unset target with the current fighter
      position and clamps long targets to `BuhCombat::JumpMaxRange`.
- [x] `IsReactableMovement` includes `IsReactableJump`.
- [x] `GetReactionInfo` includes Jump ID `9` in the Continue-legal branch.
- [x] `ExecuteMoveWithVars` applies the Jump destination from the live
      `GExecJumpTarget` / `GExecJumpTargetY` globals when `PlayerMove == 9`.
- [x] `BP_CombatManager.SelectJumpDestination` stores the preview and exec
      Jump targets, sets `PlayerMove = 9`, sets `JumpAwaitingExecute = true`,
      rebuilds predictions, and refreshes the UI.
- [x] `BP_CombatManager.ExecuteMove` calls `CombatSim|ExecuteMoveWithVars`
      instead of the broken `CombatSim|ExecuteMove` UFUNCTION, then clears the
      stale exec targets.
- [x] `WBP_CombatUI.OnClicked(BtnJumpMove)` explicitly calls
      `SetShowMouseCursor true` after switching to GameAndUI input mode.
- [x] All 12 BUH automation suites pass, including
      `BUH.JumpSimulationTests` and `BUH.CombatManagerIntegrationTests`.
- [x] PIE log inspection shows no errors and the WBP constructs successfully.
- [x] Corrected Blueprint graphs exported to
      `tests/blueprint_exports/jump_selection_flow.md`.
- [x] `notes.md` §9.30 documents the Jump execution and cursor fixes.

## Remaining (graphical verification only)

- [ ] Graphical PIE playtest: click Jump, click a ground destination, click
      Execute, and confirm the fighter travels along the parabola to the
      chosen landing point while the mouse cursor remains visible.
- [ ] Graphical PIE playtest: confirm the red/green parabola split renders
      correctly during flight and that the path clears after landing.

## Investigation notes

- The Jump destination was not applied during execution because
  `BP_CombatManager.ExecuteMove` called `CombatSim|ExecuteMove`, a UFUNCTION
  whose body only called `ResolveCombat` and never read
  `GExecJumpTarget` / `GExecJumpTargetY`. The fix routes execution through
  `CombatSim|ExecuteMoveWithVars`, which already implemented the Jump
  destination logic.
- The mouse cursor disappeared during Jump targeting because
  `OnClicked(BtnJumpMove)` set input mode to GameAndUI but never called
  `SetShowMouseCursor true`. The fix adds that call explicitly.
- Headless nullrhi PIE does not expose the game Slate UI, so the graphical
  playtest remains the only way to verify the full Jump interaction loop.
