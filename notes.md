# notes.md — Beneath Uncounted Heavens (detailed reference)

Consult this file when a specific problem arises. It holds detailed toolset gotchas, the graphical visual-test/self-wake procedure, and known limitations. The always-in-context essentials live in `AGENTS.md`.

Treat newer verified evidence as more authoritative than old progress notes. Do not retry a documented failed approach without new evidence.

> **Verification rule:** Record what was changed, how it was tested, and what remains unverified. Source inspection proves implementation structure; Blueprint compilation proves compilation; graphical screenshots prove rendering; only actual interaction testing proves mouse-driven UI behavior. A fix is "done" only at the layer the task actually requires — do not declare a UI feature complete because its buttons are clickable when the downstream behavior is unverified.

---

## 1. Purpose and usage

This is a compact, practical troubleshooting reference for the Beneath Uncounted Heavens prototype. It is not a chronological development log. Each entry follows the pattern: **Problem → Cause → Verified solution → Important caveat**.

Sections:
- §2 Unreal MCP toolset reference
- §3 Blueprint graph editing gotchas
- §4 UMG and Slate troubleshooting
- §5 C++ and Unreal Build Tool
- §6 Headless and graphical testing
- §7 Local Qwen / OpenCode / VRAM workflow
- §8 Known unresolved limitations
- §9 Important resolved issues and lessons

---

## 2. Unreal MCP toolset reference

### 2.1 Toolset names (exact strings)

| Toolset | Exact name |
|---|---|
| Editor app (PIE, capture, open level) | `EditorToolset.EditorAppToolset` (capital E) |
| Logs | `EditorToolset.LogsToolset` |
| Assets (save) | `editor_toolset.toolsets.asset.AssetTools` (singular "asset") |
| Actors | `editor_toolset.toolsets.actor.ActorTools` |
| Blueprints | `editor_toolset.toolsets.blueprint.BlueprintTools` |
| Materials | `editor_toolset.toolsets.material.MaterialTools` |
| Material instances | `editor_toolset.toolsets.material_instance.MaterialInstanceTools` (underscore between material and instance) |
| Objects (property get/set) | `editor_toolset.toolsets.object.ObjectTools` |
| Scene (level, find/remove actors) | `editor_toolset.toolsets.scene.SceneTools` |
| UMG widgets | `UMGToolSet.UMGToolSet` |
| Automation tests | `AutomationTestToolset.AutomationTestToolset` |
| Slate inspector | `SlateInspectorToolset.SlateInspectorToolset` |

Lowercase `editor_toolset.toolsets.editor.EditorAppToolset` is NOT found. Use the capital-E form.

### 2.2 `mcp_do.py` args-file format

The args file passed to `mcp_do.py` must contain **only the tool's `arguments` object**, not the `toolset_name`/`tool_name`/`arguments` wrapper. The script adds the wrapper itself.

```json
{"instance": {"refPath": "/Game/..."}, "properties": ["Team"]}
```

NOT:

```json
{"toolset_name": "...", "tool_name": "...", "arguments": {"instance": ...}}
```

### 2.3 Parameter names that differ from intuition

| Tool | Correct param | Wrong guess |
|---|---|---|
| `MaterialTools.get_expressions` | `material_or_function` | `material` |
| `MaterialTools.recompile` | `material_or_function` | `material` |
| `MaterialTools.connect_to_output` | `expression`, `output_name`, `material_property` | `output` |
| `MaterialInstanceTools.get_vector_parameter` | `instance` (refPath with `.Name` suffix), `name` | — |
| `MaterialInstanceTools.list_parameters` | `material` (not `instance`) | `instance` |
| `BlueprintTools.compile_blueprint` | `blueprint` (refPath) | `asset_path` |
| `BlueprintTools.read_graph_dsl` / `write_graph_dsl` | `graph` (EdGraph refPath), `code` | — |
| `LogsToolset.GetLogEntries` | `pattern` (required), `category` (optional), `maxEntries` | `category` alone |
| `UMGToolSet.GetWidgets` | `widgetBlueprint` (refPath, no `_C`) | `widget_blueprint` |
| `SceneTools.remove_from_scene` | `actor` (refPath) | `actor_path` |
| `ObjectTools.get_properties` | `instance` (refPath), `properties` (array) | `object` |
| `ObjectTools.set_properties` | `instance` (refPath), `values` (JSON string) | `object`, `properties` |

### 2.4 Saving assets

`AssetTools.save_assets` takes `asset_paths` (array of package paths, no `.uasset` extension):

```json
{"asset_paths": ["/Game/BUH/Prototypes/TacticalDuel/Materials/M_FighterBase", "/Game/BUH/Prototypes/TacticalDuel/Blueprints/BP_CombatManager"]}
```

Saving a Blueprint fails while PIE is running — call `StopPIE` first.

### 2.5 PIE control

`StartPIE` requires `options`:

```json
{"options": {"bSimulate": false, "playMode": "PlayMode_InViewPort", "warmupSeconds": 3}}
```

Stop with `StopPIE` (empty args `{}`). Check state with `IsPIERunning`.

### 2.6 ObjectTools limitations (last verified 2026-11-04)

- Cannot read or set **object-reference** properties (`Mesh`, `SetMaterial`, `StaticMesh`, `Material`, `MaterialRenderCustom`, `bUseDefaultMaterial`).
- Can read/set primitive properties (`capsuleRadius`, `capsuleHalfHeight`, `Team`, `PlayerMove`, etc.).
- `set_properties` cannot set Blueprint member variables like `PlayerMove` on a live PIE actor (returns "could not be set").
- `set_properties` with `SetActorHiddenInGame` / `Hidden` returns `false` (silently fails). `SceneTools.remove_from_scene` **deletes** an actor permanently from the level — fine for removing a stray/duplicate actor, but **not** a substitute for hiding a runtime prediction ghost, which must be shown/hidden at runtime via `RefreshUI` (§9.14), not deleted.
- `set_properties` params: `instance` = `{"refPath":"<soft path>"}` (an object, not a bare string) and `values` = a **JSON string** of the property map, e.g. `"{\"Team\":1}"`. Passing `values` as a JSON object (or naming it `properties`) returns `false` with no error. `get_properties` instead takes `instance` (bare refPath string is accepted) + `properties` (array of names).
- **CDO refPath** for `ActorTools.get_components`/`get_properties` is the no-colon form `AssetName.Default__ClassName` (e.g. `BP_Fighter.Default__BP_Fighter_C`). The colon form `BP_Fighter.BP_Fighter:Default__BP_Fighter` is rejected as "not valid Actor".
- **Blueprint property names are lowercase and typed:** `BP_CombatManager` exposes `state`, `playerMove`, `playerX`, `opponentX`, `bHasCurrentState`, `playerWalkTarget`, … (lowercase). `playerX`/`opponentX` are **integer**, not float. `FindFProperty`/`list_properties`/`get_properties` are case- and type-sensitive — a PascalCase name or a float read on an int property silently returns the default. Confirm names/types with `ObjectTools.list_properties` (param `instance`) before reading.
- `describe_toolset` / `list_toolsets` are **top-level** tools: call `call_tool` with an empty `toolset_name` and the target toolset in the tool's own `toolset_name` argument. Passing a toolset as the `call_tool` `toolset_name` yields "Unknown tool describe_toolset".

### 2.7 SceneTools

- `find_actors` is unreliable: may return empty for actors that exist. Use `ActorTools.get_actor_transform` with a known actor name to verify existence.
- `remove_from_scene` is the reliable way to delete an actor from the level.
- `save_actor` fails for level actors; use `AssetTools.save_assets` on the level package path.

### 2.8 Automation tests

Toolset: `AutomationTestToolset.AutomationTestToolset` (NOT `editor_toolset.toolsets.automation.*`).

Workflow: `DiscoverTests` → `ListTests` (params `nameFilter` + `tagFilter`, both required) → `RunTests` (param `testNames`) → `GetTestResults`.

Command-line `UnrealEditor-Cmd -ExecCmds="Automation RunScript ..."` fails; tests must be run via MCP in the already-running headless editor.

---

## 3. Blueprint graph editing gotchas

### 3.1 `write_graph_dsl` behavior

- Does **not** wipe the graph; it attaches bodies to found/created entry nodes and deletes only orphaned nodes. Empty events survive.
- `(fn Name () ...)` requires an explicit param list, even empty. Omitting it parses the first body form as the param list.
- Own-Blueprint functions are `CallFunction|Name`; other Blueprints' members are `Class|BPName|Member`.
- Function calls with input params: use keyword args (`:Param value`) or the first positional arg wires to the `self` pin.
- A node call in expression position returns only the first data output — no DSL syntax for other named outputs.
- `if`/`elif`/`else`: `elif` must be the last form in the body; every nested `if` needs its own closing paren.
- `write_graph_dsl` on a not-yet-created function graph fails ("is not valid EdGraph"); there is no function-creation tool in the available toolsets.
- **Literal-bind limitation:** `(bind _x 0)` is valid in `read_graph_dsl` output but **fails** `write_graph_dsl` ("expression produced no output pin. Use a node call expression, not a literal."). **Workaround (confirmed 2026-11-02):** inline the literal into the consuming node call instead of binding it — e.g. `(== _state 0)`, `(* Value 1000.0)`, `(- _x 500.0)`. This unblocks the literal-value logic the limitation previously blocked (it is how the `RefreshUI` ghost-visibility rewrite, §9.14, was applied).
- **`StateMachine|GetState` pin incompatibility:** `(StateMachine|GetState <actor>)` fails `write_graph_dsl` ("Could not connect pin Manager to UpdateContext. The pins may be incompatible types."). Use `Class|BPCombatManager|GetState <actor>` instead.
- **`Behavior|SetValue` is target-first:** `(Behavior|SetValue <target> <value>)`. Value-first fails ("Could not connect pin ReturnValue to self. The pins may be incompatible types.").
- **Bare widget-getter refs fail:** `(|GetTxtOpponentQuick)` → "does not exist". Use the full form `(Variables|WBP_CombatUI|GetTxtOpponentQuick)`.
- **`read_graph_dsl` is not a round-trip:** it renames binds to `_returnvalue_N`, re-factors inlined literals back into binds, and canonicalizes node forms (`Class|BPCombatManager|GetState` reads back as `StateMachine|GetState`; full `Variables|WBP_CombatUI|GetTxtX` reads back as `(|GetTxtX)`). A successful `write_graph_dsl` call indicates the graph edit was **accepted**, not that it compiled — always compile the Blueprint, inspect for errors, save the asset, and verify the behavior separately. Read-back DSL is not guaranteed to match the submitted syntax, so do not "re-fix" a graph based on read-back string mismatches.
- **`write_graph_dsl` applies incrementally:** a timed-out write leaves a partial graph. Raise the client timeout (`MCP_DO_TIMEOUT`, default 600s in `mcp_do.py`) for large graphs, then re-apply the full corrected DSL.

### 3.2 Node naming quirks

- Boolean variable Set/Get nodes are **written** as `Variables|Default|SetHasCurrentState` (drop the `b` prefix) but **read back** as `|SetbHasCurrentState`.
- `Utilities|Array|Get(acopy)` pin `Dimension 1` has a space — use positional args, not `:Dimension1`.
- `Game|GetPlayerController` is static (input `PlayerIndex`, default 0). `Game|Player|GetOwningPlayerController` is actually `APlayerCameraManager` — do not use it for an Actor's player controller.
- `UserInterface|CreateWidget` returns base `UserWidget`; cast with `Utilities|Casting|CastToWBP_CombatUI`. `UserInterface|Viewport|AddtoViewport` (lowercase `t`) is required or the widget never appears.
- `Development|PrintString` is the correct node for debug prints (not `Debug|PrintString` or `Utilities|PrintString`).

### 3.3 `find_node_types` and `create_node`

- `find_node_types` needs `graph`, `type_id_filter`, `context_pins` (empty array lists all nodes).
- `create_node` needs `pos` (`IntPoint`), not `position`.
- New functions from other BPs only appear in the action database after that BP is compiled; `create_node` bypasses the stale cache.
- `add_component_bound_event` does NOT work for UMG widgets. Bind widget button events with `UMGToolSet.BindToEventProperty` (`eventName="OnClicked"`, `propertyName=Btn...`, `propertyClass=/Script/UMG.Button`).

### 3.4 Stale node cleanup

When a function is removed from a Blueprint, stale `CallFunction` nodes referencing it remain in other graphs and cause compile errors ("In use pin no longer exists on node X"). Fix: `BlueprintTools.find_nodes` (title filter) → `BlueprintTools.delete_node` on each stale node. A full `write_graph_dsl` rewrite is often not viable due to the literal-bind limitation.

---

## 4. UMG and Slate troubleshooting

### 4.1 `UUserWidget` tick suppression

`UUserWidget` has `meta=(DisableNativeTick)`, so a WidgetBlueprint parented to plain `UUserWidget` does **not tick** by default — but it **does** support `NativeConstruct`, and its Blueprint `Construct` event fires when the underlying Slate widget is constructed. (The earlier note that it "never calls `NativeConstruct`" was wrong.) `WBP_CombatUI`'s parent is `UUserWidget` (NOT `UBUHCombatUI`), so any C++ diagnostics written in the unrelated `UBUHCombatUI` subclass never run on it — that is why they never fired, not because `Construct` is suppressed. Its timeline logic runs in Blueprint. Native ticking is a separate issue from construction.

`SetCanTick` is NOT a member of `UUserWidget` (it exists only on `ListViewBase`). Calling it on a `UserWidget`-derived class is a compile error.

### 4.2 `RootCanvas` zero-height bug (RESOLVED)

**Problem:** Bottom timeline, slider, and instant text were invisible.
**Cause:** `RootCanvas` (CanvasPanel) is a child of `RootBox` (VerticalBox). Its `VerticalBoxSlot` had `SizeRule=Fill` with **coefficient 0**, so `RootCanvas` was allocated zero height and every child was clipped.
**Fix:** Set the slot's `size.value` to `1` (keep `sizeRule=Fill`).
**Caveat (clarified):** The zero-height clipping was real when `RootCanvas` shared the VerticalBox with other Fill children — coefficient 0 gave it no share of the space. With the current **single** Fill child, UE allocates the full height regardless of coefficient, so the coefficient-0 state is functionally harmless (confirmed by graphical PIE PASS). Setting `size.value` to `1` remains the correct fix for robustness if a second Fill child is ever added. `ObjectTools.set_properties` cannot set the sub-object slot (returns `false`), so the fix must be applied via the UMG editor or a full WBP rebuild.

### 4.3 Background brushes do not paint

`Image`/`Border` backgrounds in `WBP_CombatUI` render unreliably under certain slot configurations. Only `TextBlock` glyphs and `Slider` tracks render reliably. The dark "panels" (Left/Right) and the bottom band are the **3D arena floor** seen through the viewport, not UMG widgets. A visible background band behind the timeline remains unresolved (cosmetic; the slider/instant display already work).

`Image.brush.resourceObject` cannot be set to `/Engine/EngineResources/WhiteSquareTexture` via MCP ("not a valid object path"). Use a FullColor tint (`imageType="FullColor"`) instead.

**Note (2026-11-04):** The opponent-button "no background / unclickable" symptom was **not** a brush limitation — it was the zero-width `RightPanel` slot (§9.15). Once the slot got real width, the opponent buttons render with backgrounds and hit-test correctly. The only remaining unresolved cosmetic is the background band behind the bottom timeline.

### 4.4 SlateInspector limitations (last verified 2026-11-04)

- Only tracks **top-level** editor Slate windows. PIE game UMG widgets are composited into the viewport and are not reachable as Slate refs.
- **Popup context menus cannot be clicked** — `Click`/`PressKey` return success but don't execute on the popup window.
- `Snapshot` requires a `ref` param (empty string = all windows), not `window_index`.
- **Geometry is meaningless under `-nullrhi`**: every widget reports `pos=0,0 size=0,0`. Only useful in a graphical PIE session.

---

## 5. C++ and Unreal Build Tool

- C++ compilation is done via UBT (`BeneathUncountedHeavEditor` Linux Development). The module is `BeneathUncountedHeav`.
- LSP false positives persist on automation/combat source files; do not treat them as real errors.
- The running headless editor does NOT pick up new C++ code until the editor is restarted. After a UBT build, kill and relaunch the headless editor.
- `CombatSim.h/.cpp` implements the deterministic resolver and the Jump playback system. `CombatSimAutomation.cpp` contains `FBuhCombatSimTest`, `FBuhCombatFlowTest`, and `FBuhWalkPreviewTest`. `CombatManagerIntegrationAutomation.cpp` contains `FBuhCombatManagerIntegrationTest`. `PathRendererAutomation.cpp` contains `BUH.PathRendererTests`, `BUH.DashPathColorTests`, `BUH.DashReactionIntegrationTest`, `BUH.DashGateRegressionTest`, `BUH.DashLifecycleRegressionTest`, `BUH.JumpSimulationTests`, `BUH.JumpPathColorTests`, `BUH.JumpPlaybackIntegrationTests`, and `BUH.PieDashDiagTest`.
- `TimelinePreviewAutomation.cpp` contains `BUH.TimelinePreviewTests`, `BUH.WalkPreviewTests`, and `BUH.PIE.WalkContinueReaction` (a limitation-reporting test, §9.43).
- All 24 `BUH.*` automation suites pass.

---

## 6. Headless and graphical testing

### 6.1 Four test layers (do not conflate)

1. **C++ simulation tests** — prove the resolver logic. Run via MCP `AutomationTestToolset`.
2. **Blueprint integration tests** — prove the controller is wired and callable. Run via MCP `AutomationTestToolset`.
3. **Headless PIE state inspection** — prove initial state and no auto-advance. Via `ObjectTools.get_properties` on live PIE actors.
4. **Graphical PIE rendering** — prove the fighters and UI actually draw. Via `tests/run_graphical_visual_test.sh`.

Passing one type of test does NOT prove another type succeeded.

### 6.2 Headless `-nullrhi` limitations (last verified 2026-11-04)

- `CaptureEditorImage` returns a 1×1 black placeholder (no real rendering).
- `ProcessEvent` on Blueprint functions in the **editor world** shows no observable gameplay side effects (e.g. `SetActorLocation`) — but this is an **editor-world context** issue (§9.6), not a `-nullrhi` limitation. In a real PIE game world the same functions take effect.
- Automated button-click → lunge action-execution **cannot be proven** in the headless editor.
- Verify PIE state via actor/property inspection, not screenshots.
- **`StopPIE` can crash the editor** (SIGSEGV observed 2026-10-30). If PIE verification is complete and no further Blueprint save is needed, leave PIE running or kill/relaunch the editor instead of relying on `StopPIE`.

### 6.3 Graphical PIE verification

`tests/run_graphical_visual_test.sh` is the only way to get a real rendered screenshot. See §7 for the VRAM-safe workflow.

The script captures the **initial state** only (both fighters, UI, prediction panel). Driving button clicks requires additional automation or manual PIE.

### 6.4 Manual PIE playtest procedure

MCP cannot click UMG buttons. In a **graphical** PIE session on `Lvl_TacticalDuel`:

1. Encounter begins with "Select a move", `Your: None`, Execute visible.
2. Click `Attack` category → Quick/Heavy buttons appear.
3. Click `Quick` → `SelectedMoveText` updates, no execution.
4. Click `Execute` → `ExecuteMove` resolves.
5. Click `Move` category → Walk button appears.
6. Click `Walk` → `DestSlider` appears.
7. Adjust `DestSlider` → preview updates, no execution.
8. Click `Execute` → Walk commits through `ExecuteMove`.
9. Reaction opportunity → select and execute a legal response.
10. Click `Reset` → positions restore to ±500, health 100, `PlayerMove` back to -1.

---

## 7. Local Qwen / OpenCode / VRAM workflow

### 7.1 Graphical visual test & self-wake

`tests/run_graphical_visual_test.sh` proves the two fighters actually draw (the headless `-nullrhi` editor renders nothing). It:
1. Waits for Qwen/llama.cpp to idle-sleep (VRAM < 4 GiB).
2. Closes the headless MCP editor.
3. Launches the full graphical editor (`UnrealEditor-xwayland`).
4. Opens `Lvl_TacticalDuel`, starts PIE, captures the game view.
5. Closes the graphical editor.
6. Writes `tmp_opencode/visual_test/<run_id>/result.json`.

**Run it via a BLOCKING bash tool call** (`bash tests/run_graphical_visual_test.sh`), NOT backgrounded with `nohup … &`. The blocking call keeps the session waiting (no inference) so llama.cpp sleeps; when the script exits, the tool call returns its result and the session's next inference wakes the model automatically.

**Never launch a nested `opencode run` from inside the script** to "wake" the model. That is a nested OpenCode process that times out and never reloads the model. The wake comes from the *original* session's own next inference.

The llama.cpp idle-sleep wakes on **any** new inference request, not only interactive-session messages. A direct `POST /v1/chat/completions` to `127.0.0.1:8082` with the model ID wakes it.

### 7.2 Headless editor + Qwen coexistence

A `UnrealEditor-Cmd -nullrhi -unattended -ModelContextProtocolStartServer` process adds ~0 GiB VRAM. It is safe to **leave the headless editor running alongside Qwen/llama.cpp** — no need to wait for llama.cpp to sleep, and no need to close the editor after editing.

### 7.3 XWayland launcher

Every *graphical* launch goes through `…/Binaries/Linux/UnrealEditor-xwayland`, a wrapper that sets `SDL_VIDEODRIVER=x11` + `QT_QPA_PLATFORM=xcb` and unsets `WAYLAND_DISPLAY`, then execs the real `UnrealEditor`. This keeps the editor off native Wayland, which otherwise freezes GNOME Shell on graceful close. The real ELF must stay named `UnrealEditor` (UE target-receipt validation rejects a renamed binary). Headless `UnrealEditor-Cmd` launches are unaffected.

---

## 8. Known unresolved limitations (last verified 2026-11-04)

- **Execute-disable (re-applicable):** Adding `SetIsEnabled` on `BtnExecute` in `RefreshUI` was previously blocked by the `write_graph_dsl` literal-bind limitation, but §3.1 now documents a workaround (inline the literal into the consuming node call). Current behavior is still safe: the `ExecuteMove` `PlayerMove==-1` guard makes a stray Execute a no-op. The disable can now be re-applied if desired.
- **TopBar cosmetic overlap (re-applicable):** `TopBarContent` (HorizontalBox) holds 5 labels with no explicit spacing; they run together ("1000 100Select a move Ready"). The Fill-spacer fix was lost in the Auto4 restoration. It can now be re-applied via the §3.1 literal-bind workaround or `set_properties` slot editing.
- **Background band behind timeline:** `Border`/`Image` backgrounds render unreliably. The slider/instant display work; a visible 80–110px background band remains unresolved (cosmetic).
- **MCP cannot click UMG buttons or simulate input:** the full interaction flow (select → execute → walk → reaction → reset) requires manual graphical PIE or a C++ automation test that calls the Blueprint functions.
- **`find_actors` unreliable:** may return empty for actors that exist. Use `ActorTools.get_actor_transform` with a known name to verify.
- **Port-8000 bind race:** can occur on editor relaunch. First identify the owning process (`lsof -ti:8000`), confirm it is a stale `UnrealEditor`/MCP process, then terminate only that PID (`kill -9 <pid>`). Do not blindly `kill -9` every PID on the port — a live editor or another service may own it.

---

## 9. Important resolved issues and lessons

### 9.1 EventTick diagnostic block (RESOLVED)

**Problem:** `BP_CombatManager`'s `EventTick` contained a leftover diagnostic block that ran a hardcoded `CombatSim` simulation on the first tick and clobbered `PlayerMove` and `PlayerX`, corrupting the encounter start state.
**Cause:** The block was committed in Phase 9B as a temporary test and never removed.
**Fix:** Deleted the `MakeState`/`WithPlayerWalkTarget`/`ResolveCombat`/`GetFinalPlayerX`/`SetPlayerX`/`SetPlayerMove 6` chain. `EventTick` now contains only the instant-accumulator logic.
**Lesson:** Temporary testing code must not remain connected to normal gameplay events. Before declaring a task complete: inspect modified BeginPlay and EventTick graphs, remove temporary hardcoded simulations, verify ResetEncounter initializes correct state, ensure no debug operation overwrites authoritative combat state, read back modified Blueprint graphs, and verify actual gameplay behavior where practical.

### 9.2 Material Color parameter not connected (RESOLVED)

