【起動方法】
フォルダ内の「TIDEGLASS.exe」をダブルクリックしてください。
ResourceフォルダやDLLファイルは、exeと一緒に置いたまま起動してください。

【基本操作】
W / A / S / D : 移動
マウス移動     : カメラ回転
マウスホイール : カメラ距離
E              : 会話・調査
Alt            : マウスカーソル表示
Esc            : 設定・所持アイテム確認
F11            : 全画面切り替え

【戦闘】
画面上のコマンドをマウスで選択します。
アイテムは戦闘中のみ使用できます。

【動作環境】
Windows 10 / 11（64bit）
OpenGL 3.3対応のグラフィック環境

【ソースコードのビルド】
必要環境:
- Visual Studio 2022
- 「C++によるデスクトップ開発」ワークロード
- MSVC v143ビルドツール（14.44.35207）
- Windows 10またはWindows 11 SDK
- Git

1. vcpkgを準備します。

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\dev\vcpkg
C:\dev\vcpkg\bootstrap-vcpkg.bat
C:\dev\vcpkg\vcpkg.exe integrate install
```

2. `turnsei/turnsei/trnsei/trnsei.sln`をVisual Studio 2022で開きます。
3. `Release | x64`を選択してビルドします。

初回ビルド時に、`vcpkg.json`で宣言された依存ライブラリが
`vcpkg_installed`へ自動的に復元されます。

※ Windowsの保護画面が表示された場合は、提出元を確認したうえで
「詳細情報」→「実行」を選択してください。
