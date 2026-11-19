I want to test how well you can use OpenSew-2 in Blender 5.3 to create clothing for my game, Beneath Uncounted Heavens.

You have access to Blender through Blender MCP. OpenSew-2 is already installed and its Clothing Design workspace works.

**Your task:** Independently create a convincing wuxia/xianxia cultivator outfit using OpenSew-2.

### Goal

Create a traditional outfit for a young male martial artist, including:

- A crossed-collar robe or tunic.
- Long, reasonably loose sleeves.
- A waist sash or belt.
- Trousers underneath.
- A blue-and-white color scheme.

The character should be able to move, fight, and perform martial-arts techniques without catastrophic clothing deformation.

### Your approach

I want you to figure out how to use OpenSew yourself.

1. Start a new Blender scene using OpenSew's male avatar.
2. Inspect the installed OpenSew-2 add-on, its Python API, operators, garment presets, and simulation tools.
3. Determine how to programmatically construct and manipulate sewing patterns through Blender MCP.
4. Develop your own approach to creating the outfit.
5. Use genuine OpenSew sewing patterns and garment simulation, not primitive cylinders or cones disguised as clothing.
6. Experiment with pattern shapes, sewing connections, garment placement, collision settings, and fabric behavior.
7. Diagnose problems and attempt sensible corrections rather than repeatedly executing the same failed operation.
8. Inspect your results visually throughout development.

### Known issues

During my manual testing:

- Trousers worked reasonably well.
- The T-shirt developed serious shoulder and sleeve distortions.
- The tank top also bunched up around the shoulders.
- The Relax operation did not significantly fix the upper-body garments.

Investigate these problems if they affect your work. Do not assume the default garment presets are reliable.

### Evaluation criteria

I care about:

- Whether you can independently discover and use OpenSew's functionality.
- Whether you can construct recognizable clothing from sewing patterns.
- Whether the resulting garment fits the avatar.
- Whether the garment behaves plausibly during simulation.
- Whether you can recognize and correct visible failures.
- Whether you can develop a better approach than my initial manual experiments.

You have freedom to experiment with different pattern designs, add-on settings, and construction techniques.

Do not claim visual success based solely on successful Python execution or modifier configuration. Inspect the actual result.

### Deliverables

- Save your work under `blender-tests/opensew_qwen_experiment/`.
- Create a final `.blend` file, even if the result is imperfect.
- Save viewport screenshots showing the finished garment from multiple angles.
- Document what you tried, what succeeded, what failed, and what you learned about OpenSew.
- Clearly distinguish completed features from unfinished or defective ones.

Do not modify my previous Blender character files or import anything into Unreal.

**Important:** This is an experiment in your ability to independently operate unfamiliar 3D modeling software. I want you to investigate, make decisions, test hypotheses, and evaluate your own results rather than simply follow a predetermined modeling recipe.