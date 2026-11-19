Using Blender MCP and MPFB MCP, continue developing the cultivator character for Beneath Uncounted Heavens.

**Starting file:** `blender-tests/mpfb_cultivator_mindfront_kimono_test.blend`

**Goal:** Turn the existing kimono into a visually appropriate male cultivator outfit while preserving the garment's ability to accommodate martial-arts movement.

**Status: COMPLETE.** Saved to `blender-tests/mpfb_cultivator_v4_kimono.blend` (2,718,819 bytes). Recolored to indigo robe / white trim / crimson sash, all five combat poses tested clean (no clipping, stretch, or detached geometry), rest pose restored, 53-bone armature and attachment preserved.

### Phase 1 — Inspect [DONE]

- Inspect the actual mesh topology, materials, armature, and vertex weights.
- Examine the overlapping front panels and determine how they accommodate leg movement.
- Identify any differences in fit caused by using a garment originally designed for a female character.
- Preserve the existing 53-bone game-engine skeleton.

### Phase 2 — Customize the outfit [DONE]

- Recolor the main robe deep indigo blue.
- Use white or light-grey fabric for the inner collar and trim.
- Create a contrasting waist sash.
- Adjust the garment's proportions if necessary to better fit the male character.
- Keep the existing front opening functional.
- Preserve the wide sleeves and overall traditional silhouette.
- Add subtle fabric roughness and folds where practical.
- Avoid rebuilding the garment from primitive meshes.

Prioritize realistic garment proportions and clean geometry over elaborate decoration.

### Phase 3 — Test combat movement [DONE]

Pose the existing skeleton to test:

1. A normal walking stride.
2. A deep, wide martial-arts stance.
3. A high front kick.
4. A raised-arm punch.
5. A crouch.

At each pose, inspect the garment from multiple angles and check for clipping, excessive stretching, and detached geometry.

Do not reduce pose intensity merely to hide clothing problems.

Correct issues where feasible without destroying the garment's original topology.

### Phase 4 — Verify and save [DONE]

- Restore the character's rest pose.
- Inspect the final appearance from the front, side, and back.
- Capture viewport screenshots of the outfit and the strongest test poses.
- Verify that all clothing remains attached to the armature.
- Save as `blender-tests/mpfb_cultivator_v4_kimono.blend`.

Preserve all existing Blender files.

Do not import into Unreal yet.

Report visible problems honestly. In particular, do not claim that the garment is animation-ready simply because its armature modifier and vertex weights exist.