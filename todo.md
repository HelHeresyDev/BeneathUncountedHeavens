# Codex Task: Get ChatGarment Working Locally on AMD ROCm

## Objective

Investigate, install, adapt, and test **ChatGarment** on my Ubuntu Linux workstation with an AMD Radeon RX 7900 XTX.

The intended application is creating Chinese wuxia/xianxia clothing for my Unreal Engine game, *Beneath Uncounted Heavens*.

I want to determine whether ChatGarment can be integrated into my existing autonomous character-creation workflow.

**Do the actual engineering work wherever feasible. Do not stop after producing installation instructions.**

However, this is an experimental tool. Do not spend substantial effort rewriting its neural network or simulator without first establishing whether the required model checkpoints are available and usable.

## 1. Repositories

Official ChatGarment:

https://github.com/biansy000/ChatGarment

Official GarmentCodeRC:

https://github.com/biansy000/GarmentCodeRC

ChatGarment also references ContourCraft-CG for garment simulation. Investigate it if necessary, but do not assume that the simulation component is required for initial sewing-pattern generation.

Check for existing local clones before downloading anything.

If missing, clone into `~/AI/`.

Preserve upstream repositories and track any necessary changes in isolated development branches.

## 2. My development environment

Hardware:

- AMD Radeon RX 7900 XTX, 24 GB VRAM
- Ubuntu 26.04 LTS
- Blender 5.3
- Unreal Engine 5.8.4

Existing local AI tools:

- Qwen 27B coding model hosted through llama.cpp/Vulkan
- OpenCode
- ComfyUI with a separate ROCm-based 3D-generation environment
- TripoSG
- Hunyuan3D Paint
- Blender MCP
- MPFB / MakeHuman
- OpenSew2
- CurioMesh-BUH

Unreal project:

`/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/`

My clothing workflow already supports some automated character generation and Blender-based garment processing.

The purpose of ChatGarment is to improve the generation of sewing patterns from descriptions and reference images, particularly for traditional Chinese clothing.

Do not replace the existing clothing tools.

## 3. First priority: Verify pretrained model availability

Before installing heavy dependencies, investigate the pretrained ChatGarment checkpoint.

The official installation expects:

`checkpoints/try_7b_lr1e_4_v3_garmentcontrol_4h100_v4_final/pytorch_model.bin`

A known GitHub issue reports that the original SharePoint download no longer works:

https://github.com/biansy000/ChatGarment/issues/30

The author suggested a Hugging Face alternative, but users reported that the available checkpoint files did not match the names required by the inference scripts.

Investigate the current situation.

### Required tasks

1. Inspect the official model-loading code.
2. Identify the model architecture and expected checkpoint structure.
3. Check the official checkpoint link.
4. Inspect the author's Hugging Face repository and available files.
5. Search for legitimate mirrors or updated inference instructions.
6. Determine whether alternative released checkpoints are compatible.
7. Verify architecture, tensor keys, dimensions, and loading requirements where possible.
8. Review model and checkpoint licenses.

Do not assume differently named checkpoints are interchangeable.

Do not fabricate or randomly rename missing weights.

Do not download an entire large checkpoint merely to discover that it is incompatible if metadata can establish that first.

If a compatible checkpoint is available, record its exact source, revision, size, and expected memory requirements.

If the checkpoint cannot be obtained or validated, document that obstacle and continue with the independent GarmentCodeRC investigation.

## 4. Investigate AMD ROCm compatibility

Read ChatGarment's installation requirements and inspect its inference code.

Identify dependencies on:

- PyTorch
- FlashAttention
- DeepSpeed
- bitsandbytes
- CUDA-specific operators
- CUDA device assumptions
- Custom extensions
- Transformers and LLaVA components

Determine which dependencies are necessary for inference versus training.

**We do not need to train ChatGarment. We only need inference.**

Avoid installing unnecessary training dependencies.

### Adaptation strategy

Prefer:

1. Native ROCm-compatible PyTorch.
2. Standard PyTorch scaled-dot-product attention where feasible.
3. Compatible inference implementations without custom CUDA kernels.
4. Disabling optional acceleration features that are not essential.
5. CPU fallbacks for small preprocessing tasks.

