# TIDEGLASS — 潮鏡都市

水没した都市を探索し、物語とターン制コマンド戦闘を進める3D RPGです。
C++とOpenGLを使用し、フィールド探索、ストーリーイベント、3Dモデル描画、
キャラクターアニメーション、戦闘UIまで実装しています。

## 作品情報

| 項目 | 内容 |
|---|---|
| ジャンル | 3D探索・ターン制コマンドRPG |
| 対応環境 | Windows 10 / 11（64bit） |
| グラフィックス | OpenGL 3.3 |
| 開発言語 | C++ |
| 開発環境 | Visual Studio 2022 |
| 制作形態 | 要追記 |
| 開発期間 | 要追記 |

## 主な特徴

- 水没都市を舞台にした3Dフィールド探索
- プレイヤーと敵の3Dモデル・アニメーション描画
- 基本攻撃、スキル、防御、アイテムを扱うターン制戦闘
- タイトル、物語、フィールド、戦闘間のシーン遷移
- JSONデータを利用したストーリー・アニメーション管理
- マウス操作に対応した戦闘・設定UI
- ビルド時のResource自動配置

## 技術的な取り組み

### 3Dモデルとアニメーション

Assimpを利用してFBX・GLBモデルを読み込み、OpenGLで描画しています。
プレイヤーと敵モデルの描画に加え、ボーンアニメーションの読み込み、
リターゲットしたアニメーションデータの再生に対応しています。

### フィールドと戦闘の統合

フィールド上の敵との接触から戦闘へ移行し、勝敗後にフィールドへ戻る
一連のゲーム進行を実装しています。戦闘では行動選択、ターン進行、
ダメージ処理、戦闘ログ、勝敗表示を管理します。

### 再現可能なビルド環境

依存ライブラリは`vcpkg.json`で宣言し、MSVC v143用のtripletを収録しています。
クリーンクローンからAssimp、GLEW、GLFW、GLM、stbを復元できます。

## 使用技術

| 技術 | 用途 |
|---|---|
| OpenGL / GLEW | 3Dレンダリング |
| GLFW | ウィンドウ、入力、OpenGLコンテキスト |
| GLM | ベクトル・行列計算 |
| Assimp | FBX・GLBモデルの読み込み |
| Dear ImGui | ゲーム内UI |
| stb_image | テクスチャ読み込み |
| nlohmann/json | JSONデータ処理 |
| vcpkg | C++依存ライブラリ管理 |

## 操作方法

| 入力 | 操作 |
|---|---|
| `W` / `A` / `S` / `D` | 移動 |
| マウス移動 | カメラ回転 |
| マウスホイール | カメラ距離変更 |
| `E` | 会話・調査 |
| `Alt` | マウスカーソル表示 |
| `Esc` | 設定・所持アイテム確認 |
| `F11` | 全画面切り替え |

戦闘では画面上のコマンドをマウスで選択します。
アイテムは戦闘中のみ使用できます。

## 実行方法

配布パッケージ内の`TIDEGLASS.exe`を起動してください。
`Resource`フォルダはexeと同じフォルダに置いたまま使用します。

Windowsの保護画面が表示された場合は、配布元を確認したうえで
「詳細情報」から「実行」を選択してください。

## ソースコードのビルド

### 必要環境

- Windows 10またはWindows 11（64bit）
- Visual Studio 2022
- 「C++によるデスクトップ開発」ワークロード
- MSVC v143ビルドツール（14.44.35207）
- Windows 10またはWindows 11 SDK
- Git

### 手順

1. vcpkgを準備します。

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\dev\vcpkg
C:\dev\vcpkg\bootstrap-vcpkg.bat
C:\dev\vcpkg\vcpkg.exe integrate install
```

2. `turnsei/turnsei/trnsei/trnsei.sln`をVisual Studio 2022で開きます。
3. `Release | x64`を選択してビルドします。

初回ビルド時に、`vcpkg.json`で宣言された依存ライブラリが
`vcpkg_installed`へ自動的に復元されます。ビルド完了後、実行に必要な
`Resource`フォルダも出力先へコピーされます。

## 主なディレクトリ構成

```text
Turnsei/
├─ README.md
├─ vcpkg.json
├─ triplets/
├─ licenses/
└─ turnsei/turnsei/trnsei/
   ├─ trnsei.sln
   └─ trnsei/
      ├─ assets/       # モデル・シェーダー処理
      ├─ src/game/     # シーン、フィールド、戦闘、物語
      ├─ imgui/        # ゲーム内UI
      └─ Resource/     # モデル、テクスチャ、物語データ
```

## ライセンスと素材出典

- [サードパーティライブラリ一覧](THIRD_PARTY_NOTICES.md)
- [素材出典一覧](ASSET_CREDITS.md)
- [ライセンス全文](licenses/)

プレイヤー・敵モデルおよび一部アニメーションは自作です。
`CombatIdle`アニメーションにはAdobe Mixamoを使用しています。
一部の背景素材には生成AIを活用しており、詳細は素材出典一覧に記載しています。
