# OpenSloper Experiment 1 — Wuxia Cross-Collar Robe

**Status: COMPLETE.** Pattern drafted, validated (`check` → 0 problems, 0 blocked, all 9 seams within tolerance), two measurement changes tested and baseline restored, all deliverables saved in `~/AI/wuxia-pattern-tests/opensloper-01/`. See `report.md` and `construction.md` there.

We are resuming development of the clothing-generation pipeline for Beneath Uncounted Heavens.

I want to test your ability to autonomously generate complex, editable sewing patterns using OpenSloper 0.4.0.

You are my existing Qwen coding model. Do not load or switch to another AI model.

## Preparation

1. Verify that OpenSloper's MCP server is connected.
2. Discover and inspect its available tools.
3. Read its relevant documentation using the available MCP tools, CLI, or official examples.
4. Create an isolated experiment directory:

`~/AI/wuxia-pattern-tests/opensloper-01/`

Do not modify the Unreal project, existing clothing experiments, or working software integrations.

## Goal

Create an editable, parametric sewing pattern for a traditional Chinese cross-collar wuxia robe.

The design should include:

- A back panel.
- Two distinct front panels with genuine diagonal crossover.
- A right-over-left closure appropriate to the intended traditional construction.
- Separate left and right wide sleeves.
- Proper shoulder and side seams.
- A collar band following the overlapping neckline.
- A separate waist sash.
- A lower garment opening or split suitable for martial-arts movement.

The garment should be designed to be worn over an inner layer, with appropriate ease.

Use reasonable adult body measurements as a starting point and document them. All important dimensions should be adjustable.

## Construction requirements

The pattern must represent actual garment construction, not just a drawing that resembles a robe.

Pay particular attention to:

1. The overlapping front panels must be separate physical pattern pieces.
2. The crossover edges must not be accidentally sewn together.
3. Sleeve attachment edges must correspond to appropriate armhole or body attachment edges.
4. The collar must follow the correct neckline geometry.
5. Seam lengths must be validated.
6. Curved edges should be mathematically defined.
7. Pattern pieces must not contain unintended self-intersections.
8. The waist sash must be an independent component.
9. The design must remain editable when measurements change.

## Use OpenSloper

Use OpenSloper's actual automation tools, documented file format, and validation operations.

Do not invent unsupported API methods.

If the MCP interface has limitations, investigate its documented CLI and Python client before introducing workarounds.

Prefer parametric construction through native OpenSloper operations.

## Deliverables

Save the following in the experiment directory:

- An editable `.sloper` pattern.
- A full SVG pattern preview.
- Individual labeled pattern-piece illustrations if supported.
- A measurements file if needed.
- A concise construction and seam specification.
- Validation results.
- A report explaining limitations and remaining defects.

If OpenSloper provides a sewn-garment visualization, export that as well.

## Validation

Run OpenSloper's pattern checks.

Fix detected issues rather than simply reporting them.

Then test changing at least two relevant measurements and confirm that the geometry updates appropriately.

Restore the intended baseline measurements before finishing.

Clearly distinguish actual validation results from visual assumptions.

## Important

Do not perform cloth simulation yet.

Do not use ChatGarment or download Design2GarmentCode model weights.

Do not attempt to rig or animate this garment.

This experiment is specifically testing whether you can use OpenSloper to construct sophisticated, valid, editable sewing patterns.

## Final report

Tell me:

1. Which OpenSloper tools you successfully used.
2. Which pattern components you created.
3. Whether the collar and front-panel overlap are geometrically correct.
4. Whether the seams passed validation.
5. Whether measurement changes worked.
6. Which features OpenSloper could not represent or validate.
7. Where you saved the pattern and SVG.
8. Whether the result is ready to attempt cloth simulation.

Perform the experiment rather than stopping after writing a plan.