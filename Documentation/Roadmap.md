# 🗺️ SpanEngine Development Roadmap

## 🏁 Phase 1: エディタ基盤とデータ永続化 (Foundation & Serialization)
**目標:** 「アセット管理・設定・保存・復帰」の完全なサイクルを確立し、エディタとしての信頼性を確保する。

### 1. Project Browser Polish (Current Focus)
- [x] **Asset Inspector:**
  - Project Browserでファイルを選択した際、Inspectorパネルにその詳細情報を表示する。
  - テクスチャ解像度、モデルの頂点数、GUIDなどの確認。
- [x] **Directory Watcher (Hot Reload):**
  - エクスプローラー側でのファイル追加・削除・変更をリアルタイムに検知し、Project Browserを自動更新する。

### 2. Global Managers (Tag & Layer)
- [x] **Tag Manager:**
  - ゲーム内で使用するタグ（Player, Enemy, UIなど）を文字列リストとして一元管理するシステム。
  - Inspectorでプルダウン選択できるようにする。
- [x] **Layer Manager:**
  - 物理衝突やレンダリング可視性を制御するためのレイヤー定義（Default, Transparent, UI, Waterなど）。
  - 衝突マトリクス（どのレイヤーとどのレイヤーが衝突するか）の設定UI。

### 3. Scene Serialization (Scene System)
- [ ] **Scene Format (.span):**
  - 現在のメモリ内Entityリストだけでなく、**「シーン全体の設定」**をJSON/YAML形式で保存・読み込み可能にする。
  - **保存対象:**
    - **Entity Graph:** 全EntityとComponentのパラメータ、親子関係。
    - **Scene Settings:** 環境光設定、スカイボックス参照、Ambient Light設定。
    - **Definitions:** そのシーンで使用されているローカルなレイヤー設定やタグ情報。
    - **Lightmap References:** ベイクされたライトマップへの参照データ。
- [ ] **Scene Hierarchy Panel:**
  - シーン間移動、複数シーンの加算ロード（Additive Load）への対応準備。

### 4. Material Asset System
- [ ] **Material Editor (.mat):**
  - コードでの生成から脱却し、アセットとしてマテリアルを定義する。
  - **保存データ:**
    - **Shader Reference:** 使用するシェーダーパス。
    - **Texture Slots:** Albedo, Normal, Roughness, Metallic, AO などのテクスチャ参照(GUID)。
    - **Parameters:** 色(Color), スカラー値(Float), ベクトル値(Vector4) のパラメータ群。
    - **Render States:** カリングモード(Front/Back/None)、ブレンドモード(Opaque/Transparent)、深度テスト設定。
  - Inspector上で`.mat`ファイルを編集し、MeshRendererにドラッグ＆ドロップで即座に反映させる。

---

## 🎮 Phase 2: ゲームプレイ機能の実装 (Gameplay Foundation)
**目標:** 「静的なシーン」を「インタラクティブなゲーム」に変えるための動的な仕組みを導入する。

### 1. Physics Integration
- [ ] **Physics Engine:** PhysX または Jolt Physics の統合。
- [ ] **Components:**
  - `RigidBody`: 物理挙動（重力、速度）の付与。
  - `Colliders`: Box, Sphere, Capsule, MeshCollider の実装。
- [ ] **Simulation:** FixedUpdateループの実装と、物理世界のシミュレーション同期。

### 2. Scripting System
- [ ] **Native Scripting (C++ Hot Reload):**
  - ゲームロジックをDLLとして分離し、エディタを再起動せずにロジックを更新・リロードする仕組み。
- [ ] **Managed Scripting (C# - Optional):**
  - Mono または .NET Core をホスティングし、Unityライクなスクリプト環境を提供する。

### 3. Input System
- [ ] **Input Action Mapping:**
  - キーボードやゲームパッドの生の入力を、「Jump」「Fire」などの抽象的なアクションにマッピングするシステム。

---

## 🎨 Phase 3: グラフィックス品質と製品化 (Graphics & Polish)
**目標:** 商用エンジンに比肩するレンダリング品質と、ゲームとしての出力機能を備える。

### 1. Advanced Lighting System
- [ ] **Dynamic Lights:**
  - **Directional Light:** 太陽光シミュレーション。CSM (Cascaded Shadow Maps) による広範囲シャドウ。
  - **Point Light:** 全方位光源。減衰（Attenuation）とシャドウマップのサポート。
  - **Spot Light:** 円錐状光源。角度、範囲、ソフトエッジの設定。
  - **Area Light:** 矩形光源（リアルタイム近似またはベイク用）。
- [ ] **Light Baking (Global Illumination):**
  - **Lightmapper:** 静的オブジェクトのライティングを計算し、ライトマップテクスチャに焼き付ける機能（UV2展開含む）。
  - **Light Probes:** ライトマップを持たない動的オブジェクトに対して、ベイクされた間接光情報を適用する仕組み。
  - **Reflection Probes:** 周囲の環境をキャプチャし、金属表面の反射をリアルにする。

### 2. Advanced Rendering (PBR & PostFx)
- [ ] **PBR Pipeline:** 完全な物理ベースレンダリング（Albedo, Normal, Metallic, Roughness, AO）の実装。
- [ ] **Image Based Lighting (IBL):** HDRIスカイボックスからの環境光サンプリング。
- [ ] **Post Processing:** Bloom, Tone Mapping (ACES), Color Grading, Vignette, Depth of Field。

### 3. Build System
- [ ] **Standalone Build:**
  - エディタ機能を除外し、ゲーム実行に必要なランタイムとアセットのみをパッケージ化した `.exe` を出力する機能。
