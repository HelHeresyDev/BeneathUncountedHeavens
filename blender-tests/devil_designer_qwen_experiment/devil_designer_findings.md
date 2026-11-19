# Devil Designer — Independent Evaluation (Qwen via Blender MCP)

Date: 2026-11-19
Blender: 5.3.0 (snap), extension `blender_org.devil_designer` v0.13.0
Avatar: MPFB `HumanService.create_human()` basemesh (neutral phenotype)
Method: 100% headless + graphical Blender MCP `execute_code`. No mouse interaction.

## 1. What Devil Designer is
A pattern-based clothing designer: a 2D flat-pattern editor (Bezier/polygon/ellipse
tools, snapping, live edge lengths), sewing connections between pattern edges, 3D
panel construction placed around an avatar, live XPBD cloth simulation, and
Keep/Reset/Stop controls. The flat pattern is stored as a JSON `Document`
(version 4) in `scene.dd_document`; the avatar is `scene.dd_avatar`.

## 2. Programmatic automation (fully scriptable)
The entire garment pipeline runs from Python with no UI:

1. `from bl_ext.blender_org.mpfb.services import HumanService`
   `basemesh = HumanService.create_human()`
2. `doc = starters.create_tank_starter(measurements, fabric='COTTON', name='Tunic',
   extra_length=0.15, hem_flare=0.05)`
   (measurements dict in cm: chest, waist, hip, torso_length, shoulder_width)
3. `scene.dd_document = doc.dumps()` ; `scene.dd_avatar = basemesh`
4. `arrangement.ensure_volumes(bpy.context, force=True)`  # synchronous body-volume fit
5. `garment.build_panels(bpy.context)` ; `garment.compile_garment(bpy.context)`
6. `for f in range(1, 121): scene.frame_set(f)`  # let cloth drape
7. `garment.keep_drape(bpy.context)`  # refine, weld seams, bake final mesh

Confirmed working end-to-end. Pattern generation is also fully scriptable via
`Session` (`s.add(points)`, `s.sew(edge_a, edge_b)`, `s.sew_chains(...)`,
`arrangement_record(volume, point, version=3)`), so custom garments can be built
without any editor.

### Background-mode gotchas
- `start_playback` / `stop` are no-ops in background (no screen). Advance frames
  manually with `scene.frame_set(f)`.
- Volume fitting must use `ensure_volumes(context, force=True)` (synchronous).
  The default `start_fit` is an async generator-driven job that never completes
  headlessly.
- Extension modules import as `bl_ext.blender_org.devil_designer` /
  `bl_ext.blender_org.mpfb`, not top-level. MCP exec namespace does not inject
  `bpy` — every snippet must `import bpy`.

## 3. Reliability (multi-angle)
- **Sleeveless tunic — WORKS.** Front and back drape cleanly. Right side shows a
  see-through gap where the front/back panels do not fully close at the side
  seams (see `tunic_right.png`).
- **Long sleeves — FAILS.** The `create_long_sleeve_starter` is not a true set-in
  sleeve; it is a tee with flat rectangular sleeve flaps (`sleeve_scale=3.2`,
  `arrangement['flat']=True`). The flaps trap against the arms and never settle
  clear. `keep_drape` is rejected by the body guard:
  `ValueError: 8 cloth points are inside Human, up to 32.2 mm` (after 300 frames).
  Evidence: `longsleeve_trapped.png`.
- **Crossed collar — NO STARTER.** No generator exists; it would require manual
  `Session` pattern construction or neckline modification, which was not attempted.

The body guard in `keep_drape` is a **hard failure** (raises), not a warning —
any drape that leaves cloth inside the body is rejected outright.

## 4. Pipeline assessment for BUH automated character generation
- **Strengths:** simple bodice garments (tank/tee/skirt) generate reliably and
  fully headless from a measurement dict + avatar mesh. Fast to set up.
- **Weaknesses:**
  - No skin-weight / rigging transfer. `keep_drape` bakes a **static** draped
    mesh, not a skinned garment. It will not deform with an animated rig — a
    dealbreaker for BUH's moving tactical fighters.
  - No automatic measurement extraction; measurements are supplied by hand.
  - Complex garments (real sleeves, collars, jackets) have no working starter and
    the flat-panel approach intersects the body.
- **Verdict:** good for rapid static costume drapes and concept art; **not
  sufficient as the sole clothing pipeline** for animated game characters.

## 5. OpenSew-2 comparison
| Aspect | Devil Designer | OpenSew-2 |
|---|---|---|
| Output | Static draped mesh (`keep_drape`) | Skinned, rigged garment that follows the avatar |
| Game-ready for animation | No (no skin weights) | Yes |
| Headless automation | Yes (simple garments) | Yes |
| Complex garments | Fails (sleeves/collars) | Better (real sewn seams, sleeves) |
| Setup speed | Very fast | More involved |

For BUH's animated characters, **OpenSew-2 is the better production fit**.
Devil Designer is a useful complement for quick concept drapes and prototyping.

## Deliverables in this directory
- `devil_designer_tunic.blend` — successful sleeveless tunic drape (primary).
- `tunic_front.png`, `tunic_right.png`, `tunic_back.png` — multi-angle renders.
- `longsleeve_trapped.png` — long-sleeve failure evidence.
- `devil_designer_findings.md` — this report.
