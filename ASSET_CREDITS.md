# Asset Credits

This document records the provenance and usage rights of non-code assets used
by TIDEGLASS. Entries marked must be resolved before the project
or executable is submitted externally.

## Assets requiring confirmation

| Asset | Files | Author/source | License or permission | Modifications | Status |
|---|---|---|---|---|---|
| Character animations | `Resource/Character_Attack_Baked.fbx`, `Resource/Character_*.json` | `CombatIdle`のみAdobe Mixamo。AttackとWalkはProject authorによる自作 | [Mixamo FAQ](https://helpx.adobe.com/creative-cloud/faq/mixamo-faq.html)およびAdobe利用条件 | Mixamoアニメーションのリターゲット、各アニメーションのJSON変換 | 確認済み |

## Original assets

| Asset | Files | Creator | Creation method | Status |
|---|---|---|---|---|
| Player character | `Resource/Character.blend`, `Resource/Character.fbx`, `Resource/Character_Gameplay.fbx` | Project author | Original 3D character model created and edited for this project | 確認済み |
| Enemy character | `Resource/Enemy.fbx` | Project author | Original 3D model created for this project | 確認済み |

## Assets created partly with generative AI

The following assets include generative-AI-assisted work. The service name,
generation date, applicable terms and extent of manual modification must be
added before external submission.

| Asset | Files | AI service | Human editing and integration | Status |
|---|---|---|---|---|
| Day sky HDRI | `Resource/DaySkyHDRI053B_2K_HDR.exr` | 要確認 | ゲーム用HDRIとして調整・配置
| Grass texture | `Resource/Grass001_2K-JPG_Color.jpg` | 要確認 | ゲーム用テクスチャとして調整・配置
| Drowned City models and textures | `Resource/DrownedCity/` | 要確認 | Blenderで生成・編集し、FBX/GLBへ出力

## Project-created working files

The repository contains Blender source files, generated FBX/GLB files and
animation JSON files. A file being present in the project does not by itself
prove that every source texture, model or animation is original. Move an item
from the table above into this section only after confirming that all of its
components were created by the project author.

For each confirmed original asset, record:

| Asset | Files | Creator | Creation method |
|---|---|---|---|
| Example | `Resource/example.*` | 制作者名 | Blenderで新規制作 |

## Required evidence for third-party assets

For every third-party asset, retain the following information:

1. Asset name and author or distributor.
2. Direct URL to the original distribution page.
3. License name or written permission.
4. Whether redistribution, modification and portfolio/commercial use are allowed.
5. Description of modifications made for this project.
6. Access or download date when relevant.

If the source or permission cannot be established, remove the asset from the
submission build or replace it with an asset whose rights can be documented.

## Mixamo animations

`CombatIdle` originates from Adobe Mixamo and was retargeted and converted for
use in this project. The Attack and Walk animations were created by the project
author.

| Output file | Original animation/service | Processing | Status |
|---|---|---|---|
| `Resource/Character_Attack_Baked.fbx` | Project authorによる自作 | Blenderで制作・ベイク | 確認済み |
| `Resource/Character_Attack.json` | Project authorによる自作 | JSONへ変換 | 確認済み |
| `Resource/Character_CombatIdle.json` | Adobe Mixamo | Blenderでリターゲット後、JSONへ変換 | 確認済み |
| `Resource/Character_Walk.json` | Project authorによる自作 | JSONへ変換 | 確認済み |
