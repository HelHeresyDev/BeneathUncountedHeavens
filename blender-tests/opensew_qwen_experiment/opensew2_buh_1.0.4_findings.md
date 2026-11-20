# OpenSew2 BUH 1.0.4 — Qwen Session Findings

Date: 2026-11-19
Blender: 5.3.0 (`/snap/bin/blender`)
Add-on: OpenSew2 BUH 1.0.4 at `~/.config/blender/5.3/scripts/addons/clothing_design/`
MCP bridge: `mcp` extension, graphical Blender, port 9876

## What 1.0.4 fixed from the 1.0.3 report

- **Relax resumes across MCP calls.** `ops.start_relax` queues a job in the scene; `ops.step_relax`
  continues it on later requests. Cancellation restores outer-layer physics. Verified by relaxing
  the outer robe across many 15-second MCP calls.
- **Sash visibility.** The new two-piece `SASH` preset stays at the waist and closes properly.
  `bpy.context.scene.cd.show_patterns = False` hides the flat 2D pattern pieces while keeping the
  fitted garment visible.
- **Garment selection.** `build.activate_garment(context, name_or_object)` works and avoids the
  `bpy.data.objects.find` `SystemError` that blocked 1.0.3.
- **T-shirt clipping.** `cd.add_garment(preset='TSHIRT', fit=0.10, arrangement_ease=0.05)` produced
  zero body intersections on the cultivator avatar, both alone and over sewn trousers.

## Current cultivator outfit state (1.0.4)

| Garment | Preset | Fit / ease | Sew result | Body fit after sew |
|---|---|---|---|---|
| `Cultivator_Trousers` | `TROUSERS` | fit 0.12 | 297 welded, 1 open removed | 0 vertices inside, 0 triangles, 0.0 m |
| `Cultivator_Inner_Robe` | `TSHIRT` | fit 0.10, ease 0.05 | 160 welded, 0 open removed | 0 vertices inside, 0 triangles, 0.0 m |
| `Cultivator_Sash` | `SASH` | fit 0.08 | 12 welded, 0 open removed | 0 vertices inside, 0 triangles, 0.0 m |
| `Cultivator_Outer_Robe` | `TSHIRT` (final, redrafted from `JACKET`) | fit 0.12, ease 0.05 | 147 welded, 15 open removed | 12 vertices inside, 55 triangles, 0.023 m after relax |

The `JACKET` preset produced bat-wing sleeves and persistent underarm intersections at every
scale tried (fit 0.15, 0.12). The `TSHIRT` preset at fit 0.14 initially gave zero intersections
but later redrafts at the same fit produced winged sleeves and ~9–17 body intersections. The
final `TSHIRT` at fit 0.12 has wide, flowing sleeves that read as intentional xianxia sleeves,
with 12 vertices inside / 55 intersecting triangles / max depth 0.023 m after a `take_in=0.0`
relax. The sash is hidden under the outer tunic in this configuration.

## Remaining issues

### 1. Outer robe sleeve silhouette and body clipping

The `JACKET` preset produced bat-wing sleeves and underarm intersections at fit 0.15 and 0.12.
The `TSHIRT` preset is more stable but its sleeve pattern is inherently wide; at fit 0.14 it
sometimes gives clean sleeves and sometimes winged sleeves with body intersections. The final
outer robe uses `TSHIRT` at fit 0.12, which leaves 12 vertices inside / 55 intersecting
triangles / max depth 0.023 m after relax. The wide sleeves read as intentional flowing
xianxia sleeves, but the underarm clipping is still present.

The `TSHIRT` outer tunic also hides the waist sash. A crossed-front robe or a shorter outer
layer would reveal the sash.

Suggestions for Codex:
- Add a dedicated long-sleeve robe preset with a crossed front and wider sleeve clearance.
- Expose per-piece arrangement ease so the sleeve caps can be pushed clear of the torso without
  loosening the body panels.
- Consider a "shoulder pin" that remains active through the settle phase when a torso layer is
  present, so the sleeve caps seat without penetrating the body.
- Add a shorter outer-robe variant or an open-front robe so the sash remains visible.

### 2. No `remove_garment` operator

Deleting a garment to redraft it requires manually removing the garment object, its pattern
pieces, and any seams whose `obj_a`/`obj_b` became `None` after the objects were deleted.
`scn.cd.seams.remove()` takes an index, not a seam object, so cleanup must collect indices first.

A `cd.remove_garment` operator that deletes the garment, its pattern pieces, its seams, and its
collider in one call would make redrafting much safer from MCP.

### 3. Relax is slow under MCP and blocks the UI

A 150-frame relax (60 relax + 90 settle) on the outer robe took many 15-second MCP calls to
complete. The job does resume correctly, but the throughput is roughly one frame per second under
MCP on this machine. A `time_budget_s` of 15 seconds advances about 14–16 frames.

Blender is unresponsive until the requested `frames_per_call` batch completes. Even with
`time_budget_s` set, the main thread stays blocked for the whole batch, so the viewport freezes
and the user cannot orbit or inspect the drape while a 50-frame step is running. This may be
contributing to the earlier timeouts and to the need to close Blender.

Suggestions:
- Default MCP-driven sewing/relaxing to a smaller `frames_per_call` (e.g. 10 or 1) so the UI
  stays responsive between frames.
- Make the time budget a hard yield point: process at most N frames, then return control to
  Blender's event loop even if the frame count has not been reached.
- Expose a progress callback or status property that the UI can poll without blocking.

## Workflow notes for 1.0.4

- Hide 2D patterns before watching the drape:
  `bpy.context.scene.cd.show_patterns = False`.
- Activate a garment by name before operations that act on the active garment:
  `build.activate_garment(bpy.context, "Cultivator_Outer_Robe")`.
- Sew inner garments first so they become fitted colliders for outer layers.
- Use `ops.start_relax` / `ops.step_relax` with `time_budget_s=15.0` for MCP-driven relaxation.
- Check `sim.body_fit` after every sew and relax, and inspect the viewport before finalizing.
- Use `frames_per_call=10` for relax steps to keep the Blender UI responsive; 50-frame batches
  freeze the viewport for the whole batch.
- The final outfit is saved at `cultivator_opensew2_final.blend` in this directory, with
  front/right/back viewport screenshots captured in the same session.
