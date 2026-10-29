# notes.md — Beneath Uncounted Heavens (detailed reference)

Consult this file when a specific problem arises. It holds the detailed toolset gotchas, the graphical visual-test/self-wake procedure, and known limitations. The always-in-context essentials live in `AGENTS.md`.

## MCP Toolset Gotchas (learned the hard way)
- `ObjectTools` cannot read or set **object-reference** properties (`Mesh`, `SetMaterial`, `StaticMesh`); primitive props (`capsuleRadius`, `capsuleHalfHeight`) work. `list_properties`/`set_properties` need `instance` + `values` (string JSON), not `object`/`properties`.
- Saving: `AssetTools.save_assets` on **package paths** with param `asset_paths` (array). `SceneTools.save_actor` fails for level actors. Saving a BP fails while PIE is running — `StopPIE` first.
- PIE: `EditorToolset.EditorAppToolset.StartPIE` needs `options` = `{"bSimulate": false, "playMode": "PlayMode_InViewPort", "warmupSeconds": 3}`. Stop with `StopPIE`; check `IsPIERunning`.
- Captures: `CaptureEditorImage` shows the full editor **with the PIE game view** in the level viewport — use it for PIE evidence. `CaptureViewport` shows the editor level viewport, not the PIE game view. Large image outputs are truncated to a file; decode with `python3 -c` using `json` + `base64`.
- Logs: `EditorToolset.LogsToolset.GetLogEntries` requires `category` and `pattern`; pass `category: ""` for the whole log.
- `GetVisibleActors` returns only actors in the editor viewport frustum — during PIE with a distant camera it misses the fighters. Use `SceneTools.find_actors` (empty `name`/`tag`/`collision_channels`) to list all actors in the PIE world.
- **Blueprint graph DSL** (`BlueprintTools.write_graph_dsl`):
  - `write_graph_dsl` does not wipe the graph; it attaches bodies to found/created entry nodes and deletes only orphaned nodes. Empty events survive.
  - `(fn Name () ...)` requires an explicit param list, even empty. Omitting it parses the first body form as the param list.
  - Own-Blueprint functions are `CallFunction|Name`; other Blueprints' members are `Class|BPName|Member`.
  - Function calls with input params: use keyword args (`:Param value`) or the first positional arg wires to the `self` pin (`Could not connect pin X to self`).
  - A node call in expression position returns only the first data output — no DSL syntax for other named outputs. Use single-output resolver functions.
  - `if`/`elif`/`else`: `elif` must be the last form in the body; every nested `if` needs its own closing paren. Unbalanced parens → `Unclosed parenthesis` / `Unexpected )`.
  - `Utilities|Array|Get(acopy)` pin `Dimension 1` has a space — use positional args, not `:Dimension1`.
  - `Game|GetPlayerController` is static (input `PlayerIndex`, default 0). `Game|Player|GetOwningPlayerController` is actually `APlayerCameraManager` — do not use it for an Actor's player controller.
  - `UserInterface|CreateWidget` returns base `UserWidget`; cast with `Utilities|Casting|CastToWBP_CombatUI`. `UserInterface|Viewport|AddtoViewport` (lowercase `t`) is required or the widget never appears.
  - `add_function_param` only supports primitives; object params need `add_object_function_param` (`object_class` refPath, `input_param`).
  - `add_component_bound_event` does NOT work for UMG widgets. Bind widget button events with `UMGToolSet.BindToEventProperty` (`eventName="OnClicked"`, `propertyName=Btn...`, `propertyClass=/Script/UMG.Button`).
  - `read_graph_dsl` needs `graph` (EdGraph refPath). `find_node_types` needs `graph`, `type_id_filter`, `context_pins` (empty array lists all nodes). `create_node` needs `pos` (`IntPoint`), not `position`.
  - New functions from other BPs only appear in the action database after that BP is compiled; `create_node` bypasses the stale cache.
- `SlateInspectorToolset.SlateInspectorToolset`: only tracks **top-level** editor Slate windows. PIE game UMG widgets are composited into the viewport and are not reachable as Slate refs. **Popup context menus cannot be clicked** — `Click`/`PressKey` return success but don't execute on the popup window. To close a tab, the user must do it manually (right-click tab → Close).
- `SlateInspectorToolset.SlateInspectorToolset.Snapshot` requires a `ref` param (empty string = all windows), not `window_index`.

