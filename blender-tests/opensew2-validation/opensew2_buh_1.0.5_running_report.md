# OpenSew2 BUH 1.0.5 — Running Report

Live log of fit tests run with the freshly installed OpenSew2 BUH 1.0.5.
Scene under test: `Theo-T-Shirt.blend` (Theo rig `CD_Man_Rig` + avatar `CD_Man_Body` + sewn `CD_Garment_T-Shirt`).

## Environment
- Blender 5.3.0 (build date 2026-11-10), launched graphically on `Theo-T-Shirt.blend`.
- OpenSew2 BUH 1.0.5 loaded; confirmed present: `build.draft_garment`, `sim.body_fit`, `sim.layer_fit`, `sim.pose_fit`.
- `pose_fit` = `frame_set` + `body_fit` per frame, so it is for keyframed animation. Static poses are measured with `body_fit` directly.

## Scene state on load
- Frame range 1–250, no actions, no NLA tracks, rig at rest (A-pose, 0 posed bones).
- The overhead-reach and forward-kick poses were transient in the prior session and are NOT stored in the file. They must be reconstructed.
- Garment on load: SUBSURF + COLLISION only, no armature modifier, no shape keys, 4 panel vertex groups. Sewn/settled but unbound.
- Rest-pose `body_fit`: 0 vertices inside, 0 intersecting triangles (matches the reported "fitting pose" result).

## Finalize / bind
- `bpy.ops.cd.finalize()` → "shape frozen; 4 stray holes closed; thickness 1.5 mm; bound to CD_Man_Rig".
- Garment now has `CD_Armature` + 52 bone weight groups (plus SUBSURF, SOLIDIFY thickness, COLLISION).

## Pose reconstruction method
- Bones aimed with a target-direction helper: `q = (0,1,0).rotation_difference(M^-1 @ desired_world_dir)`, applied as the pose bone's quaternion. Reproducible.

## Combat deformation tests
| Pose | Hand/foot check | vertices_inside | intersecting_triangles | max_depth_m |
|---|---|---|---|---|
| Rest (fitting) | hands z=1.27, head z=1.643 | 0 | 0 | 0.0 |
| Overhead reach v1 (arms straight up, hands z=1.802) | hands directly overhead | 113 | 838 | 0.0251 |
| Overhead reach v2 (natural reach, hands z=1.862) | hands x=±0.132 above head | 151 | 703 | 0.0230 |
| Raised-arm forward/high kick (left arm up, right leg up) | foot z=1.474 | 266 | 956 | 0.0251 |

Note: my reconstructions are more extreme than the prior session's poses, so 703/956 are not directly comparable to the previously reported 108/169 — but both confirm the T-shirt digs into the body under combat motion. The overhead reach concentrates intersections at the armpits (sleeve pulled into the torso); the kick concentrates them at the hip/crotch (lower fabric pulled up by the raised leg).

## Fit analysis
- The T-shirt was drafted with `preset_tshirt(fit=0.14)` — a close fit (14% over body measurement). At rest there are 0 intersections, but the armhole and body have no slack, so any combat motion drives fabric into the body.
- `sim.relax_garment` takes the garment IN (tighter) — wrong direction for reducing combat intersections.
- The correct fix is draft-time ease: re-draft with a higher `fit` so the armhole/body carry slack for motion.

## Fit work — re-draft at fit=0.25
- Deleted the fit=0.14 garment + pattern pieces; `bpy.ops.cd.add_garment(preset='TSHIRT', fit=0.25)` → 3400 verts, 3163 faces, 166 springs; `cd.finalize()` re-bound it to `CD_Man_Rig` (56 weight groups).
- Re-measured the same combat poses on the looser garment:

| Pose | fit=0.14 (old) | fit=0.25 (new) | intersecting_triangles |
|---|---|---|---|
| Overhead reach v2 (natural reach) | 703 | **299** | 299 |
| High kick (foot z=1.474) | 956 | **881** | 881 |
| Forward kick (foot forward, z≈1.46) | — | **593** | 593 |

- The overhead reach dropped sharply (703→299): extra armhole/body ease stops the sleeve digging into the torso.
- The kick improved only modestly (956→881): a T-shirt hem sits at the hip, so any leg raise pulls the lower fabric up into the hip/crotch. This is largely inherent to a T-shirt, not fixable by armhole ease alone.
- Pose-aiming caveat: this Mixamo rig's bone local +Y is rolled off the head-to-tail axis, so aiming +Y at a world direction does not always produce that head-to-tail direction (the forward-kick foot ended up above the knee). Knee/foot positions were verified from world matrices, not assumed.

## Next
- Decide with the user: push `fit` higher for the kick, accept fit=0.25, or move on to outer-robe layering (`sim.layer_fit` on the cultivator scene).

