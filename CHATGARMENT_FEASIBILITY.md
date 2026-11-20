# ChatGarment feasibility on AMD ROCm

**ChatGarment is working for local inference.** The matching author-linked checkpoint produced a structurally valid dress pattern from the official example on the RX 7900 XTX. Both wuxia text attempts produced self-intersecting patterns and omitted the requested wrap closure. **GarmentCodeRC works independently on CPU.** With validated Qwen parameters it produced an editable open-front robe. OpenSew2 transferred and welded the robe's seams, but the test garment slipped off the avatar during fitting.

The workstation ran Ubuntu 26.04.1 LTS with 24 GiB VRAM and Blender 5.3.0. The upstream clones were already present and were used at [`798c34a` for ChatGarment](https://github.com/biansy000/ChatGarment/tree/798c34ac04319668aaa0e3a3a02d44284fbd70ec) and [`e3d4476` for GarmentCodeRC](https://github.com/biansy000/GarmentCodeRC/tree/e3d4476d6f9bcb0af5ee0eccd18b0433e3df398a). Experiments, dependency locks, scripts and local outputs are in [`blender-tests/chatgarment-experiment/`](blender-tests/chatgarment-experiment/).

## Checkpoint audit

The published inference scripts use `checkpoints/try_7b_lr1e_4_v3_garmentcontrol_4h100_v4_final/pytorch_model.bin` and load it with `strict=True`. A bounded request to the documented SharePoint link returned HTTP 429 with a `Retry-After` header of 300 seconds. The author confirmed difficulty accessing that link and pointed to a public Hugging Face repository in issue #30. The author’s separate dataset contains training and evaluation data. [Official installation](https://github.com/biansy000/ChatGarment/blob/798c34ac04319668aaa0e3a3a02d44284fbd70ec/docs/Installation.md) · [Issue #30](https://github.com/biansy000/ChatGarment/issues/30) · [Dataset](https://huggingface.co/datasets/sy000/ChatGarmentDataset)

| Author-linked tree at `115bae7753818efed4a0147446ee69db23826e1f` | Bytes | Comparison to the published inference model |
| --- | ---: | --- |
| `try_lr1e_4_wholebody_pose_v2_detailT2_1float_upd/pytorch_model.bin` | 14,987,210,682 | All 818 keys and dimensions matched |
| `try_lr1e_4_wholebody_pose_v2_detailT2_upd_possibleDebug_v3_garmentcontrol_addFTdata/pytorch_model.bin` | 14,985,981,562 | Both final garment-head tensors have one output where the current model expects 76 |

I fetched 1 MiB from each candidate over HTTP Range and decoded metadata with a restricted reader that does not import Torch storage constructors or read tensor payloads. The expected tensors came from constructing the exact LLaVA/Vicuna architecture on the meta device using the pinned upstream config and tokenizer. The matching file has 32 LLaMA layers, width 4,096, rank-128 LoRA on language `q_proj` and `v_proj`, a CLIP ViT-L/14 vision encoder at 336 pixels, a 32,001-token vocabulary including `[SEG]`, and a 76-output numerical garment head. Its published LFS SHA-256 is `aa244860a81e8a39d96915388359bed46008fced60647dc144bb87389cd2e6c0`. The model topology matches the release scripts. The checkpoint's trained behavior can differ from the unavailable `v4_final` run. [Author’s model files](https://huggingface.co/sy000/chatgarment_extrafiles/tree/115bae7753818efed4a0147446ee69db23826e1f) · [LLaVA model config](https://huggingface.co/liuhaotian/llava-v1.5-7b/blob/4481d270cc22fd5c4d1bb5df129622006ccd9234/config.json)

ChatGarment source declares Apache 2.0 and GarmentCodeRC source declares MIT. The author's checkpoint repository exposes no separate license or model card. LLaVA identifies its Llama 2 base as subject to the Llama 2 Community License. I treated the author-linked weights as research inputs for this local experiment. Confirm checkpoint-specific permissions with the publisher before using the weights in production or distributing them. [ChatGarment license](https://github.com/biansy000/ChatGarment/blob/798c34ac04319668aaa0e3a3a02d44284fbd70ec/LICENSE) · [GarmentCodeRC license](https://github.com/biansy000/GarmentCodeRC/blob/e3d4476d6f9bcb0af5ee0eccd18b0433e3df398a/LICENSE) · [LLaVA model card](https://huggingface.co/liuhaotian/llava-v1.5-7b)

## ROCm and dependencies

The stock launch script assumes NVIDIA CUDA paths, launches through DeepSpeed, pins Torch 2.1.2 and Transformers 4.37.2, and selects FlashAttention 2 in the Python inference script. It explicitly uses 16-bit weights and rejects its 4-bit and 8-bit modes, so bitsandbytes is unnecessary for this run. DeepSpeed, training hooks, gradient checkpointing, TensorBoard and W&B can be bypassed for single-device generation. The published text path calls GPT-4o to normalize the user's garment description. The image path's two stages are local model calls.

The isolated runner keeps Transformers 4.37.2 and PEFT 0.10.0 for architecture compatibility, uses Python 3.12.13 and Torch 2.9.1+rocm6.4, and selects Torch SDPA. The existing ComfyUI stack was also queried read-only and reported HIP 6.4.43484, gfx1100-class Radeon RX 7900 XTX, BF16 support and 25,753,026,560 bytes total VRAM. On ROCm the Torch device is still addressed as `cuda`. The full checkpoint includes base and vision weights, so the runner reconstructs those modules from config on the meta device and materializes the matching state once. It uses `torch.load(weights_only=True, mmap=True)`. Casting 7,147,112,524 values to BF16 takes approximately 13.31 GiB for parameters alone. Generation needs more for the vision pass, attention, KV cache, and allocator reserve. [AMD’s PyTorch compatibility documentation](https://rocm.docs.amd.com/en/docs-6.4.1/compatibility/ml-compatibility/pytorch-compatibility.html)

The standalone CPU pattern environment uses Python 3.12.13, NumPy 1.26.4, SciPy 1.17.1, svgpathtools 1.8.0 and CairoSVG 2.9.1. Meshing adds CGAL 6.0.1.post202410241521 and libigl 2.6.3. Exact environment locks are in the experiment directory. GarmentCodeRC's documented simulator uses a modified NVIDIA Warp with custom collision, panel assignment and XPBD functions. The CPU pattern and box-mesh tests exercised independent paths. The forked Warp simulator and ChatGarment's subsequent ContourCraft-CG path are untested. [GarmentCodeRC install guide](https://github.com/biansy000/GarmentCodeRC/blob/e3d4476d6f9bcb0af5ee0eccd18b0433e3df398a/docs/Installation.md) · [ChatGarment simulation instructions](https://github.com/biansy000/ChatGarment/tree/798c34ac04319668aaa0e3a3a02d44284fbd70ec#after-inference)

## Actual model inference

The matching checkpoint downloaded completely and matched the published SHA-256. In a fresh run, strict loading took 17.13 seconds and used all 818 tensors. Torch SDPA ran the BF16 model on the idle Radeon without enabling the optional fused attention kernels. The tables report PyTorch memory, excluding unrelated desktop memory. [Measured results](blender-tests/chatgarment-experiment/evidence/results.json)

| Case | Panels / stitch pairs | Inference | Peak allocated / reserved | Result |
| --- | ---: | ---: | ---: | --- |
| Official dress image, two-stage reconstruction | 10 / 36 | 25.69 s on final replay | 15.05 / 15.72 GiB | Valid closed panel outlines and stitch references. Structurally recognizable bodice, sleeves, and skirt. |
| Wuxia robe, deterministic geometry description | 20 / 42 | 24.59 s | 15.21 / 15.90 GiB | Waistband back and both skirt panels self-intersected. |
| Wuxia robe, open-front coat wording | 18 / 48 | 24.15 s | 15.18 / 16.15 GiB | Both front sleeve panels self-intersected. |

The official dress passed the topology and pattern checks. Its maximum paired-edge length difference was 42.3%, which warrants reviewing intended gathers before sewing. The final single-image replay exited successfully. Both text runs made real numeric predictions through the 76-output head and exported draft JSON. They chose a conventional bodice-and-skirt construction with a joined center front despite the requested overlap and sash. The runner rejected both specifications. The text tests used deterministic local formatting in the same geometry-description schema as the upstream GPT-4o preprocessing step. The shape head and generated parameters came from ChatGarment in every model test. Local raw results and previews are under `outputs/inference-official-verified/`, `outputs/inference/`, and `outputs/inference-coat/`.

## GarmentCodeRC and Qwen

| Case | Panels | Stitch pairs | Length | Worst matching-edge length difference |
| --- | ---: | ---: | ---: | ---: |
| Upstream `t-shirt.yaml` and neutral measurements | 8 | 16 | 49.9 cm | 0.20% |
| Open-front robe, authored parameters and mean male measurements | 8 | 13 | 123.5 cm | 0.89% |
| Open-front robe, local Qwen parameters and mean male measurements | 8 | 13 | 122.9 cm | 1.18% |

All three produced closed panel outlines, valid stitch references and printable SVG/PDF and JSON files. The default `BoxMesh` resolution of 1 cm generated 9,264 vertices and 18,308 triangles for the shirt, and 30,959 vertices and 61,260 triangles for the authored robe, with no Warp runtime. The 2.5 cm attempt triggered an upstream logging bug on a short edge because `Edge.init_curve` references an undefined `name`. The CPU OBJ export uses metres and a Blender Z-up axis conversion. The generated OBJ is pre-drape geometry.

The directly validated Qwen workflow uses `qwen3.8-coder390` through the existing localhost service. It maps prose to twelve allowed GarmentCodeRC parameters. The first schema-only response exceeded the `shirt.width` range. Supplying the numeric bounds in the prompt produced parameter values that passed the local validator and generated the reported eight-panel robe. The authored and Qwen robes both keep center-front edges open, have mirrored torso and sleeve pairs, and use a V-shaped neckline. GarmentCodeRC's `openfront` omits the seam joining the front halves. The diagonal overlap, separately drafted crossover band and detachable sash still need their own geometry. [Procedural robe SVG](blender-tests/chatgarment-experiment/evidence/procedural-robe.svg)

## Blender and OpenSew2 transfer

The bridge samples the original curves and brings them into OpenSew2 as editable panels with named edges. It converts the source poses from centimetres and Y-up, restores each panel's original translation and rotation, and recreates the original seam pairs. The authored robe transferred into Blender 5.3.0 against OpenSew2's CC0 MPFB-based default male avatar. OpenSew2 registered all eight panels, 13 seams and 284 springs. Its initial arrangement reported 26 of 15,572 vertices inside the body, with zero triangle-pair intersections reported by its separate triangle metric. The T-shirt's 5.6 mm seam segments lost boundary membership at the 2.6 cm and 1.2 cm remeshing resolutions. The converter rejected that import.

OpenSew2 welded all 284 springs with zero dropped stitches in a 25-frame sew and 20-frame settle. The passes took 5.2–5.8 seconds. The resulting mesh had no loose stitch edges. The robe's original upper extent was 1.484 m. When reopened after baking, it occupied heights from −0.489 to 0.625 m while the avatar's feet were at 0 m and its top at 1.858 m. The robe dropped well below its intended position and its sleeves collapsed in the CPU preview. Its zero final intersection count reflects that displacement. The retained Blender tests demonstrate panel and seam interoperability, with fitting rejected. The test uses one static pose. Skin weighting and deformation verification remain separate work. A Blender-ready fitted result still needs shoulder placement and stabilization, wrap closure and sash construction. [Rejected drape preview](blender-tests/chatgarment-experiment/evidence/failed-drape.png)

## Reproduce the experiment

Run from `blender-tests/chatgarment-experiment/`. The commands below use the existing pinned upstream checkouts and fresh output paths. The local data paths are ignored by Git.

```sh
uv venv --python 3.12 .venv
uv pip install --python .venv/bin/python -r requirements-cpu.txt
export MPLBACKEND=Agg MPLCONFIGDIR="$PWD/.cache/matplotlib" PYTHONDONTWRITEBYTECODE=1

.venv/bin/python generate_pattern.py --design t-shirt --body mean_all --out outputs/tshirt-replay
.venv/bin/python generate_pattern.py --parameters robe-parameters.json --out outputs/robe-replay
.venv/bin/python mesh_pattern.py --pattern outputs/robe-replay/robe-replay_specification.json \
  --out outputs/robe-replay/garment-cpu.obj
.venv/bin/python qwen_parameters.py --prompt wuxia-prompt.txt --out outputs/qwen-replay.json
.venv/bin/python generate_pattern.py --parameters outputs/qwen-replay.json --out outputs/qwen-pattern-replay
.venv/bin/python -m unittest -v test_parameters.py
```

Use a new Blender profile for the transfer. `--opensew` points to the checkout that contains the `clothing_design` directory. The tested checkout is `b63fa05`.

```sh
profile=$(mktemp -d)
mkdir -p "$profile/config" "$profile/scripts" "$profile/extensions"
BLENDER_USER_CONFIG="$profile/config" BLENDER_USER_SCRIPTS="$profile/scripts" \
BLENDER_USER_EXTENSIONS="$profile/extensions" blender -b --factory-startup \
  --python-exit-code 1 --python blender_transfer.py -- --opensew ../opensew2-fork \
  --pattern outputs/robe-replay/panel-data.json --out outputs/robe-replay/transfer.blend
```

For ChatGarment, create a separate inference environment with the ROCm index ahead of PyPI and fetch only the tokenizer, configs, and the verified checkpoint. The URLs are immutable revisions. Run `checkpoint_layout.py` before downloading weights if you are checking a different candidate. Supply one local image or one geometry description, or both for the combined replay. The runner requires at least 18 GiB of GPU memory to be free before it loads the model.

```sh
uv venv --python 3.12 .chatgarment-venv
uv pip install --python .chatgarment-venv/bin/python \
  --index https://download.pytorch.org/whl/rocm6.4 --default-index https://pypi.org/simple \
  -r requirements-inference.txt

mkdir -p .weights/llava .weights/clip
llava=https://huggingface.co/liuhaotian/llava-v1.5-7b/resolve/4481d270cc22fd5c4d1bb5df129622006ccd9234
clip=https://huggingface.co/openai/clip-vit-large-patch14-336/resolve/ce19dc912ca5cd21c8a653c79e251e808ccabcd1
for f in config.json tokenizer.model tokenizer_config.json special_tokens_map.json; do
  curl -fsSL "$llava/$f" -o ".weights/llava/$f"
done
for f in config.json preprocessor_config.json; do
  curl -fsSL "$clip/$f" -o ".weights/clip/$f"
done
checkpoint=https://huggingface.co/sy000/chatgarment_extrafiles/resolve/115bae7753818efed4a0147446ee69db23826e1f/try_lr1e_4_wholebody_pose_v2_detailT2_1float_upd/pytorch_model.bin
python3 checkpoint_layout.py --url "$checkpoint" --out .weights/replay-layout.json
export HF_HUB_OFFLINE=1 HF_HOME="$PWD/.cache/hf"
.chatgarment-venv/bin/python infer_chatgarment.py \
  --metadata-dir .weights --layout .weights/replay-layout.json

curl -fL -C - "$checkpoint?download=true" -o .weights/author-alternative.pt
printf '%s  %s\n' aa244860a81e8a39d96915388359bed46008fced60647dc144bb87389cd2e6c0 \
  .weights/author-alternative.pt | sha256sum -c -
example="$HOME/AI/ChatGarment/example_data/example_imgs/72b086429d2dfe2a8de6f4403a024b2bb17446021c9e8f9ebacfc7a990ac8434.png"
.chatgarment-venv/bin/python infer_chatgarment.py --metadata-dir .weights \
  --layout .weights/replay-layout.json --weights .weights/author-alternative.pt \
  --image "$example" --out outputs/inference-image-replay
.chatgarment-venv/bin/python infer_chatgarment.py --metadata-dir .weights \
  --layout .weights/replay-layout.json --weights .weights/author-alternative.pt \
  --description wuxia-description.json --out outputs/inference-text-replay
```

The text example can return exit code 1 after writing its draft if the geometry gate rejects it. Use `wuxia-coat-description.json` to replay the second text trial. Add `--sew` to the Blender transfer command and use a fresh output filename to replay and bake the fitting trial.

## Recommendation

Use validated Qwen parameters with GarmentCodeRC for draft generation. Make crossover fronts and a separate sash explicit garment-construction features, then test the shoulder and sleeve placement in OpenSew2 before spending time on rigging or Unreal export. Keep ChatGarment available for local image-based pattern estimation experiments. Both prompt variants missed the wuxia construction, and the verified checkpoint has unresolved licensing for production use.
