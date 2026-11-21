# Beneath Uncounted Heavens — Development Notes

**Working reference · reorganized 2026-11-20**

This is the **current, task-oriented reference** for Qwen/OpenCode and collaborators. It replaces a 1,600-line collection of chronological incidents. The complete historical notes remain unchanged at [the dated archive](docs/archive/notes_legacy_2026-11-20.md). Details here are distilled from that archive, current project instructions, and recorded graphical playtests; the archive is the source for exact investigation transcripts, node names, and old fixes.

> **Evidence rule:** Implementation, compilation, headless simulation, and graphical PIE are different levels of proof. Label what was actually verified. A new commit can invalidate a previously verified behavior; do not mistake an old PASS for a current regression test.

## Quick navigation

1. [Documents and priorities](#1-documents-and-priorities)
2. [Current status and outstanding checks](#2-current-status-and-outstanding-checks)
3. [Environment, locations, and execution](#3-environment-locations-and-execution)
4. [Unreal MCP reference](#4-unreal-mcp-reference)
5. [Blueprint and UI editing hazards](#5-blueprint-and-ui-editing-hazards)
6. [Combat state and movement architecture](#6-combat-state-and-movement-architecture)
7. [Predictions, branching, and ghost visualization](#7-predictions-branching-and-ghost-visualization)
8. [Character, clothing, and asset pipeline](#8-character-clothing-and-asset-pipeline)
9. [Verification and safe workflow](#9-verification-and-safe-workflow)
10. [Failure patterns and diagnostic checklist](#10-failure-patterns-and-diagnostic-checklist)
11. [Historical lookup index](#11-historical-lookup-index)
12. [How to maintain these notes](#12-how-to-maintain-these-notes)

---

## 1. Documents and priorities

| Source | Responsibility |
|---|---|
| `AGENTS.md` | Always-in-context rules, paths, MCP access, verification and commit policy; **read in full after compaction** |
| `designbible.md` | Creative canon and game-design decisions; do not silently change them |
| `todo.md` | Current assignment and acceptance criteria; may temporarily describe a narrower task |
| **`notes.md` (this file)** | Current operational knowledge, implementation invariants, limitations, and where to look |
| [`docs/archive/notes_legacy_2026-11-20.md`](docs/archive/notes_legacy_2026-11-20.md) | All 80 original detailed incident reports, preserved verbatim |
| `tests/blueprint_exports/` and test reports | Graph readbacks, machine-checkable evidence, and test-specific detail |

**Precedence:** a fresh inspected implementation and successful current run outrank a historical note. User-observed graphical PIE failures are real failures; the user's graphical tests use fresh launches. Never explain a reproduced bug as “stale PIE” without new evidence.

**Scope:** This file is not a running chat transcript, not a comprehensive game-design bible, and not an excuse to rewrite established systems. Keep major new features in the appropriate design/implementation documents.

## 2. Current status and outstanding checks

### 2.1 Tactical duel — verified capability, not a promise of perfection

- Two-fighter, third-person tactical duel prototype in `/Game/BUH/Prototypes/TacticalDuel/Lvl_TacticalDuel`, with deterministic combat stepping in **Instants**.
- Quick, Heavy, Guard, Dodge, Walk (player-chosen straight-line destination), Dash, Jump, and a reaction-only Continue have been developed.
- **Graphical PIE observations reported by the user (Nov 2026):** ordinary Walk, Dash and Jump targeting/preview; smooth forward/backward timeline scrubbing; cyan uninterrupted-movement ghost; separate gold hypothetical-reaction ghost for Quick with Walk, Dash and Jump; airborne Jump ghost height; reaction-to-Heavy Continue preview.
- **Prediction design:** a hypothetical attack **does not replace the cyan baseline**. It computes a separate branch and annotates the original movement with a fixed reaction ghost. This is intentional and supersedes earlier “truncate the main timeline” suggestions.
- During an *actual* reaction, Continue previews the already-committed opponent attack up to the next opponent decision boundary. This is different from planning a hypothetical attack.
- Progress was observed on specific earlier commits; after modifying Blueprints or C++, repeat the relevant graphical flow. Headless test counts in the archive range from 20s upward and are **historical**, not a guarantee about the latest checkout.

### 2.2 Active focus — characters and animation

Current work has shifted to **humanoid appearance, rigging, animation, garment fitting, and Blender-to-Unreal integration**. Source investigations cover Blender 5.3, MPFB/MakeHuman, OpenSew2, Devil Designer and CurioMesh-BUH.

- MPFB cultivator versions and four animated Actions were authored and inspected in Blender; a recolored `mindfront_kimono` garment was pose-tested (see `8).
- **Do not report the MPFB v4 character as already imported into Unreal:** the original character notes explicitly say it had **not** been imported at that stage.
- OpenSew2 makes scriptable, riggable garment geometry, but **cross-front outer-robe layering, sash intersections and combat-pose clipping remain unresolved**. A clean fitting-pose render is not proof of animation readiness.
- CurioMesh-BUH static mesh repair, LOD, UV/material, collision, and Unreal import tests were performed on lantern/chest/stump examples. These are **static-asset** results, not proof of humanoid retopology or clothing success.

### 2.3 Known limitations / guardrails

- **UI polish:** crowded top HUD, translucent/poorly legible text, and unreliable bottom timeline backdrop. Cosmetic; do not destabilize a working timeline merely to fix a Border.
- **Opponent controls in reaction state:** distinguish committed action from hypothetical choices. Buttons may be rendered/dimmed outside planning; appearance alone does not prove disabled hit testing.
- **Continue legality:** only a genuine resumable movement reaction may Continue. A pre-execution Walk/Dash/Jump selection is not resumable. `BP_CombatManager.ExecuteMove` has a guard; keep it and test the real button path.
- **Headless input:** `-nullrhi` cannot prove real mouse visibility, cursor capture, hover, line drawing or animated deformation.
- **Character production:** no blanket “game-ready humanoid” claim until deformation, materials, skeletal animation, asset import and a graphical Unreal review are separately verified.
- Look at `todo.md` for the latest task; do not assume the historical unresolved list describes present priorities.

## 3. Environment, locations, and execution

| Item | Value |
|---|---|
| OS / GPU | Ubuntu Linux, AMD Radeon RX 7900 XTX; avoid CUDA-only instructions |
| Unreal engine | `/home/depressedtsukasa/Apps/Linux_Unreal_Engine_5.8.4` |
| Project root | `/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/` |
| Uproject | `BeneathUncountedHeav.uproject` at project root |
| Main module | `Source/BeneathUncountedHeav/` |
| Unreal MCP server | `http://127.0.0.1:8000/mcp`, only while editor has `-ModelContextProtocolStartServer` |
| MCP helper | `tmp_opencode/mcp_do.py` |
| Combat level | `/Game/BUH/Prototypes/TacticalDuel/Lvl_TacticalDuel` |
| Blender experiment files | `blender-tests/` |

**Graphical editor:** use `Engine/Binaries/Linux/UnrealEditor-xwayland`. It forces X11/XWayland to avoid the historical native-Wayland freeze on editor close. Do not rename the real `UnrealEditor` binary; Unreal receipts expect that name.

**Headless editor:** `UnrealEditor-Cmd -nullrhi -unattended -nosplash -ModelContextProtocolStartServer`. Its additional GPU memory use was negligible in recorded tests, so it may coexist with Qwen/llama.cpp. Rebuild C++ with UBT, then **restart the editor** to load new code.

**C++ build target:** `BeneathUncountedHeavEditor`, Linux Development. LSP warnings in UE test sources can be false positives; trust an actual UBT build.

**Visual test script:** `tests/run_graphical_visual_test.sh`. Run as a **blocking** shell tool call, not a background `nohup` job; it allows llama.cpp to idle-sleep for the graphics run. Results under `tmp_opencode/visual_test/<run_id>/result.json`. A nested `opencode run` is not a valid wake-up mechanism. The next original inference/request wakes the sleeping model. This script primarily proves the initial visible scene, **not arbitrary UMG clicking**.

**Port conflict:** before killing a PID on port 8000, identify it with `lsof -ti:8000` and confirm it is a stale Unreal/MCP process. Do not indiscriminately kill services.

## 4. Unreal MCP reference

### 4.1 Exact toolset names

| Purpose | Exact toolset name |
|---|---|
| Editor app / PIE / capture | `EditorToolset.EditorAppToolset` |
| Logs | `EditorToolset.LogsToolset` |
| Assets/save | `editor_toolset.toolsets.asset.AssetTools` |
| Actors | `editor_toolset.toolsets.actor.ActorTools` |
| Blueprints | `editor_toolset.toolsets.blueprint.BlueprintTools` |
| Materials | `editor_toolset.toolsets.material.MaterialTools` |
| Material instances | `editor_toolset.toolsets.material_instance.MaterialInstanceTools` |
| Object properties | `editor_toolset.toolsets.object.ObjectTools` |
| Scene | `editor_toolset.toolsets.scene.SceneTools` |
| UMG | `UMGToolSet.UMGToolSet` |
| Automation | `AutomationTestToolset.AutomationTestToolset` |
| Slate | `SlateInspectorToolset.SlateInspectorToolset` |

Use the top-level `list_toolsets` / `describe_toolset` to inspect schemas when uncertain. For `mcp_do.py`, the JSON args file is **only the tool's arguments object**, not the enclosing `toolset_name/tool_name/arguments` wrapper.

Examples:

```json
{"options":{"bSimulate":false,"playMode":"PlayMode_InViewPort","warmupSeconds":3}}
```

```json
{"asset_paths":["/Game/BUH/Prototypes/TacticalDuel/Blueprints/BP_CombatManager"]}
```

**PIE:** `StartPIE` requires `options`; `StopPIE` is empty args. Save assets after stopping PIE. `StopPIE` has historically crashed on Linux in some sessions: collect evidence, and prefer a controlled editor restart when necessary rather than repeatedly invoking a known crash path.

**Automation:** `DiscoverTests` → `ListTests` (needs `nameFilter` and `tagFilter`) → `RunTests` (needs `testNames`) → `GetTestResults`. Use explicitly named tests when a broad filter hangs or loses the MCP connection.

### 4.2 Tool argument traps

- `BlueprintTools.compile_blueprint`: `blueprint` should be the **object path** (`Package.AssetName`); a bare package path can fail. `read_graph_dsl`/`write_graph_dsl` use `graph` (EdGraph refPath), plus `code` for writes.
- `AssetTools.save_assets`: `asset_paths` are **package paths**, no `.uasset` extension.
- `ObjectTools.get_properties`: `instance` plus `properties` array. `set_properties`: `instance` plus `values` as a **JSON string**, not a dictionary.
- `ObjectTools.list_properties`: `instance`; verify exact case and property types before reflection. `BP_CombatManager` exposes fields such as `state`, `playerMove`, `playerX`, `bHasCurrentState` (historically lowercase), with integer positions in the Blueprint.
- `SceneTools.find_actors` filters by instance name/label and may miss existing actors. Query broadly or verify a known actor via `ActorTools.get_actor_transform`.
- `UMGToolSet.GetWidgets`: `widgetBlueprint` (not `widget_blueprint`). Widget event binding uses `UMGToolSet.BindToEventProperty` for `OnClicked`, not ordinary Blueprint component-bound event creation.
- `MaterialTools.get_expressions` and `recompile`: `material_or_function`. `connect_to_output` uses `expression`, `output_name`, `material_property`. `MaterialInstanceTools.list_parameters` expects `material`.
- `LogsToolset.GetLogEntries` requires `pattern`, with optional `category` and `maxEntries`.
- `SceneTools.remove_from_scene` **deletes** actors; never use deletion to hide a prediction ghost.

**Reflection safety:** validate `FStructProperty::Struct` against `FBuhCombatState::StaticStruct()` before reading. Copy structured state with C++ copy assignment, **not `FMemory::Memcpy`**, because `FBuhCombatState` contains a `TArray`. For actor bool visibility, prefer `AActor::IsHidden()` instead of interpreting a raw reflected bool byte.

## 5. Blueprint and UI editing hazards

### 5.1 Edit strategy: surgical before wholesale

1. Inspect the **saved** Blueprint and relevant graph; look at connected nodes with `find_nodes` / `get_node_infos`.
2. For a small fix, use `create_node`, `break_pins` and `connect_pins`. Keep existing execution branches intact.
3. Compile, inspect **warnings as well as errors**, save, re-read relevant graph nodes, then test behavior.
4. **Do not rewrite an entire EventTick via `write_graph_dsl` just to add one call.** Earlier rewrites wiped sibling EventTick flows (targeting, keyboard, scrubbing); some versions of `read_graph_dsl` also omit side execution branches.
5. DSL readback is not round-trippable. It canonicalizes node forms and literals. Do not treat differing output text as proof of corruption.

### 5.2 Specific DSL traps

- `(fn Name () ...)` requires the empty parameter list. In a function expression, own Blueprint calls are `CallFunction|Name`; cross-Blueprint calls use `Class|BPName|Member`.
- `StateMachine|GetState` can bind the wrong `UpdateContext` pin. Use `Class|BPCombatManager|GetState`.
- `read_graph_dsl` may output `(bind _x 6)`; `write_graph_dsl` can reject such literal binds. Inline the value into its consuming expression.
- Same-class `b`-prefixed bool setters are often impossible to recreate from DSL; preserve nodes and insert around them. Boolean setters can read back in an uncreatable form.
- **Impure node pruning:** `BlueprintCallable` with an unconnected exec input gets pruned and returns a default value. Pure getters used inside expressions should be `BlueprintPure` when genuinely side-effect free.
- Cross-class function/variable node names returned by `get_node_infos` are not always valid for `create_node`. Use `find_node_types` + `get_node_type_pins` to obtain a creatable ID and real pin indices.
- `create_node` uses `pos` (not `position`). A created `Actor|GetAllActorsWithTag` must actually execute before consuming its `OutActors` output.
- A timed-out graph write may leave partial edits. Verify on disk rather than assuming rollback.
- A newly added UMG button may need native `BindToEventProperty` before its `OnClicked` can be edited.

### 5.3 Input, cursor, and rendering

- Walk/Dash/Jump targeting switches to `SetInputModeGameAndUI`. Set `bHideCursorDuringCapture=false` **and explicitly `SetShowMouseCursor(true)`** on the real player controller. The prior Walk/Dash and Jump disappearing-cursor bugs came from these settings.
- Ensure the click that opens a movement submenu is not also interpreted as the ground-confirmation click during `EventTick`; ground confirmation must be a subsequent click.
- `BP_CombatManager.TargetingAction`: `-1` idle, `6` Walk, `8` Dash, `9` Jump. Reset it on BeginPlay/Reset and on cancellation. Targeting path only draws in **planning state 0**.
- Only one owner advances Jump playback: `BP_CombatManager.EventTick` calls `TickJumpPlayback`. WBP EventTick must not advance the same playback a second time.
- `WBP_CombatUI` is parented to `UUserWidget`, not `UBUHCombatUI`. Editing the unrelated C++ subclass will not fix the widget's Blueprint timeline.
- UMG `CanvasPanelSlot` with coincident anchors interprets Right/Bottom offsets as **size**, not far-edge margins. A right-anchored child with zero width can paint text but be unclickable.
- The working timeline hierarchy uses a **direct child of `RootCanvas`** with a fixed, non-stretched slot; wrapping it in the historical `BottomTimeline` Border caused clipping. Beware Zero-Fill VerticalBox ancestry and negative/non-stretched slot dimensions.
- Ghost locations need the **PIE world context** (e.g. widget `self`), not `GEngine->GetWorldContexts()[0]`. Visibility and text must be updated **after** generating preview data.

## 6. Combat state and movement architecture

### 6.1 Action IDs and state machine

| ID | Action | Notes |
|---:|---|---|
| 0 | Quick | Attack |
| 1 | Heavy | Attack |
| 2 | Guard | Defense |
| 3 | Dodge | Defense |
| 4 | Walk Toward | Earlier relative-direction implementation |
| 5 | Walk Away | Earlier relative-direction implementation |
| 6 | Walk | **Current destination-based straight-line action** |
| 7 | Continue | **Reaction-only**, resumes existing movement |
| 8 | Dash | Destination movement; early reaction lockout |
| 9 | Jump | Destination leap, vertical trajectory and delayed reaction eligibility |

`BP_CombatManager` state values: **0 Planning, 1 Resolving/playback, 2 Done, 3 Reaction opportunity**. Never confuse state 1 with a reaction (an old bug).

Important struct members: `FBuhCombatState.Fighters` (array), each fighter's `Action`, `Phase`, `PhaseInstants`, `bCommitted`, movement targets, `Height` and `ReactedThreatFighter`. Preserve the complete frozen state for Continue, not a reconstructed tuple of X/health/phase.

### 6.2 Movement and decision invariants

- Walk is **one user-selected straight-line segment**; a non-straight route requires multiple separately committed Walk actions. Walk/Dash/Jump are separate actions from attacks.
- A movement reaction occurs when the defender is legally reactable and notices an opponent attack start. Walk is reactable during eligible movement; Dash and Jump have early locked periods.
- **Continue must be legal**: the manager must be in a genuine player reaction and the authoritative fighter must have resumable movement. The C++ `IsContinueLegal` helper checks the fighter's movement phase; manager reaction flags additionally matter.
- An empty `CurrentState` or `Action=-1` is **not** a valid active movement. Reject bad transitions before `ExecuteMove`; guard fighter array sizes and action-index lookups.
- A reaction pause retains the current position/height, the original destination, action phases and reaction bookkeeping. A response can replace movement; Continue does not restart it.
- **Do not infer reality from a preview:** authoritative combat execution changes `CurrentState` and real fighter actors; scrubbed previews must never do so.
- Walk path at reaction is drawn from the authoritative paused fighter to its original target using `UpdateWalkPath`; this avoids the earlier unconditional-clear regression.
- Jump landing distance is clamped (prototype max **480 uu**), and the visual marker, prediction, and actual commit must agree.

### 6.3 Important source locations

- `Source/BeneathUncountedHeav/CombatSim.h/.cpp` — authoritative stepping, resolver, previews, ghost-moving utilities, Jump playback.
- `Source/BeneathUncountedHeav/TimelinePreviewAutomation.cpp` — timeline, branch, reaction, and ghost tests.
- `Source/BeneathUncountedHeav/PathRendererAutomation.cpp` — path, Dash/Jump playback and deformation-adjacent movement tests.
- `Content/BUH/Prototypes/TacticalDuel/Blueprints/BP_CombatManager.uasset` — current authoritative combat state and action/targeting lifecycle.
- `Content/BUH/Prototypes/TacticalDuel/Blueprints/WBP_CombatUI.uasset` — button handlers, regeneration, scrubbing, visibility and text.
- `Content/BUH/Prototypes/TacticalDuel/Lvl_TacticalDuel.umap` — level actors/tags, including prediction ghosts.

## 7. Predictions, branching, and ghost visualization

### 7.1 Two distinct preview modes

**Planning (state 0):**
1. Select Walk/Dash/Jump + destination; build a baseline through `GenerateAndStoreBaseTimeline`.
2. Scrub to the chosen baseline Instant.
3. Select hypothetical Quick/Heavy (or Guard/Dodge where meaningful).
4. `GenerateBranchFromBase` computes a **separate** branch, without replacing the main baseline. The UI keeps the original scrub range.
5. If the branch contains a legal reaction opportunity, the fixed gold ghost is placed at `GetReactionSnapshot()`. No legal reaction means no invented gold ghost; report the actual reason or consequence.

**Actual reaction (state 3):**
1. The opponent has **already committed** an attack. It must not be replaced by another hypothetical action at the same Instant.
2. A legal Continue generates `GenerateReactionPreviewFromManager` / `GenerateAndStoreReactionPreview` from the complete, frozen `CurrentState`.
3. Simulate the committed attack and continuing movement until KO or the opponent's **next decision boundary**, rather than assuming indefinite opponent idleness.
4. Do not show an unrelated planning-branch reaction marker here. The planned branch and the real committed threat have different meanings.

### 7.2 Timeline storage and ghost ownership

- `GBaseTimeline` + parallel full-state snapshots: uninterrupted planning movement.
- `GCurrentTimeline` + `GCurrentTimelineStates`: currently displayed main preview.
- `GBranchTimeline`, `GBranchTimelineStates` and `GBranchInstant`: hypothetical opponent branch data.
- `GReactionSnapshot` / `bGHasReactionSnapshot`: independent hypothetical reaction marker.
- `GhostPlayer` (cyan): follows baseline player position/height as slider scrubs.
- `GhostOpponent` (orange): shows baseline/branch opponent position as applicable.
- `GhostReaction` (gold): **fixed** at predicted reaction location/height regardless of later baseline scrubbing. Reuse the existing actor; do not spawn one each Tick.
- `DestPlayer`: target marker, not the predicted player.
- `MoveGhostsToInstant(WorldContextObject, Instant)` uses the supplied PIE world and snapshots. Main ghost moves with scrub; gold marker updates from the stored reaction snapshot, independently.

**UI ordering:** update selected action → generate/branch preview → set slider limits and selected Instant → move ghosts and update snapshot/distance/action text. Refresh-before-regenerate historically left stale labels and positions.

**Baseline invalidation:** regenerate when the selected action, movement destination or authoritative starting state changes. **Do not regenerate on every scrub**, and do not overwrite the baseline while exploring a hypothetical branch.

**Temporal display:** UI currently contains historical `6f`/`13f` labels; the game-design term is **Instants**, not frames. Polish separately rather than changing logic during a fix.

## 8. Character, clothing, and asset pipeline

These findings are current **experiments**, not a finished production character pipeline. Preserve Blender source files, generation scripts, pose samples, render views and failure counts; avoid grading a model solely by a successful export.

### 8.1 Blender 5.3 + MPFB / MakeHuman

**Saved experiments** (see `blender-tests/`):
- `mpfb_cultivator_v1.blend` and v2: earlier character baselines.
- `mpfb_cultivator_v3_animated.blend`: four in-place Actions, `Idle` (frames 1–48), `Walk` (1–24), `QuickStrike` (left-arm jab), `CombatStance`; pose-tested and neutral loop seams checked.
- `mpfb_cultivator_donitz_robe_test.blend` / `mpfb_cultivator_mindfront_kimono_test.blend`: comparative garment movement tests.
- `mpfb_cultivator_v4_kimono.blend`: indigo robe, white trim and crimson sash, 53-bone `Human.rig`, material separation and five deformation poses. **Blender-verified; original notes say not imported to UE.**

**Rigging gotchas:**
- MPFB thigh forward/back is **local X**, not Z; pelvis vertical translation is **local Y**, not Z. Probe the specific bone's real axes before keying.
- On the tested rig, a left-arm jab was more controllable than a right-arm straight punch. Verify from front **and side**.
- For a seamless loop, pose at **frame_start** must match the last frame (not frame 0 when the Action starts at frame 1).
- Blender 5.3 layered Actions do not expose the older `Action.fcurves` API. Check Action ranges/non-emptiness and sample pose bones at specific frames.
- `donitz_monk_robe` is a **closed tube** that clips badly on leg raises. `mindfront_kimono` has a front crossover that opens during a kick and was the better animated starting garment.
- MPFB bridge is **not auto-started**; after Blender restart re-enable its command server on `127.0.0.1:9877` as documented in archive `9.66.
- MPFB clothing tests without shipped `.mhw` weights use interpolated weights. Test the result in motion.

### 8.2 OpenSew2 — currently the stronger riggable garment experiment

Relevant fork: [OpenSew2 BUH, Blender 5.3](https://github.com/HelHeresyDev/opensew-2/tree/buh/blender-5.3), with [BUH guide](https://github.com/HelHeresyDev/opensew-2/blob/buh/blender-5.3/clothing_design/BUH_GUIDE.md).

**Observed version history:** `OpenSew2-BUH-1.0.1.zip` through `OpenSew2-BUH-1.0.6.zip` under `blender-tests/`. Do not mistake evidence from v1.0.2 for guaranteed behavior on another installed version.

- Graphical Sew/Drape/Settle/Relax support progress and cancel; headless Blender MCP should use **resumable steps** rather than one long synchronous call.
- Key APIs: `ops.start_sew(context, garment_name)`, `ops.step_sew(context, frames_per_call=1)`, `ops.start_relax` / `ops.step_relax`, `ops.simulation_status()`, and `ops.cancel_relax()`. `build.activate_garment` selects a target.
- `build.draft_garment` (1.0.5) accepts structured named panels/pieces, edges, seams and clearance; `sim.body_fit`, `sim.layer_fit(details=True)`, `sim.pose_fit` provide diagnostics. Inspect the installed addon schema/guide before calling them.
- Layered performance: temporarily pause unrelated outer physics; **finalize inner layers before sewing the next outer**. Large `frames_per_call` can block Blender UI for tens of seconds.
- A garment can pass rest-pose intersections and still badly clip in an overhead reach or kick. Pose-fit testing is mandatory before calling it animation-ready.
- Tested 4-layer cultivator outfit: trousers, inner robe, sash, outer robe. A `TSHIRT` outer preset with broad sleeves looked more intentional than the `JACKET` preset, but the **sash crossing and layered fit still fail**.
- v1.0.6 improved Relax `take_in=0` handling and directional layer-pair triangle counts. A cropped sleeveless draft cleared measured layers in one fitting pose but had visibly distorted armholes; no production-ready crossed-front robe has been validated.
- Detailed results: `blender-tests/opensew_qwen_experiment/`, `blender-tests/opensew2-validation/`, including `opensew2_buh_1.0.5_running_report.md`. Comparable fit tests must start from the **same saved pose/state**, because Relax results varied across scene histories.

**Prioritized unsolved garment work:** dedicated crossed-front robe pattern; independent body/sleeve/hem ease; sash-over/under layering; combat pose clipping at shoulders, armpits and hips; reliable deterministic Relax and efficient MCP stepping.

### 8.3 Devil Designer — useful static concept tool, not proven skinned clothing

Tested `blender_org.devil_designer` v0.13.0 through headless Blender scripting. Procedure uses `HumanService.create_human`, starter/panel creation, synchronous `arrangement.ensure_volumes(..., force=True)`, manual `scene.frame_set(f)`, and `garment.keep_drape`. Its tested `keep_drape` produces **static draped geometry without rig weights**, so it cannot replace riggable OpenSew2 clothing for animated fighters.

No successful crossed-collar starter. The long-sleeve test trapped cloth against arms and `keep_drape` rejected the scene due to body penetration; the sleeveless tunic had a side-seam gap. Deliverables in `blender-tests/devil_designer_qwen_experiment/`. Background `start_playback`/`stop` do not advance frames; Blender MCP Python snippets must explicitly `import bpy`.

### 8.4 CurioMesh-BUH / static prop validation

References: [CurioMesh-BUH TEST_RESULTS](https://github.com/HelHeresyDev/CurioMesh-BUH/blob/buh/blender-5.3/TEST_RESULTS.md), plus `blender-tests/triposg_lantern_cleanup/`.

- A procedural stone-lantern reconstruction reduced a broken original to a clean static mesh and collision hulls; this is not an automated humanoid workflow.
- CurioMesh “Balanced” geometry repair passed the more conservative preservation checks; “High Fidelity” introduced boundaries and was rejected. Even Balanced retained some non-manifold/bent face/UV issues requiring review.
- Imported textured lantern GLB in UE with conventional LODs (recorded render triangles **11,612 / 5,806 / 2,902**) and checked forced vs automatic LOD selection in graphical PIE.
- **Material, geometry and collision are separate acceptance gates.** FBX import was previously untextured while GLB preserved textures; a convex hull blocked a small opening, while LOD0 triangle collision passed opening probes. A passing vertex/UV count does not prove visual paint or traversable apertures.
- For the validation helper, `--out` must be inside `$RUN/exports/` with file name `<run_id>.glb`. Unreal resolves only the bare file name against `CURIOMESH_VALIDATION_EXPORTS`; placing `out.glb` in the parent directory causes a failed Unreal stage even after successful Blender export.
- Blender 5.3 scripting traps: use `bpy.ops.mesh.remove_doubles` (not `mesh.merge`) for threshold merging; `cam.data.type` for camera projection; `BLENDER_EEVEE` in this build; viewport `display_type='WIRE'` does not affect actual rendered wireframe.

### 8.5 Minimum character-to-Unreal acceptance gates

1. **Art:** full-body and face renders from front, side, rear, three-quarter; recognizable proportions and style.
2. **Topology:** mesh checks plus deforming elbow, shoulder, hip, knee, sleeves and hem.
3. **Rig/animation:** Idle and Walk loop seam; QuickStrike and a reaction/stance; sample keyframes and motion extrema, not just Action names.
4. **Clothing:** body and layer intersections in rest **and combat poses**; note qualitative visual failures even when numeric counts are zero.
5. **Export:** source `.blend` preserved; skeleton, skin weights, transforms, material/texture references and animation tracks validated in export.
6. **UE import:** inspect the actual Skeletal Mesh, Skeleton, AnimSequences, material appearance and playback in graphical Unreal. Static-mesh import success is not equivalent.

## 9. Verification and safe workflow

**Always inspect current files and the latest task before changes.**

| Layer | What it proves | What it cannot prove |
|---|---|---|
| Source / saved Blueprint readback | Structure, properties, pin connections | Runtime click behavior |
| UBT build / BP compile | C++/BP compile with warnings inspected | Correct physics, cursor, or visual quality |
| C++ automation in editor | Deterministic resolver and utility behavior | Live UMG / graphical scene |
| Headless MCP/PIE property inspection | Accessible runtime or editor-world state | Rendering and mouse interactions |
| Graphical PIE / Blender preview | Actual pixels, animation and user interaction | All edge cases unless exercised |

**Headless caveat:** UE automation can tear down PIE when tests start, leaving them executing against the **editor world**. Editor-world Blueprint `ProcessEvent` may not produce normal gameplay variable writes. Do not label an invocation-only test as proof of live PIE behavior.

**Suggested change loop:**

1. Read `AGENTS.md`, current `todo.md`, and relevant sections here. Identify the smallest possible edit and expected observation.
2. Inspect source, Blueprint graph, runtime evidence/logs; write down a reproducible failing condition.
3. Make a narrow edit, avoiding broad graph rewrites.
4. UBT build or Blueprint compile; inspect warnings, read back actual nodes; save after stopping PIE.
5. Run relevant deterministic tests and check that they assert outcomes rather than merely successful invocation.
6. For cursor/ghosts/UI movement/materials, do fresh **graphical PIE**. For clothing, render multiple views and deform the rig.
7. Clearly state **verified / inferred / still unverified**. Update `todo.md`, add a compact lesson here only if reusable, and then commit/push **verified intended changes** per `AGENTS.md`.
8. Do not push secrets, temporary logs, or unverified WIP simply to get a green check.

## 10. Failure patterns and diagnostic checklist

| Symptom | First checks | Historical references |
|---|---|---|
| Walk click hides cursor / no path | `SetInputModeGameAndUI` capture flag, explicit cursor show; `TargetingAction==6` after click; same-click cancel; WBP EventTick ground hit and drawing | Archive `9.23–9.24, `9.30, `9.40 |
| No Continue / Continue crashes | Reaction state **3**, `bReactionOpportunity`, fighter action/phase, `IsContinueLegal`, `CurrentState.Fighters.Num()`; manager execution guard | `9.17, `9.42–9.43, `9.64 |
| No preview, frozen slider | Whether any timeline was generated, correct state mode, preview generation order, slider max after data | `9.54–9.59 |
| Ghost exists but invisible | Visibility gates, `MoveGhostsToInstant` PIE context, actor tags, material, actual generation vs refresh | `9.12–9.15, `9.59, `9.61–9.63 |
| Gold ghost missing | Planning vs actual reaction; branch legality, `bHasReactionSnapshot()`, `GhostReaction` tag, `ClearReactionSnapshot()`, renderer update | `9.62–9.63 |
| Hypothetical branch shortens original | Current vs base vs branch storage; verify main timeline remains baseline; do not overwrite with combined branch | `9.62–9.63 |
| Wrong Dash path colors | Use authoritative `CurrentState` elapsed instants; pending Dash flag, not stale Blueprint countdown | `9.25–9.28 |
| Jump freezes / double moves | One Tick playback owner; valid `JumpTarget` in prestate; preserve Height and original trajectory | `9.31–9.39 |
| UMG text visible but button unclickable | Canvas slot width/anchors; input mode and capture; widget visibility vs render opacity | `9.15–9.16, `9.64 |
| Slider missing or too small | RootCanvas hierarchy; direct fixed CanvasPanelSlot; real rendered geometry, not headless Slate zeroes | `9.46–9.53 |
| UE memory crash after reflection copy | Struct type validation and proper deep copy of `TArray`, never raw memcpy | `9.57–9.58 |
| Test passes, PIE fails | Test may not exercise live game world, mouse or visual output; check automated assertions | `9.43, `9.63–9.64 |
| Outfit fine at rest, clips in motion | Test overhead reach, high kick, split/stance; measure both body and layer intersections; examine side views | `9.65–9.76 |
| Static asset looks gray / collision blocks hole | Actual GLB material binding / render UVs; disable Nanite for manual LOD test; separate collision probes | `9.78–9.80 |

When unsure, read the cited archive report rather than trying a failed workaround again.

## 11. Historical lookup index

The [full archive](docs/archive/notes_legacy_2026-11-20.md) retains **`9.1–`9.80**, including contradictory intermediate fixes and detailed logs. Use the *latest relevant* entry, not the first occurrence.

| Archive entries | Subject |
|---|---|
| `9.1–9.11 | Early prototype/PIE/editor UI, colors, duplicate meshes, camera and MCP integration |
| `9.12–9.18 | Ghost visibility, opponent button hit testing, cursor, Continue eligibility, CurrentState lifecycle |
| `9.19–9.24 | Path renderer, target clearing, EventTick DSL pitfalls, Walk/Dash targeting |
| `9.25–9.28 | Dash reaction colors, elapsed-phase state source, pure function compiler pruning |
| `9.29–9.40 | Jump, body-height/path persistence, playback, 480-uu clamp, stale targeting initialization |
| `9.41–9.53 | Timeline UI, Canvas layout investigations, first verified Walk scrubbing |
| `9.54–9.64 | Base/branch preview, Continue, authoritative state, memcpy crash, decision boundary, gold reaction ghost, invalid Continue |
| `9.65–9.67 | MPFB character actions, robe comparison, kimono recolor and deformation |
| `9.68–9.76 | OpenSew2 versions 1.0.1–1.0.6, headless sewing, Relax, layered fit, Devil Designer |
| `9.77–9.80 | Lantern cleanup, CurioMesh-BUH repair, UE textured LODs/collision, exports-path fix |

## 12. How to maintain these notes

**Use stable topical sections, not an ever-growing `9.81, 9.82, …` incident chain.**

For every reusable discovery:
- Update the appropriate existing section (and its current status). Add no more than a concise **symptom → root cause → durable rule → verification**.
- Keep one canonical description of each system. Replace superseded assumptions; do not append conflicting advice.
- Put lengthy experiments, logs, benchmark tables and images into task-specific reports under `tests/`, `blender-tests/` or `docs/` and link them here.
- Distinguish **Blender** vs **Unreal** validation, **C++** vs **Blueprint** validation, and **automatic** vs **user graphical** testing.
- If an old incident is needed in full, link the dated archive and its section number instead of repeating it.
- Review section 2 when the focus changes (combat → character pipeline → world generation), and prune completed/unreliable claims.
- Do not remove the historical archive; if another full reorganization becomes necessary, create a new dated archive first.

*This is an operational reference. The design bible remains the authority on gameplay and lore.*
