"""既存のゲーム用待機姿勢を基準に、戦闘待機のシームレスループを生成する。"""

import json
import math
import os
from mathutils import Euler, Quaternion, Vector

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
IDLE_JSON = os.path.join(ROOT, "Resource", "Character_CombatIdle.json")
FPS = 30.0
FRAMES = 121  # 4秒。0Fと120Fを同一姿勢にして継ぎ目を消す。

with open(IDLE_JSON, "r", encoding="utf-8") as stream:
    source = json.load(stream)

base = {}
for name, channel in source["channels"].items():
    base[name] = {
        "position": Vector(channel["position"][0]),
        "rotation": Quaternion(channel["rotation"][0]),
        "scale": Vector(channel["scale"][0]),
    }


def rotation_delta(degrees):
    return Euler(tuple(math.radians(v) for v in degrees), "XYZ").to_quaternion()


result = {
    name: {"position": [], "rotation": [], "scale": []}
    for name in base
}

for frame in range(FRAMES):
    phase = frame / (FRAMES - 1) * math.tau
    breath = math.sin(phase)
    breath_lift = 0.5 - 0.5 * math.cos(phase)
    settle = math.sin(phase * 2.0 + 0.35)
    gaze = math.sin(phase) * 0.65 + math.sin(phase * 2.0) * 0.20

    # 大きく揺らさず、骨盤→胸郭→肩→手の順に振幅を遅らせる。
    overlays = {
        "Hips": ((0.45 * breath, 0.0, -0.65 * settle),
                 (0.006 * settle, 0.0, 0.0)),
        "spine1": ((-0.35 * breath, 0.28 * settle, 0.35 * settle), (0, 0, 0)),
        "spine2": ((-0.75 * breath_lift, -0.32 * settle, -0.22 * settle), (0, 0, 0)),
        "spine3": ((0.95 * breath_lift, 0.25 * settle, 0.18 * settle), (0, 0, 0)),
        "neck": ((-0.28 * breath_lift, -0.45 * gaze, 0.12 * settle), (0, 0, 0)),
        "shoulder.L": ((0.0, 0.0, -0.55 * breath), (0, 0, 0)),
        "shoulder.R": ((0.0, 0.0, 0.55 * breath), (0, 0, 0)),
        "uperarm.L": ((0.48 * breath, 0.0, -0.55 * settle), (0, 0, 0)),
        "uperarm.R": ((-0.48 * breath, 0.0, 0.55 * settle), (0, 0, 0)),
        "lowerarm.L": ((0.0, 0.38 * settle, 0.0), (0, 0, 0)),
        "lowerarm.R": ((0.0, -0.38 * settle, 0.0), (0, 0, 0)),
        "hand.L": ((0.18 * settle, 0.20 * breath, 0.0), (0, 0, 0)),
        "hand.R": ((-0.18 * settle, -0.20 * breath, 0.0), (0, 0, 0)),
        "uperleg.L": ((-0.18 * settle, 0.0, 0.22 * settle), (0, 0, 0)),
        "uperleg.R": ((0.18 * settle, 0.0, -0.22 * settle), (0, 0, 0)),
        "lowerleg.L": ((0.12 * settle, 0.0, 0.0), (0, 0, 0)),
        "lowerleg.R": ((-0.12 * settle, 0.0, 0.0), (0, 0, 0)),
    }

    for name, value in base.items():
        rotation, position = overlays.get(name, ((0, 0, 0), (0, 0, 0)))
        pose_position = value["position"] + Vector(position)
        pose_rotation = (value["rotation"] @ rotation_delta(rotation)).normalized()
        result[name]["position"].append(list(pose_position))
        result[name]["rotation"].append(list(pose_rotation))
        result[name]["scale"].append(list(value["scale"]))

# 浮動小数点のsin(2π)誤差も残さず、最終キーを先頭キーと完全一致させる。
for channel in result.values():
    channel["position"][-1] = list(channel["position"][0])
    channel["rotation"][-1] = list(channel["rotation"][0])
    channel["scale"][-1] = list(channel["scale"][0])

output = {
    "name": "Combat_Idle",
    "fps": FPS,
    "frames": FRAMES,
    "channels": result,
}
with open(IDLE_JSON, "w", encoding="utf-8") as stream:
    json.dump(output, stream, ensure_ascii=False)

print("COMBAT_IDLE_REFINED", IDLE_JSON, "FRAMES", FRAMES)
