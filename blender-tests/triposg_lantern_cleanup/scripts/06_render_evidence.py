import bpy
import math
import os
from mathutils import Vector

OUTPUT_DIR = "/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/blender-tests/triposg_lantern_cleanup"
CAM_NAME = "DiagCam"
VIEWS = {
    "front": ((0, -5, 0.9), (0, 0, 0.9)),
    "side_right": ((5, 0, 0.9), (0, 0, 0.9)),
    "rear": ((0, 5, 0.9), (0, 0, 0.9)),
    "three_quarter": ((-4, -4, 1.6), (0, 0, 0.9)),
}


def ensure_camera():
    cam = bpy.data.objects.get(CAM_NAME)
    if cam is None:
        cam_data = bpy.data.cameras.new(CAM_NAME)
        cam = bpy.data.objects.new(CAM_NAME, cam_data)
        bpy.context.collection.objects.link(cam)
    cam.data.type = "ORTHO"
    cam.data.ortho_scale = 2.4
    return cam


def aim_at(cam, target):
    d = Vector(target) - cam.location
    cam.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()


def render_views(obj, prefix):
    cam = ensure_camera()
    rendered = []
    bpy.ops.object.select_all(action="DESELECT")
    for name, (loc, tgt) in VIEWS.items():
        cam.location = loc
        aim_at(cam, tgt)
        bpy.context.scene.camera = cam
        obj.hide_set(False)
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        filepath = os.path.join(OUTPUT_DIR, f"{prefix}_{name}.png")
        bpy.context.scene.render.filepath = filepath
        bpy.ops.render.render(write_still=True)
        rendered.append(os.path.basename(filepath))
    mod = obj.modifiers.new("WireframeTmp", "WIREFRAME")
    mod.thickness = 0.01
    cam.location = VIEWS["front"][0]
    aim_at(cam, VIEWS["front"][1])
    bpy.context.scene.camera = cam
    filepath = os.path.join(OUTPUT_DIR, f"{prefix}_wireframe_front.png")
    bpy.context.scene.render.filepath = filepath
    bpy.ops.render.render(write_still=True)
    obj.modifiers.remove(mod)
    obj.hide_set(True)
    rendered.append(os.path.basename(filepath))
    return rendered


def main():
    bpy.context.scene.render.engine = "BLENDER_EEVEE"
    bpy.context.scene.render.resolution_x = 1024
    bpy.context.scene.render.resolution_y = 1024
    result = {}
    for obj_name, prefix in [("Mesh_0", "original"), ("Lantern_Conservative", "conservative"), ("Lantern_Production", "production")]:
        obj = bpy.data.objects.get(obj_name)
        if obj is not None:
            result[prefix] = render_views(obj, prefix)
    bpy.ops.wm.save_mainfile(filepath=os.path.join(OUTPUT_DIR, "triposg_production.blend"))


if __name__ == "__main__":
    main()
