# OpenSloper Experiment 1 — Wuxia Cross-Collar Robe: Final Report

An editable, parametric OpenSloper 0.4.0 pattern for a traditional Chinese cross-collar wuxia robe was drafted, validated, and tested. All files are in `~/AI/wuxia-pattern-tests/opensloper-01/`.

## 1. OpenSloper tools used

Driven entirely through the OpenSloper MCP server (`opensloper-cli serve`), no invented methods:
`convert_units`, `add_variable`, `set_formula`, `add_point`, `add_edge` (line + curvePath), `add_rectangle`, `create_piece`, `set_grainline`, `set_cut`, `add_notch`, `add_seam`, `set_measurement`, `get_object`, `get_measurements`, `check`, `render` (svg, labels, garment, region), `open`, `save`. The CLI was used for `export-draft` (full SVG) and `check --json` (validation report).

## 2. Pattern components created

Six pieces: `Back`, `Front_left`, `Front_right`, `Sleeve` (cut 2, mirrored), `Collar`, `Sash`. 23 variables, 29 points, 26 edges (including 6 armhole curvePaths and a 15 cm cut-role hem slit), 9 seams, vertical grainlines on every piece, and 8 notches. The two front panels are separate physical pieces whose diagonal crossover edges are never sewn to each other.

## 3. Are the collar and front-panel overlap geometrically correct?

**Yes — and this required a real fix.** As first drafted, each front panel's diagonal slanted toward its *own* side seam, so the two edges never crossed the centre front (not a genuine cross-collar). I corrected the orientation so each shoulder line runs shoulder-tip→neck, matching the back panel, and the crossover edge slants across to the opposite side. Now the left diagonal runs from the left neck (centre-back x=19) to x=31 at the waist and the right from x=35 to x=23; they cross at ≈28 cm below the neck, and the right panel overlaps the left by `crossover_overlap` = 8 cm at the waist. The collar band (59.68 cm) equals the 16 cm neck opening plus the 43.68 cm crossover diagonal, so it follows the overlapping neckline exactly. This is verified by computed edge lengths, not by looking at the drawing.

## 4. Did the seams pass validation?

**Yes.** `check` returns `problems: []`, `blocked: 0`. All 9 seams are within the 1 mm tolerance: shoulders 19=19, sides 82=82, the four armholes 28.10 vs 28.10, and the collar seam 59.68 vs 43.68 with ease −16 (exactly `neck_width`). No self-intersections were reported on any piece.

## 5. Did measurement changes work?

**Yes.** Two changes were tested and then the baseline restored:
- `waist_circumference` 840→900 mm: `sash_length` 99→105 cm, no check problems.
- `neck_back_to_waist` 420→450 mm: `waist_y` 42→45, `robe_length` 100→103, `collar_length` 59.68→62.57 cm (the diagonal and collar recompute), no check problems.
Both were restored to 840/420 mm and the pattern returned to its exact baseline values with zero problems. The design stays fully editable.

## 6. Features OpenSloper could not represent or validate

- **Right-over-left layering**: OpenSloper has no "this edge lies over that edge" relation. The overlap is achieved geometrically (the two diagonals cross and the right panel is wider at the waist), but the software cannot *express* or *validate* which panel is on top — that is a construction assumption documented in `construction.md`, not a checked fact.
- **The hem slit** is a `cut`-role edge, so it is a real opening in the piece, but there is no representation of a facing, binding, or how the slit is finished.
- **Sash tying / knot** and the **button/pin closure** of the crossover are not modelled — the sash is an independent rectangle and the crossover is left open by design.
- **Draping, ease-in-wear, and cloth behaviour** are not validated; only flat seam lengths, curve definitions, and self-intersection are checked.
- The **sleeve armhole** is a reshaping of the body armhole; OpenSloper validates that the total lengths match (they do) but does not validate the 3D fit of a dipping sleeve cap into the armhole.
- `render`'s per-piece `region` crops the viewBox but still embeds the full pattern geometry in each file, so the "individual piece illustrations" are cropped views rather than standalone single-piece files.

## 7. Where the pattern and SVGs were saved

All in `~/AI/wuxia-pattern-tests/opensloper-01/`:
- `wuxia-robe.sloper` — the editable pattern (baseline measurements attached).
- `wuxia-robe.slm` — the documented baseline measurements (mm).
- `wuxia-robe-draft.svg` — full labelled pattern preview.
- `wuxia-robe-garment.svg` — OpenSloper's sewn-garment visualization (each piece as sewn, shaded).
- `wuxia-robe-piece-{Back,Front_left,Front_right,Sleeve,Collar,Sash}.svg` — labelled per-piece illustrations (cropped views).
- `validation.json` — the full computed check report.
- `construction.md` — this construction and seam specification.

## 8. Is it ready to attempt cloth simulation?

**Yes, with caveats.** The pattern is flat, valid, self-intersection-free, has all seam lengths matched, and is fully parametric — a sound input for a cloth-simulation drape. The caveats from §6 apply: the simulation will have to supply the right-over-left layering, the open crossover, the slit finish, and the sash behaviour that OpenSloper leaves to construction. No simulation was run in this experiment, per the task boundary.
