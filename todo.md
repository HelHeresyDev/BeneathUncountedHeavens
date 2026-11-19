Using Blender MCP and MPFB MCP, inspect the installed MakeHuman clothing assets for Beneath Uncounted Heavens.

Start from `blender-tests/mpfb_cultivator_v1.blend`. Preserve the original.

**Objective:** Determine whether the premade `donitz_monk_robe` or `mindfront_kimono` can serve as the foundation for a male wuxia/xianxia cultivator outfit.

1. Confirm both clothing assets exist in the installed MPFB library.
2. Create separate test copies of the character for each compatible garment.
3. Load each garment through MPFB's clothing tools, preserving the existing 53-bone humanoid skeleton.
4. Hide the original modern clothing.
5. Inspect the resulting garment from the front, side, and back.
6. Check whether the clothing follows the existing armature.
7. Test moderate shoulder, elbow, hip, and knee rotations.
8. Look for clipping, stretching, detached geometry, and poor proportions.
9. Do not create replacement clothing from primitives.
10. Do not attempt to repair fundamentally incompatible assets.

Save successful tests separately under `blender-tests/`.

Report which garment is the better foundation for a traditional cultivator outfit, what modifications it needs, and whether it remains usable during animation.

Do not import anything into Unreal or modify previous character files.

## Progress

- [x] 1. Confirmed both assets exist: `donitz_monk_robe` (suits02, CC0, Donitz) and `mindfront_kimono` (dress01, CC0, Mindfront).
- [x] 2. Created separate test copies: `mpfb_cultivator_donitz_robe_test.blend`, `mpfb_cultivator_mindfront_kimono_test.blend`.
- [x] 3. Loaded each garment via MPFB `add_asset`, both rigged to the existing 53-bone `Human.rig`.
- [x] 4. Hid original modern clothing (`male_casualsuit01`, `shoes01`, `high-poly`).
- [x] 5. Inspected front, side, and back of each garment.
- [x] 6. Confirmed both follow the armature (sleeves track the arms).
- [x] 7. Tested moderate shoulder/elbow/hip/knee rotations (arm raise + leg raise).
- [x] 8. Checked clipping/stretching/detached geometry/proportions.
- [x] 9. No replacement clothing from primitives created.
- [x] 10. No repair of fundamentally incompatible assets attempted.

## Report

**Better foundation: `mindfront_kimono`.**

- `donitz_monk_robe`: strong wuxia silhouette (long brown robe, wide sleeves, tied sash, high collar) and clean back drape, but the lower robe is a **fully closed tube with no slit**. During a leg raise the hem bunches, stretches, and clips badly. Not usable for animated leg movement.
- `mindfront_kimono`: same cultivator silhouette (long robe, wide sleeves, waist sash, high collar) but with **front crossover panels that open when the leg is raised**, letting the leg move through. Only mild hem lift, no severe clipping. Usable during animation.

**Modifications needed for `mindfront_kimono`:**

- Recolor the material (currently default pale pink/white) to a cultivator-appropriate color (deep indigo, white, or grey).
- Weights are interpolated (no `.mhw` file ships with the asset); the mild hem distortion is acceptable as-is but could be refined with weight painting if needed.

**Animation usability:** `mindfront_kimono` is usable. The `donitz_monk_robe` is not recommended for animated leg movement because of its closed robe tube.

Both test files are saved under `blender-tests/`. Original `mpfb_cultivator_v1.blend` and all previous character files are unchanged. Nothing was imported into Unreal.