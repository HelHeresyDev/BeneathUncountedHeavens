"""Transfer sampled GarmentCodeRC panels and seam IDs to isolated OpenSew2."""

import argparse
import json
from math import radians
from pathlib import Path
import sys
from time import perf_counter

import bpy
from mathutils import Euler, Matrix, Vector


def place_panel(piece, data):
    change_basis = Matrix(((1, 0, 0), (0, 0, -1), (0, 1, 0)))
    rotation = Euler(tuple(map(radians, data["source_rotation_deg"])), "XYZ").to_matrix()
    xs, ys = zip(*(point for _, points in data["segs"] for point in points))
    center = Vector(((min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2, 0))
    basis = (change_basis @ rotation @ change_basis.transposed()).to_4x4()
    basis.translation = change_basis @ (Vector(data["translation_m"]) + rotation @ center)
    piece.matrix_world = basis


def z_extents(obj):
    mesh = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    heights = [(mesh.matrix_world @ vertex.co).z for vertex in mesh.data.vertices]
    return [min(heights), max(heights)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--opensew", type=Path, required=True, help="Checkout containing clothing_design")
    parser.add_argument("--pattern", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--resolution", type=float, default=0.026)
    parser.add_argument("--sew", action="store_true")
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
    if args.out.exists():
        parser.error("Choose a new output path")
    source = json.loads(args.pattern.read_text())
    if any(stitch["source_flags"] for stitch in source["stitches"]):
        parser.error("This transfer has only been checked for ordinary stitching orientation")
    sys.path.insert(0, str(args.opensew.resolve()))
    import clothing_design
    from clothing_design import build, ops, patterns, sim

    clothing_design.register()
    context = bpy.context
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()
    if bpy.ops.cd.import_character(figure="MAN") != {"FINISHED"}:
        raise RuntimeError("OpenSew2 could not load the built-in fitting avatar")
    context.scene.cd.resolution = args.resolution
    name = "GarmentCodeRC"
    garment = build.draft_garment(context, {"name": name, "shoulder_pins": False,
                                         "pieces": [{"name": key, "anchor": "FREE", "segs": entry["segs"]}
                                                    for key, entry in source["panels"].items()],
                                         "seams": [stitch["ends"] for stitch in source["stitches"]]})
    transferred = build.garment_pieces(context, name)
    expected = {f"{name}_{key}": value for key, value in source["panels"].items()}
    assert {piece.name for piece in transferred} == set(expected)
    for piece in transferred:
        details = expected[piece.name]
        for label, _ in details["segs"]:
            assert len(patterns.segment_verts(piece, label)) >= 2, (piece.name, label)
        place_panel(piece, details)
    assert len(context.scene.cd.seams) == len(source["stitches"])
    context.view_layer.update()
    build.sync_garment(context, garment=name)
    report = {"blender": bpy.app.version_string, "avatar": context.scene.cd.avatar.name,
              "panels": len(transferred), "seams": len(context.scene.cd.seams),
              "stitch_springs": len(build.loose_edges(garment.data)), "initial_fit": sim.body_fit(context, garment),
              "avatar_z_m": z_extents(context.scene.cd.avatar), "initial_garment_z_m": z_extents(garment)}
    if not report["stitch_springs"]:
        raise RuntimeError("The transferred seams produced no stitch springs")
    if args.sew:
        context.scene.cd.stitch_frames = 25
        context.scene.cd.settle_frames = 20
        started = perf_counter()
        if bpy.ops.cd.sew() != {"FINISHED"}:
            raise RuntimeError("Blender did not finish the isolated OpenSew2 stitch test")
        report.update(sew=ops.simulation_status(), final_fit=sim.body_fit(context, garment),
                      remaining_loose_edges=len(build.loose_edges(garment.data)), seconds=round(perf_counter() - started, 2))
        if not sim.bake_shape(context, garment):
            raise RuntimeError("Could not bake the final fitting pose")
        for modifier in list(garment.modifiers):
            if modifier.type == "CLOTH":
                garment.modifiers.remove(modifier)
        report["baked_fit"] = sim.body_fit(context, garment)
        report["baked_garment_z_m"] = z_extents(garment)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(args.out.resolve()))
    args.out.with_suffix(".json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report), flush=True)


if __name__ == "__main__":
    main()
