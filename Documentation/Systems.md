# ⚙️ Systems Specification

システムは状態を持たず、Componentデータを読み書きして振る舞いを決定します。

## 1. Core Systems (Implemented)

### `RelationshipSystem`
親子関係の整合性を保つシステム。
- **Responsibility:** `Disconnect`, `SetParent`, `InsertBefore` の処理。

### `TransformSystem` (Implemented)
階層構造に従って座標変換行列を計算する。
- **Logic:**
  1. ルートエンティティの計算。
  2. `Parent.LocalToWorld * Self.Transform` の階層計算。
  3. 並列処理による高速化。

---

## 2. Graphics Systems (Implemented)

### `CameraSystem`
カメラパラメータをレンダラーに転送する。
- **Logic:** View行列とProjection行列を計算し、`Renderer::SetCamera`へ送る。

### `RenderingSystem`
描画コマンドの発行を行う。
- **Logic:**
  1. **Opaque Pass:** 不透明オブジェクト描画（深度書き込み）。
  2. **Transparent Pass:** 透明オブジェクト描画（深度テストのみ）。

---

## 3. Graphics Systems (Planned) 🚧

### `CullingSystem`
描画不要なオブジェクトを事前に除外する。
- **Logic:**
  - **Frustum Culling:** カメラ視錐台に入っていないオブジェクトを除外。
  - **Occlusion Culling:** 他の物体に隠れているオブジェクトを除外。

### `LODSystem`
- **Logic:** カメラとオブジェクトの距離を計算し、`LODGroup`の設定に基づいて適切なメッシュを選択する。

### `RenderSyncSystem`
- **Logic:** ECS上のTransformやMaterialデータを、GPU（Constant Buffer / Structured Buffer）へ一括転送する。

---

## 4. Physics Systems (Planned) 🚧

### `PhysicsSystem`
物理エンジンのシミュレーションステップを進める。
- **Library:** Jolt Physics
- **Logic:** `Simulate(dt)` を呼び出し、衝突判定と解決を行う。

### `PhysicsSyncSystem`
ECSと物理エンジンの同期を行う。
- **Pre-Sim:** ECSの `Transform` → Physicsの `BodyPosition` へコピー。
- **Post-Sim:** Physicsの `BodyPosition` → ECSの `Transform` へコピー。

### `TriggerEventSystem`
- **Logic:** 物理エンジンからの衝突コールバックを受け取り、`OnTriggerEnter` などのイベントを発行する。

---

## 5. Audio & Animation Systems (Planned) 🚧

### `AudioSystem`
- **Logic:** 3Dサウンドの空間計算（距離減衰、ドップラー効果）を行い、オーディオバッファを更新する。

### `AnimationSystem`
- **Logic:** アニメーションステートマシンを更新し、現在のポーズ時間を決定する。

### `SkinningSystem`
- **Logic:** `AnimationSystem`の結果に基づき、スキニング行列を計算する（Compute Shader推奨）。

---

## 6. AI & Navigation Systems (Planned) 🚧

### `NavigationSystem`
- **Logic:** `NavMeshAgent` の目的地までのパスをA*アルゴリズム等で計算し、次のフレームの移動ベクトルを算出する。