# Task: Automated 3D Asset Cleanup — Stone Lantern

We have successfully generated our first textured 3D model using ComfyUI-3D-rocm (TripoSG + Hunyuan3D Paint) on my AMD Radeon RX 7900 XTX.

The object is a traditional East Asian stone garden lantern intended as an environmental prop for **Beneath Uncounted Heavens**, our Unreal Engine 5.8.4 wuxia/xianxia sandbox RPG.

Your task is to independently investigate, clean up, optimize, and prepare this model for eventual use in Unreal Engine.

**The broader objective is to determine whether you can automate the cleanup of AI-generated 3D assets through Blender MCP.**

## Phase 1 — Locate and inspect the original asset

The generated file is named:

`triposg_painted_00001_.glb`

It's in Downloads

Import the GLB into Blender 5.3 using Blender MCP.

Before modifying anything, investigate:

- Actual polygon and vertex counts.
- Mesh topology and connected components.
- Non-manifold edges, holes, and degenerate faces.
- Inverted or inconsistent normals.
- UV mapping and texture quality.
- Material assignments and PBR texture channels.
- Overall symmetry, scale, orientation, and silhouette.
- Whether the mesh has unwanted internal geometry.
- Any intersections between architectural elements.

Create front, side, rear, and three-quarter renders of the original model.

Save an untouched baseline.

## Phase 2 — Identify visual defects

The original reference depicts a carefully constructed stone lantern with:

- A symmetrical, tiered architectural design.
- A tall ornamental finial.
- A broad roof with clean, straight edges.
- An open central chamber supported by stone pillars.
- Clearly defined openings and decorative stonework.
- A stepped pedestal and arched base.
- A consistent weathered-stone appearance.

Our AI-generated model preserves the general silhouette but has several defects:

1. Warped and irregular architectural edges.
2. Poorly defined pillars and decorative details.
3. Asymmetrical geometry that should be symmetrical.
4. A distorted, partially melted-looking pedestal and base.
5. Excessive polygon density for a relatively simple environmental prop.
6. Some inconsistent or blotchy surface textures.

Inspect the model yourself rather than relying entirely on this list.

Determine which defects can be repaired automatically and which require reconstruction.

## Phase 3 — Repair the geometry

Use Blender's Python API through Blender MCP.

You have freedom to investigate suitable techniques, including:

- Mesh cleanup and vertex welding.
- Normal recalculation.
- Limited smoothing.
- Remeshing where appropriate.
- Topology simplification.
- Identifying and removing unwanted internal faces.
- Geometric symmetry correction.
- Rebuilding damaged architectural elements.
- Replacing malformed regions with clean procedural geometry.

**Important: This is architecture, not an organic sculpture.**

Do not use aggressive smoothing that rounds off sharp stone edges.

Do not simply decimate the entire model and declare it repaired.

The lantern should have straight pillars, clean rooflines, recognizable ornamental details, and a structurally coherent base.

If individual components are too badly distorted to repair, you may reconstruct them procedurally while using the original model as a visual reference.

However, distinguish clearly between:

1. Geometry repaired from the AI-generated mesh.
2. Geometry replaced with newly constructed meshes.

If a mostly procedural reconstruction produces a much better result, that is useful information about the limitations of TripoSG.

## Phase 4 — Optimize for Unreal Engine

Prepare an optimized static-mesh version.

Targets:

- Aim for approximately 5,000–10,000 triangles if visual quality permits.
- Preserve important silhouette and architectural details.
- Avoid unnecessary geometry on flat surfaces.
- Remove invalid and unintended overlapping geometry.
- Ensure normals are correct.
- Preserve or rebuild UVs as necessary.
- Keep the pivot at the bottom center.
- Set a plausible real-world scale and document the dimensions.
- Ensure the final model is appropriate for Unreal Engine's coordinate and asset workflows.

Produce at least two versions:

**Version A — Conservative Cleanup**

Preserve as much of TripoSG's original geometry and textures as possible.

