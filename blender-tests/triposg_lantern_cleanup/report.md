# Stone Lantern Automated Cleanup Report

## Source asset
- File: `/home/depressedtsukasa/Downloads/triposg_painted_00001_.glb`
- Generator: TripoSG + Hunyuan3D Paint via ComfyUI-3D-rocm
- Intended use: environmental stone garden lantern prop for Beneath Uncounted Heavens

## Pipeline overview
1. Imported the GLB into Blender 5.3 through Blender MCP.
2. Inspected topology, materials, textures, scale, and silhouette.
3. Saved an untouched baseline `.blend` and rendered reference views.
4. Created a conservative cleanup version that preserves original geometry and textures.
5. Reconstructed a clean procedural production version because the AI mesh was too distorted to repair efficiently.
6. Added convex-hull collision proxies (`UCX_Lantern_00/01/02`).
7. Exported GLBs and verified them by reimporting.
8. Rendered front, side, rear, three-quarter, and wireframe evidence for all three versions.

## Measured comparison

| Metric | Original | Conservative Cleanup | Best Production Result |
|---|---|---|---|
| Triangles | 40000 | 39382 | 728 |
| Vertices | 27018 | 19697 | 386 |
| Non-manifold edges | 13198 | 25 | 0 |
| Non-manifold verts | 174 | 10 | 0 |
| Zero-area faces | 41 | 0 | 0 |
| Exact duplicate verts | 7025 | 0 | 0 |
| Connected islands | 725 | 1 | 1 |
| Materials / textures | 1 material, 2 embedded textures (base color + ORM-like) | 1 material, 2 embedded textures | 1 procedural stone material, no textures |
| Pivot | Center of mass | Bottom center | Bottom center |
| Height | ~1.87 m | ~1.87 m | 1.80 m |
| Visual fidelity | Distorted, melted pedestal, warped roof, noisy surface | Topology cleaned but silhouette still warped | Clean architectural lantern, recognizable at low poly count |
| Unreal readiness | Poor | Moderate | High, with UCX collision |

## Defects found in the original
- 725 disconnected islands, indicating fragmented generation.
- 13198 non-manifold edges and 174 non-manifold verts.
- 41 zero-area faces and 7025 exact duplicate verts.
- Warped roofline, irregular pillars, and a distorted pedestal.
- Blotchy, inconsistent stone texture.
- No usable UV layout for a clean production material.

## Conservative cleanup (Version A)
- Removed zero-area faces.
- Merged duplicate verts at threshold `0.0001`.
- Recalculated normals.
- Reduced non-manifold edges from 13198 to 25.
- Preserved original textures and most of the generated topology.
- Still visually warped; not suitable as a final production asset without further reconstruction.

## Production reconstruction (Version B)
- Rebuilt the lantern from clean primitives: plinth, step, four pillars, chamber, roof plate, 4-sided roof cone, and finial.
- Applied a subtle bevel, merged duplicates, recalculated normals.
- Scaled to a 1.80 m height and set the pivot to bottom center.
- Generated smart UVs.
- Created a procedural weathered-stone material using noise-driven color and bump.
- Result: 386 verts / 404 faces / 728 triangles, fully manifold, clean silhouette.

## Collision
- Three convex-hull UCX objects approximate the lantern's occupied space:
  - `UCX_Lantern_00`: base and lower pillars (28 verts, 52 faces)
  - `UCX_Lantern_01`: pillars and chamber (20 verts, 36 faces)
  - `UCX_Lantern_02`: roof and finial (101 verts, 198 faces)
- UCX objects are hidden from render and use wire display in the viewport.

## Export and verification
- `triposg_conservative.glb` exports the conservative mesh with its original material.
- `triposg_production.glb` exports the production mesh plus UCX collision objects.
- Both files were reimported into Blender and verified:
  - Conservative reimport: 39382 faces, material preserved.
  - Production reimport: 728 faces, material preserved, UCX hulls present.

## Visual evidence
All renders are in `blender-tests/triposg_lantern_cleanup/`:
- `baseline_render.png`
- `original_front.png`, `original_side_right.png`, `original_rear.png`, `original_three_quarter.png`, `original_wireframe_front.png`
- `conservative_front.png`, `conservative_side_right.png`, `conservative_rear.png`, `conservative_three_quarter.png`, `conservative_wireframe_front.png`
- `production_front.png`, `production_side_right.png`, `production_rear.png`, `production_three_quarter.png`, `production_wireframe_front.png`
- `collision_preview_front.png`

## Reusable scripts
- `scripts/01_import_inspect.py`
- `scripts/02_conservative_cleanup.py`
- `scripts/03_production_reconstruct.py`
- `scripts/04_collision_hulls.py`
- `scripts/05_export_verify.py`
- `scripts/06_render_evidence.py`

## Pipeline automation assessment
Reliably automatable:
- GLB import and scene cleanup.
- Topology inspection and defect counting.
- Zero-area face removal, duplicate merging, normal recalculation.
- Conservative cleanup of mildly defective meshes.
- Convex-hull collision generation from z-slices.
- GLB export and reimport verification.
- Multi-view orthographic rendering.

Requires object-specific reasoning:
- Deciding whether a mesh is repairable or needs reconstruction.
- Rebuilding architectural elements procedurally.
- Choosing appropriate primitive shapes, proportions, and materials.
- Evaluating visual quality and silhouette.

Recommendation:
- Use a two-stage pipeline. Stage 1 runs generic cleanup and inspection. Stage 2 uses an AI agent to inspect renders and decide between conservative repair and procedural reconstruction.
- Different asset categories need different strategies: architectural props benefit from primitive reconstruction; organic statues benefit from remeshing and smoothing; furniture and chests benefit from conservative cleanup plus decimation.

## Conclusion
AI-generated 3D props from TripoSG are useful for quick silhouette references but are not directly Unreal-ready. The conservative cleanup path reduces topology defects but cannot fix warped architecture. Procedural reconstruction produces a far better, optimized asset for architectural props. For Beneath Uncounted Heavens, AI generation should be treated as a concept blockout step, followed by automated cleanup and agent-driven reconstruction or repair.
