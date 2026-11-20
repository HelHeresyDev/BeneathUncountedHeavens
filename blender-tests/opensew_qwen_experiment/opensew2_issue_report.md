# OpenSew-2 Issue Report for Codex

Date: 2026-11-18
Blender: 5.3.0
Add-on path: `/home/depressedtsukasa/.config/blender/5.3/scripts/addons/clothing_design`
Original reference: `/home/depressedtsukasa/AI/opensew-2`

## Goal

Independently create a convincing wuxia/xianxia male cultivator outfit using OpenSew-2 sewing patterns and cloth simulation:

- Crossed-collar robe or tunic
- Long, reasonably loose sleeves
- Waist sash/belt
- Trousers underneath
- Blue-and-white color scheme
- Garment must allow movement without catastrophic deformation

## Summary

The add-on can import the male avatar and draft parametric patterns, but the current fork is not usable end-to-end from a headless/MCP Blender session. Three classes of problems block the workflow:

1. Operators depend on `context.selected_objects`, which is unavailable in LLM-generated MCP code.
2. Simulation operators (`cd.sew`) exceed the MCP request timeout even for a single garment with reduced frame counts.
3. The default presets do not produce a clean xianxia robe; sleeves distort, the front does not cross, and a wrapped sash falls to the floor.

## Blocking API / Context Issues

### 1. `cd.import_character` fails in MCP/background context

`clothing_design/ops.py` line 152, inside `CD_OT_import_character._activate`, accesses:

```python
for o in context.selected_objects:
```

In the Blender MCP `weak_sandbox` / background context this raises:

```
AttributeError: 'Context' object has no attribute 'selected_objects'
```

The import partially succeeds (the avatar mesh is linked and measured), but the operator aborts before finishing activation. A lower-level workaround is possible by calling `bpy.data.libraries.load`, linking the mesh, setting `scn.cd.avatar`, and calling `avatar.measure` directly, but the operator itself is not MCP-safe.

The same `context.selected_objects` dependency appears in:

- `clothing_design/ops.py` line 42 (`_patterns_selected`)
- `clothing_design/ops.py` line 801
- `clothing_design/sim.py` lines 484 and 504

Any operator or simulation step that reads selection will fail in headless/MCP use.

### 2. Factory-settings operator is blocked

`bpy.ops.wm.read_factory_settings(use_empty=True)` is rejected by the MCP `weak_sandbox`:

```
Operator 'bpy.ops.wm.read_factory_settings(use_empty=True)' is not allowed in LLM-generated code:
Resets all user preferences and startup file, use bpy.ops.wm.read_homefile() or
bpy.ops.wm.read_homefile(use_empty=True, use_factory_startup=True) instead
```

Workaround: `bpy.ops.wm.read_homefile(use_empty=True, use_factory_startup=True)`.

### 3. `cd.sew` times out the MCP request

Even after reducing the simulation to `stitch_frames = 12` and `settle_frames = 20` for a single garment, `bpy.ops.cd.sew()` returns:

```
MCP error -32001: Request timed out
```

The user increased the GNOME timeout to 60 seconds and restarted Blender, but the MCP request still times out. The sewing operator runs synchronously and blocks the MCP server for longer than the client allows.

Consequences:

- Only the first garment (trousers) completed sewing before the session had to stop.
- The inner robe, outer robe, and sash were left as unsewn pattern panels.
- No full-outfit drape, relax, or material pass could be completed.

Suggested fixes:

- Make `cd.sew` asynchronous, or split it into small frame-stepping calls that an external client can drive.
- Expose a lower-level Python API such as `sew_garment(garment_name, stitch_frames, settle_frames)` that does not require an operator context.
- Add a `frames_per_call` parameter so MCP can pump the simulation in chunks under the timeout.

## Garment Quality Issues

### Trousers

The `TROUSERS` preset with `fit = 0.12` produced six pattern pieces and twelve seams. After sewing, the trousers drape reasonably over the legs, though they remain baggy and show visible panel seams. This is the most successful preset.

### Inner robe (T-shirt preset, long sleeves)

`TSHIRT` preset with `fit = 0.10` and `sleeve_len = 0.92` was intended as a long-sleeved inner robe. The result repeats the known OpenSew upper-body failure:

- Shoulder and sleeve caps twist and bunch.
- The sleeves do not hang cleanly from the shoulder.
- The garment reads as a distorted T-shirt, not a robe.

The T-shirt preset is not suitable for a xianxia inner robe with long sleeves.

### Outer robe (Jacket preset, open front, knee length)

`JACKET` preset with `fit = 0.15`, `length_to = "knee"`, `sleeve_len = 0.95`, and `closed_front = False` was intended as an open outer robe. The result:

- Front panels do not overlap in a crossed-collar arrangement.
- The collar does not form a clean V or cross.
- Sleeves show the same shoulder/sleeve-cap distortion as the T-shirt.
- The garment remains an open jacket, not a wrapped cultivator robe.

A dedicated `ROBE` preset is needed, with proper front overlap, collar shaping, and sleeve caps that survive sewing.

### Sash / belt

A custom rectangular panel was wrapped around the waist using `TORSO_FRONT` anchor, `wrap = 1.0`, width `waist_girth * 1.08`, and height `0.12 m`. After the failed sewing pass, the sash:

- Does not stay at the waist.
- Falls flat to the floor as a separate trapezoid.
- Is not tied, folded, or shaped into a belt.

A sash preset needs either waist pinning, higher friction/collision, or a different construction (e.g. a closed loop with a knot piece) so it remains in place during simulation.

## Visual Evidence

A front-orthographic viewport screenshot after the trousers sewing pass shows:

- Trousers draped on the legs.
- Inner and outer robe pattern panels still floating open around the torso and arms.
- Two flat sash panels lying on the floor to the right of the character.
- No blue-and-white materials applied.

Screenshot path (viewport capture, not saved to disk): see session log.

## What Worked

- Importing the male avatar via lower-level `bpy.data.libraries.load` + `avatar.measure`.
- Drafting parametric patterns for trousers, T-shirt, jacket, and a custom sash panel.
- Arranging patterns on the body with `build.sync_garment`.
- Setting up cloth and layered collision with `sim.setup_cloth`, `sim.ensure_collider`, `sim.setup_layers`.
- Sewing the trousers preset within a single reduced-frame `cd.sew` call.

## What Did Not Work

- `cd.import_character` operator in MCP context.
- `cd.sew` for upper-body garments within the MCP timeout.
- T-shirt and jacket presets as xianxia robes.
- Custom wrapped sash staying at the waist.
- Applying blue-and-white materials (never reached due to simulation timeouts).

## Recommended Changes for the Next OpenSew-2 Version

1. **Remove `context.selected_objects` dependencies** from `_activate`, `_patterns_selected`, and `sim.py`. Use explicit object arguments or `context.view_layer.objects` so operators work in headless/MCP/background Blender.
2. **Make simulation operators MCP-safe**: either async, or callable in small frame chunks. Add a `frames_per_call` parameter or a lower-level `sew_garment()` Python API.
3. **Add a dedicated `ROBE` preset** for crossed-collar xianxia robes, with proper front overlap, collar shaping, and stable sleeve caps.
4. **Fix upper-body sleeve/shoulder distortion** in T-shirt and jacket presets, or document that they are unsuitable for long sleeves.
5. **Add a stable sash/belt preset** that wraps the waist and remains in place during simulation (pinning, friction, or closed-loop construction).
6. **Expose material/color assignment** so a blue-and-white scheme can be applied without manual node editing.
7. **Document headless/MCP-safe entry points** for avatar import, pattern drafting, sewing, and finalization.