**Problem:** Both fighter cylinders rendered dark grey instead of blue/red.
**Cause:** `M_FighterBase` had a `Color` VectorParameter expression but it was **not connected to any material output** (neither BaseColor nor EmissiveColor). The material defaulted to grey.
**Fix:** Connected the Color parameter's RGB output to both `MP_BaseColor` and `MP_EmissiveColor` via `MaterialTools.connect_to_output`. Recompiled and saved.
**Caveat:** BaseColor alone was too dim under the scene lighting. The EmissiveColor connection makes the cylinders self-illuminate with their team colors.
**Lesson:** Always verify that a material parameter is actually connected to a material output. `get_expressions` shows the parameter exists; `connect_to_output` is required to wire it.

### 9.3 Extra fighters in level (RESOLVED)

**Problem:** Four `BP_Fighter` actors in the level (Teams 0–3) instead of two.
**Fix:** Removed `BP_Fighter_C_2` and `BP_Fighter_C_3` via `SceneTools.remove_from_scene`. Saved the level.
**Caveat:** `ObjectTools.set_properties` with `SetActorHiddenInGame`/`Hidden` returns `false` (silently fails). `remove_from_scene` is the reliable deletion path.

**Status (2026-11-04):** This was historical cleanup of duplicate fighters. The current level intentionally contains **two real fighters** (`BP_Fighter_C_0`/`_C_1`), **two prediction ghosts** (`BP_Fighter_C_2`/`_C_3`, `bIsGhost=true`, §9.12), and **one destination ghost** (`BP_DestGhost`, §9.13). Do not delete the prediction/destination ghost actors — they are required for the Walk preview. (The `_C_2`/`_C_3` names were later reused for the ghost actors.)

### 9.4 WBP compile error from stale SelectWalk nodes (RESOLVED)

**Problem:** `WBP_CombatUI` failed to compile with "In use pin no longer exists on node Select Walk."
**Cause:** Two stale `|SelectWalk` self-function calls remained in the EventGraph after the function was removed.
**Fix:** `BlueprintTools.find_nodes` (title "Select Walk") → `BlueprintTools.delete_node` on both stale nodes.
**Lesson:** When removing a function from a Blueprint, audit all other graphs for stale call nodes.

### 9.5 `ExecuteWalk` state-persistence gap (RESOLVED)

**Problem:** `ExecuteWalk` set `CurrentState` but did not set `bHasCurrentState=true`, so a Walk-then-Execute sequence would re-initialize state.
**Fix:** `ExecuteWalk` now delegates to `ExecuteMove`: `(fn ExecuteWalk () (CallFunction|ExecuteMove))`. Single authoritative pathway.

### 9.6 Integration-test editor-world context (RESOLVED — understood)

**Problem:** `BUH.CombatManagerIntegrationTests` invoked Blueprint functions via `ProcessEvent` but no observable gameplay state changes.
**Cause:** The test spawns in the **editor world**, not a PIE game world. BeginPlay's tagged-actor finding does not work in the editor world; fighter references are null; variable writes do not land.
**Conclusion:** The integration test is correctly scoped as an **invocation/wiring** test (functions exist with real bytecode and are callable without crashing). It cannot verify gameplay state. Gameplay is proven by the C++ resolver suite and manual PIE.

### 9.7 `mcp_do.py` args-file double-wrapping (RESOLVED)

**Problem:** Repeated "input param missing" errors on `ObjectTools.get_properties` and other calls.
**Cause:** The args file contained the full `toolset_name`/`tool_name`/`arguments` wrapper, but `mcp_do.py` adds the wrapper itself, creating a double-nested structure.
**Fix:** The args file must contain **only** the tool's `arguments` object.

### 9.8 Toolset name corrections (RESOLVED)

- `editor_toolset.toolsets.material_instance.MaterialInstanceTools` (underscore), NOT `material.MaterialInstanceTools`.
- `EditorToolset.EditorAppToolset` (capital E), NOT `editor_toolset.toolsets.editor.EditorAppToolset`.
- `editor_toolset.toolsets.asset.AssetTools` (singular "asset"), NOT `assets.AssetTools`.

### 9.9 Duplicate hidden StaticMeshComponent swallowed material assignments (RESOLVED)

**Problem:** Both fighter cylinders rendered grey despite logs confirming `SetMaterial` was called with the correct material (`M_TestMagenta`, the C++ blue MID, `MI_Player`/`MI_Opponent`). The material was being applied but never visible.
**Cause:** Each `BP_Fighter` instance had **two** `StaticMeshComponent`s — `BodyMesh1` (first in the component list) and `BodyMesh` (the visible rendered one). `Actor|GetComponentByClass "/Script/Engine.StaticMeshComponent"` returns the **first** match, `BodyMesh1`, so every `SetMaterial`/`SetStaticMesh` targeted the hidden duplicate while the visible `BodyMesh` kept the default grey Cylinder material.
**Fix:** Removed `BodyMesh1` from both level instances via `ActorTools.remove_component`, saved the level via `AssetTools.save_assets {"asset_paths":[]}`. `BP_Fighter.BeginPlay` then applied `MI_Player`/`MI_Opponent` to the now-visible `BodyMesh`. Verified in graphical PIE (`20261102_091120`): left fighter red, right fighter blue.
**Caveat (CORRECTED 2026-11-02):** The earlier note that the CDO "still contains `BodyMesh1`" was based on a **wrong CDO refPath** (`BP_Fighter.BP_Fighter:Default__BP_Fighter`), which `ActorTools` rejects. The working CDO refPath is the **no-colon** form `BP_Fighter.Default__BP_Fighter_C`. With it, `ActorTools.get_components` on the CDO returns exactly `DefaultSceneRoot_GEN_VARIABLE`, `BodyMesh_GEN_VARIABLE`, `Capsule_GEN_VARIABLE` — **no `BodyMesh1`**. Both level instances (`BP_Fighter_C_0`/`_C_1`) also have only `BodyMesh`. The duplicate is fully gone from class and level; newly placed instances inherit a single mesh.
**Lesson:** `GetComponentByClass` returns the first matching component. If a Blueprint has duplicate components of the same class (a hidden leftover, a renamed copy, etc.), component-targeting operations silently hit the wrong one. Always list components with `ActorTools.get_components` before relying on `GetComponentByClass`, and prefer targeting a component by its unique name. A "component still exists" conclusion drawn from a *rejected* refPath is an inference, not evidence — retry with the correct no-colon CDO refPath before claiming a duplicate persists.

### 9.10 DestroyActor after SetViewTargetWithBlend breaks the camera (RESOLVED)

