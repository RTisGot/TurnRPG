# Blender retarget input

Place locally acquired animation FBX files in this directory when running the
retargeting tools with their default paths:

```text
Draw Sword 1.fbx
Stable Sword Inward Slash.fbx
```

Alternatively, pass input paths after Blender's `--` separator:

```powershell
blender --background --python retarget_draw_slash_sequence.py -- `
  "D:\animations\Draw Sword 1.fbx" `
  "D:\animations\Stable Sword Inward Slash.fbx"
```

Do not commit downloaded Mixamo FBX source files unless their redistribution
terms explicitly allow it.
