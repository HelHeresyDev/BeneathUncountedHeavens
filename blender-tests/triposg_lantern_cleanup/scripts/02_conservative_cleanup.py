import bpy
import os

OUTPUT_DIR = "/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/blender-tests/triposg_lantern_cleanup"
SOURCE_OBJ = "Mesh_0"
CONSERVATIVE_OBJ = "Lantern_Conservative"


def cleanup_mesh(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.hide_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")

    bpy.ops.mesh.delete(type="FACE")
    bpy.ops.mesh.select_non_manifold()
    bpy.ops.mesh.delete(type="FACE")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.remove_doubles(threshold=0.0001)
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.delete(type="FACE")
    bpy.ops.object.mode_set(mode="OBJECT")


def set_origin_to_bottom_center(obj):
    from mathutils import Vector
    bpy.ops.object.select_all(action="DESELECT")
    obj.hide_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bb = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
    minz = min(v.z for v in bb)
    minx = min(v.x for v in bb)
    maxx = max(v.x for v in bb)
    miny = min(v.y for v in bb)
    maxy = max(v.y for v in bb)
    bpy.context.scene.cursor.location = ((minx + maxx) / 2, (miny + maxy) / 2, minz)
    bpy.ops.object.origin_set(type="ORIGIN_CURSOR")


def main():
    source = bpy.data.objects.get(SOURCE_OBJ)
    if source is None:
        raise RuntimeError(f"Source object {SOURCE_OBJ} not found")
    if bpy.data.objects.get(CONSERVATIVE_OBJ) is not None:
        bpy.data.objects.remove(bpy.data.objects[CONSERVATIVE_OBJ], do_unlink=True)

    source.hide_set(False)
    bpy.ops.object.select_all(action="DESELECT")
    source.select_set(True)
    bpy.context.view_layer.objects.active = source
    bpy.ops.object.duplicate()
    cons = bpy.context.view_layer.objects.active
    cons.name = CONSERVATIVE_OBJ
    cleanup_mesh(cons)
    set_origin_to_bottom_center(cons)
    source.hide_set(True)
    bpy.ops.wm.save_mainfile(filepath=os.path.join(OUTPUT_DIR, "triposg_conservative.blend"))
    result = {
        "vertices": len(cons.data.vertices),
        "faces": len(cons.data.polygons),
    }


if __name__ == "__main__":
    main()
