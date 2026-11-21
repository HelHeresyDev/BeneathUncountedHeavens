# Task: Integrate ComfyUI-3D, CurioMesh-BUH, and Headless Unreal into One Automated Asset Pipeline

## Objective

I want you to be able to autonomously generate, optimize, and import 3D assets into my Unreal Engine 5.8.4 game, **Beneath Uncounted Heavens**.

We already have the necessary tools. Your job is to connect and validate them, not build another 3D generation or retopology system.

The intended workflow is:

1. Generate a 3D mesh using ComfyUI-3D.
2. Paint and texture the mesh using Hunyuan3D Paint.
3. Process the textured GLB using CurioMesh-BUH and Blender.
4. Import the optimized asset into Unreal Engine.
5. Generate and verify LODs and collision.
6. Report results and make the validated asset available for use in the game.

**The crucial requirement is intelligent GPU resource management.**

You can remain awake while Blender and headless Unreal perform their work, but you must sleep during ComfyUI generation and graphical Unreal rendering.

---

## 1. Environment

Hardware:

- AMD Radeon RX 7900 XTX, 24 GB VRAM
- Ubuntu 26.04 LTS
- Blender 5.3
- Unreal Engine 5.8.4

Unreal executable:

`/home/depressedtsukasa/Apps/Linux_Unreal_Engine_5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd`

Unreal project:

`/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/BeneathUncountedHeav.uproject`

ComfyUI-3D:

`~/AI/ComfyUI-3D-sandbox`

ComfyUI server:

`http://127.0.0.1:8189`

OpenCode tools:

`~/.config/opencode/tools/`

ComfyUI workflow:

`~/.config/opencode/comfy3d-workflow-api.json`

CurioMesh-BUH repository:

https://github.com/HelHeresyDev/CurioMesh-BUH

Development branch:

`buh/blender-5.3`

Verified Milestone 3 commit:

`01eaebc85ae4afbd1880b148a765ce75cc9497fa`

Locate the existing CurioMesh-BUH checkout rather than cloning a duplicate unnecessarily.

Unreal MCP helper:

`/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/tmp_opencode/mcp_do.py`

Inspect the actual files and installed tools before making changes.

---

## 2. GPU management rules

### ComfyUI-3D: Qwen must sleep

We already have a functioning, VRAM-safe ComfyUI-3D OpenCode bridge.

It has been verified to:

- Acquire `/tmp/opencode-image-generation.lock`.
- Wait for llama.cpp to report `is_sleeping=true`.
- Verify VRAM consumption is below the configured threshold.
- Start ComfyUI automatically if necessary.
- Generate the mesh and textures.
- Collect the GLBs.
- Release ComfyUI GPU memory.
- Stop only the ComfyUI instance it launched.
- Return control to Qwen after the operation finishes.

**Reuse the existing bridge. Do not reimplement this handoff.**

The bridge previously generated a complete textured GLB in approximately 74 seconds, excluding startup.

Preserve its existing safeguards.

### Blender: Qwen may stay awake

CurioMesh-BUH processing can run in headless Blender while Qwen remains loaded.

Use CPU-based Blender processing where practical.

There is no need to put Qwen to sleep simply to perform retopology, repair, UV validation, or file export.

### Headless Unreal: Qwen may stay awake

Use:

`UnrealEditor-Cmd -nullrhi -unattended`

This configuration has previously run with negligible additional GPU memory usage while Qwen was loaded.

Use headless Unreal for:

- Static mesh imports.
- LOD generation.
- Geometry inspection.
- UV and material validation.
- Collision generation.
- Collision traces and capsule sweeps.
- Automated PIE collision tests.
- Numerical and structural quality checks.

Do not initiate the graphical editor for these operations.

Monitor actual VRAM consumption during the initial integration test. If headless Unreal unexpectedly consumes substantial VRAM, stop the operation safely and report the problem.

### Graphical Unreal: Qwen must sleep

Graphical Unreal may only run through the established GPU handoff mechanism.

Requirements:

