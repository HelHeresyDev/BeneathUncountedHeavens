I want you to independently evaluate and use the Devil Designer clothing-design extension installed in Blender 5.3.

This is an experiment for my Unreal Engine 5.8.4 wuxia/xianxia RPG, Beneath Uncounted Heavens.

**Primary objective:** Determine whether you can use Devil Designer through Blender MCP to create fitted, simulated 3D clothing suitable for our automated character-generation pipeline.

### Phase 1 — Investigate Devil Designer

Before attempting to create clothing:

1. Locate the installed Devil Designer extension.
2. Examine its Python source code, operators, data structures, and simulation implementation.
3. Determine how its pattern editor represents garment panels, vertices, edges, and sewing connections.
4. Identify how panels are positioned around a character.
5. Determine how the simulation can be started, monitored, and stopped programmatically.
6. Investigate whether its functionality can be accessed through Blender MCP without manual mouse interaction.

Do not assume that an operation is available just because the GUI contains a button. Verify the Python implementation.

### Phase 2 — Create a test garment

Start in a fresh Blender scene.

Use MPFB or another suitable existing human-generation tool to create a male avatar.

Using **Devil Designer's actual garment construction and simulation functionality**, attempt to create a simple upper-body garment.

Start with a sleeveless tunic. If successful, attempt to add long sleeves and a crossed collar suitable for a wuxia cultivator.

Requirements:

- Construct clothing using genuine garment panels and sewing connections.
- Position the garment correctly around the avatar.
- Sew the panels together.
- Run the garment simulation.
- Evaluate how well the fabric fits the body.
- Correct problems you discover.

Do not substitute cylinders, cones, or other primitive meshes for Devil Designer-generated clothing.

### Phase 3 — Investigate automation

I want this process to eventually work through OpenCode without manual assistance.

Determine whether it is possible to:

- Generate patterns from Python.
- Specify garment measurements programmatically.
- Define seams between panels.
- Position panels around an arbitrary MPFB character.
- Execute simulations without manual GUI interaction.
- Detect when simulation is complete.
- Export or preserve the resulting mesh.
- Transfer skin weights to a character's skeleton.

If Devil Designer lacks some of these capabilities, distinguish between functionality that is missing and functionality that exists but is difficult to access.

You may create experimental Python helper scripts if necessary. Do not modify the installed extension's source code without first explaining why.

### Phase 4 — Test reliability

Test the garment on the avatar.

Inspect it from the front, side, and back. Look for:

- Fabric intersections with the body.
- Incorrect panel placement.
- Open or misaligned seams.
- Severe stretching or bunching.
- Unnatural shoulder deformation.
- Simulation instability.

Be critical of your own results. Previous experiments produced visibly defective garments that were incorrectly reported as successful.

Blender may temporarily become unresponsive during simulation. This does not necessarily indicate a crash. Allow legitimate calculations to complete, and avoid launching overlapping simulation operations.

### Phase 5 — Deliverables

Save your work under:

`blender-tests/devil_designer_qwen_experiment/`

Provide:

1. A Blender project containing your best garment.
2. Screenshots showing the garment from multiple angles.
3. Any Python scripts you developed to automate Devil Designer.
4. A report detailing what worked, what failed, and how you solved problems.
5. An assessment of whether Devil Designer can be integrated into our automated pipeline.
6. A comparison with OpenSew-2, particularly regarding upper-body garment construction and simulation.

### Important constraints

- Preserve all existing character prototypes and Blender project files.
- Do not modify my OpenSew fork.
- Do not modify Unreal project assets.
- Do not install additional software unless necessary.
- Investigate existing functionality before implementing replacements.
- If the extension cannot reasonably be automated through Blender MCP, explain the technical obstacle rather than pretending to have completed the task.

**You have substantial freedom to investigate, experiment, and solve problems independently.**

The objective is not necessarily a beautiful finished outfit. It is to establish whether Devil Designer can become a reliable, AI-controlled component of our 3D asset-generation pipeline.

### Status (2026-11-19)

- **Phase 1 — Investigate:** DONE. Mapped operators, `Document`/`Session`/`Pattern`/`Seam` data model, `arrangement_record` panel placement, XPBD simulation, and confirmed full MCP accessibility with no mouse.
- **Phase 2 — Test garment:** DONE (partial). Sleeveless tunic built and draped cleanly via genuine panels + seams. Long sleeves FAIL (flat flaps trap against arms, body guard rejects `keep_drape`); crossed collar has no starter.
- **Phase 3 — Automation:** DONE. Pattern generation, measurements, seams, panel placement, simulation start/monitor/stop, and completion detection are all scriptable. **Missing:** skin-weight transfer (output is a static drape, not rigged).
- **Phase 4 — Reliability:** DONE. Front/back clean; side-seam gap from the right; long-sleeve intersection failure documented.
- **Phase 5 — Deliverables:** DONE. `devil_designer_tunic.blend`, `tunic_{front,right,back}.png`, `longsleeve_trapped.png`, and `devil_designer_findings.md` (report + pipeline assessment + OpenSew-2 comparison) in `blender-tests/devil_designer_qwen_experiment/`.