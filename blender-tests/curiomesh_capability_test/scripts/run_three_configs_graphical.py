import sys, os, importlib.util, shutil
from pathlib import Path

WORKSPACE = Path("/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/blender-tests/curiomesh_capability_test")
INPUT = WORKSPACE / "input" / "triposg_painted_00001_.glb"
OUTPUT_DIR = WORKSPACE / "output"
OUTPUT_DIR.mkdir(exist_ok=True)

SKILL_SCRIPT = Path("/home/depressedtsukasa/.config/opencode/skills/curiomesh-retopo/scripts/curiomesh_run.py")
spec = importlib.util.spec_from_file_location("curiomesh_run", SKILL_SCRIPT)
curiomesh_run = importlib.util.module_from_spec(spec)
spec.loader.exec_module(curiomesh_run)

import bpy

def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for block in list(bpy.data.meshes):
        if block.users == 0:
            bpy.data.meshes.remove(block)
    for block in list(bpy.data.materials):
        if block.users == 0:
            bpy.data.materials.remove(block)
    for block in list(bpy.data.images):
        if block.users == 0:
            bpy.data.images.remove(block)

configs = [
    ("high_fidelity", {"use": "GAME", "detail": "2"}),
    ("balanced", {"use": "GAME", "detail": "5"}),
    ("aggressive", {"use": "GAME", "detail": "10"}),
]

for name, opts in configs:
    print(f"CONFIG_START {name}", flush=True)
    clear_scene()
    out_glb = OUTPUT_DIR / f"{name}.glb"
    report = OUTPUT_DIR / f"{name}.json"
    argv = ["blender", "--python", "wrapper.py", "--", str(INPUT), "--use", opts["use"], "--detail", opts["detail"], "--out", str(out_glb), "--report", str(report)]
    sys.argv = argv
    code = curiomesh_run.main()
    print(f"CONFIG_DONE {name} code={code}", flush=True)
    if code == 0:
        blend_path = OUTPUT_DIR / f"{name}.blend"
        bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))
        print(f"BLEND_SAVED {blend_path}", flush=True)
    clear_scene()

print("ALL_CONFIGS_DONE", flush=True)