**Version B — Best Production Result**

Use any appropriate combination of mesh repair, replacement geometry, procedural reconstruction, and texture improvements to achieve the best practical result.

Version B should prioritize quality over faithfulness to the generated topology.

## Phase 5 — Improve the materials

Inspect the existing Hunyuan3D PBR textures.

Preserve useful information, but correct obvious problems.

Investigate whether Blender can improve the material using:

- Existing generated textures.
- Procedural stone shaders.
- Texture adjustments.
- Appropriate roughness values.
- Subtle stone surface variation.

Avoid unnecessary additional AI generation for this initial experiment.

The final material should resemble weathered carved stone, not plastic or randomly colored rock.

If you replace the generated textures with procedural materials, explain why and ensure the exported asset retains a usable material.

## Phase 6 — Add collision and test export

Create a simple collision representation suitable for a static environmental prop.

Collision should approximate the lantern's occupied space without reproducing every decorative detail.

Export the final asset as GLB for inspection in Blender.

Investigate an appropriate Unreal-compatible export path, but do not import anything into the actual game project yet.

Verify the exported file by reimporting it into a fresh Blender scene.

Confirm that geometry, UVs, materials, dimensions, and orientation survive export.

## Phase 7 — Visual verification

I want actual visual evidence, not just successful Python operations.

Render both versions from identical camera positions and lighting conditions.

Provide:

- Original model.
- Conservative cleanup.
- Best production result.

Capture front, side, rear, and three-quarter views.

Include wireframe screenshots showing topology.

Compare the versions for:

- Architectural accuracy.
- Silhouette quality.
- Geometry cleanliness.
- Texture quality.
- Polygon efficiency.
- Suitability for use in Unreal.

If possible, evaluate whether the model remains recognizable at reduced polygon counts.

Do not describe an obviously defective mesh as successful simply because Blender completed an operation.

## Phase 8 — Save deliverables

Save all experiments under:

`blender-tests/triposg_lantern_cleanup/`

Create:

1. An untouched imported baseline `.blend`.
2. A conservative cleaned-up `.blend`.
3. A best-quality production `.blend`.
4. Exported GLB versions.
5. Before-and-after renders.
6. Wireframe screenshots.
7. Reusable Blender Python cleanup scripts.
8. A Markdown report describing methods, defects, and outcomes.

Use descriptive filenames.

Do not overwrite the original TripoSG file.

Do not modify my Unreal Engine project assets.

Do not modify existing OpenSew, MPFB, or ComfyUI installations.

## Phase 9 — Evaluate pipeline automation

This is especially important.

We eventually want OpenCode to perform the following workflow automatically:

**Generate reference image → TripoSG mesh → Hunyuan3D textures → Blender cleanup → Unreal-ready static mesh**

Determine which operations can be reliably automated and which require object-specific reasoning.

Identify whether a generalized cleanup script could work on:

- Stone lanterns.
- Statues.
- Treasure chests.
- Furniture.
- Rocks and natural formations.
- Weapons and decorative props.

Consider whether different asset categories require different cleanup strategies.

Recommend a practical architecture for an automated 3D asset processing pipeline.

## Final report

Provide a comparison table containing:

| Metric | Original | Conservative Cleanup | Best Result |
|---|---|---|---|
| Triangles | | | |
| Vertices | | | |
| Non-manifold defects | | | |
| Materials / textures | | | |
| Visual fidelity | | | |
| Unreal readiness | | | |

Use measured values wherever possible. Distinguish measured results from qualitative judgments.

Conclude with your assessment of whether AI-generated 3D props are worth integrating into Beneath Uncounted Heavens.

**You have considerable freedom to investigate and experiment independently.**

Do not stop after getting a technically valid mesh. The real goal is a visually convincing, optimized asset.

If the original mesh is too poor to repair efficiently, demonstrate that limitation and investigate reconstruction rather than endlessly applying ineffective mesh operations.