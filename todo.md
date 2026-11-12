# BUH — Fix Jump Reaction State: Corrected Findings After Live Graph Verification

## Verified Findings (replace previous todo items 1-3)

### Item 1 RESOLVED: Export was stale, live ExecuteMove is correct

Read back the live `BP_CombatManager:ExecuteMove` graph via MCP. The actual compiled graph contains:

- `SetCurrentState _returnvalue_7` (the final state from ResolveCombat)
- `SetbHasCurrentState true`
- `BeginJumpPlayback` gate: `(or (== PlayerMove 9) (> GetJumpElapsedFlightInstants _returnvalue_1 0))`

The gate does NOT check `GetbJumpAwaitingExecute`. The `SetJumpAwaitingExecute false` fires before the gate but does not block it because the gate checks `PlayerMove == 9`. The export at `tests/blueprint_exports/jump_selection_flow.md` was stale and has been updated to match the live graph.

### Item 2 RESOLVED: CurrentState IS updated

The live ExecuteMove graph shows `SetCurrentState` and `SetbHasCurrentState true` are present. The CurrentState lifecycle is correct in the compiled Blueprint.

### Item 3 CORRECTED: Neither environment starts a Jump

Both headless (`mcp_author17.log`) and graphical (`Saved/Logs/BeneathUncountedHeav.log`) PIE sessions show:
- All `FighterZ tick` lines: `PZ=90.00 OZ=90.00 active=0`
- Zero `BeginJumpPlayback`, `JumpPlayback TICK`, or `JumpPlayback STOP` lines
- The Jump never starts in either environment

The "working Jump animation and reaction UI" referenced in the previous todo was from a prior session before the WBP EventTick was accidentally wiped during a `write_graph_dsl` call. It is not a current-state observation.

## Real Blocker: WBP Cast Failure Prevents ExecuteMove

The `CastToWBP_CombatUI` in `BP_CombatManager:EventBeginPlay` fails, so:
- `BP_CombatManager.UI` = `None`
- WBP `manager` variable = `None` (SetManager uses the failed cast result)
- WBP Execute button calls `ExecuteMove` on `None` → no-op
- WBP keyboard shortcuts call functions on `None` → no-op

### Fix Applied: AddToViewport uses original widget reference

Changed `AddToViewport` from `_aswbp_combat_ui` (cast result) to `_returnvalue_1` (original CreateWidget result). The WBP is now in the viewport and its EventTick fires. However, `SetManager` still requires the cast result, so the WBP's `manager` variable remains `None`.

### MCP Asset Registry Issue

`AssetTools.exists` returns `false` for `/Game/BUH/Prototypes/TacticalDuel/Blueprints/WBP_CombatUI` even though the file exists on disk (4.1MB). Restarting the editor fixed `BP_CombatManager` but not `WBP_CombatUI`. `save_assets` also fails for WBP_CombatUI. The asset is loadable (compile_blueprint succeeds, CreateWidget works, WBP constructs) but the registry doesn't track it.

### Remaining Work

1. Fix the `CastToWBP_CombatUI` failure so `SetManager` can store the BP_CombatManager reference on the WBP. Without this, the WBP buttons and keyboard shortcuts cannot trigger `ExecuteMove`.
2. Once ExecuteMove is reachable, run a real Jump to a reaction pause and log the A-G sequence from the previous todo item 4.
3. Identify the first wrong transition and repair narrowly.
4. Verify with integration test and graphical PIE.
5. Update notes.md and todo.md, commit and push.

## Preserved Constraints

- Jump's 8-Instant parabola, three locked flight Instants, reaction eligibility.
- Existing Walk and Dash mechanics, Continue, mouse targeting and UMG controls.
- No per-frame SetActorLocation workaround.
- No Jump targeting redesign, no independent Blueprint height calculation.
- Deterministic per-Instant playback, authoritative simulation height.

**Commit and push to origin/master** so ChatGPT can inspect the changes. Report the commit SHA and clearly distinguish headless automation from graphical PIE verification.

Success: Jump plays normally, stops at a legitimate airborne reaction, stays suspended until a decision is made, and resumes correctly on Continue.