import bpy, sys, os
from mathutils import Vector

def script_args():
    return sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []

args = script_args()
original = args[args.index("--original") + 1]
fixed = args[args.index("--fixed") + 1]
out_dir = args[args.index("--out-dir") + 1]
os.makedirs(out_dir, exist_ok=True)

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
    for block in list(bpy.data.cameras):
        if block.users == 0:
            bpy.data.cameras.remove(block)
    for block in list(bpy.data.lights):
        if block.users == 0:
            bpy.data.lights.remove(block)

def import_mesh(path):
    bpy.ops.import_scene.gltf(filepath=path)
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    for o in bpy.context.scene.objects:
        if o.type == "MESH" and o.name == "Cube":
            bpy.data.objects.remove(o, do_unlink=True)
            meshes = [m for m in meshes if m.name != "Cube"]
    if len(meshes) > 1:
        for o in bpy.context.scene.objects:
            o.select_set(o in meshes)
        bpy.context.view_layer.objects.active = meshes[0]
        bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    return obj

def bbox_center_radius(obj):
    coords = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
    center = sum(coords, Vector()) / len(coords)
    radius = max((c - center).length for c in coords)
    return center, radius

def add_light(scene):
    light_data = bpy.data.lights.new("Sun", type="SUN")
    light_data.energy = 3.0
    light = bpy.data.objects.new("Sun", light_data)
    scene.collection.objects.link(light)
    light.rotation_euler = (0.8, 0.2, 0.5)

def render_angles(obj, prefix, out_dir, center, radius):
    add_light(bpy.context.scene)
    target = bpy.data.objects.new("Target", None)
    bpy.context.scene.collection.objects.link(target)
    target.location = center
    angles = {
        "front": (0, -3.2, 0.2),
        "three_quarter": (2.2, -2.2, 1.2),
        "top": (0.0, -0.2, 3.2),
    }
    bpy.context.scene.render.engine = "BLENDER_WORKBENCH"
    bpy.context.scene.render.resolution_x = 1024
    bpy.context.scene.render.resolution_y = 1024
    bpy.context.scene.render.image_settings.file_format = "PNG"
    for name, offset in angles.items():
        cam_data = bpy.data.cameras.new(f"Camera_{name}")
        cam = bpy.data.objects.new(f"Camera_{name}", cam_data)
        bpy.context.scene.collection.objects.link(cam)
        cam.location = center + Vector(offset) * radius
        constraint = cam.constraints.new("TRACK_TO")
        constraint.target = target
        constraint.track_axis = "TRACK_NEGATIVE_Z"
        constraint.up_axis = "UP_Y"
        bpy.context.scene.camera = cam
        bpy.context.scene.render.filepath = os.path.join(out_dir, f"{prefix}_{name}.png")
        bpy.ops.render.render(write_still=True)
        bpy.data.objects.remove(cam, do_unlink=True)
    bpy.data.objects.remove(target, do_unlink=True)

clear_scene()
obj = import_mesh(original)
center, radius = bbox_center_radius(obj)
render_angles(obj, "original", out_dir, center, radius)

clear_scene()
obj = import_mesh(fixed)
center, radius = bbox_center_radius(obj)
render_angles(obj, "fixed", out_dir, center, radius)

print("RENDER_COMPARISON_DONE")
