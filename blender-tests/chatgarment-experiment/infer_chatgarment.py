"""Run the published ChatGarment architecture with local, strictly matched weights."""

import argparse
import contextlib
import json
from pathlib import Path
import sys
import tempfile
from time import perf_counter


def make_model(metadata):
    import torch
    from accelerate import init_empty_weights
    from peft import LoraConfig, get_peft_model
    from transformers import AutoTokenizer, CLIPVisionModel
    from llava.model.language_model.llava_garment_float50 import GarmentGPTFloat50ForCausalLM, LlavaConfig

    tokenizer = AutoTokenizer.from_pretrained(metadata / "llava", local_files_only=True, use_fast=False)
    tokenizer.pad_token = tokenizer.unk_token
    tokenizer.add_tokens("[SEG]")
    config = LlavaConfig.from_pretrained(metadata / "llava", local_files_only=True)
    config.vocab_size = len(tokenizer)
    config.mm_vision_tower = str(metadata / "clip")
    config._attn_implementation = "sdpa"
    config.use_cache = True
    config.pad_token_id = tokenizer.pad_token_id
    token = tokenizer.convert_tokens_to_ids("[SEG]")
    with init_empty_weights(include_buffers=False):
        model = GarmentGPTFloat50ForCausalLM(config, seg_token_idx=token)
        tower = model.get_vision_tower()
        tower.vision_tower = CLIPVisionModel(tower.cfg_only)
        tower.is_loaded = True
        targets = [name for name, layer in model.named_modules() if isinstance(layer, torch.nn.Linear)
                   and name.startswith("model.layers.") and name.endswith((".q_proj", ".v_proj"))]
        adapter = LoraConfig(r=128, lora_alpha=256, target_modules=targets,
                             lora_dropout=0.05, bias="none", task_type="CAUSAL_LM")
        model = get_peft_model(model, adapter)
    return model, tokenizer


def compare_layout(model, candidate):
    expected = {key: list(value.shape) for key, value in model.state_dict().items()}
    actual = {key: value[1] for key, value in candidate.items()}
    missing = sorted(expected.keys() - actual.keys())
    unexpected = sorted(actual.keys() - expected.keys())
    mismatched = {key: [expected[key], actual[key]] for key in sorted(expected.keys() & actual.keys())
                  if expected[key] != actual[key]}
    result = {"expected_keys": len(expected), "checkpoint_keys": len(actual), "missing": missing,
              "unexpected": unexpected, "different_shapes": mismatched}
    print(json.dumps(result), flush=True)
    return not (missing or unexpected or mismatched)


def infer(model, tokenizer, pixels, prompt, limit):
    from llava.conversation import conv_templates
    from llava.mm_utils import tokenizer_image_token

    conversation = conv_templates["v1"].copy()
    conversation.messages = []
    conversation.append_message(conversation.roles[0], "<image>\n" + prompt)
    conversation.append_message(conversation.roles[1], None)
    ids = tokenizer_image_token(conversation.get_prompt(), tokenizer, return_tensors="pt").unsqueeze(0).to("cuda")
    tokens, floats, _ = model.evaluate(pixels, pixels, ids, max_new_tokens=limit, tokenizer=tokenizer)
    text = tokenizer.decode(tokens[0, 1:], skip_special_tokens=False).strip().replace("</s>", "")
    return text, floats


