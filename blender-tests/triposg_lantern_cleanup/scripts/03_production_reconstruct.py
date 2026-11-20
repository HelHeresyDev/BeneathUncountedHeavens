import bpy
import bmesh
import math
import os
from mathutils import Vector

OUTPUT_DIR = "/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/blender-tests/triposg_lantern_cleanup"
PRODUCTION_OBJ = "Lantern_Production"
MATERIAL_NAME = "M_StoneLantern"
TARGET_HEIGHT = 1.8


def add_cube(name, size, location):
    bpy.ops.mesh.primitive_cube_add(size=1, location=location)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = (size[0], size[1], size[2])
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return obj


def add_cylinder(name, radius, depth, location, vertices=16):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=location)
    obj = bpy.context.active_object
    obj.name = name
    return obj


def add_uv_sphere(name, radius, location, segments=16, ring_count=8):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=ring_count, radius=radius, location=location)
    obj = bpy.context.active_object
    obj.name = name
    return obj


def add_cone(name, radius1, depth, location, vertices=4):
    bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=radius1, radius2=0, depth=depth, location=location)
    obj = bpy.context.active_object
    obj.name = name
    return obj


def build_lantern_parts():
    parts = []
    parts.append(add_cube("Plinth", (1.2, 1.2, 0.2), (0, 0, 0.1)))
    parts.append(add_cube("Step", (0.95, 0.95, 0.12), (0, 0, 0.26)))
    for i, (x, y) in enumerate([(-0.38, -0.38), (0.38, -0.38), (-0.38, 0.38), (0.38, 0.38)]):
        parts.append(add_cube(f"Pillar_{i}", (0.16, 0.16, 0.6), (x, y, 0.62)))
    parts.append(add_cube("Chamber", (0.55, 0.55, 0.4), (0, 0, 1.12)))
    parts.append(add_cube("RoofPlate", (1.0, 1.0, 0.1), (0, 0, 1.3029)))
    roof = add_cone("RoofCone", 1.2, 0.4, (0, 0, 1.5529), vertices=4)
    roof.rotation_euler = (0, 0, math.radians(45))
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    parts.append(roof)
    parts.append(add_cylinder("FinialCyl", 0.08, 0.08, (0, 0, 1.8029), vertices=12))
    parts.append(add_uv_sphere("FinialBall", 0.09, (0, 0, 1.8529), segments=16, ring_count=8))
    return parts


def join_parts(parts):
    bpy.ops.object.select_all(action="DESELECT")
    for p in parts:
        p.hide_set(False)
        p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    return bpy.context.active_object


def clean_and_finalize(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.hide_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    mod = obj.modifiers.new("Bevel", "BEVEL")
    mod.width = 0.008
    mod.segments = 1
    mod.limit_method = "ANGLE"
    mod.angle_limit = math.radians(30)
    bpy.ops.object.modifier_apply(modifier="Bevel")

    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.remove_doubles(threshold=0.0001)
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode="OBJECT")

    from mathutils import Vector
    bb = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
    minz = min(v.z for v in bb)
    maxz = max(v.z for v in bb)
    height = maxz - minz
    scale_factor = TARGET_HEIGHT / height
    obj.scale = (scale_factor, scale_factor, scale_factor)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

    bb = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
    minz = min(v.z for v in bb)
    minx = min(v.x for v in bb)
    maxx = max(v.x for v in bb)
    miny = min(v.y for v in bb)
    maxy = max(v.y for v in bb)
    bpy.context.scene.cursor.location = ((minx + maxx) / 2, (miny + maxy) / 2, minz)
    bpy.ops.object.origin_set(type="ORIGIN_CURSOR")

    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")


def create_stone_material():
    if bpy.data.materials.get(MATERIAL_NAME):
        return bpy.data.materials[MATERIAL_NAME]
    mat = bpy.data.materials.new(MATERIAL_NAME)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()
    output = nodes.new("ShaderNodeOutputMaterial")
    output.location = (600, 0)
    bsdf = nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.location = (300, 0)
    bsdf.inputs["Metallic"].default_value = 0.0
    bsdf.inputs["Roughness"].default_value = 0.85
    links.new(bsdf.outputs["BSDF"], output.inputs["Surface"])

    texco = nodes.new("ShaderNodeTexCoord")
    texco.location = (-800, 0)
    noise1 = nodes.new("ShaderNodeTexNoise")
    noise1.location = (-600, 200)
    noise1.inputs["Scale"].default_value = 8.0
    noise1.inputs["Detail"].default_value = 6.0
    links.new(texco.outputs["Object"], noise1.inputs["Vector"])
    ramp = nodes.new("ShaderNodeValToRGB")
    ramp.location = (-400, 200)
    ramp.color_ramp.elements[0].color = (0.16, 0.145, 0.13, 1.0)
    ramp.color_ramp.elements[1].color = (0.34, 0.32, 0.29, 1.0)
    links.new(noise1.outputs["Fac"], ramp.inputs["Fac"])
    links.new(ramp.outputs["Color"], bsdf.inputs["Base Color"])

    noise2 = nodes.new("ShaderNodeTexNoise")
    noise2.location = (-600, -200)
    noise2.inputs["Scale"].default_value = 25.0
    noise2.inputs["Detail"].default_value = 8.0
    links.new(texco.outputs["Object"], noise2.inputs["Vector"])
    bump = nodes.new("ShaderNodeBump")
    bump.location = (-300, -200)
    bump.inputs["Strength"].default_value = 0.18
    bump.inputs["Distance"].default_value = 0.02
    links.new(noise2.outputs["Fac"], bump.inputs["Height"])
    links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


def main():
    if bpy.data.objects.get(PRODUCTION_OBJ) is not None:
        bpy.data.objects.remove(bpy.data.objects[PRODUCTION_OBJ], do_unlink=True)
    parts = build_lantern_parts()
    obj = join_parts(parts)
    obj.name = PRODUCTION_OBJ
    clean_and_finalize(obj)
    mat = create_stone_material()
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    bpy.ops.wm.save_mainfile(filepath=os.path.join(OUTPUT_DIR, "triposg_production.blend"))
    result = {
        "vertices": len(obj.data.vertices),
        "faces": len(obj.data.polygons),
    }


if __name__ == "__main__":
    main()
