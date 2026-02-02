# Project: Span Engine - Technical Specification

## 1. プロジェクト概要 (Project Overview)
- **Project Name:** Span Engine
- **Goal:** Unityの優れたUX (エディタの使いやすさ) と、Unity DOTSを凌駕する極限のパフォーマンス (C++ネイティブ・DOD) を融合させた、次世代ECSゲームエンジンの開発。
- **Target Platform:** Windows (DirectX 12)
- **Development Language:** C++20 (Engine Core & User Logic)
- **Core Philosophy:**
  - **Data-Oriented:** メモリアウトを最優先し、Cache Missを撲滅する。
  - **Editor First:** すべての機能はエディタで視覚的に操作可能であること。
  - **No Garbage Collection:** メモリ管理を完全に制御し、スパイクを排除する。

## 2. コア・アーキテクチャ (Core Architecture)

### 2.1 ECS (Entity Component System)
- **Type:** **Archetype-based ECS** (Unity ECS/DOTSスタイル)
  - Sparse Set方式よりも反復処理 (Iteration) 速度を優先。
  - コンポーネント追加/削除のコストよりも、毎フレームのループ処理速度に最適化。
- **Memory Layout:** Chunk-based Memory Allocation (16KB blocks).
- **Entity:** 64-bit ID (Index + Generation)
- **Component:** POD (Plain Old Data) struct. メソッドを持たない純粋なデータ。
- **System:** ロジックの集合体。状態を持たず、Componentデータを読み書きする。

### 2.2 並行処理 (Threading)
- **Fiber-based Job System:** OSスレッド切り替えコストを排除したファイバー制タスク管理。
- **Automatic Parallelization:** システム間の依存関係グラフを構築し、独立したシステムを自動的に別スレッドで実行。
- **Determinism:** 実行順序を厳密に管理し、常に同じ結果を保証する。

### 2.3 リフレクション & ユーザーコード (Reflection & User Code)
- **Code Generation:** 専用Header Toolにより、ビルド前にC++ソースを解析してメタデータを生成。
- **Syntax:** Unreal Engineライクなマクロを使用。
  ```cpp
  struct MyComponent
  {
    float value = 0.0f;

    MyComponent() = default;
    MyComponent(float v) : value(v) {}

    SPAN_INSPECTOR_BEGIN(MyComponent)
      SPAN_FIELD(value, Tooltip("Value within the component"))
    SPAN_INSPECTOR_END()
  }
  ```
- **Hot Reload:** C++ DLLのホットリロードをサポートし、エディタ再起動なしでロジック更新可能。

## 3. 標準ライブラリ仕様 (Standard Library Specification)

### 3.1 Core / Hierarchy
**[Components]**
- `Name`: Entity名 (string)
- `Tag`: 検索用タグ (string/hash)
- `Guid`: セーブデータ/Prefab用ユニークID
- `Relationship`: 親子関係のリスト
- `LocalToWorld`: 計算済みのワールド変換行列 (Matrix4x4 / Cached)
- `Active`: Entityを無効化するためのタグコンポーネント

**[Systems]**
- `RelationshipSystem`: 親子関係の変更検知・リンク修正
- `PrefabSpawnerSystem`: Prefabのインスタンス化処理
- `TransformSystem`: Hierarchyをトラバースし、Local行列 * 親行列 = World行列を計算 (並列化必須)

### 3.2 Graphics & Rendering (DirectX 12)
**[Components]**
- `MeshFilter`: メッシュリソースID
- `MeshRenderer`: マテリアル設定、CastShadow、ReceiveShadow (ライトベイクも必須)
- `MaterialOverride`: インスタンスごとのマテリアルパラメータ上書き
- `Light`: Type (Directional/Point/Spot/Area), Color, Intensity, Range
- `LightProbeGroup`: SH(球面調和関数)データ
- `ReflectionProbe`: Cubemapデータ
- `PostProcessVolume`: Bloom, ToneMapping, ColorGrading設定 (Global/Local)
- `Decal`: 投影テクスチャ設定
- `ParticleEmitter`: GPUパーティクル設定
- `LODGroup`: 距離閾値とメッシュIDのリスト