Do not attempt to install NVIDIA CUDA drivers or switch the system away from AMD.

Avoid patching unrelated global packages.

### GPU memory

The RX 7900 XTX has 24 GB VRAM.

Estimate and measure whether the model can run within that limit.

My local Qwen model occupies much of the GPU memory when loaded.

Coordinate GPU-heavy tests so ChatGarment does not compete with loaded Qwen, ComfyUI, or graphical Unreal.

Do not permanently modify my existing Qwen service or GPU configuration.

## 5. Create an isolated environment

Create an independent ChatGarment environment using an appropriate Python version and verified ROCm-compatible dependencies.

Do not modify existing virtual environments belonging to:

- ComfyUI
- Qwen
- Blender
- OpenSew2
- CurioMesh-BUH
- Unreal

Investigate the actual Python and PyTorch compatibility requirements before selecting versions.

Avoid copying old installation commands blindly if those dependency versions are incompatible with current ROCm.

Document all installed packages and versions.

Prefer a reproducible setup script or dependency lockfile.

If the installation fails, preserve the logs and identify the root cause before trying another approach.

## 6. Test ChatGarment inference

If a compatible checkpoint and working inference stack are available, perform real inference.

### Test A: Official example

Use an example included in the ChatGarment repository.

Run image-based garment reconstruction or another supported inference route.

Verify that the model actually loads and produces garment parameters or sewing-pattern JSON.

Inspect the output format and validate it against GarmentCodeRC.

Do not count a successful model import as successful inference.

### Test B: Wuxia garment

Create a test using this clothing description:

"A traditional Chinese wuxia cultivator's outer robe, inspired by historical hanfu. Cross-collar front, wide flowing sleeves, ankle-length body, overlapping front panels, loose waist with fabric sash, layered appearance, designed for a male martial artist. Practical for movement but elegant and flowing."

The garment should have:

- A clearly identifiable front opening or overlap.
- Two wide sleeves.
- A separate torso section.
- Suitable length and proportions.
- Logical sewing-pattern pieces.
- Reasonable symmetry.
- Adjustable garment dimensions.

Generate the pattern and inspect whether the topology and sewing information are structurally meaningful.

If the checkpoint cannot handle this garment type, document the limitation rather than claiming success.

## 7. Investigate GarmentCodeRC independently

This phase is important even if ChatGarment inference fails.

GarmentCodeRC may still provide useful procedural clothing functionality without requiring the ChatGarment language model.

Inspect its supported garment representations, parameter systems, and standalone pattern-generation capabilities.

### Test basic pattern generation

Using its existing examples or Python interface:

1. Generate a garment configuration.
2. Produce 2D sewing-pattern pieces.
3. Verify seams and panel dimensions.
4. Export supported pattern data.
5. Check whether these outputs can be reused or edited programmatically.

Explore its existing support for open-front garments, which may help with hanfu-like designs.

### Simulation compatibility

Investigate whether its cloth simulation requires NVIDIA Warp or another CUDA-specific implementation.

Determine whether:

- Pattern generation works on CPU.
- Garment meshing works without GPU simulation.
- Simulation can run on CPU.
- An alternative cloth simulator can be used.
- Blender or OpenSew2 could consume the pattern output.

Do not undertake a major CUDA-to-ROCm simulation rewrite during this experiment.

**A working 2D pattern generator is already a useful result, even without successful draping.**

## 8. Investigate integration with Blender and OpenSew2

Our long-term goal is:

**Clothing reference or description → sewing patterns → garment mesh → fitting/rigging → Unreal Engine**

Investigate how ChatGarment/GarmentCodeRC outputs could feed our existing Blender clothing workflow.

Specifically, determine whether generated patterns can be:

- Exported as SVG, DXF, JSON, or another supported representation.
- Imported into Blender.
- Converted into usable garment panels.
- Sewn or assembled using existing tools.
- Fitted to a MakeHuman/MPFB character.
- Prepared for rigging and animation.

OpenSew2 is already under active development in my toolchain. Avoid building a separate garment simulation system if the existing tools can be reused.

