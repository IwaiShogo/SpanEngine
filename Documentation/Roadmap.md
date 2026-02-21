# 🗺️ SpanEngine Development Roadmap

## 🏁 Phase 1: エディタ基盤とデータ永続化 (Foundation & Serialization)
**目標:** 「アセット管理・設定・保存・復帰」の完全なサイクルを確立し、エディタとしての信頼性を確保する。

### 1. Project Browser & Asset System
- [x] **Asset Inspector:** アセットの詳細情報（GUID、設定など）の表示。
- [x] **Directory Watcher (Hot Reload):** 外部でのファイル変更を検知し自動更新。
- [x] **Asset Manager:** キャッシュ機構、GUIDベースの参照解決の実装。

### 2. Global Managers (Tag & Layer)
- [x] **Tag Manager:** タグの文字列管理とInspectorでの割り当て。
- [x] **Layer Manager:** 物理・描画用のレイヤー定義と衝突マトリクスUI。

### 3. Material Asset System
- [x] **Material Editor (.mat):** アセットとしてのマテリアル定義。
- [x] **PBR Shader Foundation:** HLSLでのCook-Torrance BRDF、6テクスチャスロット対応。
- [ ] **Asset Preview Pipeline:** Inspector内でマテリアルやモデルの球体/サムネイルをリアルタイム描画するオフスクリーンレンダリング。

---

## 💡 Phase 2: 高度なライティングと環境構築 (Lighting & Environment)
**目標:** PBRマテリアルのポテンシャルを100%引き出し、シーンの「画作り」を完成させる。

### 1. Environment & IBL (Image-Based Lighting)
- [ ] **Skybox System:** HDRIテクスチャを用いたスカイボックスの描画。
- [ ] **IBL Pipeline:** - 環境マップから Irradiance Map (拡散反射) と Pre-filter Map (鏡面反射)、BRDF LUTの事前計算(コンピュートシェーダー)。
  - PBRシェーダーに環境光を統合し、金属(Metallic)の完全な反射を実現。

### 2. Dynamic Lighting System
- [ ] **Light Components:**
  - `DirectionalLight`: 太陽光。CSM (Cascaded Shadow Maps) による広範囲シャドウ。
  - `PointLight`: 全方位光源。距離減衰とOmnidirectional Shadow Map。
  - `SpotLight`: 円錐状光源。角度、減衰設定。
- [ ] **Forward+/Deferred Rendering:** 多数のライトを効率的に処理するためのレンダリングパスの構築。

---

## 📦 Phase 3: シーン管理と世界構築 (Scene Management)
**目標:** 作成した環境とエンティティ群を「一つの世界」として完全に保存・復元・遷移させる。

### 1. Advanced Scene Serialization
- [ ] **Scene Format (.span) の完全化:** 以下の要素を全てJSON/YAMLでシリアライズする。
  - **[Meta]**: フォーマットバージョン、シーン名、GUID。
  - **[Environment Settings]**: 
    - SkyboxマテリアルGUID、Ambient Color設定、Global Fog（霧）設定。
  - **[Render Settings]**: 
    - メインカメラのEntity GUID、Post-Processing Profile GUID。
  - **[Entities]**: 
    - 全EntityのID、親子関係(Relationship)、全Componentデータ（LightやCamera設定含む）。
  - **[Definitions]**:
    - そのシーンローカルで使用するタグ・レイヤー設定（※将来的にProject Settingsへ移行も検討）。

### 2. Scene Management System
- [ ] **Scene Runtime Operations:**
  - `LoadScene(path)`: 現在のシーンを破棄し、新しいシーンをロードする。
  - `LoadSceneAdditive(path)`: 現在のシーンを維持したまま、別シーンを重ねてロードする（オープンワールドやUIオーバーレイ用）。
- [ ] **Scene Hierarchy Panel Polish:**
  - エンティティのドラッグ＆ドロップによる親子関係構築。
  - シーン間のエンティティ移動。

---

## 🎮 Phase 4: ゲームプレイ機能の実装 (Gameplay Foundation)
**目標:** 「静的なシーン」を「インタラクティブなゲーム」に変えるための動的な仕組みを導入する。

### 1. Physics Integration
- [ ] **Physics Engine:** PhysX または Jolt Physics の統合。
- [ ] **Components:** `RigidBody` (重力、速度)、`BoxCollider`, `SphereCollider`, `MeshCollider`。
- [ ] **Simulation:** FixedUpdateループの実装と、物理世界のTransform同期。

### 2. Scripting System
- [ ] **Native Scripting (C++ Hot Reload):** ゲームロジックをDLLとして分離し、エディタを再起動せずにロジックを更新。
- [ ] **Managed Scripting (C# - Optional):** Mono または .NET Core をホスティング。

### 3. Input System
- [ ] **Input Action Mapping:** キーボードやパッドの生入力を「Jump」「Fire」などのアクションにマッピングするシステム。

---

## 🎬 Phase 5: グラフィックス品質と製品化 (Graphics & Polish)
**目標:** 商用エンジンに比肩する最終品質と、ゲームとしての出力機能を備える。

### 1. Post Processing Stack
- [ ] **Effects:** Bloom, Tone Mapping (ACES), Color Grading, Vignette, Depth of Field。
- [ ] **PostProcess Volume:** シーンの特定領域に入るとポストエフェクトが滑らかに切り替わる仕組み。

### 2. Build System
- [ ] **Standalone Build:**
  - エディタ機能を除外し、ゲーム実行に必要なランタイムとアセット（パック化）のみを `.exe` として出力するパッケージング機能。