**Problem:** Adding a pawn `DestroyActor` chain to `BP_CombatManager.BeginPlay` produced an all-sky/fog view (no floor, no fighters) with no log errors.
**Cause:** The `DestroyActor` ran **after** `SetViewTargetWithBlend` (to `CameraActor`). Destroying the pawn mid-blend (the blend's start was the pawn) cancelled the view transition, leaving the camera with no valid target.
**Fix:** Reordered `BeginPlay` so the pawn is destroyed **before** `SetViewTargetWithBlend`. The blend then starts from the default view and transitions to the `CameraActor`. Verified in graphical PIE (`20261102_091120`): white pawn hidden, floor + fighters visible.
**Lesson:** When hiding the default pawn by destroying it, do so before setting the view target, not after.

**Status (2026-11-04, historical):** This immediate-destroy reorder is now historical. The current cursor work (§9.16) replaced it with a **delayed** lookup-and-destruction so the view blend to `CameraActor` can complete first. See §9.16 for the current implementation.

### 9.11 Editor-visible team colors via Construction Script (RESOLVED)

**Problem:** Fighter cylinders were grey in the level editor before PIE because their team material was assigned in `BeginPlay`, which only runs at runtime. The todo required the colors to be visible in the editor without pressing Play.
**Fix:** Moved the team-material assignment into the `BP_Fighter` **Construction Script** (`UserConstructionScript` graph), which runs both in the editor and at runtime spawn:
- `ApplyMaterial` and `BeginPlay` now target the mesh with the explicit accessor `Variables|Default|GetBodyMesh` (not `GetComponentByClass`).
- The Construction Script binds `GetBodyMesh` and sets `MI_Player` when `Team == 0` else `MI_Opponent`.
- `BeginPlay` only sets the Cylinder `StaticMesh` on `GetBodyMesh`; it no longer assigns team materials (no duplicated init).
- `BP_CombatManager.ResetEncounter` still calls `Class|BPFighter|ApplyMaterial` to restore team colors after flashes, so the fixed `ApplyMaterial` preserves hit/block flash + reset behavior.
**Verified (graphical, `20261102_101311`):** pre-PIE editor view shows left opponent red / right player blue with no PIE running; flipping an instance's `Team` 0→1 recolors that cylinder red (Construction Script re-runs on the property change); PIE view keeps left red / right blue with the pawn hidden and the combat UI intact. All four combat automation tests pass.
**Lesson:** For any material/mesh that must be visible in the editor (not just at runtime), assign it in the Construction Script, not `BeginPlay`. The Construction Script re-runs whenever a variable it reads changes, so instance-editable variables (like `Team`) drive live recoloring in the Details panel. Always use an explicit component accessor (`Variables|Default|GetBodyMesh`) for gameplay-critical material/mesh writes, never `GetComponentByClass`.

### 9.12 Predictive ghost restoration (RESOLVED)

**Problem:** The Walk destination slider worked, but the predictive ghost did not move when the destination changed. The player could not tell how far they were planning to walk before pressing Execute.

**Cause:**
- No `GhostPlayer`/`GhostOpponent` actors existed in `Lvl_TacticalDuel` (they had been removed in a prior cleanup, §9.3).
- `DestSlider.OnValueChanged` never repositioned any ghost — it only called `SelectWalkDestination` and `RefreshUI`.
- `TimelineSlider.OnValueChanged` targeted tagged actors (`GhostPlayer`/`GhostOpponent`) that did not exist.
- C++ `MoveGhostsToInstant` targeted actors **named** `BP_Fighter_C_2`/`BP_Fighter_C_3`, inconsistent with the widget's tag-based targeting.
- `DestSlider` range is `0..1` (a fraction), **not** a world X coordinate. `SelectWalkDestination` used the raw slider value directly as `PlayerWalkTarget` — a bug.

**Fix:**
- Added `bIsGhost` bool variable to `BP_Fighter`; made it instance-editable via `set_variable_instance_editable`.
- Rewrote `BP_Fighter` `UserConstructionScript`: if `bIsGhost` → `MI_GhostPlayer` (Team 0) or `MI_GhostOpponent` (Team 1); else `MI_Player`/`MI_Opponent`. The Construction Script's team-color assignment no longer overwrites a ghost's special material.
- Placed `BP_Fighter_C_2` (GhostPlayer, Team 0, `bIsGhost=true`, Tags `GhostPlayer`) at `(-500,0,90)` yaw 0 and `BP_Fighter_C_3` (GhostOpponent, Team 1, `bIsGhost=true`, Tags `GhostOpponent`) at `(500,0,90)` yaw 180 in `Lvl_TacticalDuel`.
- Created translucent ghost materials: `M_GhostBase` (`BLEND_Translucent`, `Color` → `MP_BaseColor`), `MI_GhostPlayer` (cyan-blue `(0.2, 0.8, 1.0, 1.0)`), `MI_GhostOpponent` (orange-red `(1.0, 0.45, 0.2, 1.0)`).
- Rewrote `WBP_CombatUI` `DestSlider.OnValueChanged`: maps `Value` to `WorldX = Value*1000 - 500`, calls `SelectWalkDestination(manager, WorldX)`, moves GhostPlayer to `(WorldX,0,90)`, moves GhostOpponent to `(OpponentX,0,90)`, displays readout `Walk: X <PlayerX> -> <WorldX> | Distance: <dist> uu` in `TechniqueDetailsText`, calls `RefreshUI`.
- Edited `CombatSim.cpp` `MoveGhostsToInstant` to use tag-based targeting (`Actor->Tags.Contains(TEXT("GhostPlayer"))` / `TEXT("GhostOpponent")`) instead of name checks. Rebuild pending.

**DSL gotchas encountered:**
- Boolean variable getter naming: the `b` prefix is stripped — `bIsGhost` → `GetIsGhost`, not `GetbIsGhost`. First write failed with `Variables|Default|GetbIsGhost does not exist`.
- `write_graph_dsl` parenthesis counting: when rewriting a full function body, count parens per line with a script. A single extra `)` on a nested `select` line causes "Unexpected )"; a missing one causes "Unclosed parenthesis". The `StateText` line in `RefreshUI` has 6 closing parens (not 5).
- Literal-bind limitation (§3.1) blocks rewriting `RefreshUI` to add a conditional ghost reset (section 8). The ghost reset is documented as a manual test step instead.

**Verified:**
- All 4 `BUH.*` automation tests passed.
- Headless PIE: ghost positions and properties confirmed correct (GhostPlayer at -500, GhostOpponent at 500, both `bIsGhost=true`, correct tags and Team).
- Assets saved: `WBP_CombatUI`, `BP_Fighter`, `Lvl_TacticalDuel`, ghost materials.

**Known limitation:**
- **[ARCHIVED]** Section 8 (stale ghost reset when Walk cancelled) is resolved by the `RefreshUI` centralization in §9.14 — ghost show/hide now funnels through `RefreshUI` on every state change, so the literal-bind block no longer matters.
- **[ARCHIVED]** The tag-based `MoveGhostsToInstant` C++ edit was applied and the module rebuilt (all four `BUH.*` tests pass, §5); the name-based fallback is no longer relied on. The earlier "rebuild pending" note is superseded.
- Headless `-nullrhi` `CaptureEditorImage` returns 1×1 placeholder; graphical capture required for visual ghost/color evidence. MCP cannot click/drag UMG sliders; graphical interaction verification requires manual steps.

**Lesson:** When a predictive ghost system stops working, check (1) whether the ghost actors still exist in the level, (2) whether the slider handler actually repositions them, (3) whether the slider's value range matches the expected world-space coordinate space, and (4) whether C++ and Blueprint targeting use the same actor identification (tags vs names). A slider with range `0..1` is a fraction, not a world coordinate — always map it explicitly.

### 9.13 Destination ghost rendered grey after editor relaunch (RESOLVED)

**Problem:** In Walk mode the destination ghost (`BP_DestGhost`) rendered grey (default Cylinder) instead of yellow.

**Cause:** `MI_GhostDest` was created in the editor's memory but **never saved to disk**. The `BP_DestGhost` ConstructionScript's `SetMaterial` referenced `/Game/.../MI_GhostDest.MI_GhostDest`; when the headless editor was relaunched (after the graphical visual test), the unsaved material instance was lost. `SetMaterial` on a non-existent material reference fails silently, so the mesh fell back to the default grey Cylinder. The material file was absent from `Content/.../Materials/` on disk.

**Fix:** Recreated `MI_GhostDest` via `MaterialInstanceTools.create` (parent `M_GhostBase`), set `Color` to yellow `(1.0, 0.85, 0.2, 1.0)`, **saved it with `AssetTools.save_assets`**, recompiled `BP_DestGhost`, and saved it. Verified by placing a temporary untagged `BP_DestGhost` at center, saving the level, and doing a graphical capture — the center cylinder rendered yellow while the orange opponent and cyan player stayed correct. Removed the temp actor and re-saved the level.

**Lesson:** A material instance (or any asset) created via MCP exists only in the editor's memory until `AssetTools.save_assets` persists it. An editor relaunch discards unsaved assets, and a `SetMaterial`/soft-reference to a missing asset fails **silently** (no log error), leaving the mesh on its default material. After creating any asset that another asset references, save it before relaunching the editor. To verify a material's on-screen color when the target actor is hidden by UI logic, place a temporary untagged instance of that actor in the level, save the level, and do a graphical capture.

### 9.14 Ghost visibility centralized in RefreshUI (RESOLVED)

**Problem:** Stale ghost cylinders persisted after combat advanced. `DestPlayer` was always hidden, and `GhostPlayer`/`GhostOpponent` were repositioned by the slider/timeline handlers but never hidden or shown, so leftover ghosts remained at old positions after Execute/Reset/Back.

**Cause:** Ghost hide/show was scattered across widget handlers (`DestSlider.OnValueChanged`, `TimelineSlider.OnValueChanged`) with no centralized control. `RefreshUI` only handled `DestPlayer`, and the Opponent/Execute/Reset/Back button handlers never reset the hypothetical-opponent state or re-ran `RefreshUI`.

**Fix:** Centralized all three ghost visibilities in `RefreshUI`:
- `DestPlayer` hidden unless `PlayerMove == 6` (Walk).
- `GhostPlayer`/`GhostOpponent` hidden unless `PlayerMove >= 0` AND `HypotheticalOpponentMove >= 0`.
- Opponent button handlers (`OpponentQuickButton`/`Heavy`/`Guard`/`Dodge`) now call `RefreshUI` after `SetHypotheticalOpponentMove`.
- `BtnExecute`/`BtnReset`/`BtnBack` reset `HypotheticalOpponentMove` to -1 **before** `RefreshUI`.

**Verified:**
- Both graphs compiled clean via `write_graph_dsl` (no error); asset saved (`returnValue:true`).
- All 4 `BUH.*` automation tests pass.
- Headless PIE: real fighters visible (`bHidden:false`), both prediction ghosts hidden (`bHidden:true`) at encounter start — exactly the centralized logic's output when no move/opponent move is selected.

**Lesson:** Centralize all preview-ghost hide/show in one function (`RefreshUI`) and have every state-changing handler (select, execute, reset, back) funnel through it after updating the authoritative state — scattering `SetActorHiddenInGame` across slider/timeline handlers is what produced the stale ghosts. `bHidden` is readable via `ObjectTools.get_properties` on a live PIE actor, so initial-state visibility can be verified headless even though `set_properties` cannot set it. The full show/hide-on-interaction flow still needs manual graphical PIE (MCP cannot click UMG buttons, §8). The `write_graph_dsl` node-form fixes that made this rewrite possible are documented in §3.1.

### 9.15 Right-anchored CanvasPanel slot with zero width made opponent buttons unclickable (RESOLVED)

**Problem:** The four opponent "hypothetical" buttons (`OpponentQuickButton`/`Heavy`/`Guard`/`Dodge`) rendered their text but never received hover/press/click. The left player buttons worked fine. A diagnostic `TextBlock` (`TempDiagnosticText`) bound to `OpponentDodgeButton`'s `OnHovered`/`OnPressed` never updated. The buttons' properties were byte-identical to the working player buttons (same `clickMethod`/`visibility`/`renderOpacity`/style/slot), so the bug was not in the buttons or their ancestor chain.

**Cause:** `RightPanel`'s `CanvasPanelSlot` had **coincident horizontal anchors** (`Min.X=1`, `Max.X=1`, right-anchored) with `Right=0`. When both anchors on an axis coincide, the offset on that axis defines the slot's **arranged size**, not a margin. `Right=0` → **zero arranged width**. The child `SizeBox` (desired width 300) still painted its buttons outside that zero-width rect, so the text was visible but the hit-test area was empty. The working `LeftPanel` is left-anchored (`Min.X=Max.X=0`) with `Right=300` (width 300) — that asymmetry is why the left worked and the right did not.

**Red herrings ruled out:** (1) The full-canvas `LeftPanelBG` `Image` at zOrder 0 was suspected of swallowing right-side hits, but within the same zOrder `RightPanel` (index 18) paints above the image (index 0), so it never blocked the right side — setting it `SelfHitTestInvisible` changed nothing. (2) The §4.3 "background brushes don't paint" note was a misread of this same zero-width symptom.

**Fix:** Set the slot's `Right` offset to `300` (offsets `Left=-380, Top=60, Right=300, Bottom=-80`), kept anchors `(1,0)->(1,1)`, set `bAutoSize=false` and `Alignment=(0,0)`. Applied via `ObjectTools.set_properties` on `WidgetTree.RootCanvas.CanvasPanelSlot_18` with the correct args shape (`instance={"refPath":...}`, `values="<JSON string>"` — see §2.6). Compiled + saved `WBP_CombatUI`.

**Verified (graphical PIE, `2026-11-04 14-32-42`):** Hovering the Dodge button shows `DODGE HOVER` in the top-center diagnostic and the button highlights (hover state) with a real background; all four opponent buttons now render with backgrounds and receive mouse events.

**Status (partially resolved, 2026-11-04):** RightPanel hit-testing is fixed and the Dodge hover is verified in graphical PIE. However, all four opponent button **click** handlers (`OpponentQuick`/`Heavy`/`Guard`/`Dodge` → `SetHypotheticalOpponentMove` → prediction) still require complete manual verification, and end-to-end hypothetical selection + timeline generation is not yet confirmed. Do not reopen the hit-testing investigation without new evidence, and do not declare the prediction feature complete merely because the buttons are now clickable.

**Lesson:** A `CanvasPanel` slot with coincident anchors on an axis uses the offset on that axis as the widget's **size**, not a margin. A right-anchored slot (`Min.X=Max.X=1`) with `Right=0` has zero width — children paint but are unclickable. The signature symptom is **visible button text with no hover/press/click** while an identical button in a correctly-sized slot works. When a UMG widget renders but doesn't hit-test, check the slot's arranged width first (`UCanvasPanel::GetGeometryForSlot` or a post-layout geometry read), before suspecting z-order, visibility, or input mode.

### 9.16 Persistent mouse cursor for tactical combat (RESOLVED)

**Problem:** The mouse cursor was captured by the first-person mouse-look, so the combat UI buttons could not be hovered or clicked.

**Cause:** The default first-person player controller's mouse-look input mapping context (`IMC_Default`/`IMC_MouseLook`) was active during the encounter, capturing the mouse for camera rotation and hiding the cursor.

**Fix:**
- `BP_CombatManager.BeginPlay` sets the player input mode to **UI Only** so the cursor is visible and all mouse input routes to the UMG.
- `WBP_CombatUI` is focusable so it can receive and dispatch mouse events.
- Removed the `IMC_Default`/`IMC_MouseLook` mapping contexts from `BP_FirstPersonPlayerController` so mouse-look no longer captures the cursor during combat.
- The default pawn is hidden via a **delayed** lookup-and-destruction (after the view blend to `CameraActor` completes), replacing the earlier immediate-destroy sequence (§9.10, now historical).

**Distinction:** This is the **tactical combat** input mode. Future exploration/movement gameplay will need a different input mode (game + UI) where the cursor is hidden and the mouse drives camera look. Do not conflate the two.

**Verified (graphical PIE, 2026-11-04):** cursor visible over the UI; opponent and player buttons hover and click (§9.15). All four `BUH.*` tests pass.

### 9.17 Continue Walk button missing for Walk Toward / Walk Away (RESOLVED)

**Problem:** After selecting Walk Toward or Walk Away and clicking Execute, the Continue button did not appear during a reaction opportunity, even though "Reaction opportunity" was displayed. The button only worked for Walk destination (Action 6).

**Root cause:** The UI Walk buttons commit Actions 4 (`BtnWalkToward` → `SelectWalkToward`) and 5 (`BtnWalkAway` → `SelectWalkAway`). There is no Walk destination button in the UI. `GetReactionInfo` (line 1573) only included decision 7 in `LegalActions` when `PlayerCurrentAction == 6`, so Actions 4 and 5 produced an empty list and the button stayed Collapsed. The `RefreshUI` visibility chain and button parent chain were structurally correct.

**Evidence:** Diagnostic `PrintString` nodes added to `ExecuteMove` (before `RefreshUI`) confirmed in the graphical PIE log (`feedback.md`): `bReactionOpportunity=1`, `ReactionFighter=0`, `ObservedThreatAction=1`, `PlayerMove=6`. The diagnostic prints were removed after the fix.

**Fix:**
- `CombatSim.cpp` line 1573: `GetReactionInfo` eligibility changed from `PlayerCurrentAction == 6` to `PlayerCurrentAction == 4 || PlayerCurrentAction == 5 || PlayerCurrentAction == 6`, matching `IsReactableWalk`.
- `CombatSim.cpp` line 59: `MoveName(7)` renamed from "Continue Walk" to "Continue".
- `WBP_CombatUI`: `TxtContinueWalk` text changed from "Continue Walk" to "Continue" via `ObjectTools.set_properties`.
- `BP_CombatManager.ExecuteMove` restored to clean state (diagnostic prints removed) via `write_graph_dsl` with the boolean variable write-form fix (`Variables|Default|GetHasCurrentState` / `SetHasCurrentState`, not `|GetbHasCurrentState`).

**Verified:**
- All 4 `BUH.*` automation suites pass.
- Graphical PIE confirmed by the user: Continue button appears during reaction opportunities for Walk Toward and Walk Away.

**Lesson:** When a reaction decision is missing from the UI, check the C++ eligibility check (`GetReactionInfo`) against the actual action IDs the UI commits. The UI Walk buttons commit Actions 4 and 5, not 6. `IsReactableWalk` already covered all three, but `GetReactionInfo` did not. The two checks must stay in sync. Diagnostic `PrintString` nodes in `ExecuteMove` are the fastest way to confirm the actual PIE state when headless PIE cannot reproduce the scenario.

### 9.18 Stale CurrentState lifecycle caused walk to commit from wrong positions (RESOLVED)

**Problem:** Destination Walk (Action 6) committed from stale fighter positions instead of the current `PlayerX`/`OpponentX`. The walk target was correct, but the starting position in the sim state was wrong, causing the fighter to move from the wrong location.

**Cause:** `ExecuteMove` selects between two states: if `bHasCurrentState` is true, it reuses the stored `CurrentState`; otherwise it builds a fresh `MakeState` from `PlayerX`/`OpponentX`. After any prior `ExecuteMove`, `bHasCurrentState` was set to true and never cleared. `ResetEncounter` restored positions and health but did **not** clear `bHasCurrentState` or `CurrentState`, so the next `ExecuteMove` reused the stale state from the previous encounter.

**Fix:**
- `ResetEncounter` now calls `Variables|Default|SetHasCurrentState false`, ensuring the first `ExecuteMove` after a reset uses a fresh `MakeState` with current `PlayerX`/`OpponentX`.
- `ExecuteMove` uses `Variables|Default|GetHasCurrentState` (not the non-existent `|GetbHasCurrentState`) to read the flag.
- Added `LogCombatState` (C++ helper in `CombatSim.cpp`) for `[WalkTrace]` diagnostics before/after `ResolveCombat` in `ExecuteMove`, proving the state transition.

**Verified:**
- `[WalkTrace]` logs confirmed: PreResolve shows fresh state with correct positions; PostResolve shows the walk progressing from the correct starting position.
- All 4 `BUH.*` automation suites pass.
- PIE property inspection: `PlayerMove=-1` (idle), `PlayerX=-500`, `OpponentX=500` after `ResetEncounter`.
- First scene is clean: no green walk-target line (`PlayerMove != 6`), no visible ghost (`BP_DestGhost_C_0.bHidden=true`).

**Lesson:** A boolean "has valid state" flag must be cleared on every reset path. If `ResetEncounter` restores positions but leaves `bHasCurrentState=true`, the next execution reuses the old `CurrentState` with stale positions. The flag and the state it guards must be reset together. The `b` prefix is stripped in Blueprint variable node names (`bHasCurrentState` → `GetHasCurrentState`/`SetHasCurrentState`), so the old `|GetbHasCurrentState` form fails with "does not exist".

### 9.19 Reusable movement path renderer replaces DrawGroundLine (RESOLVED)

**Problem:** The Walk targeting visualization used `DrawDebugLine`, a development-only API that does not persist in shipping builds and cannot support future multi-segment colored paths (Dash red/green, Jump 3D parabola). Additionally, the path line rendered black instead of bright green, and the path was never cleared after targeting ended.

**Cause:** No dedicated runtime path renderer existed. `DrawGroundLine` in `CombatSim.cpp` called `DrawDebugLine` directly. The unlit materials had `Constant3Vector` connected to `MP_BaseColor` instead of `MP_EmissiveColor` — Unlit shading uses Emissive for visible color. Path clearing was not wired because `write_graph_dsl` fails on the pre-existing `StateMachine|GetState` node in `WBP_CombatUI:EventGraph`.

**Fix:**
- Created `ABUHPathRenderer` C++ actor (`Source/BeneathUncountedHeav/PathRenderer.h`/`.cpp`) with a `USplineComponent` root and four `UStaticMeshComponent` segments. UE 5.8 `USplineMeshComponent` lacks `StartSplinePointIndex`/`EndSplinePointIndex`, so `UStaticMeshComponent` cubes positioned along the spline are used instead.
- `SetLine(Start, End, Color)` positions the spline with two points and places the four cube segments along it. `ClearPath()` hides all segments. `SetPathVisible(bool)` toggles visibility.
- `M_PathGreen` and `M_PathRed` unlit materials: `Constant3Vector` connected to `MP_EmissiveColor` (not `MP_BaseColor`). Assigned to the actor's `GreenMaterial`/`RedMaterial` `EditDefaultsOnly` properties via `ConstructorHelpers::FObjectFinder` in the constructor.
- Actor `BUHPathRenderer_1` placed in `Lvl_TacticalDuel` with tag `PathRenderer`.
- `UBuhCombatSim::DrawGroundLine` modified to iterate `TActorIterator<ABUHPathRenderer>`, call `SetLine(Start, End, EPathSegmentColor::Green)` on the first found renderer, and fall back to `DrawDebugLine` if none exists.
- `DrawWalkPath` and `ClearWalkPath` static helpers added to `UBuhCombatSim` as wrappers around the PathRenderer.
- **Path clearing wired via targeted node-level edits** (not DSL rewrite):
  - `BP_CombatManager:ExecuteMove` — `ClearWalkPath` node added after `RefreshUI`, connected via exec chain + `GetPlayerController` → `PC` pin.
  - `BP_CombatManager:ResetEncounter` — same pattern.
  - `WBP_CombatUI:EventGraph` walk-cancel branch — `ClearWalkPath` node inserted between `SetActorHiddenInGame` and the next node in the right-click/Escape cancel chain.
  - Enter-key execute, R-key reset, BtnExecute, BtnReset, BtnBack all route through `ExecuteMove` or `ResetEncounter`, so they inherit the clearing automatically.

**Verified:**
- PIE: actor found with correct materials, tag, spline, and 4 mesh segments.
- All 4 `BUH.*` automation suites pass (run twice: after material fix and after ClearWalkPath wiring).
- No PathRenderer errors in logs.
- Both Blueprints compiled cleanly and saved.

**Lesson:** UE 5.8 `USplineMeshComponent` does not expose `StartSplinePointIndex`/`EndSplinePointIndex`; use `UStaticMeshComponent` segments positioned along a `USplineComponent` instead. Unlit materials must supply color through `MP_EmissiveColor`, not `MP_BaseColor`. `EditDefaultsOnly` properties on C++ actors must be assigned in the constructor via `ConstructorHelpers::FObjectFinder`. The `write_graph_dsl` tool cannot rewrite graphs containing `StateMachine|GetState` nodes; use targeted node-level edits (`create_node`, `connect_pins`, `break_pins`) or C++ routing instead. When a function graph (e.g. `ExecuteMove`, `ResetEncounter`) is the natural insertion point, adding `ClearWalkPath` there covers all callers (keyboard, buttons, EventTick) without modifying the complex EventGraph.

### 9.20 Path renderer upgraded to a reusable segment pool + renderer-only automation (RESOLVED)

**Problem:** The original `ABUHPathRenderer` used four fixed `UStaticMeshComponent` segments, which cannot represent variable segment counts or sampled 3D curves (Jump parabola) with per-segment red/green boundaries. The verification todo required proving the renderer alone draws straight/segmented/curved paths, reuses components without leaking, and that the real gameplay call path (`DrawWalkPath`/`ClearWalkPath`) works.

**Cause:** Fixed 4-segment design; no explicit per-segment color ranges; no automation exercising the renderer or the CombatSim statics.

**Fix:**
- Rewrote `PathRenderer.h`/`.cpp`: replaced the fixed `MeshSegments` array with a `Transient` `SegmentPool` (`TArray<TObjectPtr<UStaticMeshComponent>>`) grown on demand via `NewObject<UStaticMeshComponent>` + `SetupAttachment` + `RegisterComponent` + `SetCastShadow(false)`. `SetPath(Points, Segments)` now takes `FBuhPathSegment {StartPointIndex, EndPointIndex, Color}` and places one piece per consecutive point pair inside each segment's index range, so red/green transitions land on supplied boundaries. `SetLine` wraps 2 points / 1 segment. `LastPathPieceCount` tracks visibility count because `USceneComponent` has no public `IsHiddenInGame()` getter.
- Added `PathRendererAutomation.cpp` → `BUH.PathRendererTests` (EditorContext): A straight green line; B straight red→green at a supplied boundary (not midpoint); C sampled 3D parabolic arc with real vertical curvature and a red→green boundary at sample 3 (not the apex); D 20 varied draws proving pool reuse (size == high-water mark, bounded) and no actor/component leak (count returns to baseline); E full lifecycle through the real `UBuhCombatSim::DrawWalkPath`/`ClearWalkPath`/`DrawGroundLine` statics via a spawned `APlayerController` (appear → update → clear → reappear → ground line → final clear).
- `DrawGroundLine`'s `DrawDebugLine` fallback is **kept** as a no-renderer safety net; it is unreachable in the shipped level because `BUHPathRenderer_1` is placed.

**Verified:**
- `BUH.PathRendererTests` passes (A/B/C/D/E). All 4 existing `BUH.*` suites still pass — no regression.
- PIE: `BUHPathRenderer_1` present with `M_PathGreen`/`M_PathRed`/`Cube`; no PathRenderer errors in logs.

**Lesson:** For variable-count, multi-color, sampled paths, a per-point-pair reusable static-mesh pool beats a fixed segment count and beats `USplineMeshComponent` (which only connects two endpoints and cannot follow intermediate samples). Track visibility count in a member (`LastPathPieceCount`) rather than querying `IsHiddenInGame` (no public getter). A `for (TActorIterator ...; It; ++It) { ...; break; }` loop trips `-Werror,-Wunreachable-code-loop-increment`; use `TActorIterator It(World); if (It) ...` instead. The CombatSim statics resolve the world from `PC->GetWorld()`, so a headless lifecycle test must spawn an `APlayerController` in the editor world to drive them.

### 9.21 Removed per-frame PrintString flood from WBP_CombatUI:EventTick (RESOLVED)

**Problem:** The screen was flooded every frame with three obsolete diagnostics in the `EventTick` walk-targeting branch: `Mouse: X/Y`, `Ground: X/Y`, and `DIAGONAL TARGET - X only will be used`. The "X only" warning contradicted the verified 2D Walk implementation (`SelectWalkDestination` receives both X and Y).

**Cause:** Leftover `Development|PrintString` exec nodes (and a misleading `Branch` wrapping the diagonal one) inside the `(if (|GetbWalkTargeting) ...)` branch, which runs every frame while targeting.

**Fix (targeted node-level edits, no EventGraph rewrite):**
- Identified the three nodes by dumping each `PrintString`'s `InString` pin via `get_node_infos`: `K2Node_CallFunction_23` (Mouse, InString ← string-append), `K2Node_CallFunction_30` (Ground, InString ← string-append), `K2Node_CallFunction_41` (DIAGONAL, literal text) inside `K2Node_IfThenElse_1`.
- Rewired the exec chain and deleted: `IfThenElse_0.then → CallFunction_25.execute` then delete `CallFunction_23`; `CallFunction_27.then → CallFunction_31.execute` then delete `CallFunction_30`; `CallFunction_38.then → IfThenElse_2.execute` then delete `CallFunction_41` and `IfThenElse_1`.
- Retained the non-flood diagnostics: `WBP_CombatUI CONSTRUCTED` (one-shot) and the four `HYPOTHETICAL CLICK: ...` (fire only on button press).

**Verified:**
- `compile_blueprint` clean; re-read DSL confirms `Mouse:`/`Ground:`/`DIAGONAL` gone, `SelectWalkDestination` still receives both `_returnvalue_2` (X) and `_returnvalue_3` (Y) — 2D Walk intact, no X-only fallback.
- All 5 `BUH.*` automation suites pass (5/5, 0 failed) — no gameplay regression.
- PIE: exactly one `BUHPathRenderer_1` actor, no accumulation.
- Lifecycle (§2) confirmed by code + `BUH.PathRendererTests` E: `SetLine` is persistent; EventTick redraws the confirmed line while `GetPlayerMove == 6` (Walk), so confirm keeps the line fixed, cancel-reselection restores it (move still 6), cancel-without-target clears it (move ≠ 6), and no redraw occurs after `ClearWalkPath` when move ≠ 6.

**Lesson:** To remove an exec node mid-chain, reconnect its predecessor's `then` to its successor's `execute` (`connect_pins` auto-breaks the old single exec link) before `delete_node`; for a `Branch` whose only purpose is a diagnostic, delete the child `PrintString` first, then the `Branch`, after rewiring the branch's input `then` to the next exec node. `get_node_infos` exposes each pin's `value` and `connected_pins`, which is how you map an anonymous `K2Node_CallFunction_N` to its actual `PrintString` text. `find_actors` returns `refPath` objects (actor name = last path segment), not `name`/`actorClass` fields.

### 9.22 Stale Walk path/marker lingered after Back → other action (RESOLVED)

**Problem:** After selecting Walk, confirming a destination, clicking Back, then choosing another action (e.g. Quick Strike), the green Walk path and the yellow DestPlayer marker remained visible. The screenshot showed "Your: Quick" with the old green line still rendered.

**Cause (confirmed from live `WBP_CombatUI:EventGraph` DSL):**
- `OnClicked(BtnBack)` set `HypotheticalOpponentMove=-1`, `RefreshUI`, toggled buttons, and hid the DestPlayer marker, but never called `ClearWalkPath`, never reset `PlayerMove`, and never set `bWalkTargeting=false`.
- `OnClicked(BtnQuick/Heavy/Guard/Dodge)` called `Select*` + `RefreshUI` + button visibility but none called `ClearWalkPath`, hid DestPlayer, or exited Walk targeting.
- `EventTick` (line 66–68) redraws the Walk line every frame while `GetPlayerMove == 6`; it never clears when leaving Walk. `SetLine` on `ABUHPathRenderer` is persistent, so the last-drawn line lingers. After Walk confirm `PlayerMove==6`; Back left it 6, so EventTick kept redrawing to the (hidden) DestPlayer location until `PlayerMove` changed.

**Fix (shared helper + targeted node edits, no EventGraph rewrite):**
- Created a `ClearPendingMovementPreview` function graph on `WBP_CombatUI` that does, in order: `CombatSim|ClearWalkPath (Game|GetPlayerController 0)`, hide all `DestPlayer` actors (`Actor|GetAllActorsWithTag` + `Rendering|SetActorHiddenInGame`), and `Class|BPCombatManager|SetPlayerMove -1 (Variables|Default|GetManager)` (deselect so EventTick stops redrawing).
- Wired the helper as the first exec node in all five handlers: Back (`K2Node_ComponentBoundEvent_19`), Quick (`_0`), Heavy (`_1`), Guard (`_2`), Dodge (`_5`). For Quick/Heavy/Guard/Dodge the subsequent `Select*` overrides `PlayerMove` to 0/1/2/3; for Back it stays `-1`.
- `SetPlayerMove` is a cross-class variable-setter on `BPCombatManager` (type_id `Class|BPCombatManager|SetPlayerMove`), which `create_node` CAN create — unlike same-class setters (`|SetbWalkTargeting` on `WBP_CombatUI` itself), which `create_node`/`write_graph_dsl` cannot create. So the helper cannot reset `bWalkTargeting`; documented as a known limitation (only matters for the edge case of clicking an action *during* active targeting, which is not in the test scenarios — after Walk confirm `bWalkTargeting` is already false).

**Verified:**
- `compile_blueprint` clean; re-read DSL confirms `ClearPendingMovementPreview` is the first node in Back/Quick handlers.
- All 5 `BUH.*` automation suites pass (5/5, 0 failed) — no gameplay regression.
- Graphical PIE 11-scenario verification is user-driven (headless `-nullrhi` cannot click UMG buttons); see the manual steps in `todo.md` §5.

**Lesson:** `K2Node_ComponentBoundEvent`'s exec `then` output is at pin `index_id 1` (index 0 is the `OutputDelegate` data pin) — connecting index 0 to an exec input fails with "Could not connect pin OutputDelegate to execute". `create_node` creates cross-class variable-setters (`Class|X|SetVar`) but not same-class ones (`|SetVar`); route same-class variable sets through a different mechanism or accept the limitation. A shared preview-cleanup helper called at every action-selection boundary is cleaner than per-button workarounds, and resetting `PlayerMove` to the no-move sentinel (`-1`, i.e. `PlayerMove < 0`) is what actually stops the EventTick redraw — clearing the renderer geometry alone is not enough while `PlayerMove==6`.

### 9.23 write_graph_dsl wipes EventTick bodies; cross-class TargetingAction replaces same-class targeting flags (RESOLVED — Dash button event still open)

**Problem 1 (EventTick wipe):** While wiring Dash targeting, a `write_graph_dsl` call targeting `WBP_CombatUI:EventGraph` with a small Dash test block **replaced the entire EventTick body** with only that block, instead of attaching to the existing exec flow. The original Walk targeting, keyboard shortcuts (One/Two/Three/Four/Enter/R), and timeline slider (Left/Right) all vanished from the exec flow. `find_nodes` confirmed `DrawGroundLine`, `GetbWalkTargeting`, and `ClearWalkPath` nodes were deleted (not orphaned). This contradicts the earlier belief that `write_graph_dsl` "attaches bodies to found entry nodes (does not wipe the graph)" — that holds for **function graphs**, but for **event graphs** (EventTick) it wipes the event body.

**Problem 2 (same-class variable nodes not creatable):** The original EventTick gated Walk targeting on the same-class boolean `bWalkTargeting` (getter type_id `|GetbWalkTargeting`). Both `create_node` and `write_graph_dsl` fail to create any same-class `WBP_CombatUI` variable getter/setter — even for variables that already exist (`|GetbWalkTargeting`, `|SetbWalkTargeting`, `|GetbTargetingDash`, full-form `Variables|WBP_CombatUI|GetWalkTargeting` all return "does not exist"). The existing nodes were created through manual editor work. So the wiped EventTick could not be restored verbatim via MCP.

**Problem 3 (cross-class variable setters mis-wire):** `(Class|BPCombatManager|SetTargetingAction _manager -1)` fails `write_graph_dsl` with `Could not connect pin Manager to TargetingAction. The pins may be incompatible types.` Cross-class variable **getters**, comparisons, and **function calls** merge cleanly, but cross-class variable **setters** do not.

**Fix (cross-class TargetingAction design, no same-class variable nodes):**
- Added `TargetingAction` int to `BP_CombatManager` (default `-1`; `6`=Walk, `8`=Dash, `-1`=none).
- Added three wrapper functions on `BP_CombatManager` to dodge the setter mis-wire: `BeginWalkTargeting` (`SetTargetingAction 6`), `BeginDashTargeting` (`SetTargetingAction 8`), `CancelTargeting` (`SetTargetingAction -1`).
- Rewrote `WBP_CombatUI:EventTick` from scratch using only creatable nodes: gate on `(>= (Class|BPCombatManager|GetTargetingAction _manager) 0)`, branch on `== 8` for Dash (`DrawDashPath` + `SelectDashDestination`) vs Walk (`DrawGroundLine` + `SelectWalkDestination`), `CancelTargeting` on Left-click confirm and Right-click/Escape cancel, plus after-confirm `DrawGroundLine`/`DrawDashPath`, the keyboard shortcuts, and the timeline slider. Uses `Class|BPCombatManager|GetState` (NOT `StateMachine|GetState`, which fails with an `UpdateContext` pin conflict).
- Rewrote `OnClicked(BtnWalkMove)` to call `BeginWalkTargeting` (replacing the now-unused same-class `bWalkTargeting`/`bSuppressWalkClick`/`bShowMouseCursor` setters).
- Added `BtnDashMove` (Button) + `TxtDashMove` ("Dash") to the Move submenu via `UMGToolSet.UMGToolSet.AddWidget`, set the text via `ObjectTools.set_properties` (params are `instance` + `values` as a JSON string), and made it a variable via `ToggleWidgetAsVariable`.

**Verified:** `compile_blueprint` clean on both `WBP_CombatUI` and `BP_CombatManager`; all 5 `BUH.*` automation suites pass (5/5, 0 failed).

**RESOLVED (BtnDashMove OnClicked event, automatic):** The `OnClicked` `K2Node_ComponentBoundEvent` for a newly-added widget variable is NOT registered by `ToggleWidgetAsVariable`/`CompileWidgetBlueprint`/`compile_blueprint`/`save_assets`/`load_asset`, and both `write_graph_dsl` (`event OnClicked(BtnDashMove)`) and `create_node` (`AddEvent|OnClicked(BtnDashMove)`) fail with "does not exist". The fix is the **native editor API** exposed by the installed MCP server: `UMGToolSet.UMGToolSet.BindToEventProperty` (wraps `FWidgetBlueprintOperationUtils::BindToEventProperty`, UMGEditor module). Call it with `widgetBlueprint`=WBP_CombatUI, `eventName`="OnClicked", `propertyName`="BtnDashMove" (the variable that owns the event), `propertyClass`=`/Script/UMG.Button`. It returns `true` and creates the same bound-event node the Widget Designer's "+" button makes. After that, `write_graph_dsl` with `(event OnClicked(BtnDashMove) ...)` attaches the body normally (it calls `BeginDashTargeting` + cursor + targeting text). Compiled clean, saved, all 5 `BUH.*` suites pass, widget still visible/clickable. **No manual editor step is needed.**

**Lesson:** (1) `write_graph_dsl` on an **event graph** (EventTick) wipes the event body — only function-graph writes attach; never use it to "add a block" to EventTick, rewrite the whole event instead. (2) Same-class `WBP_CombatUI` variable getters/setters are not creatable by MCP — design targeting gates around cross-class variables/functions. (3) Cross-class variable setters mis-wire in `write_graph_dsl`; wrap them in a same-class function on the target class and call that. (4) `StateMachine|GetState` conflicts on an `UpdateContext` pin — use `Class|BPCombatManager|GetState`. (5) A newly-added widget variable's `OnClicked` ComponentBoundEvent is not class-registered by the ordinary compile/save/reload path — use `UMGToolSet.UMGToolSet.BindToEventProperty` (native `FWidgetBlueprintOperationUtils::BindToEventProperty`) to create the bound-event node automatically, then populate it with `write_graph_dsl`. (6) `ObjectTools.set_properties` takes `instance` + `values` (JSON string), not `widget`/`properties`.

### 9.24 Dash cursor disappearance, stale initial state, and misplaced Dash button (RESOLVED)

**Problem 1 (cursor disappears after Dash click):** Both `SetInputModeGameAndUI` nodes in `WBP_CombatUI` (`K2Node_CallFunction_359` in `OnClicked(BtnWalkMove)`, `K2Node_CallFunction_530` in `OnClicked(BtnDashMove)`) had `bHideCursorDuringCapture = true`. For the Game-and-UI input mode the cursor is shown by default, but the capture-hide flag suppressed it the moment the targeting input mode was applied, so the cursor vanished on the Dash (and Walk) button click. `Game|Player|SetShowMouseCursor` does not exist as a DSL node form, so the fix had to target the input-mode node's own pin. **Fix:** `BlueprintTools.set_pin_value` on input pin index 4 (`bHideCursorDuringCapture`) = `false` on both nodes. Verified both read back `false`.

**Problem 2 (initial state not clean):** Two gaps. (a) `EventConstruct` hid `BtnWalkMove` but never `BtnDashMove`, so the Dash button was visible on a fresh launch. (b) `ResetEncounter` reset health/moves/positions/materials and called `ClearWalkPath` + `RefreshUI`, but did not reset `TargetingAction` or hide the `DestPlayer` marker, so a mid-session Reset (R key) left stale targeting state and a visible marker. **Fix:** added a `GetBtnDashMove` "Hidden" line to `EventConstruct` (and mirrored `BtnDashMove` visibility in all 12 events that reference `BtnWalkMove` — shown in `OnClicked(BtnMove)`/`OnClicked(BtnDodge)`, hidden in the rest). For `ResetEncounter`, `write_graph_dsl` could not recreate the same-class bool setters `|SetbHasCurrentState`/`|SetbReactionOpportunity` (they exist in the graph but are not creatable), so instead of rewriting the function I **appended nodes via `create_node` + `connect_pins`**: a `CallFunction|CancelTargeting` node (resets `TargetingAction` to -1) wired after `ClearWalkPath.then`, then an `Actor|GetAllActorsWithTag "DestPlayer"` node feeding a `Rendering|SetActorHiddenInGame` node (bHidden=true). Verified the fresh PIE CombatManager reads `State=0, TargetingAction=-1, PlayerMove=-1, OpponentMove=1, health 100/100, bHasCurrentState=false`.

**Problem 3 (Dash button below Execute):** `BtnDashMove` had been appended to the end of `LeftPanelContent` (child index 17), placing it below Execute rather than alongside Walk (child index 8). **Fix:** `UMGToolSet.UMGToolSet.MoveWidget` with `widget=BtnDashMove`, `newParent=LeftPanelContent`, `childIndex=9`. Verified the new order: index 8 `BtnWalkMove`, 9 `BtnDashMove`, 10 `DestSlider`. The `OnClicked(BtnDashMove)` binding and `BeginDashTargeting` call were preserved (no duplicate button).

**Verified:** `compile_blueprint` clean on both `WBP_CombatUI` and `BP_CombatManager`; both saved; all 5 `BUH.*` automation suites pass (5/5, 0 failed). Fresh PIE CombatManager state confirmed clean via `ObjectTools.get_properties`.

**Still requires manual graphical PIE confirmation:** (1) the cursor actually staying visible after the Dash click and while moving over the arena (a visual property that `-nullrhi` headless PIE cannot capture); (2) the full section-5 action-transition sequence (fresh Play → Move submenu → Dash → confirm → Back → Walk → Dash → Quick → Execute → Reset → repeat); (3) that clicking another UMG button during targeting does not confirm a ground destination (the original Walk flow had no explicit click-suppression and worked in PIE, so the identical Dash flow is expected to behave the same — confirm visually).

**Lesson:** (1) A disappearing cursor under `SetInputModeGameAndUI` is usually `bHideCursorDuringCapture=true` on the input-mode node — fix it with `set_pin_value` on that node's input pin, not a `SetShowMouseCursor` call (which has no DSL node form). (2) When `write_graph_dsl` cannot recreate same-class bool setters that already exist in a function graph, don't rewrite the function — **append** the needed nodes with `create_node` + `connect_pins` and wire them into the existing exec chain. (3) A widget "below Execute" is a child-index problem in the panel — fix it with `UMGToolSet.MoveWidget` (`childIndex`), not by recreating the widget. (4) Fresh-launch UI cleanliness requires the Construct event to hide every submenu button, not just the ones that existed when it was written.

### 9.25 Dash path stayed all-red at a reaction pause; remaining path must reflect reaction eligibility (RESOLVED)

**Problem:** During Dash execution, `WBP_CombatUI:EventTick` redraws the remaining Dash path every frame via `CombatSim|DrawDashPath` (node `K2Node_CallFunction_276`) from the fighter's **current** position to the `DestPlayer` marker. But `DrawDashPath` always applied the 3-locked-Instant red boundary from the path start, ignoring how many movement Instants had already elapsed. At a reaction pause (`PlayerPhase==2`, `PlayerInstantsInPhase>=3`) the entire remaining path should be green (reactable), but it was drawn all red — the locked window was being re-counted from the current position instead of from the original Dash start.

**Root cause:** `DrawDashPath` had no notion of elapsed movement Instants. It unconditionally simulated 3 locked Instants from `Start` and colored that prefix red. During execution `Start` is the fighter's current position (already partway along the Dash), so the 3-Instant red prefix was being applied to the *remaining* path rather than the *original* locked window.

**Fix (C++):** `UBuhCombatSim::DrawDashPath` now takes `int32 ElapsedMovementInstants = 0`. It computes `RemainingLocked = FMath::Max(0, 3 - ElapsedMovementInstants)`. If `RemainingLocked <= 0` (all locked Instants elapsed) it draws the entire remaining line green via `SetLine`. Otherwise it simulates the remaining locked Instants Instant-by-Instant from `Start` (90 uu/Instant) to compute the red/green boundary, then draws red `Start→Boundary` and green `Boundary→End` via `SetPath` (or a single red `SetLine` if the path is entirely within the locked window). The default `0` keeps the planning-preview call (node `K2Node_CallFunction_243`) unchanged — it still draws the first 3 Instants red.

**Fix (Blueprint):** In `WBP_CombatUI:EventTick`, the execution-path `DrawDashPath` call (`K2Node_CallFunction_276`) now passes `(PlayerPhase==2) ? PlayerInstantsInPhase : 0` as the 4th argument. Built with `create_node` + `connect_pins` (same-class variable getters are not creatable, so cross-class `Class|BPCombatManager|GetPlayerPhase` / `GetPlayerInstantsinPhase` getters were used): a `GetPlayerPhase` node, an `Equal(==)` node (B=2), a `GetPlayerInstantsinPhase` node, and a `SelectInt` node (A=InstantsInPhase, B=0, bPickA=Equal), wired to the `ElapsedMovementInstants` pin. `PlayerInstantsInPhase` only counts movement Instants in Phase 2, so the `Phase==2` guard prevents passing startup/recovery Instants as elapsed movement.

**Verified:** C++ rebuilt clean; all 6 `BUH.*` automation suites pass (6/6, 0 failed), including the new `BUH.DashPathColorTests` (A–G) which drive `DrawDashPath` with a live `APlayerController` and check the red/green boundary for every elapsed-Instant count: A) Elapsed=0 → red 0..270/green 270..720, B) Elapsed=1 → red 0..180, C) Elapsed=2 → red 0..90, D) Elapsed=3 (reaction pause) → all green, E) Elapsed=4 → all green, F) 2-Instant path Elapsed=0 → all red, G) 3-Instant path Elapsed=0 → all red. `WBP_CombatUI` compiled and saved; fresh PIE CombatManager state confirmed clean.

