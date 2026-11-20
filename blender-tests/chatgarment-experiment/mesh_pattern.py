"""Run GarmentCodeRC's CPU BoxMesh triangulation and save a Z-up OBJ in metres."""

import argparse
import json
from pathlib import Path
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=Path.home() / "AI/GarmentCodeRC")
    parser.add_argument("--pattern", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    sys.path.insert(0, str(args.repo.resolve()))
    from pygarment.meshgen.boxmeshgen import BoxMesh

    mesh = BoxMesh(args.pattern.resolve(), res=1.0)
    mesh.load()
    if not mesh.loaded or len(mesh.vertices) == 0 or len(mesh.faces) == 0:
        raise RuntimeError("BoxMesh did not produce usable geometry")
    with args.out.open("x") as output:
        for x, y, z in mesh.vertices:
            output.write(f"v {x / 100} {-z / 100} {y / 100}\n")
        for face in mesh.faces:
            output.write("f " + " ".join(str(int(index) + 1) for index in face) + "\n")
    print(json.dumps({"vertices": len(mesh.vertices), "faces": len(mesh.faces), "stitches": len(mesh.stitches)}))


if __name__ == "__main__":
    main()
