# OpenSew2 BUH 1.0.3 — Qwen Session Findings

Date: 2026-11-19
Blender: 5.3.0 (`/snap/bin/blender`)
Add-on: OpenSew2 BUH 1.0.3 at `~/.config/blender/5.3/scripts/addons/clothing_design/`
MCP bridge: `mcp` extension 1.0.3, graphical Blender, port 9876

## Context

Continuing the wuxia/xianxia cultivator outfit test after the 1.0.2 headless session was lost.
The user installed 1.0.3, restarted Blender graphically so the work could be watched, and asked
that only one garment be sewn at a time.

The 1.0.3 change that pauses outer-garment physics while an inner garment is being sewn is a
major improvement. Sewing a full garment with `frames_per_call=50` completed in a small number of
MCP calls without timing out.

## Garments built

| Garment | Preset | Fit | Result |
|---|---|---|---|
| `Cultivator_Trousers` | `TROUSERS` | 0.12 | 297 welded, 1 open stitch removed; 0 body intersections |
| `Cultivator_Inner_Robe` | `TSHIRT` | 0.10 | 155 welded, 5 open stitches removed; 42 vertices inside body, 135 intersecting triangles, max depth 0.045 m |
| `Cultivator_Outer_Robe` | `JACKET` | 0.15 | 191 welded, 20 open stitches removed; 5 vertices inside body, 71 intersecting triangles, max depth 0.013 m |
| `Cultivator_Sash` | custom rectangle | 0.02 ease | 6 welded, 0 open stitches removed; 0 body intersections |

Materials assigned: white to trousers and inner robe, deep blue to outer robe and sash.

## Issues observed

### 1. `bpy.data.objects.find(ob)` raises `SystemError` under MCP

Calling `bpy.data.objects.find(some_object)` from MCP-executed Python fails with:

```
SystemError: <built-in method find of bpy_prop_collection object at 0x...> returned a result with an exception set
```

Workaround that works:

```python
for i, o in enumerate(bpy.data.objects):
    if o is ob:
        scene.cd.active_garment = i
        break
```

This happened twice (sash setup and outer-robe relax). It is not specific to OpenSew, but it blocks
the natural way to set `scene.cd.active_garment` from scripted code.

### 2. Custom sash panel is hard to get right

The `RECTANGLE` preset places a flat panel at chest height. To make a waist sash I created a custom
rectangle with `patterns.create_panel`, `anchor='TORSO_FRONT'`, `height` at the waist, and
`wrap=1.0`, then added a seam between the left and right edges to close it into a band.

The resulting garment did sit at waist height with zero body intersections, but the 2D pattern
piece (`Cultivator_Sash_Panel`) remained visible in the viewport far away from the body, making it
look like the sash had fallen to the floor. In a watched graphical session this is confusing.

Suggestions:
- Hide or collapse pattern pieces once `build.sync_garment` has produced the garment.
- Add a dedicated `SASH` or `BELT` preset that wraps a narrow band at the waist and closes the
  overlap automatically.
- Make the pattern-piece collection optional or off by default after garment creation.

### 3. Inner robe (TSHIRT) intersects the body

After sewing and settling, the inner robe had 42 vertices inside the body and a max penetration of
4.5 cm. The outer jacket mostly hides this, but it is not clean.

Likely causes:
- The T-shirt preset's armpit/side clearance is too tight for a loose robe silhouette.
- The sleeve panels intersect the torso at the underarm.

Suggestions:
- Increase the default arrangement ease for the T-shirt preset, or expose a per-piece clearance
  override.
- Add a "robe" variant of the top preset with a wider body and longer, looser sleeves.

### 4. `relax_garment_steps` is not safe to drive in large MCP chunks

`sim.relax_garment_steps` returns a generator that advances the cloth solver frame by frame.
Advancing 50 frames in one MCP call was fine. Advancing 100 frames timed out the MCP request and
left Blender unresponsive enough that the user had to close it.

Also, the generator is local to the executing Python scope; a second call to
`relax_garment_steps` starts a new relaxation from the beginning rather than continuing the old one.

Suggestions:
- Provide a `cd.relax` operator that accepts `frames_per_call` and stores its state in the scene,
  mirroring the stepped sewing API.
- Or expose a `relax_garment` operator that runs synchronously but is known to be safe, and keep
  the generator for internal use only.
- Cap or warn when an MCP call tries to advance more than N frames.

### 5. 1.0.3 API differences from 1.0.2

The older helper that called `patterns.create_panel(scene, avatar, name, piece, ease=...)` no
longer matches the 1.0.3 signature. The correct path is now the `cd.add_garment` operator, which
takes `preset`, `fit`, and `name` and handles pattern creation, seams, garment sync, cloth setup,
collider, and layer assignment.

Measurement is also module-level now:

```python
from clothing_design import avatar
m = avatar.measure(bpy.context)
z = avatar.z_of(m, "waist")
girth = avatar.girth_at(m, z)
```

`scene.cd.avatar` is the body `Object`, not a wrapper with methods.

## What worked well

- Sewing one garment at a time with `ops.start_sew` / `ops.step_sew(frames_per_call=50)` was fast
  and stable.
- The outer-physics pause in 1.0.3 made layering much more practical.
- `cd.add_garment` is a clean one-call way to add a preset garment.
- Body-fit reporting via `sim.body_fit` gave concrete intersection numbers.

## Suggested next steps for Codex

1. Add a `SASH`/`BELT` preset and hide pattern pieces after garment sync.
2. Fix or document the `bpy.data.objects.find` issue under MCP, or provide an OpenSew helper that
   sets the active garment by object.
3. Make relaxation stepped and stateful like sewing.
4. Improve the T-shirt/robe preset clearance to avoid underarm intersections.
5. Consider a "Wuxia robe" preset: crossed front, long loose sleeves, knee-length, with a separate
   sash.
