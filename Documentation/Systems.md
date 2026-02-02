# ⚙️ Systems Specification

システムは状態を持たず、Componentデータを読み書きして振る舞いを決定します。

## 1. Core Systems

### `RelationshipSystem`
親子関係の整合性を保つシステム。
- **Source:** `Engine/Source/Runtime/Systems/Core/RelationshipSystem.h`
- **Responsibility:**
  - `Disconnect()`: エンティティを親から切り離し、兄弟リンクを修復する。
  - `SetParent()`: 親子付け替えを行い、リンクリストを更新する。
  - `InsertBefore()`: 兄弟順序の並び替えを行う。

### `TransformSystem`
階層構造に従って座標変換行列を計算する。
- **Source:** `Engine/Source/Runtime/Systems/Core/TransformSystem.h` (Planned)
- **Logic:**
  1. 親を持たないルートエンティティの `LocalToWorld` を計算。
  2. 階層を下りながら、`Parent.LocalToWorld * Self.Transform` を計算。
  3. 並列処理（Parallel For）による最適化を行う。

---

## 2. Graphics Systems

### `CameraSystem`
カメラパラメータをレンダラーに転送する。
- **Source:** `Engine/Source/Runtime/Systems/Graphics/CameraSystem.h`
- **Dependencies:** `Camera`, `LocalToWorld`
- **Logic:**
  1. `LocalToWorld` から View行列（逆行列）を計算。
  2. ウィンドウのアスペクト比と `Camera` 設定から Projection行列を計算。
  3. `Renderer::SetCamera()` を呼び出し、GPUの定数バッファを更新。

### `RenderingSystem`
描画コマンドの発行を行う。
- **Source:** `Engine/Source/Runtime/Systems/Graphics/RenderingSystem.h`
- **Dependencies:** `MeshFilter`, `MeshRenderer`, `LocalToWorld`
- **Render Pass:**
  1. **Opaque Pass (不透明):**
     - マテリアルが `IsTransparent() == false` のオブジェクトを描画。
     - 深度バッファへの書き込みを行う。
  2. **Transparent Pass (透明):**
     - マテリアルが `IsTransparent() == true` のオブジェクトを描画。
     - 深度テストは行うが、深度書き込みは行わない（Blend有効）。

---

## 3. Input Systems

### `Input` (Static Class)
ハードウェア入力を管理する静的クラス。
- **Source:** `Engine/Source/Core/Input/Input.h`
- **Features:**
  - `GetKey()`, `GetKeyDown()`, `GetKeyUp()`: キーボード状態取得。
  - `GetMousePosition()`, `GetMouseDelta()`: マウス座標・移動量。
  - `GetAxis()`: ゲームパッドのアナログ入力。
  - `SetLockCursor()`: FPS視点用にカーソルをロック・非表示にする機能。