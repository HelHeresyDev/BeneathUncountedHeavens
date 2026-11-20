import bpy, bmesh, json, sys, os
from mathutils import Vector

def script_args():
    return sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []

args = script_args()
out_path = args[args.index("--out") + 1]
entries = []
i = 0
while i < len(args):
    if args[i] == "--out":
        i += 2
        continue
    entries.append((args[i], args[i + 1]))
    i += 2

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

def load(path):
    clear_scene()
    if path.endswith(".blend"):
        with bpy.data.libraries.load(path, link=False) as (src, dst):
            dst.objects = list(src.objects)
        for obj in dst.objects:
            if obj is not None and obj.type == "MESH":
                bpy.context.scene.collection.objects.link(obj)
        meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
        retopo = [o for o in meshes if o.name.endswith("_Retopo")]
        return (retopo or meshes)[0]
    bpy.ops.import_scene.gltf(filepath=path)
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH" and o.name != "Cube"]
    if len(meshes) > 1:
        for o in bpy.context.scene.objects:
            o.select_set(o in meshes)
        bpy.context.view_layer.objects.active = meshes[0]
        bpy.ops.object.join()
    return bpy.context.view_layer.objects.active

def measure(obj):
    me = obj.data
    verts = len(me.vertices)
    faces = len(me.polygons)
    quads = sum(1 for p in me.polygons if len(p.vertices) == 4)
    tris = sum(1 for p in me.polygons if len(p.vertices) == 3)
    ngons = sum(1 for p in me.polygons if len(p.vertices) > 4)

    bm = bmesh.new()
    bm.from_mesh(me)
    bm.verts.ensure_lookup_table()
    visited = set()
    components = 0
    for v in bm.verts:
        if v.index in visited:
            continue
        components += 1
        stack = [v]
        while stack:
            cv = stack.pop()
            if cv.index in visited:
                continue
            visited.add(cv.index)
            for e in cv.link_edges:
                ov = e.other_vert(cv)
                if ov.index not in visited:
                    stack.append(ov)
    non_manifold = sum(1 for e in bm.edges if len(e.link_faces) != 2)
    open_edges = sum(1 for e in bm.edges if len(e.link_faces) == 1)
    degenerate = sum(1 for f in bm.faces if f.calc_area() < 1e-8)
    bm.free()

    uv_count = len(me.uv_layers)
    mats = [m.name for m in me.materials if m]
    textures = []
    for m in me.materials:
        if m and m.use_nodes:
            for n in m.node_tree.nodes:
                if n.type == "TEX_IMAGE" and n.image:
                    textures.append({"image": n.image.name, "size": [n.image.size[0], n.image.size[1]]})

    coords = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
    min_co = [min(c[i] for c in coords) for i in range(3)]
    max_co = [max(c[i] for c in coords) for i in range(3)]
    size = [max_co[i] - min_co[i] for i in range(3)]

    return {
        "vertices": verts,
        "faces": faces,
        "quads": quads,
        "tris": tris,
        "ngons": ngons,
        "components": components,
        "non_manifold_edges": non_manifold,
        "open_edges": open_edges,
        "degenerate_faces": degenerate,
        "uv_maps": uv_count,
        "materials": mats,
        "textures": textures,
        "bbox_min": min_co,
        "bbox_max": max_co,
        "bbox_size": size,
    }

results = {}
for label, path in entries:
    obj = load(path)
    results[label] = measure(obj)

with open(out_path, "w") as f:
    json.dump(results, f, indent=2)
print("MEASURE_DONE")