**[Systems]**
- `CullingSystem` Frustum Culling + Occlusion Culling
- `LODSystem`: カメラ距離計算とMeshFilterの切り替え
- `RenderingSystem`: Render Graph構築、DrawCall発行 (Indirect Drawing)
- `RenderSyncSystem`: ECSデータをGPUバッファへ転送

### 3.3 Animation (GPU Skinning)
**[Components]**
- `Skeleton`: ボーン階層データ
- `SkinnedMeshRenderer`: ボーンウェイト付きメッシュ
- `Animator`: ステートマシン管理
- `BlendTree`: モーションブレンド設定

**[Systems]**
- `AnimationSystem`: ステート進行管理 (CPU)
- `SkinningSystem`: 今ぴゅーとシェーダーによる頂点変形計算 (GPU)

### 3.4 Physics (Jolt Physics Integration)
**[Components]**
- `BoxCollider`, `SphereCollider`, `CapsuleCollider`, `CylinderCollider`, `ConeCollider`, `MeshCollider`
- `Rigidbody`: Mass, Drag, IsKinematic, GravityScale
- `CharacterController`: StepOffset, SlopeLimit
- `Joint`: Type (Hinge/Ball/Fixed), ConnectedBody
- `PhysicsMaterial`: Friction, Restitution

**[Systems]**
- `PhysicsSystem`: 物理ワールドのシミュレーション実行
- `PhysicsSyncSystem`: ECS Transform <-> Physics Body の同期
- `TriggerEventSystem`: 衝突検知とイベント発行

### 3.5 Audio (Spatial Audio)
**[Components]**
- `AudioSource`: Clip, Valume, Pitch, Loop, SpatialBlend (2D/3D)
- `AudioListener`: リスナー位置 (通常カメラ)
- `ReverbZone`: リバーブプリセット、範囲

**[Systems]**
- `AudioSystem`: 3D座標計算、ミキシング

## 3.6 UI System (Engine Native ECS-UI)
**[Components]**
- `RectTransform`: Anchor, Pivot, Offset, SizeDelta
- `Canvas`: SortOrder, RenderMode (Overlay/Camera)
- `Image`: Sprite, Color, Material
- `Text`: FontAsset(FreeType SDF), String, FontSize, Alignment
- `Button`, `Slider`, `Toggle`: Interactable設定

**[Systems]**
- `UISystem`: レイアウト計算、入力判定、メッシュ生成

### 3.7 AI & Navigation
**[Components]**
- `NavMeshAgent`: Speed, Acceleration, Destination
- `NavMeshObstacle`: Shape, Carve

**[Systems]**
- `NavigationSystem`: パス検索 (A* / FlowField), 回避行動計算

### 3.9 Input System
- **Architecture:** Action-based Input System
- **Features:**
  - Device Layouts (Keyboard, Mouse, Gamepad)
  - Action Maps (Gameplay, UI)
  - Bindings (Physical Key -> Logical Action)

## 4. エディタ　UI/UX 仕様 (Editor Specification)

### 4.1 UI Design Framework
- **Library:** Dear ImGui (Docking Branch), ImGuizmo, ImNode, ImGraph
- **Theme:** **"Unity-like Dark Theme"**
  - Fonts: Roboto / Noto Sans (TTF, Anti-aliased)
  - Icons: FontAwesome
  - Style: Rounded corners, refined padding, dark gray palette.

### 4.2 Implemented Windows (Editor Features)
1.  **Main Viewport:**
  - Game View / Scene View切り替え
  - Gizmos描画 (ImGuizmo採用: Translate/Rotate/Scale)
  - Grid, Debug Draw
2.  **Hierarchy:** Entityツリー表示
  - Entityツリー表示 (Drag & Dropによる親子変更)
3.  **Inspector:**
  - Component編集 (CheckboxでEnable/Disable切り替え)
  - System Config編集
  - Asset Drag & Drop対応
4.  **Project Browser:**
  - ディレクトリツリー & ファイルグリッド
  - **Meta Files:** `.meta` 生成によるGUID管理
  - Asset Import Pipeline