**Still requires manual graphical PIE confirmation:** the remaining Dash path turning green at a reaction pause (a visual property `-nullrhi` headless PIE cannot capture). Steps: Play → Move → Dash → click a destination ≥ 4 Instants away → Execute → watch the path; the first 3 Instants stay red, and once the fighter passes the 3rd Instant the remaining path should turn fully green.

**Lesson:** (1) A path renderer that always applies a fixed locked prefix from the path start is wrong during execution, because the path start is the fighter's *current* position, not the original action start — the renderer must be told how many locked Instants have already elapsed. (2) When a C++ static needs a new optional parameter, give it a default value so existing call sites (planning preview) are unaffected, and wire the new value only at the execution call site. (3) `PlayerInstantsInPhase` is phase-relative, so guard it with `Phase==2` before using it as elapsed *movement* Instants. (4) After an editor restart the `AutomationTestToolset` requires `DiscoverTests()` before `ListTests`/`RunTests` — without it every query returns 0 tests even though the module loaded fine.

### 9.26 Live Dash reaction path still red — stale Blueprint countdown, not CurrentState (PARTIAL — real cause was node pruning, see §9.27)

**Problem:** After the §9.25 fix, the user re-tested in graphical PIE and the remaining Dash path was still mostly red at a reaction pause ("Reaction opportunity" shown, Continue legal, player mid-Dash). The §9.25 fix wired the execution `DrawDashPath` to `(PlayerPhase==2) ? PlayerInstantsInPhase : 0`, but that did not change the visible behavior.

**Root cause (confirmed by inspecting the live graph, not assumed):** `BP_CombatManager:EventTick` maintains its **own** `PlayerPhase`/`PlayerInstantsInPhase` countdown, decremented every 0.05s while `PlayerPhase==2`, that is **separate from** the authoritative CombatSim `CurrentState`. That Blueprint countdown drifts from the real simulation (it is a UI-side approximation, not the source of truth), so `WBP_CombatUI:EventTick` was feeding a stale, often-wrong elapsed value into `DrawDashPath`. The fix had to read elapsed movement Instants from the **authoritative `CurrentState`**, not from those Blueprint variables.

**Fix (C++):** Added a read-only helper `UBuhCombatSim::GetDashElapsedMovementInstants(const FBuhCombatState& S)` returning `Fighters[0].PhaseInstants` only when `Fighters[0].Action==8 && Fighters[0].Phase==2` (Dash, Active phase), else `0`. It derives elapsed movement directly from the saved authoritative state — no independent reconstruction from Blueprint variables, no `CurrentState` overwrite.

**Fix (Blueprint, surgical node wiring):** `write_graph_dsl` on the corrected `WBP_CombatUI:EventTick` failed with `Could not connect pin Manager to UpdateContext. The pins may be incompatible types. in: (bind _state (StateMachine|GetState _manager)) in: (event UserInterface|EventTick)` (the known `StateMachine|GetState` pin conflict). The graph stayed intact, so the rewire was done with targeted `create_node`/`connect_pins`/`break_pins`:
- New `GetCurrentState` node (`K2Node_VariableGet_50`) fed from the existing manager getter `K2Node_VariableGet_114`.
- New `GetDashElapsedMovementInstants` node (`K2Node_CallFunction_107`) fed by `GetCurrentState`'s `CurrentState` output.
- Broke the old `SelectInt` (`K2Node_CallFunction_12`) → execution `DrawDashPath` (`K2Node_CallFunction_276`) input 4 link.
- Connected `GetDashElapsedMovementInstants` `ReturnValue` → `DrawDashPath` input 4 (`ElapsedMovementInstants`).
- The planning-preview `DrawDashPath` (`K2Node_CallFunction_243`) keeps its default `ElapsedMovementInstants=0` (first 3 Instants red), unchanged.

**Regression test (real integration, not synthetic):** Added `BUH.DashReactionIntegrationTest` in `PathRendererAutomation.cpp`. It builds a real encounter (`MakeState(-500,500,100,100,0)`), commits a Dash (`Fighters[0].Action=8, Phase=2, PhaseInstants=3, DashTarget=220`), runs `ResolveCombat(S,-1,1,200)` to a genuine reaction pause (asserts `StoppedReason==5` + `bReactionOpportunity`), then asserts `GetDashElapsedMovementInstants` equals the state's `PhaseInstants` (≥3) and that `DrawDashPath` with that value yields a fully green remaining path. A regression guard re-runs the same state with `Elapsed=0` and asserts it yields a red segment — proving the old stale-value bug is caught.

**Verified:** C++ rebuilt clean (LSP include-path noise in `PathRendererAutomation.cpp` is not a real error — UE build is source of truth). Headless editor restarted (PID `243967`), `Lvl_TacticalDuel` opened, `DiscoverTests` run, then: `BUH.DashReactionIntegrationTest` Success; combined `BUH.DashPathColorTests` + `BUH.PathRendererTests` + `BUH.DashReactionIntegrationTest` all Success (3 passed, 0 failed). `WBP_CombatUI` compiled clean and saved.

**Still requires manual graphical PIE confirmation:** the remaining Dash path turning green at a reaction pause is a visual property `-nullrhi` headless PIE cannot capture. Steps: Play → Move → Dash → click a destination ≥ 4 Instants away → Execute → watch the path; the first 3 Instants stay red, and once the fighter passes the 3rd Instant the remaining path should turn fully green.

**Correction (see §9.27):** this §9.26 fix was necessary but **not sufficient**. The user's graphical-PIE logs proved the `GetDashElapsedMovementInstants` node was **pruned** by the compiler (impure `BlueprintCallable` used as a nested data expression with an unconnected exec pin), so its output was read as default `0` regardless of the correct `CurrentState` wiring. The path stayed red because of the pruning, not the stale countdown. The real fix (declare the helper `BlueprintPure`) is in §9.27.

**Lesson:** (1) A UI-side phase/Instant countdown maintained in `EventTick` is **not** the source of truth — derive elapsed movement from the authoritative `CurrentState` via a read-only C++ helper. (2) `write_graph_dsl` still cannot handle `StateMachine|GetState` on an event graph; when the DSL fails, the graph is intact and the rewire can be done surgically with `create_node`/`connect_pins`/`break_pins`. (3) A regression test must drive the **real** resolver to a reaction pause and read the **real** helper, not just feed a synthetic elapsed value — that is what catches a stale-value bug that a direct-value test misses. (4) Keep the planning-preview call on its default `ElapsedMovementInstants=0` so the initial red preview is unaffected.

### 9.27 Proven Blueprint compiler pruning: impure helper used as nested data expression (RESOLVED — cursor instrumentation added)

**Problem:** The user supplied two graphical-PIE logs (`Saved/Logs/BeneathUncountedHeav.log`, `BeneathUncountedHeav_2.log`) that both contained the same `WBP_CombatUI` compiler warning: `GetDashElapsedMovementInstants was pruned because its Exec pin is not connected, the connected value is not available and will instead be read as default`. A second warning reported the same for `GetFinalInstantIndex`. This is the proven root cause of the red remaining Dash path: the helper was pruned, so its output was read as default `0` (not the real elapsed value), making `RemainingLocked=3` and the whole remaining path red. The §9.26 surgical wiring had connected the **data** pins but left the **exec** pin of the impure (`BlueprintCallable`) node disconnected, which is exactly what triggers the prune.

**Root cause (confirmed, not assumed):** `UBuhCombatSim::GetDashElapsedMovementInstants` and `UBuhCombatSim::GetFinalInstantIndex` were declared `UFUNCTION(BlueprintCallable)` (impure — they carry an exec pin) but were wired as **nested data expressions** inside the execution `DrawDashPath` call in `WBP_CombatUI:EventTick` with no exec-pin connection. The Blueprint compiler prunes an impure call whose exec input is unconnected and substitutes the default output value. Both helpers are genuinely read-only (no game-state mutation), so they belong as `BlueprintPure`.

**Fix (C++, approach B from the todo):** Changed both declarations in `CombatSim.h` from `BlueprintCallable` to `BlueprintPure`:
- `GetDashElapsedMovementInstants(const FBuhCombatState& S)` → `BlueprintPure`.
- `GetFinalInstantIndex()` → `BlueprintPure`.
Rebuilt C++ (`Build.sh BeneathUncountedHeavEditor`, Result: Succeeded). Restarted the headless editor to load the new module. No Blueprint node surgery was needed: once the function is pure it has no exec pin, so the existing nested data-expression wiring (`GetCurrentState` → `GetDashElapsedMovementInstants` → `DrawDashPath` arg 4) is valid as-is and the `CurrentState` input is preserved.

**Verified:** After the rebuild + fresh editor load, `WBP_CombatUI` compiles with **zero** pruning warnings (both `GetDashElapsedMovementInstants` and `GetFinalInstantIndex` gone). The asset was saved and a fresh editor load recompiles it cleanly, so the fix persists from disk. All 7 `BUH.*` automation suites pass (0 failed). `GetFinalInstantIndex` is read-only and feeds the timeline prediction range, so pruning it to 0 would have collapsed the prediction to instant 0; making it pure restores the correct final-instant index.

**Cursor instrumentation (§4):** The on-disk `OnClicked(BtnDashMove)` / `OnClicked(BtnWalkMove)` handlers already call `SetInputModeGameAndUI(..., bHideCursorDuringCapture=false)`, so that call should not hide the cursor. To find the real failure point, `LogCursorState(PC, Tag)` was wired surgically (create_node + break_pins + connect_pins) immediately **after** `SetInputModeGameAndUI` in both handlers: Dash node `K2Node_CallFunction_109` (Tag `DashAfterInput`, PC from `GetPlayerController` `K2Node_CallFunction_110`), Walk node `K2Node_CallFunction_111` (Tag `WalkAfterInput`, PC from `K2Node_CallFunction_112`). Each is inserted between `SetInputModeGameAndUI` and `Begin*Targeting` by breaking the old `then→execute` link and reconnecting through the new node. The WBP compiles clean and all 7 BUH suites still pass. The actual cursor-state failure point (and whether `bHideCursorDuringCapture=false` is sufficient or a later point hides the cursor) must be read from the `DashDiag`/`LogCursorState` log lines after a real graphical-PIE Dash and Walk click.

**Lesson:** (1) A `BlueprintCallable` (impure) function used as a nested data expression with its exec pin unconnected is **pruned** by the compiler and its output read as default — the warning is the tell. If the function is genuinely read-only, declare it `BlueprintPure` (no exec pin) instead of forcing an exec connection into a data-expression slot. (2) "Compiles successfully" is not a successful repair if the pruning warning is still present — inspect the compiler warnings, not just the return value. (3) Surgical `create_node`/`break_pins`/`connect_pins` remains the reliable way to insert nodes into an existing exec chain when `write_graph_dsl` cannot express the change.

### 9.28 Confirmed replacement Dash turned green before Execute — pending-decision flag on the manager instance (RESOLVED)

**Problem:** During an interrupted Dash, choosing a new Dash showed the correct red commitment section while the targeting cursor moved. Clicking the ground to confirm the new Dash turned the path green again, before Execute. The previous fix (`55b302c`) had added a global `UBuhCombatSim::GbDashAwaitingExecute` and made `GetDashElapsedMovementInstants` return zero when the flag was set. That put UI selection state inside the resolver and still did not fix the lifecycle.

**Root cause (confirmed from live graph readback, not assumed):** The after-confirm `DrawDashPath` branch in `WBP_CombatUI:EventTick` ran whenever `PlayerMove == 8` and `TargetingAction < 0`. After `SelectDashDestination`, `PlayerMove` is `8` and `TargetingAction` is `-1`, so the branch drew the path using the **old** authoritative `CurrentState`'s elapsed movement instants. The old Dash had already elapsed its 3 locked instants (it was at a reaction pause), so `GetDashElapsedMovementInstants` returned ≥3 and the whole remaining path drew green. The confirmed-but-unexecuted replacement Dash was indistinguishable from an already-executed Dash because nothing recorded that the player had confirmed a new destination but not yet pressed Execute.

**Fix (instance flag, pure resolver):**
- C++: removed `UBuhCombatSim::GbDashAwaitingExecute` and `SetDashAwaitingExecute`. Restored `GetDashElapsedMovementInstants` to the pure calculation `(F.Action == 8 && F.Phase == 2) ? F.PhaseInstants : 0`. No global UI state in the resolver.
- `BP_CombatManager`: added an instance-editable bool `bDashAwaitingExecute` and a `BlueprintPure` function `IsDashAwaitingExecute` that returns it. The flag is written by the lifecycle functions:
  - `BeginDashTargeting` → `false` (fresh targeting).
  - `SelectDashDestination` → `true` (confirmed, awaiting Execute).
  - `ClearDashAwaitingExecute` → `false` (called by the WBP right-click/Escape cancel branch, in addition to `CancelTargeting`).
  - `ExecuteMove` → `false` (the commit now starts a fresh period from the resolved state).
  - `ResetEncounter` → `false`.
