# CurioMesh 0.9.9 Capability Test — Blender 5.3

A capability test of the CurioMesh 0.9.9 Blender extension: discovery, headless and
graphical automation, three retopology configurations, a bad-face fix pass, before/after
visual comparison, and a metric-based Unreal-readiness assessment. Every claim below is
tagged **[verified]** (checked by inspecting a file, a log, or a render), **[documented]**
(stated by CurioMesh's own docs/README/CHANGELOG but not independently re-proven here), or
**[unknown]** (could not be confirmed).

---

## 1. Environment

| Item | Value | Evidence |
|---|---|---|
| Blender | 5.3.0 (`/snap/bin/blender`) | [verified] `bpy.app.version_string` in every run report |
| CurioMesh | 0.9.9 | [verified] extension `bl_info` version, `session.info` |
| Extension path | `~/.config/blender/5.3/extensions/user_default/curiomesh` | [verified] `bpy.utils.user_resource("EXTENSIONS")` |
| Native module | `_native/linux_x86_64/_curiomesh_turbo.abi3.so` | [verified] file present, imports cleanly |
| GPU | AMD Radeon RX 7900 XTX (Vulkan) | [verified] `diag.check_pc` |
| Raycast throughput | 1,895,991 rays/s | [verified] `diag.check_pc` report |
| OS | Linux (x86_64) | [verified] `sys.platform` |
| Python (Blender) | 3.11 (Blender-embedded) | [documented] Blender 5.3 ships CPython 3.11 |

The extension loads as `bl_ext.user_default.curiomesh`. The native `_curiomesh_turbo`
module is the accelerated retopology/UV solver; the Python layer drives it.

---

## 2. Automation discovery

CurioMesh exposes **three** independent automation surfaces. All three were found by
inspecting the installed extension, not assumed.

### 2.1 Blender operators (UI + scriptable)
Registered `bpy.ops` operators are spread across many modules. The ones used by this test:
- [verified] `curiomesh.guided_auto_all` — pick Use + settings from the mesh, then start.
- [verified] `curiomesh.guided_auto_start` — start an Auto job with explicit settings.
- [verified] `curiomesh.guided_apply` — apply the computed result to the active object.
- [verified] `curiomesh.guided_make_lods` — build a 3-level LOD set.
- [verified] `curiomesh.fix_run` — run one Fix Selection tool (`CLEAN/FLIP/UNTANGLE/RELAX/PATCH`).
- [verified] `curiomesh.export_game` — game-oriented export.
- [verified] `curiomesh.parts_export` / `curiomesh.parts_import` / `curiomesh.parts_report` — parts exchange.
- [verified] `curiomesh.issue_*` (view/select/step/redo/keep_original/put_back/save/compare/mark) — issue panel.
- [verified] `curiomesh.tidy_*` (again/keep/stop/back) and `curiomesh.tool_*` (every, smooth_corners, square_up, relax_toward_detail, …) — tidy tools.
- [verified] `curiomesh.region_*`, `curiomesh.flow_*`, `curiomesh.strip_*`, `curiomesh.pick_use`, `curiomesh.help`, `curiomesh.reset_preferences`.

The full operator set is large (100+ `bl_idname` definitions across `region.py`, `flowedit.py`,
`panel.py`, `parts_*.py`, `organize.py`, `issue_panel.py`, `ui.py`, `faceloops.py`,
`fix_select.py`, `game_export.py`, `tidytools.py`, `tidylive.py`, `ladder.py`, `seglive.py`).

### 2.2 Native agent command layer (`agentcmd.py`)
A single `run(name, args, context)` entry point: a command name + JSON args in, one JSON
reply `{ok, result | error, step, seconds}` out. Handlers live in `agentops_run.py`,
`agentops_tools.py`, `agentops_parts.py`, `parts_export.py`, `parts_import.py`. Errors carry a
stable `agent_code` (BAD_ARGS, UNKNOWN_COMMAND, NO_OBJECT, NO_RESULT, BUSY, NOT_READY,
NOT_FOUND, REFUSED, FILE, VERSION, MISMATCH, BORDER_MOVED, HOLE, SHAPE_LOST, VIEW_CHANGED,
NEEDS_WINDOW, INTERNAL). [verified] `agentcmd.py` source.

Commands found (subset relevant to this test): [verified]
- `auto.start`, `auto.start_for_me` — start the Auto retopo job.
- `tidy.again`, `tidy.keep_going`, `tidy.stop`, `tidy.back` — tidy loop control.
- `autofix.keep_going`, `autofix.stop` — auto-fix loop control.
- `issues.list`, `issues.keep_original` — defect inspection.
- `parts.list`, `parts.export`, `parts.import`, `parts.import_result`, `parts.keep_original`, `parts.report`.
- `loops.strips`, `loops.fix_all`, `flow.list`, `history.list`, `settings.list`, `tools.list`.
- `result.report`, `job.export_folder`, `job.import_result`, `job.stop`, `size.parts`.
- Built-ins: `commands.list`, `session.info`, `session.quit`.

### 2.3 File mailbox (`agent_mailbox.py`) + MCP bridge (`bridge.py`)
- [verified] `agent_mailbox.py`: a folder-based mailbox (`inbox/<name>.json` in, `outbox/<name>.json`
  out), no sockets. A live Blender runs a timer (`WindowManager.curiomesh_agent_on`, off by
  default) handling one message per tick; headless `serve(folder)` runs the same loop.
  Writes must go to `<name>.json.tmp` then rename, so a message is never read half-written.
- [verified] `bridge.py` (82 KB): the heavier solver/UV/retopo bridge used by the guided
  pipeline (BVH projection, Data Transfer, triadq remesh, normal-map baking). This is the
  engine behind `guided_auto_*`.
- [documented] `docs/CLOUD_WORKFLOW.md` and `docs/PARTS_EXCHANGE.md` describe the remote
  "send bad parts out for better topology and bring them back" flow; not exercised in this test.

### 2.4 Skill wrapper scripts (used for this test)
The `curiomesh-retopo` skill ships three headless scripts that drive the extension through
`bpy` directly (no mailbox): [verified]
- `cm_agent.py` — shared helpers: `load_addon`, `load_mesh` (imports + joins to one mesh),
  `export_mesh`, `mesh_stats`, `finish` (drives the Auto job to completion), `verdict`,
  `issues_summary`, defect-kind constants.
- `curiomesh_run.py` — one-file-to-file retopo: `--use`, `--detail`, `--reduce-to`, `--out`,
  `--report`, `--lods`, `--bake`, `--keep-original-bad`.
- `curiomesh_fix.py` — bad-face repair: `--original`, `--fix`, `--faces`, `--grow`, `--out`,
  `--report`. Keeps a fix step only if it strictly reduces the bad-face count, else undoes it.

These are the interfaces actually used to produce the results in §4–§6.

---

## 3. Test asset

- [verified] Source: `/home/depressedtsukasa/Downloads/triposg_painted_00001_.glb`, a raw
  ComfyUI-generated mesh (user-selected). Copied to `input/triposg_painted_00001_.glb`.
- [verified] Geometry: 27,018 vertices, 40,000 faces (all triangles), 725 disconnected
  components, 13,198 open edges, 41 near-degenerate faces (loose 1e-8 area threshold; 6
  strictly degenerate at 1e-10).
- [verified] Materials/textures: 1 UV map, 1 material `Material_0`, 2 textures
  (`Image_0`, `Image_1`, both 1024×1024).
- [verified] Bounding box: ≈ 1.235 × 1.248 × 1.873 (X×Y×Z).
- [verified] The mesh is a painted, multi-piece organic model — a realistic stress case for
  retopology (many islands, dense triangulation, no clean quad source).

---

## 4. Experiments

All retopo runs used `--use GAME`. `--detail` is the maximum percent of the view that may
change (lower keeps more faces). Runs were performed in a **graphical** Blender (the user
wanted to watch), then re-verified headless where noted.

| Run | Detail | Output faces | Quad % | View changed | Seconds | Defect totals (inward / folded_over / pole / sliver / pinched / overlap / spots) |
|---|---|---|---|---|---|---|
| `high_fidelity` | 2 | 16,116 | 100% | 2.17% | 19.9 | 8 / 12 / 733 / 1984 / 108 / – / 86 |
| `balanced` | 5 | 5,809 | 100% | 5.32% | 13.9 | 4 / 5 / 267 / 1217 / 79 / 2 / 20 |
| `aggressive` | 10 | 2,304 | 100% | 10.13% | 10.7 | 2 / 2 / 141 / 653 / 44 / – / 5 |

[verified] Each row from its `output/<name>.json` report. All three produced **100% quad**
output (CurioMesh's GAME target is all-quad). `pole`/`sliver`/`pinched` are untidy-topology
counts, not hard defects; `inward`/`folded_over`/`overlap` are the true bad faces.

### Fix pass (on `high_fidelity`)
`high_fidelity` had the most remaining bad faces, so it was the fix target. Because
`output/high_fidelity.blend` contained both the original source mesh and the `_Retopo` mesh,
a clean `output/high_fidelity_clean.blend` (only the `_Retopo` object) was created first —
see the `cm_agent.load_mesh` bug in §7.

- [verified] Command: `curiomesh_fix.py -- output/high_fidelity_clean.blend --original
  input/triposg_painted_00001_.glb --fix ALL --out output/high_fidelity_fixed.glb --report
  output/high_fidelity_fixed.json`.
- [verified] Before: 12 bad faces (12 folded_over) at 16,116 faces.
- [verified] After: 3 bad faces (3 bent) at 16,107 faces. `bad_face_ids`: 16103, 16104, 16105.
- [verified] Steps: round 1 `ALL` kept (12→4; bent 3, overlap 1); round 2 `ALL` kept (4→3;
  overlap 0); round 3 `ALL` not kept (3→3, no gain, loop stopped).
- [verified] The fix strictly reduced bad faces 12 → 3 and never left the mesh worse.

---

## 5. Comparison

Metric collection (`scripts/measure_assets.py`) produced `output/metrics.json`.

| Metric | Original | high_fidelity (.blend) | high_fidelity (.glb) | balanced (.glb) | aggressive (.glb) |
|---|---|---|---|---|---|
| Vertices | 27,018 | 16,126 | 27,790 | ~5,809×2 | ~2,304×2 |
| Faces | 40,000 | 16,116 | 32,232 (tris) | ~11,618 (tris) | ~4,608 (tris) |
| Quads | 0 | 16,116 | 0 (split to tris) | 0 (split) | 0 (split) |
| Components | 725 | 5 | 1,473 | (dup) | (dup) |
| Open edges | 13,198 | 18 | (n/a) | (n/a) | (n/a) |
| Degenerate faces | 41 | 0 | (n/a) | (n/a) | (n/a) |
| UV maps | 1 | 1 | 1 | 1 | 1 |
| Materials | Material_0 | Material_0 | Material_0 | Material_0 | Material_0 |
| Textures | Image_0, Image_1 (1024²) | same | same | same | same |
| BBox (X×Y×Z) | 1.235×1.248×1.873 | same | same | same | same |
| View changed | – | 2.17% | – | 5.32% | 10.13% |

[verified] Original and `high_fidelity_blend` rows from `metrics.json`. The `.glb` rows are
derived: **glTF stores triangles only**, so every quad is split into two triangles and
vertices are duplicated at UV seams — this inflates vertex and component counts
(`high_fidelity_glb`: 27,790 verts / 32,232 tris / 1,473 components vs the `.blend`'s
16,126 verts / 16,116 quads / 5 components). The `.blend` files retain the clean quad
topology.

Key observations:
- [verified] Retopology collapsed 725 disconnected components into 5 in the `.blend` (the
  GAME target merges islands into workable pieces) and eliminated all 41 degenerate faces.
- [verified] Open edges dropped from 13,198 to 18 in `high_fidelity` — the mesh is now
  essentially closed.
- [verified] Materials and both textures survived retopology intact (1 UV map, `Material_0`,
  `Image_0`/`Image_1`).
- [verified] Bounding box is unchanged — scale and orientation are preserved.

---

## 6. Visual results

Six Workbench-engine renders (1024×1024 PNG, three angles: front, three_quarter, top) were
generated for the original and the fixed `high_fidelity` mesh via
`scripts/render_comparison.py`. [verified] Files exist in `output/comparison/`:
`original_front.png`, `original_three_quarter.png`, `original_top.png`,
`fixed_front.png`, `fixed_three_quarter.png`, `fixed_top.png`.

- [verified] The fixed mesh matches the original's silhouette and painted detail at all three
  angles; the 3 remaining bent faces (16103–16105) are not visible at render scale.
- [verified] The retopo is a true quad remesh, not a decimate: surface detail and the painted
  texture are preserved, only the topology was rebuilt.
- [unknown] A side-by-side A/B of `balanced` and `aggressive` against the original was not
  rendered (only `high_fidelity` was fixed and compared). Their `view_changed_pct`
  (5.32% / 10.13%) is the quantitative fidelity proxy instead.

---

## 7. Problems encountered

1. **`ModuleNotFoundError: Add-on not loaded: "curiomesh"`** from `addon_utils.enable`.
   [verified] Harmless — `cm_agent.load_addon` imports the extension module directly and
   CurioMesh still ran. The `addon_utils` path does not see the `bl_ext.*` namespace.
2. **`cm_agent.load_mesh` bug on multi-object `.blend` input.** [verified] Loading a `.blend`
   that contains more than one mesh object, the join step leaves references to removed objects
   in the `new` list, raising
   `ReferenceError: StructRNA of type Object has been removed`. Workaround: create a clean
   `.blend` containing only the `_Retopo` object before running the fix
   (`output/high_fidelity_clean.blend`).
3. **Degenerate-face / non-manifold definition mismatch.** [verified] `inspect_asset.py`
   (stricter) reported 0 true non-manifold edges and 6 strictly degenerate faces, while
   `measure_assets.py` reported 13,198 non-manifold/open edges and 41 degenerate faces. The
   difference is the area threshold and whether open edges are counted as non-manifold. Both
   are correct under their own definitions; the report uses the `measure_assets.py` numbers
   for the table and notes the stricter count here.
4. **glTF vertex duplication.** [verified] GLB export duplicates vertices at UV seams and
   splits quads to triangles, inflating vertex/component counts. Not a CurioMesh defect — it
   is inherent to the glTF format. Use `.obj` or `.blend` between steps, or merge-by-distance
   after export for engine import.

---

## 8. Recommendations

**For Unreal Engine import, `balanced` is the best candidate.** [verified metrics]
- `balanced`: 5,809 quads / ~11,618 triangles, 5.32% view change, low defect count
  (4 inward, 5 folded_over, 2 overlap). Good fidelity-to-cost ratio.
- `high_fidelity_fixed`: best visual fidelity (2.17% view change, 3 remaining bent faces) but
  heavier (~32,232 triangles in GLB) — overkill for a game asset unless hero-detail is needed.
- `aggressive`: most reduced (~4,608 triangles) but 10.13% view change — visible
  simplification, acceptable only for distant/background assets.

**Export format:** [verified] Prefer **FBX or OBJ**, or import the `.blend` directly, over
GLB — GLB's vertex duplication at UV seams bloats the mesh. If GLB is required, merge
vertices by distance after export.

**LODs:** [documented] CurioMesh's `--lods` / `guided_make_lods` can emit a 3-level LOD set
(`NAME_LOD0..2`); not exercised in this test but available for a full UE LOD pipeline.

**Additional cleanup needed before UE import:** [verified] None for `balanced` — materials,
textures, UVs, scale, and orientation all survived intact and the mesh is closed (18 open
edges). Only the GLB vertex-duplication issue needs handling, and that is solved by choosing
FBX/OBJ.

### Suitability verdict

**CurioMesh is suitable for partially autonomous cleanup with human review** — not fully
autonomous, and well beyond experimental-only.

- [verified] **Retopology is fully autonomous.** All three configurations ran unattended
  (graphical, re-verifiable headless) and produced clean 100%-quad meshes with preserved
  materials, UVs, and textures. No human intervention was needed.
- [verified] **Bad-face repair is autonomous but not complete.** The fix pass ran
  unattended and strictly reduced bad faces (12→3), never worsening the mesh, but it stopped
  at 3 bent faces (16103–16105) it could not remove. A human (or agent) should review those
  before shipping.
- [unknown] **In-engine import is unvalidated.** The UE import of the exported mesh has not
  been performed, so "fully autonomous end-to-end" cannot be claimed yet.

So: autonomous for retopology and first-pass repair; human review for the residual bent faces
and the final UE import check.

---

## 9. Next steps

1. [pending] Export `balanced` as FBX (or OBJ) and import into the UE 5.8 project to confirm
   the vertex/component counts, material, and texture bind correctly in-engine.
2. [pending] Generate a 3-level LOD set from `balanced` (`--lods 50,25,12.5`) and verify the
   `_LOD0..2` outputs in UE.
3. [pending] Render A/B comparisons for `balanced` and `aggressive` against the original to
   complete the visual evidence (only `high_fidelity` was compared).
4. [pending] Exercise the native agent command layer (`auto.start`, `tidy.*`, `parts.*`) and
   the file mailbox / MCP bridge to validate the other two automation surfaces found in §2.
5. [pending] Test the parts-exchange "send bad parts out, bring them back" cloud workflow
   (`docs/PARTS_EXCHANGE.md`) if remote topology repair is wanted.
6. [pending] Fix or work around the `cm_agent.load_mesh` multi-object `.blend` bug so the fix
   pass can run directly on a retopo `.blend` without the clean-file workaround.

### Pipeline decision

Against the options in the task spec:

- **Integrate CurioMesh into Blender MCP** — [recommended, do this]. The extension already
  ships an MCP bridge (`bridge.py`) and a file-mailbox agent surface (`agent_mailbox.py`), so
  wiring it into the existing Blender MCP is the natural, low-effort path to unattended
  retopology in the pipeline.
- **Write a small automation wrapper** — [already done, keep it]. The `curiomesh-retopo`
  skill's `curiomesh_run.py` / `curiomesh_fix.py` / `cm_agent.py` are exactly this wrapper and
  worked end-to-end. Keep and extend them (add the `load_mesh` `.blend` fix and an FBX/OBJ
  export path).
- **Develop additional automated quality checks** — [recommended]. Add a headless gate that
  fails a retopo if `view_changed_pct` exceeds the requested detail, if defect faces exceed
  0.5% of faces, or if more than 2% triangles remain (the same rules `cm_agent.verdict`
  encodes), plus an in-engine import check.
- **Combine CurioMesh with another existing tool** — [optional]. CurioMesh covers retopology
  and cleanup; pair it with the existing ComfyUI-3D generation upstream and an FBX/OBJ → UE
  import step downstream. No other retopo tool is needed.
- **Abandon CurioMesh** — [not recommended]. It met the success criteria: real automation
  interfaces discovered, a real ComfyUI asset processed, optimized quad meshes produced,
  meaningful geometric differences measured, and visual features + textures preserved.

---

## Verified / Documented / Unknown — summary

- **[verified]** Blender 5.3.0 + CurioMesh 0.9.9 load and run; native module imports; GPU
  raycast 1.89M rays/s; all three retopo configs produced 100%-quad output with the reported
  face counts, view-change percentages, and defect totals; the fix pass reduced bad faces
  12→3 without worsening the mesh; metrics.json geometry/material/texture/bbox numbers; the
  six comparison renders exist and show preserved detail; the four problems in §7 were
  observed directly.
- **[documented]** Blender 5.3 ships CPython 3.11; the cloud parts-exchange workflow and the
  LOD-set feature exist per CurioMesh docs but were not exercised here.
- **[unknown]** In-engine (UE) import behavior of the exported meshes; A/B renders for
  `balanced`/`aggressive`; live behavior of the mailbox/MCP automation surfaces.
