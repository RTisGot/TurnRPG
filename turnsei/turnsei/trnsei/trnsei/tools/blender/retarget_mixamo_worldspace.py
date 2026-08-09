"""Mixamoアニメーションをゲームリグへ正規のワールド空間差分でリターゲットする。

重要事項:
- Mixamo FBXのArmatureオブジェクトに含まれるX=90度、scale=0.01も計算へ含める。
- ソースの「ワールド姿勢 * ワールドレスト姿勢の逆」を動作差分とする。
- ゲームJSONはnode.transformへ乗算される差分なので、targetのmatrix_basisを書き出す。
"""

import bpy
import json
import math
import os
import sys
from mathutils import Matrix, Quaternion, Vector

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
TARGET_FBX = os.path.join(ROOT, "Resource", "Character_Gameplay.fbx")
OUTPUT_JSON = os.path.join(ROOT, "Resource", "Character_Attack.json")
OUTPUT_BLEND = os.path.join(ROOT, "Resource", "Character_CombatAnimation.blend")
OUTPUT_FBX = os.path.join(ROOT, "Resource", "Character_Attack_Baked.fbx")
INPUT_DIR = os.path.join(os.path.dirname(__file__), "input")
SCRIPT_ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
DRAW_FBX = (
    os.path.abspath(SCRIPT_ARGS[0])
    if len(SCRIPT_ARGS) >= 1
    else os.path.join(INPUT_DIR, "Draw Sword 1.fbx")
)
SLASH_FBX = (
    os.path.abspath(SCRIPT_ARGS[1])
    if len(SCRIPT_ARGS) >= 2
    else os.path.join(INPUT_DIR, "Stable Sword Inward Slash.fbx")
)
FPS = 30.0

BONE_MAP = {
    "mixamorig:Hips": "Hips",
    "mixamorig:Spine": "spine1",
    "mixamorig:Spine1": "spine2",
    "mixamorig:Spine2": "spine3",
    "mixamorig:Neck": "neck",
    # 元モデル由来の文字化け名だが、実際に顔・髪を保持するHeadボーン。
    "mixamorig:Head": "繝懊・繝ｳ.005",
    "mixamorig:LeftShoulder": "shoulder.L",
    "mixamorig:LeftArm": "uperarm.L",
    "mixamorig:LeftForeArm": "lowerarm.L",
    "mixamorig:LeftHand": "hand.L",
    "mixamorig:RightShoulder": "shoulder.R",
    "mixamorig:RightArm": "uperarm.R",
    "mixamorig:RightForeArm": "lowerarm.R",
    "mixamorig:RightHand": "hand.R",
    "mixamorig:LeftUpLeg": "uperleg.L",
    "mixamorig:LeftLeg": "lowerleg.L",
    "mixamorig:LeftFoot": "foot.L",
    "mixamorig:LeftToeBase": "toe.L",
    "mixamorig:RightUpLeg": "uperleg.R",
    "mixamorig:RightLeg": "lowerleg.R",
    "mixamorig:RightFoot": "foot.R",
    "mixamorig:RightToeBase": "toe.R",
}
for source_side, target_side in (("Left", "L"), ("Right", "R")):
    for source_finger, target_finger in (
        ("Thumb", "thumb"), ("Index", "index"), ("Middle", "middle"),
        ("Ring", "ring"), ("Pinky", "pinky"),
    ):
        for joint in range(1, 4):
            BONE_MAP[f"mixamorig:{source_side}Hand{source_finger}{joint}"] = (
                f"{target_finger}{joint}.{target_side}"
            )


def import_clip(path):
    before = set(bpy.context.scene.objects)
    bpy.ops.import_scene.fbx(filepath=path)
    armature = next(obj for obj in bpy.context.scene.objects
                    if obj not in before and obj.type == "ARMATURE")
    action = armature.animation_data.action
    return armature, int(round(action.frame_range[0])), int(round(action.frame_range[1]))


def replace_rotation(matrix, rotation):
    result = rotation.to_matrix().to_4x4()
    result.translation = matrix.translation
    return result