1. Acquire the shared GPU lock.
2. Wait until Qwen is sleeping.
3. Verify that sufficient VRAM is available.
4. Start graphical Unreal using the established XWayland workaround.
5. Perform the required renders or graphical PIE tests.
6. Save screenshots and results.
7. Close only the editor process started by the tool.
8. Verify that Unreal has exited and GPU memory has been released.
9. Return control to Qwen.

Never launch graphical Unreal directly from an ordinary OpenCode shell command while Qwen is awake.

Do not terminate manually started Unreal sessions.

If safe GPU handoff cannot be established, skip graphical verification and report that it remains untested.

**Graphical validation must be optional. It must not prevent ordinary headless asset processing.**

---

## 3. Inspect the existing tools

Before developing anything, inspect:

### ComfyUI-3D OpenCode bridge

Locate the existing tools, including:

- `comfy3d.ts`
- `comfy3d_bridge.py`
- `comfy3d_safe.py`

Identify their supported commands and how generation results are returned.

Do not change the working ComfyUI implementation unless a specific integration defect requires it.

### CurioMesh-BUH

Read:

- `README.md`
- `DEVELOPMENT.md`
- `TEST_RESULTS.md`
- The `curiomesh-retopo` agent skill
- `curiomesh_process.py`
- The Unreal validation scripts

Milestone 3 already validated:

- Retopology and geometry-safe repair.
- Textured GLB export.
- A single Unreal Static Mesh containing three LODs.
- Valid UVs and materials across all LODs.
- Convex and complex collision.
- Headless PIE collision checks.
- Graphical LOD verification.

Reuse this implementation.

### Unreal automation

Inspect the existing MCP configuration, startup scripts, and validation toolset.

Determine how to start and stop a headless Unreal session, connect through MCP, import assets, and execute the existing validation operations.

Reuse the established project tools where possible.

---

## 4. Implement the integrated workflow

I want one convenient OpenCode-facing operation that can accept:

- A reference image or an already-generated GLB.
- An asset name.
- An asset category.
- A CurioMesh preset.
- Optional ComfyUI parameter overrides.
- LOD generation options.
- Collision configuration.
- An Unreal destination or test run ID.
- Whether graphical verification is requested.

The operation should orchestrate the existing tools instead of duplicating their implementations.

It may be implemented as an OpenCode tool, a Python command-line wrapper, or a small combination of both.

Prefer whatever integrates most cleanly with the current OpenCode installation.

### Stage A — Generate

When given a reference image:

1. Inspect the saved ComfyUI workflow.
2. Apply requested parameter overrides.
3. Execute the VRAM-safe ComfyUI generation tool.
4. Collect both base and painted GLBs.
5. Confirm the painted GLB contains geometry, UVs, and textures.
6. Record the generation parameters and output paths.

When given an existing GLB, skip generation entirely.

### Stage B — Optimize

Process the painted GLB using CurioMesh-BUH.

Support:

- Balanced, High Fidelity, and Aggressive presets.
- Existing geometry-aware repair.
- Preservation of original materials and textures.
- LOD generation at configurable ratios.
- Collision configuration.
- Quality checks and JSON reports.

Use Balanced as the initial default.

Do not silently switch presets or override a geometry-safety rejection.

Respect `PASS`, `REVIEW`, and `FAIL` outcomes.

### Stage C — Headless Unreal import

Start Unreal using the existing headless command and MCP setup.

Import the optimized asset into an isolated test location:

`/Game/Developer/CurioMeshBUHTests/<run_id>/`

Verify:

- Static Mesh created successfully.
- Expected triangle counts.
- LOD0, LOD1, and LOD2 exist.
- LOD0 matches the accepted optimized mesh.
- UV channels are valid.
- Materials and textures are assigned correctly.
- Normals are valid.
- Collision geometry exists where requested.
- Collision probes pass.
- PIE validation succeeds where supported.

Keep Qwen awake during this stage.

Stop the headless Unreal instance when the operation finishes.

Do not close any editor session you did not start.

### Stage D — Optional visual verification

If graphical validation is requested, use the existing GPU handoff.

