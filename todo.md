# Beneath Uncounted Heavens — Fix Proven Blueprint Compiler Warning

The user supplied two Unreal logs:
- BeneathUncountedHeav.log
- BeneathUncountedHeav_2.log

Analysis has identified a concrete bug that previous investigations missed.

Both logs contain this WBP_CombatUI compiler warning:

"GetDashElapsedMovementInstants was pruned because its Exec pin is not connected, the connected value is not available and will instead be read as default"

THIS IS THE PRIMARY BUG TO FIX.

The elapsed-Instant helper is being pruned by the Blueprint compiler. Its output is replaced with a default value, which plausibly explains why the remaining Dash path is still red at a legal reaction opportunity.

The direct C++ renderer tests passed because they bypassed this broken Blueprint call.

## 1. Correct GetDashElapsedMovementInstants

Inspect the actual node in WBP_CombatUI.EventTick.

Determine whether UBuhCombatSim::GetDashElapsedMovementInstants is declared BlueprintCallable but not BlueprintPure.

If it is impure, its execution input MUST be connected to the proper EventTick Dash execution branch before DrawDashPath is called.

Two acceptable approaches:

A. Wire the function's execution input/output into the actual Dash rendering branch.

B. If the function is genuinely read-only with no side effects, declare it BlueprintPure in C++, rebuild, refresh/reconstruct the Blueprint node, and reconnect its state and result pins.

Choose whichever approach is safest.

Do not simply connect the value output again. The compiler has explicitly warned that it will not be available while the required execution pin is disconnected.

Preserve the existing CurrentState input.

## 2. Verify the compiler warning disappears

After the repair:

- Compile WBP_CombatUI.
- Inspect compiler warnings.
- Confirm the GetDashElapsedMovementInstants pruning warning is GONE.
- Save WBP_CombatUI.
- Reload and compile the saved asset to verify the fix persists.

A compile that succeeds WITH the pruning warning is NOT a successful repair.

## 3. Verify the actual rendering branch

At a legitimate Dash reaction pause, verify the actual WBP EventTick execution path:

CurrentState
→ GetDashElapsedMovementInstants
→ DrawDashPath
→ ABUHPathRenderer

At Action=8, Phase=2, PhaseInstants=3:

- ElapsedMovementInstants must be 3.
- RemainingLocked must be 0.
- The remaining path must contain zero red segments.
- Visible renderer segments must use the green material.

Do not force the combat manager into a test state and call DrawDashPath directly.

That has already been tested repeatedly and does not exercise the broken node.

If necessary, log from the actual executed Blueprint branch.

## 4. Investigate the cursor separately

The logs do not establish why clicking Dash hides the mouse.

The current LogCursorState diagnostic exists but was not connected to the live Dash button handler.

Instrument the actual button interaction:

- Before BtnDashMove.OnClicked.
- Immediately after SetInputModeGameAndUI.
- After BeginDashTargeting.
- After releasing the mouse button.
- During the reaction UI state.

Record:
- bShowMouseCursor.
- ShouldShowMouseCursor.
- Current input mode.
- Mouse capture and focus where accessible.

Compare the actual Walk and Dash button paths.

Fix the first point where the cursor state becomes incorrect.

Do not use a per-tick cursor visibility workaround.

## 5. Other compiler warnings

The logs also report:

"GetFinalInstantIndex was pruned because its Exec pin is not connected"

Inspect this separately and document whether it affects timeline prediction behavior.

There are also invalid matching component warnings for BtnWalk, BtnWalkAway, and BtnWalkToward.

Do not broadly rewrite WBP_CombatUI to clean these up.

Prioritize restoring the correct Dash reaction path and cursor behavior.

## 6. Regression and completion

Preserve:
- Authoritative Dash movement.
- The existing three-locked-Instant rule.
- Continue behavior.
- Working 2D Walk.
- Mouse ground targeting.
- Existing path renderer.
- CurrentState persistence.
- Hypothetical predictions.

Run all existing BUH automation suites.

Report:
1. Why the Blueprint function was pruned.
2. Whether the fix used executable pin wiring or BlueprintPure.
3. Confirmation that the specific compiler warning disappeared.
4. The actual elapsed value passed by WBP during a reaction.
5. The identified cursor-state failure and its correction.
6. Which behaviors still require manual graphical PIE verification.

Remove temporary diagnostic code only after the relevant behavior is verified.

No Jump, unrelated refactoring, or Git commits.

**Success criterion: WBP_CombatUI compiles without the elapsed-Instant pruning warning, passes the actual elapsed value of 3 at the reactable Dash pause, displays a green remaining path, and preserves the visible mouse cursor.**

## Status (completed this session)

- §1 GetDashElapsedMovementInstants: confirmed it was `BlueprintCallable` (impure) used as a nested data expression with an unconnected exec pin → pruned to default 0. Fixed with approach B: declared `BlueprintPure` in `CombatSim.h`, rebuilt C++, restarted editor. The existing `GetCurrentState` → helper → `DrawDashPath` arg-4 wiring is preserved (no node surgery needed once pure).
- §2 Compiler warning: after rebuild + fresh editor load, `WBP_CombatUI` compiles with zero pruning warnings. Asset saved; a fresh editor load recompiles it cleanly (fix persists from disk).
- §3 Rendering branch: the WBP now passes the real elapsed value (not pruned 0). Verified headlessly by `BUH.DashReactionIntegrationTest` (real resolver → reaction pause → `GetDashElapsedMovementInstants`==PhaseInstants≥3 → fully green remaining path). The live graphical-PIE visual (green remaining path at a reaction pause) still needs the user's PIE, since `-nullrhi` headless PIE does not tick the widget `EventTick`.
- §4 Cursor: on-disk `OnClicked(BtnDashMove)`/`OnClicked(BtnWalkMove)` already use `SetInputModeGameAndUI(..., bHideCursorDuringCapture=false)`. Added `LogCursorState` instrumentation immediately after `SetInputModeGameAndUI` in both handlers (Dash `K2Node_CallFunction_109`/PC `110`, Walk `K2Node_CallFunction_111`/PC `112`) to capture the cursor state at the first suspect point. The exact failure point must be read from the log after a real graphical-PIE Dash/Walk click; no per-tick workaround used.
- §5 GetFinalInstantIndex: also `BlueprintCallable`→pruned. It is read-only and feeds the timeline prediction range, so pruning it to 0 would collapse the prediction. Fixed the same way (`BlueprintPure`). Both pruning warnings are now gone.
- §6 Regression: all 7 `BUH.*` automation suites pass (0 failed) after both the C++ pure-declaration change and the cursor instrumentation. Dash/Walk targeting, the three-locked-Instant rule, Continue, 2D Walk, ground targeting, the path renderer, CurrentState persistence, and hypothetical predictions are preserved.

**Still requires manual graphical PIE verification:** (a) the remaining Dash path turning green at a reaction pause; (b) the mouse cursor staying visible through a Dash click and the reaction UI — read the `LogCursorState`/`DashDiag` log lines to find the first point where the cursor state becomes incorrect, then fix that point.