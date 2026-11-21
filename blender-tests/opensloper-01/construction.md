# Wuxia Cross-Collar Robe — Construction & Seam Specification

Editable, parametric OpenSloper pattern. Units: **cm** (the `.slm` measurement file is in **mm**; `@key` references are auto-converted). Baseline body: 175 cm tall adult, 100 cm chest, 84 cm waist, 98 cm hip (see `wuxia-robe.slm`).

## Pieces (6)

| Piece | Cut | Notes |
|---|---|---|
| `Back` | 1 | Centre-back panel, 54 cm wide × 100 cm long |
| `Front_left` | 1 | Left crossover panel |
| `Front_right` | 1 | Right crossover panel (mirror of left, right-over-left) |
| `Sleeve` | 2, mirrored | One wide sleeve piece, cut twice mirrored for left/right |
| `Collar` | 1 | Collar band, 59.68 × 6 cm |
| `Sash` | 1 | Waist sash, 99 × 9 cm, no seams (tied) |

## Variables (23, all adjustable)

Body-derived: `shoulder_length`=@shoulder_length (19), `back_width`=@across_back+back_ease/2+over_layer_ease/2 (54), `robe_length`=@neck_back_to_waist+hem_drop (100), `waist_y`=@neck_back_to_waist (42), `sash_length`=@waist_circumference+sash_overlap (99).

Design/ease: `back_ease`=8, `front_ease`=8, `over_layer_ease`=6, `hem_drop`=58, `crossover_overlap`=8, `armhole_depth`=18, `armhole_control`=12, `sleeve_length`=55, `sleeve_flare`=12, `armhole_half_width`=16, `armhole_dip`=15, `armhole_dip_control`=11.74, `collar_width`=6, `sash_width`=9, `neck_width`=16.

Derived: `inner_x`=back_width/2+crossover_overlap/2 (31) — x of each front panel's crossover edge at the waist, i.e. centre-front plus half the overlap; `collar_length`=neck_width+sqrt((inner_x−shoulder_length)²+waist_y²) (59.68) — neck opening plus the crossover diagonal.

## Edges & roles

- **Back**: two shoulder lines (19 each), a 16 cm neckline, two armhole `curvePath`s (chord 18, control 12), two vertical side seams (82), a 54 cm hem.
- **Front panels** (each): a 19 cm shoulder line, an armhole `curvePath`, a vertical side seam (82), a hem, a vertical inner edge (58), and the **diagonal crossover edge** (43.68 cm) from the waist point up to the neck. The diagonal slants across the centre front — it is never sewn to the other panel's diagonal.
- **Sleeve**: two armhole `curvePath`s dipping into the sleeve (control 11.74), two flared side seams (56.29 each), a 56 cm hem.
- **Collar / Sash**: rectangles.
- **Cut edge**: `FL_slit`, a 15 cm `cut`-role edge on `Front_left` from the hem up to y=85 — the lower opening for movement.

## Seams (9, all validated)

| Seam | Side A | Side B | Ease |
|---|---|---|---|
| Shoulder_left / Shoulder_right | Back shoulder line (19) | Front shoulder line (19) | 0 |
| Side_left / Side_right | Back side seam (82) | Front side seam (82) | 0 |
| Armhole_front_left/right | Front armhole (28.10) | Sleeve front armhole (28.10) | 0 |
| Armhole_back_left/right | Back armhole (28.10) | Sleeve back armhole (28.10) | 0 |
| Collar_crossover | Collar long edge (59.68) | Front_left diagonal (43.68) | −16 (= −neck_width) |

The collar is exactly `neck_width` (16 cm) longer than the diagonal: it wraps the 16 cm neck opening plus the 43.68 cm crossover edge. The sleeve armholes are a reshaping (dipping curve) of the body armholes; only total length must match, and it does (28.10 vs 28.10, within the 1 mm tolerance).

## Construction notes

- **Right-over-left**: the right front panel's crossover edge lies over the left panel's. Both diagonals cross at ≈28 cm below the neck; the visible overlap strip at the waist is `crossover_overlap` = 8 cm.
- **Grainlines**: vertical (90°) on all six pieces.
- **Notches**: at the shoulder and underarm points on the back and both front panels, to match the armhole/side seams.
- **Sash**: independent rectangle, no seams — wrapped and tied at the waist.
