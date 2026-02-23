# 🗺️ SpanEngine Development Roadmap

## 🏁 Phase 1: エディタ基盤とデータ永続化 (Foundation & Serialization) - [COMPLETED]
**目標:** 「アセット管理・設定・保存・復帰」の完全なサイクルを確立し、エディタとしての信頼性を確保する。

### 1. Project Browser & Asset System
- [x] **Asset Inspector:** アセットの詳細情報の表示。
- [x] **Directory Watcher (Hot Reload):** 外部でのファイル変更を検知し自動更新。
- [x] **Asset Manager:** キャッシュ機構、GUIDベースの参照解決の実装。

### 2. Global Managers (Tag & Layer)
- [x] **Tag Manager:** タグの文字列管理とInspectorでの割り当て。
- [x] **Layer Manager:** 物理・描画用のレイヤー定義と衝突マトリクスUI。

### 3. Material Asset System
- [x] **Material Editor (.mat):** アセットとしてのマテリアル定義。
- [x] **PBR Shader Foundation:** HLSLでのCook-Torrance BRDF、6テクスチャスロット対応。

---

## 💡 Phase 2: 高度なライティングと極限の最適化 (Lighting & Rendering) - [IN PROGRESS]
**目標:** 最先端の技術(Forward+)を用い、「無数の光と影があっても軽く、息を呑むほど美しい」レンダリングパイプラインを構築する。

### 1. Dynamic Lighting & Shadows
- [x] **Light Components:** Directional, Point, Spot の基礎実装。
- [x] **Shadow Mapping:** Directional / Spot Light の影 (Texture2DArray)。
- [x] **Omnidirectional Shadows:** Point Light の影 (TextureCube)。
- [ ] **Forward+ Shading:** Compute Shaderを用いたライトのカリング。数千のライトを配置しても処理落ちしない極限の最適化。
- [ ] **Area Lights (LTC):** 面積を持った光源（蛍光灯や窓ガラス）の正確なPBR計算。

### 2. Environment & Advanced Illumination
- [x] **Procedural Skybox & Environment Setup:** 環境光の動的変更とPBRへの反映。
- [ ] **HDRI Image-Based Lighting (IBL):** パノラマ画像からIrradiance/Prefilter Mapを生成し、現実の景色を金属やガラスに畳み込む。
- [ ] **SSAO (Screen Space Ambient Occlusion):** 画面空間でのリアルな陰影の追加。
- [ ] **GPU Lightmapper (DXR):** DirectX Raytracingを用いた、超高速・高性能なGI（グローバルイルミネーション）の事前計算ベイク。

---

## 🛠️ Phase 3: エディタUXの極み (Ultimate Editor UX)
**目標:** 開発者が「触っていて気持ちいい」と感じる、Unity/Unrealに匹敵するエディタ体験の構築。

### 1. Editor State & UI Polish
- [ ] **Edit / Play / Pause Mode:** エディタ内でのゲーム実行状態の完全な分離と、状態の保存・復元。
- [ ] **Perfect Menu Bar:** File, Edit, GameObject, Component, Window などの洗練されたメニュー構造。
- [ ] **Advanced Inspector:** カスタムプロパティドロワー、より詳細で直感的なUI（スライダー、折りたたみ等）。
- [ ] **Undo / Redo System:** Command Patternを用いた、Inspectorやシーン変更の完全な履歴管理。

### 2. Scene View & Object Creation
- [ ] **Gizmos & Billboards:** カメラやライトのアイコン（ビルボード）をシーンビューに描画。
- [ ] **Light Range Gizmos:** ライトの到達範囲（Range, Cone Angle）をワイヤーフレームで視覚化。
- [ ] **Entity Builder:** `Create Empty` 時のアーキテクチャのクリーン化。
- [ ] **Create Menu Categories:** `Create -> 3D Object -> Cube`, `Light -> Spot Light` のように、最初から適切なコンポーネントを持ったエンティティを生成。
- [ ] **Component Filter:** `Editor Camera` などのエディタ専用コンポーネントを `Add Component` メニューから隠す（属性による制御）。

---

## 📦 Phase 4: シーン管理と世界構築 (Scene Management)
**目標:** 作成した環境とエンティティ群を「一つの世界」として完全に保存・復元・遷移させる。

### 1. Advanced Scene Serialization
- [ ] **Scene Format (.span) の完全化:** Meta, Environment, Entities の完全なJSON/YAMLシリアライズ。
- [ ] **Environment Settings Polish:** 露出、フォグ、IBLアセット参照などをより詳細にシーン単位で保存。

### 2. Scene Management System
- [ ] **Runtime Scene API:** `LoadScene`, `LoadSceneAdditive` による動的ロード。
- [ ] **Hierarchy Panel Polish:** ドラッグ＆ドロップによる親子関係(Transform Hierarchy)の構築と維持。

---

## 🎮 Phase 5: ゲームプレイ機能の実装 (Gameplay Foundation)
**目標:** 「静的なシーン」を「インタラクティブなゲーム」に変えるための動的な仕組みを導入する。

### 1. Physics Integration
- [ ] **Physics Engine:** Jolt Physics (最新の軽量物理エンジン) の統合。
- [ ] **Components:** `RigidBody`, `BoxCollider`, `SphereCollider`, `MeshCollider`。

### 2. Scripting & Input System
- [ ] **Native Scripting (C++ Hot Reload):** ゲームロジックをDLLとして分離し、エディタを再起動せずに更新。
- [ ] **Input Action Mapping:** キー・パッド入力をアクション名に抽象化する最新のインプットシステム。

---

## 🎬 Phase 6: グラフィックス品質と製品化 (Post Processing & Build)
**目標:** 商用エンジンに比肩する最終品質と、ゲームとしての出力機能を備える。

### 1. Post Processing Stack
- [ ] **Effects:** Bloom, ACES Tone Mapping, Color Grading, Vignette, Depth of Field。

### 2. Build System
- [ ] **Standalone Build:** エディタ機能を除外し、ゲーム実行に必要なランタイムとアセットのみを `.exe` として出力するパッケージング機能。
