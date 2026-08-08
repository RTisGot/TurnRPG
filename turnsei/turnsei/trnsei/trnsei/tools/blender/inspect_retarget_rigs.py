"""ゲームリグとMixamoリグの階層・レスト軸・骨長を比較出力する。"""

import bpy
import os

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
TARGET = os.path.join(ROOT, "Resource", "Character.blend")
SOURCE = r"C:\Users\rere0\Downloads\Stable Sword Inward Slash.fbx"


def dump(label, armature, names):
    print("===", label, "OBJECT", armature.name,
          "LOC", tuple(round(v, 5) for v in armature.location),
          "ROT", tuple(round(v, 5) for v in armature.rotation_euler),
          "SCALE", tuple(round(v, 5) for v in armature.scale))
    for name in names:
        bone = armature.data.bones.get(name)
        if bone is None:
            print("MISSING", name)
            continue
        parent = bone.parent.name if bone.parent else "-"
        head = tuple(round(v, 5) for v in bone.head_local)
        tail = tuple(round(v, 5) for v in bone.tail_local)
        matrix = bone.matrix_local.to_3x3()
        x_axis = tuple(round(v, 5) for v in matrix.col[0].normalized())
        y_axis = tuple(round(v, 5) for v in matrix.col[1].normalized())
        z_axis = tuple(round(v, 5) for v in matrix.col[2].normalized())
        print("BONE", name, "PARENT", parent, "LEN", round(bone.length, 5),
              "HEAD", head, "TAIL", tail, "X", x_axis, "Y", y_axis, "Z", z_axis)


bpy.ops.wm.open_mainfile(filepath=TARGET)
target = next(obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE")
before = set(bpy.context.scene.objects)
bpy.ops.import_scene.fbx(filepath=SOURCE)
source = next(obj for obj in bpy.context.scene.objects if obj not in before and obj.type == "ARMATURE")

dump("TARGET", target, (
    "Hips", "spine1", "spine2", "spine3", "neck",
    "shoulder.L", "uperarm.L", "lowerarm.L", "hand.L",
    "shoulder.R", "uperarm.R", "lowerarm.R", "hand.R",
    "uperleg.L", "lowerleg.L", "foot.L", "toe.L",
    "uperleg.R", "lowerleg.R", "foot.R", "toe.R",
))
dump("SOURCE", source, (
    "mixamorig:Hips", "mixamorig:Spine", "mixamorig:Spine1", "mixamorig:Spine2", "mixamorig:Neck",
    "mixamorig:LeftShoulder", "mixamorig:LeftArm", "mixamorig:LeftForeArm", "mixamorig:LeftHand",
    "mixamorig:RightShoulder", "mixamorig:RightArm", "mixamorig:RightForeArm", "mixamorig:RightHand",
    "mixamorig:LeftUpLeg", "mixamorig:LeftLeg", "mixamorig:LeftFoot", "mixamorig:LeftToeBase",
    "mixamorig:RightUpLeg", "mixamorig:RightLeg", "mixamorig:RightFoot", "mixamorig:RightToeBase",
))