## Outer-robe layering (cultivator scene)
- Scene: `cultivator_opensew2_final.blend` (avatar `CD_Man_Body`, 4 draped garments: Trousers L1, Inner_Robe L2, Sash L3, Outer_Robe L4).
- Baseline `layer_fit` on the original outer robe (TSHIRT fit 0.12, ease 0.05): Trousers 92, Inner_Robe 258, **Sash 1195** intersecting triangles. The sash crossing is the dominant failure.
- Bounding boxes (original): outer robe front Y=-0.238 vs sash front Y=-0.175 — the outer robe billows forward past the sash.
- Re-drafted outer robe at **fit 0.08** (tighter body), `arrangement_ease 0.05`, sewn (150 welded), relaxed 150 frames.
- New `layer_fit`: Trousers **0**, Inner_Robe **272**, Sash **857**.
  - Sash crossing improved 1195→857 (−28%). Trousers cleared (92→0). Inner_Robe slightly worse (258→272).
- Render: `blender-tests/opensew2-validation/cultivator_fit008.png` — outer robe is tighter but still billows at the front; the sash still crosses.
- Re-drafted again at **fit 0.05** (tighter), sewn (147 welded), relaxed 150 frames.
- New `layer_fit`: Trousers **3**, Inner_Robe **241**, Sash **612**.
  - Sash crossing improved further: 1195→857 (fit 0.08)→**612** (fit 0.05), a 49% reduction from the original.
  - Inner_Robe improved: 258→272 (fit 0.08)→**241** (fit 0.05), now better than the original.
  - Trousers: 92→0 (fit 0.08)→**3** (fit 0.05), negligible.
- Render: `blender-tests/opensew2-validation/cultivator_fit005.png` — outer robe is tighter; the sash still crosses but less severely.
- The TSHIRT preset's single `fit` value affects body and sleeves together. Fit 0.05 tightens the body substantially, but the front hem still drapes over the sash. A crossed-front or shorter outer layer (per 1.0.4 findings) would be the structural fix.

## Outer-robe layering — continued (fresh scene, jacket test)
- Reloaded `cultivator_opensew2_final.blend` fresh. Re-drafted outer robe at **fit 0.05** (TSHIRT), sewn (149 welded), relaxed 150 frames.
- `layer_fit`: Trousers **0**, Inner_Robe **255**, Sash **912**.
- **Non-determinism finding**: the same fit 0.05 gave Sash **612** on a pre-relaxed scene but **912** on a fresh scene. The relaxation path depends on the initial state; results are not reproducible across scene reloads.
- Tried **JACKET** preset at fit 0.05: Sash **2380**, Trousers 37 — far worse than TSHIRT. The jacket's front panels and collar create more intersection surface with the sash. Reverted to TSHIRT.
- Render: `blender-tests/opensew2-validation/cultivator_fit005_fresh.png` — outer robe still billows at the front; sash still crosses.

## Outer-robe layering — fit 0.08 on fresh scene
- Re-drafted outer robe at **fit 0.08** (TSHIRT) on the fresh cultivator scene, sewn (159 welded), relaxed 150 frames.
- `layer_fit`: Trousers **15**, Inner_Robe **279**, Sash **633**.
- Fit 0.08 is better than fit 0.05 on the fresh scene (Sash 912) and comparable to the pre-relaxed fit 0.08 result (Sash 857). The sash is still crossed, but the outer robe's front hem sits slightly higher.
- Render: `blender-tests/opensew2-validation/cultivator_fit008_fresh.png` — outer robe still billows at the front; sash still crosses, but less severely than fit 0.05.

## Improvement areas for Codex (OpenSew2 BUH fork)
1. **Relaxation non-determinism**: `start_relax` + `step_relax` produce different settled states for the same garment/fit depending on scene history. A seeded or canonical relaxation (fixed initial pose, fixed solver seed) would make `layer_fit` results reproducible.
2. **No dedicated outer-robe preset**: the TSHIRT preset is a poor proxy for a xianxia outer robe — its single `fit` value couples body and sleeve ease, and its front hem drapes over a waist sash. A `ROBE` or `CROSS_FRONT` preset with independent body/sleeve/hem parameters would be the structural fix.
3. **`layer_fit` does not report direction**: it counts intersecting triangles but does not say which garment is intruding. A per-layer "who-is-inside-whom" report would make diagnosis faster.
4. **Relax throughput under MCP**: ~1 frame/sec, 150-frame relax takes many 15-second MCP calls and frequently times out. A batched `relax_until_settled` operator (with an internal convergence check) would eliminate the manual step loop.
5. **`cd.finalize` vs draped-state mismatch**: `finalize` bakes and binds to the rig, removing the CLOTH modifier. But `layer_fit` and `relax` expect draped (CLOTH-modifier) garments. There is no "finalize but keep CLOTH" mode, so a finalized garment cannot be re-measured with `layer_fit`.
6. **Garment naming after `add_garment`**: the new garment is named `CD_Garment_<Preset>` (e.g. `CD_Garment_T-Shirt`), not the intended name. Renaming requires updating `cd.garment`, `cd.garment_name`, and the pattern-piece names separately. A `name` parameter on `add_garment` would avoid this.
