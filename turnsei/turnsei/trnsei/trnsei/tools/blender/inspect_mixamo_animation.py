"""Mixamo FBXの骨格・アクション情報をリターゲット前に検査する。"""

import bpy
import os
import sys


def argument_after_double_dash():
    argv = sys.argv
    return argv[argv.index("--") + 1] if "--" in argv else ""


source = os.path.abspath(argument_after_double_dash())
if not os.path.isfile(source):
    raise FileNotFoundError(source)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=source)

for obj in bpy.context.scene.objects:
    if obj.type != "ARMATURE":
        continue
    action = obj.animation_data.action if obj.animation_data else None
    print("ARMATURE", obj.name)
    print("BONES", "|".join(bone.name for bone in obj.data.bones))
    if action:
        print("ACTION", action.name, "RANGE", tuple(action.frame_range), "FPS", bpy.context.scene.render.fps)
