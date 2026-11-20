import bpy
import os

OUTPUT_DIR = "/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/blender-tests/triposg_lantern_cleanup"
CONSERVATIVE_OBJ = "Lantern_Conservative"
PRODUCTION_OBJ = "Lantern_Production"
UCX_NAMES = ["UCX_Lantern_00", "UCX_Lantern_01", "UCX_Lantern_02"]


def export_selection(filepath, objects):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.hide_set(False)
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.export_scene.gltf(filepath=filepath, use_selection=True, export_apply=True)
    for obj in objects:
        if obj.name != PRODUCTION_OBJ:
            obj.hide_set(True)


def verify_import(filepath):
    before = set(bpy.data.objects.keys())
    bpy.ops.import_scene.gltf(filepath=filepath)
    new = [n for n in set(bpy.data.objects.keys()) - before]
    info = []
    for n in new:
        ob = bpy.data.objects[n]
        if ob.type == "MESH":
            info.append({
                "name": n,
                "vertices": len(ob.data.vertices),
                "faces": len(ob.data.polygons),
                "materials": [m.name for m in ob.data.materials],
            })
        bpy.data.objects.remove(ob, do_unlink=True)
    return info


def main():
    cons = bpy.data.objects.get(CONSERVATIVE_OBJ)
    prod = bpy.data.objects.get(PRODUCTION_OBJ)
    ucx = [bpy.data.objects.get(n) for n in UCX_NAMES if bpy.data.objects.get(n)]
    if cons is None or prod is None:
        raise RuntimeError("Conservative or production object missing")

    export_selection(os.path.join(OUTPUT_DIR, "triposg_conservative.glb"), [cons])
    export_selection(os.path.join(OUTPUT_DIR, "triposg_production.glb"), [prod] + ucx)

    result = {
        "conservative_reimport": verify_import(os.path.join(OUTPUT_DIR, "triposg_conservative.glb")),
        "production_reimport": verify_import(os.path.join(OUTPUT_DIR, "triposg_production.glb")),
    }


if __name__ == "__main__":
    main()
