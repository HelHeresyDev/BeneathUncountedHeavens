import bpy, sys, json, math, bmesh
from mathutils import Vector

def script_args():
    return sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []

args = script_args()
glb_path = None
for i, a in enumerate(args):
    if a == "--glb" and i + 1 < len(args):
        glb_path = args[i + 1]

if glb_path:
    bpy.ops.import_scene.gltf(filepath=glb_path)

meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
if not meshes:
    print("INSPECT_RESULT " + json.dumps({"error": "no mesh objects found"}, default=str))
    sys.exit(0)

def mesh_stats(obj):
    me = obj.data
    verts = len(me.vertices)
    faces = len(me.polygons)
    quads = sum(1 for p in me.polygons if len(p.vertices) == 4)
    tris = sum(1 for p in me.polygons if len(p.vertices) == 3)
    ngons = sum(1 for p in me.polygons if len(p.vertices) > 4)
    bm = bmesh.new()
    bm.from_mesh(me)
    open_edges = sum(1 for e in bm.edges if len(e.link_faces) == 1)
    non_manifold = sum(1 for e in bm.edges if len(e.link_faces) > 2)
    bm.free()
    degenerate = sum(1 for p in me.polygons if len(set(p.vertices)) < 3 or p.area < 1e-10)
    uv_layers = [uv.name for uv in me.uv_layers]
    mat_slots = [s.material.name if s.material else None for s in obj.material_slots]
    bbox = [Vector(c) for c in obj.bound_box]
    min_x = min(v.x for v in bbox); max_x = max(v.x for v in bbox)
    min_y = min(v.y for v in bbox); max_y = max(v.y for v in bbox)
    min_z = min(v.z for v in bbox); max_z = max(v.z for v in bbox)
    # connected components via union-find on vertices
    parent = list(range(verts))
    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x
    def union(a, b):
        ra, rb = find(a), find(b)
        if ra != rb:
            parent[ra] = rb
    for e in me.edges:
        union(e.vertices[0], e.vertices[1])
    islands = len(set(find(i) for i in range(verts)))
    return {
        "object": obj.name,
        "vertices": verts,
        "faces": faces,
        "quads": quads,
        "tris": tris,
        "ngons": ngons,
        "open_edges": open_edges,
        "non_manifold_edges": non_manifold,
        "degenerate_faces": degenerate,
        "uv_layers": uv_layers,
        "materials": mat_slots,
        "bounding_box": {"min": [min_x, min_y, min_z], "max": [max_x, max_y, max_z]},
        "connected_components": islands,
    }

def material_info():
    out = []
    seen = set()
    for obj in meshes:
        for slot in obj.material_slots:
            mat = slot.material
            if mat is None or mat.name in seen:
                continue
            seen.add(mat.name)
            images = []
            if mat.use_nodes:
                for n in mat.node_tree.nodes:
                    if n.type == "TEX_IMAGE" and n.image:
                        img = n.image
                        images.append({"name": img.name, "size": [img.size[0], img.size[1]], "file": img.filepath})
            out.append({"name": mat.name, "images": images})
    return out

result = {
    "meshes": [mesh_stats(o) for o in meshes],
    "materials": material_info(),
    "object_count": len(bpy.context.scene.objects),
    "mesh_count": len(meshes),
}
print("INSPECT_RESULT " + json.dumps(result, default=str))
