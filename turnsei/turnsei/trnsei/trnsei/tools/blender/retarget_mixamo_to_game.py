"""Mixamoのアニメーションをゲーム固有リグへ移し、ランタイムJSONへ書き出す。

使い方:
  blender --background --python retarget_mixamo_to_game.py -- source.fbx output.json Basic_Attack

Mixamoとゲーム側ではボーン名が異なるため、各ボーンの先頭フレームからの
ローカル回転差分をゲーム側の基準姿勢へ合成する。絶対姿勢をコピーすると
レスト軸の違いで手足がねじれるため、差分だけを移すのが重要。
"""

import bpy
import json
import os
import sys
from mathutils import Quaternion, Vector


ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
TARGET_BLEND = os.path.join(ROOT, "Resource", "Character.blend")
BASE_JSON = os.path.join(ROOT, "Resource", "Character_Walk.json")
FPS = 30.0

BONE_MAP = {
    "mixamorig:Hips": "Hips",
    "mixamorig:Spine": "spine1",
    "mixamorig:Spine1": "spine2",
    "mixamorig:Spine2": "spine3",
    "mixamorig:Neck": "neck",
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

# Mixamoとゲームリグでは各ボーンのローカル軸が完全には一致しない。
# 全身へ回転差分を100%移すと腰・胸・脚の誤差が連鎖して体がねじれるため、
# 片手剣を振る右腕を主動作とし、それ以外は必要な補助量だけ移す。
TRANSFER_WEIGHT = {
    "Hips": 0.10,
    "spine1": 0.12,
    "spine2": 0.16,
    "spine3": 0.20,
    "neck": 0.08,
    "繝懊・繝ｳ.005": 0.06,
    "shoulder.R": 0.72,
    "uperarm.R": 0.78,
    "lowerarm.R": 0.76,
    "hand.R": 0.58,
    "shoulder.L": 0.15,
    "uperarm.L": 0.18,
    "lowerarm.L": 0.15,
    "hand.L": 0.12,
    "uperleg.L": 0.0,
    "lowerleg.L": 0.0,
    "foot.L": 0.0,
    "toe.L": 0.0,
    "uperleg.R": 0.0,
    "lowerleg.R": 0.0,
    "foot.R": 0.0,
    "toe.R": 0.0,
}

for side, mixamo_side, game_side in (("L", "Left", "L"), ("R", "Right", "R")):
    for mixamo_finger, game_finger in (
        ("Thumb", "thumb"), ("Index", "index"), ("Middle", "middle"),
        ("Ring", "ring"), ("Pinky", "pinky"),
    ):
        for joint in range(1, 4):
            BONE_MAP[f"mixamorig:{mixamo_side}Hand{mixamo_finger}{joint}"] = (
                f"{game_finger}{joint}.{game_side}"
            )


def arguments():
    argv = sys.argv[sys.argv.index("--") + 1:]
    if len(argv) != 3:
        raise RuntimeError("source.fbx output.json clip_name の3引数が必要です")
    return map(os.path.abspath, argv[:2]), argv[2]


(source_fbx, output_json), clip_name = arguments()
with open(BASE_JSON, "r", encoding="utf-8") as stream:
    base_data = json.load(stream)

# ゲーム側の先頭姿勢を保持する。未対応ボーンもこの値を全フレームへ複製し、
# 髪・衣服など既存スキンに必要なチャンネルを欠落させない。
base = {}
for name, channel in base_data["channels"].items():
    base[name] = {
        "position": Vector(channel["position"][0]),
        "rotation": Quaternion(channel["rotation"][0]),
        "scale": Vector(channel["scale"][0]),
    }

bpy.ops.wm.open_mainfile(filepath=TARGET_BLEND)
before = set(bpy.context.scene.objects)
bpy.ops.import_scene.fbx(filepath=source_fbx)
source_armature = next(
    obj for obj in bpy.context.scene.objects
    if obj not in before and obj.type == "ARMATURE"
)
action = source_armature.animation_data.action
first = int(round(action.frame_range[0]))
last = int(round(action.frame_range[1]))
frames = last - first + 1

# 先頭フレームをMixamo側の基準姿勢とする。アニメーションクリップに含まれる
# レスト姿勢のオフセットを除去でき、異なる骨格間での恒常的なねじれを防げる。
bpy.context.scene.frame_set(first)
reference_rotation = {
    source: source_armature.pose.bones[source].rotation_quaternion.copy()
    for source in BONE_MAP if source in source_armature.pose.bones
}
reference_hips = source_armature.pose.bones["mixamorig:Hips"].location.copy()

channels = {
    name: {"position": [], "rotation": [], "scale": []}
    for name in base
}

for source_frame in range(first, last + 1):
    bpy.context.scene.frame_set(source_frame)
    sampled = {}
    for source_name, target_name in BONE_MAP.items():
        bone = source_armature.pose.bones.get(source_name)
        if bone is None or target_name not in base:
            continue
        # Blenderのクォータニオンは w,x,y,z 順。基準の逆を左から掛け、
        # Mixamoクリップ内で生じた回転差分だけを抽出する。
        delta = reference_rotation[source_name].inverted() @ bone.rotation_quaternion
        weight = TRANSFER_WEIGHT.get(target_name, 0.32)
        # 恒等回転から差分へ補間して転送量を制限する。特に脚を0にすることで
        # 足裏を固定し、上半身の軸差が下半身へ伝播するのを防ぐ。
        weighted_delta = Quaternion((1.0, 0.0, 0.0, 0.0)).slerp(delta, weight)
        sampled[target_name] = base[target_name]["rotation"] @ weighted_delta

    for target_name, target_base in base.items():
        position = target_base["position"].copy()
        if target_name == "Hips":
            # 攻撃時の踏み込みは残しつつ、大きなルート移動はゲーム側の移動処理と
            # 二重適用になるため抑える。Mixamoはcm、ゲームリグはm相当のため1/100。
            root_delta = (source_armature.pose.bones["mixamorig:Hips"].location - reference_hips) * 0.01
            root_delta.x = max(-0.30, min(0.30, root_delta.x))
            root_delta.y = max(-0.30, min(0.30, root_delta.y))
            root_delta.z = max(-0.12, min(0.12, root_delta.z))
            # Stable系モーションではその場斬りを優先し、ルート移動は使わない。
            # 実際の半歩移動はゲーム側が管理するため二重移動にもならない。
        rotation = sampled.get(target_name, target_base["rotation"])
        channels[target_name]["position"].append(list(position))
        channels[target_name]["rotation"].append(list(rotation))
        channels[target_name]["scale"].append(list(target_base["scale"]))

result = {
    "name": clip_name,
    "fps": FPS,
    "frames": frames,
    "channels": channels,
}
if clip_name == "Basic_Attack":
    # 25フレームの大剣モーションでは中盤直前が最速区間。ダメージ判定を
    # ここへ合わせ、見た目より先に敵が反応する違和感をなくす。
    result["events"] = {
        "anticipation": max(1, round(frames * 0.20)),
        "hit": max(2, round(frames * 0.44)),
        "recovery": max(3, round(frames * 0.68)),
    }

with open(output_json, "w", encoding="utf-8") as stream:
    json.dump(result, stream, ensure_ascii=False)

print("RETARGETED", source_fbx)
print("OUTPUT", output_json)
print("FRAMES", frames, "MAPPED_BONES", len(sampled))
