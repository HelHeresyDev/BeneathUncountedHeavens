"""Generate and check a GarmentCodeRC sewing pattern without a model or simulator."""

import argparse
from collections import Counter
import json
import math
from pathlib import Path
import sys


PARAMETERS = (
    "meta.upper", "shirt.openfront", "shirt.length", "shirt.width", "shirt.flare",
    "collar.f_collar", "collar.fc_depth", "sleeve.sleeveless", "sleeve.length",
    "sleeve.connecting_width", "sleeve.end_width", "sleeve.sleeve_angle",
)


def apply_parameters(design, values):
    if not isinstance(values, dict) or set(values) - set(PARAMETERS):
        raise ValueError("Supply a JSON object using only the supported parameter names")
    changes = []
    for name, value in values.items():
        group, key = name.split(".")
        field = design[group][key]
        kind = field["type"]
        limits = field["range"]
        if kind in {"float", "int"}:
            allowed = (int, float) if kind == "float" else (int,)
            valid = type(value) in allowed and math.isfinite(value) and limits[0] <= value <= limits[1]
        elif kind == "bool":
            valid = type(value) is bool
        else:
            valid = isinstance(value, str) and value in limits
        if not valid:
            raise ValueError(f"Invalid {name} value {value!r}. Expected {kind} in {limits!r}.")
        changes.append((field, value))
    for field, value in changes:
        field["v"] = value


def to_panels(pattern):
    units = pattern.properties["units_in_meter"]
    measurements = {}
    for name, panel in pattern.pattern["panels"].items():
        segments, lengths, outline = [], [], []
        svg_vertices = [[x, -y] for x, y in panel["vertices"]]
        for index, edge in enumerate(panel["edges"]):
            curve = pattern._edge_as_curve(svg_vertices, edge)
            length = float(curve.length()) / units
            if not math.isfinite(length) or length <= 0:
                raise ValueError(f"Degenerate edge {name}.{index}")
            count = max(2, math.ceil(length / 0.01))
            points = [[point.real / units, -point.imag / units]
                      for point in (curve.point(i / count) for i in range(count + 1))]
            outline.extend(points[:-1])
            segments.append([f"e{index}", points])
            lengths.append(length)
        if any(math.dist(segment[1][-1], segments[(i + 1) % len(segments)][1][0]) > 1e-7
               for i, segment in enumerate(segments)):
            raise ValueError(f"Open outline on {name}")
        axis_x, axis_y = zip(*outline)
        area = abs(sum(a[0] * b[1] - b[0] * a[1] for a, b in zip(outline, outline[1:] + outline[:1]))) / 2
        if not math.isfinite(area) or area < 1e-6:
            raise ValueError(f"Degenerate panel {name}")
        measurements[name] = {"segs": segments, "width_m": max(axis_x) - min(axis_x),
                              "height_m": max(axis_y) - min(axis_y), "area_m2": area, "lengths": lengths,
                              "translation_m": [coordinate / units for coordinate in panel["translation"]],
                              "source_rotation_deg": panel["rotation"]}
    stitches, assigned = [], Counter()
    for seam in pattern.pattern["stitches"]:
        first, second = seam[:2]
        a, ea = first["panel"], first["edge"]
        b, eb = second["panel"], second["edge"]
        if any(not 0 <= edge < len(measurements[name]["lengths"]) for name, edge in ((a, ea), (b, eb))):
            raise ValueError("Stitch refers to a missing edge")
        assigned.update(((a, ea), (b, eb)))
        length_a, length_b = measurements[a]["lengths"][ea], measurements[b]["lengths"][eb]
        stitches.append({"ends": [a, f"e{ea}", b, f"e{eb}"], "source_flags": seam[2:],
                         "lengths_m": [length_a, length_b], "relative_delta": abs(length_a - length_b) / max(length_a, length_b)})
    if any(count > 1 for count in assigned.values()):
        raise ValueError("A pattern edge is assigned to more than one stitch endpoint")
    return measurements, stitches


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=Path.home() / "AI/GarmentCodeRC")
    parser.add_argument("--body", choices=("mean_all", "mean_male", "mean_female"), default="mean_male")
    parser.add_argument("--design", choices=("t-shirt", "default_new"), default="default_new")
    parser.add_argument("--parameters", type=Path)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    repo = args.repo.resolve()
    sys.path.insert(0, str(repo))
    import yaml
    from assets.bodies.body_params import BodyParameters
    from assets.garment_programs.meta_garment import MetaGarment

    body = BodyParameters(str(repo / "assets/bodies" / f"{args.body}.yaml"))
    design = yaml.safe_load((repo / "assets/design_params" / f"{args.design}.yaml").read_text())["design"]
    if args.parameters:
        apply_parameters(design, json.loads(args.parameters.read_text()))
    garment = MetaGarment(args.out.name, body, design)
    garment.assert_non_empty()
    garment.assert_total_length()
    if garment.is_self_intersecting():
        raise ValueError("GarmentCodeRC reports a self-intersecting pattern")
    pattern = garment.assembly()
    panels, stitches = to_panels(pattern)
    args.out.mkdir(parents=True, exist_ok=False)
    destination = pattern.serialize(str(args.out), to_subfolder=False, with_3d=False,
                                    with_text=True, view_ids=True, with_printable=True)
    report = {"panels": panels, "stitches": stitches, "body": args.body, "design": args.design,
              "pattern_self_intersecting": False, "garment_length_cm": garment.length()}
    Path(destination, "panel-data.json").write_text(json.dumps(report, indent=2) + "\n")
    Path(destination, "design.yaml").write_text(yaml.safe_dump({"design": design}, sort_keys=False))
    print(f"{args.out.name}: {len(panels)} panels, {len(stitches)} stitches, {garment.length():.1f} cm garment length")


if __name__ == "__main__":
    main()
