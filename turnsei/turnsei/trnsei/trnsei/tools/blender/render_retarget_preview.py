"""リターゲット結果の主要フレームを正面からレンダリングする検証用ツール。"""

import bpy
import math
import os
from mathutils import Vector

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SOURCE = os.path.join(ROOT, "Resource", "Character_CombatAnimation.blend")
OUTPUT_DIR = os.path.join(ROOT, "Resource", "AnimationPreview")
FRAMES = (0, 15, 35, 53, 72, 90)

bpy.ops.wm.open_mainfile(filepath=SOURCE)
os.makedirs(OUTPUT_DIR, exist_ok=True)
armatures = [obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE"]
target = next(obj for obj in armatures if obj.animation_data and obj.animation_data.action and
              obj.animation_data.action.name.startswith("Basic_Attack_Mixamo"))

# 読み込んだMixamo参照モデルは検証画像へ混ぜない。
for obj in bpy.context.scene.objects:
    if obj.type == "ARMATURE" and obj != target:
        obj.hide_render = True
        for child in obj.children_recursive:
            child.hide_render = True

camera_data = bpy.data.cameras.new("RetargetPreviewCamera")
camera = bpy.data.objects.new("RetargetPreviewCamera", camera_data)
bpy.context.collection.objects.link(camera)
bpy.context.scene.camera = camera
camera.location = (0.0, -7.5, 1.55)
direction = Vector((0.0, 0.0, 1.25)) - camera.location
camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
camera_data.lens = 58

world = bpy.context.scene.world or bpy.data.worlds.new("PreviewWorld")
bpy.context.scene.world = world
world.color = (0.025, 0.035, 0.05)
for location, energy, size in (((-3, -4, 6), 1100, 4), ((4, -1, 3), 800, 3)):
    light_data = bpy.data.lights.new("PreviewArea", "AREA")
    light_data.energy = energy
    light_data.shape = "DISK"
    light_data.size = size
    light = bpy.data.objects.new("PreviewArea", light_data)
    bpy.context.collection.objects.link(light)
    light.location = location
    light.rotation_euler = (math.radians(25), 0, math.radians(20))

scene = bpy.context.scene
scene.render.engine = "BLENDER_EEVEE_NEXT"
scene.render.resolution_x = 480
scene.render.resolution_y = 720
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.render.film_transparent = False

for frame in FRAMES:
    scene.frame_set(frame)
    scene.render.filepath = os.path.join(OUTPUT_DIR, f"attack_{frame:03d}.png")
    bpy.ops.render.render(write_still=True)
    print("RENDERED", scene.render.filepath)