## Graphical Visual Test & Self-Wake (Phase 4)
`run_graphical_visual_test.sh` proves the two fighters actually draw (the headless `-nullrhi` editor renders nothing). It waits for Qwen/llama.cpp to idle-sleep (VRAM < 4 GiB), closes the headless MCP editor, launches the full graphical editor, opens `Lvl_TacticalDuel`, starts PIE, captures the game view, closes the graphical editor, and writes `tmp_opencode/visual_test/<run_id>/result.json`.
- **Run it via a BLOCKING bash tool call** (`bash run_graphical_visual_test.sh`), NOT backgrounded with `nohup … &`. The blocking call keeps the session waiting (no inference) so llama.cpp sleeps; when the script exits, the tool call returns its result and the session's next inference — an HTTP request to the llama.cpp server (`127.0.0.1:8082`, model `qwen3.8-coder390`, `--sleep-idle-seconds 60`) — wakes the model automatically.
- **Never launch a nested `opencode run` from inside the script** to "wake" the model. That is a nested OpenCode process that times out and never reloads the model — it was the original self-wake bug. The wake comes from the *original* session's own next inference, not a second process.
- Verified end-to-end: the model slept first (VRAM ~1.3 GiB), the graphical test captured a real screenshot showing both fighters, the editor closed, and the session resumed automatically with no manual user message.
- The llama.cpp idle-sleep wakes on **any** new inference request, not only interactive-session messages. A direct `POST /v1/chat/completions` to `127.0.0.1:8082` with the model ID wakes it (do not use `/health`, `/props`, or `/models` — those are excluded from triggering reload).

## Known Limitations (do not re-litigate without new evidence)
- **Fighter mesh renders at runtime (fixed).** `BodyMesh` is a `StaticMeshComponent` whose `StaticMesh` is set in the CDO default properties — NOT via `SetStaticMesh` in BeginPlay (that left `staticMesh=None` and drew nothing). Verified in PIE: `BodyMesh.StaticMesh=/Engine/BasicShapes/Cylinder.Cylinder`, `Mobility=Movable`, `bHiddenInGame=False`, and the graphical PIE screenshot shows both fighter cylinders.
- **Camera framing (verified via graphical screenshot):** a dedicated `CameraActor` at `(0,-1500,400)` (yaw=90, pitch=-11.7) frames both fighters at `(±500,0,90)`. `BP_CombatManager` BeginPlay calls `SetViewTargetWithBlend` on the PlayerController to make it the active camera — the level uses the FirstPerson template's PC/pawn, and the default-PC / GameMode `PlayerControllerClass` route was infeasible via headless MCP (class-ref defaults not settable, no `SetCameraActor` node). Verified: the CameraManager's active camera sits at the CameraActor's transform, and the graphical PIE screenshot shows both fighter cylinders in frame. Note: the level reverted to a yaw=0 `CameraActor` after the project-directory move; it was re-spawned at yaw=90 and the save persisted (confirmed by reloading the level).
- **Headless capture:** under `-nullrhi`, `CaptureEditorImage` returns a 1x1 placeholder (no real rendering). Verify PIE state via actor/property inspection (e.g. the CameraManager's `TransformComponent0` location), not a screenshot.
- **SlateInspector cannot click popup context menus** (see MCP Gotchas) — UI tabs/panels can't be closed programmatically.
- C++ compilation is a known limitation; this test uses Blueprints only. (Superseded: Phase 7 now uses a C++ deterministic resolver — see Resume State below.)

## Phase 7 Resume State (as of shutdown 2026-10-27)
- **C++ deterministic resolver DONE + verified.** `CombatSim.h/.cpp` implements `UBuhCombatSim::ResolveCombat` (Instant-by-Instant), `MakeState`, and 8 mandatory tests via `RunAllTests()`. All 8 PASS (ran as automation test `BUH.CombatSimTests`, state Success). Getters `GetFinalPlayerX/OpponentX/PlayerHealth/OpponentHealth` added (single-int32 outputs, because the DSL returns only the first data output — no struct member access). Module compiled; `libUnrealEditor-BeneathUncountedHeav.so` linked.
- **Blueprint integration IN PROGRESS.** `BP_CombatManager` now has `PlayerX`/`OpponentX` int32 vars (default -500/500), SAVED to disk. `BeginPlay` still runs the old TEST_A sequences (section 8 cleanup pending). `ExecuteMove` still calls old `ApplyExchange`; `BuildPredictions` still evaluates 4 player moves (the mistake to fix).
- **Next steps:** (1) rewrite `ExecuteMove` = MakeState(PlayerX,OpponentX,PlayerHealth,OpponentHealth,Seed) → ResolveCombat(state,PlayerMove,OpponentMove,200) → set PlayerX/OpponentX/healths from getters → SetActorLocation → update HUD. (2) rewrite `BuildPredictions` to evaluate opponent responses (0,1,2,3) to the player's SELECTED move on a cloned state. (3) clean `BeginPlay` to a playable planning state. (4) graphical PIE verification. (5) honest `phase7_report.md`.
- **Node types confirmed:** `CombatSim|MakeState`, `CombatSim|ResolveCombat`, `CombatSim|GetFinalPlayerX` (and the other 3 getters), `Transformation|SetActorLocation`. Standard MakeVector / int→float nodes still need locating (PCG nodes pollute the search).
- **Editor:** relaunch headless via the AGENTS.md command; wait for port 8000. Old PID is dead after shutdown.