- `WBP_CombatUI:EventTick`: the after-confirm `DrawDashPath` (node `K2Node_CallFunction_276`) now takes its `ElapsedMovementInstants` pin from `Math|Integer|SelectInt(0, GetDashElapsedMovementInstants(CurrentState), IsDashAwaitingExecute)`. While awaiting Execute the value is `0` (entire path red); after Execute or for a reaction-pause continuation it is the authoritative elapsed value (green travelled prefix, red locked suffix). The active-targeting `DrawDashPath` (node `K2Node_CallFunction_107`'s sibling in the `TargetingAction >= 0` branch) still leaves the pin unconnected (default `0`), so a new Dash preview is red from the first instant.
- The right-click/Escape cancel branch calls both `CancelTargeting` and `ClearDashAwaitingExecute`, so the flag cannot leak into the next encounter. `CancelTargeting` alone only resets `TargetingAction`.

**Regression test (`BUH.DashLifecycleRegressionTest`, C++-only):** Headless `ProcessEvent` on a Blueprint actor does not reliably land variable writes (§9.23/§9.26), so the test does not drive the Blueprint. It asserts the on-disk `BP_CombatManager` asset exposes `bDashAwaitingExecute` and `IsDashAwaitingExecute`, then mirrors the UI's `SelectInt` rule in a local `EffectiveDashElapsed` helper and drives the real `DrawDashPath`/renderer through every required transition: reaction-pause continuation (all green), replacement targeting (red), confirmed replacement awaiting Execute (still red across simulated UI refreshes), cancel (old green continuation restored), continue (old green progress preserved), short replacement Dash ≤270 uu (entirely red), and Execute (fresh commitment period, new elapsed < 3). A test that merely calls `DrawDashPath(..., 0)` would not catch this bug; this one exercises the pending-decision rule against the authoritative state.

**Blueprint export:** `tests/blueprint_exports/dash_selection_flow.md` contains the actual `read_graph_dsl` readback of every relevant graph (`BeginDashTargeting`, `SelectDashDestination`, `CancelTargeting`, `ClearDashAwaitingExecute`, `IsDashAwaitingExecute`, `ExecuteMove`, `ResetEncounter`, `EventTick`, `BeginPlay`, and the Dash-relevant branches of `WBP_CombatUI:EventGraph`), with the event execution order, variable writes, branches, and the exact `DrawDashPath` argument sources for each context.

**Verified:**
- C++ rebuilt clean (`Build.sh BeneathUncountedHeavEditor`, Result: Succeeded).
- `BP_CombatManager` and `WBP_CombatUI` compiled clean and saved.
- All 9 `BUH.*` automation suites pass, run through `AutomationTestToolset.RunTests` inside a headless MCP editor: `PathRendererTests`, `DashPathColorTests`, `DashReactionIntegrationTest`, `DashGateRegressionTest`, `DashLifecycleRegressionTest`, `CombatSimTests`, `CombatFlowTests`, `WalkPreviewTests`, `CombatManagerIntegrationTests` — 9 passed, 0 failed.
- Graphical PIE visual confirmation of the red-after-confirm behavior is still user-driven (`-nullrhi` headless PIE cannot click UMG buttons or capture the path color). The C++ lifecycle test proves the rule the UI now implements; the on-disk graph readback proves the UI implements that rule.

**Environment gotcha (test harness, not game code):** Command-line `UnrealEditor-Cmd -ExecCmds=Automation RunTests ...;Quit` runs are flaky on this machine. The forked `UnrealTraceServer` sometimes terminates with "no sponsors or connections active" and sends `signal 15` to the editor before the test starts, killing the run with no `Test Started` line. `-trace=default~`, `-trace=`, and `-NoTrace` did not stop the fork. The reliable path is to launch a headless MCP editor (`-ModelContextProtocolStartServer`) and run the same tests through `AutomationTestToolset.RunTests`, which does not fork the trace server. Also: a second headless editor left running from an MCP session can interfere with a separate command-line automation run; kill it first (`kill -9 $(cat /tmp/mcp_editor_pid.txt)`).

**Lesson:** (1) When a confirmed-but-unexecuted decision must render differently from an executed one, record the pending decision on the **owning actor instance** (`BP_CombatManager.bDashAwaitingExecute`) and read it through a `BlueprintPure` accessor — do not push UI selection state into a global resolver flag, and do not treat `PlayerMove == 8` as "new Dash" because an existing Dash also has action 8. (2) The renderer's elapsed argument must be `SelectInt(0, authoritative_elapsed, is_awaiting_execute)` at the after-confirm call site: `0` while awaiting, authoritative value otherwise. (3) A lifecycle regression test that cannot drive Blueprint variables headless should assert the on-disk asset exposes the flag/accessor and then mirror the exact UI rule in C++ against the real renderer — that still catches the bug without relying on unreliable `ProcessEvent` variable writes. (4) For flaky command-line automation on this box, run tests through the MCP `AutomationTestToolset` instead of `-ExecCmds`.

### 9.29 Jump implemented as the third playable movement action (RESOLVED)

**Problem:** The tactical-duel prototype had Walk and Dash as playable movement actions, but Jump was only a curved preview line with no real simulation, reaction window, committed parabola, or UI button.

**Cause:** `FBuhFighterState` had no Jump fields, `UBuhCombatSim` had no Jump movement/reaction/commit logic, `BP_CombatManager` had no Jump lifecycle functions, and `WBP_CombatUI` had no Jump button or targeting branch.

**Fix (C++):**
- Extended `FBuhFighterState` with `JumpTarget`, `JumpTargetY`, `JumpHeight`, and Jump snapshot fields without breaking serialization.
- Added Jump `MoveData[9] = { 1, 8, 3, 0, 9999, 0, 9999 }` (startup 1, flight 8, recovery 3).
- Added `PrepareJumpCommit`, Jump movement in `StepCombatInstant`, Jump commit in `ResolveCombat` and `GenerateTimeline`, snapshot height fills, and `MoveGhostsToInstant` using `Z = 90.0f + Height`.
- Added `DrawJumpPath` and `GetJumpElapsedFlightInstants` (declared `BlueprintPure`).
- Added `BUH.JumpSimulationTests` and `BUH.JumpPathColorTests` to `PathRendererAutomation.cpp`.

**Fix (Blueprint):**
- Added `JumpAwaitingExecute` bool, `BeginJumpTargeting`, `SelectJumpDestination`, `ClearJumpAwaitingExecute`, and `IsJumpAwaitingExecute` to `BP_CombatManager`.
- Added `BtnJumpMove` (Button) and `TxtJumpMove` (TextBlock) to `WBP_CombatUI`'s Move submenu via `UMGToolSet.AddWidget`, bound `OnClicked(BtnJumpMove)` via `UMGToolSet.BindToEventProperty`, and set text/color via `ObjectTools.set_properties`.
- Rewrote `WBP_CombatUI:EventTick` to add the Jump targeting preview (`TargetingAction == 9` → `DrawJumpPath`), confirmation (`SelectJumpDestination`), cancel (`ClearJumpAwaitingExecute`), and confirmed-Jump execution branch (`PlayerMove == 9` → `DrawJumpPath` with `SelectInt(0, GetJumpElapsedFlightInstants, IsJumpAwaitingExecute)`).
- Updated `DashGateRegressionTest` to check the new `IsDashAwaitingExecute` gate structure instead of the old CompareInt node names, because the full EventTick rewrite replaced that structure.

**Verified:**
- C++ rebuilt clean (`RunUBT.sh -project="..." BeneathUncountedHeavEditor Development Linux`, Result: Succeeded).
- All 12 `BUH.*` automation suites pass, including `BUH.JumpSimulationTests`, `BUH.JumpPathColorTests`, and the updated `BUH.DashGateRegressionTest`.
- `BP_CombatManager` and `WBP_CombatUI` compiled clean and saved.
- Blueprint export: `tests/blueprint_exports/jump_selection_flow.md` contains the actual `read_graph_dsl` readback of every Jump graph and the EventTick Jump branches.

**Lesson:** (1) A movement action that is only a preview line is not a real action — it needs authoritative simulation, reaction legality, commit/clamp logic, snapshots, ghosts, and a path renderer. (2) The same `SelectInt(0, authoritative_elapsed, is_awaiting_execute)` rule that fixed Dash also applies to Jump: the confirmed-but-unexecuted path draws fully red, and the executed path shows the real red/green split. (3) When a full EventTick rewrite replaces old node names, regression tests that check those names must be updated to check the new structure, not deleted. (4) `write_graph_dsl` can still fail on `StateMachine|GetState` / `CustomizableObjectInstance|GetCurrentState` nodes in an event graph; the reliable path is targeted node-level edits or a full rewrite that avoids those type IDs.

### 9.30 Jump destination not applied during execution + Jump cursor disappeared (RESOLVED)

**Problem 1 (Jump destination ignored):** Clicking Jump, clicking a ground destination, then Execute caused the fighter to stay in place instead of traveling to the chosen landing point. The preview parabola and the committed red parabola rendered correctly, but the simulation never applied the destination.

**Root cause (confirmed from live graph readback, not assumed):** `BP_CombatManager.ExecuteMove` called `CombatSim|ExecuteMove`, a `BlueprintCallable` UFUNCTION whose body only called `ResolveCombat` and never read the `GExecJumpTarget` / `GExecJumpTargetY` globals that `SelectJumpDestination` writes. The fully implemented `CombatSim|ExecuteMoveWithVars` helper, which already applied the Jump destination when `PlayerMove == 9`, was never called from the Blueprint.

**Fix (Blueprint):** `BP_CombatManager.ExecuteMove` now calls `CombatSim|ExecuteMoveWithVars` with the live `PlayerWalkTarget` / `PlayerWalkTargetY` values and the current `PlayerY`. After execution it clears the stale exec targets with `CombatSim|SetExecDashTarget -9999 0` and `CombatSim|SetExecJumpTarget -9999 0`, and clears `JumpAwaitingExecute`.

**Problem 2 (cursor disappeared during Jump targeting):** Clicking the Jump button switched input mode to GameAndUI but the mouse cursor vanished, so the player could not see where they were clicking to set the destination.

**Root cause:** `OnClicked(BtnJumpMove)` called `SetInputModeGameAndUI` but never called `SetShowMouseCursor true`. The Walk and Dash handlers had the same pattern but had been fixed earlier; the Jump handler was written without the explicit cursor-show call.

**Fix (Blueprint):** `OnClicked(BtnJumpMove)` now explicitly calls `(|SetbShowMouseCursor true _returnvalue)` immediately after `SetInputModeGameAndUI`, then logs cursor state via `CombatSim|LogCursorState`.

**Verified:**
- C++ rebuilt clean (`Build.sh BeneathUncountedHeavEditor`, Result: Succeeded).
- All 12 `BUH.*` automation suites pass, including `BUH.JumpSimulationTests` and `BUH.CombatManagerIntegrationTests`.
- `BP_CombatManager` and `WBP_CombatUI` compiled clean and saved.
- PIE log inspection shows no errors and the WBP constructs successfully.
- Corrected Blueprint graphs exported to `tests/blueprint_exports/jump_selection_flow.md`.

**Status (updated by §9.31):** the destination-execution half is now proven by `BUH.JumpPlaybackIntegrationTests`, which drives the real `BeginJumpPlayback`/`TickJumpPlayback` path and confirms the fighter travels to the chosen landing point. The mouse-cursor visibility during Jump targeting still requires manual graphical PIE confirmation because headless `-nullrhi` PIE cannot click UMG buttons or capture the cursor.

**Lesson:** (1) A `BlueprintCallable` UFUNCTION that only calls `ResolveCombat` is not a complete execution helper — if the Blueprint needs to pass live variables (walk target, jump target), it must call the `ExecuteMoveWithVars` overload that reads them. (2) Every targeting button handler that switches to GameAndUI input mode must explicitly call `SetShowMouseCursor true`; the input-mode call alone does not guarantee the cursor is visible. (3) Stale exec targets must be cleared after every execution so a later Walk/Dash/Jump does not accidentally reuse an old destination.

### 9.31 Airborne Jump movement and interrupted-Jump path persistence (RESOLVED)

**Problem:** Jump execution moved the fighter horizontally to the landing point but never changed its Z coordinate, so the fighter did not visibly travel through the air. An interrupted Jump also lost its committed parabola: the path disappeared at the reaction pause and Continue did not restore it. The old integration test for an interrupted Jump used a fresh grounded state, so it never actually exercised an airborne reaction pause.

**Cause:**
- `ExecuteMove` teleported the player actor directly to `FinalState.PlayerX` / `FinalState.PlayerY` at `Z=90` for every action, including Jump, and used the raw `FinalState.OpponentX` for the opponent instead of the authoritative world-Z helper.
- `GenerateTimeline` built reaction-pause and KO snapshots without `PlayerHeight` / `OpponentHeight`, so any consumer of a snapshot read height `0` even when the fighter was airborne.
- `TickJumpPlayback` called `DrawJumpPathFromState` on the stored pre-execution state. For a Jump that state still has `Action=-1`, so the path drawer immediately cleared the path instead of reconstructing the parabola.
- The interrupted-Jump regression test started from a fresh grounded state and expected a reaction pause, but `CheckReactionPause` only triggers when an attack starts and the defender is already reactable. A fresh Jump is not reactable until `PhaseInstants>=3`, while the opponent Heavy Strike starts at instant 0, so no pause occurs.

**Fix (C++):**
- `GenerateTimeline` now fills `PlayerHeight` and `OpponentHeight` on reaction-pause and KO snapshots.
- `TickJumpPlayback` builds a corrected `DrawState` copy of the pre-execution state, overriding the player's `Action=9`, `Phase=2`, `PhaseInstants`, `X`, `Y`, and `Height`, and passes that to `DrawJumpPathFromState`. The parabola is reconstructed from the original `JumpStartX/Y` and `JumpTarget/JumpTargetY`, so the path persists through a reaction pause and is regenerated on Continue.
- `BUH.JumpPlaybackIntegrationTests` now drives an airborne interrupted-Jump prestate: `MakeStateWithPhases(-500, 500, 100, 100, 0, 2, 3, 0, 0)` with `Action=9`, `JumpStartX=-500`, `JumpTarget=-20`, `X=-320`, `Height=206.25`, then `ResolveCombat(InterruptPre, 7, 1, 200)` and `BeginJumpPlayback(..., 7, 1, ...)`. It asserts the fighter is visibly airborne at the reaction pause, that the path is reconstructed after the pause, and that Continue resumes the parabola.
- Test actors now have a registered `USceneComponent` root so `SetActorLocation` / `GetActorLocation` work.

**Fix (Blueprint):**
- Added a `PreExecutionState` struct variable of type `BuhCombatState` to `BP_CombatManager`.
- Rewrote `ExecuteMove` to bind `PreExecutionState`, apply `WithPlayerJumpTargetFromPreview` when `PlayerMove == 9`, call `ExecuteMoveWithVars` with `PreExecutionState`, skip the player `SetActorLocation` for Jump/airborne states, use `GetFinalOpponentWorldZ` for the opponent, and call `BeginJumpPlayback` when the player committed a Jump.
- Wired `WBP_CombatUI.EventTick` to call `TickJumpPlayback` with the tick delta and branch on its result.

**Verified:**
- C++ rebuilt clean (`RunUBT.sh BeneathUncountedHeavEditor Linux Development`, Result: Succeeded).
- `BUH.JumpPlaybackIntegrationTests` passes, proving the fighter travels along the parabola, stays airborne at a reaction pause, and resumes the path after Continue.
- All 13 `BUH.*` automation suites pass via `AutomationTestToolset.RunTests` with the explicit test names.
- `BP_CombatManager` and `WBP_CombatUI` compiled clean and saved.
- Corrected `ExecuteMove` and `EventTick` graphs exported to `tests/blueprint_exports/jump_selection_flow.md`.

**Environment gotcha:** `RunTestsByFilter` with the broad `"BUH."` filter closed the MCP socket on this machine. The reliable path is `AutomationTestToolset.RunTests` with the explicit array of 13 `BUH.*` test names.

**Lesson:** (1) A movement action that teleports the actor to its final position is not a visible movement — it needs a deterministic per-Instant playback that applies the authoritative height every tick. (2) Snapshots must carry every field a consumer needs; a reaction-pause snapshot without `PlayerHeight` silently reports a grounded fighter. (3) When a path drawer receives a state that no longer represents the active movement (e.g. a pre-execution state with `Action=-1`), build a corrected state that restores the movement fields before drawing. (4) A reaction-pause regression test must start from a state where the defender is already reactable; a fresh Jump is not reactable until `PhaseInstants>=3`, so a fresh-state test will never trigger the pause. (5) `RunTestsByFilter` with a broad prefix can close the MCP socket; run the explicit test-name array instead.

### 9.32 WBP AddToViewport used failed cast result; MCP asset registry out of sync (SUPERSEDED by §9.33/§9.34 — cast eliminated via SetWidgetManager, duplicate tick removed)

**Problem:** After a `write_graph_dsl` call accidentally wiped the WBP EventTick body, the WBP was restored from git but its EventTick never fired in PIE. `BP_CombatManager.UI` was `None`, the WBP `manager` variable was never set, and the Execute button / keyboard shortcuts could not trigger `ExecuteMove`. Neither headless nor graphical PIE logs showed any `BeginJumpPlayback` — the Jump never started.

**Cause:**
- `BP_CombatManager:EventBeginPlay` called `AddToViewport` on `_aswbp_combat_ui` (the `CastToWBP_CombatUI` result) instead of `_returnvalue_1` (the `CreateWidget` result). When the cast failed, `AddToViewport None` was a no-op — the widget was constructed but never added to the viewport, so its EventTick never ran.
- `CastToWBP_CombatUI` fails because the MCP asset registry does not track `WBP_CombatUI`. `AssetTools.exists` returns `false` for the asset even though the file exists on disk (4.1 MB). `save_assets` and `get_asset_class` also fail. Restarting the editor fixed `BP_CombatManager` but not `WBP_CombatUI`.
- The "working Jump animation" referenced in the prior todo was from a session before the WBP EventTick was wiped; it is not a current-state observation.

**Fix applied:**
- Changed `AddToViewport` to use `_returnvalue_1` (original `CreateWidget` result) so the widget is in the viewport and its EventTick fires even when the cast fails.
- Changed `SetInputModeUIOnly` to `SetInputModeGameAndUI` so the player can interact with the game world while the UI is active.
- Updated `tests/blueprint_exports/jump_selection_flow.md` to match the live `ExecuteMove` graph (the export was stale).

**Still open (now resolved — see §9.33/§9.34):**
- The cast failure was eliminated in §9.33 by `SetWidgetManager` (C++ `FObjectProperty::SetObjectPropertyValue`), so the WBP `manager` property is set without a cast and the UI can reach `ExecuteMove`.
- The redundant `HandleTargetingTick` debug handler was removed from the manager EventTick in §9.34; the WBP mouse-confirmation is the single targeting path.
- The MCP asset registry quirk for `WBP_CombatUI` (`exists`/`get_asset_class` fail while the file is on disk) persists but no longer blocks the workflow, since `SetWidgetManager` bypasses the cast and `save_assets` succeeds.

**Lesson:** (1) Always use the original `CreateWidget` result for `AddToViewport`, not the cast result — a failed cast silently drops the widget from the viewport. (2) The MCP asset registry can lose track of assets that exist on disk; `exists` / `save_assets` / `get_asset_class` may all fail while `find_assets` / `load_asset` / `compile_blueprint` still work. Restarting the editor may fix some assets but not others. (3) A "working" observation from a prior session is not evidence of current behavior — always verify with fresh PIE logs.

### 9.33 WBP cast eliminated via SetWidgetManager; reaction-pause freeze fixed by manager-driven TickJumpPlayback (VERIFIED)

**Problem:** (1) `CastToWBP_CombatUI` still failed, so the WBP `manager` property stayed `None` and the UI could not reach `ExecuteMove`. (2) A Jump interrupted by an opponent reaction appeared to "fall" / snap to ground Z=90 instead of freezing at the airborne height.

**Cause:**
- The WBP `manager` property could not be set through the Blueprint cast (which failed) or through a Blueprint `SetManager` function (not a C++ `UFUNCTION`). Reflective writes that compiled crashed: `UUserWidget::SetProperty` (no such member), `FProperty::SetPropertyValue_InContainer` (no such member). The only reflective write that both compiles and runs without a `Signal 11` crash is `FObjectProperty::SetObjectPropertyValue(ContainerPtrToValuePtr<UObject*>(Widget), ManagerObj)`.
- The "fall" was not a physics bug. `TickJumpPlayback` was never being called in the real flow — the WBP EventTick was not reliably driving it — so the actor stayed at ground Z=90 while the playback statics believed it was airborne. The fighter never actually moved along the parabola.

**Fix applied:**
- Added `UBuhCombatSim::SetWidgetManager(UUserWidget* Widget, UObject* Manager)` that finds the WBP `Manager` property, checks it is an `FObjectProperty`, and calls `ObjProp->SetObjectPropertyValue(ObjProp->ContainerPtrToValuePtr<UObject*>(Widget), ManagerObj)`. `BP_CombatManager:EventBeginPlay` now calls `CombatSim|SetWidgetManager _returnvalue_1 _self` and drops the `CastToWBP_CombatUI` entirely; `CreateWidget`'s result is used directly for `SetUI`, `SetWidgetManager`, `AddToViewport`, and `SetInputModeGameAndUI`.
- Added `UBuhCombatSim::TriggerReactionJump(AActor* Context)` (diagnosis scaffolding) and a `BUH.TriggerReactionJump` console command that finds the `PlayerFighter` tagged actor and passes it as context, so the world is obtained from `Context->GetWorld()` (not `GEngine`, which returns null from Blueprint BeginPlay). It builds the reaction prestate (opponent mid-Heavy `Phase=3 Action=1`, player grounded), resolves with `PlayerMove=9, OpponentMove=1`, and calls the real `BeginJumpPlayback`.
- Moved per-frame `TickJumpPlayback` into `BP_CombatManager:EventTick` (`CombatSim|TickJumpPlayback DeltaSeconds`), which fires reliably. `TickJumpPlayback` no-ops safely when no playback is active.

**Verified (mcp_author33.log, real PIE flow):**
- `SetWidgetManager: Manager prop type=ObjectProperty name=Manager`, `PropertyClass=.../BP_CombatManager.BP_CombatManager_C`, `DONE (SetObjectPropertyValue)`; `WBP_CombatUI CONSTRUCTED`; no `Signal 11`.
- `TriggerReactionJump RESOLVE: reaction=1 ... finalHeight=206.25`; `BeginJumpPlayback called active=1`.
- Timeline ascends: inst=0 Z=90, inst=1 Z=186.2, inst=2 Z=255.0, inst=3 Z=296.2.
- At inst=4 the reaction triggers (`reaction=1`), freezing the fighter at `playerZ=296.2` (= 90 + 206.25), XY frozen at -320; `JumpPlayback STOP: index=5 count=5 reaction=1`; the fighter then holds `PZ=296.25 active=0` every subsequent frame. No fall, no snap to ground.

**Still open:** Headless PIE cannot click the WBP buttons (Slate snapshot shows no widget buttons under `-nullrhi`), so the full UI-driven Jump (click Jump → click destination → click Execute) needs a graphical PIE run to confirm a real button click reaches `ExecuteMove` and starts playback. The C++ playback path and the manager-driven tick are verified.

**Lesson:** (1) To set a Blueprint object-reference property from C++ without a crash, use `FObjectProperty::SetObjectPropertyValue` on the `ContainerPtrToValuePtr` slot — `UUserWidget::SetProperty` and `FProperty::SetPropertyValue_InContainer` do not exist and crash. (2) `GEngine->GetWorldFromContextObject(GEngine, ...)` returns null when called from Blueprint BeginPlay; pass the actor as an `AActor* Context` and use `Context->GetWorld()`. (3) A "falling" fighter that never moved was a missing playback driver, not a physics error — drive `TickJumpPlayback` from a tick that actually fires (the manager EventTick), and confirm with `JumpPlayback TICK` logs before blaming gravity. (4) `finalHeight` in the resolve log is height above ground; the actor Z is `90 + Height`.

### 9.34 Jump playback ownership and airborne path: sole driver, exclusive path rendering, no duplicate mouse input (VERIFIED headless)

**Problem:** After the WBP was restored to the viewport (§9.33), the saved graphs showed BOTH `BP_CombatManager.EventTick → TickJumpPlayback(DeltaSeconds)` AND `WBP_CombatUI.EventTick → TickJumpPlayback(InDeltaTime)`. With the widget running again, the same static playback timeline advanced twice per frame. Separately, the WBP drew the Jump preview whenever `PlayerMove == 9` (which stays 9 after Execute), overwriting the C++ playback's authoritative remaining path — the "straight green line offscreen" the user saw. And the manager EventTick's `HandleTargetingTick` duplicated the WBP's own mouse-confirmation, so the same ground click could be processed twice.

**Cause:**
- The WBP EventTick had been given a `TickJumpPlayback` call as a stopgap when the manager tick was not reliably driving playback; once the manager EventTick became the reliable driver, the WBP call became a duplicate.
- The WBP's confirmed-Jump preview was gated only on `PlayerMove == 9 && JumpAwaitingExecute`; `PlayerMove` is not reset to -1 after Execute, and the targeting block was gated only on `TargetingAction >= 0`, so stale preview drawing could persist into playback.
- `HandleTargetingTick` was a C++ debug-input workaround added when the WBP EventTick was not firing; after the cast fix made the WBP tick fire, it became a redundant second mouse handler.

**Fix applied:**
- Removed `(CombatSim|TickJumpPlayback InDeltaTime)` from `WBP_CombatUI.EventTick`. `BP_CombatManager.EventTick` is now the sole playback driver (one `TickJumpPlayback(DeltaSeconds)` per frame).
- Gated every WBP movement-path draw (targeting parabola, Walk ground line, confirmed Jump preview, confirmed Dash preview) on `State == 0` (Planning) via `(and _returnvalue_1 ...)` where `_returnvalue_1` is `(== _state _bzero)`. While playback owns the path (State 1) or a reaction is pending (State 3), the WBP draws no movement path; the C++ `TickJumpPlayback`/`DrawJumpPathFromState` is the sole path owner.
- Removed `(CombatSim|HandleTargetingTick ...)` from `BP_CombatManager.EventTick`. The WBP EventTick mouse-confirmation (`WasInputKeyJustPressed LeftMouseButton` → `SelectJumpDestination`/`SelectDashDestination`/`SelectWalkDestination`) is the single authoritative targeting-confirmation path.
- DSL note: the manager `State`/`CurrentState` variable getters must be written as `Class|BPCombatManager|GetState` / `Class|BPCombatManager|GetCurrentState`. The `StateMachine|GetState` prefix is now ambiguous (it matches a state-machine `GetState` with an `UpdateContext` pin) and `Variables|BPCombatManager|GetState` does not exist; both fail to compile.

**Verified (headless PIE, mcp_author33.log + read-back graphs):**
- Read-back: WBP EventTick has no `TickJumpPlayback`; all path draws gated on `State==0`. Manager EventTick has exactly one `TickJumpPlayback` and no `HandleTargetingTick`.
- `BUH.JumpGroundedReactionTest` passes: fighter ascends 90 → 186.2 → 255.0 → 296.2, freezes at `Z=296.25` at the reaction boundary (`reaction=1`, `finalHeight=206.25`), holds `active=0` with no fall through 5 post-reaction ticks.
- Manager EventTick fires every frame (`FighterZ tick` logs), confirming it is the live playback driver.

**Still open:** Headless PIE cannot click the WBP buttons (Slate shows no widget buttons under `-nullrhi`) and MCP has no console-command runner, so the real UI-driven Jump (click Jump → click destination → click Execute → `ExecuteMove` → `BeginJumpPlayback`) needs a graphical PIE run to confirm the manager State changes to Reaction, Continue appears, and the fighter stays airborne with the correct remaining arc.

**Lesson:** (1) When two ticks can drive the same static playback, make exactly one owner the driver and remove the other — a stopgap call left in a second tick becomes a double-advance the moment both ticks fire. (2) Gate presentation-layer path drawing on the manager's authoritative `State`, not on a move int that is not reset after Execute; `PlayerMove == 9` alone is not a safe "draw the preview" condition. (3) A debug input handler added to work around a dead tick must be removed once the real UI tick fires, or the same click gets processed twice. (4) Blueprint variable getters for a class are addressed as `Class|<ClassName>|Get<Var>` in the DSL; category-style prefixes like `StateMachine|GetState` are ambiguous and can bind to the wrong function.

### 9.35 Jump playback received the unadjusted prestate (JumpTarget -9999) → Jump in place (RESOLVED)

**Problem:** After the §9.34 ownership fix, the user re-tested in graphical PIE: the blue fighter became airborne but the remaining trajectory was wrong. The resolver used the real Jump destination, but the visual playback jumped in place.

**Cause:** The saved `BP_CombatManager.ExecuteMove` graph built `PreExecutionState = WithPlayerJumpTargetFromPreview(_returnvalue)` (JumpTarget set from `GPlayerJumpTarget`) and resolved via `ExecuteMoveWithVars` (which applies `GExecJumpTarget`), but the final `BeginJumpPlayback` call passed the ORIGINAL `_returnvalue` (JumpTarget = -9999) as its `PreState`. `BeginJumpPlayback` (CombatSim.cpp:3315) calls `GenerateTimeline(PreState, ...)` and stores `GJumpPlaybackPreState = PreState`; `PrepareJumpCommit` (CombatSim.cpp:369) substitutes the takeoff XY when `JumpTarget < -9998.f`, so the playback jumped in place. `DrawJumpPathFromState` read the same wrong `GJumpPlaybackPreState`, producing the incorrect trajectory. The resolver and playback started from different states.

**Fix applied:** Smallest targeted edit to `BP_CombatManager.ExecuteMove` — the final `BeginJumpPlayback` call now passes `(Variables|Default|GetPreExecutionState)` instead of `_returnvalue` as the `PreState` argument. No second targeting state; `ExecuteMoveWithVars` and `WithPlayerJumpTargetFromPreview` both apply the same destination (`GExecJumpTarget == GPlayerJumpTarget`, set together by `SelectJumpDestination`). Jump's deterministic XY and parabolic height are preserved.

**Regression test (`BUH.JumpTargetHandoffTest`, C++):** Player on the right (X=500), opponent mid-Heavy (Phase=3), Jump landing point to the left and offset on Y (-20, 120). Exercises the manager's exact functions (SetPreviewJumpTarget/SetExecJumpTarget, MakeStateWithPhases, WithPlayerJumpTargetFromPreview, ExecuteMoveWithVars, BeginJumpPlayback with the committed prestate). Asserts the playback prestate contains the target (not -9999), the fighter changes XY during flight, the snapshots agree with the resolver, the path ends at the clamped destination, and the reaction freezes at the correct XYZ. A control timeline generated from the ORIGINAL state (JumpTarget=-9999) jumps in place (movedXY=0), proving the unfixed path is caught. A test that manually passes an already-correct JumpTarget to BeginJumpPlayback would not catch this bug.

**Verified (headless PIE, mcp_author34.log):**
- Read-back: `BeginJumpPlayback` now uses `(Variables|Default|GetPreExecutionState)`.
- `BUH.JumpTargetHandoffTest` passes: resolver `reaction=1 finalX=324.6 finalY=40.5 finalHeight=206.25`; playback timeline takeoff (500,0) → landing (324.6,40.5), reaction instant 4, height 206.25; real fighter actor moved to (324.6,40.5) and froze at Z=296.25 (= 90 + 206.25). Control (original state) movedXY=0 (Jump in place).
- All 5 Jump tests pass: `BUH.JumpSimulationTests`, `BUH.JumpPathColorTests`, `BUH.JumpPlaybackIntegrationTests`, `BUH.JumpGroundedReactionTest`, `BUH.JumpTargetHandoffTest` (passed=5, failed=0).

**Still open:** The full UI-driven graphical-PIE click-through (click Jump → click destination → click Execute → `ExecuteMove` → `BeginJumpPlayback` → State=Reaction → Continue) still needs a graphical PIE run, because headless `-nullrhi` PIE cannot click WBP buttons. The state-handoff bug itself is proven fixed headless.

**Lesson:** (1) When a resolver and a visual playback both derive from a "pre-execution state", pass the SAME committed state to both — if the playback receives the unadjusted original state (target = -9999), `PrepareJumpCommit` silently substitutes the takeoff and the fighter jumps in place even though the resolver used the real destination. (2) A regression test must replicate the manager's exact state-building sequence (original state → WithPlayerJumpTargetFromPreview → BeginJumpPlayback with the committed prestate) and include a control that proves the unfixed path fails; a test that hands BeginJumpPlayback an already-correct target cannot catch a handoff bug. (3) `ExecuteMoveWithVars` applies `GExecJumpTarget` and `WithPlayerJumpTargetFromPreview` applies `GPlayerJumpTarget`; because `SelectJumpDestination` sets both to the same value, the resolver and the committed prestate agree — the only way they diverge is if the playback is given a different state object.

### 9.36 Missed midair reaction-pause for Jump vs a simultaneously-committed Heavy (RESOLVED)

**Problem:** In graphical PIE the user committed Jump against an opponent that committed Heavy on the same decision. The blue fighter flew the full parabola and landed without ever pausing; the HUD stayed "Select a move / Ready" instead of "Reaction opportunity". The specially constructed `BUH.JumpGroundedReactionTest` passed, but the ordinary fresh-encounter case failed.

**Cause:** `CheckReactionPause` (CombatSim.cpp) gated a threat on `AttackStart[Threat] != Threat`. But `AttackStart` is reset to -1 at the start of every `StepCombatInstant` (CombatSim.cpp:196-197) and only reflects the *current* instant's commits (`PendingAttackStart`). A Heavy committed at instant 0 therefore has `AttackStart[1] == -1` by instant 4, exactly when the Jump becomes reactable (`Action==9 && Phase==2 && PhaseInstants>=3`). The threat was invisible at the first legal opportunity, so the reaction was lost and the Jump ran to landing.

**Fix applied:** Refined the `CheckReactionPause` gate (smallest targeted edit, no new state machine). A threat is now eligible if it is a *committed* attack still in Phase 1 startup (`bThreatObservable = (Action==0||Action==1) && Phase==1 && bCommitted`) OR it started this instant (`AttackStart[Threat]==Threat`). A "later-reactable" pause (threat observable but not started this instant) additionally requires the *defender* to be committed this encounter (`Def.bCommitted`). That last clause is what keeps an already-moving fighter that was locked when the threat started (the `DA7` dash, `bCommitted==false`) from getting a stale pause, while a freshly-committed Jump (`bCommitted==true`) still reacts. `bCommitted` is set by both the decision commit loop (CombatSim.cpp:451,461) and the AI auto-commit (CombatSim.cpp:571), so it reliably marks a real commit; a directly-set `Action` in a test setup leaves it false.

**Reconciliation of existing Jump tests:** With the broader gate, `BUH.JumpSimulationTests` (Vertical Jump) and the playback trajectory cases needed updating to match the contract in `todo.md`. Vertical Jump now *expects* a reaction (`StoppedReason==5 && bReactionOpportunity && Height>50`). The Clamp, Short Jump, and Completed-Jump playback cases now place the opponent in recovery (`MakeStateWithPhases(..., 0, 0, 0, 3, 0)`, opponent Phase 3 / PI 0) so no qualifying threat exists and the Jump completes, preserving their trajectory assertions. The opponent becomes ready at instant 8, exactly when the Jump lands, so no reaction fires.

**Regression test (`BUH.JumpFreshEncounterReactionTest`, C++):** A fresh grounded encounter (both ready, no artificial recovery) where the player commits Jump (9) and the opponent commits Heavy (1) on the same decision, resolved with reaction pauses enabled. Asserts the reaction fires at the first legal instant (instant 4), `StoppedReason==5`, `ReactionFighter==0`, the player is still airborne in the active flight phase, and the height is ~206.25 uu at the three-flight-instant boundary. The timeline also stops at the same reaction state. This is the test that reproduces the real gameplay the user saw fail.

**Verified (headless PIE, mcp_author_reactfix3.log):**
- `BUH.JumpFreshEncounterReactionTest` passes: `reaction=1 stoppedReason=5 finalHeight=206.25`, timeline `reactionInstant=4 reactionHeight=206.25`.
- All 16 BUH automation suites pass (passed=16, failed=0), including the new regression and the reconciled `BUH.JumpSimulationTests` / `BUH.JumpPlaybackIntegrationTests`.
- `Saved/CombatTestReport.txt`: 69 passed, 0 failed. The three checks that the first (over-broad) gate broke — `W1 walking-toward-moves-fighter`, `W2 walking-away-moves-fighter`, `DA7 dash-locked-window-no-pause` — all pass again.

**Lesson:** (1) `AttackStart` is per-instant; a threat committed on an earlier instant is invisible to a later `CheckReactionPause` via `AttackStart`, so a reaction gate must look at the threat's *observable* state (committed + Phase 1 startup), not just the start instant. (2) To avoid stale pauses, distinguish a freshly-committed movement (`bCommitted==true`) from an already-moving one (`bCommitted==false`): a "later-reactable" pause requires the defender to have committed this encounter, so a fighter that was locked when the threat started does not react to it. (3) When broadening a reaction gate, re-run the *whole* BUH suite — the first fix passed the Jump regression but silently broke the walking and dash-locked-window tests, which only the full run surfaced.

### 9.37 Jump reaction Continue button missing + remaining trajectory changed shape (RESOLVED)

**Problem:** In graphical PIE the user committed Jump against a Heavy. The fighter became airborne, the opponent's Heavy triggered a legal midair reaction, and the HUD showed "Reaction opportunity" with the fighter frozen. But (1) the Continue button never appeared, and (2) the remaining Jump path changed shape at the pause instead of continuing the original parabola.

**Cause:**
- **Continue button:** `WBP_CombatUI.RefreshUI` shows `BtnContinueWalk` when `GetReactionInfo` returns `bIsReaction` and `LegalActions` contains 7. `GetReactionInfo` requires `bReactionOpportunity==true`, `ReactionFighter==0`, and `CurrentState.Fighters.Num()>=2`, and it reads `CurrentState.Fighters[0].Action` to decide whether Continue is legal. The old `ExecuteMove` graph never wrote the resolver's final (airborne) state to `CurrentState`, never set `bReactionOpportunity`, and never set `bHasCurrentState`. So `GetReactionInfo` saw a grounded/empty `CurrentState` and returned `bIsReaction=false`, hiding the button.
- **Trajectory shape:** `TickJumpPlayback` built its `DrawState` from `GJumpPlaybackPreState`, whose `JumpStartX/Y` were still 0 (the pre-commit default, because `MakeState` does not set `JumpStartX/Y` and `PrepareJumpCommit` only runs during resolver stepping). The timeline snapshots came from the resolver's committed state (correct `JumpStart`), but the drawn remaining path used the prestate's `JumpStart=0`, so the remaining parabola was drawn from the wrong start point and visibly changed shape.

**Fix applied:**
- **C++ (`CombatSim.h`/`CombatSim.cpp`):** Added static floats `GJumpPlaybackJumpStartX/Y` and `GJumpPlaybackJumpTargetX/Y`. `BeginJumpPlayback` sets them from `Result.FinalState.Fighters[0]` (the committed `JumpStartX/Y` and `JumpTarget/Y`); `TickJumpPlayback` overrides `DrawState.Fighters[0].JumpStartX/Y` and `JumpTarget/Y` with those committed values; `StopJumpPlayback` resets them to 0. The remaining path now always uses the resolver's committed takeoff and destination.
- **Blueprint (`BP_CombatManager.ExecuteMove`):** Added a `CurrentState`/`bHasCurrentState`/`bReactionOpportunity` handoff. When `PlayerMove != 7` (not Continue), the pre-resolve state is stored in `CurrentState`. The resolve call is `ExecuteMoveWithVars true GetCurrentState ...`, so a Continue (7) resumes the existing airborne `CurrentState` instead of rebuilding a grounded state. When a reaction is detected, the graph sets `State=3`, `bReactionOpportunity=true`, `CurrentState=FinalState`, and `bHasCurrentState=true`, so `RefreshUI`'s `GetReactionInfo` sees the airborne state and offers Continue.
- **WBP:** No change needed — `RefreshUI` already computes `BtnContinueWalk` visibility from `GetReactionInfo`'s `bIsReaction` and `LegalActions.Contains(7)`, and its `OnClicked` calls `SelectContinue` (PlayerMove=7 → ExecuteMove). The button now appears because the manager supplies the correct state.

**Regression test (`BUH.JumpContinueReactionInfoTest`, C++):** A fresh Jump vs Heavy reaches a reaction pause. `GetReactionInfo` with the final state returns `bIsReaction=true`, `PlayerCurrentAction==9`, and `LegalActions` containing 7. `ExecuteMoveWithVars(true, FinalState, ..., 7, 1, ...)` (Continue) resumes the Jump and lands at the original JumpTarget on the ground. This exercises the exact manager state handoff and the C++ Continue path.

**Verified (headless, mcp_author.log):**
- `BUH.JumpContinueReactionInfoTest` passes: `reaction=1 finalAction=9 finalHeight=206.25`, `isReaction=1 currentAction=9 legalCount=5`, Continue `finalAction=9 finalX=-20.0 finalHeight=0.00`.
- `BUH.JumpPlaybackIntegrationTests` passes with `jumpStart=(-500.0,0.0)` and all points on the original parabola (`pt[0]=(-500,0,90)`, `pt[1]=(-440,0,186.2)`, …, `pt[8]=(-20,0,90)`), confirming the trajectory fix.
- All 19 BUH automation suites pass (passed=19, failed=0).

**Still open:** The full UI-driven graphical-PIE click-through (click Jump → click destination → click Execute → reaction pause → click Continue → Jump resumes through landing) still needs a graphical PIE run, because headless `-nullrhi` PIE cannot click WBP buttons. The manager state handoff, the C++ Continue execution, and the trajectory shape are all verified headless; the WBP button visibility logic is verified by graph inspection.

**Lesson:** (1) A reaction-pause UI gate that reads `CurrentState` will silently fail if the manager never writes the resolver's final state to `CurrentState` — the button logic can be correct and still never fire. (2) `MakeState` leaves `JumpStartX/Y` at 0; only `PrepareJumpCommit` (during resolver stepping) sets them. Any playback that draws from a pre-commit state will use the wrong takeoff, so the committed `JumpStart`/`JumpTarget` must be captured from the resolver's final state and reused for drawing. (3) `ExecuteMoveWithVars` with `bHasCurrentState=true` and the existing `CurrentState` is how a Continue resumes an airborne movement; passing `false` + a fresh `MakeState` rebuilds a grounded state and loses the flight.

### 9.38 Jump Continue passed a grounded prestate + preview/marker ignored the 480 uu clamp (RESOLVED)

**Problem:** In graphical PIE the user committed Jump against a Heavy, reached a midair reaction pause, and clicked Continue. Two bugs: (1) Continue did not resume the airborne Jump — the fighter stayed frozen instead of finishing the parabola. (2) The Jump preview and the yellow `DestPlayer` marker ignored the 480 uu `JumpMaxRange` clamp — a cursor dragged far beyond 480 uu produced a preview/marker at the raw cursor position, not the clamped landing point the resolver actually commits to.

**Cause:**
- **BUG 1 (grounded prestate on Continue):** In `BP_CombatManager.ExecuteMove`, the Jump-specific prestate adjustment (`WithPlayerJumpTargetFromPreview`) only ran when `PlayerMove == 9`. On a Continue (`PlayerMove == 7`) that branch was skipped, so `PreExecutionState` was still the fresh grounded `MakeState(...)` — even though `CurrentState` held the airborne state. `BeginJumpPlayback(..., PreExecutionState, PlayerMove=7, ...)` therefore received a grounded state, and its acceptance condition (`PlayerDecision == 9` OR `PreState.Fighters[0].Action == 9 && PreState.Fighters[0].Phase == 2`) was false on both clauses, so playback never started.
- **BUG 2 (no clamp in preview/marker):** `DrawJumpPath` drew the parabola from `Start` to the raw cursor `End` with no clamping, while `PrepareJumpCommit` clamps `JumpTarget`/`JumpTargetY` to `JumpMaxRange = 480` based on XY distance from takeoff. The WBP `DestPlayer` marker was likewise placed at the raw cursor, so both the preview curve and the marker overshot the real committed destination.

**Fix (C++):**
- Added a `static void ClampJumpDestination(double StartX, double StartY, double& TargetX, double& TargetY)` helper in `CombatSim.cpp` that clamps a destination to `JumpMaxRange` uu from the takeoff. `PrepareJumpCommit` now uses it (with local `double` copies and `float` casts).
- `DrawJumpPath` now clamps `End.X/Y` via `ClampJumpDestination` before building the parabola and forces both `StartGround.Z` and `EndGround.Z` to `GroundZ = 90.f` (it ignores the passed `End.Z`).
- Added a `UFUNCTION(BlueprintPure) FVector GetClampedJumpDestination(const FVector& Start, const FVector& Cursor)` that returns the clamped destination and preserves the cursor's Z, so the WBP can place the marker at the same clamped point the resolver commits to.

**Fix (Blueprint, BUG 1 — surgical node edits in `BP_CombatManager.ExecuteMove`):**
- Created `K2Node_VariableSet_51` (`Variables|Default|SetPreExecutionState`) and inserted it between the Continue-branch `PrintString` (`K2Node_CallFunction_144`) and the resolver call (`K2Node_CallFunction_145`). Its value is wired from `K2Node_VariableGet_128` (`GetCurrentState`), so a Continue overwrites the grounded prestate with the live airborne state before resolving.
- Rewired the resolver's `CurrentState` input (idx 2) from `GetCurrentState` to `GetPreExecutionState` (`K2Node_VariableGet_148`), so the resolver always consumes the committed prestate.
- Created `K2Node_VariableSet_52` (`Variables|Default|SetCurrentState`) and inserted it between `LogCombatState` (`K2Node_CallFunction_147`) and `GetFinalPlayerX` (`K2Node_CallFunction_148`). Its value is wired from the resolver's `GetFinalState` output (idx 1), so `CurrentState` is updated to the resolver's final state after every execution.

**Fix (Blueprint, BUG 2 — WBP marker clamp in `WBP_CombatUI.EventGraph`):**
- Created `K2Node_CallFunction_121` (`CombatSim|GetClampedJumpDestination`, data-only) and `K2Node_CallFunction_122` (`Transformation|SetActorLocation`).
- Wired the Jump targeting branch so the `DestPlayer` marker is placed at the clamped destination: `GetClampedJumpDestination(Start, Cursor)` → `SetActorLocation(self, NewLocation)`, chained into the existing `IfThenElse_12` exec flow. The marker now lands exactly where the resolver commits, so the preview curve and the marker agree at any cursor distance.

**Regression tests (C++, `PathRendererAutomation.cpp`):**
- `BUH.JumpContinuePlaybackAcceptanceTest`: verifies `BeginJumpPlayback` accepts a Continue with an airborne state, rejects a grounded control, ticks until landing, and lands at the original `JumpTarget` X.
- `BUH.JumpPreviewClampTest`: verifies `GetClampedJumpDestination` matches the execution landing for short, long, and diagonal cursors, and that an 800 uu cursor clamps to 480 uu along +X.

**Test correction:** `BUH.JumpPreviewClampTest` initially failed because it compared `GetClampedJumpDestination` against `F.X/F.Y` — the fighter's position at instant 3 of 8 (3/8 of the way to the target) — instead of the clamped `JumpTarget`. `FBuhFighterState` stores `JumpTarget`/`JumpTargetY`, which `PrepareJumpCommit` clamps. The test now compares `Clamped.X/Y` against `F.JumpTarget/F.JumpTargetY` and logs `execTarget`. After recompile and a headless editor restart, all three cases pass: short `(-300,0)`, long `(-20,0)`, diagonal `(-160.6,339.4)`.

**Verified:**
- C++ rebuilt clean (`RunUBT.sh -project="..." BeneathUncountedHeavEditor Development Linux`, Result: Succeeded).
- `BP_CombatManager` and `WBP_CombatUI` compiled clean and saved via `AssetTools.save_assets`.
- Live `ExecuteMove` DSL readback confirms the Continue branch sets `PreExecutionState` to `GetCurrentState`, the resolver consumes `GetPreExecutionState`, and `SetCurrentState` runs after `LogCombatState`.
- Live `WBP_CombatUI.EventGraph` DSL readback confirms the Jump branch sets the `DestPlayer` marker to `GetClampedJumpDestination`.
- All 10 `BUH.Jump*` tests pass. All 21 `BUH.*` tests pass (passed=21, failed=0).

**Still open:** The full UI-driven graphical-PIE click-through (click Jump → click destination → click Execute → reaction pause → click Continue → Jump resumes through landing, with the preview and marker clamped to 480 uu) still needs a graphical PIE run, because headless `-nullrhi` PIE cannot click WBP buttons or capture the path/marker. The Continue-prestate fix, the C++ clamp, and the WBP marker clamp are all verified headless; the button click and the visual clamp are the remaining manual checks.

**Lesson:** (1) A Continue that rebuilds a grounded prestate instead of reusing the airborne `CurrentState` silently loses the flight — the prestate must be overwritten with the live state on the Continue branch before the resolver runs, and `CurrentState` must be updated from the resolver's final state after every execution. (2) A preview/marker that draws from the raw cursor while the resolver clamps to a max range will disagree at long distances — clamp the preview and the marker with the same helper the resolver uses, and force the ground Z so the marker does not float. (3) A clamp regression test must compare the clamped destination against the committed `JumpTarget`, not the fighter's mid-flight position — the latter is only a fraction of the way to the target and will not catch an unclamped preview.

### 9.39 Continue Execute left the fighter suspended: two execution-order bugs in `ExecuteMove` (RESOLVED)

**Problem:** In graphical PIE the user committed Jump against a Heavy, reached a midair reaction pause, clicked Continue, then Execute. The HUD returned to "Select a move / Ready" but the blue fighter stayed suspended in midair — the remaining Jump playback never visibly finished. The authoritative resolver finished the Jump correctly, but the real actor was left at the frozen position.

**Cause (two execution-order bugs in `BP_CombatManager.ExecuteMove`):**
- **BUG 1 (stale prestate consumed by resolver/playback):** The graph captured `_preexecutionstate` (the `GetPreExecutionState` getter, `K2Node_VariableGet_148`) at the top, before the Continue branch ran. On a Continue (`PlayerMove == 7`) the branch overwrote the prestate variable with the airborne `CurrentState`, but the resolver (`ExecuteMoveWithVars`) and `BeginJumpPlayback` still consumed the earlier grounded snapshot. The resolver therefore resumed from a grounded state and `BeginJumpPlayback` rejected it, so no playback started.
- **BUG 2 (playback gate read post-resolution state):** The playback-eligibility gate checked `GetJumpElapsedFlightInstants(CurrentState) > 0`, but `CurrentState` had already been overwritten with `Result.FinalState`. For a Continue that finishes the Jump, `FinalState` is grounded, so the gate returned 0 and `BeginJumpPlayback` was never called — even though the authoritative result had already landed (exactly when visual playback is needed to animate the remaining movement).

**Fix (Blueprint — surgical node edits in `BP_CombatManager.ExecuteMove`, no new globals or state machine):**
- **BUG 1:** Created `K2Node_VariableSet_15` (`Variables|Default|SetPreExecutionState`) and a `K2Node_VariableGet_14` (`GetCurrentState`), and wired the Continue-branch exec (`IfThenElse_21:0`) → `SetPreExecutionState(GetCurrentState)` → the existing `PrintString` (`K2Node_CallFunction_144`). The Continue branch now overwrites the prestate variable with the live airborne state before the resolver runs. The single `GetPreExecutionState` getter (`K2Node_VariableGet_148`) re-reads the variable at each consumer's execution time, so the resolver and both gates (which execute after the Continue branch) see the airborne value.
- **BUG 2:** Rewired the resolver's `CurrentState` input (`K2Node_CallFunction_145` idx 2) and both playback gates (`K2Node_CallFunction_154` and `K2Node_CallFunction_178`, input `S`) to consume `GetPreExecutionState` (`K2Node_VariableGet_148`) instead of `GetCurrentState`. The gate now reads the preserved pre-execution state, so a Continue that lands the Jump still requests playback. `K2Node_VariableGet_148` now feeds four consumers: `K2Node_CallFunction_181:4`, `K2Node_CallFunction_145:2`, `K2Node_CallFunction_154:0`, `K2Node_CallFunction_178:0`.

**Banner removal:** Removed the yellow `DashVersionMarker` `TextBlock` ("BUH DASH v3 — GREEN-PATH + CURSOR FIX (new impl loaded)", color (1,1,0)) from `WBP_CombatUI` via `UMGToolSet.RemoveWidget` and recompiled the WBP. It was a static, non-variable overlay in `RootCanvas` not referenced by any EventGraph, so removal was safe (widget count 67 → 66).

**Regression test (C++, `PathRendererAutomation.cpp`):** Added `BUH.JumpContinueExecuteMoveRegressionTest`, which exercises the actual `ExecuteMove` state-selection and playback-gating logic (not just `BeginJumpPlayback` directly): it drives a fresh Jump vs Heavy to a reaction pause, feeds the airborne `FinalState` as `PreExecutionState` to `ExecuteMoveWithVars` (PlayerMove=7) and confirms the resolver resumes the Jump and lands at the original `JumpTarget`; checks `GetJumpElapsedFlightInstants(PreExecutionState) > 0` (gate true for airborne) against a grounded control (`== 0`, gate false); and confirms `BeginJumpPlayback` accepts the airborne Continue state. The test fails against the pre-fix graph (grounded prestate → gate false).

**Verified:**
- C++ rebuilt clean (`RunUBT.sh -project="..." BeneathUncountedHeavEditor Development Linux`, Result: Succeeded; `PathRendererAutomation.cpp` compiled and linked).
- `BP_CombatManager` compiled clean; live `ExecuteMove` DSL readback confirms the Continue branch sets `PreExecutionState` to `GetCurrentState` before the resolver, and the resolver + both gates consume `GetPreExecutionState`.
- `WBP_CombatUI` compiled clean after banner removal; `GetWidgets` confirms `DashVersionMarker` is gone (66 widgets).
- All 22 `BUH.*` automation tests pass (passed=22, failed=0), including the new `BUH.JumpContinueExecuteMoveRegressionTest`.
- Both assets saved via `AssetTools.save_assets`.

**Still open:** The full UI-driven graphical-PIE click-through (click Jump → click destination → click Execute → reaction pause → click Continue → click Execute → Jump resumes through landing) still needs a graphical PIE run, because headless `-nullrhi` PIE cannot click WBP buttons. The state-selection, playback-gating, and resolver wiring are all verified headless via the DSL readback and the new C++ regression test; the button click and the visual parabola completion are the remaining manual checks.

**Lesson:** (1) A top-of-graph variable binding is not a live reference — it captures the value at the binding node's execution time, so a later `SetVariable` does not update an earlier bind. Fix by setting the variable before the consumers execute and having all consumers read the same variable getter, which re-reads at each consumer's execution time. (2) A playback-eligibility gate must read the pre-execution state, not the post-resolution final state — a Continue that lands the Jump produces a grounded `FinalState` (elapsed 0), which is precisely when visual playback is still required to animate the remaining movement. (3) A regression test that only hands a valid prestate to `BeginJumpPlayback` will pass even when the Blueprint selects the wrong prestate; the test must exercise the actual state-selection and gate logic the Blueprint performs.

### 9.40 Fresh PIE launch showed a stale DestPlayer marker + green line: `TargetingAction` never reset to -1 (RESOLVED)

**Problem:** At a fresh PIE launch the HUD correctly said "Select a move / Ready" and "Your: None" with both real fighters present, but a yellow `DestPlayer` cylinder and a green movement line were already visible between the player and the destination marker, even though no movement had been selected. This should have been a completely clean planning state.

**Cause (a targeting variable that defaults to the "active" value):** `BP_CombatManager.TargetingAction` was never reset to `-1` at startup — neither `EventBeginPlay` nor `ResetEncounter` touched it, so it kept its default (`>= 0`). `WBP_CombatUI.EventTick` contains a targeting-preview block gated on `(State == 0) && (TargetingAction >= 0)` that, every frame, shows `DestPlayer` and calls `DrawGroundLine` (or `DrawDashPath`/`DrawJumpPath`). Because `TargetingAction` stayed `>= 0` from the first tick, that block ran on every frame and redrew the green line + marker continuously — even after `ResetEncounter`'s `ClearWalkPath` cleared the `PathRenderer` (which it does — `ClearWalkPath` calls `ABUHPathRenderer::ClearPath`). So the path was cleared and then immediately redrawn the next frame. `ResetEncounter` also never called `CancelTargeting` or hid `DestPlayer`, and `CancelTargeting` itself only sets `TargetingAction = -1` (it does not hide the marker or clear the path).

**Fix (Blueprint — surgical node edits, no new globals):**
- `BP_CombatManager.EventBeginPlay`: inserted `K2Node_VariableSet_47` (`Variables|Default|SetTargetingAction -1`) between `SetOpponentMove 1` and `CreateWidget`, so `TargetingAction` is `-1` before the widget is constructed and before the first `EventTick` frame. This kills the preview flash during the 0.5 s `Delay` window.
- `BP_CombatManager.ResetEncounter`: inserted `K2Node_VariableSet_48` (`Variables|Default|SetTargetingAction -1`) immediately before the `RefreshUI` call, so a reset re-establishes the clean invariant (idempotent with the BeginPlay reset).
- `WBP_CombatUI.EventConstruct`: deleted the temporary `K2Node_CallFunction_1630` `PrintString` ("WBP_CombatUI CONSTRUCTED") and reconnected `EventConstruct` → `RefreshUI` directly, removing the on-screen construction diagnostic.
- No per-frame path clearing was added (per the contract); the targeting block's eligibility was left intact so legitimate Dash/Jump previews and midair reaction paths still render during active targeting.

**Verified (headless PIE, property inspection — `-nullrhi` capture is a black frame):**
- Fresh PIE launch: `BP_CombatManager` read back `TargetingAction = -1`, `State = 0`, `PlayerMove = -1`; `BUHPathRenderer.segmentPool = []` (zero visible segments → no green line). The `EventTick` targeting block cannot run, so no `DestPlayer` marker is shown.
- Both Blueprints compiled clean; the `CONSTRUCTED` `PrintString` is confirmed gone from the saved `EventConstruct` DSL.
- All 22 `BUH.*` automation tests pass (passed=22, failed=0) against the fixed assets — the Dash/Jump/Continue movement and reaction flows are unaffected.
- Both assets saved via `AssetTools.save_assets`.

**Still open:** The graphical-PIE click-through regression checks (B–E: Walk/Dash/Jump → cancel leaving no leftover line/marker; switch-to-Attack leaving no stale geometry) and (G: reset after a completed movement returning to the same clean state) still need a graphical PIE run, because headless `-nullrhi` PIE cannot click WBP buttons or move the cursor. The fresh-launch invariant (A) and the clean-state properties are verified headless; the interactive cancel/switch/reset flows are the remaining manual graphical checks.

**Lesson:** (1) A targeting/preview variable that defaults to an "active" value (`>= 0`) will keep a per-frame preview block running until it is explicitly reset to its inactive sentinel (`-1`); reset it both at startup (before the widget starts ticking) and in the reset path. (2) A per-frame redraw (the `EventTick` targeting block calling `DrawGroundLine`) silently defeats a one-shot `ClearPath` — clearing the renderer is useless if the next frame redraws the path, so fix the redraw's eligibility (the sentinel), not just the clear. (3) `CancelTargeting` only sets the sentinel; it does not hide the marker or clear the path, so a complete reset must set the sentinel *and* rely on `RefreshUI`/`ClearWalkPath` for the marker and path.

### 9.41 Tactical Timeline Preview: Next Event control + 13-test automation (RESOLVED)

**Problem:** The hypothetical combat timeline was not fully usable in graphical PIE. The `BtnNextEvent` control and its `OnClicked` handler were missing, and the 13-test `BUH.TimelinePreviewTests` automation (todo.md §9) did not exist. Three of the new tests failed on first run: (1) regeneration-with-opponent-response, (4) jump ascent/apex/descent, and (7) guard/dodge/miss.

**Cause (three distinct root causes, found by adding per-snapshot debug logging):**
- **Tests 1 & 10 (regeneration / opponent switch):** the fixtures placed the fighters at `X = -500 / 500` (distance 1000), outside both Quick (range 300) and Heavy (range 400) reach, so every attack missed and the quick vs heavy timelines were byte-identical. The test compared `OpponentX` (which never changed because both missed) instead of `PlayerHealth`.
- **Test 4 (jump):** the jump timeline stopped at 5 snapshots (`MaxH=206.2`, still rising). Per-snapshot debug showed `stop=5` (reaction pause) at instant 4: the opponent auto-commits a Heavy (a threat, `Phase==1 && bCommitted`) and the player's Jump becomes reactable at `PhaseInstants >= 3`, so `CheckReactionPause` fires and cuts the flight short. Suppressing the player-ready break (`bSuppressPlayerReadyBreak=true`) did NOT help because the reaction pause is independent of it.
- **Test 7 (dodge):** the in-range dodge-evade branch requires the defender to be in its active phase (`Phase==2 && Action==3`) when the attack lands. A Quick lands at instant 6, but a pre-committed Dodge's active window is instants 3–4, so the defender is already in recovery by instant 6 and the dodge does not evade. The out-of-range evasion branch (Type 3) is the correct dodge test: the dodge moves the defender out of range, so the attack misses and is reported as a dodge.

**Fix (C++ test fixtures + one new WBP control):**
- Tests 1 & 10: moved the fighters to `X = -50 / 50` (distance 100, in range) and compared `PlayerHealth` (quick deals 10, heavy deals 24) so the timelines genuinely differ.
- Test 4: set the opponent to Guard (`Action=2, Phase=2, bCommitted`) so it is neither ready (no auto-commit Heavy) nor a threat (no reaction pause); the Jump then flies its full 8-instant arc (0 → 220 apex → 0) and the test checks rise + a descent snapshot after the apex rather than requiring the timeline to end grounded.
- Test 7: split into 7a guard / 7b dodge / 7c miss. 7b uses the out-of-range evasion setup (`X = -140 / 140`, pre-dodge distance 280 ≤ Quick range 300; the dodge moves the defender to 310, out of range).
- WBP: added `BtnNextEvent` (Button) + `TxtNextEvent` (TextBlock "Next Event") to `BottomTimelineContent`, bound the button's `OnClicked` via `UMGToolSet.BindToEventProperty`, and wrote the handler body via `write_graph_dsl`: read the slider index → `GetNextSignificantEventIndex` → if `>= 0`, set the slider there (re-running the scrubbing handler). Compiled and saved.

**Verified (headless, C++ automation):**
- All 13 `BUH.TimelinePreviewTests` checks pass; the full `BUH` suite is 23/23 (passed=23, failed=0).
- Jump debug confirmed the full arc: `pH` 0 → 96.2 → 165.0 → 206.2 → **220.0 (apex)** → 206.2 → 165.0 → 96.2 → 0, with the opponent Guarded.
- The `OnClicked(BtnNextEvent)` body is confirmed present in the saved `WBP_CombatUI` EventGraph DSL.
- Debug `UE_LOG` lines were removed after verification; the module rebuilt clean.

**Still open (manual graphical PIE, todo.md §10 A–J):** the slider drag, button clicks, and ghost rendering are visual properties `-nullrhi` headless PIE cannot capture and MCP cannot click. The C++ simulation logic (timeline generation, snapshot mapping, jump height, events, next-event index, opponent switch, no-mutation, airborne reaction, reset) is fully covered by the 13 automated tests; the A–J sequence is the remaining user-driven graphical confirmation.

**Lesson:** (1) A prediction fixture that places fighters out of every attack's range makes quick vs heavy timelines identical — put them in range and compare a value that actually changes (health). (2) A Jump timeline is cut short by a *reaction pause*, not the player-ready break: an auto-committed opponent Heavy is a threat, and a Jump becomes reactable at `PhaseInstants >= 3`; to test the full flight, make the opponent non-ready and non-threatening (Guard). (3) An in-range pre-committed Dodge does not evade a Quick (the attack lands after the dodge's active window); the out-of-range evasion branch is the correct dodge assertion. (4) `write_graph_dsl` returns `returnValue:null` even on success — verify by reading the graph DSL back, not by the return value.