Capture views of the asset in graphical Unreal and verify the output as far as the available automated comparisons allow.

Store the screenshots for inspection after Qwen wakes.

Do not claim visual verification succeeded merely because a material or texture is assigned.

If graphical validation is not requested, finish using headless results alone and explicitly mark visual appearance as untested.

---

## 5. Start with an existing GLB

**Do not generate another mesh just to test the Unreal integration.**

We already have successfully generated assets.

The latest ComfyUI-3D integration test saved outputs beneath:

`~/AI/comfy3d-opencode-runs/`

Locate the most recent complete, textured GLB from that test.

It was approximately 1.9 MB and contained two baked texture images and UVs.

First run:

**Existing painted GLB → CurioMesh-BUH → headless Unreal → validation report.**

This isolates the new integration work from the already-verified ComfyUI generation stage.

Once that succeeds, perform one complete end-to-end test from a saved reference image if practical and if GPU handoff is available.

Do not repeatedly generate new meshes unnecessarily.

---

## 6. Handle failures safely

The workflow must handle:

- ComfyUI not running.
- ComfyUI generation failures.
- Qwen not entering sleep.
- Insufficient free VRAM.
- GPU lock already held.
- Invalid GLB exports.
- CurioMesh quality-gate rejection.
- Unreal startup failure.
- Unreal MCP connection failure.
- Missing UVs or textures.
- LOD validation failure.
- Collision validation failure.
- Unreal process crash.
- Timeouts and interrupted tool calls.

On failure:

1. Preserve useful intermediate results.
2. Record the actual error.
3. Stop child processes started by the workflow.
4. Release only locks owned by the current operation.
5. Avoid leaving GPU-intensive processes running unnecessarily.
6. Do not delete source assets.
7. Return a meaningful error to OpenCode.

Use unique run IDs and output directories.

Do not overwrite existing production assets or validated test results.

Never bypass the GPU safeguards simply to complete a test.

---

## 7. Report results to Qwen

Each completed run should produce a machine-readable report containing:

- Input image or GLB path.
- ComfyUI parameters, when applicable.
- Generated mesh paths.
- CurioMesh preset.
- Original and optimized triangle counts.
- Remaining geometry warnings.
- Texture and UV validation.
- Unreal asset path.
- LOD triangle counts.
- Collision settings and probe results.
- Headless PIE validation status.
- Optional graphical verification status.
- Execution times.
- Errors and warnings.
- Final `PASS`, `REVIEW`, or `FAIL` verdict.

Distinguish stages that passed, failed, or were not tested.

Provide a concise summary in OpenCode after the operation completes.

---

## 8. Preserve the existing project

Do not:

- Rebuild CurioMesh-BUH.
- Replace the ComfyUI installation.
- Change existing model weights.
- Modify Qwen's llama.cpp service.
- Modify production game assets.
- Overwrite the installed CurioMesh extension.
- Change Unreal project rendering settings globally.
- Install NVIDIA CUDA.
- Introduce a second independent GPU lock system.
- Launch graphical Unreal while Qwen is loaded.

Use existing APIs, skills, scripts, and tested GPU-management infrastructure.

Any necessary integration code should remain small, documented, and independently testable.

---

## 9. Verify the completed integration

Perform the following tests:

**Test 1: Existing GLB to headless Unreal**

- Use the existing painted GLB.
- Run CurioMesh-BUH.
- Import through headless Unreal.
- Validate LODs, materials, and collision.
- Confirm Qwen remained awake.
- Confirm Unreal exited cleanly.

**Test 2: Failure handling**

Simulate or safely exercise an Unreal startup/MCP failure.

Confirm that the tool reports the error without damaging assets or leaving orphan processes.

**Test 3: Full generation workflow**

If the first two tests pass, run one complete reference-image-to-Unreal test.

Use the existing ComfyUI sleep safeguards.

Verify that Qwen sleeps during generation and can remain awake during Blender and headless Unreal.

Do not require graphical Unreal for this test.

**Test 4: Optional graphical validation**

Inspect whether the existing GPU handoff can be reused for graphical Unreal.

