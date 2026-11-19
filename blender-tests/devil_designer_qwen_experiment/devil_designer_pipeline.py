import bpy
from bl_ext.blender_org.mpfb.services import HumanService
from bl_ext.blender_org.devil_designer import starters, arrangement, garment

scene = bpy.context.scene

# 1. Male/neutral avatar
basemesh = HumanService.create_human()

# 2. Sleeveless tunic pattern (cm measurements)
measurements = {
    "chest": 96.0,
    "waist": 80.0,
    "hip": 96.0,
    "torso_length": 40.0,
    "shoulder_width": 44.0,
}
doc = starters.create_tank_starter(
    measurements, fabric="COTTON", name="Tunic",
    extra_length=0.15, hem_flare=0.05,
)

# 3. Attach document + avatar
scene.dd_document = doc.dumps()
scene.dd_avatar = basemesh

# 4. Fit body volumes (synchronous; required headlessly)
arrangement.ensure_volumes(bpy.context, force=True)

# 5. Build panels around the body, assemble the sewn simulation
garment.build_panels(bpy.context)
garment.compile_garment(bpy.context)

# 6. Let the cloth drape (manual frame advance in background mode)
for f in range(1, 121):
    scene.frame_set(f)

# 7. Refine, weld seams, bake the final static mesh
garment.keep_drape(bpy.context)