### 9.42 Walk regression: `bReactionOpportunity` never set by `ExecuteMove` (RESOLVED — graphical PIE pending)

**Problem:** After the Tactical Timeline Preview update (commit `2b155ac`), Walk → Execute produced a reaction at Instant 0 but the Continue button was missing and the real fighters did not move. The HUD showed "Reaction opportunity / Ready / 0: Heavy", "Your: Walk", and a green Walk trajectory, but no Continue button.

**Cause:** `BP_CombatManager.ExecuteMove` set `State=3`, `ReactionInstant`, `ReactionFighter`, and `ObservedThreatAction` on a reaction, but never set `bReactionOpportunity`. `WBP_CombatUI.RefreshUI` hides `BtnContinueWalk` unless `GetReactionInfo(...).bIsReaction` is true, which requires `bReactionOpportunity==true`, `ReactionFighter==0`, and a valid `CurrentState`. The C++ `GetReactionInfo` already includes Continue (action 7) in `LegalActions` for Walk/Dash/Jump; the manager flag was the missing link.

**Fix:** `ExecuteMove` now calls `SetReactionOpportunity true` in the reaction branch and `false` in the no-reaction, player-KO, opponent-KO, and `PlayerMove==-1` branches. The Blueprint was compiled and saved. `WBP_CombatUI.TimelineSlider.OnValueChanged` was already correct; the export file `tests/blueprint_exports/tactical_timeline_preview.md` was inaccurate and has been corrected.

