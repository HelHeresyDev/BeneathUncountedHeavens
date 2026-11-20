"""Ask a local OpenAI-compatible model for bounded GarmentCodeRC parameters."""

import argparse
import json
from pathlib import Path
from urllib.parse import urlsplit
from urllib.request import Request, urlopen

import yaml

from generate_pattern import PARAMETERS, apply_parameters


def json_schema(design):
    fields = {}
    for parameter in PARAMETERS:
        group, key = parameter.split(".")
        field = design[group][key]
        limits = field["range"]
        kind = field["type"]
        if kind in {"float", "int"}:
            fields[parameter] = {"type": "number" if kind == "float" else "integer",
                                 "minimum": limits[0], "maximum": limits[1]}
        elif kind == "bool":
            fields[parameter] = {"type": "boolean"}
        else:
            fields[parameter] = {"type": "string", "enum": [choice for choice in limits if choice is not None]}
    return {"type": "object", "properties": fields, "required": list(fields), "additionalProperties": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--endpoint", default="http://127.0.0.1:8082")
    parser.add_argument("--repo", type=Path, default=Path.home() / "AI/GarmentCodeRC")
    parser.add_argument("--prompt", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    if urlsplit(args.endpoint).hostname not in {"127.0.0.1", "localhost", "::1"}:
        parser.error("This experiment only sends prompts to a local Qwen server")
    design = yaml.safe_load((args.repo / "assets/design_params/default_new.yaml").read_text())["design"]
    schema = json_schema(design)
    with urlopen(args.endpoint + "/v1/models", timeout=10) as response:
        model = json.load(response)["data"][0]["id"]
    bounds = {key: [spec["minimum"], spec["maximum"]] for key, spec in schema["properties"].items() if "minimum" in spec}
    prompt = ("Map the user's request to a single GarmentCodeRC upper garment. "
              "Use the exact flat keys and ranges in the JSON schema. Treat shirt.length as a multiplier, "
              "using at most 3.0 for a long robe so it clears the floor. "
              "Use shirt.openfront and a V neckline to approximate a wrap. The available schema has no "
              "true overlapping panels or separate sash. Return every field. "
              f"The hard inclusive numeric bounds are {json.dumps(bounds)}.\n" + args.prompt.read_text())
    messages = [{"role": "user", "content": prompt}]
    for attempt in range(2):
        request = Request(args.endpoint + "/v1/chat/completions", method="POST", headers={"Content-Type": "application/json"},
                          data=json.dumps({"model": model, "messages": messages, "temperature": 0, "max_tokens": 1024,
                                           "chat_template_kwargs": {"enable_thinking": False},
                                           "response_format": {"type": "json_schema", "json_schema": {"schema": schema}}}).encode())
        with urlopen(request, timeout=110) as response:
            output = json.load(response)
        answer = output["choices"][0]["message"]["content"]
        try:
            values = json.loads(answer)
            if not isinstance(values, dict) or set(values) != set(PARAMETERS):
                raise ValueError("Model output omitted or added parameter names")
            apply_parameters(design, values)
        except ValueError as error:
            if attempt:
                raise
            print(f"Rejected Qwen output: {error}", flush=True)
            messages.extend([{"role": "assistant", "content": answer},
                             {"role": "user", "content": f"Correct this violation and return all keys: {error}"}])
        else:
            break
    args.out.write_text(json.dumps(values, indent=2) + "\n")
    print(f"Validated {len(values)} parameters from {model}")


if __name__ == "__main__":
    main()