5.  **Console:** ログ出力 (Info/\Warn/Error), ソースコードジャンプ機能
6.  **System Monitor:** SystemごとのCPU時間、Entity処理数のグラフ表示, System Inspectorのようなもの
7.  **Archetype Viewer:** メモリ内のArchetype構成とChunk使用率の可視化
8.  **Shader Graph Editor:** ノードベースシェーダー作成
9.  **Animator Controller:** ノードベースステートマシン編集
10. **Audio Mixer:** バス管理、ボリューム調整
11. **UI Editor:** 2D UI専用編集モード
12. **NavMesh Baker:** ナビメッシュ生成設定
13. **Profiler:** 詳細なパフォーマンス分析 (CPU/GPU/Memory)

## 5. プロジェクト構成 (Project Structure)

### 5.1 フォルダ構成 (Directory Tree)
```text
Root/
┣━━ Bin/                      # ビルド後の実行ファイル (Editor.exe, Game.exe)
┣━━ Build/                    # CMake中間ファイル (Git無視)
┣━━ Tools/                    # 開発用ツール
┃    ┗━━ ReflectionParser/    # Python script for code generation
┃
┣━━ Engine/                   # [CORE] ゲームエンジン本体
┃    ┣━━ Source/
┃    ┃    ┣━━ Core/           # Memory, Math, Container, Log
┃    ┃    ┣━━ Runtime/        # ECS, Physics, Render, Audio, Script systems
┃    ┃    ┣━━ Editor/         # ImGui wrapper, Window implementations
┃    ┃    ┗━━ Developer/      # Tests, Profiler
┃    ┣━━ ThirdParty/          # JoltPhysics, ImGui, FreeType, DXC, etc...
┃    ┗━━ Shaders/             # HLSL Source codes
┃
┗━━ Projects/                 # [USER] ユーザープロジェクト領域
    ┣━━ MyGameProject/        # ゲームごとのルート
    ┃    ┣━━ Assets/          # Raw assets (png, fbx, wav)
    ┃    ┣━━ Source/          # User Components & Systems (C++)
    ┃    ┣━━ Config/          # Engine/Input Settings
    ┃    ┣━━ Cache/           # Imported assets (internal binary format)
    ┗━━ ...
```

## 6. その他要修正箇所

### 6.1 Inspector
- [ ] static: 現在はダミーになっている。
- [ ] tag: 仮実装なのでTagManagerを実装する必要がある。
- [ ] layer: 仮実装なのでLayerManagerを実装する必要がある。
- [ ] AddComponent: 仮実装なので実装する。
- [ ] Active: 現在アクティブを切り替えても反映されていないので実装が必要。

### 6.2 Hierarchy
- [ ] Create **: 物理挙動が出来てから様々な図形を反映させる。(Box, Sphere, Capsule, Cone, Cylinder, etc...)
- [ ] 検索機能: 上部に名前を検索できるバーを表示、一部一致、大文字小文字が違くても候補に出す高度な検索機能。
- [ ] アイコン表示: 入っているコンポーネントによってアイコンを描画する。また、エンティティがどのようなオブジェクトか示すアイコン。
- [ ] Have Script: ユーザー作成のコンポーネントが入っていると何かわかるように表示する。
- [ ] 階層を示す色付け: エンティティがどの階層にいるかわかるようにわかるように色を付ける。
- [ ] Tag & Layer: EntityがどのTagとLayerに属しているかを表示する。 (Entityの行に2行に小さいfont sizeで表示)
- [ ] Prefab: Prefabか否かを文字色で示す。
- [ ] 編集のロック: Inspectorなどでコンポーネントの値を編集できないようにする。それを視覚的に表示する。
- [ ] 罫線表示: 階層を罫線で表示する。
- [ ] static の切り替え: entityがstaticかそうでないかを切り替えれる。
- [ ] オブジェクトを色分けできるようにする。
- [ ] 子のオブジェクトの数を表示。
- [ ] これらの機能をオンオフしたり詳細に設定できるもの。

### 6.0 その他
- [ ] #include <SpanEngine>のようなものを実装。SpanEngineの機能をインクルード一発で使えるように実装。
- [ ] エディターカメラがHierarchyに表示されてしまっている。
- [ ] エディターカメラの操作とQWERが混同している。
- [ ] ドッキングが上手くいっていない。
- [ ] ギズモが操作出来ない。
- [ ] シーンギズモの修正。
- [ ] シーンビューのアスペクト比は変わるが、引き延ばされるようになる。
