"""ベイク済み攻撃の肘・膝角度と曲げ面の連続性を検証する。"""

import bpy
import math
import os

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SOURCE = os.path.join(ROOT, "Resource", "Character_CombatAnimation.blend")
bpy.ops.wm.open_mainfile(filepath=SOURCE)
armature = next(obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE" and
                obj.animation_data and obj.animation_data.action and
                obj.animation_data.action.name.startswith("Basic_Attack_Mixamo"))

CHAINS = {
    "elbow.L": ("uperarm.L", "lowerarm.L", "hand.L", 150.0),
    "elbow.R": ("uperarm.R", "lowerarm.R", "hand.R", 150.0),
    "knee.L": ("uperleg.L", "lowerleg.L", "foot.L", 145.0),
    "knee.R": ("uperleg.R", "lowerleg.R", "foot.R", 145.0),
}
errors = 0
for label, (upper_name, lower_name, end_name, limit) in CHAINS.items():
    minimum = 180.0
    maximum = 0.0
    plane_flips = 0
    previous_normal = None
    for frame in range(bpy.context.scene.frame_start, bpy.context.scene.frame_end + 1):
        bpy.context.scene.frame_set(frame)
        upper = armature.pose.bones[upper_name]
        lower = armature.pose.bones[lower_name]
        end = armature.pose.bones[end_name]
        first = (lower.head - upper.head).normalized()
        second = (end.head - lower.head).normalized()
        flex = math.degrees(first.angle(second))
        minimum = min(minimum, flex)
        maximum = max(maximum, flex)
        normal = first.cross(second)
        # 15度未満では二本の骨がほぼ一直線で面法線の符号が数値的に
        # 不定になるため、逆関節判定は明確に曲がっている区間だけで行う。
        if normal.length > 0.26:
            normal.normalize()
            if previous_normal is not None and previous_normal.dot(normal) < -0.25:
                plane_flips += 1
            previous_normal = normal
    if maximum > limit + 0.5 or plane_flips:
        errors += 1
    print("JOINT", label, "FLEX_MIN", round(minimum, 2), "FLEX_MAX", round(maximum, 2),
          "LIMIT", limit, "PLANE_FLIPS", plane_flips)
print("ANATOMY_ERRORS", errors)
