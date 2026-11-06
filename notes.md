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
4. **Graphical PIE rendering** — prove the fighters and UI actually draw. Via `run_graphical_visual_test.sh`.

Passing one type of test does NOT prove another type succeeded.

### 6.2 Headless `-nullrhi` limitations (last verified 2026-11-04)

- `CaptureEditorImage` returns a 1×1 black placeholder (no real rendering).
- `ProcessEvent` on Blueprint functions in the **editor world** shows no observable gameplay side effects (e.g. `SetActorLocation`) — but this is an **editor-world context** issue (§9.6), not a `-nullrhi` limitation. In a real PIE game world the same functions take effect.
- Automated button-click → lunge action-execution **cannot be proven** in the headless editor.
- Verify PIE state via actor/property inspection, not screenshots.
- **`StopPIE` can crash the editor** (SIGSEGV observed 2026-10-30). If PIE verification is complete and no further Blueprint save is needed, leave PIE running or kill/relaunch the editor instead of relying on `StopPIE`.

### 6.3 Graphical PIE verification

`run_graphical_visual_test.sh` is the only way to get a real rendered screenshot. See §7 for the VRAM-safe workflow.

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

`run_graphical_visual_test.sh` proves the two fighters actually draw (the headless `-nullrhi` editor renders nothing). It:
1. Waits for Qwen/llama.cpp to idle-sleep (VRAM < 4 GiB).
2. Closes the headless MCP editor.
3. Launches the full graphical editor (`UnrealEditor-xwayland`).
4. Opens `Lvl_TacticalDuel`, starts PIE, captures the game view.
5. Closes the graphical editor.
6. Writes `tmp_opencode/visual_test/<run_id>/result.json`.

**Run it via a BLOCKING bash tool call** (`bash run_graphical_visual_test.sh`), NOT backgrounded with `nohup … &`. The blocking call keeps the session waiting (no inference) so llama.cpp sleeps; when the script exits, the tool call returns its result and the session's next inference wakes the model automatically.

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
