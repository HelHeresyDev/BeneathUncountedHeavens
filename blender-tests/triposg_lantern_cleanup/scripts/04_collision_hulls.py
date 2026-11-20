import bpy
import bmesh
import os
from bmesh.types import BMVert, BMFace
from mathutils import Vector

OUTPUT_DIR = "/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/blender-tests/triposg_lantern_cleanup"
SOURCE_OBJ = "Lantern_Production"
UCX_NAMES = ["UCX_Lantern_00", "UCX_Lantern_01", "UCX_Lantern_02"]
Z_SLICES = [(-1.0, 0.30), (0.30, 1.35), (1.35, 3.0)]


def make_convex_hull_object(name, source_obj, z_min, z_max):
    mesh = bpy.data.meshes.new(name)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)

    bm = bmesh.new()
    bm.from_mesh(source_obj.data)
    for v in list(bm.verts):
        world_z = (source_obj.matrix_world @ v.co).z
        if world_z < z_min or world_z > z_max:
            bm.verts.remove(v)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.0001)
    if len(bm.verts) < 4:
        bm.free()
        bpy.data.objects.remove(obj, do_unlink=True)
        bpy.data.meshes.remove(mesh)
        return None

    geom = bm.verts[:] + bm.edges[:] + bm.faces[:]
    res = bmesh.ops.convex_hull(bm, input=geom, use_existing_faces=False)
    hull_faces = {e for e in res["geom"] if isinstance(e, BMFace)}
    for f in list(bm.faces):
        if f not in hull_faces:
            bm.faces.remove(f)
    bmesh.ops.delete(bm, geom=bm.verts[:], context="VERTS")
    bm.to_mesh(mesh)
    bm.free()

    obj.hide_render = True
    obj.display_type = "WIRE"
    return obj


def main():
    source = bpy.data.objects.get(SOURCE_OBJ)
    if source is None:
        raise RuntimeError(f"Source object {SOURCE_OBJ} not found")
    for name in UCX_NAMES:
        old = bpy.data.objects.get(name)
        if old is not None:
            bpy.data.objects.remove(old, do_unlink=True)
    created = []
    for name, (z_min, z_max) in zip(UCX_NAMES, Z_SLICES):
        obj = make_convex_hull_object(name, source, z_min, z_max)
        if obj is not None:
            created.append({"name": name, "vertices": len(obj.data.vertices), "faces": len(obj.data.polygons)})
    bpy.ops.wm.save_mainfile(filepath=os.path.join(OUTPUT_DIR, "triposg_production.blend"))
    result = {"created": created}


if __name__ == "__main__":
    main()