If possible, demonstrate one minimal pattern-to-Blender transfer using supported formats.

Use isolated test scenes.

Do not modify existing character or clothing assets.

## 9. Investigate local Qwen integration

The published ChatGarment workflow uses GPT-4o for some text-generation and editing preprocessing.

Investigate whether those steps can instead use my existing local Qwen model or deterministic prompt templates.

The important distinction is:

- Qwen may be able to prepare garment descriptions and parameters.
- ChatGarment's fine-tuned model is responsible for generating its learned garment representation.

Do not assume Qwen can directly replace ChatGarment's model.

Test a substitution only where the actual interfaces support it.

If ChatGarment inference is unavailable, assess whether Qwen could generate valid GarmentCodeRC parameters directly, with schema validation and executable tests.

That would provide an alternative workflow without requiring ChatGarment model weights.

## 10. Quality evaluation

For each successful output, evaluate:

- Sewing-pattern validity.
- Panel completeness.
- Seam consistency.
- Parametric editability.
- Front-opening structure.
- Sleeve design.
- Symmetry where appropriate.
- Blender interoperability.
- Potential for character fitting.
- Potential for animation.
- Failure modes and limitations.

Do not claim a garment is animation-ready solely because it can be displayed in Blender.

A complete animated garment requires further fitting, rigging, and deformation verification.

## 11. Development scope and safeguards

This is an investigation and limited compatibility implementation—not authorization for an unlimited rewrite.

Do not:

- Alter my existing AI environments.
- Install NVIDIA drivers.
- Download huge model files from unverified sources.
- Start an expensive training run.
- Replace my OpenSew2 installation.
- Modify production Unreal assets.
- Rebuild the entire garment simulator without evidence that it is necessary.
- Introduce cloud inference dependencies without documenting them.
- Publish proprietary model weights to GitHub.

Use upstream-compatible changes where practical.

If a small, well-defined patch enables ROCm inference, implement and test it.

If the checkpoint is unavailable or the architecture is fundamentally incompatible, stop pursuing that path after documenting the evidence. Continue on GarmentCodeRC where useful.

## 12. Deliverables

Create a report:

`CHATGARMENT_FEASIBILITY.md`

Include:

1. Source repositories and revisions.
2. Exact checkpoint availability.
3. Checkpoint compatibility findings.
4. Dependency audit.
5. AMD ROCm compatibility.
6. Environment setup and installed versions.
7. Actual inference results or errors.
8. GarmentCodeRC standalone results.
9. Wuxia robe experiment results.
10. Blender/OpenSew2 integration findings.
11. GPU memory usage where measured.
12. Remaining blockers and their likely difficulty.
13. Reproducible commands.
14. Final recommendation.

Also save any generated sewing patterns, JSON files, SVGs, previews, or Blender test scenes in a dedicated experimental directory.

Include appropriate scripts and tests for any changes made.

If a useful tool or compatibility layer is produced, commit it to a separate development branch or repository with proper upstream attribution. Do not publish checkpoints or modify unrelated repositories.

## Final decision

At the end, classify ChatGarment as one of:

**WORKING** — Actual model inference works locally and produces valid garment-pattern data.

**PARTIALLY WORKING** — Some meaningful functionality works, but the full model or downstream workflow is blocked.

**BLOCKED** — The required model or runtime cannot currently be made operational within reasonable scope.

**NOT WORTH PURSUING** — The technical effort outweighs its practical benefit to our existing Blender/OpenSew2 workflow.

Evaluate GarmentCodeRC separately, because it may be useful even if ChatGarment is blocked.

## Execution instructions

Start by inspecting the official checkpoint sources and dependency requirements.

Do not immediately install every dependency.

Once the checkpoint situation is understood, establish the smallest viable inference environment.

Attempt a genuine model test if technically possible.

Then investigate GarmentCodeRC as an independent route to procedural garment creation.

Work through the achievable phases without repeatedly asking for confirmation.

Provide a final technical report with verified results, useful outputs, exact blockers, and a recommendation on whether to continue development.

**The goal is to determine whether ChatGarment will actually help us create better wuxia clothing—not to spend weeks maintaining another complex AI tool for its own sake.**