def run_case(args, model, tokenizer, processor, case, image_path, description):
    import torch
    from json_repair import repair_json
    from PIL import Image
    from llava.garment_utils_v2 import run_garmentcode_parser_float50
    from llava.mm_utils import process_images
    from pygarment.pattern.wrappers import VisPattern
    from generate_pattern import to_panels

    destination = (args.out / case).resolve()
    destination.mkdir(parents=True, exist_ok=False)
    image = Image.open(image_path).convert("RGB")
    pixels = process_images([image], processor, model.config).to(device="cuda", dtype=torch.bfloat16)
    started = perf_counter()
    stages = {}
    if description is None:
        description, _ = infer(model, tokenizer, pixels,
                               "Can you describe the geometry features of the garments worn by the model in the Json format?",
                               args.max_tokens)
        stages["image_description_generated"] = description
        description = description.replace("upper_garment", "upperbody_garment").replace("lower_garment", "lowerbody_garment")
        question = "Can you estimate the sewing pattern code based on the image and Json format garment geometry description?"
    else:
        question = "Can you estimate the outfit sewing pattern code based on the Json format garment geometry description?"
    response, floats = infer(model, tokenizer, pixels, question + "\n" + description, args.max_tokens)
    text = response.replace("[STARTS]", "").replace("[SEG]", "").replace("[ENDS]", "")
    structured = repair_json(text, return_objects=True)
    stages.update(response=response, parsed=structured, seconds=round(perf_counter() - started, 2),
                  peak_allocated_bytes=torch.cuda.max_memory_allocated(), peak_reserved_bytes=torch.cuda.max_memory_reserved(),
                  float_predictions=floats.detach().float().cpu().tolist() if floats is not None else [])
    result = destination / "inference.json"
    result.write_text(json.dumps(stages, indent=2) + "\n")
    if not isinstance(structured, dict) or floats is None:
        raise ValueError(f"{case} returned no usable sewing parameter object or numeric predictions")
    paths = run_garmentcode_parser_float50([], structured, floats.detach().float().cpu().numpy(), str(destination))
    patterns = {}
    for path in map(Path, paths):
        pattern = VisPattern(path)
        panels, stitches = to_panels(pattern)
        if not panels:
            raise ValueError(f"No panels in {path.name}")
        patterns[str(path.relative_to(destination))] = {
            "panels": len(panels), "stitches": len(stitches),
            "self_intersecting": bool(pattern.is_self_intersecting()),
            "max_relative_stitch_length_delta": max((stitch["relative_delta"] for stitch in stitches), default=0),
        }
    stages["patterns"] = patterns
    result.write_text(json.dumps(stages, indent=2) + "\n")
    if any(pattern["self_intersecting"] for pattern in patterns.values()):
        raise ValueError(f"{case} produced a self-intersecting pattern")
    print(case, json.dumps(patterns), stages["seconds"], flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=Path.home() / "AI/ChatGarment")
    parser.add_argument("--garmentcode", type=Path, default=Path.home() / "AI/GarmentCodeRC")
    parser.add_argument("--metadata-dir", type=Path, required=True)
    parser.add_argument("--layout", type=Path, required=True)
    parser.add_argument("--weights", type=Path)
    parser.add_argument("--image", type=Path)
    parser.add_argument("--description", type=Path)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--max-tokens", type=int, default=768)
    args = parser.parse_args()
    if args.weights and (not args.out or not (args.image or args.description)):
        parser.error("Inference requires an output path and an image or description")
    repo, garmentcode = args.repo.resolve(), args.garmentcode.resolve()
    metadata = args.metadata_dir.resolve()
    image_path = args.image.resolve() if args.image else None
    description = args.description.read_text().strip() if args.description else None
    if args.out:
        args.out = args.out.resolve()
    sys.path[:0] = [str(repo), str(garmentcode)]
    model, tokenizer = make_model(metadata)
    if not compare_layout(model, json.loads(args.layout.read_text())):
        raise SystemExit(1)
    if not args.weights:
        return

    import torch
    from transformers import CLIPImageProcessor

    if not torch.version.hip or not torch.cuda.is_available():
        raise RuntimeError("A usable ROCm PyTorch device is required")
    free, total = torch.cuda.mem_get_info()
    if free < 18 * 1024**3:
        raise RuntimeError(f"Free GPU memory {free / 1024**3:.1f} GiB is below the test's 18 GiB budget")
    torch.cuda.reset_peak_memory_stats()
    started = perf_counter()
    weights = torch.load(args.weights, map_location="cpu", weights_only=True, mmap=True)
    model.load_state_dict(weights, strict=True, assign=True)
    del weights
    model = model.to(device="cuda", dtype=torch.bfloat16).eval()
    torch.cuda.synchronize()
    print(f"Strictly loaded {len(model.state_dict())} tensors in {perf_counter() - started:.2f} s on "
          f"{torch.cuda.get_device_name(0)} ({total / 1024**3:.1f} GiB)", flush=True)
    processor = CLIPImageProcessor.from_pretrained(metadata / "clip", local_files_only=True)
    failures = []
    with tempfile.TemporaryDirectory(prefix="chatgarment-runtime-") as scratch:
        root = Path(scratch)
        (root / "assets").symlink_to(garmentcode / "assets", target_is_directory=True)
        (root / "docs").symlink_to(repo / "docs", target_is_directory=True)
        with contextlib.chdir(root), torch.inference_mode():
            for name, path, desc in [("official-image", image_path, None),
                                     ("wuxia-text", repo / "docs/images/black_img.jpg" if description else None, description)]:
                if path:
                    try:
                        run_case(args, model, tokenizer, processor, name, path, desc)
                    except Exception as error:
                        failures.append(f"{name}: {type(error).__name__}: {error}")
    if failures:
        raise RuntimeError("\n".join(failures))


if __name__ == "__main__":
    main()