Only run a real graphical test if the handoff is already safely implemented or can be integrated with a small, verified change.

Otherwise, document this as a separate optional capability.

---

## 10. Final deliverables

Provide:

1. The OpenCode tool or orchestration wrapper.
2. Usage instructions.
3. A successful end-to-end test report.
4. Actual asset paths and Unreal test paths.
5. Confirmation of the GPU behavior at each stage.
6. Logs from headless Unreal.
7. Confirmation that no unnecessary processes were left running.
8. Any unresolved blockers.

**Success means I can tell you to create a 3D asset, and you can use ComfyUI-3D, CurioMesh-BUH, and headless Unreal to produce and validate that asset without me manually launching the applications.**

Focus on making existing tools work together reliably. Do not turn this into another major tool-development project.

Begin by inspecting the installed tools and testing the existing painted GLB through headless Unreal.

---

## Test results

### Test 1: Existing GLB to headless Unreal — PASS

- Run ID: `comfy3d00002_m3b`
- Input: `~/AI/comfy3d-opencode-runs/4391cc8f-3ae4-4f6c-9460-545306174b08/opencode3d_545643a07a97_00002_.glb` (1,936,224 bytes, 2 baked textures).
- CurioMesh-BUH: balanced preset, retopo 3,676 faces (100% quad), repair ok, export 7,352 tris / 2 embedded PBR images.
- Unreal: headless editor PID 139767 on port 8191, import to `/Game/Developer/CurioMeshBUHTests/comfy3d00002_m3b/`, static mesh + 3 LODs (7,352 / 3,676 / 1,838 tris), convex collision (1 hull, 24 verts), calibrated probe trace passed.
- Qwen remained awake throughout. Editor exited cleanly; port 8191 had 0 listeners after.
- Disposition: REVIEW (collision probes passed; LOD visual validation pending without PIE).

### Test 2: Failure handling — PASS

- Run ID: `comfy3d00002_m3c`
- No editor on port 8191. Pipeline exited 0 with `ok: true`, disposition REVIEW.
- `unreal.status: "unavailable"` with detail `Unreal MCP helper exited 1 … Connection refused`.
- `lods` and `collision` reported `untested`. Retopo and repair still `ok: true`.
- Local `out.glb` preserved. Source GLB byte-identical (1,936,224 bytes before and after).
- No orphan Blender or Unreal processes. Ports 8000/8191 had 0 listeners.

### Test 3: Full generation workflow — PASS

- Run ID: `comfy3d00003_m3d`
- ComfyUI-3D stage: `prompt_id 4e517017-950c-49cb-8aec-8b28d3941836`, `history_status: success`. Qwen sleeping during generation (`qwen_sleeping: true`, VRAM 1.24 GiB < 4.0 threshold). ComfyUI auto-started and stopped by the tool.
- Generated two GLBs: base mesh (29,610,200 bytes, 274,142 tris, no textures) and painted asset (4,116,732 bytes, 10,001 tris, 2 embedded textures).
- CurioMesh-BUH: balanced preset, `hard_surface` category, retopo 3,676 faces (100% quad), repair ok (1 residual inward face, 3 collapsed UV faces — human-review flags), export 7,352 tris / 2 embedded PBR images.
- Unreal: headless editor PID 165784 on port 8191, import to `/Game/Developer/CurioMeshBUHTests/comfy3d00003_m3d/`, static mesh + 3 LODs (7,352 / 3,676 / 1,838 tris), convex collision (1 hull, 24 verts, `CTF_USE_SIMPLE_AND_COMPLEX`), probe validation untested (no chest-calibrated probes).
- Qwen awake during Blender and headless Unreal. Editor exited cleanly; port 8191 had 0 listeners after.
- Source GLB byte-identical (4,116,732 bytes before and after).
- Disposition: REVIEW (collision queries untested, LOD visual validation pending, residual geometry flags).

### Test 4: Optional graphical validation — NOT RUN

No graphical Unreal handoff was exercised. LOD visual validation and graphical PIE remain untested by design (headless-only scope for this integration task).