## **Found a missing legality check for Continue**

I inspected the latest GitHub commit, `d809a90`, including `CombatSim.cpp`.

**Continue (action 7) is only supposed to resume movement that is already underway.** Selecting Walk in the planning menu doesn't qualify because the movement hasn't actually started.

The current simulator handles Continue like this:

```
if (Decision == 7)
{
    if (IsReactableMovement(F))
    {
        /* Continue Walk/Dash: preserve existing state */
    }
    continue;
}
```

The problem is that **an illegal Continue is silently ignored rather than rejected**.

If the player hasn't started walking, `IsReactableMovement(F)` returns false, but the rest of the simulation can still proceed. This leaves the UI and combat manager at risk of disagreeing about the player's selected action.

I can't yet prove that's the precise crash mechanism without the crash log. However, accepting Continue outside a valid reaction is a real control-flow defect worth fixing.

The correct behavior should be:

- **Walk selected but not executed:** Continue is unavailable.
- **Walk executed, then interrupted by an enemy attack:** Continue becomes available.
- **Continue selected during that reaction:** Resume the existing movement without restarting it.
- **Invalid Continue requested anyway:** Reject safely without changing the combat state or crashing.

## **Repair prompt for Qwen**

# BUH — Unreal Crash: Continue Before Movement Execution

Repository: `HelHeresyDev/BeneathUncountedHeavens`

Baseline: `d809a90`

## Crash reproduction

The user reports:

1. Start a fresh combat encounter.
2. Select Walk.
3. Without executing Walk, switch to Continue.
4. Unreal Editor crashes.

This transition should be illegal but must never crash.

## Investigation

First inspect the latest local Unreal crash log and call stack from `Saved/Logs` and `Saved/Crashes`.

Identify the failing function and source line. Do not assume the illegal Continue selection is the direct crash cause.

Inspect:

- `WBP_CombatUI.EventGraph` and `BtnContinueWalk.OnClicked`
- `WBP_CombatUI.RefreshUI`
- `BP_CombatManager.SetPlayerMove`
- `BP_CombatManager.ExecuteMove`
- `UBuhCombatSim::GetReactionInfo`
- `UBuhCombatSim::ResolveCombat`
- `UBuhCombatSim::GenerateTimeline`

The current C++ simulator silently ignores Continue when `IsReactableMovement(F)` is false. This is not adequate validation.

Also inspect uses of `MoveData[F.Action]`, because a fighter with `Action == -1` and an active phase could produce an out-of-bounds access.

## Required fix

### UI validation

Continue must only be available during a genuine player reaction opportunity when the player has an active, legally resumable Walk, Dash, or Jump.

Selecting Walk in planning mode must never enable Continue.

Do not determine legality from the selected `PlayerMove` alone. Check the authoritative combat state and actual reaction status.

### Combat manager validation

Reject Continue before changing `PlayerMove` if no legal reaction exists.

Preserve the player's pending Walk selection and destination when an invalid Continue request occurs.

Do not enter resolving mode or mutate authoritative combat state on an invalid request.

### C++ safety

Validate Continue before attempting to resolve it.

A rejected Continue must produce a safe, defined failure rather than silently running a simulation with no valid action.

Guard action-array indexing against invalid action IDs, without disguising other combat-state corruption.

Do not modify the behavior of a legitimate Continue.

## Regression tests

Add tests covering:

1. Fresh encounter → Continue attempted: rejected safely.
2. Select Walk → Continue without Execute: rejected safely.
3. Select Dash → Continue without Execute: rejected safely.
4. Select Jump → Continue without Execute: rejected safely.
5. Walk → Execute → genuine Heavy reaction → Continue: succeeds.
6. Valid Continue preserves the original movement target, phase and reaction bookkeeping.
7. Invalid Continue never changes authoritative combat state, fighter positions or health.
8. Reset and action cancellation leave no stale Continue eligibility.

Test the relevant Blueprint execution path where possible. Headless C++ tests alone do not establish that the live UI cannot crash.

## Constraints

Preserve:

- Baseline movement prediction
- Hypothetical opponent branches
- Gold reaction ghost
- Reaction → Continue timeline
- Existing combat timing

Keep the fix focused on crash prevention and legal action-state transitions.

Build, run tests, compile and save modified Blueprints, update notes.md and todo.md, then commit and push.

Report the actual crash stack, confirmed root cause, files changed, test results, and remaining graphical PIE checks.

Do not declare the crash fixed solely because all headless automation tests pass.

**Priority:** Fix this crash before making more prediction-system changes. We need Continue to be a strictly legal reaction action, not something the UI can accidentally request during ordinary planning.

## Status (2026-11-17)

- [x] Inspected crash log and identified root cause: empty `FinalState.Fighters` in `ResolveCombat` early return, unguarded `GetFinal*` indexing, and missing Continue legality guard in `ExecuteMove`.
- [x] Added `UBuhCombatSim::IsContinueLegal` and made `ResolveCombat` return a safe two-fighter error state with `StoppedReason = 0`.
- [x] Added a surgical Continue guard to `BP_CombatManager:ExecuteMove` that rejects illegal Continue without entering resolving mode.
- [x] Restored opponent-button visibility/opacity and the three ghost-hide chains in `WBP_CombatUI:RefreshUI`.
- [x] Compiled and saved `BP_CombatManager` and `WBP_CombatUI`.
- [x] Added C++ regression tests `V5` and `V6`; full non-PIE `BUH.*` suite passes 26/26.
- [ ] Graphical PIE verification: invalid Continue rejected safely, valid Continue resumes movement, opponent buttons dimmed correctly, ghosts hide/show correctly.