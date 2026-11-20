import bpy
import bmesh
import os
import sys

SOURCE_GLB = "/home/depressedtsukasa/Downloads/triposg_painted_00001_.glb"
OUTPUT_DIR = "/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/blender-tests/triposg_lantern_cleanup"


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for block_list in (bpy.data.meshes, bpy.data.materials, bpy.data.images, bpy.data.cameras, bpy.data.lights):
        for block in list(block_list):
            if block.users == 0:
                block_list.remove(block)


def add_baseline_camera_and_light():
    cam_data = bpy.data.cameras.new("BaselineCamera")
    cam_data.type = "ORTHO"
    cam_data.ortho_scale = 2.4
    cam = bpy.data.objects.new("BaselineCamera", cam_data)
    bpy.context.collection.objects.link(cam)
    cam.location = (0, -5, 0)
    cam.rotation_euler = (1.5708, 0, 0)

    light_data = bpy.data.lights.new("BaselineSun", "SUN")
    light_data.energy = 3.0
    light = bpy.data.objects.new("BaselineSun", light_data)
    bpy.context.collection.objects.link(light)
    light.location = (3, -4, 5)
    light.rotation_euler = (0.9, 0.2, 0.4)

    bpy.context.scene.camera = cam
    bpy.context.scene.render.engine = "BLENDER_EEVEE"
    bpy.context.scene.render.resolution_x = 1024
    bpy.context.scene.render.resolution_y = 1024


def inspect_mesh(obj):
    mesh = obj.data
    bm = bmesh.new()
    bm.from_mesh(mesh)
    stats = {
        "vertices": len(bm.verts),
        "edges": len(bm.edges),
        "faces": len(bm.faces),
        "non_manifold_edges": sum(1 for e in bm.edges if not e.is_manifold),
        "non_manifold_verts": sum(1 for v in bm.verts if not v.is_manifold),
        "zero_area_faces": sum(1 for f in bm.faces if f.calc_area() < 1e-8),
    }
    bm.free()

    bm = bmesh.new()
    bm.from_mesh(mesh)
    visited = set()
    islands = 0
    for v in bm.verts:
        if v.index in visited:
            continue
        islands += 1
        stack = [v]
        while stack:
            cur = stack.pop()
            if cur.index in visited:
                continue
            visited.add(cur.index)
            stack.extend(cur.link_verts)
    bm.free()
    stats["connected_islands"] = islands

    exact_duplicates = 0
    seen = set()
    for v in mesh.vertices:
        key = (round(v.co.x, 6), round(v.co.y, 6), round(v.co.z, 6))
        if key in seen:
            exact_duplicates += 1
        else:
            seen.add(key)
    stats["exact_duplicate_verts"] = exact_duplicates

    bb = [obj.matrix_world @ __import__("mathutils").Vector(c) for c in obj.bound_box]
    stats["world_min"] = [min(axis) for axis in zip(*bb)]
    stats["world_max"] = [max(axis) for axis in zip(*bb)]
    stats["world_size"] = [max(axis) - min(axis) for axis in zip(*bb)]
    return stats


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    clear_scene()
    bpy.ops.import_scene.gltf(filepath=SOURCE_GLB)
    obj = bpy.context.view_layer.objects.active
    if obj is None or obj.type != "MESH":
        raise RuntimeError("No mesh imported")
    obj.name = "Mesh_0"
    stats = inspect_mesh(obj)
    add_baseline_camera_and_light()
    bpy.context.scene.render.filepath = os.path.join(OUTPUT_DIR, "baseline_render.png")
    bpy.ops.render.render(write_still=True)
    bpy.ops.wm.save_mainfile(filepath=os.path.join(OUTPUT_DIR, "triposg_baseline.blend"))
    result = {"stats": stats}


if __name__ == "__main__":
    main()