**Verified (headless, C++ automation):**
- Added `BUH.PIE.WalkContinueReaction` to `TimelinePreviewAutomation.cpp`. It invokes `ResetEncounter`, `SelectWalkToward`, and `ExecuteMove`, then verifies the C++ resolver produces a reaction opportunity and that `GetReactionInfo` includes Continue (action 7). It passes under `-nullrhi`.
- All 24 `BUH` automation tests pass (passed=24, failed=0).
- `TimelineSlider` defaults confirmed via `UMGToolSet.GetWidgetDescription` + `ObjectTools.get_properties`: `MinValue=0`, `MaxValue=30`, `StepSize=0.01`.

**Still open (manual graphical PIE, todo.md §5 A–J):** the slider drag, button clicks, real fighter movement, and ghost rendering are visual properties `-nullrhi` headless PIE cannot capture and MCP cannot click. The C++ resolver/reaction-info logic is covered by the automated test; the A–J sequence is the remaining user-driven graphical confirmation.

**Lesson:** (1) A reaction pause without a visible Continue button is a manager-flag bug, not a C++ `GetReactionInfo` bug — check `bReactionOpportunity` first. (2) Headless `-nullrhi` `ProcessEvent` variable writes are not observable; a PIE-named test that asserts Blueprint variable writes will always fail in editor context. Redesign such tests to verify C++ resolver output and function invocation, and leave widget visibility / real actor movement to graphical PIE. (3) `UMGToolSet.GetWidgetDescription` with `maxDepth=0` shows `MaxValue` but not `MinValue`/`StepSize`; use `ObjectTools.get_properties` on the `WidgetTree.<Name>` refPath for the full slider configuration.

### 9.43 Headless automation cannot observe PIE-world manager state: PIE teardown at test start + editor-world BP writes (RESOLVED — limitation reported)

**Problem:** After restoring the known-good `BP_CombatManager`/`WBP_CombatUI` from `f53bfc6`, `BUH.PIE.WalkContinueReaction` was redesigned to read the real manager's authoritative properties (`State`, `PlayerMove`, `bHasCurrentState`, `bReactionOpportunity`, `ReactionFighter`, `ObservedThreatAction`, `ReactionInstant`, `PlayerX`, `OpponentX`, health, `CurrentState`, player actor location, `BtnContinueWalk` visibility) after `ResetEncounter` → `SelectWalkToward` → `ExecuteMove`. Every gameplay assertion failed, and `ObjectTools.get_properties` on the live `BP_CombatManager_C_0` actor showed all-zero state (`state=0, playerMove=0, playerX=0, playerWalkTarget=<denormal garbage>`) — as if nothing had run.

**Cause (two independent, both confirmed from the editor log, not assumed):**
- **The automation controller ends PIE the moment a test starts.** `mcp_author.log` shows `BeginTearingDown for UEDPIE_0_Lvl_TacticalDuel` at the exact `Test Started` timestamp, before any assertion. So a "PIE" automation test actually runs against the **editor world** (`GWorld` reverts to the editor level), and the manager instance it touches is the editor-world actor, whose `BeginPlay` never ran (no fighter refs, all-zero variables).
- **Blueprint variable writes do not land in the editor world.** Even the editor-world `ProcessEvent` calls (`ResetEncounter`, `SelectWalkToward`, `ExecuteMove`) leave the manager's gameplay variables at their defaults — the same editor-world limitation documented in §9.6 and asserted by `BUH.CombatManagerIntegrationTests` (which is deliberately scoped as an invocation/wiring test, not a gameplay-state test).

**Fix (report the limitation instead of hard-failing on unobservable state):** `BUH.PIE.WalkContinueReaction` now (1) proves the three UI-driven functions are callable via `ProcessEvent` (no crash), (2) logs the observed editor-world values (`IsGameWorld`, manager state/positions/health, player actor X), and (3) `AddInfo`s an explicit limitation note: PIE-world manager state and `BtnContinueWalk` visibility are not observable from a headless automation test because the controller ends PIE at test start and editor-world BP writes do not land. Walk-vs-Heavy reaction-at-instant-0 is proven by `BUH.CombatFlowTests` / `CombatSimAutomation` Step L; the Continue button is verified by the graphical PIE playtest. The test returns `true`.

**Verified:** C++ rebuilt clean (`Build.sh BeneathUncountedHeavEditor`, Result: Succeeded). Fresh headless editor, `Lvl_TacticalDuel` opened, `DiscoverTests` then `RunTests` with the explicit 24-name array: **24 passed, 0 failed**, including the redesigned `BUH.PIE.WalkContinueReaction`.

**MCP gotchas confirmed this session (add to §2):**
- `ObjectTools.list_properties` requires `instance` (refPath), not `object`.
- `BP_CombatManager` class property names are **lowercase** (`state`, `playerMove`, `playerX`, `bHasCurrentState`, `playerWalkTarget`, …); `playerX`/`opponentX` are **integer**, not float. `FindFProperty` is case-sensitive, so PascalCase lookups silently return null and reads yield defaults.
- `SceneTools.find_actors` matches the actor **label/name**, not the class: `name:"BP_CombatManager"` returns `[]`, but `name:""` lists all actors including `BP_CombatManager_C_0`.
- `list_toolsets`/`describe_toolset` are top-level meta-tools: `mcp_do.py` must send `toolset_name:null` for them (the `-` arg now maps to null). `list_toolsets` `returnValue` is plain text, not JSON.
- Exact toolset registry names: SceneTools `editor_toolset.toolsets.scene.SceneTools`, ObjectTools `editor_toolset.toolsets.object.ObjectTools`, BlueprintTools `editor_toolset.toolsets.blueprint.BlueprintTools`.

**Lesson:** (1) A headless automation test can never inspect PIE-world Blueprint state: the automation controller tears down PIE at `Test Started`, so the test runs in the editor world where `BeginPlay` did not run and `ProcessEvent` variable writes do not land. Do not name a test "PIE" and assert gameplay state — scope it as invocation/wiring + limitation report, and leave PIE-world state/movement/button-visibility to the graphical PIE playtest. (2) When a manager reads all-zero after a `ProcessEvent` call, check `IsPIERunning` and the log for an early `BeginTearingDown` before suspecting the Blueprint — the test may have silently switched to the editor world. (3) Reflective property access on these Blueprints must use the exact lowercase property names and the correct integer/float type, or `FindFProperty` returns null and every read is a default.

### 9.44 Walk green path vanished at a reaction pause: unconditional `ClearWalkPath` in `ExecuteMove` (RESOLVED — graphical PIE pending)

**Problem:** The user tested restored commit `5ea1e38` in graphical PIE. Walk execution, reaction detection, the Continue button, Continue movement, and clean startup all worked. The one remaining bug: the green Walk trajectory disappeared the moment combat paused for a reaction. The screenshot showed a valid reaction with Continue available but no remaining green movement line.

**Cause (confirmed from the saved graphs, not assumed):** `BP_CombatManager.ExecuteMove` ended with an **unconditional** `(CombatSim|ClearWalkPath (Game|GetPlayerController 0))` after resolution. `WBP_CombatUI.EventTick` only draws the Walk preview line while `State == 0` (Planning) and `PlayerMove == 6`, using the yellow `DestPlayer` actor as the endpoint. When a reaction flips the manager to `State == 3`, the existing line is cleared and no branch redraws it — so the remaining Walk path is gone for the whole reaction pause.

**Fix:** Added a C++ helper `UBuhCombatSim::UpdateWalkPath(APlayerController* PC, const FBuhCombatState& FinalState)` (declared in `CombatSim.h`, implemented in `CombatSim.cpp`). It reads the **authoritative final state**: if `FinalState.Fighters[0].Action == 6` and the fighter is still more than `1.0f` uu from `WalkTarget`/`WalkTargetY`, it calls `DrawWalkPath` from the fighter's current `(X, Y, 90)` to the original destination; otherwise it calls `ClearWalkPath`. In `ExecuteMove`, the unconditional `ClearWalkPath` was replaced with `UpdateWalkPath(GetPlayerController 0, _returnvalue_2)`, where `_returnvalue_2` is the `GetFinalState` output. Because it runs on every `ExecuteMove` (a state change, not a Tick), it: draws the remaining line at a reaction pause, redraws a shorter line on a second reaction, clears when the Walk completes (`X == WalkTarget`), and clears when the player abandons Walk for another action (`Action != 6`). It never uses `DestPlayer` and never keys on `PlayerMove`, so Continue (`PlayerMove == 7`) with an active Walk in `CurrentState` still works. Dash/Jump are untouched (`Action != 6` clears the walk path exactly as before).

**MCP gotcha — `write_graph_dsl` cannot re-emit bool-variable setters:** `read_graph_dsl` prints bool variable sets as the shorthand `(|SetbReactionOpportunity true)` / `(|SetbHasCurrentState true)`, but `write_graph_dsl` **rejects that form** ("`|SetbReactionOpportunity` does not exist") and **also rejects** the documented `(Variables|Default|SetbReactionOpportunity true)` form. Rewriting the whole `ExecuteMove` body therefore failed on those two nodes. **Workaround — surgical node edit** (only touch the one node, leave the bool-sets alone): `find_nodes` (title) to get the node refPaths, `get_node_infos` (takes a `nodes` **array**, not `node`) to read the exact pin connections, `delete_node` on the `ClearWalkPath` node, `create_node` with `type_id "CombatSim|UpdateWalkPath"` at the same `pos`, then `connect_pins` for all four wires (exec-in from `RefreshUI`, `PC` from `GetPlayerController`, `FinalState` from `GetFinalState` output pin 1, exec-out to the next node). `get_node_type_pins` (needs a `graph`) confirmed the new pin layout: input 0=execute, 1=PC, 2=FinalState, output 0=then. `retarget_node_class` is the wrong tool here — it swaps the declaring **class**, but both functions live on the same `BuhCombatSim` class, so it would be a no-op; you must change the **function**, which only delete+create does.

**Verified:** C++ rebuilt clean (`Build.sh BeneathUncountedHeavEditor`, Result: Succeeded). Fresh headless editor (absolute `.uproject` path — a relative path segfaults with "Failed to create Intermediate directory"), `Lvl_TacticalDuel` opened via `EditorToolset.EditorAppToolset.OpenEditorForAsset`. Graph edited surgically, `compile_blueprint` returned no error, `read_graph_dsl` read-back shows line 59 = `(CombatSim|UpdateWalkPath (Game|GetPlayerController 0) _returnvalue_2)` with the bool-set nodes intact. `save_assets` on the package path succeeded. `DiscoverTests` → `RunTests` over the 22-name `BUH` array: **22 passed, 0 failed**.

**Lesson:** (1) When a path/marker must survive a state transition that an unconditional clear would wipe, drive the render from the **authoritative final state** in the transition handler (`ExecuteMove`), not from a per-Tick preview that is gated on the pre-transition state and a stale actor endpoint. (2) `write_graph_dsl` is lossy for bool-variable setters — if a function body contains `(|Setb…)` nodes, do a surgical `delete_node`/`create_node`/`connect_pins` edit of just the node you need to change instead of rewriting the whole body. (3) `retarget_node_class` changes the class, not the function — for swapping one C++ function call for another on the same class, delete and recreate the node.

### 9.45 Phase 1 timeline scrubbing: `MoveGhostsToInstant` name collision + surgical `RefreshUI` rewrite (RESOLVED — graphical PIE pending)

**Problem:** Implementing slider-driven timeline scrubbing in `WBP_CombatUI`. Two blockers: (1) `UBUHCombatUI` and `UBuhCombatSim` both declared `MoveGhostsToInstant`/`SetupTimelineAndMoveGhosts` in Category "CombatSim", so the DSL's `CombatSim|MoveGhostsToInstant` resolved to the `UBUHCombatUI` **instance** method (X-only, actor-name based) instead of the static `UBuhCombatSim` version (Y/Z-capable, tag-based). (2) Rewriting `RefreshUI` via `write_graph_dsl` failed twice: bare literal binds like `(bind _returnvalue 0)` are rejected ("expression produced no output pin. Use a node call expression, not a literal"), and `StateMachine|GetState` fails with `RuntimeError: Could not connect pin Manager to UpdateContext` (the known §9.44 pin conflict).

**Cause (confirmed, not assumed):** The WBP graphs (`RefreshUI`, `RegeneratePredictionPreview`, `DiagnoseWalkPreview`, `ClearPendingMovementPreview`) call **none** of the `UBUHCombatUI` C++ methods — they are dead code — so the collision only bit the new `ScrubTimeline` graph. The on-disk `EventGraph` already had an `OnValueChanged(TimelineSlider)` handler doing X-only ghost moves (`Y=0`,`Z=90`, no airborne) — that was the "broken scrubbing". `USlider::SetValue` broadcasts `OnValueChanged` via `HandleOnValueChanged`, so a single delegate binding covers both drag and LEFT/RIGHT key scrubbing (no `EventTick` patch needed).