# ゲームが実際にロードするFBXを唯一の基準リグにする。別の.blendを基準に
# matrix_basisを生成すると、ボーンロールとFBX pre/post rotationの差が
# node.transformへ二重に入り、肘・膝が逆方向へ曲がる。
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=TARGET_FBX)
target = next(obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE")
draw, draw_first, draw_last = import_clip(DRAW_FBX)
slash, slash_first, slash_last = import_clip(SLASH_FBX)

target_world_inverse_rotation = target.matrix_world.to_quaternion().inverted()
ordered_target = sorted(target.pose.bones, key=lambda bone: len(bone.parent_recursive))


def rest_world_head(armature, bone_name):
    return armature.matrix_world @ armature.data.bones[bone_name].head_local


def humanoid_world_basis(armature, prefix):
    """左右・前方・上方から右手座標系の人体基準軸を作る。"""
    if prefix:
        hips_name = prefix + "Hips"
        neck_name = prefix + "Neck"
        left_name = prefix + "LeftShoulder"
        right_name = prefix + "RightShoulder"
    else:
        hips_name, neck_name = "Hips", "neck"
        left_name, right_name = "shoulder.L", "shoulder.R"
    hips = rest_world_head(armature, hips_name)
    neck = rest_world_head(armature, neck_name)
    left = rest_world_head(armature, left_name)
    right_point = rest_world_head(armature, right_name)
    right_axis = (right_point - left).normalized()
    up_axis = (neck - hips).normalized()
    forward_axis = up_axis.cross(right_axis).normalized()
    right_axis = forward_axis.cross(up_axis).normalized()
    # mathutils.Matrixは行指定なので転置して各軸を列へ格納する。
    return Matrix((right_axis, forward_axis, up_axis)).transposed()


def sample(source, first, last):
    """ソースの各フレームをtargetのローカル差分TRSへ変換する。"""
    target_to_source = {
        target_name: source_name for source_name, target_name in BONE_MAP.items()
        if source_name in source.pose.bones and target_name in target.pose.bones
    }
    source_body_basis = humanoid_world_basis(source, "mixamorig:")
    target_body_basis = humanoid_world_basis(target, "")
    source_to_target_body = (
        target_body_basis @ source_body_basis.inverted()).to_quaternion().normalized()
    source_rest_world_rotation = {
        target_name: (source.matrix_world @
                      source.data.bones[source_name].matrix_local).to_quaternion().normalized()
        for target_name, source_name in target_to_source.items()
    }
    target_rest_world_rotation = {
        target_name: (target.matrix_world @
                      target.data.bones[target_name].matrix_local).to_quaternion().normalized()
        for target_name in target_to_source
    }
    result = []
    for frame in range(first, last + 1):
        bpy.context.scene.frame_set(frame)
        bpy.context.view_layer.update()
        desired_armature_rotation = {}
        for target_name, source_name in target_to_source.items():
            source_pose_world = (source.matrix_world @
                                 source.pose.bones[source_name].matrix).to_quaternion().normalized()
            # ソースのレスト/姿勢を先にtarget人体座標へ揃え、bind offsetで
            # targetレストへ移す。world差分を左乗算するとボーンロールが異なる
            # 肘・膝で曲げ軸が反転するため、rest^-1 * poseの順序を守る。
            aligned_source_rest = (source_to_target_body @
                source_rest_world_rotation[target_name] @
                source_to_target_body.inverted()).normalized()
            aligned_source_pose = (source_to_target_body @ source_pose_world @
                source_to_target_body.inverted()).normalized()
            # ソースの姿勢差分はワールド空間の回転として求める。
            #
            #   pose = motion * rest
            #   motion = pose * inverse(rest)
            #
            # 以前は inverse(rest) * pose を target_rest の右側へ掛けていた。
            # その式はソース骨のローカル軸（ボーンロール）を対象骨へ直接持ち込み、
            # 肩・肘・膝が本来とは異なる方向へ曲がる原因になる。
            world_motion = (aligned_source_pose @
                            aligned_source_rest.inverted()).normalized()
            desired_world = (world_motion @
                             target_rest_world_rotation[target_name]).normalized()
            desired_armature_rotation[target_name] = (
                target_world_inverse_rotation @ desired_world).normalized()

        pose_matrices = {}
        frame_data = {}
        for bone in ordered_target:
            if bone.parent:
                rest_relative = bone.parent.bone.matrix_local.inverted() @ bone.bone.matrix_local
                parent_space = pose_matrices[bone.parent.name] @ rest_relative
            else:
                parent_space = bone.bone.matrix_local.copy()
            pose_matrix = parent_space.copy()
            if bone.name in desired_armature_rotation:
                pose_matrix = replace_rotation(
                    pose_matrix, desired_armature_rotation[bone.name])
            pose_matrices[bone.name] = pose_matrix
            basis = parent_space.inverted() @ pose_matrix
            location, rotation, scale = basis.decompose()
            frame_data[bone.name] = {
                "position": location,
                "rotation": rotation.normalized(),
                "scale": scale,
            }
        result.append(frame_data)
    return result


draw_samples = sample(draw, draw_first, draw_last)
slash_samples = sample(slash, slash_first, slash_last)

# 6Fクロスフェード。Mixamo原本のフレームは変更せず、クリップ境界だけ補間する。
sequence = list(draw_samples)
transition_frames = 6
for index, slash_pose in enumerate(slash_samples):
    if index < transition_frames:
        factor = (index + 1) / transition_frames
        pose = {}
        for name in draw_samples[-1]:
            a = draw_samples[-1][name]
            b = slash_pose[name]
            pose[name] = {
                "position": a["position"].lerp(b["position"], factor),
                "rotation": a["rotation"].slerp(b["rotation"], factor),
                "scale": a["scale"].lerp(b["scale"], factor),
            }
        sequence.append(pose)
    else:
        sequence.append(slash_pose)

# 最後の8Fのみレスト差分へ戻す。攻撃本編のMixamoキーは加工しない。
identity_pose = {
    bone.name: {
        "position": Vector((0, 0, 0)),
        "rotation": Quaternion((1, 0, 0, 0)),
        "scale": Vector((1, 1, 1)),
    }
    for bone in target.pose.bones
}
last_attack_pose = sequence[-1]
sheath_ready_pose = draw_samples[-1]

# 斬撃末尾のフォロースルーから、柄を鞘へ合わせられる姿勢へ接続する。
for recovery in range(1, 7):
    factor = recovery / 6.0
    pose = {}
    for name, end in sheath_ready_pose.items():
        start = last_attack_pose[name]
        pose[name] = {
            "position": start["position"].lerp(end["position"], factor),
            "rotation": start["rotation"].slerp(end["rotation"], factor),
            "scale": start["scale"].lerp(end["scale"], factor),
        }
    sequence.append(pose)

# 抜刀を逆順に辿り、胸郭・肩・肘・手首を連動させて左腰へ納刀する。
for source_pose in reversed(draw_samples[:-1]):
    sequence.append({
        name: {
            "position": value["position"].copy(),
            "rotation": value["rotation"].copy(),
            "scale": value["scale"].copy(),
        }
        for name, value in source_pose.items()
    })

# 鍔が収まったあとに短い静止を置き、待機姿勢への切替を目立たせない。
last_sheath_pose = sequence[-1]
for recovery in range(1, 7):
    factor = recovery / 6.0
    pose = {}
    for name, end in identity_pose.items():
        start = last_sheath_pose[name]
        pose[name] = {
            "position": start["position"].lerp(end["position"], factor),
            "rotation": start["rotation"].slerp(end["rotation"], factor),
            "scale": start["scale"].lerp(end["scale"], factor),
        }
    sequence.append(pose)


# ゲームリグでは左右の大腿がHips配下ではなく独立ルートになっているため、
# Mixamoの脚回転を100%入れると膝が過剰に曲がる。完全固定にもせず、大腿と
# 膝は38%、足首22%、つま先15%を残して踏み込みを表現する。
# 右腕3関節の角速度最大点を命中フレームとして採用する。
hit_frame = len(draw_samples)
max_velocity = -1.0
for frame in range(len(draw_samples) + 1, len(draw_samples) + len(slash_samples)):
    velocity = 0.0
    for name in ("uperarm.R", "lowerarm.R", "hand.R"):
        velocity += sequence[frame - 1][name]["rotation"].rotation_difference(
            sequence[frame][name]["rotation"]).angle
    if velocity > max_velocity:
        max_velocity = velocity
        hit_frame = frame

# 命中前後は剣腕の肘を伸ばし、刃先まで力が抜けないシルエットを作る。
# 下腕をレスト方向へ寄せるだけなので、上腕・胸郭が作る斬撃方向は保持される。
# 完全な0度固定にはせず、わずかな屈曲を残して肘のロック感を避ける。
extension_start = max(0, hit_frame - 7)
extension_peak = min(len(sequence) - 1, hit_frame + 5)
extension_end = min(len(sequence) - 1, hit_frame + 20)
for frame in range(extension_start, extension_end + 1):
    if frame <= extension_peak:
        t = (frame - extension_start) / max(1, extension_peak - extension_start)
    else:
        t = 1.0 - (frame - extension_peak) / max(1, extension_end - extension_peak)
    t = max(0.0, min(1.0, t))
    t = t * t * (3.0 - 2.0 * t)
    lower_arm = sequence[frame]["lowerarm.R"]
    lower_arm["rotation"] = lower_arm["rotation"].slerp(
        Quaternion((1.0, 0.0, 0.0, 0.0)), 0.85 * t).normalized()

channels = {name: {"position": [], "rotation": [], "scale": []}
            for name in sequence[0]}
# qと-qは同じ姿勢だが、符号がフレーム間で反転すると補間経路が不安定になる。
# 書き出し前に全ボーンのクォータニオン半球を連続化する。
for frame in range(1, len(sequence)):
    for name in sequence[frame]:
        previous = sequence[frame - 1][name]["rotation"]
        current = sequence[frame][name]["rotation"]
        if previous.dot(current) < 0.0:
            current.negate()
for pose in sequence:
    for name, value in pose.items():
        channels[name]["position"].append(list(value["position"]))
        channels[name]["rotation"].append(list(value["rotation"]))
        channels[name]["scale"].append(list(value["scale"]))

output = {
    "name": "Basic_Attack",
    "fps": FPS,
    "frames": len(sequence),
    "channels": channels,
    "events": {"draw_end": len(draw_samples) - 1, "hit": hit_frame,
               "follow_through_end": len(draw_samples) + len(slash_samples) - 1,
               "sheath_start": len(draw_samples) + len(slash_samples) + 5,
               "sheath_end": len(sequence) - 7},
}
with open(OUTPUT_JSON, "w", encoding="utf-8") as stream:
    json.dump(output, stream, ensure_ascii=False)

# Blenderで検品できるベイク済みActionも保存する。
action = bpy.data.actions.new("Basic_Attack_Mixamo_Worldspace")
target.animation_data_create()
target.animation_data.action = action
for frame, pose in enumerate(sequence):
    for name, value in pose.items():
        bone = target.pose.bones[name]
        bone.rotation_mode = "QUATERNION"
        bone.location = value["position"]
        bone.rotation_quaternion = value["rotation"]
        bone.scale = value["scale"]
        bone.keyframe_insert("location", frame=frame, group=name)
        bone.keyframe_insert("rotation_quaternion", frame=frame, group=name)
        bone.keyframe_insert("scale", frame=frame, group=name)
bpy.context.scene.frame_start = 0
bpy.context.scene.frame_end = len(sequence) - 1
bpy.context.scene.render.fps = int(FPS)
bpy.context.scene.frame_set(0)
bpy.ops.wm.save_as_mainfile(filepath=OUTPUT_BLEND)

# ゲーム側AssimpへBlenderの最終ローカル変換をそのまま渡すアニメーション専用FBX。
# JSONのposeDelta経路を通さず、FBX node transformをクリップ値で置換させる。
bpy.ops.object.select_all(action="DESELECT")
target.select_set(True)
bpy.context.view_layer.objects.active = target
bpy.ops.export_scene.fbx(
    filepath=OUTPUT_FBX,
    use_selection=True,
    object_types={"ARMATURE"},
    add_leaf_bones=False,
    bake_anim=True,
    bake_anim_use_all_bones=True,
    bake_anim_use_nla_strips=False,
    bake_anim_use_all_actions=False,
    bake_anim_force_startend_keying=True,
    bake_anim_step=1.0,
    bake_anim_simplify_factor=0.0,
    path_mode="STRIP",
)

print("WORLDSPACE_RETARGET_COMPLETE")
print("SOURCE_OBJECT_ROTATION_INCLUDED", tuple(round(v, 5) for v in slash.rotation_euler))
print("SOURCE_OBJECT_SCALE_INCLUDED", tuple(round(v, 5) for v in slash.scale))
print("DRAW_FRAMES", len(draw_samples), "SLASH_FRAMES", len(slash_samples))
print("OUTPUT_FRAMES", len(sequence), "HIT_FRAME", hit_frame)
print("BAKED_FBX", OUTPUT_FBX)
