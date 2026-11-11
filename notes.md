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
- `CombatSim.h/.cpp` implements the deterministic resolver. `CombatSimAutomation.cpp` contains `FBuhCombatSimTest`, `FBuhCombatFlowTest`, `FBuhWalkPreviewTest`. `CombatManagerIntegrationAutomation.cpp` contains `FBuhCombatManagerIntegrationTest`.
- All four automation tests pass: `BUH.WalkPreviewTests`, `BUH.CombatSimTests`, `BUH.CombatFlowTests`, `BUH.CombatManagerIntegrationTests`.

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

**Still requires manual graphical PIE confirmation:** the fighter actually traveling along the parabola to the chosen landing point, and the mouse cursor remaining visible during Jump targeting. Headless `-nullrhi` PIE cannot click UMG buttons or capture the path.

**Lesson:** (1) A `BlueprintCallable` UFUNCTION that only calls `ResolveCombat` is not a complete execution helper — if the Blueprint needs to pass live variables (walk target, jump target), it must call the `ExecuteMoveWithVars` overload that reads them. (2) Every targeting button handler that switches to GameAndUI input mode must explicitly call `SetShowMouseCursor true`; the input-mode call alone does not guarantee the cursor is visible. (3) Stale exec targets must be cleared after every execution so a later Walk/Dash/Jump does not accidentally reuse an old destination.