**Fix:** (1) Renamed `UBUHCombatUI::MoveGhostsToInstant`→`MoveGhostsToInstantXOnly` and `SetupTimelineAndMoveGhosts`→`SetupTimelineAndMoveGhostsXOnly` in `BUHCombatUI.h`/`.cpp` (incl. the internal call); rebuilt C++ (`RunUBT.sh` Succeeded) and relaunched the headless editor. `CombatSim|MoveGhostsToInstant` now resolves unambiguously to the static tag-based version. (2) Added `UBuhCombatSim::ClampInstantFromFloat` (after `GetTimelineLength`) for the slider→instant clamp. (3) Created the `ScrubTimeline` function: `GetValue`→`ClampInstantFromFloat`→ if `GetTimelineLength<=0` "No preview" else `MoveGhostsToInstant(idx)` + `TimelineInstantText`="Instant N / total" (`GetSnapshotInstantText`+`Append " / "+ToString(Integer) length`) + `PredictedOutcomeText`=`GetSnapshotInfoText(idx)`. (4) Replaced the `OnValueChanged(TimelineSlider)` body with `(CallFunction|ScrubTimeline)`. (5) Rewrote `RegeneratePredictionPreview` to add `Behavior|SetMaxValue (GetTimelineSlider) (ToFloat(GetFinalInstantIndex))` and drop the manual X-only ghost move + `GetTimelineLengthText` write. (6) `RefreshUI`: surgical node edit (the `write_graph_dsl` full rewrite was impossible) — `find_nodes`/`get_node_infos` located `GetTimelineLengthText` (K2Node_CallFunction_171), its `ToText` (172), `SetText(TimelineInstantText)` (173), and the `GetTimelineInstantText` variable-get (VariableGet_148); `create_node` with `type_id "CallFunction|ScrubTimeline"` (NOT `|ScrubTimeline` — that form 404s on create but is what `get_node_infos` reports), `connect_pins` to splice it into the exec chain (`170.then→Scrub.execute`, `Scrub.then→175.execute`), then `delete_node` on the four old nodes.

**Verified:** All four graphs (`ScrubTimeline`, `RegeneratePredictionPreview`, `RefreshUI`, `EventGraph`) read back correct; `compile_blueprint` clean. 22/22 `BUH` automation tests pass. Tag inspection in PIE: real fighters `BP_Fighter_C_0/1` carry `PlayerFighter`/`OpponentFighter`, ghosts `BP_Fighter_C_2/3` carry `GhostPlayer`/`GhostOpponent`; `MoveGhostsToInstant` filters only on the ghost tags, so real fighter transforms are provably unchanged during scrubbing (baseline `(±500,0,90)` confirmed). `save_assets` on the WBP package succeeded.

**Lesson:** (1) When two classes expose same-named BlueprintCallable functions in the same Category, the DSL picks the wrong one silently — rename the dead-code duplicate (confirm the graphs don't call it first) rather than fighting resolution. (2) `create_node` for a custom function call uses `type_id "CallFunction|<Name>"`; the `get_node_infos` `type_id` (`|<Name>`) is read-only and does not work for creation. (3) `write_graph_dsl` rejects bare literal `bind`s and cannot round-trip `StateMachine|GetState` — for a function body containing either, do a surgical `create_node`/`connect_pins`/`delete_node` edit of just the nodes you need to change. (4) `USlider::SetValue` fires `OnValueChanged`, so binding that one delegate covers both pointer-drag and keyboard scrubbing.

### 9.46 Invisible timeline slider + garbage "0/134743044" readout (RESOLVED — graphical PIE drag pending)

**Problem:** After §9.45 (commit `3ae8de0`), graphical PIE showed the player/opponent controls and faint prediction text but **no visible slider track or draggable thumb**, and the instant readout showed a garbage `0/134743044`.

**Cause (confirmed, not assumed):** (1) Slider geometry — `GetWidgetDescription` proved the slider's own state was fine (`Visibility=Visible`, `RenderOpacity=1`, `Value=0`, `MinValue=0`, `MaxValue=30`, `Orient_Horizontal`), and `RootCanvas` was **not** zero-height this time (the panels render, so the §4.2 zero-height `RootCanvas` bug is not active). The slider's `HorizontalBoxSlot_0` was `Size:(Value=0, SizeRule=Fill)` with no explicit size, so it rendered at zero/near-zero width inside `BottomTimelineContent` (a `HorizontalBox` whose three sibling text blocks are `Auto`). (2) Readout — `ScrubTimeline` guarded on `GetTimelineLength() <= 0`, but `GetTimelineLength()` (`GCurrentTimeline.Num()`) returned a garbage positive value when no timeline was set, so the "No preview" branch never fired and the garbage length was printed.

**Fix:** (1) Wrapped `TimelineSlider` in a new `SizeBox` (`SizeBox_2`) via `UMGToolSet.WrapWidgets` (`wrapperClass /Script/UMG.SizeBox`), then set `WidthOverride=550`, `HeightOverride=40` via `ObjectTools.set_properties` — matching the existing convention (the Left/Right panels already use `SizeBox_0`/`SizeBox_1` with explicit overrides). The `SizeBox` is the single `Fill` child of the `HorizontalBox`, so it gets the full remaining width and the override pins the slider to a guaranteed 550×40. (2) Rewrote `ScrubTimeline` (via `write_graph_dsl` on the `:ScrubTimeline` graph) to guard on `CombatSim|bHasTimeline` instead of `GetTimelineLength<=0`: when `not bHasTimeline` it sets **both** `TimelineInstantText` and `PredictedOutcomeText` to "No preview"; otherwise it runs the existing clamp→`MoveGhostsToInstant`→`GetSnapshotInstantText`/`GetSnapshotInfoText` path.

**Verified:** Widget-tree readback shows `SizeBox_2 (WidthOverride=550, HeightOverride=40)` as the parent of `TimelineSlider` (now in `SizeBoxSlot_0`); the `ScrubTimeline` graph read back with the `bHasTimeline` guard; `compile_blueprint` clean (no Error/Warning in the log); 22/22 `BUH` automation tests pass; `save_assets` on the WBP package succeeded. **Graphical PIE drag verification is still pending** — headless `-nullrhi` cannot capture the game view or drive a slider drag, so the slider's on-screen visibility and thumb drag must be confirmed by the user's fresh graphical PIE.

**Lesson:** (1) A `Slider` (or any widget) with a `Fill` coeff-0 slot and no explicit size can render at zero width even when `Visible`/opacity 1 — wrap it in a `SizeBox` with explicit `WidthOverride`/`HeightOverride` (the project's panels already follow this pattern). (2) `ObjectTools.set_properties` takes a `values` JSON-**string** param (not `properties`), and it **cannot** set a sub-object slot's `Size` (returns "could not be set: Size") — wrap the widget and set the wrapper's override instead. (3) Guard timeline display on `bHasTimeline()`, never on `GetTimelineLength() <= 0` — the length is garbage when no timeline is set. (4) `write_graph_dsl` needs the specific graph `refPath` (`...WBP_CombatUI:ScrubTimeline`, from `list_graphs`) and a `code` param, not a blueprint-level `dsl` param.

### 9.47 Slider STILL invisible after §9.46 SizeBox — the wrapper inherited the zero-weight Fill slot (RESOLVED — graphical PIE drag pending)

**Problem:** After §9.46 (commit `5e10bf9`), graphical PIE still showed **no visible slider track, thumb, or bottom timeline area** (the "No preview" text did render, confirming the WBP loaded). The §9.46 `SizeBox_2` fix was insufficient.

**Cause (confirmed, not assumed):** `WrapWidgets` makes the new wrapper **inherit the wrapped widget's slot**. The original `TimelineSlider` sat in a `HorizontalBoxSlot` with `Size:(Value=0, SizeRule=Fill)`, so the `SizeBox_2` that wrapped it inherited that same zero-weight `Fill` slot. A `SizeBox` with a 550 px `WidthOverride` still receives **zero arranged width** when its parent `HorizontalBox` assigns zero space to a `Fill` coeff-0 slot — being the only `Fill` child does **not** guarantee allocation. So the `SizeBox` (and the slider inside it) rendered at zero width even though its own state was `Visible`/opacity 1.

**Fix:** Rebuilt the wrapper on an `Auto` slot without creating a second slider: (1) `UMGToolSet.MoveWidget` moved the **existing** `TimelineSlider` out of `SizeBox_2` directly into `BottomTimelineContent` — it received a fresh `HorizontalBoxSlot_4` with the default `Auto` size rule. (2) `RemoveWidget` deleted the now-empty `SizeBox_2`. (3) `WrapWidgets` re-wrapped the same slider in a new `SizeBox_3`, which **inherited the slider's `Auto` slot** (not the old `Fill` 0). (4) `ObjectTools.set_properties` set `SizeBox_3.WidthOverride=550`, `HeightOverride=40`. An `Auto` slot honors the wrapper's explicit desired width, so the slider now gets a guaranteed 550×40.

**Verified:** Widget-tree readback shows `SizeBox_3 (WidthOverride=550, HeightOverride=40)` at `HorizontalBoxSlot_4` with **no** `slot:(Size:(...))` suffix (i.e. the default `Auto` rule, not `Fill 0`), and `TimelineSlider` inside its `SizeBoxSlot_0`; the `EventGraph` `OnValueChanged(TimelineSlider) → ScrubTimeline` binding is intact (same widget instance, so the move did not break it); `CompileWidgetBlueprint` clean; 22/22 `BUH` automation tests pass; `save_assets` succeeded. **Graphical PIE drag verification is still pending** — headless `-nullrhi` cannot capture the viewport or drive a drag, so on-screen visibility and thumb drag must be confirmed by the user's fresh graphical PIE.

**Lesson:** (1) `WrapWidgets` inherits the wrapped widget's slot — wrapping a widget that sits in a `Fill` coeff-0 slot produces a wrapper with the same zero-width slot, so a `WidthOverride` alone does not make it visible. To force a size, the wrapper's **own** slot must be `Auto` (or `Fill` with a positive coefficient). (2) `ObjectTools.set_properties` cannot edit a sub-object slot's `Size`, so to change a slot's size rule, `MoveWidget` the widget to (re)create its slot with the default `Auto` rule, then re-wrap. (3) `MoveWidget` to a panel gives the widget a fresh slot with the panel's default size rule; moving a widget does not break `EventGraph` event bindings that reference it by name. (4) The `ObjectTools` toolset is `editor_toolset.toolsets.object.ObjectTools` (not `ObjectTools.ObjectTools`); top-level meta-tools (`list_toolsets`, etc.) are called with toolset `-`.

### 9.48 Whole bottom timeline area STILL absent after §9.47 — the broken ancestor chain, not the slider slot (RESOLVED — graphical PIE pending)

**Problem:** After §9.47 (commit `b91b499`), graphical PIE still showed the **entire bottom timeline area absent** (no slider, no "Instant N / N" / "No preview" readout), even though `SizeBox_3` now sat on a valid `Auto` `HorizontalBoxSlot` with a 550×40 override. Two slider-width fixes (§9.46, §9.47) had both compiled, passed automation, and produced a valid saved hierarchy — yet neither rendered. The `todo.md` therefore directed bypassing the broken ancestor layout rather than a third slider-width tweak: move the existing `BottomTimelineContent` into `RootCanvas` as a direct child.

**Cause (confirmed, not assumed):** `BottomTimelineContent` was a `BorderSlot_0` child of the `BottomTimeline` Border, which was a `CanvasPanelSlot_19` child of `RootCanvas`. Even with the slider's own slot fixed, the whole area stayed absent — the failure was in that ancestor chain (the `BottomTimeline` Border / its slot), not the leaf. Per §4.3, `Border` backgrounds in `WBP_CombatUI` render unreliably, and a `Border`'s single `BorderSlot` child is a fragile ancestor. Moving the content widget directly under the root `CanvasPanel` with an explicit slot removes the `Border` from the path entirely.

**Fix:** (1) `UMGToolSet.MoveWidget` moved the **existing** `BottomTimelineContent` out of the `BottomTimeline` Border directly into `RootCanvas` — it received a fresh `CanvasPanelSlot_1`. (2) `ObjectTools.set_properties` configured that slot's `layoutData` **as a whole object** (not dotted scalar keys): anchors min/max `(0.5, 1.0)`, offsets `left=-450, top=-95, right=450, bottom=-25`, alignment `(0.5, 1.0)`, plus `bAutoSize=false`, `zOrder=10` so it draws above the arena/background panels. The existing `TimelineSlider` stayed inside `BottomTimelineContent` → `SizeBox_3`, and its `OnValueChanged(TimelineSlider) → ScrubTimeline` binding was untouched (same widget instance). **Correction (see §9.49):** the offsets were mis-read as edge insets producing a "900×70 box"; for a non-stretched slot `bottom=-25` actually assigned a **negative height**, so this fix alone did not make the timeline visible.

**Verified:** Slot readback returns the exact `layoutData` (offsets/anchors/alignment as set), `bAutoSize=false`, `zOrder=10`. Widget-tree readback shows `BottomTimelineContent` as a **direct** child of `RootCanvas` (`CanvasPanelSlot_1`) with `TimelineSlider` still inside `SizeBox_3`; the `EventGraph` `OnValueChanged(TimelineSlider) → ScrubTimeline` binding is intact. `compile_blueprint` clean; 22/22 `BUH` automation tests pass; `save_assets` succeeded. `RootBox` has exactly one child (`RootCanvas`), so `RootCanvas` is the sole `Fill` child and gets full height (consistent with prior graphical evidence that the side/top panels render), which makes the bottom anchor valid. **Graphical PIE visibility/clickability verification is still pending** — headless `-nullrhi` cannot capture the viewport, so on-screen rendering and slider drag must be confirmed by the user's fresh graphical PIE.

**Lesson:** (1) `ObjectTools.set_properties` **cannot** set a `CanvasPanelSlot`'s nested `layoutData` sub-object properties via dotted scalar keys (`layoutData.anchors.minimum.x`, `layoutData.offsets.left`, … all report "could not be set"); it **can** set the whole `layoutData` object by passing `"layoutData": {"anchors":{…},"offsets":{…},"alignment":{…}}` in the `values` JSON. That whole-object form is the reliable way to set a `CanvasPanelSlot`'s anchors/offsets/alignment. (2) When an entire UI area is absent even though the leaf widget's own slot is valid, the fault is in the **ancestor chain** — bypass it by `MoveWidget`-ing the content widget directly under a known-good panel (`RootCanvas`) with an explicit slot, instead of repeatedly tweaking the leaf. (3) A `Border`'s single `BorderSlot` child is a fragile ancestor for reliable rendering; a direct `CanvasPanelSlot` child of the root `CanvasPanel` is more robust. (4) `MoveWidget` of a container does not break `EventGraph` bindings on its descendants (bindings reference widgets by name, not by parent).

### 9.49 Timeline STILL invisible after §9.48 — `FMargin` offsets mis-read for a non-stretched `CanvasPanelSlot` (negative height) (RESOLVED — graphical PIE pending)

**Problem:** After §9.48 (commit `512a9e3`), the timeline remained **completely invisible** in graphical PIE, even though `BottomTimelineContent` was now a direct child of `RootCanvas` with an explicit `CanvasPanelSlot_1`. The §9.48 offsets had been described as producing a 900×70 panel, but that arithmetic was wrong.

**Cause (confirmed, not assumed):** For a `CanvasPanelSlot` whose min and max anchors are **identical** (non-stretched), the `FMargin` offsets are **not** edge insets. They mean: `Left` = position X, `Top` = position Y, `Right` = **WIDTH**, `Bottom` = **HEIGHT**. The §9.48 values `left=-450, top=-95, right=450, bottom=-25` therefore assigned the panel a width of 450 and a **negative height of −25** — a widget with negative height renders nothing. That is why bypassing the old parent container did not help: the slot itself was degenerate.

**Fix:** Re-set `CanvasPanelSlot_1`'s whole `layoutData` object via `ObjectTools.set_properties` with the correct non-stretched semantics: anchors min=max=`(0.5, 1.0)`, alignment `(0.5, 1.0)`, offsets `left=0, top=-25, right=900, bottom=70`, `bAutoSize=false`, `zOrder=10`. This yields a 900×70 panel, horizontally centered on the bottom-center anchor and positioned 25 px above the canvas bottom. No second slider, no hierarchy rebuild — the existing `BottomTimelineContent` → `SizeBox_3` → `TimelineSlider` chain and its `OnValueChanged(TimelineSlider) → ScrubTimeline` binding are untouched.

**Verified:** Slot readback returns the exact corrected `layoutData` (offsets `0, -25, 900, 70`; anchors min=max `(0.5,1)`; alignment `(0.5,1)`; `bAutoSize=false`; `zOrder=10`). Visibility chain is clean — `BottomTimelineContent` and `SizeBox_3` are `SelfHitTestInvisible` (rendered, children hit-testable), `TimelineSlider` is `Visible` (clickable); none are `Collapsed`/`Hidden`. `compile_blueprint` clean; 22/22 `BUH` automation tests pass; `save_assets` succeeded. **Limitation (reported, not resolved):** the native `CanvasPanelSlot` getters `GetPosition`/`GetSize`/`GetAutoSize`/`GetAnchors` are non-UFUNCTION C++ virtuals and there is no MCP tool that invokes arbitrary UObject methods (no `ProcessEvent`/call-function in `BlueprintTools` or any toolset), so they cannot be called directly. Verification was done by reading back the stored `layoutData` and applying the documented non-stretched semantics (`Right`=WIDTH=900, `Bottom`=HEIGHT=70, `Left`=X=0, `Top`=Y=−25) rather than inferring dimensions by subtracting offsets. Final on-screen proof still requires the user's fresh graphical PIE.

 **Lesson:** (1) For a **non-stretched** `CanvasPanelSlot` (min=max anchors), `FMargin` offsets are `Left`=X, `Top`=Y, `Right`=WIDTH, `Bottom`=HEIGHT — **not** left/right/top/bottom edge insets. Setting `Bottom` negative gives a negative height → invisible widget. (2) Never compute a slot's dimensions by subtracting `Left` from `Right` (or `Top` from `Bottom`); that subtraction is only valid for a **stretched** slot (min≠max). (3) The native slot getters (`GetPosition`, `GetSize`, …) are not UFUNCTIONs and are not invocable through the MCP toolsets — verify slot geometry by reading back `layoutData` and applying the correct stretched/non-stretched semantics, and treat on-screen rendering as requiring graphical PIE.

### 9.50 Timeline STILL invisible after §9.49 — stop guessing, instrument RootCanvas geometry (DIAGNOSTIC SETUP DONE — graphical PIE interpretation pending)

**Problem:** After §9.49 (commit `fcb577e`), the user's fresh graphical PIE showed **still no slider, track, thumb, or `Instant N / N` readout**. Four prior fixes (§9.46 `SizeBox` override, §9.47 `Fill`→`Auto`, §9.48 reparent to `RootCanvas`, §9.49 corrected positive offsets) all compiled, passed automation, and produced valid saved hierarchies — yet none rendered. `todo.md` therefore banned a fifth speculative layout tweak and ordered a measurement-first diagnosis: inspect the historical `RootCanvas` `Fill=0`, run a controlled A/B placement, add independent visible labels, and log real `GetCachedGeometry()` in graphical PIE.

**Confirmed finding (not assumed):** `RootCanvas`'s `VerticalBoxSlot_0` still carries `size = {value: 0, sizeRule: "Fill"}`. The earlier assumption that a single `Fill` child receives the full parent height **regardless of the Fill coefficient** is **not verified** for the current graphical layout, and the visible top-anchored buttons do **not** prove `RootCanvas` has non-zero height (top-anchored widgets render even inside a zero-height canvas). This is now the prime suspect and is what the on-screen readout must confirm or refute.

**Diagnostic setup (all in `WBP_CombatUI`, slider + `OnValueChanged(TimelineSlider) → ScrubTimeline` binding untouched):**
- **A/B placement:** `BottomTimelineContent`'s `CanvasPanelSlot_1` set to a fixed, unambiguous location — anchors `minimum`=`maximum`=`(0,0)`, alignment `(0,0)`, offsets `left=300, top=400, right=900, bottom=70` (a 900×70 box at screen `(300,400)`), `bAutoSize=false`, `zOrder=100`. This is a **diagnostic** position, not the final layout.
- **Independent labels:** `DiagTimelineTest` `TextBlock` directly under `RootCanvas` (`CanvasPanelSlot_2`, `(300,350)`, 500×50, `zOrder=101`, yellow, `Visible`, opacity 1, text `"TIMELINE LAYOUT TEST"`); `DiagKnownGood` `TextBlock` inside the known-good `RightPanelContent` container (`VerticalBoxSlot_6`, cyan, `Visible`, opacity 1, text `"KNOWN-GOOD CONTAINER"`).
- **On-screen geometry readout:** `DiagGeometryReadout` `TextBlock` directly under `RootCanvas` (`CanvasPanelSlot_3`, `(10,10)`, 700×220, `zOrder=200`, amber, `AutoWrapText`, `Visible`, opacity 1). Exposed as a Blueprint variable (`ToggleWidgetAsVariable`).
- **C++ measurement:** `UBUHCombatUI::GetTimelineGeometryDebug()` (`BlueprintCallable`, category `Diagnostic`) calls `GetWidgetFromName` for `RootBox`, `RootCanvas`, `BottomTimelineContent`, `SizeBox_3`, `TimelineSlider` and reports each one's `GetCachedGeometry()` → `GetAbsolutePosition()`/`GetAbsoluteSize()`, visibility as `int32` (`4`=Visible), and `RenderOpacity`, as a multi-line `FString`. Rebuilt clean with `RunUBT.sh … BeneathUncountedHeavEditor Development Linux -WaitMutex`.
- **Wiring:** `UpdateGeometryDebug` function graph = `CastToBUHCombatUI(self)` → `GetTimelineGeometryDebug` → `SetText(Text)` on the `DiagGeometryReadout` variable. `EventTick` calls it as its final statement (the full pre-existing `EventTick` body — scrub, targeting, move selection, `ExecuteMove`, `ResetEncounter` — was restored intact after `write_graph_dsl` wiped it; see lesson 5).

**Interpretation (todo.md §3 outcome table, to be applied to the user's screenshot):** both labels visible + timeline missing → inspect `BottomTimelineContent`/children; only the known-good-container label visible → `RootCanvas` arrangement/clipping of new direct-child slots; both labels AND the timeline visible at the fixed coords → bottom-anchoring/`RootCanvas`-height problem (then set `VerticalBoxSlot_0` to `Fill Value=1` **only if** zero-height is confirmed); neither label visible → stale WBP class/reference.

**New gotchas (verified this session):**
1. **Anchor JSON keys are `minimum`/`maximum`, not `min`/`max`.** Passing `min`/`max` is silently ignored (offsets/`zOrder` still apply) while the anchors stay at their previous values — the A/B slot first attempt did exactly this and had to be re-issued with `minimum`/`maximum`.
2. **`Widget|SetText` targets `URichTextBlock`, not `UTextBlock`.** Its `self` pin is `Rich Text Block Object Reference`. The `UTextBlock` node is **`Widget|SetText(Text)`** (`self`=`Text Object Reference`, `InText`=`Text`). Using the wrong one fails with "Could not connect pin … to self".
3. **A C++ member function's `self` pin needs an explicit cast.** The DSL `self` (Self Reference) yields the base `UUserWidget`, but `GetTimelineGeometryDebug`'s `self` pin is `BUHCombat UI Object Reference` — a downcast that the type checker rejects. Bridge it with `Utilities|Casting|CastToBUHCombatUI` (data output pin `AsBUHCombat UI` → continuation variable `_asbuhcombat_ui`).
4. **`read_graph_dsl` mis-renders some node type_ids, and `write_graph_dsl` type-checks strictly.** `Class|BPCombatManager|GetState` is rendered as `StateMachine|GetState` (which resolves to the *animation* state machine, `UpdateContext`/`Node` pins) and `Class|BPCombatManager|GetCurrentState` (returns `FBuhCombatState`) as `CustomizableObjectInstance|GetCurrentState`. Re-writing a captured full-body DSL therefore fails with "Could not connect pin Manager to UpdateContext" until those two are substituted back to their real `Class|BPCombatManager|…` type_ids.
5. **`write_graph_dsl` on an event that already has a body WIPEs that body** (contrary to the "attaches … does not wipe the graph" note). Writing `(event UserInterface|EventTick … (CallFunction|UpdateGeometryDebug))` replaced the entire `EventTick` statement list with just the call. Always capture the event's DSL via `read_graph_dsl` first and restore the full body (with the call appended) in one write.

**Status:** WBP compiled clean, saved, 22/22 `BUH` automation tests pass, all five widgets (`DiagTimelineTest`, `DiagKnownGood`, `DiagGeometryReadout`, `BottomTimelineContent`, `TimelineSlider`) confirmed present in the tree. Final cause identification and the real fix still require the user's fresh graphical PIE screenshot of the three on-screen elements + the `DiagGeometryReadout` values.
