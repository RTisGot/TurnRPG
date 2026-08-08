"""Mixamoの抜刀と片手剣斬撃をゲーム固有リグへワールド空間でリターゲットする。

ローカルEuler角の単純コピーは、ボーンロールが異なるリグ間では肩・肘・手首を
別方向へ回してしまう。本ツールは各ボーンのレスト姿勢からワールド回転差を求め、
ゲームリグのレスト姿勢へ移植した後、親子階層からローカル姿勢を逆算する。
"""

import bpy
import json
import math
import os
from mathutils import Matrix, Quaternion, Vector


ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
TARGET_BLEND = os.path.join(ROOT, "Resource", "Character.blend")
BASE_JSON = os.path.join(ROOT, "Resource", "Character_Walk.json")
OUTPUT_JSON = os.path.join(ROOT, "Resource", "Character_Attack.json")
OUTPUT_BLEND = os.path.join(ROOT, "Resource", "Character_CombatAnimation.blend")
DRAW_FBX = r"C:\Users\rere0\Downloads\Draw Sword 1.fbx"
SLASH_FBX = r"C:\Users\rere0\Downloads\Stable Sword Inward Slash.fbx"
FPS = 30.0
IK_REACH_SAMPLES = []

BONE_MAP = {
    "mixamorig:Hips": "Hips",
    "mixamorig:Spine": "spine1",
    "mixamorig:Spine1": "spine2",
    "mixamorig:Spine2": "spine3",
    "mixamorig:Neck": "neck",
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

for mixamo_side, game_side in (("Left", "L"), ("Right", "R")):
    for mixamo_finger, game_finger in (
        ("Thumb", "thumb"), ("Index", "index"), ("Middle", "middle"),
        ("Ring", "ring"), ("Pinky", "pinky"),
    ):
        for joint in range(1, 4):
            BONE_MAP[f"mixamorig:{mixamo_side}Hand{mixamo_finger}{joint}"] = (
                f"{game_finger}{joint}.{game_side}"
            )


def import_clip(path):
    """FBXを読み込み、追加されたアーマチュアとフレーム範囲を返す。"""
    before = set(bpy.context.scene.objects)
    bpy.ops.import_scene.fbx(filepath=path)
    armature = next(
        obj for obj in bpy.context.scene.objects
        if obj not in before and obj.type == "ARMATURE"
    )
    action = armature.animation_data.action
    return armature, int(round(action.frame_range[0])), int(round(action.frame_range[1]))


def matrix_with_rotation(matrix, rotation):
    """既存の移動量を保ったまま回転だけを差し替える。"""
    result = rotation.to_matrix().to_4x4()
    result.translation = matrix.translation
    return result


def set_pose_matrix(pose_bone, desired_matrix):
    """希望するアーマチュア空間行列からmatrix_basisを逆算する。"""
    parent = pose_bone.parent
    if parent:
        rest_relative = parent.bone.matrix_local.inverted() @ pose_bone.bone.matrix_local
        pose_bone.matrix_basis = (parent.matrix @ rest_relative).inverted() @ desired_matrix
    else:
        pose_bone.matrix_basis = pose_bone.bone.matrix_local.inverted() @ desired_matrix


bpy.ops.wm.open_mainfile(filepath=TARGET_BLEND)
target = next(obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE")
with open(BASE_JSON, "r", encoding="utf-8") as stream:
    walk = json.load(stream)

# 腕が左右に自然に下りている歩行フレームを、復帰姿勢として選ぶ。
best_frame = 0
best_score = -1.0e9
for frame in range(walk["frames"]):
    for name, channel in walk["channels"].items():
        bone = target.pose.bones.get(name)
        if bone is None:
            continue
        bone.rotation_mode = "QUATERNION"
        bone.location = channel["position"][frame]
        bone.rotation_quaternion = channel["rotation"][frame]
        bone.scale = channel["scale"][frame]
    bpy.context.view_layer.update()
    left = target.pose.bones.get("hand.L")
    right = target.pose.bones.get("hand.R")
    if left and right:
        score = (left.head - right.head).length - abs(left.head.z - right.head.z) * 0.5
        if score > best_score:
            best_score = score
            best_frame = frame

base = {}
for bone in target.pose.bones:
    channel = walk["channels"].get(bone.name)
    base[bone.name] = {
        "position": Vector(channel["position"][best_frame]) if channel else Vector(),
        "rotation": Quaternion(channel["rotation"][best_frame]) if channel else Quaternion(),
        "scale": Vector(channel["scale"][best_frame]) if channel else Vector((1, 1, 1)),
    }


def reset_target_pose():
    for name, value in base.items():
        bone = target.pose.bones.get(name)
        if bone is None:
            continue
        bone.rotation_mode = "QUATERNION"
        bone.location = value["position"]
        bone.rotation_quaternion = value["rotation"]
        bone.scale = value["scale"]
    bpy.context.view_layer.update()


reset_target_pose()
draw_armature, draw_first, draw_last = import_clip(DRAW_FBX)
slash_armature, slash_first, slash_last = import_clip(SLASH_FBX)


def sample_clip(source, first, last):
    """Mixamo各フレームを親ボーン基準のローカル回転差として変換する。"""
    samples = []
    source_for_target = {
        target_name: source_name for source_name, target_name in BONE_MAP.items()
        if source_name in source.pose.bones and target_name in target.pose.bones
    }

    # レスト姿勢における「親から見たボーン軸」を比較する。アーマチュア空間の
    # 上下軸を直接コピーすると、今回のように腰から全身が反転する。
    axis_conversion = {}
    for target_name, source_name in source_for_target.items():
        source_bone = source.data.bones[source_name]
        target_bone = target.data.bones[target_name]
        source_relative = (source_bone.parent.matrix_local.inverted() @ source_bone.matrix_local
                           if source_bone.parent else source_bone.matrix_local)
        target_relative = (target_bone.parent.matrix_local.inverted() @ target_bone.matrix_local
                           if target_bone.parent else target_bone.matrix_local)
        axis_conversion[target_name] = (
            target_relative.to_quaternion().inverted() @ source_relative.to_quaternion()
        ).normalized()

    # 各クリップ先頭を基準にすることで、Mixamoモデル固有のT/Aポーズ差を除去する。
    bpy.context.scene.frame_set(first)
    bpy.context.view_layer.update()
    source_reference = {
        target_name: source.pose.bones[source_name].matrix_basis.to_quaternion().copy()
        for target_name, source_name in source_for_target.items()
    }

    # ローカル軸補正後は全身の重心移動を十分に戻す。腰・胸・脚が斬撃へ
    # 連動しないと、腕だけが胴体を横切って剣が身体へめり込む。
    transfer_weight = {
        "Hips": 0.26,
        "spine1": 0.46,
        "spine2": 0.56,
        "spine3": 0.64,
        "neck": 0.38,
        "shoulder.R": 0.80,
        "uperarm.R": 1.0,
        "lowerarm.R": 1.0,
        "hand.R": 1.0,
        "shoulder.L": 0.62,
        "uperarm.L": 0.72,
        "lowerarm.L": 0.72,
        "hand.L": 0.66,
        "uperleg.L": 0.56,
        "lowerleg.L": 0.56,
        "foot.L": 0.52,
        "toe.L": 0.44,
        "uperleg.R": 0.56,
        "lowerleg.R": 0.56,
        "foot.R": 0.52,
        "toe.R": 0.44,
    }

    def body_basis(armature):
        hips = armature.pose.bones["mixamorig:Hips" if armature == source else "Hips"].head
        head_name = "mixamorig:Head" if armature == source else "neck"
        left_name = "mixamorig:LeftShoulder" if armature == source else "shoulder.L"
        right_name = "mixamorig:RightShoulder" if armature == source else "shoulder.R"
        up = (armature.pose.bones[head_name].head - hips).normalized()
        right = (armature.pose.bones[right_name].head -
                 armature.pose.bones[left_name].head).normalized()
        forward = right.cross(up).normalized()
        right = up.cross(forward).normalized()
        return right, up, forward

    def map_body_vector(vector, source_axes, target_axes):
        return (target_axes[0] * vector.dot(source_axes[0]) +
                target_axes[1] * vector.dot(source_axes[1]) +
                target_axes[2] * vector.dot(source_axes[2]))

    # ゲームリグの体軸は固定し、Mixamo側は各クリップ先頭の体軸を基準にする。
    reset_target_pose()
    target_axes = body_basis(target)
    source_axes = body_basis(source)

    def pose_matrices_from_channels(frame_data):
        matrices = {}
        ordered = sorted(target.pose.bones, key=lambda bone: len(bone.parent_recursive))
        for bone in ordered:
            value = frame_data[bone.name]
            basis = Matrix.LocRotScale(value["position"], value["rotation"], value["scale"])
            if bone.parent:
                rest_relative = bone.parent.bone.matrix_local.inverted() @ bone.bone.matrix_local
                matrices[bone.name] = matrices[bone.parent.name] @ rest_relative @ basis
            else:
                matrices[bone.name] = bone.bone.matrix_local @ basis
        return matrices

    def set_global_direction(frame_data, matrices, bone_name, desired_direction):
        """ボーンのY軸を指定方向へ向け、親基準のローカル回転へ戻す。"""
        bone = target.pose.bones[bone_name]
        current = matrices[bone_name]
        current_direction = (current.to_quaternion() @ Vector((0, 1, 0))).normalized()
        swing = current_direction.rotation_difference(desired_direction.normalized())
        desired = matrix_with_rotation(current, swing @ current.to_quaternion())
        if bone.parent:
            rest_relative = bone.parent.bone.matrix_local.inverted() @ bone.bone.matrix_local
            parent_space = matrices[bone.parent.name] @ rest_relative
        else:
            parent_space = bone.bone.matrix_local
        basis = parent_space.inverted() @ desired
        position, rotation, scale = basis.decompose()
        frame_data[bone_name] = {
            "position": position,
            "rotation": rotation.normalized(),
            "scale": scale,
        }
        matrices.update(pose_matrices_from_channels(frame_data))

    def solve_right_arm_ik(frame_data):
        """Mixamoの肩・肘・手首位置から、ゲームリグの2ボーンIKを解析的に解く。"""
        source_upper = source.pose.bones["mixamorig:RightArm"]
        source_lower = source.pose.bones["mixamorig:RightForeArm"]
        source_hand = source.pose.bones["mixamorig:RightHand"]
        source_shoulder = source_upper.head.copy()
        source_elbow = source_lower.head.copy()
        source_wrist = source_hand.head.copy()
        source_total = max(source_upper.length + source_lower.length, 1.0e-5)
        # 肘が完全に伸び切る直前で止める。二本の骨が一直線になると、
        # IKの曲げ方向が反転して1フレームだけ腕が跳ねるため92%を上限にする。
        reach_ratio = min(0.92, max(0.42, (source_wrist - source_shoulder).length / source_total))
        IK_REACH_SAMPLES.append(reach_ratio)

        matrices = pose_matrices_from_channels(frame_data)
        upper = target.pose.bones["uperarm.R"]
        lower = target.pose.bones["lowerarm.R"]
        shoulder = matrices["uperarm.R"].translation.copy()
        length_upper = upper.bone.length
        length_lower = lower.bone.length
        target_total = length_upper + length_lower

        mapped_reach = map_body_vector(source_wrist - source_shoulder, source_axes, target_axes)
        if mapped_reach.length < 1.0e-5:
            return
        wrist = shoulder + mapped_reach.normalized() * target_total * reach_ratio
        line = (wrist - shoulder)
        distance = line.length
        direction = line.normalized()

        mapped_elbow = map_body_vector(source_elbow - source_shoulder, source_axes, target_axes)
        pole = mapped_elbow - direction * mapped_elbow.dot(direction)
        if pole.length < 1.0e-5:
            pole = target_axes[2].cross(direction)
        pole.normalize()

        along = (length_upper * length_upper - length_lower * length_lower +
                 distance * distance) / (2.0 * distance)
        height = max(0.0, length_upper * length_upper - along * along) ** 0.5
        elbow = shoulder + direction * along + pole * height

        set_global_direction(frame_data, matrices, "uperarm.R", elbow - shoulder)
        matrices = pose_matrices_from_channels(frame_data)
        actual_elbow = matrices["lowerarm.R"].translation.copy()
        set_global_direction(frame_data, matrices, "lowerarm.R", wrist - actual_elbow)

    for frame in range(first, last + 1):
        bpy.context.scene.frame_set(frame)
        bpy.context.view_layer.update()
        frame_data = {}
        for name, value in base.items():
            source_name = source_for_target.get(name)
            if source_name:
                source_rotation = source.pose.bones[source_name].matrix_basis.to_quaternion()
                source_delta = source_reference[name].inverted() @ source_rotation
                conversion = axis_conversion[name]
                converted_delta = conversion @ source_delta @ conversion.inverted()
                weight = transfer_weight.get(name, 0.72)
                converted_delta = Quaternion().slerp(converted_delta, weight)
                rotation = (value["rotation"] @ converted_delta).normalized()
            else:
                rotation = value["rotation"].copy()
            frame_data[name] = {
                "position": value["position"].copy(),
                "rotation": rotation,
                "scale": value["scale"].copy(),
            }
        # 回転転送後に到達位置をIKで保証する。腕を曲げるだけの姿勢や胸への
        # めり込みを防ぎ、Mixamoと同じ伸展率をゲームリグの骨長で再現する。
        solve_right_arm_ik(frame_data)
        samples.append(frame_data)
    return samples


draw_samples = sample_clip(draw_armature, draw_first, draw_last)
slash_samples = sample_clip(slash_armature, slash_first, slash_last)

# 抜刀の最終姿勢から斬撃冒頭へ6Fでクロスフェードする。
sequence = list(draw_samples)
transition_frames = 6
for index, slash_pose in enumerate(slash_samples):
    if index < transition_frames:
        factor = (index + 1) / transition_frames
        blended = {}
        for name in base:
            a = draw_samples[-1][name]
            b = slash_pose[name]
            blended[name] = {
                "position": a["position"].lerp(b["position"], factor),
                "rotation": a["rotation"].slerp(b["rotation"], factor),
                "scale": a["scale"].lerp(b["scale"], factor),
            }
        sequence.append(blended)
    else:
        sequence.append(slash_pose)

# 最終姿勢から戦闘待機へ短く戻し、アニメーション終了時の跳ねを抑える。
recovery_frames = 8
for recovery in range(1, recovery_frames + 1):
    factor = recovery / recovery_frames
    pose = {}
    for name, value in base.items():
        previous = sequence[-1][name]
        pose[name] = {
            "position": previous["position"].lerp(value["position"], factor),
            "rotation": previous["rotation"].slerp(value["rotation"], factor),
            "scale": previous["scale"].lerp(value["scale"], factor),
        }
    sequence.append(pose)

# Mixamo二クリップの接続部とIK解のフレーム間回転を速度制限する。
# ポーズ自体を平均化するのではなく、1Fで進める最大角度だけを制限するため、
# 斬撃の軌道を保ちながら関節の瞬間反転を除去できる。
for frame in range(1, len(sequence)):
    for name in base:
        previous = sequence[frame - 1][name]["rotation"]
        current = sequence[frame][name]["rotation"]
        angle = previous.rotation_difference(current).angle
        if name == "hand.R":
            max_angle = math.radians(28.0)
        elif name in ("uperarm.R", "lowerarm.R", "shoulder.R"):
            max_angle = math.radians(22.0)
        elif name.startswith("spine") or name in ("Hips", "neck"):
            max_angle = math.radians(12.0)
        else:
            max_angle = math.radians(16.0)
        if angle > max_angle:
            sequence[frame][name]["rotation"] = previous.slerp(current, max_angle / angle)

# 手首・前腕の角速度が最大のフレームを実際の命中タイミングにする。
slash_start = len(draw_samples)
hit_frame = slash_start
max_motion = -1.0
for frame in range(slash_start + 1, len(draw_samples) + len(slash_samples)):
    motion = 0.0
    for name in ("uperarm.R", "lowerarm.R", "hand.R"):
        previous = sequence[frame - 1][name]["rotation"]
        current = sequence[frame][name]["rotation"]
        motion += previous.rotation_difference(current).angle
    if motion > max_motion:
        max_motion = motion
        hit_frame = frame

channels = {
    name: {"position": [], "rotation": [], "scale": []}
    for name in base
}
for pose in sequence:
    for name, value in pose.items():
        channels[name]["position"].append(list(value["position"]))
        channels[name]["rotation"].append(list(value["rotation"]))
        channels[name]["scale"].append(list(value["scale"]))

result = {
    "name": "Basic_Attack",
    "fps": FPS,
    "frames": len(sequence),
    "channels": channels,
    "events": {
        "draw_end": len(draw_samples) - 1,
        "hit": hit_frame,
        "recovery": len(draw_samples) + len(slash_samples),
    },
}
with open(OUTPUT_JSON, "w", encoding="utf-8") as stream:
    json.dump(result, stream, ensure_ascii=False)

# Blender上でも骨格を直接確認できる編集用Actionを保存する。
action = bpy.data.actions.new("Basic_Attack_Mixamo_Retarget")
target.animation_data_create()
target.animation_data.action = action
for frame, pose in enumerate(sequence):
    for name, value in pose.items():
        bone = target.pose.bones.get(name)
        if bone is None:
            continue
        bone.location = value["position"]
        bone.rotation_mode = "QUATERNION"
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

print("RETARGET_COMPLETE")
print("DRAW_FRAMES", len(draw_samples))
print("SLASH_FRAMES", len(slash_samples))
print("OUTPUT_FRAMES", len(sequence))
print("HIT_FRAME", hit_frame)
print("BASE_FRAME", best_frame)
print("IK_REACH_MIN", round(min(IK_REACH_SAMPLES), 4))
print("IK_REACH_MAX", round(max(IK_REACH_SAMPLES), 4))
