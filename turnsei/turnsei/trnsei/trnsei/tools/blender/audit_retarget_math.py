"""Mixamo→ゲームリグの回転転送と関節方向を非破壊で監査する。

Blenderをバックグラウンド実行し、ファイルは保存せず標準出力だけを生成する。
"""

import bpy
import math
import os
import sys
from mathutils import Quaternion


ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
TARGET_BLEND = os.path.join(ROOT, "Resource", "Character.blend")

BONE_MAP = {
    "mixamorig:Hips": "Hips",
    "mixamorig:Spine": "spine1",
    "mixamorig:Spine1": "spine2",
    "mixamorig:Spine2": "spine3",
    "mixamorig:LeftArm": "uperarm.L",
    "mixamorig:LeftForeArm": "lowerarm.L",
    "mixamorig:LeftHand": "hand.L",
    "mixamorig:RightArm": "uperarm.R",
    "mixamorig:RightForeArm": "lowerarm.R",
    "mixamorig:RightHand": "hand.R",
    "mixamorig:LeftUpLeg": "uperleg.L",
    "mixamorig:LeftLeg": "lowerleg.L",
    "mixamorig:LeftFoot": "foot.L",
    "mixamorig:RightUpLeg": "uperleg.R",
    "mixamorig:RightLeg": "lowerleg.R",
    "mixamorig:RightFoot": "foot.R",
}


def argument_after_double_dash():
    argv = sys.argv
    return os.path.abspath(argv[argv.index("--") + 1]) if "--" in argv else ""


def angle_degrees(a, b):
    # qと-qは同じ姿勢なので、内積の絶対値から最短角だけを測る。
    dot = max(-1.0, min(1.0, abs(a.normalized().dot(b.normalized()))))
    return math.degrees(2.0 * math.acos(dot))


def corrected_target_world_rotation(source, target, source_name, target_name):
    """レスト空間の差分をターゲットのレスト姿勢へ右乗算する。"""
    source_rest = (source.matrix_world @ source.data.bones[source_name].matrix_local).to_quaternion()
    source_pose = (source.matrix_world @ source.pose.bones[source_name].matrix).to_quaternion()
    target_rest = (target.matrix_world @ target.data.bones[target_name].matrix_local).to_quaternion()
    return (target_rest @ (source_rest.inverted() @ source_pose)).normalized()


def old_target_world_rotation(source, target, source_name, target_name):
    """現行実装の世界空間左乗算。比較専用。"""
    source_rest = (source.matrix_world @ source.data.bones[source_name].matrix_local).to_quaternion()
    source_pose = (source.matrix_world @ source.pose.bones[source_name].matrix).to_quaternion()
    target_rest = (target.matrix_world @ target.data.bones[target_name].matrix_local).to_quaternion()
    return ((source_pose @ source_rest.inverted()) @ target_rest).normalized()


source_path = argument_after_double_dash()
if not os.path.isfile(source_path):
    raise FileNotFoundError("Usage: blender --background --python audit_retarget_math.py -- <mixamo.fbx>")

bpy.ops.wm.open_mainfile(filepath=TARGET_BLEND)
target = next(obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE")
before = set(bpy.context.scene.objects)
bpy.ops.import_scene.fbx(filepath=source_path)
source = next(obj for obj in bpy.context.scene.objects if obj not in before and obj.type == "ARMATURE")
action = source.animation_data.action
first, last = (int(round(value)) for value in action.frame_range)

print("AUDIT_SOURCE", source_path)
print("FRAME_RANGE", first, last)
for target_name in ("uperleg.L", "uperleg.R", "shoulder.L", "shoulder.R"):
    bone = target.data.bones[target_name]
    print("TARGET_PARENT", target_name, bone.parent.name if bone.parent else "<ROOT>")

max_difference = {target_name: 0.0 for target_name in BONE_MAP.values()}
max_step = {target_name: 0.0 for target_name in BONE_MAP.values()}
previous = {}
source_joint_ranges = {name: [180.0, 0.0] for name in ("elbow.L", "elbow.R", "knee.L", "knee.R")}

for frame in range(first, last + 1):
    bpy.context.scene.frame_set(frame)
    bpy.context.view_layer.update()
    for source_name, target_name in BONE_MAP.items():
        if source_name not in source.pose.bones or target_name not in target.pose.bones:
            continue
        corrected = corrected_target_world_rotation(source, target, source_name, target_name)
        old = old_target_world_rotation(source, target, source_name, target_name)
        max_difference[target_name] = max(max_difference[target_name], angle_degrees(old, corrected))
        if target_name in previous:
            max_step[target_name] = max(max_step[target_name], angle_degrees(previous[target_name], corrected))
        previous[target_name] = corrected

    for label, upper, lower in (
        ("elbow.L", "mixamorig:LeftArm", "mixamorig:LeftForeArm"),
        ("elbow.R", "mixamorig:RightArm", "mixamorig:RightForeArm"),
        ("knee.L", "mixamorig:LeftUpLeg", "mixamorig:LeftLeg"),
        ("knee.R", "mixamorig:RightUpLeg", "mixamorig:RightLeg"),
    ):
        upper_vector = (source.pose.bones[upper].tail - source.pose.bones[upper].head).normalized()
        lower_vector = (source.pose.bones[lower].tail - source.pose.bones[lower].head).normalized()
        bend = math.degrees(upper_vector.angle(lower_vector))
        source_joint_ranges[label][0] = min(source_joint_ranges[label][0], bend)
        source_joint_ranges[label][1] = max(source_joint_ranges[label][1], bend)

for target_name in BONE_MAP.values():
    if max_difference[target_name] > 0.0:
        print("ROTATION_AUDIT", target_name,
              "OLD_VS_CORRECT_MAX_DEG", round(max_difference[target_name], 3),
              "CORRECT_MAX_STEP_DEG", round(max_step[target_name], 3))
for label, limits in source_joint_ranges.items():
    print("SOURCE_JOINT_RANGE", label, "MIN_DEG", round(limits[0], 3), "MAX_DEG", round(limits[1], 3))

print("RECOMMENDED_HINGE_LIMIT knee extension=0 flexion=140 swing=8")
print("RECOMMENDED_HINGE_LIMIT elbow extension=0 flexion=145 swing=10")
print("AUDIT_COMPLETE_NO_FILES_WRITTEN")
